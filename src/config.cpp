#include "kraken/config.hpp"

namespace kraken {

//------------------------------------------------------------------------------
// ClientConfig::Builder implementation
//------------------------------------------------------------------------------

ClientConfig::Builder& ClientConfig::Builder::url(std::string url) {
    config_.url_ = std::move(url);
    return *this;
}

ClientConfig::Builder& ClientConfig::Builder::api_key(std::string key) {
    config_.api_key_ = std::move(key);
    return *this;
}

ClientConfig::Builder& ClientConfig::Builder::api_secret(std::string secret) {
    config_.api_secret_ = std::move(secret);
    return *this;
}

ClientConfig::Builder& ClientConfig::Builder::queue_capacity(size_t capacity) {
    config_.queue_capacity_ = capacity;
    return *this;
}

ClientConfig::Builder& ClientConfig::Builder::validate_checksums(bool validate) {
    config_.validate_checksums_ = validate;
    return *this;
}

ClientConfig::Builder& ClientConfig::Builder::backoff(std::unique_ptr<BackoffStrategy> strategy) {
    config_.backoff_strategy_ = std::move(strategy);
    return *this;
}

ClientConfig::Builder& ClientConfig::Builder::backoff(std::shared_ptr<BackoffStrategy> strategy) {
    config_.backoff_strategy_ = std::move(strategy);
    return *this;
}

ClientConfig::Builder& ClientConfig::Builder::on_reconnect(ReconnectCallback callback) {
    config_.on_reconnect_ = std::move(callback);
    return *this;
}

ClientConfig::Builder& ClientConfig::Builder::gap_detection(bool enabled) {
    config_.gap_config_.enabled = enabled;
    return *this;
}

ClientConfig::Builder& ClientConfig::Builder::gap_tolerance(int tolerance) {
    config_.gap_config_.gap_tolerance = tolerance;
    return *this;
}

ClientConfig::Builder& ClientConfig::Builder::on_gap(GapCallback callback) {
    config_.on_gap_ = std::move(callback);
    return *this;
}

ClientConfig::Builder& ClientConfig::Builder::telemetry(TelemetryConfig config) {
    config_.telemetry_config_ = std::move(config);
    return *this;
}

ClientConfig::Builder& ClientConfig::Builder::connection_timeouts(ConnectionTimeouts timeouts) {
    config_.connection_timeouts_ = std::move(timeouts);
    return *this;
}

ClientConfig::Builder& ClientConfig::Builder::security(SecurityConfig security) {
    config_.security_config_ = std::move(security);
    return *this;
}

ClientConfig::Builder& ClientConfig::Builder::rate_limiting(bool enabled, double requests_per_sec, size_t burst_size) {
    if (enabled) {
        config_.rate_limiter_ = std::make_shared<RateLimiter>(requests_per_sec, burst_size, true);
    } else {
        config_.rate_limiter_.reset();
    }
    return *this;
}

ClientConfig::Builder& ClientConfig::Builder::use_queue(bool enable) {
    config_.use_queue_ = enable;
    return *this;
}

// Legacy methods (deprecated)
ClientConfig::Builder& ClientConfig::Builder::reconnect_attempts(int attempts) {
    legacy_attempts_ = attempts;
    legacy_mode_ = true;
    return *this;
}

ClientConfig::Builder& ClientConfig::Builder::reconnect_delay(std::chrono::milliseconds delay) {
    legacy_delay_ = delay;
    legacy_mode_ = true;
    return *this;
}

ClientConfig ClientConfig::Builder::build() {
    // If no backoff strategy set, create one
    if (!config_.backoff_strategy_) {
        if (legacy_mode_) {
            // Use legacy fixed delay for backward compatibility
            config_.backoff_strategy_ = std::make_shared<FixedBackoff>(
                legacy_delay_, legacy_attempts_);
        } else {
            // Use conservative exponential backoff as default
            config_.backoff_strategy_ = ExponentialBackoff::conservative();
        }
    }
    return config_;
}

} // namespace kraken
