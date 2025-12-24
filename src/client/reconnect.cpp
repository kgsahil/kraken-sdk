/// @file reconnect.cpp
/// @brief Reconnection logic with exponential backoff

#include "internal/client_impl.hpp"
#include "internal/connection.hpp"

namespace kraken {

void KrakenClient::Impl::handle_reconnect() {
    if (stop_requested_) return;
    
    set_connection_state(ConnectionState::Reconnecting);
    
    // Reset backoff for fresh reconnection sequence
    if (backoff_strategy_) {
        backoff_strategy_->reset();
    }
    
    while (!stop_requested_) {
        // Check circuit breaker before attempting reconnection
        if (circuit_breaker_ && !circuit_breaker_->can_attempt()) {
            // Circuit is open - wait for timeout before trying again
            safe_invoke_error_callback(
                ErrorCode::ConnectionFailed,
                "Circuit breaker is open - connection failures exceeded threshold",
                ""
            );
            // Wait for circuit breaker timeout
            std::this_thread::sleep_for(circuit_breaker_->config().timeout_ms);
            continue;
        }
        
        // Check if we should stop retrying
        if (backoff_strategy_ && backoff_strategy_->should_stop()) {
            break;
        }
        
        // Get delay with exponential backoff and jitter
        auto delay = backoff_strategy_ 
            ? backoff_strategy_->next_delay() 
            : std::chrono::milliseconds(1000);
        
        int attempt = backoff_strategy_ ? backoff_strategy_->current_attempt() : 1;
        int max_attempts = backoff_strategy_ ? backoff_strategy_->max_attempts() : 10;
        
        // Notify via callback if configured
        if (config_.on_reconnect()) {
            ReconnectEvent event{
                attempt,
                max_attempts,
                delay,
                "Connection lost"
            };
            try {
                config_.on_reconnect()(event);
            } catch (...) {
                // Callback threw - ignore
            }
        }
        
        // Update telemetry if enabled
        if (telemetry_) {
            telemetry_->metrics().increment_reconnect_attempts();
        }
        
        // Wait before attempting reconnection
        if (delay.count() > 0) {
            std::this_thread::sleep_for(delay);
        }
        
        if (stop_requested_) break;
        
        try {
            connection_ = std::make_unique<Connection>(
                config_.url(),
                config_.connection_timeouts(),
                config_.security_config(),
                config_.rate_limiter()
            );
            connection_->connect();
            set_connection_state(ConnectionState::Connected);
            
            // Record success in circuit breaker
            if (circuit_breaker_) {
                circuit_breaker_->record_success();
            }
            
            // Reset backoff on successful connection
            if (backoff_strategy_) {
                backoff_strategy_->reset();
            }
            
            // Reset gap tracker for fresh sequence tracking
            gap_tracker_.reset_all();
            
            // Resubscribe (failures in send_subscribe are handled internally)
            std::lock_guard<std::mutex> lock(subscriptions_mutex_);
            for (auto& pair : subscriptions_) {
                auto& sub = pair.second;
                if (sub->is_active() && !sub->is_paused()) {
                    send_subscribe(sub->channel(), sub->symbols(), sub->depth());
                }
            }
            
            return;
        } catch (const std::exception& e) {
            // Record failure in circuit breaker
            if (circuit_breaker_) {
                circuit_breaker_->record_failure();
            }
            
            // Log and continue with backoff
            safe_invoke_error_callback(
                ErrorCode::ConnectionFailed,
                std::string("Reconnect attempt ") + std::to_string(attempt) + " failed: " + e.what(),
                ""
            );
        }
    }
    
    // Failed to reconnect after all attempts
    set_connection_state(ConnectionState::Disconnected);
    safe_invoke_error_callback(
        ErrorCode::ConnectionFailed,
        "Failed to reconnect after maximum attempts",
        ""
    );
}

} // namespace kraken

