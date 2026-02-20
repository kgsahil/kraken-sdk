/// @file presets.hpp
/// @brief Pre-configured strategy presets
/// 
/// Provides ready-to-use strategies for common trading patterns.

#pragma once

#include "base.hpp"
#include "price_alert.hpp"
#include "volume_spike.hpp"
#include "composite.hpp"
#include "../core/types.hpp"
#include <memory>
#include <cmath>
#include <algorithm>

namespace kraken {

// Forward declaration
struct OrderBook;

/// @brief Pre-configured strategies for common trading patterns
/// 
/// Provides ready-to-use strategies for common trading patterns:
/// - Breakout detection
/// - Support/resistance levels
/// - Volume confirmation
/// 
/// @example
/// @code
/// // Breakout above resistance with volume confirmation
/// auto breakout = StrategyPresets::breakout("BTC/USD", 50000.0, 2.0);
/// 
/// // Support level detection
/// auto support = StrategyPresets::support_level("BTC/USD", 45000.0, 1.0, 10.0);
/// @endcode
class StrategyPresets {
public:
    /// @brief Breakout strategy: Price breaks above threshold with volume confirmation
    /// 
    /// Triggers when:
    /// - Price breaks above threshold
    /// - Volume exceeds N× average
    /// 
    /// @param symbol Trading pair
    /// @param price_threshold Price breakout level
    /// @param volume_multiplier Volume spike multiplier (default: 2.0)
    /// @return Composite strategy combining price and volume conditions
    // NOLINTBEGIN(bugprone-easily-swappable-parameters)
    static std::shared_ptr<CompositeStrategy> breakout(
        const std::string& symbol,
        double price_threshold,
        double volume_multiplier = 2.0) {
    // NOLINTEND(bugprone-easily-swappable-parameters)
        auto price = PriceAlert::Builder()
            .symbol(symbol)
            .above(price_threshold)
            .recurring(true)
            .build();
        
        auto volume = VolumeSpike::Builder()
            .symbols({symbol})
            .multiplier(volume_multiplier)
            .lookback(20)
            .build();
        
        return CompositeStrategy::and_(price, volume);
    }
    
    /// @brief Support level strategy: Price approaches support with order book confirmation
    /// 
    /// Triggers when:
    /// - Price is near support level (within tolerance)
    /// - Order book shows strong bid liquidity (support)
    /// 
    /// @param symbol Trading pair
    /// @param support_level Support price level
    /// @param tolerance_percent Price tolerance (default: 1% of support level)
    /// @param min_liquidity Minimum bid liquidity required (default: 10.0)
    /// @return Multi-source strategy
    static std::shared_ptr<AlertStrategy> support_level(
        const std::string& symbol,
        double support_level,
        double tolerance_percent = 1.0,
        double min_liquidity = 10.0) {
        
        class SupportStrategy : public AlertStrategy {
            std::string symbol_;
            double support_level_;
            double tolerance_;
            double min_liquidity_;
            
        public:
            // NOLINTBEGIN(bugprone-easily-swappable-parameters)
            SupportStrategy(std::string sym, double support, double tol, double liq)
                : symbol_(std::move(sym)), support_level_(support), tolerance_(tol), min_liquidity_(liq) {}
            // NOLINTEND(bugprone-easily-swappable-parameters)
            
            bool check(const Ticker& /*unused*/) override { return false; }
            bool check(const OrderBook& /*unused*/) override { return false; }
            bool check(const Trade& /*unused*/) override { return false; }
            bool check(const OHLC& /*unused*/) override { return false; }
            
            bool check(const Ticker& ticker, const OrderBook& book) override {
                if (ticker.symbol != symbol_) return false;
                
                // Price within tolerance of support
                double price_diff = std::abs(ticker.last - support_level_);
                double tolerance_abs = support_level_ * (tolerance_ / 100.0);
                if (price_diff > tolerance_abs) return false;
                
                // Strong bid liquidity (support)
                return book.total_bid_liquidity(5) >= min_liquidity_;
            }
            
            std::string name() const override { return "SupportLevel"; }
            std::vector<std::string> symbols() const override { return {symbol_}; }
            
            std::string get_alert_message(const Ticker& ticker) const override {
                return "Price " + std::to_string(ticker.last) + " near support level " +
                    std::to_string(support_level_) + " (tolerance: " + std::to_string(tolerance_) +
                    "%) with strong bid liquidity";
            }
        };
        
        return std::make_shared<SupportStrategy>(symbol, support_level, tolerance_percent, min_liquidity);
    }
    
    /// @brief Resistance level strategy: Price approaches resistance with order book confirmation
    /// 
    /// Triggers when:
    /// - Price is near resistance level (within tolerance)
    /// - Order book shows strong ask liquidity (resistance)
    /// 
    /// @param symbol Trading pair
    /// @param resistance_level Resistance price level
    /// @param tolerance_percent Price tolerance (default: 1% of resistance level)
    /// @param min_liquidity Minimum ask liquidity required (default: 10.0)
    /// @return Multi-source strategy
    static std::shared_ptr<AlertStrategy> resistance_level(
        const std::string& symbol,
        double resistance_level,
        double tolerance_percent = 1.0,
        double min_liquidity = 10.0) {
        
        class ResistanceStrategy : public AlertStrategy {
            std::string symbol_;
            double resistance_level_;
            double tolerance_;
            double min_liquidity_;
            
        public:
            // NOLINTBEGIN(bugprone-easily-swappable-parameters)
            ResistanceStrategy(std::string sym, double resistance, double tol, double liq)
                : symbol_(std::move(sym)), resistance_level_(resistance), tolerance_(tol), min_liquidity_(liq) {}
            // NOLINTEND(bugprone-easily-swappable-parameters)
            
            bool check(const Ticker& /*unused*/) override { return false; }
            bool check(const OrderBook& /*unused*/) override { return false; }
            bool check(const Trade& /*unused*/) override { return false; }
            bool check(const OHLC& /*unused*/) override { return false; }
            
            bool check(const Ticker& ticker, const OrderBook& book) override {
                if (ticker.symbol != symbol_) return false;
                
                // Price within tolerance of resistance
                double price_diff = std::abs(ticker.last - resistance_level_);
                double tolerance_abs = resistance_level_ * (tolerance_ / 100.0);
                if (price_diff > tolerance_abs) return false;
                
                // Strong ask liquidity (resistance)
                return book.total_ask_liquidity(5) >= min_liquidity_;
            }
            
            std::string name() const override { return "ResistanceLevel"; }
            std::vector<std::string> symbols() const override { return {symbol_}; }
            
            std::string get_alert_message(const Ticker& ticker) const override {
                return "Price " + std::to_string(ticker.last) + " near resistance level " +
                    std::to_string(resistance_level_) + " (tolerance: " + std::to_string(tolerance_) +
                    "%) with strong ask liquidity";
            }
        };
        
        return std::make_shared<ResistanceStrategy>(symbol, resistance_level, tolerance_percent, min_liquidity);
    }
};

} // namespace kraken

