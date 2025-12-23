/// @file spread_alert.hpp
/// @brief Spread monitoring alert strategy
/// 
/// Monitors bid-ask spread and triggers when spread is outside specified range.

#pragma once

#include "base.hpp"
#include <limits>

namespace kraken {

/// @brief Alert when spread is outside range
/// 
/// Monitors the bid-ask spread and triggers when spread is below minimum
/// or above maximum threshold.
/// 
/// @example
/// @code
/// auto alert = SpreadAlert::Builder()
///     .symbol("BTC/USD")
///     .min_spread(1.0)
///     .max_spread(10.0)
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
        
        Builder& min_spread(double min) {
            min_spread_ = min;
            return *this;
        }
        
        Builder& max_spread(double max) {
            max_spread_ = max;
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
    
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    SpreadAlert(std::string symbol, double min_spread, double max_spread)
        : symbol_(std::move(symbol))
        , min_spread_(min_spread)
        , max_spread_(max_spread) {}
    
    bool check(const Ticker& ticker) override {
        if (ticker.symbol != symbol_) {
            return false;
        }
        
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

