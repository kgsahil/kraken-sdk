/// @file strategies.hpp
/// @brief Trading strategy engine and alert system
/// 
/// Provides built-in alert strategies (PriceAlert, VolumeSpike, SpreadAlert)
/// and extensible base class for custom trading strategies.

#pragma once

#include "types.hpp"
#include <memory>
#include <vector>
#include <string>
#include <functional>
#include <chrono>
#include <deque>
#include <unordered_map>
#include <limits>

namespace kraken {

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
        char buf[512];
        snprintf(buf, sizeof(buf),
            R"({"strategy":"%s","symbol":"%s","price":%.8f,"message":"%s","timestamp_ms":%lld})",
            strategy_name.c_str(), symbol.c_str(), price, message.c_str(),
            static_cast<long long>(ts));
        return std::string(buf);
    }
};

/// @brief Callback type for alerts
/// 
/// Called when a strategy condition is met.
/// 
/// @param alert Alert information
using AlertCallback = std::function<void(const Alert&)>;

//------------------------------------------------------------------------------
// Strategy Base Class
//------------------------------------------------------------------------------

/// @brief Base class for alert strategies
/// 
/// Implement this interface to create custom trading strategies that
/// monitor market conditions and trigger alerts.
/// 
/// @example
/// @code
/// class CustomStrategy : public AlertStrategy {
///     bool check(const Ticker& ticker) override {
///         return ticker.spread() > 100.0;
///     }
///     std::string name() const override { return "CustomStrategy"; }
///     std::vector<std::string> symbols() const override { return {"BTC/USD"}; }
/// };
/// @endcode
class AlertStrategy {
public:
    virtual ~AlertStrategy() = default;
    
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
};

//------------------------------------------------------------------------------
// PriceAlert Strategy
//------------------------------------------------------------------------------

/// @brief Alert when price crosses a threshold
/// 
/// Monitors a trading pair and triggers when price goes above or below
/// specified thresholds. One-time alert (fires once, then requires reset).
/// 
/// @example
/// @code
/// auto alert = PriceAlert::Builder()
///     .symbol("BTC/USD")
///     .above(100000.0)
///     .build();
/// client.add_alert(alert, [](const Alert& a) {
///     std::cout << "Price alert: " << a.message << std::endl;
/// });
/// @endcode
class PriceAlert : public AlertStrategy {
public:
    /// @brief Builder for PriceAlert configuration
    class Builder {
    public:
        Builder& symbol(const std::string& sym) {
            symbol_ = sym;
            return *this;
        }
        
        Builder& above(double threshold) {
            above_ = threshold;
            return *this;
        }
        
        Builder& below(double threshold) {
            below_ = threshold;
            return *this;
        }
        
        std::shared_ptr<PriceAlert> build() {
            return std::make_shared<PriceAlert>(symbol_, above_, below_);
        }
        
    private:
        std::string symbol_;
        double above_ = std::numeric_limits<double>::max();
        double below_ = std::numeric_limits<double>::lowest();
    };
    
    PriceAlert(std::string symbol, double above, double below)
        : symbol_(std::move(symbol))
        , above_(above)
        , below_(below) {}
    
    bool check(const Ticker& ticker) override {
        if (fired_) return false;
        
        bool triggered = false;
        if (ticker.last >= above_) {
            triggered = true;
            last_message_ = "Price above " + std::to_string(above_);
        } else if (ticker.last <= below_) {
            triggered = true;
            last_message_ = "Price below " + std::to_string(below_);
        }
        
        if (triggered) {
            fired_ = true;
        }
        return triggered;
    }
    
    std::string name() const override { return "PriceAlert"; }
    
    std::vector<std::string> symbols() const override { 
        return {symbol_}; 
    }
    
    void reset() override { fired_ = false; }
    
    const std::string& last_message() const { return last_message_; }
    
private:
    std::string symbol_;
    double above_;
    double below_;
    bool fired_ = false;
    std::string last_message_;
};

//------------------------------------------------------------------------------
// VolumeSpike Strategy
//------------------------------------------------------------------------------

