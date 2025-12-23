/// @file base.hpp
/// @brief Base classes and types for trading strategies
/// 
/// Defines the AlertStrategy base class and Alert struct that all strategies
/// inherit from and use.

#pragma once

#include "../core/types.hpp"
#include <memory>
#include <vector>
#include <string>
#include <functional>
#include <chrono>

namespace kraken {

// Forward declarations
struct OrderBook;
struct Trade;
struct OHLC;

//------------------------------------------------------------------------------
// Alert Types
//------------------------------------------------------------------------------

/// @brief Alert information when a strategy condition is met
/// 
/// Contains details about the alert trigger including strategy name,
/// symbol, price, and timestamp.
struct Alert {
    std::string strategy_name;  ///< Name of the strategy that triggered
    std::string symbol;         ///< Trading pair symbol
    double price = 0.0;         ///< Price at which alert triggered
    std::string message;        ///< Alert message
    std::chrono::system_clock::time_point timestamp;  ///< Alert timestamp
    
    /// @brief Convert to JSON string for web integration
    /// @return JSON representation of the alert
    std::string to_json() const {
        auto ts = std::chrono::duration_cast<std::chrono::milliseconds>(
            timestamp.time_since_epoch()).count();
        std::string json = R"({"strategy":")" + strategy_name + R"(","symbol":")" + symbol +
            R"(","price":)" + std::to_string(price) + R"(,"message":")" + message +
            R"(","timestamp_ms":)" + std::to_string(static_cast<int64_t>(ts)) + "}";
        return json;
    }
};

/// @brief Callback type for alerts
/// 
/// Called when a strategy condition is met.
/// 
/// @param alert Alert information
using AlertCallback = std::function<void(const Alert&)>;

//------------------------------------------------------------------------------
// AlertStrategy Base Class
//------------------------------------------------------------------------------

/// @brief Base class for all trading alert strategies
/// 
/// Users can create custom strategies by inheriting from this class and
/// implementing the required virtual methods.
/// 
/// @example
/// @code
/// class MyStrategy : public AlertStrategy {
///     bool check(const Ticker& ticker) override {
///         return ticker.last > 50000.0;
///     }
///     std::string name() const override { return "MyStrategy"; }
///     std::vector<std::string> symbols() const override { return {"BTC/USD"}; }
/// };
/// @endcode
class AlertStrategy {
public:
    virtual ~AlertStrategy() = default;
    
    // Non-copyable, non-movable (base class for polymorphic use)
    AlertStrategy(const AlertStrategy&) = delete;
    AlertStrategy& operator=(const AlertStrategy&) = delete;
    AlertStrategy(AlertStrategy&&) = delete;
    AlertStrategy& operator=(AlertStrategy&&) = delete;
    
protected:
    AlertStrategy() = default;
    
public:
    /// @brief Check if alert condition is met
    /// 
    /// Called on every ticker update. Return true to trigger the alert.
    /// 
    /// @param ticker Current ticker data
    /// @return true if condition met, alert should fire
    virtual bool check(const Ticker& ticker) = 0;
    
    /// @brief Get strategy name (for logging/debugging)
    /// @return Strategy name
    virtual std::string name() const = 0;
    
    /// @brief Get symbols this strategy applies to
    /// @return List of trading pairs this strategy monitors
    virtual std::vector<std::string> symbols() const = 0;
    
    /// @brief Reset strategy state (e.g., for re-arming alerts)
    /// 
    /// Called when strategy is removed or reset. Override to clear
    /// internal state if needed.
    virtual void reset() {}
    
    /// @brief Get custom alert message (optional)
    /// 
    /// Override this to provide detailed alert messages with context.
    /// Default implementation returns empty string (uses generic message).
    /// 
    /// @param ticker Current ticker data that triggered the alert
    /// @return Custom alert message, or empty string to use default
    virtual std::string get_alert_message(const Ticker& /*ticker*/) const {
        return "";  // Empty = use default message
    }
    
    /// @brief Check if strategy needs OrderBook data
    /// 
    /// Override to return true if strategy requires order book updates.
    /// When true, strategy will also receive OrderBook data via check(const OrderBook&).
    /// 
    /// @return true if strategy needs order book data
    virtual bool needs_orderbook() const { return false; }
    
    /// @brief Check if strategy needs Trade data
    /// 
    /// Override to return true if strategy requires trade updates.
    /// When true, strategy will also receive Trade data via check(const Trade&).
    /// 
    /// @return true if strategy needs trade data
    virtual bool needs_trades() const { return false; }
    
    /// @brief Check if strategy needs OHLC/candle data
    /// 
    /// Override to return true if strategy requires OHLC updates.
    /// When true, strategy will also receive OHLC data via check(const OHLC&).
    /// 
    /// @return true if strategy needs OHLC data
    virtual bool needs_ohlc() const { return false; }
    
    /// @brief Check alert condition with OrderBook data (optional)
    /// 
    /// Override this if strategy needs order book data.
    /// Only called if needs_orderbook() returns true.
    /// 
    /// @param book Current order book data
    /// @return true if condition met, alert should fire
    virtual bool check(const OrderBook& /*book*/) { return false; }
    
    /// @brief Check alert condition with Trade data (optional)
    /// 
    /// Override this if strategy needs trade data.
    /// Only called if needs_trades() returns true.
    /// 
    /// @param trade Recent trade data
    /// @return true if condition met, alert should fire
    virtual bool check(const Trade& /*trade*/) { return false; }
    
    /// @brief Check alert condition with both Ticker and OrderBook (optional)
    /// 
    /// Override this for strategies that need both data sources.
    /// Called when both ticker and order book updates are available.
    /// 
    /// @param ticker Current ticker data
    /// @param book Current order book data
    /// @return true if condition met, alert should fire
    virtual bool check(const Ticker& /*ticker*/, const OrderBook& /*book*/) { return false; }
    
    /// @brief Check alert condition with OHLC data (optional)
    /// 
    /// Override this if strategy needs OHLC/candle data.
    /// Only called if needs_ohlc() returns true.
    /// 
    /// @param ohlc Current OHLC candle data
    /// @return true if condition met, alert should fire
    virtual bool check(const OHLC& /*ohlc*/) { return false; }
    
    /// @brief Check if strategy is enabled
    /// 
    /// Override to provide custom enable/disable logic.
    /// Default implementation always returns true.
    /// 
    /// @return true if strategy is enabled and should be evaluated
    virtual bool is_enabled() const { return true; }
    
    /// @brief Enable the strategy
    /// 
    /// Override to provide custom enable logic.
    /// Default implementation does nothing (strategy is always enabled).
    virtual void enable() {}
    
    /// @brief Disable the strategy
    /// 
    /// Override to provide custom disable logic.
    /// Default implementation does nothing (strategy is always enabled).
    virtual void disable() {}
};

} // namespace kraken

