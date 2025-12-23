/// @file snapshots.cpp
/// @brief Data snapshot management

#include "internal/client_impl.hpp"

namespace kraken {

std::optional<Ticker> KrakenClient::Impl::latest_ticker(const std::string& symbol) const {
    std::shared_lock lock(snapshots_mutex_);
    auto it = latest_tickers_.find(symbol);
    if (it != latest_tickers_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::optional<OrderBook> KrakenClient::Impl::latest_book(const std::string& symbol) const {
    std::shared_lock lock(snapshots_mutex_);
    auto it = latest_books_.find(symbol);
    if (it != latest_books_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::shared_ptr<Telemetry> KrakenClient::Impl::get_telemetry_instance() const {
    return telemetry_;
}

std::unordered_map<std::string, Ticker> KrakenClient::Impl::all_tickers() const {
    std::shared_lock lock(snapshots_mutex_);
    return latest_tickers_;
}

uint64_t KrakenClient::Impl::gap_count() const {
    return gap_tracker_.gap_count();
}

} // namespace kraken

