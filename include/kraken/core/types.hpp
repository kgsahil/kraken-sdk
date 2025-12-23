/// @file types.hpp
/// @brief Core data types for Kraken WebSocket API
/// 
/// Defines all data structures, enums, and callback types used throughout the SDK.

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

/// @brief Channel types for WebSocket subscription
/// 
/// Defines the available data channels that can be subscribed to.
enum class Channel {
    Ticker,  ///< Real-time ticker updates (bid, ask, last, volume)
    Trade,   ///< Trade execution updates
    Book,    ///< Order book updates (bids and asks)
    OHLC     ///< OHLC (Open/High/Low/Close) candle data
};

/// @brief Order side (buy or sell)
enum class Side {
    Buy,   ///< Buy order
    Sell   ///< Sell order
};

/// @brief Order type
enum class OrderType {
    Market,  ///< Market order (execute immediately at current price)
    Limit    ///< Limit order (execute at specified price or better)
};

/// @brief Error codes for SDK operations
/// 
/// Used in Error struct and exceptions to categorize failures.
enum class ErrorCode {
    None = 0,                ///< No error
    ConnectionFailed,        ///< Failed to establish WebSocket connection
    ConnectionClosed,        ///< Connection closed unexpectedly
    AuthenticationFailed,    ///< Authentication failed (invalid API key/secret)
    InvalidSymbol,           ///< Invalid trading pair symbol
    InvalidOrder,            ///< Invalid order parameters
    RateLimited,             ///< Rate limit exceeded
    QueueOverflow,           ///< Message queue overflow (messages dropped)
    ChecksumMismatch,        ///< Order book checksum validation failed
    ParseError,              ///< JSON parsing error
    Timeout,                 ///< Operation timed out
    CallbackError            ///< User callback threw an exception
};

/// @brief Connection state machine
/// 
/// Tracks the current state of the WebSocket connection.
enum class ConnectionState {
    Disconnected,  ///< Not connected
    Connecting,    ///< Connection attempt in progress
    Connected,     ///< Connected and ready
    Reconnecting   ///< Reconnecting after disconnection
};

//------------------------------------------------------------------------------
// Data Types
//------------------------------------------------------------------------------

/// @brief Ticker data from Kraken WebSocket
/// 
/// Contains real-time market data for a trading pair including bid, ask, last price,
/// and 24-hour statistics.
struct Ticker {
    std::string symbol;      ///< Trading pair (e.g., "BTC/USD")
    double bid = 0.0;         ///< Best bid price
    double ask = 0.0;         ///< Best ask price
    double last = 0.0;        ///< Last trade price
    double volume_24h = 0.0;  ///< 24-hour trading volume
    double high_24h = 0.0;    ///< 24-hour high price
    double low_24h = 0.0;     ///< 24-hour low price
    std::string timestamp;    ///< Timestamp of the update
    
    /// @brief Calculate bid-ask spread
    /// @return Spread in price units (ask - bid)
    double spread() const { return ask - bid; }
    
    /// @brief Calculate spread as percentage of mid price
    /// @return Spread percentage (0.0 if mid price is zero)
    double spread_percent() const { 
        double mid = mid_price();
        return mid > 0 ? (spread() / mid) * 100.0 : 0.0;
    }
    
    /// @brief Calculate mid price (average of bid and ask)
    /// @return Mid price
    double mid_price() const { return (bid + ask) / 2.0; }
    
    /// @brief Convert to JSON string for web integration
    /// @return JSON representation of the ticker
    std::string to_json() const {
        char buf[512];
        snprintf(buf, sizeof(buf),
            R"({"symbol":"%s","bid":%.8f,"ask":%.8f,"last":%.8f,"volume_24h":%.2f,"high_24h":%.8f,"low_24h":%.8f,"spread":%.8f,"mid":%.8f,"timestamp":"%s"})",
            symbol.c_str(), bid, ask, last, volume_24h, high_24h, low_24h, 
            spread(), mid_price(), timestamp.c_str());
        return std::string(buf);
    }
};

/// @brief Trade execution data
/// 
/// Represents a single trade execution on the exchange.
struct Trade {
    std::string symbol;      ///< Trading pair (e.g., "BTC/USD")
    double price = 0.0;       ///< Execution price
    double quantity = 0.0;    ///< Trade quantity
    Side side = Side::Buy;    ///< Buy or sell side
    std::string timestamp;   ///< Trade timestamp
    
    /// @brief Get trade value (price × quantity)
    /// @return Total trade value
    double value() const { return price * quantity; }
    
    /// @brief Convert to JSON string for web integration
    /// @return JSON representation of the trade
    std::string to_json() const {
        char buf[256];
        const char* side_str = (side == Side::Buy) ? "buy" : "sell";
        snprintf(buf, sizeof(buf),
            R"({"symbol":"%s","price":%.8f,"quantity":%.8f,"side":"%s","value":%.2f,"timestamp":"%s"})",
            symbol.c_str(), price, quantity, side_str, value(), timestamp.c_str());
        return std::string(buf);
    }
};

/// @brief Price level in an order book
/// 
/// Represents a single price level with its quantity.
struct PriceLevel {
    double price = 0.0;      ///< Price level
    double quantity = 0.0;  ///< Available quantity at this price
};

