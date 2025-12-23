/// @file metrics_collector.hpp
/// @brief Metrics collection for telemetry

#pragma once

#include <string>
#include <atomic>
#include <unordered_map>
#include <mutex>
#include <cstdint>

namespace kraken {

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
    std::string to_prometheus() const;
    
    /// Convert to JSON format
    std::string to_json() const;

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

} // namespace kraken