/// @brief Alert when volume exceeds N× recent average
/// 
/// Monitors trading volume and triggers when current volume exceeds
/// the average of recent samples by a multiplier.
/// 
/// @example
/// @code
/// auto alert = VolumeSpike::Builder()
///     .symbols({"BTC/USD", "ETH/USD"})
///     .multiplier(2.5)
///     .lookback(50)
///     .build();
/// @endcode
class VolumeSpike : public AlertStrategy {
public:
    /// @brief Builder for VolumeSpike configuration
    class Builder {
    public:
        Builder& symbols(const std::vector<std::string>& syms) {
            symbols_ = syms;
            return *this;
        }
        
        Builder& multiplier(double mult) {
            multiplier_ = mult;
            return *this;
        }
        
        Builder& lookback(size_t n) {
            lookback_ = n;
            return *this;
        }
        
        std::shared_ptr<VolumeSpike> build() {
            return std::make_shared<VolumeSpike>(symbols_, multiplier_, lookback_);
        }
        
    private:
        std::vector<std::string> symbols_;
        double multiplier_ = 2.0;
        size_t lookback_ = 50;
    };
    
    VolumeSpike(std::vector<std::string> symbols, double multiplier, size_t lookback)
        : symbols_(std::move(symbols))
        , multiplier_(multiplier)
        , lookback_(lookback) {}
    
    bool check(const Ticker& ticker) override {
        auto& state = states_[ticker.symbol];
        
        // Add current volume to history
        state.history.push_back(ticker.volume_24h);
        
        // Keep only lookback samples
        while (state.history.size() > lookback_) {
            state.history.pop_front();
        }
        
        // Need enough samples to calculate average
        if (state.history.size() < lookback_ / 2) {
            return false;
        }
        
        // Calculate average (excluding current)
        double sum = 0.0;
        for (size_t i = 0; i < state.history.size() - 1; ++i) {
            sum += state.history[i];
        }
        double avg = sum / (state.history.size() - 1);
        
        // Check if current volume exceeds threshold
        return ticker.volume_24h > avg * multiplier_;
    }
    
    std::string name() const override { return "VolumeSpike"; }
    
    std::vector<std::string> symbols() const override { 
        return symbols_; 
    }
    
    void reset() override {
        states_.clear();
    }
    
private:
    std::vector<std::string> symbols_;
    double multiplier_;
    size_t lookback_;
    
    struct SymbolState {
        std::deque<double> history;
    };
    std::unordered_map<std::string, SymbolState> states_;
};

//------------------------------------------------------------------------------
// SpreadAlert Strategy
//------------------------------------------------------------------------------

/// @brief Alert when spread crosses a threshold
/// 
/// Monitors bid-ask spread and triggers when spread goes outside
/// the specified min/max range.
/// 
/// @example
/// @code
/// auto alert = SpreadAlert::Builder()
///     .symbol("BTC/USD")
///     .min_spread(10.0)
///     .max_spread(100.0)
///     .build();
/// @endcode
class SpreadAlert : public AlertStrategy {
public:
    /// @brief Builder for SpreadAlert configuration
    class Builder {
    public:
        Builder& symbol(const std::string& sym) {
            symbol_ = sym;
            return *this;
        }
        
        Builder& max_spread(double max) {
            max_spread_ = max;
            return *this;
        }
        
        Builder& min_spread(double min) {
            min_spread_ = min;
            return *this;
        }
        
        std::shared_ptr<SpreadAlert> build() {
            return std::make_shared<SpreadAlert>(symbol_, min_spread_, max_spread_);
        }
        
    private:
        std::string symbol_;
        double min_spread_ = 0.0;
        double max_spread_ = std::numeric_limits<double>::max();
    };
    
    SpreadAlert(std::string symbol, double min_spread, double max_spread)
        : symbol_(std::move(symbol))
        , min_spread_(min_spread)
        , max_spread_(max_spread) {}
    
    bool check(const Ticker& ticker) override {
        double spread = ticker.spread();
        return spread < min_spread_ || spread > max_spread_;
    }
    
    std::string name() const override { return "SpreadAlert"; }
    
    std::vector<std::string> symbols() const override { 
        return {symbol_}; 
    }
    
private:
    std::string symbol_;
    double min_spread_;
    double max_spread_;
};

} // namespace kraken

