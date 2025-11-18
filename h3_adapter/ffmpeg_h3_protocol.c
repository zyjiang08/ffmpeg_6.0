/*
 * QUIC/HTTP3 protocol via QuicheEngine
 * Copyright (c) 2025
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/**
 * @file
 * QUIC/HTTP3 protocol implementation using QuicheEngine
 *
 * This protocol handler enables FFmpeg to download media over HTTP/3 using QUIC.
 * It bridges the asynchronous QuicheEngine API with FFmpeg's synchronous I/O model
 * using a producer-consumer pattern with a ring buffer.
 */

#include "avformat.h"
#include "avio.h"
#include "url.h"
#include "libavutil/opt.h"
#include "libavutil/avstring.h"
#include "libavutil/log.h"

#include <quiche_engine_c_api.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>

#define RING_BUFFER_SIZE (4 * 1024 * 1024)  // 4 MB ring buffer

typedef struct QuicheContext {
    const AVClass *class;
    QuicheEngineHandle *quiche_handle;

    // Ring buffer for producer-consumer data flow
    uint8_t *buffer;
    volatile int write_pos;  // Modified by network thread
    volatile int read_pos;   // Modified by FFmpeg thread
    int buffer_size;

    // State flags
    volatile int eof_reached;
    volatile int error_occurred;
    int error_code;

    // Synchronization primitives
    pthread_mutex_t mutex;
    pthread_cond_t data_available_cond;   // Signaled when data is written
    pthread_cond_t space_available_cond;  // Signaled when data is read

    // Connection state
    volatile int connected;
    volatile int connection_failed;

    // Statistics
    int64_t bytes_received;
    int64_t bytes_read;
} QuicheContext;

// ============================================================================
// Ring Buffer Utilities
// ============================================================================

static int ring_buffer_available_data(QuicheContext *h)
{
    return h->write_pos - h->read_pos;
}

static int ring_buffer_available_space(QuicheContext *h)
{
    return h->buffer_size - ring_buffer_available_data(h);
}

static int ring_buffer_write(QuicheContext *h, const uint8_t *data, int size)
{
    if (ring_buffer_available_space(h) < size) {
        return -1; // Buffer full
    }

    int write_idx = h->write_pos % h->buffer_size;
    int tail_space = h->buffer_size - write_idx;

    if (tail_space >= size) {
        // Contiguous write
        memcpy(h->buffer + write_idx, data, size);
    } else {
        // Wrap-around write
        memcpy(h->buffer + write_idx, data, tail_space);
        memcpy(h->buffer, data + tail_space, size - tail_space);
    }

    h->write_pos += size;
    return size;
}

static int ring_buffer_read(QuicheContext *h, uint8_t *data, int size)
{
    int available = ring_buffer_available_data(h);
    if (available == 0) {
        return 0; // Buffer empty
    }

    int to_read = FFMIN(size, available);
    int read_idx = h->read_pos % h->buffer_size;
    int tail_space = h->buffer_size - read_idx;

    if (tail_space >= to_read) {
        // Contiguous read
        memcpy(data, h->buffer + read_idx, to_read);
    } else {
        // Wrap-around read
        memcpy(data, h->buffer + read_idx, tail_space);
        memcpy(data + tail_space, h->buffer, to_read - tail_space);
    }

    h->read_pos += to_read;
    return to_read;
}

// ============================================================================
// QuicheEngine Callbacks (called from network thread)
// ============================================================================

static void on_connect_cb(void *opaque, const char *conn_id, const char *error)
{
    QuicheContext *h = (QuicheContext *)opaque;

    pthread_mutex_lock(&h->mutex);

    if (error) {
        av_log(h, AV_LOG_ERROR, "QUIC connection failed: %s\n", error);
        h->connection_failed = 1;
        h->error_occurred = 1;
        h->error_code = AVERROR_EXTERNAL;
    } else {
        av_log(h, AV_LOG_INFO, "QUIC connected: %s\n", conn_id ? conn_id : "unknown");
        h->connected = 1;
    }

    pthread_cond_broadcast(&h->data_available_cond);
    pthread_mutex_unlock(&h->mutex);
}

