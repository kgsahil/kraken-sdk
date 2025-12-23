/// @file price_alert.hpp
/// @brief Price threshold alert strategy
/// 
/// Monitors a trading pair and triggers when price goes above or below
/// specified thresholds.

#pragma once

#include "base.hpp"
#include <limits>
#include <chrono>

namespace kraken {

/// @brief Alert when price crosses a threshold
/// 
/// Monitors a trading pair and triggers when price goes above or below
/// specified thresholds. Supports recurring alerts and cooldown periods.
/// 
/// @example
/// @code
/// auto alert = PriceAlert::Builder()
///     .symbol("BTC/USD")
///     .above(100000.0)
///     .recurring(true)
///     .cooldown(std::chrono::seconds(5))
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
        
        /// @brief Enable recurring alerts (default: one-time)
        /// 
        /// If true, alert will fire every time condition is met.
        /// If false, alert fires once then requires reset.
        /// 
        /// @param recurring Whether to allow recurring alerts
        /// @return Builder reference for chaining
        Builder& recurring(bool recurring = true) {
            recurring_ = recurring;
            return *this;
        }
        
        /// @brief Set cooldown period between alerts (default: 0 = no cooldown)
        /// 
        /// Prevents alert spam by requiring minimum time between alerts.
        /// 
        /// @param cooldown_ms Cooldown period in milliseconds
        /// @return Builder reference for chaining
        Builder& cooldown(std::chrono::milliseconds cooldown_ms) {
            cooldown_ = cooldown_ms;
            return *this;
        }
        
        std::shared_ptr<PriceAlert> build() {
            return std::make_shared<PriceAlert>(symbol_, above_, below_, recurring_, cooldown_);
        }
        
    private:
        std::string symbol_;
        double above_ = std::numeric_limits<double>::max();
        double below_ = std::numeric_limits<double>::lowest();
        bool recurring_ = false;
        std::chrono::milliseconds cooldown_{0};
    };
    
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    PriceAlert(std::string symbol, double above, double below,
        bool recurring, std::chrono::milliseconds cooldown)
        : symbol_(std::move(symbol))
        , above_(above)
        , below_(below)
        , recurring_(recurring)
        , cooldown_(cooldown) {}
    
    bool check(const Ticker& ticker) override {
        // Check cooldown
        auto now = std::chrono::steady_clock::now();
        if (cooldown_.count() > 0 && last_fired_time_ != std::chrono::steady_clock::time_point{}) {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_fired_time_);
            if (elapsed < cooldown_) {
                return false;  // Still in cooldown
            }
        }
        
        // Check if already fired (for one-time alerts)
        if (!recurring_ && fired_) {
            return false;
        }
        
        // Store previous price for message context
        double prev_price = last_price_;
        last_price_ = ticker.last;
        
        // Check thresholds
        bool triggered = false;
        if (ticker.symbol == symbol_) {
            if (above_ != std::numeric_limits<double>::max() && ticker.last >= above_) {
                triggered = true;
            } else if (below_ != std::numeric_limits<double>::lowest() && ticker.last <= below_) {
                triggered = true;
            }
        }
        
        if (triggered) {
            fired_ = true;
            last_fired_time_ = now;
            last_message_ = build_message(ticker, prev_price);
        }
        
        return triggered;
    }
    
    std::string name() const override { return "PriceAlert"; }
    
    std::vector<std::string> symbols() const override { 
        return {symbol_}; 
    }
    
    void reset() override {
        fired_ = false;
        last_fired_time_ = std::chrono::steady_clock::time_point{};
        last_price_ = 0.0;
        last_message_.clear();
    }
    
    std::string get_alert_message(const Ticker& ticker) const override {
        if (!last_message_.empty()) {
            return last_message_;
        }
        return build_message(ticker, 0.0);
    }
    
    /// @brief Check if alert has fired at least once
    /// @return true if alert has fired
    bool has_fired() const { return fired_; }
    
    /// @brief Get number of times alert has fired (for recurring alerts)
    /// @return Fire count
    size_t fire_count() const { return fire_count_; }
    
    /// @brief Check if alert is recurring
    /// @return true if recurring
    bool is_recurring() const { return recurring_; }
    
    /// @brief Get the last alert message
    /// @return Last message (for backward compatibility)
    static std::string last_message() {
        // This is a workaround for backward compatibility
        // In practice, get_alert_message() should be used
        return "";
    }
    
private:
    std::string build_message(const Ticker& ticker, double prev_price) const {
        std::string msg = "Price alert: " + ticker.symbol + " = $" + std::to_string(ticker.last);
        
        if (prev_price > 0.0) {
            double change = ticker.last - prev_price;
            double change_pct = (change / prev_price) * 100.0;
            msg += " (";
            if (change > 0) msg += "+";
            msg += std::to_string(change_pct) + "%, prev: $" + std::to_string(prev_price) + ")";
        }
        
        if (above_ != std::numeric_limits<double>::max() && ticker.last >= above_) {
            msg += " [ABOVE $" + std::to_string(above_) + "]";
        } else if (below_ != std::numeric_limits<double>::lowest() && ticker.last <= below_) {
            msg += " [BELOW $" + std::to_string(below_) + "]";
        }
        
        return msg;
    }
    
    std::string symbol_;
    double above_;
    double below_;
    bool recurring_;
    std::chrono::milliseconds cooldown_;
    bool fired_ = false;
    size_t fire_count_ = 0;
    std::chrono::steady_clock::time_point last_fired_time_{};
    double last_price_ = 0.0;
    mutable std::string last_message_;
};

} // namespace kraken

