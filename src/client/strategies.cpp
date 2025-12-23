/// @file strategies.cpp
/// @brief Strategy management (client wrapper)

#include "internal/client_impl.hpp"

namespace kraken {

int KrakenClient::Impl::add_alert(std::shared_ptr<AlertStrategy> strategy, 
                                   AlertCallback callback) {
    // Wrap callback to track telemetry if enabled
    AlertCallback wrapped_callback = callback;
    if (telemetry_) {
        wrapped_callback = [this, callback](const Alert& alert) {
            // Update telemetry
            telemetry_->metrics().increment_alerts_triggered(alert.strategy_name);
            // Call original callback
            callback(alert);
        };
    }
    return strategy_engine_.add(std::move(strategy), std::move(wrapped_callback));
}

void KrakenClient::Impl::remove_alert(int alert_id) {
    strategy_engine_.remove(alert_id);
}

void KrakenClient::Impl::enable_alert(int alert_id) {
    strategy_engine_.enable(alert_id);
}

void KrakenClient::Impl::disable_alert(int alert_id) {
    strategy_engine_.disable(alert_id);
}

bool KrakenClient::Impl::is_alert_enabled(int alert_id) const {
    return strategy_engine_.is_enabled(alert_id);
}

size_t KrakenClient::Impl::alert_count() const {
    return strategy_engine_.count();
}

std::vector<std::pair<int, std::string>> KrakenClient::Impl::get_alerts() const {
    return strategy_engine_.get_alerts();
}

} // namespace kraken

