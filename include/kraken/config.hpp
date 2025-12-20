#pragma once

#include <string>
#include <chrono>

namespace kraken {

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
    int reconnect_attempts() const { return reconnect_attempts_; }
    std::chrono::milliseconds reconnect_delay() const { return reconnect_delay_; }
    bool validate_checksums() const { return validate_checksums_; }
    
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
    int reconnect_attempts_ = 10;
    std::chrono::milliseconds reconnect_delay_{1000};
    bool validate_checksums_ = true;
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
    
    /// Set number of reconnection attempts
    Builder& reconnect_attempts(int attempts);
    
    /// Set delay between reconnection attempts
    Builder& reconnect_delay(std::chrono::milliseconds delay);
    
    /// Enable/disable order book checksum validation
    Builder& validate_checksums(bool validate);
    
    /// Build the configuration
    ClientConfig build();
    
private:
    ClientConfig config_;
};

} // namespace kraken

