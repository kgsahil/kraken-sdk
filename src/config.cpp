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

ClientConfig::Builder& ClientConfig::Builder::reconnect_attempts(int attempts) {
    config_.reconnect_attempts_ = attempts;
    return *this;
}

ClientConfig::Builder& ClientConfig::Builder::reconnect_delay(std::chrono::milliseconds delay) {
    config_.reconnect_delay_ = delay;
    return *this;
}

ClientConfig::Builder& ClientConfig::Builder::validate_checksums(bool validate) {
    config_.validate_checksums_ = validate;
    return *this;
}

ClientConfig ClientConfig::Builder::build() {
    return config_;
}

} // namespace kraken

