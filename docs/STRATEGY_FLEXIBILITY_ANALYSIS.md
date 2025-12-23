# Strategy System Flexibility Analysis

## Current Flexibility ✅

### What Users Can Do

1. **Simple Custom Strategies** - Very Easy
   ```cpp
   class MyStrategy : public AlertStrategy {
       bool check(const Ticker& ticker) override {
           return ticker.spread() > 50.0;
       }
       std::string name() const override { return "MyStrategy"; }
       std::vector<std::string> symbols() const override { 
           return {"BTC/USD"}; 
       }
   };
   ```

2. **Stateful Strategies** - Easy (like VolumeSpike)
   - Can store internal state (history, counters, etc.)
   - Can track multiple symbols independently
   - Can implement complex logic in `check()`

3. **Access to Full Ticker Data**
   - All ticker fields available: bid, ask, last, volume, high, low
   - Helper methods: `spread()`, `spread_percent()`, `mid_price()`, `imbalance()`

## ✅ IMPLEMENTED: Multi-Data Source Support

### What's Now Available

1. **OrderBook-Based Strategies** ✅
   - Override `needs_orderbook()` to return `true`
   - Implement `check(const OrderBook& book)`
   - Access: `total_bid_liquidity()`, `total_ask_liquidity()`, `imbalance()`, depth levels

2. **Trade-Based Strategies** ✅
   - Override `needs_trades()` to return `true`
   - Implement `check(const Trade& trade)`
   - Access: price, quantity, side, value

3. **Multi-Source Strategies** ✅
   - Implement `check(const Ticker& ticker, const OrderBook& book)`
   - Automatically called when both ticker and order book updates are available
   - Enables complex conditions like "price threshold AND order book support"

4. **Custom Alert Messages** ✅
   - Override `get_alert_message(const Ticker& ticker)` 
   - All strategies can now provide context-rich messages
   - No more generic "Strategy condition met" messages

## Remaining Limitations (Future Enhancements)

### 1. **OHLC Data Not Available**
- Cannot access candle/pattern data yet
- Would require subscribing to OHLC channel and storing history

### 2. **No Strategy Composition**
- Cannot combine strategies with AND/OR logic
- Cannot create strategy chains (if A then check B)
- No way to disable/enable strategies dynamically

### 4. **No Strategy Composition**
- Cannot combine strategies with AND/OR logic
- Cannot create strategy chains (if A then check B)
- No way to disable/enable strategies dynamically

### 5. **No Configuration Support**
- Cannot configure strategies from config files
- No environment variable support for strategy parameters
- Must hardcode thresholds in code

## Proposed Improvements 🚀

### Priority 1: Multi-Data Source Strategies

**Problem:** Strategies can only use Ticker data.

**Solution:** Extend `AlertStrategy` to support multiple data types:

```cpp
class AlertStrategy {
public:
    // Current (Ticker-based)
    virtual bool check(const Ticker& ticker) { return false; }
    
    // New: OrderBook-based
    virtual bool check(const OrderBook& book) { return false; }
    
    // New: Trade-based  
    virtual bool check(const Trade& trade) { return false; }
    
    // New: Multi-source
    virtual bool check(const Ticker& ticker, const OrderBook& book) { return false; }
};
```

**Benefit:** Enables strategies like:
- "Alert when order book imbalance > 70% AND price crosses threshold"
- "Alert when large trade (>$1M) executes AND spread widens"
- "Alert when bid depth drops below threshold"

### Priority 2: Custom Alert Messages

**Problem:** Only PriceAlert can provide detailed messages.

**Solution:** Add virtual method for custom messages:

```cpp
class AlertStrategy {
public:
    // New: Allow strategies to provide custom alert messages
    virtual std::string get_alert_message(const Ticker& ticker) const {
        return "Strategy condition met";
    }
};
```

**Benefit:** All strategies can provide context-rich messages.

### Priority 3: Strategy Composition

**Problem:** Cannot combine strategies or create complex logic.

**Solution:** Add composite strategies:

```cpp
// AND strategy
auto combined = CompositeStrategy::and_(
    price_alert,
    volume_spike
);

// OR strategy  
auto either = CompositeStrategy::or_(
    price_alert,
    spread_alert
);
```

**Benefit:** Users can create complex trading logic without coding.

### Priority 4: Configuration Support

**Problem:** Must hardcode strategy parameters.

**Solution:** Add strategy factories from config:

```cpp
// From config file
auto alert = StrategyFactory::from_config("price_alert_btc.json");
// Or from environment
auto alert = StrategyFactory::from_env("PRICE_ALERT_BTC");
```

**Benefit:** Deploy strategies without code changes.

## Recommendation

**Start with Priority 1 (Multi-Data Source)** - This is the biggest limitation and would unlock the most powerful strategies.

**Then Priority 2 (Custom Messages)** - Easy to implement, high user value.

**Priority 3 and 4** can come later as they're more advanced features.

