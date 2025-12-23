/// @file otlp_exporter.hpp
/// @brief OTLP HTTP exporter for metrics

#pragma once

#include "config.hpp"
#include <memory>

namespace kraken {

// Forward declaration
class Telemetry;

/// OTLP HTTP exporter for metrics
/// 
/// Exports metrics to OTLP-compatible backends via HTTP/HTTPS.
/// Runs in a background thread and periodically exports metrics.
class OtlpHttpExporter {
public:
    explicit OtlpHttpExporter(const TelemetryConfig& config);
    ~OtlpHttpExporter();
    
    /// Start the exporter
    /// @param telemetry Shared pointer to telemetry instance
    /// @return true if started successfully
    bool start(std::shared_ptr<Telemetry> telemetry);
    
    /// Stop the exporter
    void stop();
    
    /// Flush metrics immediately
    /// @return true if export succeeded
    bool flush();

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace kraken

