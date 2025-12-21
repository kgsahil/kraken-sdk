#pragma once

#include "backoff.hpp"
#include "gap_detector.hpp"
#include "telemetry.hpp"
#include <string>
#include <chrono>
#include <memory>
#include <functional>

namespace kraken {

/// Reconnect event information
struct ReconnectEvent {
    int attempt;           ///< Current attempt number (1-based)
    int max_attempts;      ///< Maximum attempts (0 = infinite)
    std::chrono::milliseconds delay;  ///< Delay before this attempt
    std::string reason;    ///< Reason for reconnection
};

/// Callback for reconnection events
using ReconnectCallback = std::function<void(const ReconnectEvent&)>;

/// Client configuration with builder pattern
class ClientConfig {
public:
    /// Builder for fluent configuration (forward declaration)
    class Builder;
    
    // Accessors
    const std::string& url() const { return url_; }
    const std::string& api_key() const { return api_key_; }
    const std::string& api_secret() const { return api_secret_; }
    size_t queue_capacity() const { return queue_capacity_; }
    bool validate_checksums() const { return validate_checksums_; }
    
    /// Get backoff strategy (cloned for thread safety)
    std::unique_ptr<BackoffStrategy> backoff_strategy() const {
        return backoff_strategy_ ? backoff_strategy_->clone() 
                                 : ExponentialBackoff::conservative();
    }
    
    /// Get reconnect callback (may be null)
    const ReconnectCallback& on_reconnect() const { return on_reconnect_; }
    
    /// Get gap detection config
    const SequenceTracker::Config& gap_detection_config() const { return gap_config_; }
    
    /// Get gap callback (may be null)
    const GapCallback& on_gap() const { return on_gap_; }
    
    /// Get telemetry configuration
    const TelemetryConfig& telemetry_config() const { return telemetry_config_; }
    
    // Legacy accessors (deprecated, for backward compatibility)
    [[deprecated("Use backoff_strategy() instead")]]
    int reconnect_attempts() const { 
        return backoff_strategy_ ? backoff_strategy_->max_attempts() : 10; 
    }
    
    [[deprecated("Use backoff_strategy() instead")]]
    std::chrono::milliseconds reconnect_delay() const { 
        return std::chrono::milliseconds(1000); 
    }
    
    // Check if authenticated
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
    std::shared_ptr<BackoffStrategy> backoff_strategy_;
    ReconnectCallback on_reconnect_;
    SequenceTracker::Config gap_config_;
    GapCallback on_gap_;
    TelemetryConfig telemetry_config_;
};

//------------------------------------------------------------------------------
// Builder implementation (defined after ClientConfig is complete)
//------------------------------------------------------------------------------

class ClientConfig::Builder {
public:
    Builder() = default;
    
    /// Set WebSocket URL
    Builder& url(std::string url);
    
    /// Set API key for authenticated endpoints
    Builder& api_key(std::string key);
    
    /// Set API secret for authenticated endpoints
    Builder& api_secret(std::string secret);
    
    /// Set message queue capacity (power of 2 recommended)
    Builder& queue_capacity(size_t capacity);
    
    /// Enable/disable order book checksum validation
    Builder& validate_checksums(bool validate);
    
    /// Set backoff strategy for reconnection
    /// @example .backoff(ExponentialBackoff::conservative())
    Builder& backoff(std::unique_ptr<BackoffStrategy> strategy);
    
    /// Set backoff strategy using shared_ptr (for reuse)
    Builder& backoff(std::shared_ptr<BackoffStrategy> strategy);
    
    /// Set callback for reconnection events
    Builder& on_reconnect(ReconnectCallback callback);
    
    /// Enable/disable gap detection
    Builder& gap_detection(bool enabled);
    
    /// Set gap tolerance (allow up to N missing messages)
    Builder& gap_tolerance(int tolerance);
    
    /// Set gap detection callback
    Builder& on_gap(GapCallback callback);
    
    /// Set telemetry configuration
    Builder& telemetry(TelemetryConfig config);
    
    // Legacy methods (deprecated, for backward compatibility)
    [[deprecated("Use backoff() instead")]]
    Builder& reconnect_attempts(int attempts);
    
    [[deprecated("Use backoff() instead")]]
    Builder& reconnect_delay(std::chrono::milliseconds delay);
    
    /// Build the configuration
    ClientConfig build();
    
private:
    ClientConfig config_;
    
    // Legacy backoff params (used if backoff() not called)
    int legacy_attempts_ = 10;
    std::chrono::milliseconds legacy_delay_{1000};
    bool legacy_mode_ = false;
};

} // namespace kraken

