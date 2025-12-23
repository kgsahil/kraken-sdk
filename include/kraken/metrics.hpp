/// @file metrics.hpp
/// @brief Runtime metrics for SDK performance monitoring
/// 
/// Provides real-time statistics about message processing, queue depth,
/// connection state, and latency.

#pragma once

#include "core/types.hpp"
#include <chrono>
#include <atomic>

namespace kraken {

/// @brief Runtime metrics for monitoring SDK performance
/// 
/// Provides real-time statistics about message processing, queue depth,
/// connection state, and latency. Updated atomically for thread-safe access.
struct Metrics {
    uint64_t messages_received = 0;   ///< Total messages received from WebSocket
    uint64_t messages_processed = 0;  ///< Total messages processed successfully
    uint64_t messages_dropped = 0;    ///< Messages dropped due to queue overflow
    size_t queue_depth = 0;            ///< Current queue depth (approximate)
    ConnectionState connection_state = ConnectionState::Disconnected;  ///< Current connection state
    std::chrono::microseconds latency_max_us{0};  ///< Maximum processing latency observed
    std::chrono::steady_clock::time_point start_time;  ///< SDK start time
    
    /// @brief Calculate messages per second (approximate)
    /// @return Messages per second since start, or 0.0 if just started
    double messages_per_second() const {
        auto elapsed = std::chrono::steady_clock::now() - start_time;
        auto seconds = std::chrono::duration<double>(elapsed).count();
        if (seconds < 0.001) return 0.0;
        return static_cast<double>(messages_processed) / seconds;
    }
    
    /// @brief Get uptime in seconds
    /// @return Uptime duration since SDK start
    std::chrono::seconds uptime() const {
        auto elapsed = std::chrono::steady_clock::now() - start_time;
        return std::chrono::duration_cast<std::chrono::seconds>(elapsed);
    }
    
    /// @brief Format uptime as HH:MM:SS
    /// @return Human-readable uptime string (e.g., "01:23:45")
    std::string uptime_string() const {
        auto secs = uptime().count();
        int hours = secs / 3600;
        int mins = (secs % 3600) / 60;
        int s = secs % 60;
        
        char buf[16];
        snprintf(buf, sizeof(buf), "%02d:%02d:%02d", hours, mins, s);
        return std::string(buf);
    }
    
    /// @brief Convert to JSON string for web dashboards
    /// @return JSON representation of all metrics
    std::string to_json() const {
        char buf[512];
        snprintf(buf, sizeof(buf),
            R"({"messages_received":%llu,"messages_processed":%llu,"messages_dropped":%llu,"queue_depth":%zu,"connection_state":"%s","latency_max_us":%lld,"uptime_seconds":%lld,"msg_per_sec":%.2f})",
            static_cast<unsigned long long>(messages_received),
            static_cast<unsigned long long>(messages_processed),
            static_cast<unsigned long long>(messages_dropped),
            queue_depth,
            to_string(connection_state),
            static_cast<long long>(latency_max_us.count()),
            static_cast<long long>(uptime().count()),
            messages_per_second());
        return std::string(buf);
    }
};

} // namespace kraken

