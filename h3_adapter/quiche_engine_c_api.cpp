/*
 * QuicheEngine C API Implementation
 *
 * This adapter allows C code (FFmpeg) to use the C++ QuicheEngine library.
 * It runs QuicheEngine in a separate thread and delivers data via callbacks.
 */

#include "quiche_engine_c_api.h"
#include "quiche_engine.h"  // C++ QuicheEngine header from engine/include/
#include <thread>
#include <atomic>
#include <mutex>
#include <vector>
#include <cstring>

using namespace quiche;

// Internal structure wrapping the C++ QuicheEngine
struct QuicheEngineHandle {
    QuicheEngine* engine;
    std::thread worker_thread;
    std::atomic<bool> should_stop;
    std::atomic<QuicheState> state;

    // Callbacks and user data
    quiche_connect_callback_t on_connect;
    quiche_data_callback_t on_data;
    quiche_finish_callback_t on_finish;
    void* opaque;

    // Error tracking
    std::mutex error_mutex;
    std::string last_error;

    QuicheEngineHandle()
        : engine(nullptr)
        , should_stop(false)
        , state(QUICHE_STATE_IDLE)
        , on_connect(nullptr)
        , on_data(nullptr)
        , on_finish(nullptr)
        , opaque(nullptr)
    {}

    ~QuicheEngineHandle() {
        if (engine) {
            delete engine;
            engine = nullptr;
        }
    }
};

// Event callback adapter: converts C++ events to C callbacks
static void global_event_callback(
    QuicheEngine* engine,
    EngineEvent event,
    const EventData& data,
    void* user_data
) {
    QuicheEngineHandle* handle = static_cast<QuicheEngineHandle*>(user_data);
    if (!handle) return;

    switch (event) {
        case EngineEvent::EVT_CONNECTED:
            handle->state = QUICHE_STATE_CONNECTED;
            if (handle->on_connect) {
                handle->on_connect(
                    handle->opaque,
                    engine->getScid().c_str(),
                    nullptr
                );
            }
            break;

        case EngineEvent::EVT_STREAM_READABLE:
            handle->state = QUICHE_STATE_DOWNLOADING;
            // Data will be pulled via read() in the worker thread
            break;

        case EngineEvent::EVT_DOWNLOAD_COMPLETED:
            handle->state = QUICHE_STATE_FINISHED;
            if (handle->on_finish) {
                handle->on_finish(handle->opaque, nullptr);
            }
            handle->should_stop = true;
            break;

        case EngineEvent::EVT_CONNECTION_CLOSED:
            if (handle->state != QUICHE_STATE_FINISHED) {
                handle->state = QUICHE_STATE_ERROR;
                std::lock_guard<std::mutex> lock(handle->error_mutex);
                handle->last_error = "Connection closed unexpectedly";
                if (handle->on_finish) {
                    handle->on_finish(handle->opaque, handle->last_error.c_str());
                }
            }
            handle->should_stop = true;
            break;

        case EngineEvent::EVT_ERROR:
            handle->state = QUICHE_STATE_ERROR;
            {
                std::lock_guard<std::mutex> lock(handle->error_mutex);
                handle->last_error = data.str_val;
                if (handle->on_finish) {
                    handle->on_finish(handle->opaque, handle->last_error.c_str());
                }
            }
            handle->should_stop = true;
            break;

        default:
            break;
    }
}

