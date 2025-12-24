/// @file lifecycle.cpp
/// @brief Client lifecycle management (construction, connection, event loop)

#include "internal/client_impl.hpp"
#include "internal/connection.hpp"
#include "kraken/logger.hpp"
#include "kraken/telemetry.hpp"
#include "queue.cpp"  // NOLINT(bugprone-suspicious-include) - Template instantiation
#include "internal/parser.hpp"

namespace kraken {

namespace {
    std::unique_ptr<MessageQueue<Message>> create_queue(const ClientConfig& config) {
        if (!config.use_queue()) {
            return nullptr;
        }
        return std::make_unique<DefaultMessageQueue<Message>>(config.queue_capacity());
    }
}  // namespace

KrakenClient::Impl::Impl(ClientConfig config)
    : config_(std::move(config))
    , queue_(config_.use_queue() ? create_queue(config_) : nullptr)
    , start_time_(std::chrono::steady_clock::now())
    , backoff_strategy_(config_.backoff_strategy())
    , circuit_breaker_(config_.circuit_breaker_enabled() 
        ? std::make_unique<CircuitBreaker>(config_.circuit_breaker_config())
        : nullptr)
    , gap_tracker_(config_.gap_detection_config()) {
    
    // Set up gap callback - combine user callback with telemetry
    if (config_.on_gap() || config_.telemetry_config().enable_metrics) {
        gap_tracker_.on_gap([this](const GapInfo& gap) {
            // Update telemetry if enabled
            if (telemetry_) {
                telemetry_->metrics().increment_gaps_detected();
            }
            // Call user callback if configured
            if (config_.on_gap()) {
                try {
                    config_.on_gap()(gap);
                } catch (...) {
                    // User callback threw - ignore
                }
            }
        });
    }
    
    // Initialize telemetry if configured
    if (config_.telemetry_config().enable_metrics) {
        telemetry_ = Telemetry::create(config_.telemetry_config());
        // Start telemetry services (HTTP server, OTLP export)
        telemetry_->start();
    }
}

KrakenClient::Impl::~Impl() {
    stop();
}

void KrakenClient::Impl::connect() {
    if (state_ == ConnectionState::Connected || 
        state_ == ConnectionState::Connecting) {
        return;
    }
    
    set_connection_state(ConnectionState::Connecting);
    
    try {
        connection_ = std::make_unique<Connection>(
            config_.url(),
            config_.connection_timeouts(),
            config_.security_config(),
            config_.rate_limiter()
        );
        connection_->connect();
        set_connection_state(ConnectionState::Connected);
    } catch (const std::exception& e) {
        set_connection_state(ConnectionState::Disconnected);
        throw ConnectionError(std::string("Failed to connect: ") + e.what());
    }
}

void KrakenClient::Impl::disconnect() {
    if (connection_) {
        connection_->close();
        connection_.reset();
    }
    set_connection_state(ConnectionState::Disconnected);
}

bool KrakenClient::Impl::is_connected() const {
    return state_ == ConnectionState::Connected;
}

ConnectionState KrakenClient::Impl::connection_state() const {
    return state_;
}

void KrakenClient::Impl::set_connection_state(ConnectionState state) {
    state_.store(state, std::memory_order_relaxed);
    
    // Update telemetry if enabled
    if (telemetry_) {
        telemetry_->metrics().set_connection_state(static_cast<int>(state));
    }
    
    // Notify callback
    std::shared_lock lock(callbacks_mutex_);
    if (state_callback_) {
        state_callback_(state);
    }
}

void KrakenClient::Impl::run() {
    if (running_) return;
    
    // Connect if not connected
    if (!is_connected()) {
        connect();
    }
    
    // Send any pending subscriptions
    send_pending_subscriptions();
    
    running_ = true;
    stop_requested_ = false;
    
    // Start I/O thread
    io_thread_ = std::thread([this]() { io_loop(); });
    
    // Run dispatcher in current thread (only if queue is enabled)
    if (queue_) {
        dispatcher_loop();
    } else {
        // Direct mode: I/O thread handles everything, just wait for it
        if (io_thread_.joinable()) {
            io_thread_.join();
        }
    }
    
    // Wait for I/O thread to finish
    if (io_thread_.joinable()) {
        io_thread_.join();
    }
    
    running_ = false;
}

void KrakenClient::Impl::run_async() {
    if (running_) return;
    
    // Connect if not connected
    if (!is_connected()) {
        connect();
    }
    
    // Send any pending subscriptions
    send_pending_subscriptions();
    
    running_ = true;
    stop_requested_ = false;
    
    // Start I/O thread
    io_thread_ = std::thread([this]() { io_loop(); });
    
    // Start dispatcher thread (only if queue is enabled)
    if (queue_) {
        dispatcher_thread_ = std::thread([this]() { dispatcher_loop(); });
    }
}

void KrakenClient::Impl::stop() {
    // Use atomic compare-and-swap to prevent race conditions
    bool expected = true;
    if (!running_.compare_exchange_strong(expected, false)) {
        return;  // Already stopping or stopped
    }
    
    stop_requested_ = true;
    
    // Stop telemetry services
    if (telemetry_) {
        telemetry_->stop();
    }
    
    // Wake up dispatcher thread if waiting on condition variable
    queue_cv_.notify_all();
    
    // Close connection to unblock I/O thread
    if (connection_) {
        connection_->close();
    }
    
    // Wait for threads (with timeout protection)
    if (io_thread_.joinable()) {
        io_thread_.join();
    }
    if (queue_ && dispatcher_thread_.joinable()) {
        dispatcher_thread_.join();
    }
}

bool KrakenClient::Impl::is_running() const {
    return running_;
}

} // namespace kraken

