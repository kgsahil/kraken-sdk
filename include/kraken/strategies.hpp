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
#include <algorithm>
#include <stdexcept>

namespace kraken {

// Forward declarations
struct OrderBook;
struct Trade;

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
        
        bool triggered = false;
        std::string condition;
        
        if (ticker.last >= above_) {
            triggered = true;
            condition = "above";
            double threshold = above_;
            if (prev_price > 0.0) {
                double change_pct = ((ticker.last - prev_price) / prev_price) * 100.0;
                last_message_ = "Price " + condition + " $" + std::to_string(threshold) 
                              + " (was $" + std::to_string(prev_price) 
                              + ", change " + (change_pct >= 0 ? "+" : "") 
                              + std::to_string(change_pct) + "%)";
            } else {
                last_message_ = "Price " + condition + " $" + std::to_string(threshold);
            }
        } else if (ticker.last <= below_) {
            triggered = true;
            condition = "below";
            double threshold = below_;
            if (prev_price > 0.0) {
                double change_pct = ((ticker.last - prev_price) / prev_price) * 100.0;
                last_message_ = "Price " + condition + " $" + std::to_string(threshold) 
                              + " (was $" + std::to_string(prev_price) 
                              + ", change " + (change_pct >= 0 ? "+" : "") 
                              + std::to_string(change_pct) + "%)";
            } else {
                last_message_ = "Price " + condition + " $" + std::to_string(threshold);
            }
        }
        
        if (triggered) {
            fired_ = true;
            last_fired_time_ = now;
            fire_count_++;
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
        fire_count_ = 0;
    }
    
    const std::string& last_message() const { return last_message_; }
    
    /// @brief Check if alert has fired
    /// @return true if alert has fired at least once
    bool has_fired() const { return fired_; }
    
    /// @brief Get number of times alert has fired
    /// @return Fire count
    size_t fire_count() const { return fire_count_; }
    
    /// @brief Check if alert is recurring
    /// @return true if recurring, false if one-time
    bool is_recurring() const { return recurring_; }
    
private:
    std::string symbol_;
    double above_;
    double below_;
    bool recurring_;
    std::chrono::milliseconds cooldown_;
    bool fired_ = false;
    std::string last_message_;
    double last_price_ = 0.0;
    std::chrono::steady_clock::time_point last_fired_time_{};
    size_t fire_count_ = 0;
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
    
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
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
    
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
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

//------------------------------------------------------------------------------
// Strategy Composition (AND/OR Logic)
//------------------------------------------------------------------------------

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
        std::string op = (logic_ == Logic::AND) ? "AND" : "OR";
        std::string result = "Composite(" + op + ": ";
        for (size_t i = 0; i < strategies_.size(); ++i) {
            if (i > 0) result += ", ";
            result += strategies_[i]->name();
        }
        result += ")";
        return result;
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
    
    std::string get_alert_message(const Ticker& ticker) const override {
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
        } else {
            // Any can return true
            return std::any_of(strategies_.begin(), strategies_.end(),
                [&check_func](const auto& s) {
                    return s->is_enabled() && check_func(s);
                });
        }
    }
    
    Logic logic_;
    std::vector<std::shared_ptr<AlertStrategy>> strategies_;
    bool enabled_ = true;
};

//------------------------------------------------------------------------------
// Strategy Presets
//------------------------------------------------------------------------------

/// @brief Factory for common strategy presets
/// 
/// Provides pre-configured strategies for common trading patterns:
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
/// auto support = StrategyPresets::support_level("BTC/USD", 45000.0, 10.0);
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
    static std::shared_ptr<CompositeStrategy> breakout(
        const std::string& symbol,
        double price_threshold,
        double volume_multiplier = 2.0) {
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
            SupportStrategy(const std::string& sym, double support, double tol, double liq)
                : symbol_(sym), support_level_(support), tolerance_(tol), min_liquidity_(liq) {}
            
            bool check(const Ticker& ticker) override { return false; }
            bool check(const OrderBook& book) override { return false; }
            bool check(const Trade& trade) override { return false; }
            bool check(const OHLC& ohlc) override { return false; }
            
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
                char buf[256];
                snprintf(buf, sizeof(buf),
                    "Price %.2f near support level %.2f (tolerance: %.2f%%) with strong bid liquidity",
                    ticker.last, support_level_, tolerance_);
                return std::string(buf);
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
            ResistanceStrategy(const std::string& sym, double resistance, double tol, double liq)
                : symbol_(sym), resistance_level_(resistance), tolerance_(tol), min_liquidity_(liq) {}
            
            bool check(const Ticker& ticker) override { return false; }
            bool check(const OrderBook& book) override { return false; }
            bool check(const Trade& trade) override { return false; }
            bool check(const OHLC& ohlc) override { return false; }
            
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

