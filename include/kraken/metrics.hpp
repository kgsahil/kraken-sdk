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
};

} // namespace kraken

