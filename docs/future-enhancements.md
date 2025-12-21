# Future Enhancements

Review this after core SDK implementation is complete.

---

## Quick Wins (Low Effort, High Impact)

| Feature | Description | Why It Helps |
|---------|-------------|--------------|
| **Latency Tracking** | P50/P99 latency in Metrics struct | Proves "HFT-grade" claims with numbers |
| **Rate Limit Handling** | Callback when rate limited, auto-throttle | Production concern, prevents bans |
| **Snapshot Request** | Get current book state without streaming | Useful for initialization |

---

## Unique "Wow" Features (Differentiators)

Pick ONE to stand out from competitors:

### 1. Time-Travel Replay
```cpp
// Record market data to file
client.start_recording("btc_session.bin");
// Later: replay for strategy testing
client.replay("btc_session.bin", 2.0);  // 2x speed
```
**Why:** Strategy developers can test without live connection.

### 2. Anomaly Detection
```cpp
client.on_anomaly([](const Anomaly& a) {
    // a.type: SpreadSpike, VolumeSpike, PriceGap
    // a.symbol, a.severity, a.details
});
client.enable_anomaly_detection({
    .spread_threshold = 0.5,  // 0.5% spread triggers alert
    .volume_spike = 3.0       // 3x normal volume
});
```
**Why:** Traders love alerts. Shows domain knowledge.

### 3. Multi-Exchange Abstraction
```cpp
// Same interface, different exchanges
auto kraken = Exchange::create("kraken", config);
auto binance = Exchange::create("binance", config);  // Future

kraken->subscribe(Channel::Ticker, {"BTC/USD"}, callback);
```
**Why:** Shows extensibility thinking. Kraken is reference implementation.

### 4. Latency Heatmap API
```cpp
struct LatencyBucket {
    std::chrono::microseconds min, max;
    uint64_t count;
};
std::vector<LatencyBucket> histogram = client.get_latency_histogram();
```
**Why:** Visualizers can show latency distribution over time.

### 5. Strategy Hooks
```cpp
client.on_signal([](const Signal& s) {
    // Built-in momentum/mean-reversion signals
    // s.type: CrossOver, Breakout, MeanReversion
});
client.enable_signal(SignalType::Momentum, {
    .fast_period = 10,
    .slow_period = 30
});
```
**Why:** Turns SDK into a mini-strategy framework.

---

## Benchmarks to Add

```cpp
// Run and report:
- Messages/second throughput
- P50, P95, P99 message latency
- Queue push/pop latency
- Order round-trip time
- Memory usage (peak, steady-state)
- Comparison vs other SDKs (if time)
```

---

## Demo Ideas

1. **Terminal Order Book** - ncurses-based, shows bids/asks updating live
2. **Spread Chart** - ASCII chart of spread over time
3. **Trade Tape** - Scrolling trades with color (green=buy, red=sell)
4. **Latency Monitor** - Live P99 latency display

---

## Review Checklist

Before submission, verify:
- [ ] All examples compile and run
- [ ] Benchmarks show competitive numbers
- [ ] Video demo recorded (2 minutes max)
- [ ] README has quick-start that works
- [ ] No compiler warnings
- [ ] Tests pass

