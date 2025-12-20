#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <functional>
#include <limits>

namespace kraken {

//------------------------------------------------------------------------------
// Enums
//------------------------------------------------------------------------------

/// Channel types for subscription
enum class Channel {
    Ticker,
    Trade,
    Book,
    OHLC
};

/// Order side
enum class Side {
    Buy,
    Sell
};

/// Order type
enum class OrderType {
    Market,
    Limit
};

/// Error codes
enum class ErrorCode {
    None = 0,
    ConnectionFailed,
    ConnectionClosed,
    AuthenticationFailed,
    InvalidSymbol,
    InvalidOrder,
    RateLimited,
    QueueOverflow,
    ChecksumMismatch,
    ParseError,
    Timeout,
    CallbackError  // User callback threw an exception
};

/// Connection state
enum class ConnectionState {
    Disconnected,
    Connecting,
    Connected,
    Reconnecting
};

//------------------------------------------------------------------------------
// Data Types
//------------------------------------------------------------------------------

/// Ticker data from Kraken
struct Ticker {
    std::string symbol;
    double bid = 0.0;
    double ask = 0.0;
    double last = 0.0;
    double volume_24h = 0.0;
    double high_24h = 0.0;
    double low_24h = 0.0;
    std::string timestamp;
    
    /// Calculate bid-ask spread
    double spread() const { return ask - bid; }
    
    /// Calculate mid price
    double mid_price() const { return (bid + ask) / 2.0; }
};

/// Trade data
struct Trade {
    std::string symbol;
    double price = 0.0;
    double quantity = 0.0;
    Side side = Side::Buy;
    std::string timestamp;
};

/// Price level in order book
struct PriceLevel {
    double price = 0.0;
    double quantity = 0.0;
};

/// Order book snapshot
struct OrderBook {
    std::string symbol;
    std::vector<PriceLevel> bids;
    std::vector<PriceLevel> asks;
    bool is_valid = true;  // CRC32 checksum passed
    uint32_t checksum = 0;
    
    /// Get best bid (highest buy price)
    const PriceLevel* best_bid() const {
        return bids.empty() ? nullptr : &bids.front();
    }
    
    /// Get best ask (lowest sell price)
    const PriceLevel* best_ask() const {
        return asks.empty() ? nullptr : &asks.front();
    }
    
    /// Calculate spread
    double spread() const {
        if (bids.empty() || asks.empty()) return 0.0;
        return asks.front().price - bids.front().price;
    }
    
    /// Calculate mid price
    double mid_price() const {
        if (bids.empty() || asks.empty()) return 0.0;
        return (bids.front().price + asks.front().price) / 2.0;
    }
};

/// OHLC candle data
struct OHLC {
    std::string symbol;
    double open = 0.0;
    double high = 0.0;
    double low = 0.0;
    double close = 0.0;
    double volume = 0.0;
    int64_t timestamp = 0;
    int interval = 0;  // Minutes
};

//------------------------------------------------------------------------------
// Callback Types
//------------------------------------------------------------------------------

using TickerCallback = std::function<void(const Ticker&)>;
using TradeCallback = std::function<void(const Trade&)>;
using BookCallback = std::function<void(const std::string& symbol, const OrderBook&)>;
using OHLCCallback = std::function<void(const OHLC&)>;

//------------------------------------------------------------------------------
// Utility Functions
//------------------------------------------------------------------------------

/// Convert Channel to string
inline const char* to_string(Channel channel) {
    switch (channel) {
        case Channel::Ticker: return "ticker";
        case Channel::Trade:  return "trade";
        case Channel::Book:   return "book";
        case Channel::OHLC:   return "ohlc";
        default:              return "unknown";
    }
}

/// Convert Side to string
inline const char* to_string(Side side) {
    switch (side) {
        case Side::Buy:  return "buy";
        case Side::Sell: return "sell";
        default:         return "unknown";
    }
}

/// Convert ConnectionState to string
inline const char* to_string(ConnectionState state) {
    switch (state) {
        case ConnectionState::Disconnected:  return "disconnected";
        case ConnectionState::Connecting:    return "connecting";
        case ConnectionState::Connected:     return "connected";
        case ConnectionState::Reconnecting:  return "reconnecting";
        default:                             return "unknown";
    }
}

} // namespace kraken

