# Trading Strategy Engine - Technical Design

The SDK's killer feature: Apply trading strategies to market data, get alerts when conditions are met.

---

## API Design

### Core Interface

```cpp
namespace kraken {

/// Base class for alert strategies
class AlertStrategy {
public:
    virtual ~AlertStrategy() = default;
    
    /// Check if alert condition is met
    /// @param ticker Current ticker data
    /// @return true if condition met, alert should fire
    virtual bool check(const Ticker& ticker) = 0;
    
    /// Strategy name (for logging/debugging)
    virtual std::string name() const = 0;
    
    /// Symbols this strategy applies to
    virtual std::vector<std::string> symbols() const = 0;
};

/// Alert information when strategy triggers
struct Alert {
    std::string strategy_name;
    std::string symbol;
    double price;
    std::string message;
    std::chrono::system_clock::time_point timestamp;
};

/// Callback type for alerts
using AlertCallback = std::function<void(const Alert&)>;

} // namespace kraken
```

### Client Integration

```cpp
class KrakenClient {
public:
    /// Add an alert strategy
    /// @return Alert ID (for removal)
    int add_alert(std::shared_ptr<AlertStrategy> strategy, AlertCallback callback);
    
    /// Remove an alert by ID
    void remove_alert(int alert_id);
    
    /// Get count of active alerts
    size_t alert_count() const;
};
```

---

## Built-in Strategies

### 1. PriceAlert

Alert when price crosses a threshold.

```cpp
class PriceAlert : public AlertStrategy {
public:
    class Builder {
    public:
        Builder& symbol(const std::string& sym);
        Builder& above(double threshold);  // Alert when price > threshold
        Builder& below(double threshold);  // Alert when price < threshold
        std::shared_ptr<PriceAlert> build();
    };
    
    bool check(const Ticker& ticker) override;
    std::string name() const override { return "PriceAlert"; }
    std::vector<std::string> symbols() const override;
    
private:
    std::string symbol_;
    double above_ = std::numeric_limits<double>::max();
    double below_ = std::numeric_limits<double>::lowest();
    bool fired_ = false;  // One-shot: don't fire again until reset
};
```

**Usage:**
```cpp
auto alert = kraken::PriceAlert::Builder()
    .symbol("BTC/USD")
    .above(50000.0)
    .build();

client.add_alert(alert, [](const kraken::Alert& a) {
    std::cout << "🚨 " << a.symbol << " hit $" << a.price << "\n";
});
```

### 2. VolumeSpike

Alert when volume exceeds N× recent average.

```cpp
class VolumeSpike : public AlertStrategy {
public:
    class Builder {
    public:
        Builder& symbols(const std::vector<std::string>& syms);
        Builder& multiplier(double mult);  // e.g., 2.0 = 2x average
        Builder& lookback(size_t n);       // Samples to average
        std::shared_ptr<VolumeSpike> build();
    };
    
    bool check(const Ticker& ticker) override;
    std::string name() const override { return "VolumeSpike"; }
    std::vector<std::string> symbols() const override;
    
private:
    std::vector<std::string> symbols_;
    double multiplier_ = 2.0;
    size_t lookback_ = 50;
    
    // Track volume history per symbol
    struct SymbolState {
        std::deque<double> history;
        double average = 0.0;
    };
    std::unordered_map<std::string, SymbolState> states_;
};
```

**Usage:**
```cpp
auto spike = kraken::VolumeSpike::Builder()
    .symbols({"BTC/USD", "ETH/USD"})
    .multiplier(2.0)
    .lookback(50)
    .build();

client.add_alert(spike, [](const auto& a) {
    std::cout << "📊 Volume spike on " << a.symbol << "\n";
});
```

### 3. SpreadAlert (Optional)

Alert when spread changes significantly.

```cpp
class SpreadAlert : public AlertStrategy {
public:
    class Builder {
    public:
        Builder& symbol(const std::string& sym);
        Builder& max_spread(double max);  // Alert if spread > max
        Builder& min_spread(double min);  // Alert if spread < min
        std::shared_ptr<SpreadAlert> build();
    };
    
    bool check(const Ticker& ticker) override;
    std::string name() const override { return "SpreadAlert"; }
    std::vector<std::string> symbols() const override;
};
```

---

## Implementation

### Strategy Engine Class

