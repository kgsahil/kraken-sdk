/// @file circuit_breaker.cpp
/// @brief Circuit Breaker implementation

#include "kraken/connection/circuit_breaker.hpp"
#include <algorithm>

namespace kraken {

CircuitBreaker::CircuitBreaker(CircuitBreakerConfig config)
    : config_(std::move(config))
    , last_failure_time_(std::chrono::steady_clock::now())
    , open_time_(std::chrono::steady_clock::now())
    , window_start_time_(std::chrono::steady_clock::now()) {}

bool CircuitBreaker::can_attempt() const {
    CircuitState current_state = state_.load(std::memory_order_acquire);
    
    if (current_state == CircuitState::Closed) {
        return true;
    }
    
    if (current_state == CircuitState::Open) {
        // Check if timeout has elapsed
        std::lock_guard<std::mutex> lock(mutex_);
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - open_time_).count();
        
        if (elapsed >= config_.min_open_time_ms.count()) {
            // Timeout elapsed - transition to half-open
            const_cast<CircuitBreaker*>(this)->transition_to_half_open();
            return true;
        }
        return false;
    }
    
    // Half-open state - allow limited attempts
    return true;
}

void CircuitBreaker::record_success() {
    CircuitState current_state = state_.load(std::memory_order_acquire);
    
    if (current_state == CircuitState::HalfOpen) {
        uint32_t count = success_count_.fetch_add(1, std::memory_order_relaxed) + 1;
        
        if (count >= config_.success_threshold) {
            transition_to_closed();
        }
    } else if (current_state == CircuitState::Closed) {
        // Reset failure window on success
        std::lock_guard<std::mutex> lock(mutex_);
        window_start_time_ = std::chrono::steady_clock::now();
        failure_count_.store(0, std::memory_order_relaxed);
    }
}

void CircuitBreaker::record_failure() {
    CircuitState current_state = state_.load(std::memory_order_acquire);
    
    std::lock_guard<std::mutex> lock(mutex_);
    last_failure_time_ = std::chrono::steady_clock::now();
    
    if (current_state == CircuitState::HalfOpen) {
        // Any failure in half-open state immediately opens circuit
        transition_to_open();
        return;
    }
    
    if (current_state == CircuitState::Closed) {
        // Check if we're still in the failure window
        auto now = std::chrono::steady_clock::now();
        auto window_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - window_start_time_).count();
        
        if (window_elapsed > config_.failure_window_ms.count()) {
            // Window expired - reset
            window_start_time_ = now;
            failure_count_.store(0, std::memory_order_relaxed);
        }
        
        uint32_t count = failure_count_.fetch_add(1, std::memory_order_relaxed) + 1;
        
        if (count >= config_.failure_threshold) {
            transition_to_open();
        }
    }
}

CircuitState CircuitBreaker::state() const {
    return state_.load(std::memory_order_acquire);
}

uint32_t CircuitBreaker::failure_count() const {
    return failure_count_.load(std::memory_order_relaxed);
}

uint32_t CircuitBreaker::success_count() const {
    return success_count_.load(std::memory_order_relaxed);
}

void CircuitBreaker::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    state_.store(CircuitState::Closed, std::memory_order_release);
    failure_count_.store(0, std::memory_order_relaxed);
    success_count_.store(0, std::memory_order_relaxed);
    window_start_time_ = std::chrono::steady_clock::now();
    last_failure_time_ = std::chrono::steady_clock::now();
    open_time_ = std::chrono::steady_clock::now();
}

const CircuitBreakerConfig& CircuitBreaker::config() const {
    return config_;
}

void CircuitBreaker::set_config(CircuitBreakerConfig config) {
    std::lock_guard<std::mutex> lock(mutex_);
    config_ = std::move(config);
}

void CircuitBreaker::transition_to_open() {
    state_.store(CircuitState::Open, std::memory_order_release);
    open_time_ = std::chrono::steady_clock::now();
    success_count_.store(0, std::memory_order_relaxed);
}

void CircuitBreaker::transition_to_half_open() {
    state_.store(CircuitState::HalfOpen, std::memory_order_release);
    success_count_.store(0, std::memory_order_relaxed);
}

void CircuitBreaker::transition_to_closed() {
    state_.store(CircuitState::Closed, std::memory_order_release);
    failure_count_.store(0, std::memory_order_relaxed);
    success_count_.store(0, std::memory_order_relaxed);
    window_start_time_ = std::chrono::steady_clock::now();
}

} // namespace kraken

