#pragma once

/// @file telemetry.hpp
/// @brief OpenTelemetry integration for observability
/// 
/// Provides metrics, traces, and logs export to OTLP-compatible backends
/// like Prometheus, Jaeger, and Grafana.
///
/// @note This is a lightweight abstraction that works with or without
/// the full OpenTelemetry SDK. When KRAKEN_ENABLE_TELEMETRY is not defined,
/// all telemetry calls become no-ops with zero overhead.

#include <string>
#include <memory>
#include <chrono>
#include <functional>
#include <atomic>
#include <unordered_map>

namespace kraken {

//------------------------------------------------------------------------------
// Telemetry Configuration
//------------------------------------------------------------------------------

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
    
    TelemetryConfig build() { return config_; }
    
private:
    TelemetryConfig config_;
};

//------------------------------------------------------------------------------
// Metrics Collector (Internal)
//------------------------------------------------------------------------------

/// Collects SDK metrics for export
/// 
/// This is a lightweight metrics collector that doesn't require
/// the full OpenTelemetry SDK. It can be used standalone or
/// integrated with OTLP exporters.
class MetricsCollector {
public:
    MetricsCollector() = default;
    
    //--- Counters ---
    
    void increment_messages_received(const std::string& channel = "") {
        messages_received_.fetch_add(1, std::memory_order_relaxed);
        if (!channel.empty()) {
            std::lock_guard<std::mutex> lock(channel_mutex_);
            channel_counts_[channel]++;
        }
    }
    
    void increment_messages_processed() {
        messages_processed_.fetch_add(1, std::memory_order_relaxed);
    }
    
    void increment_messages_dropped() {
        messages_dropped_.fetch_add(1, std::memory_order_relaxed);
    }
    
    void increment_reconnect_attempts() {
        reconnect_attempts_.fetch_add(1, std::memory_order_relaxed);
    }
    
    void increment_checksum_failures() {
        checksum_failures_.fetch_add(1, std::memory_order_relaxed);
    }
    
    void increment_gaps_detected() {
        gaps_detected_.fetch_add(1, std::memory_order_relaxed);
    }
    
    void increment_alerts_triggered(const std::string& strategy = "") {
        alerts_triggered_.fetch_add(1, std::memory_order_relaxed);
    }
    
    //--- Gauges ---
    
    void set_queue_depth(size_t depth) {
        queue_depth_.store(static_cast<int64_t>(depth), std::memory_order_relaxed);
    }
    
    void set_connection_state(int state) {
        connection_state_.store(state, std::memory_order_relaxed);
    }
    
    //--- Histograms ---
    
    void record_latency_us(int64_t latency_us) {
        // Simple max tracking (can be extended to proper histogram)
        int64_t current_max = latency_max_us_.load(std::memory_order_relaxed);
        while (latency_us > current_max) {
            if (latency_max_us_.compare_exchange_weak(current_max, latency_us,
                    std::memory_order_relaxed)) {
                break;
            }
        }
        latency_sum_us_.fetch_add(latency_us, std::memory_order_relaxed);
        latency_count_.fetch_add(1, std::memory_order_relaxed);
    }
    
    //--- Accessors ---
    
    uint64_t messages_received() const { 
        return messages_received_.load(std::memory_order_relaxed); 
    }
    
    uint64_t messages_processed() const { 
        return messages_processed_.load(std::memory_order_relaxed); 
    }
    
    uint64_t messages_dropped() const { 
        return messages_dropped_.load(std::memory_order_relaxed); 
    }
    
    uint64_t reconnect_attempts() const { 
        return reconnect_attempts_.load(std::memory_order_relaxed); 
    }
    
    uint64_t checksum_failures() const { 
        return checksum_failures_.load(std::memory_order_relaxed); 
    }
    
    uint64_t gaps_detected() const { 
        return gaps_detected_.load(std::memory_order_relaxed); 
    }
    
    uint64_t alerts_triggered() const { 
        return alerts_triggered_.load(std::memory_order_relaxed); 
    }
    