static void on_data_cb(void *opaque, const uint8_t *data, size_t len, int is_fin)
{
    QuicheContext *h = (QuicheContext *)opaque;

    pthread_mutex_lock(&h->mutex);

    // Wait for space in the ring buffer
    while (ring_buffer_available_space(h) < (int)len && !h->error_occurred) {
        av_log(h, AV_LOG_DEBUG, "Ring buffer full, waiting for space...\n");
        pthread_cond_wait(&h->space_available_cond, &h->mutex);
    }

    if (h->error_occurred) {
        pthread_mutex_unlock(&h->mutex);
        return;
    }

    // Write data to ring buffer
    int written = ring_buffer_write(h, data, (int)len);
    if (written < 0) {
        av_log(h, AV_LOG_ERROR, "Failed to write to ring buffer\n");
        h->error_occurred = 1;
        h->error_code = AVERROR(ENOMEM);
    } else {
        h->bytes_received += written;
        av_log(h, AV_LOG_DEBUG, "Received %d bytes (total: %"PRId64")\n",
               written, h->bytes_received);
    }

    if (is_fin) {
        av_log(h, AV_LOG_INFO, "Received FIN, download complete\n");
        h->eof_reached = 1;
    }

    pthread_cond_signal(&h->data_available_cond);
    pthread_mutex_unlock(&h->mutex);
}

static void on_finish_cb(void *opaque, const char *error)
{
    QuicheContext *h = (QuicheContext *)opaque;

    pthread_mutex_lock(&h->mutex);

    if (error) {
        av_log(h, AV_LOG_ERROR, "QUIC connection finished with error: %s\n", error);
        h->error_occurred = 1;
        h->error_code = AVERROR_EXTERNAL;
    } else {
        av_log(h, AV_LOG_INFO, "QUIC connection finished successfully\n");
        h->eof_reached = 1;
    }

    pthread_cond_broadcast(&h->data_available_cond);
    pthread_mutex_unlock(&h->mutex);
}

// ============================================================================
// FFmpeg URLProtocol Implementation
// ============================================================================

static int quiche_open(URLContext *uc, const char *url, int flags)
{
    QuicheContext *h = uc->priv_data;
    char hostname[256], path[1024];
    char portstr[10];
    int port;

    av_log(uc, AV_LOG_INFO, "Opening QUIC connection to: %s\n", url);

    // Parse URL: h3://hostname:port/path
    av_url_split(NULL, 0, NULL, 0, hostname, sizeof(hostname),
                 &port, path, sizeof(path), url);

    if (port <= 0) {
        port = 443; // Default HTTPS port
    }
    snprintf(portstr, sizeof(portstr), "%d", port);

    // Allocate ring buffer
    h->buffer_size = RING_BUFFER_SIZE;
    h->buffer = av_malloc(h->buffer_size);
    if (!h->buffer) {
        av_log(uc, AV_LOG_ERROR, "Failed to allocate ring buffer\n");
        return AVERROR(ENOMEM);
    }

    // Initialize state
    h->write_pos = 0;
    h->read_pos = 0;
    h->eof_reached = 0;
    h->error_occurred = 0;
    h->error_code = 0;
    h->connected = 0;
    h->connection_failed = 0;
    h->bytes_received = 0;
    h->bytes_read = 0;

    // Initialize synchronization primitives
    pthread_mutex_init(&h->mutex, NULL);
    pthread_cond_init(&h->data_available_cond, NULL);
    pthread_cond_init(&h->space_available_cond, NULL);

    // Create QuicheEngine handle
    h->quiche_handle = quiche_engine_create();
    if (!h->quiche_handle) {
        av_log(uc, AV_LOG_ERROR, "Failed to create QuicheEngine\n");
        av_free(h->buffer);
        return AVERROR(ENOMEM);
    }

    // Start asynchronous connection
    int ret = quiche_engine_connect_async(
        h->quiche_handle,
        hostname,
        path[0] ? path : "/",
        hostname,  // Use hostname as IP for now (DNS resolution in engine)
        portstr,
        on_connect_cb,
        on_data_cb,
        on_finish_cb,
        h
    );

    if (ret < 0) {
        av_log(uc, AV_LOG_ERROR, "Failed to start QUIC connection\n");
        quiche_engine_destroy(h->quiche_handle);
        av_free(h->buffer);
        return AVERROR_EXTERNAL;
    }

    // Wait for connection establishment or failure
    pthread_mutex_lock(&h->mutex);
    while (!h->connected && !h->connection_failed && !h->error_occurred) {
        av_log(uc, AV_LOG_DEBUG, "Waiting for connection...\n");
        pthread_cond_wait(&h->data_available_cond, &h->mutex);
    }
    int final_error = h->error_code;
    pthread_mutex_unlock(&h->mutex);

    if (final_error) {
        av_log(uc, AV_LOG_ERROR, "Connection failed with error code: %d\n", final_error);
        quiche_engine_close(h->quiche_handle);
        quiche_engine_destroy(h->quiche_handle);
        av_free(h->buffer);
        return final_error;
    }

    av_log(uc, AV_LOG_INFO, "QUIC connection established successfully\n");
    return 0;
}