/// @brief Order book snapshot with CRC32 checksum validation
/// 
/// Contains the current state of the order book (bids and asks) for a trading pair.
/// The order book is maintained incrementally from WebSocket updates and validated
/// using CRC32 checksums to detect data corruption.
struct OrderBook {
    std::string symbol;                    ///< Trading pair (e.g., "BTC/USD")
    std::vector<PriceLevel> bids;          ///< Bid levels (sorted descending by price)
    std::vector<PriceLevel> asks;          ///< Ask levels (sorted ascending by price)
    bool is_valid = true;                  ///< True if CRC32 checksum validation passed
    uint32_t checksum = 0;                 ///< Calculated CRC32 checksum
    
    /// @brief Get best bid (highest buy price)
    /// @return Pointer to best bid, or nullptr if empty
    const PriceLevel* best_bid() const {
        return bids.empty() ? nullptr : &bids.front();
    }
    
    /// @brief Get best ask (lowest sell price)
    /// @return Pointer to best ask, or nullptr if empty
    const PriceLevel* best_ask() const {
        return asks.empty() ? nullptr : &asks.front();
    }
    
    /// @brief Calculate spread (best ask - best bid)
    /// @return Spread in price units, or 0.0 if book is empty
    double spread() const {
        if (bids.empty() || asks.empty()) return 0.0;
        return asks.front().price - bids.front().price;
    }
    
    /// @brief Calculate mid price (average of best bid and ask)
    /// @return Mid price, or 0.0 if book is empty
    double mid_price() const {
        if (bids.empty() || asks.empty()) return 0.0;
        return (bids.front().price + asks.front().price) / 2.0;
    }
    
    /// @brief Calculate total bid liquidity up to a depth
    /// @param depth Number of levels to include (default: 10)
    /// @return Total bid quantity
    double total_bid_liquidity(size_t depth = 10) const {
        double total = 0.0;
        size_t n = std::min(depth, bids.size());
        for (size_t i = 0; i < n; ++i) total += bids[i].quantity;
        return total;
    }
    
    /// @brief Calculate total ask liquidity up to a depth
    /// @param depth Number of levels to include (default: 10)
    /// @return Total ask quantity
    double total_ask_liquidity(size_t depth = 10) const {
        double total = 0.0;
        size_t n = std::min(depth, asks.size());
        for (size_t i = 0; i < n; ++i) total += asks[i].quantity;
        return total;
    }
    
    /// @brief Get bid/ask imbalance ratio
    /// 
    /// Returns a value between -1 and +1:
    /// - Positive values indicate more bid liquidity (bullish)
    /// - Negative values indicate more ask liquidity (bearish)
    /// 
    /// @param depth Number of levels to consider (default: 10)
    /// @return Imbalance ratio (-1 to +1), or 0.0 if insufficient liquidity
    double imbalance(size_t depth = 10) const {
        double bid_liq = total_bid_liquidity(depth);
        double ask_liq = total_ask_liquidity(depth);
        double total = bid_liq + ask_liq;
        if (total < 0.0001) return 0.0;
        return (bid_liq - ask_liq) / total;
    }
    
    /// @brief Convert to JSON string for web integration
    /// @param levels Number of price levels to include (default: 10)
    /// @return JSON representation of the order book
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

/// @brief OHLC (Open/High/Low/Close) candle data
/// 
/// Represents a single candlestick/bar for a trading pair.
struct OHLC {
    std::string symbol;    ///< Trading pair (e.g., "BTC/USD")
    double open = 0.0;      ///< Opening price
    double high = 0.0;      ///< Highest price
    double low = 0.0;       ///< Lowest price
    double close = 0.0;     ///< Closing price
    double volume = 0.0;    ///< Trading volume
    int64_t timestamp = 0;  ///< Candle timestamp (Unix epoch)
    int interval = 0;       ///< Candle interval in minutes
};

//------------------------------------------------------------------------------
// Callback Types
//------------------------------------------------------------------------------

/// @brief Callback for ticker updates
/// @param ticker The ticker data
using TickerCallback = std::function<void(const Ticker&)>;

/// @brief Callback for trade updates
/// @param trade The trade data
using TradeCallback = std::function<void(const Trade&)>;

/// @brief Callback for order book updates
/// @param symbol The trading pair symbol
/// @param book The order book snapshot
using BookCallback = std::function<void(const std::string& symbol, const OrderBook&)>;

/// @brief Callback for OHLC candle updates
/// @param ohlc The OHLC data
using OHLCCallback = std::function<void(const OHLC&)>;

//------------------------------------------------------------------------------
// Utility Functions
//------------------------------------------------------------------------------

/// @brief Convert Channel enum to string
/// @param channel The channel enum value
/// @return String representation (e.g., "ticker", "trade")
inline const char* to_string(Channel channel) {
    switch (channel) {
        case Channel::Ticker: return "ticker";
        case Channel::Trade:  return "trade";
        case Channel::Book:   return "book";
        case Channel::OHLC:   return "ohlc";
        default:              return "unknown";
    }
}

/// @brief Convert Side enum to string
/// @param side The side enum value
/// @return String representation ("buy" or "sell")
inline const char* to_string(Side side) {
    switch (side) {
        case Side::Buy:  return "buy";
        case Side::Sell: return "sell";
        default:         return "unknown";
    }
}

/// @brief Convert ConnectionState enum to string
/// @param state The connection state enum value
/// @return String representation (e.g., "connected", "disconnected")
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

