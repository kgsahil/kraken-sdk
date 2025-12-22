/// @file rate_limiter.hpp
/// @brief Token bucket rate limiter for WebSocket message throttling
/// 
/// Implements a thread-safe token bucket algorithm to limit the rate of
/// outbound WebSocket messages to prevent API rate limiting from Kraken.
/// 
/// @example
/// @code
/// RateLimiter limiter(10.0, 20);  // 10 requests/sec, burst of 20
/// 
/// if (limiter.acquire()) {
///     connection.send(message);
/// } else {
///     // Wait until tokens are available
///     std::this_thread::sleep_for(limiter.wait_time());
/// }
/// @endcode

#pragma once

#include <chrono>
#include <atomic>
#include <mutex>
#include <thread>

namespace kraken {

/// @brief Token bucket rate limiter
/// 
/// Thread-safe rate limiter using token bucket algorithm. Tokens are
/// refilled at a constant rate (requests per second), with a maximum
/// burst capacity.
/// 
/// @note This is a production-grade implementation optimized for low latency.
class RateLimiter {
public:
    /// @brief Construct rate limiter
    /// 
    /// @param requests_per_sec Rate limit (tokens per second)
    /// @param burst_size Maximum burst capacity (max tokens)
    /// @param enabled Whether rate limiting is enabled (default: true)
    /// 
    /// @note If enabled=false, acquire() always returns true immediately.
    explicit RateLimiter(double requests_per_sec = 10.0, 
                        size_t burst_size = 20,
                        bool enabled = true);
    
    /// @brief Try to acquire a token (non-blocking)
    /// 
    /// Attempts to consume one token. Returns true if token was acquired,
    /// false if rate limit would be exceeded.
    /// 
    /// @return true if token acquired, false if rate limited
    /// 
    /// @note Thread-safe
    bool acquire();
    
    /// @brief Acquire a token (blocking)
    /// 
    /// Blocks until a token is available, then consumes it.
    /// 
    /// @param timeout Maximum time to wait (0 = wait indefinitely)
    /// @return true if token acquired, false if timeout
    /// 
    /// @note Thread-safe
    bool acquire_blocking(std::chrono::milliseconds timeout = std::chrono::milliseconds(0));
    
    /// @brief Get estimated wait time until next token is available
    /// 
    /// @return Estimated milliseconds until next token, or 0 if available now
    /// 
    /// @note Thread-safe
    std::chrono::milliseconds wait_time() const;
    
    /// @brief Get current token count (for monitoring)
    /// @return Current number of available tokens
    /// 
    /// @note Thread-safe
    double tokens() const;
    
    /// @brief Enable or disable rate limiting
    /// 
    /// When disabled, acquire() always returns true immediately.
    /// 
    /// @param enabled Whether to enable rate limiting
    void set_enabled(bool enabled);
    
    /// @brief Check if rate limiting is enabled
    /// @return true if enabled
    bool is_enabled() const { return enabled_.load(); }
    
    /// @brief Reset the rate limiter (clear all tokens)
    /// 
    /// Useful for testing or after rate limit errors from server.
    void reset();
    
    /// @brief Get statistics
    struct Stats {
        size_t total_requests = 0;      ///< Total acquire() calls
        size_t allowed_requests = 0;    ///< Requests that were allowed
        size_t rate_limited = 0;        ///< Requests that were rate limited
        double current_tokens = 0.0;     ///< Current token count
    };
    
    /// @brief Get current statistics
    /// @return Current stats snapshot
    Stats get_stats() const;

private:
    void refill_tokens() const;
    
    mutable std::mutex mutex_;
    mutable double tokens_;              // Current token count (fractional)
    double refill_rate_;                 // Tokens per second
    size_t max_tokens_;                  // Maximum burst capacity
    std::atomic<bool> enabled_;
    
    mutable std::chrono::steady_clock::time_point last_refill_;
    
    // Statistics (lock-free)
    mutable std::atomic<size_t> total_requests_{0};
    mutable std::atomic<size_t> allowed_requests_{0};
    mutable std::atomic<size_t> rate_limited_{0};
};

} // namespace kraken

