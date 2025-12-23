/// @file config.hpp
/// @brief Telemetry configuration

#pragma once

#include <string>
#include <chrono>

namespace kraken {

/// Telemetry configuration for OTLP export
struct TelemetryConfig {
    std::string otlp_endpoint = "http://localhost:4317";
    std::string service_name = "kraken-sdk";
    std::string service_version = "1.0.0";
    std::string environment = "development";
    
    bool enable_metrics = true;
    bool enable_traces = false;   // Disabled by default (overhead)
    bool enable_logs = false;     // Disabled by default
    
    std::chrono::seconds metrics_interval{15};
    
    // HTTP server for Prometheus scraping
    bool enable_http_server = false;
    uint16_t http_server_port = 9090;
    
    // OTLP export settings
    bool enable_otlp_export = false;
    int max_export_retries = 3;
    std::chrono::milliseconds export_timeout{5000};
    
    /// Create a builder for fluent configuration
    class Builder;
};

class TelemetryConfig::Builder {
public:
    Builder& otlp_endpoint(std::string endpoint) {
        config_.otlp_endpoint = std::move(endpoint);
        return *this;
    }
    
    Builder& service_name(std::string name) {
        config_.service_name = std::move(name);
        return *this;
    }
    
    Builder& service_version(std::string version) {
        config_.service_version = std::move(version);
        return *this;
    }
    
    Builder& environment(std::string env) {
        config_.environment = std::move(env);
        return *this;
    }
    
    Builder& metrics(bool enabled) {
        config_.enable_metrics = enabled;
        return *this;
    }
    
    Builder& traces(bool enabled) {
        config_.enable_traces = enabled;
        return *this;
    }
    
    Builder& logs(bool enabled) {
        config_.enable_logs = enabled;
        return *this;
    }
    
    Builder& metrics_interval(std::chrono::seconds interval) {
        config_.metrics_interval = interval;
        return *this;
    }
    
    Builder& http_server(bool enabled, uint16_t port = 9090) {
        config_.enable_http_server = enabled;
        config_.http_server_port = port;
        return *this;
    }
    
    Builder& otlp_export(bool enabled) {
        config_.enable_otlp_export = enabled;
        return *this;
    }
    
    Builder& export_retries(int retries) {
        config_.max_export_retries = retries;
        return *this;
    }
    
    Builder& export_timeout(std::chrono::milliseconds timeout) {
        config_.export_timeout = timeout;
        return *this;
    }
    
    TelemetryConfig build() { return config_; }
    
private:
    TelemetryConfig config_;
};

} // namespace kraken

