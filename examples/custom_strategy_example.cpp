/// @file custom_strategy_example.cpp
/// @brief Example demonstrating how to create custom trading strategies
/// 
/// This example shows the flexibility of the strategy system:
/// 1. Simple strategies (Ticker-only)
/// 2. OrderBook-based strategies
/// 3. Multi-source strategies (Ticker + OrderBook)
/// 4. Custom alert messages

#include "kraken/kraken.hpp"
#include "common.hpp"
#include <iostream>
#include <iomanip>

using namespace kraken;

//------------------------------------------------------------------------------
// Example 1: Simple Ticker-Based Strategy
//------------------------------------------------------------------------------

/// Simple strategy: Alert when spread exceeds threshold
class WideSpreadStrategy : public AlertStrategy {
    double max_spread_;
    std::string symbol_;
    
public:
    WideSpreadStrategy(const std::string& symbol, double max_spread)
        : symbol_(symbol), max_spread_(max_spread) {}
    
    bool check(const Ticker& ticker) override {
        return ticker.symbol == symbol_ && ticker.spread() > max_spread_;
    }
    
    std::string name() const override { return "WideSpreadStrategy"; }
    
    std::vector<std::string> symbols() const override {
        return {symbol_};
    }
    
    // Custom message with context
    std::string get_alert_message(const Ticker& ticker) const override {
        char buf[256];
        snprintf(buf, sizeof(buf),
            "Wide spread detected: %.2f (threshold: %.2f, spread %%: %.2f%%)",
            ticker.spread(), max_spread_, ticker.spread_percent());
        return std::string(buf);
    }
};

//------------------------------------------------------------------------------
// Example 2: OrderBook-Based Strategy
//------------------------------------------------------------------------------

/// Strategy: Alert when order book imbalance is extreme
class OrderBookImbalanceStrategy : public AlertStrategy {
    double imbalance_threshold_;  // 0.7 = 70% imbalance
    std::string symbol_;
    
public:
    OrderBookImbalanceStrategy(const std::string& symbol, double threshold)
        : symbol_(symbol), imbalance_threshold_(threshold) {}
    
    // Indicate we need order book data
    bool needs_orderbook() const override { return true; }
    
    bool check(const OrderBook& book) override {
        if (book.symbol != symbol_ || book.bids.empty() || book.asks.empty()) {
            return false;
        }
        
        double bid_liquidity = book.total_bid_liquidity(10);
        double ask_liquidity = book.total_ask_liquidity(10);
        double total = bid_liquidity + ask_liquidity;
        
        if (total == 0.0) return false;
        
        double imbalance = std::abs(bid_liquidity - ask_liquidity) / total;
        return imbalance > imbalance_threshold_;
    }
    
    std::string name() const override { return "OrderBookImbalanceStrategy"; }
    
    std::vector<std::string> symbols() const override {
        return {symbol_};
    }
    
    std::string get_alert_message(const Ticker& ticker) const override {
        return "Extreme order book imbalance detected";
    }
};

//------------------------------------------------------------------------------
// Example 3: Multi-Source Strategy (Ticker + OrderBook)
//------------------------------------------------------------------------------

/// Strategy: Alert when price moves AND order book shows support/resistance
class PriceWithLiquidityStrategy : public AlertStrategy {
    double price_threshold_;
    double min_liquidity_;
    std::string symbol_;
    
public:
    PriceWithLiquidityStrategy(const std::string& symbol, 
                              double price_threshold, 
                              double min_liquidity)
        : symbol_(symbol), price_threshold_(price_threshold), 
          min_liquidity_(min_liquidity) {}
    
    // Use multi-source check
    bool check(const Ticker& ticker, const OrderBook& book) override {
        if (ticker.symbol != symbol_) return false;
        
        // Price must be above threshold
        if (ticker.last < price_threshold_) return false;
        
        // Order book must have sufficient liquidity (support)
        double bid_liquidity = book.total_bid_liquidity(5);
        return bid_liquidity >= min_liquidity_;
    }
    
    std::string name() const override { return "PriceWithLiquidityStrategy"; }
    
    std::vector<std::string> symbols() const override {
        return {symbol_};
    }
    
    std::string get_alert_message(const Ticker& ticker) const override {
        char buf[256];
        snprintf(buf, sizeof(buf),
            "Price %.2f above threshold %.2f with strong order book support",
            ticker.last, price_threshold_);
        return std::string(buf);
    }
};

//------------------------------------------------------------------------------
// Main Example
//------------------------------------------------------------------------------

int main(int argc, char* argv[]) {
    auto config = load_config(argc, argv);
    if (!config) {
        return 1;
    }
    
    KrakenClient client(*config);
    
    // Setup error handling
    client.on_error([](const Error& err) {
        std::cerr << "Error: " << err.message << std::endl;
    });
    
    // Example 1: Simple ticker-based strategy
    auto wide_spread = std::make_shared<WideSpreadStrategy>("BTC/USD", 100.0);
    client.add_alert(wide_spread, [](const Alert& alert) {
        std::cout << "🚨 " << alert.strategy_name << ": " << alert.message << std::endl;
    });
    
    // Example 2: OrderBook-based strategy
    auto imbalance = std::make_shared<OrderBookImbalanceStrategy>("BTC/USD", 0.7);
    client.add_alert(imbalance, [](const Alert& alert) {
        std::cout << "🚨 " << alert.strategy_name << ": " << alert.message << std::endl;
    });
    
    // Example 3: Multi-source strategy
    auto price_liquidity = std::make_shared<PriceWithLiquidityStrategy>(
        "BTC/USD", 50000.0, 10.0);
    client.add_alert(price_liquidity, [](const Alert& alert) {
        std::cout << "🚨 " << alert.strategy_name << ": " << alert.message << std::endl;
    });
    
    // Subscribe to ticker and order book
    client.subscribe("BTC/USD");
    client.subscribe_book("BTC/USD", 10);
    
    std::cout << "Monitoring BTC/USD with custom strategies..." << std::endl;
    std::cout << "Press Ctrl+C to stop\n" << std::endl;
    
    // Run for 60 seconds
    client.run_async();
    std::this_thread::sleep_for(std::chrono::seconds(60));
    client.stop();
    
    return 0;
}

