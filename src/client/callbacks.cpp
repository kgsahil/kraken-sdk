/// @file callbacks.cpp
/// @brief Callback registration and safe invocation

#include "internal/client_impl.hpp"
#include "internal/connection.hpp"
#include "kraken/core/error.hpp"

namespace kraken {

void KrakenClient::Impl::on_ticker(TickerCallback callback) {
    std::unique_lock lock(callbacks_mutex_);
    ticker_callback_ = std::move(callback);
}

void KrakenClient::Impl::on_trade(TradeCallback callback) {
    std::unique_lock lock(callbacks_mutex_);
    trade_callback_ = std::move(callback);
}

void KrakenClient::Impl::on_book(BookCallback callback) {
    std::unique_lock lock(callbacks_mutex_);
    book_callback_ = std::move(callback);
}

void KrakenClient::Impl::on_ohlc(OHLCCallback callback) {
    std::unique_lock lock(callbacks_mutex_);
    ohlc_callback_ = std::move(callback);
}

void KrakenClient::Impl::on_error(ErrorCallback callback) {
    std::unique_lock lock(callbacks_mutex_);
    error_callback_ = std::move(callback);
}

void KrakenClient::Impl::on_connection_state(ConnectionStateCallback callback) {
    std::unique_lock lock(callbacks_mutex_);
    state_callback_ = std::move(callback);
}

void KrakenClient::Impl::safe_invoke_error_callback(ErrorCode code, 
                                                     const std::string& message,
                                                     const std::string& details) {
    std::shared_lock lock(callbacks_mutex_);
    if (error_callback_) {
        Error err{code, message, details};
        try {
            error_callback_(err);
        } catch (...) {
            // Error callback threw - ignore to prevent crash
        }
    }
}

void KrakenClient::Impl::safe_send_message(const std::string& message) {
    if (!connection_ || !connection_->is_open()) {
        safe_invoke_error_callback(ErrorCode::ConnectionClosed, 
                                   "Cannot send: connection not open", "");
        return;
    }
    
    try {
        connection_->send(message);
    } catch (const std::exception& e) {
        safe_invoke_error_callback(ErrorCode::ConnectionClosed, 
                                   std::string("Send failed: ") + e.what(), "");
    }
}

} // namespace kraken

