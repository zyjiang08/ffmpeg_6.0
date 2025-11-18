/*
 * QuicheEngine C API Wrapper for FFmpeg H3 Protocol Integration
 *
 * This file provides a C interface to the C++ QuicheEngine library,
 * allowing FFmpeg (pure C) to use HTTP/3 functionality.
 *
 * Location: quiche/engine/demo/player/ffmpeg/h3_adapter/
 * Dependencies:
 *   - quiche/engine/include/quiche_engine.h
 *   - lib/macos/x86_64/libquicheengine.a
 */

#ifndef QUICHE_ENGINE_C_API_H
#define QUICHE_ENGINE_C_API_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Opaque handle to QuicheEngine instance */
typedef struct QuicheEngineHandle QuicheEngineHandle;

/* Connection states */
typedef enum {
    QUICHE_STATE_IDLE = 0,
    QUICHE_STATE_CONNECTING,
    QUICHE_STATE_CONNECTED,
    QUICHE_STATE_DOWNLOADING,
    QUICHE_STATE_FINISHED,
    QUICHE_STATE_ERROR
} QuicheState;

/* Callbacks */

/**
 * Connection callback
 * @param opaque User data pointer
 * @param conn_id Connection ID string (NULL if failed)
 * @param error Error message (NULL if success)
 */
typedef void (*quiche_connect_callback_t)(
    void* opaque,
    const char* conn_id,
    const char* error
);

/**
 * Data callback - called when data is received
 * @param opaque User data pointer
 * @param data Received data buffer
 * @param len Data length in bytes
 * @param is_fin 1 if this is the last data, 0 otherwise
 */
typedef void (*quiche_data_callback_t)(
    void* opaque,
    const uint8_t* data,
    size_t len,
    int is_fin
);

/**
 * Finish callback - called when download completes or error occurs
 * @param opaque User data pointer
 * @param error Error message (NULL if success)
 */
typedef void (*quiche_finish_callback_t)(
    void* opaque,
    const char* error
);

/* API Functions */

/**
 * Create a new QuicheEngine instance
 * @return Handle to QuicheEngine, or NULL on failure
 */
QuicheEngineHandle* quiche_engine_create(void);

/**
 * Destroy QuicheEngine instance and free resources
 * @param handle QuicheEngine handle
 */
void quiche_engine_destroy(QuicheEngineHandle* handle);

/**
 * Start an asynchronous HTTP/3 connection and download
 *
 * @param handle QuicheEngine handle
 * @param hostname Server hostname (e.g., "example.com")
 * @param uri Request URI (e.g., "/video.mp4")
 * @param ip Server IP address (e.g., "1.2.3.4")
 * @param port Server port (e.g., "443")
 * @param on_connect Callback when connection established
 * @param on_data Callback when data received
 * @param on_finish Callback when download finished
 * @param opaque User data pointer passed to callbacks
 * @return 0 on success, -1 on failure
 */
int quiche_engine_connect_async(
    QuicheEngineHandle* handle,
    const char* hostname,
    const char* uri,
    const char* ip,
    const char* port,
    quiche_connect_callback_t on_connect,
    quiche_data_callback_t on_data,
    quiche_finish_callback_t on_finish,
    void* opaque
);

/**
 * Get current connection state
 * @param handle QuicheEngine handle
 * @return Current state
 */
QuicheState quiche_engine_get_state(QuicheEngineHandle* handle);

/**
 * Get last error message
 * @param handle QuicheEngine handle
 * @return Error message string (valid until next API call)
 */
const char* quiche_engine_get_last_error(QuicheEngineHandle* handle);

/**
 * Close connection and stop worker thread
 * @param handle QuicheEngine handle
 */
void quiche_engine_close(QuicheEngineHandle* handle);

/**
 * Check if engine is running (connecting, connected, or downloading)
 * @param handle QuicheEngine handle
 * @return 1 if running, 0 otherwise
 */
int quiche_engine_is_running(QuicheEngineHandle* handle);

#ifdef __cplusplus
}
#endif

#endif /* QUICHE_ENGINE_C_API_H */
