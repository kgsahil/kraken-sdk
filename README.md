# Kraken WebSocket SDK

A **production-grade C++ SDK** for real-time market data streaming with built-in **trading strategies** and **performance monitoring**.

[![C++17](https://img.shields.io/badge/C++-17-blue.svg)](https://en.cppreference.com/w/cpp/17)
[![License: MIT](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/Platform-Linux-lightgrey.svg)](BUILDING.md)

---

## 🎯 Key Features

| Feature | Description |
|---------|-------------|
| **Lock-Free Queue** | HFT-grade SPSC queue (rigtorp) for zero-contention message passing |
| **Order Book Checksum** | CRC32 validation to detect missed updates |
| **Trading Strategies** | Built-in alert system (PriceAlert, VolumeSpike, SpreadAlert) |
| **Performance Dashboard** | Real-time terminal UI showing metrics |
| **Subscription Management** | Pause, resume, add/remove symbols dynamically |
| **ABI Stability** | PIMPL pattern for binary compatibility |
| **Optimized** | `std::variant` messages, lock-free metrics, O(log n) order books |

---

## ⚡ Quickstart

```cpp
#include <kraken/kraken.hpp>

int main() {
    kraken::KrakenClient client;
    
    client.on_ticker([](const auto& t) {
        std::cout << t.symbol << ": $" << t.last << "\n";
    });
    
    client.subscribe(kraken::Channel::Ticker, {"BTC/USD", "ETH/USD"});
    client.run();
    
    return 0;
}
```

**5 lines to get started.**

---

## 🚨 Trading Strategies

Apply strategies to tickers and get notified when conditions are met.

### Price Alert

```cpp
#include <kraken/kraken.hpp>

int main() {
    kraken::KrakenClient client;
    
    // Alert when BTC exceeds $50,000
    auto price_alert = kraken::PriceAlert::Builder()
        .symbol("BTC/USD")
        .above(50000.0)
        .build();
    
    client.add_alert(price_alert, [](const kraken::Alert& a) {
        std::cout << "🚨 " << a.symbol << " hit $" << a.price << "\n";
    });
    
    client.subscribe(kraken::Channel::Ticker, {"BTC/USD"});
    client.run();
}
```

### Volume Spike Detection

```cpp
// Alert on 2x average volume
auto volume_spike = kraken::VolumeSpike::Builder()
    .symbols({"BTC/USD", "ETH/USD"})
    .multiplier(2.0)
    .lookback(50)  // Use last 50 samples for average
    .build();

client.add_alert(volume_spike, [](const auto& a) {
    std::cout << "📊 Volume spike: " << a.symbol << "\n";
});
```

### Spread Alert

```cpp
// Alert when spread tightens
auto spread_alert = kraken::SpreadAlert::Builder()
    .symbol("BTC/USD")
    .below(10.0)  // Alert when spread < $10
    .build();

client.add_alert(spread_alert, [](const auto& a) {
    std::cout << "💰 Tight spread on " << a.symbol << "\n";
});
```

### Custom Strategy

```cpp
class MyStrategy : public kraken::AlertStrategy {
public:
    bool check(const kraken::Ticker& t) override {
        return t.spread() < 5.0 && t.volume_24h > 1000000;
    }
    
    std::string name() const override { return "MyStrategy"; }
    std::vector<std::string> symbols() const override { return {"BTC/USD"}; }
};

auto custom = std::make_shared<MyStrategy>();
client.add_alert(custom, [](const auto& a) {
    std::cout << "Custom alert: " << a.message << "\n";
});
```

---

## 📊 Live Performance Dashboard

Real-time terminal dashboard showing SDK performance:

```bash
./build/dashboard
```

```
╔═══════════════════════════════════════════════════════════════╗
║               KRAKEN SDK LIVE DASHBOARD                       ║
╠═══════════════════════════════════════════════════════════════╣
║ Status: connected       Uptime: 00:15:32                 ║
╠═══════════════════════════════════════════════════════════════╣
║                       PERFORMANCE METRICS                     ║
╠═══════════════════════════════════════════════════════════════╣
║ Messages Received:  23,456        Messages/sec: 25.4         ║
║ Messages Processed: 23,456        Queue Depth:  0             ║
║ Messages Dropped:   0              Max Latency:  371 µs       ║
╠═══════════════════════════════════════════════════════════════╣
║                          TICKERS                              ║
╠═══════════════════════════════════════════════════════════════╣
║  Symbol     │    Price    │    Bid      │    Ask      │ Chg  ║
╠═════════════╪═════════════╪═════════════╪═════════════╪══════╣
║ BTC/USD    │ $  88117.20 │ $  88117.10 │ $  88117.20 │      ║
║ ETH/USD    │ $   2976.43 │ $   2976.42 │ $   2976.43 │      ║
╚═══════════════════════════════════════════════════════════════╝
```

---

## 🌐 Web Integration (JSON Serialization)

All data types include `to_json()` methods for easy integration with web dashboards and APIs:

```cpp
client.on_ticker([](const kraken::Ticker& t) {
    // Send to WebSocket server, REST API, or message queue
    std::string json = t.to_json();
    // {"symbol":"BTC/USD","bid":88117.10,"ask":88117.20,"last":88117.20,...}
    
    websocket_server.broadcast(json);
});

client.on_book([](const std::string& symbol, const kraken::OrderBook& book) {
    // Get top 10 levels as JSON
    std::string json = book.to_json(10);
    // {"symbol":"BTC/USD","bids":[[88117.10,1.5],...],"asks":[[88117.20,2.0],...]}
    
    redis_client.publish("orderbook:" + symbol, json);
});
```

### Order Book Analytics

```cpp
// Built-in liquidity and imbalance calculations
double bid_liq = book.total_bid_liquidity(10);   // Sum of top 10 bid quantities
double ask_liq = book.total_ask_liquidity(10);   // Sum of top 10 ask quantities
double imbalance = book.imbalance(10);           // -1 to +1 (positive = more bids)

// Use in trading logic
if (imbalance > 0.3) {
    // Strong buying pressure detected
}
```

### Metrics for Monitoring

```cpp
auto metrics = client.get_metrics();
std::cout << metrics.to_json() << std::endl;
// {"messages_received":23456,"messages_processed":23456,"queue_depth":0,...}
```

---

## 🎯 SDK Scope

### ✅ What This SDK Does

| Capability | Status |
|------------|--------|
| Real-time ticker streaming | ✅ |
| Real-time trade streaming | ✅ |
| Order book with checksum validation | ✅ |
| OHLC candle streaming | ✅ |
| Price/volume/spread alerts | ✅ |
| Custom strategy engine | ✅ |
| Performance metrics | ✅ |
| JSON serialization | ✅ |
| Auto-reconnection | ✅ |

### ❌ What This SDK Does NOT Do

| Capability | Status | Notes |
|------------|--------|-------|
| Order placement | ❌ | Use Kraken REST API |
| Order cancellation | ❌ | Use Kraken REST API |
| Account balance | ❌ | Use Kraken REST API |
| Position tracking | ❌ | Implement in your trading bot |
| Historical data | ❌ | Use Kraken REST API |

**This is a market data SDK**, not a full trading system. Use it to:
- Feed real-time data to trading bots
- Power web dashboards
- Trigger alerts and signals
- Monitor market conditions

For order execution, pair this SDK with the [Kraken REST API](https://docs.kraken.com/rest/).

---

## 🏗 Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                       KRAKEN SDK                             │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  ┌────────────┐      ┌────────────┐      ┌────────────┐     │
│  │ WebSocket  │      │   SPSC     │      │ Dispatcher │     │
│  │ I/O Thread │─────▶│   Queue    │─────▶│   Thread   │     │
│  │            │      │ (lock-free)│      │            │     │
│  └────────────┘      └────────────┘      └─────┬──────┘     │
│        │                                       │             │
│        │                                       ▼             │
│        │                               ┌────────────┐        │
│        │                               │  Strategy  │        │
│        │                               │   Engine   │        │
│        │                               └─────┬──────┘        │
│        │                                     │                │
│        ▼                                     ▼                │
│  ┌────────────┐                       ┌────────────┐         │
│  │   Kraken   │                       │    User    │         │
│  │  Exchange  │                       │ Callbacks  │         │
│  └────────────┘                       └────────────┘         │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

**Two-thread reactor pattern:**
- **I/O Thread**: Handles WebSocket connection, JSON parsing, pushes to lock-free queue
- **Dispatcher Thread**: Consumes from queue, runs callbacks and strategy evaluation

**Optimizations:**
- `std::variant` for messages (75% memory reduction)
- Lock-free atomics for metrics (zero contention)
- Condition variable for efficient thread wake-up
- O(log n) order book updates with `std::map`

---

## 📈 Performance

Benchmarked on Ubuntu 22.04 with live Kraken API:

| Metric | Verified Result |
|--------|--------|
| **JSON Parsing** | **1.0-1.75 µs** |
| **Queue Operations** | **11-13 ns** (75-88M ops/sec) |
| **Order Book Updates** | **< 2 µs** for 100 levels |
| **Messages Dropped** | **0** (even under load) |
| **Queue Capacity** | 65,536 (configurable) |
| **Throughput** | Limited by Kraken API rate (~15-20 msg/sec public) |
| **Memory per Message** | ~200 bytes (`std::variant`) |

*All benchmarks verified with Google Benchmark in Release mode. See [docs/BENCHMARK_RESULTS.md](docs/BENCHMARK_RESULTS.md).*

### Integration Benchmark (Live API)

Run end-to-end performance test:
```bash
cd build
./benchmark_integration 30  # Run for 30 seconds
```

### Microbenchmarks (Google Benchmark)

Component-level performance metrics:
```bash
cd build
./bench_parser      # JSON parsing performance
./bench_queue        # SPSC queue throughput
./bench_orderbook    # Order book operations
./bench_checksum     # CRC32 checksum calculation
```

See [docs/BENCHMARKS.md](docs/BENCHMARKS.md) for detailed benchmark documentation.

---

## 🛠 Installation

### Requirements

- **Platform:** Linux (WSL supported)
- **Compiler:** GCC 9+ or Clang 10+ with C++17 support
- **Dependencies:**
  - Boost >= 1.70 (system component)
  - OpenSSL >= 1.1.1
  - CMake >= 3.16

### Quick Build (WSL/Ubuntu)

```bash
# Install dependencies
sudo apt-get update
sudo apt-get install -y build-essential cmake libssl-dev libboost-system-dev

# Clone and build
git clone https://github.com/kgsahil/kraken-sdk.git
cd kraken-sdk
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)

# Run examples
./quickstart
./strategies
./dashboard
./orderbook
```

See [BUILDING.md](BUILDING.md) for detailed instructions.

### CMake Integration

```cmake
find_package(KrakenSDK REQUIRED)
target_link_libraries(your_app PRIVATE kraken::kraken)
```

---

## 📚 Examples

### Order Book with Checksum Validation

```cpp
#include <kraken/kraken.hpp>

int main() {
    kraken::KrakenClient client;
    
    client.on_book([](const std::string& symbol, const kraken::OrderBook& book) {
        if (!book.is_valid) {
            std::cerr << "⚠️ " << symbol << ": checksum failed!\n";
            return;
        }
        
        auto best_bid = book.best_bid();
        auto best_ask = book.best_ask();
        
        if (best_bid && best_ask) {
            std::cout << symbol << " spread: $" 
                      << (best_ask->price - best_bid->price) << "\n";
        }
    });
    
    // Subscribe to order book with depth 10
    client.subscribe_book({"BTC/USD"}, 10);
    client.run();
}
```

### Subscription Management

```cpp
// Subscribe
auto sub = client.subscribe(kraken::Channel::Ticker, {"BTC/USD", "ETH/USD"});

// Pause subscription (stops receiving updates)
sub.pause();

// Resume
sub.resume();

// Add more symbols
sub.add_symbols({"SOL/USD"});

// Remove symbols
sub.remove_symbols({"ETH/USD"});

// Unsubscribe completely
sub.unsubscribe();
```

### Connection State Monitoring

```cpp
client.on_connection_state([](kraken::ConnectionState state) {
    switch (state) {
        case kraken::ConnectionState::Connected:
            std::cout << "🟢 Connected\n";
            break;
        case kraken::ConnectionState::Reconnecting:
            std::cout << "🟡 Reconnecting...\n";
            break;
        case kraken::ConnectionState::Disconnected:
            std::cout << "🔴 Disconnected\n";
            break;
        default:
            break;
    }
});
```

### Metrics

```cpp
auto metrics = client.get_metrics();

std::cout << "Messages/sec: " << metrics.messages_per_second() << "\n";
std::cout << "Total processed: " << metrics.messages_processed << "\n";
std::cout << "Dropped: " << metrics.messages_dropped << "\n";
std::cout << "Max latency: " << metrics.latency_max_us.count() << " µs\n";
std::cout << "Uptime: " << metrics.uptime_string() << "\n";
```

---

## 🔧 Configuration

```cpp
auto config = kraken::ClientConfig::Builder()
    .url("wss://ws.kraken.com/v2")
    .queue_capacity(131072)  // Power of 2 recommended
    .reconnect_attempts(10)
    .reconnect_delay(std::chrono::milliseconds(1000))
    .validate_checksums(true)
    .build();

kraken::KrakenClient client(config);
```

### Configuration Options

| Option | Default | Description |
|--------|---------|-------------|
| `url` | `wss://ws.kraken.com/v2` | WebSocket endpoint |
| `queue_capacity` | `65536` | Message queue size (power of 2) |
| `reconnect_attempts` | `10` | Max reconnection attempts |
| `reconnect_delay` | `1000ms` | Delay between attempts |
| `validate_checksums` | `true` | Enable CRC32 validation |

---

## 📖 API Reference

### KrakenClient

```cpp
class KrakenClient {
public:
    // Construction
    KrakenClient();
    explicit KrakenClient(ClientConfig config);
    ~KrakenClient();
    
    // Non-copyable, movable
    KrakenClient(const KrakenClient&) = delete;
    KrakenClient(KrakenClient&&) noexcept;
    
    // Callbacks (thread-safe)
    void on_ticker(TickerCallback callback);
    void on_trade(TradeCallback callback);
    void on_book(BookCallback callback);
    void on_ohlc(OHLCCallback callback);
    void on_error(ErrorCallback callback);
    void on_connection_state(ConnectionStateCallback callback);
    
    // Connection
    void connect();
    void disconnect();
    bool is_connected() const;
    
    // Subscriptions
    Subscription subscribe(Channel channel, std::vector<std::string> symbols);
    Subscription subscribe_book(std::vector<std::string> symbols, int depth = 10);
    
    // Strategies
    int add_alert(std::shared_ptr<AlertStrategy> strategy, AlertCallback callback);
    void remove_alert(int alert_id);
    size_t alert_count() const;
    
    // Event loop
    void run();           // Blocking
    void run_async();     // Non-blocking (spawns threads)
    void stop();
    bool is_running() const;
    
    // Metrics
    Metrics get_metrics() const;
};
```

### Data Types

```cpp
// Ticker
struct Ticker {
    std::string symbol;
    double bid, ask, last;
    double volume_24h, high_24h, low_24h;
    std::string timestamp;
    
    double spread() const;      // ask - bid
    double mid_price() const;   // (bid + ask) / 2
};

// Order Book
struct OrderBook {
    std::string symbol;
    std::vector<PriceLevel> bids, asks;
    bool is_valid;  // CRC32 checksum passed
    
    const PriceLevel* best_bid() const;
    const PriceLevel* best_ask() const;
    double spread() const;
};

// Subscription Handle
class Subscription {
public:
    void pause();
    void resume();
    void unsubscribe();
    void add_symbols(const std::vector<std::string>& symbols);
    void remove_symbols(const std::vector<std::string>& symbols);
    bool is_active() const;
    bool is_paused() const;
    Channel channel() const;
    std::vector<std::string> symbols() const;
};
```

### Channels

```cpp
enum class Channel {
    Ticker,  // 24h ticker updates
    Trade,   // Recent trades
    Book,    // Order book (requires depth)
    OHLC     // OHLC candles
};
```

---

## 🧵 Thread Safety

| Operation | Thread Safety |
|-----------|---------------|
| Callback registration (`on_ticker`, etc.) | ✅ Thread-safe |
| Subscriptions (`subscribe`, `subscribe_book`) | ✅ Thread-safe |
| Alert management (`add_alert`, `remove_alert`) | ✅ Thread-safe |
| Connection queries (`is_connected`, `get_metrics`) | ✅ Thread-safe |
| `stop()` | ✅ Thread-safe (can call from any thread) |
| `run()`, `run_async()` | ⚠️ Call once from single thread |

**Notes:**
- `stop()` is thread-safe and can be called from signal handlers or other threads
- `run()` and `run_async()` should only be called once - they start the event loop
- Callbacks are invoked on the dispatcher thread. If your callbacks need thread safety, use synchronization primitives inside them

---

## 📁 Project Structure

```
kraken-sdk/
├── include/kraken/     # Public API headers
├── src/                # Implementation
├── examples/           # Example programs
├── tools/              # Benchmark tool
├── tests/              # Unit tests
├── docs/               # Technical documentation
├── CMakeLists.txt
├── BUILDING.md         # Build instructions
└── README.md           # This file
```

---

## 🧪 Testing

```bash
cd build
ctest --output-on-failure
```

---

## 📄 License

MIT License - see [LICENSE](LICENSE) file for details.

---

## 🙏 Acknowledgements

- [rigtorp/SPSCQueue](https://github.com/rigtorp/SPSCQueue) - Lock-free queue
- [RapidJSON](https://github.com/Tencent/rapidjson) - Fast JSON parsing
- [Boost.Beast](https://github.com/boostorg/beast) - WebSocket client

---

## 🚀 Built for Kraken Forge Hackathon 2025

**This SDK transforms raw market data into actionable trading intelligence.**
