/// @file config.hpp
/// @brief Client configuration with builder pattern
/// 
/// Provides ClientConfig class and builder for configuring all aspects of
/// the KrakenClient including connection, authentication, queue, reconnection,
/// gap detection, telemetry, and security settings.

#pragma once

#include "../connection/backoff.hpp"
#include "../connection/gap_detector.hpp"
#include "../telemetry/telemetry.hpp"
#include "../connection/connection_config.hpp"
#include "../rate_limiter.hpp"
#include <string>
#include <chrono>
#include <memory>
#include <functional>

namespace kraken {

/// @brief Reconnect event information
/// 
/// Passed to reconnect callbacks to provide details about reconnection attempts.
struct ReconnectEvent {
    int attempt;           ///< Current attempt number (1-based)
    int max_attempts;      ///< Maximum attempts (0 = infinite)
    std::chrono::milliseconds delay;  ///< Delay before this attempt
    std::string reason;    ///< Reason for reconnection
};

/// @brief Callback for reconnection events
/// 
/// Called before each reconnection attempt.
/// 
/// @param event Reconnection event details
using ReconnectCallback = std::function<void(const ReconnectEvent&)>;

/// @brief Client configuration with builder pattern
/// 
/// Configures all aspects of the KrakenClient including connection settings,
/// authentication, queue behavior, reconnection strategy, and telemetry.
/// 
/// @example
/// @code
/// auto config = ClientConfig::Builder()
///     .url("wss://ws.kraken.com/v2")
///     .api_key("your_key")
///     .api_secret("your_secret")
///     .queue_capacity(65536)
///     .validate_checksums(true)
///     .backoff(ExponentialBackoff::conservative())
///     .build();
/// @endcode
class ClientConfig {
public:
    /// Builder for fluent configuration (forward declaration)
    class Builder;
    
    //--------------------------------------------------------------------------
    // Accessors
    //--------------------------------------------------------------------------
    
    /// @brief Get WebSocket URL
    /// @return WebSocket URL
    const std::string& url() const { return url_; }
    
    /// @brief Get API key
    /// @return API key (empty if not authenticated)
    const std::string& api_key() const { return api_key_; }
    
    /// @brief Get API secret
    /// @return API secret (empty if not authenticated)
    const std::string& api_secret() const { return api_secret_; }
    
    /// @brief Get message queue capacity
    /// @return Queue capacity (power of 2 recommended)
    size_t queue_capacity() const { return queue_capacity_; }
    
    /// @brief Check if checksum validation is enabled
    /// @return true if order book checksums are validated
    bool validate_checksums() const { return validate_checksums_; }
    
    /// @brief Check if message queue is enabled
    /// @return true if SPSC queue is used (two-thread architecture),
    ///         false if messages processed directly in I/O thread
    bool use_queue() const { return use_queue_; }
    
    /// @brief Get backoff strategy (cloned for thread safety)
    /// @return Unique pointer to cloned backoff strategy
    std::unique_ptr<BackoffStrategy> backoff_strategy() const {
        return backoff_strategy_ ? backoff_strategy_->clone() 
                                 : ExponentialBackoff::conservative();
    }
    
    /// @brief Get reconnect callback
    /// @return Reconnect callback (may be null)
    const ReconnectCallback& on_reconnect() const { return on_reconnect_; }
    
    /// @brief Get gap detection configuration
    /// @return Gap detection config
    const SequenceTracker::Config& gap_detection_config() const { return gap_config_; }
    
    /// @brief Get gap callback
    /// @return Gap callback (may be null)
    const GapCallback& on_gap() const { return on_gap_; }
    
    /// @brief Get telemetry configuration
    /// @return Telemetry config
    const TelemetryConfig& telemetry_config() const { return telemetry_config_; }
    
    /// @brief Get connection timeout configuration
    /// @return Connection timeouts
    const ConnectionTimeouts& connection_timeouts() const { return connection_timeouts_; }
    
    /// @brief Get security configuration
    /// @return Security/TLS config
    const SecurityConfig& security_config() const { return security_config_; }
    
    /// @brief Get rate limiter configuration
    /// @return Rate limiter (may be null if disabled)
    std::shared_ptr<RateLimiter> rate_limiter() const { return rate_limiter_; }
    
    /// @brief Check if rate limiting is enabled
    /// @return true if rate limiting is enabled
    bool rate_limiting_enabled() const { return rate_limiter_ != nullptr; }
    
    // Legacy accessors (deprecated, for backward compatibility)
    [[deprecated("Use backoff_strategy() instead")]]
    int reconnect_attempts() const { 
        return backoff_strategy_ ? backoff_strategy_->max_attempts() : 10; 
    }
    
    [[deprecated("Use backoff_strategy() instead")]]
    std::chrono::milliseconds reconnect_delay() const { 
        return std::chrono::milliseconds(1000); 
    }
    
    /// @brief Check if authentication is configured
    /// @return true if both API key and secret are set
    bool is_authenticated() const { 
        return !api_key_.empty() && !api_secret_.empty(); 
    }
    
private:
    friend class Builder;
    