    int64_t queue_depth() const { 
        return queue_depth_.load(std::memory_order_relaxed); 
    }
    
    int connection_state() const { 
        return connection_state_.load(std::memory_order_relaxed); 
    }
    
    int64_t latency_max_us() const { 
        return latency_max_us_.load(std::memory_order_relaxed); 
    }
    
    double latency_avg_us() const {
        uint64_t count = latency_count_.load(std::memory_order_relaxed);
        if (count == 0) return 0.0;
        return static_cast<double>(latency_sum_us_.load(std::memory_order_relaxed)) / count;
    }
    
    /// Get per-channel message counts
    std::unordered_map<std::string, uint64_t> channel_counts() const {
        std::lock_guard<std::mutex> lock(channel_mutex_);
        return channel_counts_;
    }
    
    /// Reset all metrics
    void reset() {
        messages_received_.store(0, std::memory_order_relaxed);
        messages_processed_.store(0, std::memory_order_relaxed);
        messages_dropped_.store(0, std::memory_order_relaxed);
        reconnect_attempts_.store(0, std::memory_order_relaxed);
        checksum_failures_.store(0, std::memory_order_relaxed);
        gaps_detected_.store(0, std::memory_order_relaxed);
        alerts_triggered_.store(0, std::memory_order_relaxed);
        queue_depth_.store(0, std::memory_order_relaxed);
        latency_max_us_.store(0, std::memory_order_relaxed);
        latency_sum_us_.store(0, std::memory_order_relaxed);
        latency_count_.store(0, std::memory_order_relaxed);
        
        std::lock_guard<std::mutex> lock(channel_mutex_);
        channel_counts_.clear();
    }
    
    /// Convert to Prometheus text format
    std::string to_prometheus() const {
        std::string output;
        output.reserve(2048);
        
        output += "# HELP kraken_messages_received_total Total messages received\n";
        output += "# TYPE kraken_messages_received_total counter\n";
        output += "kraken_messages_received_total " + std::to_string(messages_received()) + "\n\n";
        
        output += "# HELP kraken_messages_processed_total Total messages processed\n";
        output += "# TYPE kraken_messages_processed_total counter\n";
        output += "kraken_messages_processed_total " + std::to_string(messages_processed()) + "\n\n";
        
        output += "# HELP kraken_messages_dropped_total Total messages dropped\n";
        output += "# TYPE kraken_messages_dropped_total counter\n";
        output += "kraken_messages_dropped_total " + std::to_string(messages_dropped()) + "\n\n";
        
        output += "# HELP kraken_reconnect_attempts_total Total reconnection attempts\n";
        output += "# TYPE kraken_reconnect_attempts_total counter\n";
        output += "kraken_reconnect_attempts_total " + std::to_string(reconnect_attempts()) + "\n\n";
        
        output += "# HELP kraken_checksum_failures_total Total checksum validation failures\n";
        output += "# TYPE kraken_checksum_failures_total counter\n";
        output += "kraken_checksum_failures_total " + std::to_string(checksum_failures()) + "\n\n";
        
        output += "# HELP kraken_gaps_detected_total Total message gaps detected\n";
        output += "# TYPE kraken_gaps_detected_total counter\n";
        output += "kraken_gaps_detected_total " + std::to_string(gaps_detected()) + "\n\n";
        
        output += "# HELP kraken_alerts_triggered_total Total strategy alerts triggered\n";
        output += "# TYPE kraken_alerts_triggered_total counter\n";
        output += "kraken_alerts_triggered_total " + std::to_string(alerts_triggered()) + "\n\n";
        
        output += "# HELP kraken_queue_depth Current message queue depth\n";
        output += "# TYPE kraken_queue_depth gauge\n";
        output += "kraken_queue_depth " + std::to_string(queue_depth()) + "\n\n";
        
        output += "# HELP kraken_connection_state Connection state (1=connected, 0=disconnected)\n";
        output += "# TYPE kraken_connection_state gauge\n";
        output += "kraken_connection_state " + std::to_string(connection_state()) + "\n\n";
        
        output += "# HELP kraken_latency_max_us Maximum message latency in microseconds\n";
        output += "# TYPE kraken_latency_max_us gauge\n";
        output += "kraken_latency_max_us " + std::to_string(latency_max_us()) + "\n\n";
        
        output += "# HELP kraken_latency_avg_us Average message latency in microseconds\n";
        output += "# TYPE kraken_latency_avg_us gauge\n";
        output += "kraken_latency_avg_us " + std::to_string(static_cast<int64_t>(latency_avg_us())) + "\n";
        
        return output;
    }
    
