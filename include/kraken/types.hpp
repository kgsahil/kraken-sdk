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
    
    /// Calculate spread as percentage of mid price
    double spread_percent() const { 
        double mid = mid_price();
        return mid > 0 ? (spread() / mid) * 100.0 : 0.0;
    }
    
    /// Calculate mid price
    double mid_price() const { return (bid + ask) / 2.0; }
    
    /// Convert to JSON string (for web integration)
    std::string to_json() const {
        char buf[512];
        snprintf(buf, sizeof(buf),
            R"({"symbol":"%s","bid":%.8f,"ask":%.8f,"last":%.8f,"volume_24h":%.2f,"high_24h":%.8f,"low_24h":%.8f,"spread":%.8f,"mid":%.8f,"timestamp":"%s"})",
            symbol.c_str(), bid, ask, last, volume_24h, high_24h, low_24h, 
            spread(), mid_price(), timestamp.c_str());
        return std::string(buf);
    }
};

/// Trade data
struct Trade {
    std::string symbol;
    double price = 0.0;
    double quantity = 0.0;
    Side side = Side::Buy;
    std::string timestamp;
    
    /// Get trade value (price × quantity)
    double value() const { return price * quantity; }
    
    /// Convert to JSON string (for web integration)
    std::string to_json() const {
        char buf[256];
        const char* side_str = (side == Side::Buy) ? "buy" : "sell";
        snprintf(buf, sizeof(buf),
            R"({"symbol":"%s","price":%.8f,"quantity":%.8f,"side":"%s","value":%.2f,"timestamp":"%s"})",
            symbol.c_str(), price, quantity, side_str, value(), timestamp.c_str());
        return std::string(buf);
    }
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
    
    /// Calculate total bid liquidity up to a depth
    double total_bid_liquidity(size_t depth = 10) const {
        double total = 0.0;
        size_t n = std::min(depth, bids.size());
        for (size_t i = 0; i < n; ++i) total += bids[i].quantity;
        return total;
    }
    
    /// Calculate total ask liquidity up to a depth
    double total_ask_liquidity(size_t depth = 10) const {
        double total = 0.0;
        size_t n = std::min(depth, asks.size());
        for (size_t i = 0; i < n; ++i) total += asks[i].quantity;
        return total;
    }
    
    /// Get bid/ask imbalance ratio (-1 to +1, positive = more bids)
    double imbalance(size_t depth = 10) const {
        double bid_liq = total_bid_liquidity(depth);
        double ask_liq = total_ask_liquidity(depth);
        double total = bid_liq + ask_liq;
        if (total < 0.0001) return 0.0;
        return (bid_liq - ask_liq) / total;
    }
    
    /// Convert to JSON string (for web integration, top N levels)
    std::string to_json(size_t levels = 10) const {
        std::string json = R"({"symbol":")" + symbol + R"(","bids":[)";
        size_t n = std::min(levels, bids.size());
        for (size_t i = 0; i < n; ++i) {
            if (i > 0) json += ",";
            char buf[64];
            snprintf(buf, sizeof(buf), "[%.8f,%.8f]", bids[i].price, bids[i].quantity);
            json += buf;
        }
        json += R"(],"asks":[)";
        n = std::min(levels, asks.size());
        for (size_t i = 0; i < n; ++i) {
            if (i > 0) json += ",";
            char buf[64];
            snprintf(buf, sizeof(buf), "[%.8f,%.8f]", asks[i].price, asks[i].quantity);
            json += buf;
        }
        char buf[128];
        snprintf(buf, sizeof(buf), R"(],"spread":%.8f,"mid":%.8f,"imbalance":%.4f,"valid":%s})",
            spread(), mid_price(), imbalance(levels), is_valid ? "true" : "false");
        json += buf;
        return json;
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

