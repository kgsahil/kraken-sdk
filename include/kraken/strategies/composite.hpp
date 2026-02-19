/// @file composite.hpp
/// @brief Composite strategy for combining multiple strategies
/// 
/// Allows creating complex strategies by combining simpler ones with AND/OR logic.

#pragma once

#include "base.hpp"
#include <algorithm>
#include <vector>
#include <stdexcept>

namespace kraken {

/// @brief Composite strategy that combines multiple strategies with AND/OR logic
/// 
/// Allows creating complex strategies by combining simpler ones:
/// - AND: All strategies must trigger
/// - OR: Any strategy can trigger
/// 
/// @example
/// @code
/// auto price_alert = PriceAlert::Builder().symbol("BTC/USD").above(50000).build();
/// auto volume_spike = VolumeSpike::Builder().symbols({"BTC/USD"}).multiplier(2.0).build();
/// 
/// // AND: Both must trigger
/// auto combined = CompositeStrategy::and_(price_alert, volume_spike);
/// 
/// // OR: Either can trigger
/// auto either = CompositeStrategy::or_(price_alert, volume_spike);
/// @endcode
class CompositeStrategy : public AlertStrategy {
public:
    enum class Logic { AND, OR };
    
    /// @brief Create AND composite (all strategies must trigger)
    /// @param strategies Strategies to combine (all must trigger)
    /// @return Composite strategy
    static std::shared_ptr<CompositeStrategy> and_(
        std::vector<std::shared_ptr<AlertStrategy>> strategies) {
        return std::make_shared<CompositeStrategy>(Logic::AND, std::move(strategies));
    }
    
    /// @brief Create AND composite from two strategies
    static std::shared_ptr<CompositeStrategy> and_(
        std::shared_ptr<AlertStrategy> a,
        std::shared_ptr<AlertStrategy> b) {
        return and_({std::move(a), std::move(b)});
    }
    
    /// @brief Create OR composite (any strategy can trigger)
    /// @param strategies Strategies to combine (any can trigger)
    /// @return Composite strategy
    static std::shared_ptr<CompositeStrategy> or_(
        std::vector<std::shared_ptr<AlertStrategy>> strategies) {
        return std::make_shared<CompositeStrategy>(Logic::OR, std::move(strategies));
    }
    
    /// @brief Create OR composite from two strategies
    static std::shared_ptr<CompositeStrategy> or_(
        std::shared_ptr<AlertStrategy> a,
        std::shared_ptr<AlertStrategy> b) {
        return or_({std::move(a), std::move(b)});
    }
    
    CompositeStrategy(Logic logic, std::vector<std::shared_ptr<AlertStrategy>> strategies)
        : logic_(logic), strategies_(std::move(strategies)) {
        if (strategies_.empty()) {
            throw std::invalid_argument("CompositeStrategy requires at least one strategy");
        }
    }
    
    bool check(const Ticker& ticker) override {
        if (!is_enabled()) return false;
        return evaluate([&ticker](auto& s) { return s->check(ticker); });
    }
    
    bool check(const OrderBook& book) override {
        if (!is_enabled()) return false;
        return evaluate([&book](auto& s) { return s->check(book); });
    }
    
    bool check(const Trade& trade) override {
        if (!is_enabled()) return false;
        return evaluate([&trade](auto& s) { return s->check(trade); });
    }
    
    bool check(const Ticker& ticker, const OrderBook& book) override {
        if (!is_enabled()) return false;
        return evaluate([&ticker, &book](auto& s) { return s->check(ticker, book); });
    }
    
    bool check(const OHLC& ohlc) override {
        if (!is_enabled()) return false;
        return evaluate([&ohlc](auto& s) { return s->check(ohlc); });
    }
    
    bool needs_orderbook() const override {
        return std::any_of(strategies_.begin(), strategies_.end(),
            [](const auto& s) { return s->needs_orderbook(); });
    }
    
    bool needs_trades() const override {
        return std::any_of(strategies_.begin(), strategies_.end(),
            [](const auto& s) { return s->needs_trades(); });
    }
    
    bool needs_ohlc() const override {
        return std::any_of(strategies_.begin(), strategies_.end(),
            [](const auto& s) { return s->needs_ohlc(); });
    }
    
    std::string name() const override {
        std::string name = "Composite(";
        name += (logic_ == Logic::AND) ? "AND" : "OR";
        name += ")";
        return name;
    }
    
    std::vector<std::string> symbols() const override {
        std::vector<std::string> all_symbols;
        for (const auto& s : strategies_) {
            auto syms = s->symbols();
            all_symbols.insert(all_symbols.end(), syms.begin(), syms.end());
        }
        // Remove duplicates
        std::sort(all_symbols.begin(), all_symbols.end());
        all_symbols.erase(std::unique(all_symbols.begin(), all_symbols.end()), all_symbols.end());
        return all_symbols;
    }
    
    void reset() override {
        for (auto& s : strategies_) {
            s->reset();
        }
    }
    
    bool is_enabled() const override {
        return enabled_ && std::all_of(strategies_.begin(), strategies_.end(),
            [](const auto& s) { return s->is_enabled(); });
    }
    
    void enable() override {
        enabled_ = true;
        for (auto& s : strategies_) {
            s->enable();
        }
    }
    
    void disable() override {
        enabled_ = false;
        for (auto& s : strategies_) {
            s->disable();
        }
    }
    
    std::string get_alert_message(const Ticker& /*ticker*/) const override {
        std::string msg = name() + " triggered";
        if (logic_ == Logic::AND) {
            msg += " (all conditions met)";
        } else {
            msg += " (any condition met)";
        }
        return msg;
    }
    
private:
    template<typename Func>
    bool evaluate(Func&& check_func) {
        if (logic_ == Logic::AND) {
            // All must return true
            return std::all_of(strategies_.begin(), strategies_.end(),
                [&check_func](const auto& s) {
                    return !s->is_enabled() || check_func(s);
                });
        }
        
        // Any can return true
        return std::any_of(strategies_.begin(), strategies_.end(),
            [&check_func](const auto& s) {
                return s->is_enabled() && check_func(s);
            });
    }
    
    Logic logic_;
    std::vector<std::shared_ptr<AlertStrategy>> strategies_;
    bool enabled_ = true;
};

} // namespace kraken