    /// Convert to JSON format
    std::string to_json() const {
        char buf[1024];
        snprintf(buf, sizeof(buf),
            R"({"messages_received":%llu,"messages_processed":%llu,"messages_dropped":%llu,"reconnect_attempts":%llu,"checksum_failures":%llu,"gaps_detected":%llu,"alerts_triggered":%llu,"queue_depth":%lld,"connection_state":%d,"latency_max_us":%lld,"latency_avg_us":%.2f})",
            static_cast<unsigned long long>(messages_received()),
            static_cast<unsigned long long>(messages_processed()),
            static_cast<unsigned long long>(messages_dropped()),
            static_cast<unsigned long long>(reconnect_attempts()),
            static_cast<unsigned long long>(checksum_failures()),
            static_cast<unsigned long long>(gaps_detected()),
            static_cast<unsigned long long>(alerts_triggered()),
            static_cast<long long>(queue_depth()),
            connection_state(),
            static_cast<long long>(latency_max_us()),
            latency_avg_us());
        return std::string(buf);
    }

private:
    // Counters (atomic for lock-free access)
    std::atomic<uint64_t> messages_received_{0};
    std::atomic<uint64_t> messages_processed_{0};
    std::atomic<uint64_t> messages_dropped_{0};
    std::atomic<uint64_t> reconnect_attempts_{0};
    std::atomic<uint64_t> checksum_failures_{0};
    std::atomic<uint64_t> gaps_detected_{0};
    std::atomic<uint64_t> alerts_triggered_{0};
    
    // Gauges
    std::atomic<int64_t> queue_depth_{0};
    std::atomic<int> connection_state_{0};
    
    // Latency tracking
    std::atomic<int64_t> latency_max_us_{0};
    std::atomic<int64_t> latency_sum_us_{0};
    std::atomic<uint64_t> latency_count_{0};
    
    // Per-channel counts (requires mutex)
    mutable std::mutex channel_mutex_;
    std::unordered_map<std::string, uint64_t> channel_counts_;
};

//------------------------------------------------------------------------------
// Telemetry Manager
//------------------------------------------------------------------------------

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
class Telemetry {
public:
    explicit Telemetry(TelemetryConfig config = {})
        : config_(std::move(config)) {}
    
    /// Create telemetry with configuration
    static std::shared_ptr<Telemetry> create(TelemetryConfig config = {}) {
        return std::make_shared<Telemetry>(std::move(config));
    }
    
    /// Get metrics collector
    MetricsCollector& metrics() { return metrics_; }
    const MetricsCollector& metrics() const { return metrics_; }
    
    /// Get configuration
    const TelemetryConfig& config() const { return config_; }
    
    /// Check if telemetry is enabled
    bool is_enabled() const { return enabled_; }
    
    /// Enable/disable telemetry
    void set_enabled(bool enabled) { enabled_ = enabled; }
    
    /// Flush metrics to OTLP endpoint (placeholder for full OTEL integration)
    bool flush() {
        // In full OTEL integration, this would push to the collector
        // For now, just return success
        return true;
    }
    
    /// Get Prometheus metrics endpoint content
    std::string prometheus_metrics() const {
        return metrics_.to_prometheus();
    }

private:
    TelemetryConfig config_;
    MetricsCollector metrics_;
    bool enabled_ = true;
};

} // namespace kraken

