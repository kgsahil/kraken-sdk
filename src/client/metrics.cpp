/// @file metrics.cpp
/// @brief Metrics collection

#include "internal/client_impl.hpp"

namespace kraken {

Metrics KrakenClient::Impl::get_metrics() const {
    Metrics m;
    
    // If telemetry is enabled, prefer telemetry metrics (they're more comprehensive)
    if (telemetry_) {
        const auto& tel_metrics = telemetry_->metrics();
        m.messages_received = tel_metrics.messages_received();
        m.messages_processed = tel_metrics.messages_processed();
        m.messages_dropped = tel_metrics.messages_dropped();
        m.queue_depth = static_cast<size_t>(tel_metrics.queue_depth());
        m.connection_state = static_cast<ConnectionState>(tel_metrics.connection_state());
        m.latency_max_us = std::chrono::microseconds(tel_metrics.latency_max_us());
    } else {
        // Fall back to direct atomic reads
        m.messages_received = msg_received_.load(std::memory_order_relaxed);
        m.messages_processed = msg_processed_.load(std::memory_order_relaxed);
        m.messages_dropped = msg_dropped_.load(std::memory_order_relaxed);
        
        // Queue size might not be thread-safe, but it's approximate anyway
        // SPSC queue size() is typically safe for reading
        if (queue_) {
            m.queue_depth = queue_->size();
        } else {
            m.queue_depth = 0;
        }
        
        m.connection_state = state_.load(std::memory_order_relaxed);
        m.latency_max_us = std::chrono::microseconds(
            latency_max_us_.load(std::memory_order_relaxed)
        );
    }
    
    m.start_time = start_time_;
    return m;
}

} // namespace kraken

