#pragma once

#include "types.hpp"
#include <chrono>
#include <atomic>

namespace kraken {

/// Runtime metrics for monitoring SDK performance
struct Metrics {
    uint64_t messages_received = 0;
    uint64_t messages_processed = 0;
    uint64_t messages_dropped = 0;
    size_t queue_depth = 0;
    ConnectionState connection_state = ConnectionState::Disconnected;
    std::chrono::microseconds latency_max_us{0};
    std::chrono::steady_clock::time_point start_time;
    
    /// Calculate messages per second (approximate)
    double messages_per_second() const {
        auto elapsed = std::chrono::steady_clock::now() - start_time;
        auto seconds = std::chrono::duration<double>(elapsed).count();
        if (seconds < 0.001) return 0.0;
        return static_cast<double>(messages_processed) / seconds;
    }
    
    /// Get uptime in seconds
    std::chrono::seconds uptime() const {
        auto elapsed = std::chrono::steady_clock::now() - start_time;
        return std::chrono::duration_cast<std::chrono::seconds>(elapsed);
    }
    
    /// Format uptime as HH:MM:SS
    std::string uptime_string() const {
        auto secs = uptime().count();
        int hours = secs / 3600;
        int mins = (secs % 3600) / 60;
        int s = secs % 60;
        
        char buf[16];
        snprintf(buf, sizeof(buf), "%02d:%02d:%02d", hours, mins, s);
        return std::string(buf);
    }
    
    /// Convert to JSON string (for web dashboards)
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

