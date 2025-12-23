/// @file strategy_engine_impl.cpp
/// @brief StrategyEngine implementation

#include "internal/client_impl.hpp"
#include "kraken/strategies/strategies.hpp"
#include <algorithm>

namespace kraken {

int StrategyEngine::add(std::shared_ptr<AlertStrategy> strategy, AlertCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    int id = next_id_++;
    strategies_[id] = {std::move(strategy), std::move(callback)};
    return id;
}

void StrategyEngine::remove(int id) {
    std::lock_guard<std::mutex> lock(mutex_);
    strategies_.erase(id);
}

void StrategyEngine::evaluate(const Ticker& ticker) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (auto& pair : strategies_) {
        auto& entry = pair.second;
        
        // Skip if disabled
        if (!entry.enabled || !entry.strategy->is_enabled()) {
            continue;
        }
        
        // Check if strategy applies to this symbol
        auto syms = entry.strategy->symbols();
        if (std::find(syms.begin(), syms.end(), ticker.symbol) == syms.end()) {
            continue;
        }
        
        // Check if condition is met
        if (entry.strategy->check(ticker)) {
            Alert alert;
            alert.strategy_name = entry.strategy->name();
            alert.symbol = ticker.symbol;
            alert.price = ticker.last;
            
            // Try to get custom message from strategy
            std::string custom_msg = entry.strategy->get_alert_message(ticker);
            if (!custom_msg.empty()) {
                alert.message = custom_msg;
            } else {
                alert.message = "Strategy condition met";
            }
            
            alert.timestamp = std::chrono::system_clock::now();
            
            // Fire callback
            entry.callback(alert);
        }
    }
}

size_t StrategyEngine::count() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return strategies_.size();
}

std::vector<std::pair<int, std::string>> StrategyEngine::get_alerts() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::pair<int, std::string>> result;
    result.reserve(strategies_.size());
    for (const auto& pair : strategies_) {
        result.emplace_back(pair.first, pair.second.strategy->name());
    }
    return result;
}

void StrategyEngine::evaluate(const OrderBook& book) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (auto& pair : strategies_) {
        auto& entry = pair.second;
        
        // Skip if disabled
        if (!entry.enabled || !entry.strategy->is_enabled()) {
            continue;
        }
        
        // Only evaluate strategies that need order book data
        if (!entry.strategy->needs_orderbook()) {
            continue;
        }
        
        // Check if strategy applies to this symbol
        auto syms = entry.strategy->symbols();
        if (std::find(syms.begin(), syms.end(), book.symbol) == syms.end()) {
            continue;
        }
        
        // Check if condition is met
        if (entry.strategy->check(book)) {
            Alert alert;
            alert.strategy_name = entry.strategy->name();
            alert.symbol = book.symbol;
            alert.price = book.mid_price();
            alert.message = entry.strategy->get_alert_message(Ticker{});  // Empty ticker for book-only
            if (alert.message.empty()) {
                alert.message = "Strategy condition met (order book)";
            }
            alert.timestamp = std::chrono::system_clock::now();
            entry.callback(alert);
        }
    }
}

void StrategyEngine::evaluate(const Trade& trade) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (auto& pair : strategies_) {
        auto& entry = pair.second;
        
        // Skip if disabled
        if (!entry.enabled || !entry.strategy->is_enabled()) {
            continue;
        }
        
        // Only evaluate strategies that need trade data
        if (!entry.strategy->needs_trades()) {
            continue;
        }
        
        // Check if strategy applies to this symbol
        auto syms = entry.strategy->symbols();
        if (std::find(syms.begin(), syms.end(), trade.symbol) == syms.end()) {
            continue;
        }
        
        // Check if condition is met
        if (entry.strategy->check(trade)) {
            Alert alert;
            alert.strategy_name = entry.strategy->name();
            alert.symbol = trade.symbol;
            alert.price = trade.price;
            // Create a minimal ticker for message generation
            Ticker ticker_for_msg;
            ticker_for_msg.symbol = trade.symbol;
            ticker_for_msg.last = trade.price;
            alert.message = entry.strategy->get_alert_message(ticker_for_msg);
            if (alert.message.empty()) {
                alert.message = "Strategy condition met (trade)";
            }
            alert.timestamp = std::chrono::system_clock::now();
            entry.callback(alert);
        }
    }
}

void StrategyEngine::evaluate(const Ticker& ticker, const OrderBook& book) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (auto& pair : strategies_) {
        auto& entry = pair.second;
        
        // Skip if disabled
        if (!entry.enabled || !entry.strategy->is_enabled()) {
            continue;
        }
        
        // Check if strategy applies to this symbol
        auto syms = entry.strategy->symbols();
        if (std::find(syms.begin(), syms.end(), ticker.symbol) == syms.end()) {
            continue;
        }
        
        // Check if condition is met (multi-source check)
        if (entry.strategy->check(ticker, book)) {
            Alert alert;
            alert.strategy_name = entry.strategy->name();
            alert.symbol = ticker.symbol;
            alert.price = ticker.last;
            alert.message = entry.strategy->get_alert_message(ticker);
            if (alert.message.empty()) {
                alert.message = "Strategy condition met (ticker + order book)";
            }
            alert.timestamp = std::chrono::system_clock::now();
            entry.callback(alert);
        }
    }
}

void StrategyEngine::evaluate(const OHLC& ohlc) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (auto& pair : strategies_) {
        auto& entry = pair.second;
        
        // Skip if disabled
        if (!entry.enabled || !entry.strategy->is_enabled()) {
            continue;
        }
        
        // Only evaluate strategies that need OHLC data
        if (!entry.strategy->needs_ohlc()) {
            continue;
        }
        
        // Check if strategy applies to this symbol
        auto syms = entry.strategy->symbols();
        if (std::find(syms.begin(), syms.end(), ohlc.symbol) == syms.end()) {
            continue;
        }
        
        // Check if condition is met
        if (entry.strategy->check(ohlc)) {
            Alert alert;
            alert.strategy_name = entry.strategy->name();
            alert.symbol = ohlc.symbol;
            alert.price = ohlc.close;
            Ticker ticker_for_msg;
            ticker_for_msg.symbol = ohlc.symbol;
            ticker_for_msg.last = ohlc.close;
            alert.message = entry.strategy->get_alert_message(ticker_for_msg);
            if (alert.message.empty()) {
                alert.message = "Strategy condition met (OHLC)";
            }
            alert.timestamp = std::chrono::system_clock::now();
            entry.callback(alert);
        }
    }
}

void StrategyEngine::enable(int id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = strategies_.find(id);
    if (it != strategies_.end()) {
        it->second.enabled = true;
        it->second.strategy->enable();
    }
}

void StrategyEngine::disable(int id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = strategies_.find(id);
    if (it != strategies_.end()) {
        it->second.enabled = false;
        it->second.strategy->disable();
    }
}

bool StrategyEngine::is_enabled(int id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = strategies_.find(id);
    if (it != strategies_.end()) {
        return it->second.enabled && it->second.strategy->is_enabled();
    }
    return false;
}

} // namespace kraken

