/// @file circuit_breaker.hpp
/// @brief Circuit Breaker pattern for connection resilience
/// 
/// Implements the Circuit Breaker pattern to prevent cascading failures
/// when the WebSocket connection repeatedly fails. Automatically opens
/// the circuit after failure threshold is reached, and attempts recovery
/// after a timeout period.

#pragma once

#include <atomic>
#include <chrono>
#include <mutex>
#include <cstdint>

namespace kraken {

/// @brief Circuit Breaker state
enum class CircuitState {
    Closed,    ///< Normal operation - requests pass through
    Open,      ///< Service failing - requests rejected immediately
    HalfOpen   ///< Testing recovery - limited requests allowed
};

/// @brief Circuit Breaker configuration
struct CircuitBreakerConfig {
    /// Failure threshold before opening circuit
    uint32_t failure_threshold = 5;
    
    /// Success threshold in half-open state to close circuit
    uint32_t success_threshold = 2;
    
    /// Timeout before transitioning from open to half-open (milliseconds)
    std::chrono::milliseconds timeout_ms{30000};
    
    /// Time window for counting failures (milliseconds)
    std::chrono::milliseconds failure_window_ms{60000};
    
    /// Minimum time in open state before attempting recovery (milliseconds)
    std::chrono::milliseconds min_open_time_ms{10000};
};

/// @brief Circuit Breaker for connection resilience
/// 
/// Prevents cascading failures by automatically opening the circuit
/// when connection failures exceed a threshold, and attempting recovery
/// after a timeout period.
/// 
/// Thread-safe: Uses atomic operations and mutex for state management.
/// 
/// @example
/// CircuitBreaker cb(CircuitBreakerConfig{});
/// 
/// if (cb.can_attempt()) {
///     try {
///         // Attempt connection
///         cb.record_success();
///     } catch (...) {
///         cb.record_failure();
///     }
/// } else {
///     // Circuit is open - reject immediately
/// }
class CircuitBreaker {
public:
    /// @brief Construct circuit breaker with configuration
    /// @param config Circuit breaker configuration
    explicit CircuitBreaker(CircuitBreakerConfig config = {});
    
    /// @brief Check if a request can be attempted
    /// 
    /// Returns true if:
    /// - Circuit is closed (normal operation)
    /// - Circuit is half-open (testing recovery)
    /// 
    /// Returns false if:
    /// - Circuit is open (service failing)
    /// 
    /// @return true if request can be attempted
    bool can_attempt() const;
    
    /// @brief Record a successful operation
    /// 
    /// In half-open state, this may transition to closed state
    /// if success threshold is reached.
    void record_success();
    
    /// @brief Record a failed operation
    /// 
    /// May transition to open state if failure threshold is reached.
    void record_failure();
    
    /// @brief Get current circuit state
    /// @return Current state (closed, open, or half-open)
    CircuitState state() const;
    
    /// @brief Get failure count in current window
    /// @return Number of failures
    uint32_t failure_count() const;
    
    /// @brief Get success count in half-open state
    /// @return Number of successes
    uint32_t success_count() const;
    
    /// @brief Reset circuit breaker to closed state
    /// 
    /// Clears all failure/success counts and resets to initial state.
    void reset();
    
    /// @brief Get configuration
    /// @return Current configuration
    const CircuitBreakerConfig& config() const;
    
    /// @brief Update configuration
    /// @param config New configuration
    void set_config(CircuitBreakerConfig config);

private:
    void check_and_transition();
    void transition_to_open();
    void transition_to_half_open();
    void transition_to_closed();
    
    CircuitBreakerConfig config_;
    mutable std::mutex mutex_;
    
    std::atomic<CircuitState> state_{CircuitState::Closed};
    std::atomic<uint32_t> failure_count_{0};
    std::atomic<uint32_t> success_count_{0};
    
    std::chrono::steady_clock::time_point last_failure_time_;
    std::chrono::steady_clock::time_point open_time_;
    std::chrono::steady_clock::time_point window_start_time_;
};

} // namespace kraken

