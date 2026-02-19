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
#include <unordered_map>

namespace kraken {

//------------------------------------------------------------------------------
// Enums
//------------------------------------------------------------------------------

/// @brief Channel types for WebSocket subscription
/// 
/// Defines the available data channels that can be subscribed to.
enum class Channel : std::uint8_t {
    Ticker,      ///< Real-time ticker updates (bid, ask, last, volume)
    Trade,       ///< Trade execution updates
    Book,        ///< Order book updates (bids and asks)
    OHLC,        ///< OHLC (Open/High/Low/Close) candle data
    OwnTrades,   ///< User's executed trades (private, requires auth)
    OpenOrders,  ///< User's open orders (private, requires auth)
    Balances     ///< Account balances (private, requires auth)
};

/// @brief Order side (buy or sell)
enum class Side : std::uint8_t {
    Buy,   ///< Buy order
    Sell   ///< Sell order
};

/// @brief Order type
enum class OrderType : std::uint8_t {
    Market,  ///< Market order (execute immediately at current price)
    Limit    ///< Limit order (execute at specified price or better)
};

/// @brief Error codes for SDK operations
/// 
/// Used in Error struct and exceptions to categorize failures.
enum class ErrorCode : std::uint8_t {
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
        // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,hicpp-avoid-c-arrays,modernize-avoid-c-arrays) - snprintf requires C array
        std::array<char, 512> buf{};
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg,hicpp-vararg,cert-err33-c) - snprintf is safe here
        const int result = snprintf(buf.data(), buf.size(),
            R"({"symbol":"%s","bid":%.8f,"ask":%.8f,"last":%.8f,"volume_24h":%.2f,"high_24h":%.8f,"low_24h":%.8f,"spread":%.8f,"mid":%.8f,"timestamp":"%s"})",
            symbol.c_str(), bid, ask, last, volume_24h, high_24h, low_24h, 
            spread(), mid_price(), timestamp.c_str());
        (void)result;  // Suppress unused result warning
        return {buf.data()};
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
        // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,hicpp-avoid-c-arrays,modernize-avoid-c-arrays) - snprintf requires C array
        std::array<char, 256> buf{};
        const char* side_str = (side == Side::Buy) ? "buy" : "sell";
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg,hicpp-vararg,cert-err33-c) - snprintf is safe here
        const int result = snprintf(buf.data(), buf.size(),
            R"({"symbol":"%s","price":%.8f,"quantity":%.8f,"side":"%s","value":%.2f,"timestamp":"%s"})",
            symbol.c_str(), price, quantity, side_str, value(), timestamp.c_str());
        (void)result;  // Suppress unused result warning
        return {buf.data()};
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
            // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,hicpp-avoid-c-arrays,modernize-avoid-c-arrays) - snprintf requires C array
            std::array<char, 64> buf{};
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg,hicpp-vararg,cert-err33-c) - snprintf is safe here
            const int result = snprintf(buf.data(), buf.size(), "[%.8f,%.8f]", bids[i].price, bids[i].quantity);
            (void)result;  // Suppress unused result warning
            json += buf.data();
        }
        json += R"(],"asks":[)";
        n = std::min(levels, asks.size());
        for (size_t i = 0; i < n; ++i) {
            if (i > 0) json += ",";
            // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,hicpp-avoid-c-arrays,modernize-avoid-c-arrays) - snprintf requires C array
            std::array<char, 64> buf{};
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg,hicpp-vararg,cert-err33-c) - snprintf is safe here
            const int result = snprintf(buf.data(), buf.size(), "[%.8f,%.8f]", asks[i].price, asks[i].quantity);
            (void)result;  // Suppress unused result warning
            json += buf.data();
        }
        // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,hicpp-avoid-c-arrays,modernize-avoid-c-arrays) - snprintf requires C array
        std::array<char, 128> buf{};
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg,hicpp-vararg,cert-err33-c) - snprintf is safe here
        const int result = snprintf(buf.data(), buf.size(), R"(],"spread":%.8f,"mid":%.8f,"imbalance":%.4f,"valid":%s})",
            spread(), mid_price(), imbalance(levels), is_valid ? "true" : "false");
        (void)result;  // Suppress unused result warning
        json += buf.data();
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

/// @brief Order status
enum class OrderStatus : std::uint8_t {
    Pending,    ///< Order is pending
    Open,       ///< Order is open (active)
    Closed,     ///< Order is closed (filled or cancelled)
    Cancelled,  ///< Order was cancelled
    Expired     ///< Order expired
};

/// @brief User's open order
/// 
/// Represents an active order placed by the user.
struct Order {
    std::string order_id;      ///< Unique order identifier
    std::string symbol;        ///< Trading pair (e.g., "BTC/USD")
    Side side = Side::Buy;     ///< Buy or sell side
    OrderType type = OrderType::Limit;  ///< Market or limit order
    OrderStatus status = OrderStatus::Open;  ///< Order status
    double price = 0.0;        ///< Order price (for limit orders)
    double quantity = 0.0;     ///< Order quantity
    double filled = 0.0;        ///< Quantity already filled
    double remaining = 0.0;    ///< Remaining quantity to fill
    std::string timestamp;     ///< Order timestamp
    std::string userref;       ///< User reference ID (optional)
    
    /// @brief Get fill percentage (0.0 to 100.0)
    /// @return Fill percentage
    double fill_percentage() const {
        if (quantity <= 0.0) return 0.0;
        return (filled / quantity) * 100.0;
    }
    
    /// @brief Check if order is fully filled
    /// @return true if filled >= quantity
    bool is_filled() const {
        return filled >= quantity;
    }
    