static int quiche_read(URLContext *uc, unsigned char *buf, int size)
{
    QuicheContext *h = uc->priv_data;
    int bytes_read = 0;

    pthread_mutex_lock(&h->mutex);

    // Wait for data to become available
    while (ring_buffer_available_data(h) == 0 &&
           !h->eof_reached &&
           !h->error_occurred) {
        av_log(uc, AV_LOG_DEBUG, "Waiting for data...\n");
        pthread_cond_wait(&h->data_available_cond, &h->mutex);
    }

    // Check for errors
    if (h->error_occurred) {
        int err = h->error_code;
        pthread_mutex_unlock(&h->mutex);
        av_log(uc, AV_LOG_ERROR, "Read error: %d\n", err);
        return err;
    }

    // Read available data
    int available = ring_buffer_available_data(h);
    if (available > 0) {
        bytes_read = ring_buffer_read(h, buf, size);
        h->bytes_read += bytes_read;
        av_log(uc, AV_LOG_DEBUG, "Read %d bytes (total: %"PRId64")\n",
               bytes_read, h->bytes_read);

        // Signal network thread that space is available
        pthread_cond_signal(&h->space_available_cond);
    } else if (h->eof_reached) {
        // No more data and EOF reached
        bytes_read = AVERROR_EOF;
        av_log(uc, AV_LOG_INFO, "Reached EOF\n");
    }

    pthread_mutex_unlock(&h->mutex);

    return bytes_read;
}

static int quiche_close(URLContext *uc)
{
    QuicheContext *h = uc->priv_data;

    av_log(uc, AV_LOG_INFO, "Closing QUIC connection\n");
    av_log(uc, AV_LOG_INFO, "Statistics: Received %"PRId64" bytes, Read %"PRId64" bytes\n",
           h->bytes_received, h->bytes_read);

    // Signal shutdown and clean up engine
    if (h->quiche_handle) {
        quiche_engine_close(h->quiche_handle);
        quiche_engine_destroy(h->quiche_handle);
        h->quiche_handle = NULL;
    }

    // Clean up synchronization primitives
    pthread_mutex_destroy(&h->mutex);
    pthread_cond_destroy(&h->data_available_cond);
    pthread_cond_destroy(&h->space_available_cond);

    // Free ring buffer
    av_free(h->buffer);

    return 0;
}

#define OFFSET(x) offsetof(QuicheContext, x)
#define D AV_OPT_FLAG_DECODING_PARAM

static const AVOption options[] = {
    { NULL }
};

static const AVClass quiche_context_class = {
    .class_name = "h3",
    .item_name  = av_default_item_name,
    .option     = options,
    .version    = LIBAVUTIL_VERSION_INT,
};

const URLProtocol ff_h3_protocol = {
    .name                = "h3",
    .url_open            = quiche_open,
    .url_read            = quiche_read,
    .url_close           = quiche_close,
    .priv_data_size      = sizeof(QuicheContext),
    .priv_data_class     = &quiche_context_class,
    .flags               = URL_PROTOCOL_FLAG_NETWORK,
};
