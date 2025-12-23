/// @file metrics_collector.cpp
/// @brief MetricsCollector implementation

#include "kraken/telemetry/metrics_collector.hpp"
#include <array>
#include <cstdio>

namespace kraken {

std::string MetricsCollector::to_prometheus() const {
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

std::string MetricsCollector::to_json() const {
    std::array<char, 1024> buf{};
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg,hicpp-vararg) - snprintf is safe here
    const int result = snprintf(buf.data(), buf.size(),
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
    (void)result;  // Suppress unused result warning
    return {buf.data()};
}

} // namespace kraken