    /// @brief Convert to JSON string for web integration
    /// @return JSON representation of the order
    std::string to_json() const {
        // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,hicpp-avoid-c-arrays,modernize-avoid-c-arrays) - snprintf requires C array
        std::array<char, 512> buf{};
        const char* side_str = (side == Side::Buy) ? "buy" : "sell";
        const char* type_str = (type == OrderType::Market) ? "market" : "limit";
        const char* status_str = "open";
        switch (status) {
            case OrderStatus::Pending: status_str = "pending"; break;
            case OrderStatus::Open: status_str = "open"; break;
            case OrderStatus::Closed: status_str = "closed"; break;
            case OrderStatus::Cancelled: status_str = "cancelled"; break;
            case OrderStatus::Expired: status_str = "expired"; break;
        }
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg,hicpp-vararg,cert-err33-c) - snprintf is safe here
        const int result = snprintf(buf.data(), buf.size(),
            R"({"order_id":"%s","symbol":"%s","side":"%s","type":"%s","status":"%s","price":%.8f,"quantity":%.8f,"filled":%.8f,"remaining":%.8f,"fill_percent":%.2f,"timestamp":"%s"})",
            order_id.c_str(), symbol.c_str(), side_str, type_str, status_str,
            price, quantity, filled, remaining, fill_percentage(), timestamp.c_str());
        (void)result;  // Suppress unused result warning
        return {buf.data()};
    }
};

/// @brief User's executed trade
/// 
/// Represents a trade that was executed for the user's account.
struct OwnTrade {
    std::string trade_id;      ///< Unique trade identifier
    std::string order_id;      ///< Associated order ID
    std::string symbol;        ///< Trading pair (e.g., "BTC/USD")
    Side side = Side::Buy;     ///< Buy or sell side
    double price = 0.0;        ///< Execution price
    double quantity = 0.0;     ///< Trade quantity
    double fee = 0.0;          ///< Trading fee
    std::string fee_currency;  ///< Currency of the fee
    std::string timestamp;     ///< Trade timestamp
    
    /// @brief Get trade value (price × quantity)
    /// @return Total trade value
    double value() const { return price * quantity; }
    
    /// @brief Get net value (value - fee)
    /// @return Net trade value after fees
    double net_value() const { return value() - fee; }
    
    /// @brief Convert to JSON string for web integration
    /// @return JSON representation of the trade
    std::string to_json() const {
        // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,hicpp-avoid-c-arrays,modernize-avoid-c-arrays) - snprintf requires C array
        std::array<char, 512> buf{};
        const char* side_str = (side == Side::Buy) ? "buy" : "sell";
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg,hicpp-vararg,cert-err33-c) - snprintf is safe here
        const int result = snprintf(buf.data(), buf.size(),
            R"({"trade_id":"%s","order_id":"%s","symbol":"%s","side":"%s","price":%.8f,"quantity":%.8f,"value":%.2f,"fee":%.8f,"fee_currency":"%s","net_value":%.2f,"timestamp":"%s"})",
            trade_id.c_str(), order_id.c_str(), symbol.c_str(), side_str,
            price, quantity, value(), fee, fee_currency.c_str(), net_value(), timestamp.c_str());
        (void)result;  // Suppress unused result warning
        return {buf.data()};
    }
};

/// @brief Account balance for a currency
/// 
/// Represents the balance of a specific currency in the user's account.
struct Balance {
    std::string currency;      ///< Currency code (e.g., "BTC", "USD")
    double available = 0.0;    ///< Available balance (can be used for trading)
    double reserved = 0.0;     ///< Reserved balance (locked in orders)
    double total = 0.0;        ///< Total balance (available + reserved)
    
    /// @brief Convert to JSON string for web integration
    /// @return JSON representation of the balance
    std::string to_json() const {
        // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,hicpp-avoid-c-arrays,modernize-avoid-c-arrays) - snprintf requires C array
        std::array<char, 256> buf{};
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg,hicpp-vararg,cert-err33-c) - snprintf is safe here
        const int result = snprintf(buf.data(), buf.size(),
            R"({"currency":"%s","available":%.8f,"reserved":%.8f,"total":%.8f})",
            currency.c_str(), available, reserved, total);
        (void)result;  // Suppress unused result warning
        return {buf.data()};
    }
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

/// @brief Callback for order updates (open orders)
/// @param order The order data
using OrderCallback = std::function<void(const Order&)>;

/// @brief Callback for user's executed trades
/// @param trade The trade data
using OwnTradeCallback = std::function<void(const OwnTrade&)>;

/// @brief Callback for balance updates
/// @param balances Map of currency to balance
using BalanceCallback = std::function<void(const std::unordered_map<std::string, Balance>&)>;

/// @brief Callback for subscription confirmation
/// @param channel The confirmed channel name (e.g., "ticker", "book")
/// @param symbols The confirmed trading pairs
using SubscribedCallback = std::function<void(const std::string& channel, 
                                              const std::vector<std::string>& symbols)>;

//------------------------------------------------------------------------------
// Utility Functions
//------------------------------------------------------------------------------

/// @brief Convert Channel enum to string
/// @param channel The channel enum value
/// @return String representation (e.g., "ticker", "trade")
inline const char* to_string(Channel channel) {
    switch (channel) {
        case Channel::Ticker:     return "ticker";
        case Channel::Trade:       return "trade";
        case Channel::Book:        return "book";
        case Channel::OHLC:        return "ohlc";
        case Channel::OwnTrades:   return "ownTrades";
        case Channel::OpenOrders:  return "openOrders";
        case Channel::Balances:    return "balances";
        default:                    return "unknown";
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