// Worker thread function: runs the QUIC connection and download
static void download_worker(
    QuicheEngineHandle* handle,
    std::string hostname,
    std::string uri,
    std::string ip,
    std::string port
) {
    // Create engine instance
    handle->engine = new QuicheEngine();

    // Configure engine
    ConfigMap config;
    config[ConfigKey::MAX_IDLE_TIMEOUT] = ConfigValue(uint64_t(30000)); // 30 seconds
    config[ConfigKey::INITIAL_MAX_DATA] = ConfigValue(uint64_t(10485760)); // 10MB
    config[ConfigKey::INITIAL_MAX_STREAM_DATA_BIDI_LOCAL] = ConfigValue(uint64_t(10485760));
    config[ConfigKey::INITIAL_MAX_STREAM_DATA_BIDI_REMOTE] = ConfigValue(uint64_t(10485760));
    config[ConfigKey::INITIAL_MAX_STREAMS_BIDI] = ConfigValue(uint64_t(100));
    config[ConfigKey::VERIFY_PEER] = ConfigValue(false); // Disable cert verification for testing

    if (!handle->engine->open(config)) {
        std::lock_guard<std::mutex> lock(handle->error_mutex);
        handle->last_error = handle->engine->getLastError();
        handle->state = QUICHE_STATE_ERROR;
        if (handle->on_connect) {
            handle->on_connect(handle->opaque, nullptr, handle->last_error.c_str());
        }
        handle->should_stop = true;
        delete handle->engine;
        handle->engine = nullptr;
        return;
    }

    // Set event callback
    handle->engine->setEventCallback(global_event_callback, handle);

    // Connect
    handle->state = QUICHE_STATE_CONNECTING;
    std::string conn_id = handle->engine->connect(
        hostname,
        uri,
        ip,
        port,
        10000,  // timeout: 10 seconds
        0       // from_byte: start from beginning
    );

    if (conn_id.empty()) {
        std::lock_guard<std::mutex> lock(handle->error_mutex);
        handle->last_error = handle->engine->getLastError();
        handle->state = QUICHE_STATE_ERROR;
        if (handle->on_connect) {
            handle->on_connect(handle->opaque, nullptr, handle->last_error.c_str());
        }
        handle->should_stop = true;
        delete handle->engine;
        handle->engine = nullptr;
        return;
    }

    // Download loop: continuously read data and deliver via callback
    std::vector<uint8_t> buffer(65536); // 64KB read buffer
    bool fin = false;

    while (!handle->should_stop) {
        ssize_t read_len = handle->engine->read(
            buffer.data(),
            buffer.size(),
            fin,
            100  // timeout: 100ms
        );

        if (read_len > 0) {
            // Deliver data to callback
            if (handle->on_data) {
                handle->on_data(
                    handle->opaque,
                    buffer.data(),
                    static_cast<size_t>(read_len),
                    fin ? 1 : 0
                );
            }
        } else if (read_len < 0) {
            // Error already reported via event callback
            break;
        }

        if (fin) {
            // End of stream
            break;
        }
    }

    // Cleanup
    if (handle->engine) {
        handle->engine->close(0, "Download finished");
        delete handle->engine;
        handle->engine = nullptr;
    }
}

// C API implementations

extern "C" {

QuicheEngineHandle* quiche_engine_create(void) {
    try {
        return new QuicheEngineHandle();
    } catch (...) {
        return nullptr;
    }
}

void quiche_engine_destroy(QuicheEngineHandle* handle) {
    if (handle) {
        quiche_engine_close(handle);
        delete handle;
    }
}

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
) {
    if (!handle || !hostname || !uri || !ip || !port) {
        return -1;
    }

    // Store callbacks and user data
    handle->on_connect = on_connect;
    handle->on_data = on_data;
    handle->on_finish = on_finish;
    handle->opaque = opaque;
    handle->should_stop = false;
    handle->state = QUICHE_STATE_IDLE;

    // Launch worker thread
    try {
        handle->worker_thread = std::thread(
            download_worker,
            handle,
            std::string(hostname),
            std::string(uri),
            std::string(ip),
            std::string(port)
        );
        return 0;
    } catch (...) {
        std::lock_guard<std::mutex> lock(handle->error_mutex);
        handle->last_error = "Failed to create worker thread";
        handle->state = QUICHE_STATE_ERROR;
        return -1;
    }
}

QuicheState quiche_engine_get_state(QuicheEngineHandle* handle) {
    if (!handle) {
        return QUICHE_STATE_ERROR;
    }
    return handle->state;
}

const char* quiche_engine_get_last_error(QuicheEngineHandle* handle) {
    if (!handle) {
        return "Invalid handle";
    }
    std::lock_guard<std::mutex> lock(handle->error_mutex);
    return handle->last_error.c_str();
}

void quiche_engine_close(QuicheEngineHandle* handle) {
    if (handle) {
        handle->should_stop = true;
        if (handle->worker_thread.joinable()) {
            handle->worker_thread.join();
        }
    }
}

int quiche_engine_is_running(QuicheEngineHandle* handle) {
    if (!handle) {
        return 0;
    }
    QuicheState state = handle->state;
    return (state == QUICHE_STATE_CONNECTING ||
            state == QUICHE_STATE_CONNECTED ||
            state == QUICHE_STATE_DOWNLOADING) ? 1 : 0;
}

} // extern "C"