```cpp
// src/strategy_engine.hpp

class StrategyEngine {
public:
    /// Add a strategy
    int add(std::shared_ptr<AlertStrategy> strategy, AlertCallback callback);
    
    /// Remove a strategy
    void remove(int id);
    
    /// Evaluate all strategies against a ticker update
    void evaluate(const Ticker& ticker);
    
    /// Get number of active strategies
    size_t count() const;
    
private:
    struct Entry {
        std::shared_ptr<AlertStrategy> strategy;
        AlertCallback callback;
    };
    
    std::unordered_map<int, Entry> strategies_;
    std::mutex mutex_;
    std::atomic<int> next_id_{1};
};
```

### Engine Implementation

```cpp
// src/strategy_engine.cpp

int StrategyEngine::add(std::shared_ptr<AlertStrategy> strategy, AlertCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    int id = next_id_++;
    strategies_[id] = {strategy, callback};
    return id;
}

void StrategyEngine::remove(int id) {
    std::lock_guard<std::mutex> lock(mutex_);
    strategies_.erase(id);
}

void StrategyEngine::evaluate(const Ticker& ticker) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (auto& [id, entry] : strategies_) {
        // Check if strategy applies to this symbol
        auto syms = entry.strategy->symbols();
        if (std::find(syms.begin(), syms.end(), ticker.symbol) == syms.end()) {
            continue;
        }
        
        // Check if condition is met
        if (entry.strategy->check(ticker)) {
            Alert alert{
                entry.strategy->name(),
                ticker.symbol,
                ticker.last,
                "Strategy condition met",
                std::chrono::system_clock::now()
            };
            
            // Fire callback
            entry.callback(alert);
        }
    }
}
```

### Integration with Client

```cpp
// In client_impl.cpp

void KrakenClient::Impl::dispatch(const Message& msg) {
    if (msg.type == MessageType::Ticker) {
        auto ticker = msg.as_ticker();
        
        // Call user callback
        {
            std::shared_lock lock(callbacks_mutex_);
            if (ticker_callback_) {
                ticker_callback_(ticker);
            }
        }
        
        // Evaluate strategies
        strategy_engine_.evaluate(ticker);
    }
    // ... other message types
}
```

---

## Custom Strategy Example

```cpp
// User creates a custom strategy
class TightSpreadHighVolume : public kraken::AlertStrategy {
public:
    TightSpreadHighVolume(const std::vector<std::string>& syms)
        : symbols_(syms) {}
    
    bool check(const kraken::Ticker& t) override {
        // Alert when spread is tight AND volume is high
        return t.spread() < 5.0 && t.volume_24h > 1000000;
    }
    
    std::string name() const override { 
        return "TightSpreadHighVolume"; 
    }
    
    std::vector<std::string> symbols() const override { 
        return symbols_; 
    }
    
private:
    std::vector<std::string> symbols_;
};

// Usage
auto custom = std::make_shared<TightSpreadHighVolume>(
    std::vector<std::string>{"BTC/USD", "ETH/USD"}
);

client.add_alert(custom, [](const auto& alert) {
    std::cout << "💰 Good trading opportunity on " << alert.symbol << "\n";
});
```

---

## Implementation Checklist

### Core (Day 2 Morning, 3-4 hours)
- [ ] `AlertStrategy` base class
- [ ] `Alert` struct
- [ ] `StrategyEngine` class
- [ ] `add_alert()` / `remove_alert()` in client
- [ ] Integration with dispatcher

### PriceAlert (Day 2 Morning, 1 hour)
- [ ] Builder pattern
- [ ] `above()` / `below()` conditions
- [ ] One-shot behavior (don't repeat fire)

### VolumeSpike (Day 2 Morning, 1-2 hours)
- [ ] Builder pattern
- [ ] Volume history tracking
- [ ] Moving average calculation
- [ ] Multiplier comparison

### Optional (If Time Permits)
- [ ] SpreadAlert
- [ ] PriceChangePercent
- [ ] Unit tests for strategies

---

## Thread Safety

- `StrategyEngine::evaluate()` runs on **dispatcher thread** (same as callbacks)
- `add_alert()` / `remove_alert()` are **thread-safe** (mutex-protected)
- Strategies should be **stateless or internally synchronized**

---

## Why This Feature Wins

1. **Practical Value** - Traders actually need alerts
2. **Easy to Demo** - Show alert firing in real-time
3. **Extensible** - Custom strategies show good design
4. **Different** - Most SDKs just stream data
5. **Quick to Implement** - 4-5 hours total

**This transforms the SDK from "data pipe" to "trading intelligence platform."**
