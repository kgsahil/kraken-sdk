/// @file prometheus_server.hpp
/// @brief HTTP server for Prometheus metrics scraping

#pragma once

#include <memory>
#include <cstdint>

namespace kraken {

// Forward declaration
class Telemetry;

/// HTTP server for exposing Prometheus metrics
/// 
/// Provides a simple HTTP server that exposes metrics in Prometheus
/// text format for scraping. Uses Boost.Beast for async HTTP handling.
class MetricsHttpServer {
public:
    MetricsHttpServer(std::shared_ptr<Telemetry> telemetry, uint16_t port);
    ~MetricsHttpServer();
    
    /// Start the HTTP server
    /// @return true if started successfully
    bool start();
    
    /// Stop the HTTP server
    void stop();
    
    /// Check if server is running
    bool is_running() const;
    
    /// Get the port the server is listening on
    uint16_t port() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace kraken