    std::string url_ = "wss://ws.kraken.com/v2";
    std::string api_key_;
    std::string api_secret_;
    size_t queue_capacity_ = 65536;
    bool validate_checksums_ = true;
    bool use_queue_ = true;  // Default: use queue (two-thread architecture)
    std::shared_ptr<BackoffStrategy> backoff_strategy_;
    ReconnectCallback on_reconnect_;
    SequenceTracker::Config gap_config_;
    GapCallback on_gap_;
    TelemetryConfig telemetry_config_;
    ConnectionTimeouts connection_timeouts_;
    SecurityConfig security_config_;
    std::shared_ptr<RateLimiter> rate_limiter_;  // null if disabled
};

//------------------------------------------------------------------------------
// Builder implementation (defined after ClientConfig is complete)
//------------------------------------------------------------------------------

/// @brief Builder for ClientConfig (fluent interface)
/// 
/// Provides a fluent API for constructing ClientConfig objects.
class ClientConfig::Builder {
public:
    /// @brief Default constructor
    Builder() = default;
    
    /// @brief Set WebSocket URL
    /// @param url WebSocket URL (e.g., "wss://ws.kraken.com/v2")
    /// @return Reference to this builder
    Builder& url(std::string url);
    
    /// @brief Set API key for authenticated endpoints
    /// @param key API key
    /// @return Reference to this builder
    Builder& api_key(std::string key);
    
    /// @brief Set API secret for authenticated endpoints
    /// @param secret API secret
    /// @return Reference to this builder
    Builder& api_secret(std::string secret);
    
    /// @brief Set message queue capacity
    /// @param capacity Queue capacity (power of 2 recommended for SPSC queue)
    /// @return Reference to this builder
    Builder& queue_capacity(size_t capacity);
    
    /// @brief Enable/disable message queue
    /// 
    /// If false, messages are processed directly in the I/O thread.
    /// This reduces latency but blocks I/O during callback execution.
    /// 
    /// If true (default), messages go through SPSC queue to dispatcher thread.
    /// This allows I/O to continue while callbacks execute.
    /// 
    /// @param enable true to use queue (default), false to process directly
    /// @return Reference to this builder
    Builder& use_queue(bool enable);
    
    /// @brief Enable/disable order book checksum validation
    /// @param validate true to validate CRC32 checksums (default: true)
    /// @return Reference to this builder
    Builder& validate_checksums(bool validate);
    
    /// @brief Set backoff strategy for reconnection
    /// @param strategy Unique pointer to backoff strategy
    /// @return Reference to this builder
    /// @example .backoff(ExponentialBackoff::conservative())
    Builder& backoff(std::unique_ptr<BackoffStrategy> strategy);
    
    /// @brief Set backoff strategy using shared_ptr (for reuse)
    /// @param strategy Shared pointer to backoff strategy
    /// @return Reference to this builder
    Builder& backoff(std::shared_ptr<BackoffStrategy> strategy);
    
    /// @brief Set callback for reconnection events
    /// @param callback Reconnect callback
    /// @return Reference to this builder
    Builder& on_reconnect(ReconnectCallback callback);
    
    /// @brief Enable/disable gap detection
    /// @param enabled true to enable gap detection (default: false)
    /// @return Reference to this builder
    Builder& gap_detection(bool enabled);
    
    /// @brief Set gap tolerance (allow up to N missing messages)
    /// @param tolerance Number of missing messages to tolerate before reporting gap
    /// @return Reference to this builder
    Builder& gap_tolerance(int tolerance);
    
    /// @brief Set gap detection callback
    /// @param callback Gap callback
    /// @return Reference to this builder
    Builder& on_gap(GapCallback callback);
    
    /// @brief Set telemetry configuration
    /// @param config Telemetry configuration
    /// @return Reference to this builder
    Builder& telemetry(TelemetryConfig config);
    
    /// @brief Set connection timeout configuration
    /// @param timeouts Connection timeout settings
    /// @return Reference to this builder
    Builder& connection_timeouts(ConnectionTimeouts timeouts);
    
    /// @brief Set security configuration
    /// @param security TLS/SSL security settings
    /// @return Reference to this builder
    Builder& security(SecurityConfig security);
    
    /// @brief Enable/disable rate limiting
    /// 
    /// When enabled, outbound WebSocket messages are throttled to prevent
    /// API rate limiting from Kraken.
    /// 
    /// @param enabled true to enable rate limiting (default: false)
    /// @param requests_per_sec Rate limit in requests per second (default: 10.0)
    /// @param burst_size Maximum burst capacity (default: 20)
    /// @return Reference to this builder
    Builder& rate_limiting(bool enabled, double requests_per_sec = 10.0, size_t burst_size = 20);
    
    // Legacy methods (deprecated, for backward compatibility)
    [[deprecated("Use backoff() instead")]]
    Builder& reconnect_attempts(int attempts);
    
    [[deprecated("Use backoff() instead")]]
    Builder& reconnect_delay(std::chrono::milliseconds delay);
    
    /// @brief Build the configuration
    /// @return ClientConfig object
    ClientConfig build();
    
private:
    ClientConfig config_;
    
    // Legacy backoff params (used if backoff() not called)
    int legacy_attempts_ = 10;
    std::chrono::milliseconds legacy_delay_{1000};
    bool legacy_mode_ = false;
};

} // namespace kraken

