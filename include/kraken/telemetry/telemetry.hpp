/// @file telemetry.hpp
/// @brief Main telemetry interface
/// 
/// Provides a unified interface for metrics, traces, and logs.
/// Can be used standalone or integrated with OpenTelemetry SDK.
///
/// @note This is a lightweight abstraction that works with or without
/// the full OpenTelemetry SDK. When KRAKEN_ENABLE_TELEMETRY is not defined,
/// all telemetry calls become no-ops with zero overhead.
///
/// This is the main include file for the telemetry module.
/// It includes all telemetry components for convenience.

#pragma once

#include "config.hpp"
#include "metrics_collector.hpp"
#include "prometheus_server.hpp"
#include "otlp_exporter.hpp"
#include <memory>
#include <string>

namespace kraken {

/// Main telemetry interface
/// 
/// Provides a unified interface for metrics, traces, and logs.
/// Can be used standalone or integrated with OpenTelemetry SDK.
/// 
/// @example
/// auto telemetry = kraken::Telemetry::create(
///     kraken::TelemetryConfig::Builder()
///         .otlp_endpoint("http://localhost:4317")
///         .service_name("my-trading-bot")
///         .metrics(true)
///         .build()
/// );
/// 
/// // Access metrics
/// telemetry->metrics().increment_messages_received();
/// std::cout << telemetry->metrics().to_prometheus();
class Telemetry : public std::enable_shared_from_this<Telemetry> {
public:
    /// Create telemetry with configuration (use this instead of constructor)
    static std::shared_ptr<Telemetry> create(TelemetryConfig config = {}) {
        return std::shared_ptr<Telemetry>(new Telemetry(std::move(config)));
    }
    
    ~Telemetry();
    
    /// Get metrics collector
    MetricsCollector& metrics() { return metrics_; }
    const MetricsCollector& metrics() const { return metrics_; }
    
    /// Get configuration
    const TelemetryConfig& config() const { return config_; }
    
    /// Check if telemetry is enabled
    bool is_enabled() const { return enabled_; }
    
    /// Enable/disable telemetry
    void set_enabled(bool enabled) { enabled_ = enabled; }
    
    /// Start telemetry services (HTTP server, OTLP export)
    /// @return true if started successfully
    bool start();
    
    /// Stop telemetry services
    void stop();
    
    /// Flush metrics to OTLP endpoint
    /// @return true if export succeeded
    bool flush();
    
    /// Get Prometheus metrics endpoint content
    std::string prometheus_metrics() const {
        return metrics_.to_prometheus();
    }
    
    /// Check if HTTP server is running
    bool is_http_server_running() const;
    
    /// Get HTTP server port
    uint16_t http_server_port() const;

private:
    // Private constructor - use create() instead
    explicit Telemetry(TelemetryConfig config);
    
    TelemetryConfig config_;
    MetricsCollector metrics_;
    bool enabled_ = true;
    
    // PIMPL for HTTP server and OTLP exporter
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace kraken
