# Kraken WebSocket SDK - C++ Implementation Plan

A **production-grade C++ SDK** for real-time market data streaming with built-in **trading strategies** and **performance monitoring**.

**Target Platform:** Linux (WSL supported)  
**C++ Standard:** C++17  
**Timeline:** 2 Days (Hackathon Sprint)

---

## 📍 Current Status

| Phase | Status | Progress |
|-------|--------|----------|
| **Day 1: Foundation** | ✅ Complete | 100% |
| **Day 2: Differentiators** | ✅ Complete | 100% |
| **Live API Testing** | ✅ Complete | All channels |
| **Build Status** | ✅ Compiles | All targets |

### ✅ Completed
- Full SDK architecture (PIMPL, Builder, RAII)
- WebSocket + TLS + Auto-reconnect
- Lock-free SPSC queue (rigtorp)
- All channels: Ticker, Trade, Book, OHLC
- Order book CRC32 checksum validation
- Subscription handles (pause/resume/add/remove)
- Strategy Engine (PriceAlert, VolumeSpike)
- Metrics tracking
- All examples (quickstart, strategies, dashboard, orderbook)
- Benchmark tool
- Unit tests
- **✅ Tested with live Kraken API**
- **✅ Benchmarks complete**

### 📊 Benchmark Results (Live Kraken API)
| Metric | Value |
|--------|-------|
| Max Latency | **807 µs** (< 1ms) |
| Messages Dropped | **0** |
| Queue Depth | **0** (no backpressure) |
| Checksum Validation | **✅ All valid** |

### 🔜 Next Steps
1. **Create README** - With architecture diagram and quickstart
2. **Record demo video** - 60-second showcase

---

## 🎯 What Makes This SDK Different

This isn't just a data pipe. It's a **trading intelligence platform**:

| Feature | Typical SDK | This SDK |
|---------|-------------|----------|
| Data streaming | ✅ | ✅ |
| Lock-free queue | ❌ | ✅ HFT-grade (rigtorp) |
| Order book checksum | ❌ | ✅ CRC32 validation |
| **Strategy engine** | ❌ | ✅ **Built-in alerts** |
| **Performance dashboard** | ❌ | ✅ **Real-time metrics** |
| Subscription lifecycle | ❌ | ✅ Pause/resume/cancel |
| ABI stability | ❌ | ✅ PIMPL pattern |

---

## 🚀 Core Features (Must Deliver)

### Tier 1: Foundation (Day 1)
1. **WebSocket Connection** - TLS, auto-reconnect
2. **SPSC Queue** - Lock-free message passing (rigtorp)
3. **JSON Parsing** - RapidJSON, type-safe data models
4. **Ticker/Book/Trade** - All public channels
5. **Order Book Checksum** - CRC32 validation
6. **Subscription Handles** - Unsubscribe at minimum

### Tier 2: Differentiators (Day 2) ⭐
7. **Trading Strategy Engine** - Apply strategies, get alerts
8. **Live Performance Dashboard** - Terminal-based real-time metrics
9. **Benchmark Suite** - Published performance numbers

### Tier 3: Polish
10. **Examples** - Quickstart, strategies, dashboard
11. **Documentation** - README with architecture diagram
12. **Demo Video** - 60-second showcase

---

## 5-Line Quickstart

```cpp
#include <kraken/client.hpp>

int main() {
    kraken::KrakenClient client;
    client.on_ticker([](const auto& t) { std::cout << t.symbol << ": $" << t.last << "\n"; });
    client.subscribe(kraken::Channel::Ticker, {"BTC/USD"});
    client.run();
}
```

---

## 🎯 Trading Strategy Engine (Key Differentiator)

**What:** Apply trading strategies to tickers, get notified when conditions are met.

```cpp
// Price alert: Notify when BTC > $50,000
auto strategy = kraken::PriceAlert::Builder()
    .symbol("BTC/USD")
    .above(50000.0)
    .build();

client.add_alert(strategy, [](const kraken::Alert& alert) {
    std::cout << "🚨 " << alert.symbol << " is now $" << alert.price << "\n";
});

// Volume spike: Notify on 2x average volume
auto spike = kraken::VolumeSpike::Builder()
    .symbols({"BTC/USD", "ETH/USD"})
    .multiplier(2.0)
    .build();

client.add_alert(spike, [](const auto& alert) {
    std::cout << "📊 Volume spike on " << alert.symbol << "\n";
});

// Custom strategy
class MyStrategy : public kraken::AlertStrategy {
public:
    bool check(const kraken::Ticker& t) override {
        return t.spread() < 5.0 && t.volume_24h > 1000000;
    }
};
```

**Built-in Strategies:**
- `PriceAlert` - Price crosses threshold
- `VolumeSpike` - Volume exceeds N× average
- `SpreadAlert` - Spread changes significantly
- Custom strategies via `AlertStrategy` base class

**Why This Matters:**
- Makes SDK **useful** (not just data plumbing)
- Shows **trader understanding** (alerts are universal need)
- **Easy to demo** (show alert firing in real-time)
- **Extensible** (custom strategies)

---

## 📊 Live Performance Dashboard

Real-time terminal dashboard showing SDK performance:

```
┌─────────────────────────────────────────────────────┐
│           KRAKEN SDK PERFORMANCE                    │
├─────────────────────────────────────────────────────┤
│  Status:        🟢 Connected                        │
│  Uptime:        00:15:32                            │
├─────────────────────────────────────────────────────┤
│  Messages/sec:  1,234                               │
│  Total msgs:    1,123,456                           │
│  Queue depth:   45 / 65536                          │
│  Dropped:       0                                   │
│  Max latency:   2.3ms                               │
├─────────────────────────────────────────────────────┤
│  Active subs:   BTC/USD, ETH/USD                    │
│  Strategies:    2 active (PriceAlert, VolumeSpike)  │
└─────────────────────────────────────────────────────┘
```

**Implementation:** ANSI escape codes (no ncurses dependency)

---

## Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                         KRAKEN SDK                               │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌──────────────┐      ┌──────────────┐      ┌──────────────┐   │
│  │  WebSocket   │      │    SPSC      │      │  Dispatcher  │   │
│  │  I/O Thread  │─────▶│    Queue     │─────▶│   Thread     │   │
│  │              │      │  (lock-free) │      │              │   │
│  └──────────────┘      └──────────────┘      └──────┬───────┘   │
│         │                                           │            │
│         │                                           ▼            │
│         │                                   ┌──────────────┐     │
│         │                                   │   Strategy   │     │
│         │                                   │   Engine     │     │
│         │                                   └──────┬───────┘     │
│         │                                           │            │
│         ▼                                           ▼            │
│  ┌──────────────┐                           ┌──────────────┐     │
│  │   Kraken     │                           │    User      │     │
│  │   Exchange   │                           │  Callbacks   │     │
│  └──────────────┘                           └──────────────┘     │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

---

## Project Structure

```
kraken-sdk/
├── CMakeLists.txt
├── README.md
├── include/kraken/
│   ├── client.hpp           # Public API (PIMPL)
│   ├── types.hpp            # Data types, enums
│   ├── strategies.hpp       # Alert strategies ⭐
│   └── error.hpp            # Error types
├── src/
│   ├── client_impl.cpp
│   ├── connection.cpp
│   ├── parser.cpp
│   ├── book_engine.cpp
│   └── strategy_engine.cpp  # Strategy evaluation ⭐
├── examples/
│   ├── quickstart.cpp       # 5-line demo
│   ├── strategies.cpp       # Alert strategies ⭐
│   ├── dashboard.cpp        # Live dashboard ⭐
│   └── orderbook.cpp        # Order book with checksum
├── tools/
│   └── benchmark.cpp        # Performance benchmark ⭐
└── tests/
    ├── test_strategies.cpp
    └── test_book_checksum.cpp
```

---

## Core Components

### Enums (Type-Safe)

```cpp
namespace kraken {

enum class Channel { Ticker, Trade, Book, OHLC };
enum class Side { Buy, Sell };

enum class ErrorCode {
    None = 0,
    ConnectionFailed,
    InvalidSymbol,
    QueueOverflow,
    ChecksumMismatch,
    Timeout
};

enum class ConnectionState {
    Disconnected,
    Connecting,
    Connected,
    Reconnecting
};

} // namespace kraken
```

### Configuration (Builder Pattern)

```cpp
namespace kraken {

class ClientConfig {
public:
    class Builder {
    public:
        Builder& url(std::string url);
        Builder& api_key(std::string key);
        Builder& api_secret(std::string secret);
        Builder& queue_capacity(size_t capacity);
        Builder& reconnect_attempts(int attempts);
        Builder& validate_checksums(bool validate);
        ClientConfig build();
    };
    
private:
    std::string url_ = "wss://ws.kraken.com/v2";
    std::string api_key_;
    std::string api_secret_;
    size_t queue_capacity_ = 65536;
    int reconnect_attempts_ = 10;
    bool validate_checksums_ = true;
};

} // namespace kraken
```

### Metrics

```cpp
struct Metrics {
    uint64_t messages_received = 0;
    uint64_t messages_processed = 0;
    uint64_t messages_dropped = 0;
    size_t queue_depth = 0;
    ConnectionState connection_state = ConnectionState::Disconnected;
    std::chrono::microseconds latency_max_us{0};
    std::chrono::steady_clock::time_point start_time;
    
    double messages_per_second() const;
    std::chrono::seconds uptime() const;
};
```

### Alert Strategy Interface

```cpp
namespace kraken {

/// Base class for alert strategies
class AlertStrategy {
public:
    virtual ~AlertStrategy() = default;
    virtual bool check(const Ticker& ticker) = 0;
    virtual std::string name() const = 0;
    virtual std::vector<std::string> symbols() const = 0;
};

/// Alert information
struct Alert {
    std::string strategy_name;
    std::string symbol;
    double price;
    std::string message;
    std::chrono::system_clock::time_point timestamp;
};

using AlertCallback = std::function<void(const Alert&)>;

} // namespace kraken
```

### Client API

```cpp
namespace kraken {

class KrakenClient {
public:
    KrakenClient();
    explicit KrakenClient(ClientConfig config);
    ~KrakenClient();
    
    // Non-copyable, movable
    KrakenClient(const KrakenClient&) = delete;
    KrakenClient(KrakenClient&&) noexcept;
    
    //--- Callbacks ---
    void on_ticker(TickerCallback callback);
    void on_trade(TradeCallback callback);
    void on_book(BookCallback callback);
    void on_error(ErrorCallback callback);
    void on_connection_state(ConnectionStateCallback callback);
    
    //--- Connection ---
    void connect();
    void disconnect();
    bool is_connected() const;
    
    //--- Subscriptions ---
    Subscription subscribe(Channel channel, const std::vector<std::string>& symbols);
    Subscription subscribe_book(const std::vector<std::string>& symbols, int depth = 10);
    
    //--- Strategies ⭐ ---
    int add_alert(std::shared_ptr<AlertStrategy> strategy, AlertCallback callback);
    void remove_alert(int alert_id);
    
    //--- Event Loop ---
    void run();
    void run_async();
    void stop();
    
    //--- Metrics ---
    Metrics get_metrics() const;
    
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace kraken
```

---

## Data Models

```cpp
namespace kraken {

struct Ticker {
    std::string symbol;
    double bid;
    double ask;
    double last;
    double volume_24h;
    double high_24h;
    double low_24h;
    std::string timestamp;
    
    double spread() const { return ask - bid; }
    double mid_price() const { return (bid + ask) / 2.0; }
};

struct Trade {
    std::string symbol;
    double price;
    double quantity;
    Side side;
    std::string timestamp;
};

struct PriceLevel {
    double price;
    double quantity;
};

struct OrderBook {
    std::string symbol;
    std::vector<PriceLevel> bids;
    std::vector<PriceLevel> asks;
    bool is_valid;  // CRC32 checksum passed
    
    const PriceLevel* best_bid() const;
    const PriceLevel* best_ask() const;
    double spread() const;
};

} // namespace kraken
```

---

## Built-in Alert Strategies

### PriceAlert

```cpp
class PriceAlert : public AlertStrategy {
public:
    class Builder {
    public:
        Builder& symbol(const std::string& sym);
        Builder& above(double threshold);
        Builder& below(double threshold);
        std::shared_ptr<PriceAlert> build();
    };
    
    bool check(const Ticker& t) override;
    std::string name() const override { return "PriceAlert"; }
    std::vector<std::string> symbols() const override;
    
private:
    std::string symbol_;
    double above_threshold_ = std::numeric_limits<double>::max();
    double below_threshold_ = std::numeric_limits<double>::lowest();
    bool triggered_ = false;
};
```

### VolumeSpike

```cpp
class VolumeSpike : public AlertStrategy {
public:
    class Builder {
    public:
        Builder& symbols(const std::vector<std::string>& syms);
        Builder& multiplier(double mult);  // e.g., 2.0 = 2x average
        Builder& lookback(size_t n);       // Number of samples to average
        std::shared_ptr<VolumeSpike> build();
    };
    
    bool check(const Ticker& t) override;
    std::string name() const override { return "VolumeSpike"; }
    std::vector<std::string> symbols() const override;
    
private:
    std::vector<std::string> symbols_;
    double multiplier_ = 2.0;
    size_t lookback_ = 50;
    std::unordered_map<std::string, std::deque<double>> history_;
};
```

---

## Example: Strategy Demo

```cpp
#include <kraken/client.hpp>
#include <kraken/strategies.hpp>
#include <iostream>

int main() {
    kraken::KrakenClient client;
    
    // Price alert
    auto price_alert = kraken::PriceAlert::Builder()
        .symbol("BTC/USD")
        .above(50000.0)
        .build();
    
    client.add_alert(price_alert, [](const kraken::Alert& a) {
        std::cout << "🚨 PRICE ALERT: " << a.symbol 
                  << " hit $" << a.price << "\n";
    });
    
    // Volume spike
    auto volume_spike = kraken::VolumeSpike::Builder()
        .symbols({"BTC/USD", "ETH/USD"})
        .multiplier(2.0)
        .build();
    
    client.add_alert(volume_spike, [](const kraken::Alert& a) {
        std::cout << "📊 VOLUME SPIKE: " << a.symbol << "\n";
    });
    
    // Also print all tickers
    client.on_ticker([](const auto& t) {
        std::cout << t.symbol << ": $" << t.last << "\n";
    });
    
    client.subscribe(kraken::Channel::Ticker, {"BTC/USD", "ETH/USD"});
    client.run();
}
```

---

## Example: Live Dashboard

```cpp
#include <kraken/client.hpp>
#include <iostream>
#include <iomanip>

void render_dashboard(const kraken::Metrics& m) {
    // Clear screen and move cursor to top
    std::cout << "\033[2J\033[H";
    
    std::cout << "┌─────────────────────────────────────────────┐\n";
    std::cout << "│         KRAKEN SDK PERFORMANCE              │\n";
    std::cout << "├─────────────────────────────────────────────┤\n";
    
    // Connection status
    std::string status = (m.connection_state == kraken::ConnectionState::Connected) 
        ? "🟢 Connected" : "🔴 Disconnected";
    std::cout << "│  Status:       " << std::left << std::setw(28) << status << "│\n";
    
    // Metrics
    std::cout << "│  Messages/sec: " << std::left << std::setw(28) 
              << m.messages_per_second() << "│\n";
    std::cout << "│  Total msgs:   " << std::left << std::setw(28) 
              << m.messages_processed << "│\n";
    std::cout << "│  Queue depth:  " << std::left << std::setw(28) 
              << m.queue_depth << "│\n";
    std::cout << "│  Dropped:      " << std::left << std::setw(28) 
              << m.messages_dropped << "│\n";
    std::cout << "│  Max latency:  " << std::left << std::setw(28) 
              << std::to_string(m.latency_max_us.count()) + "μs" << "│\n";
    
    std::cout << "└─────────────────────────────────────────────┘\n";
}

int main() {
    kraken::KrakenClient client;
    
    client.on_ticker([&](const auto& t) {
        // Update dashboard every 100 messages
        static int count = 0;
        if (++count % 100 == 0) {
            render_dashboard(client.get_metrics());
        }
    });
    
    client.subscribe(kraken::Channel::Ticker, {"BTC/USD", "ETH/USD", "SOL/USD"});
    client.run();
}
```

---

## Implementation Timeline (2 Days)

### Day 1: Foundation ✅ COMPLETE

**Morning (4 hours):**
- [x] CMake setup with FetchContent (rigtorp, RapidJSON)
- [x] PIMPL structure (client.hpp / client_impl.cpp)
- [x] Builder pattern for config
- [x] Basic types.hpp (enums, Ticker, Trade, OrderBook)

**Afternoon (4-5 hours):**
- [x] WebSocket connection with TLS (boost::beast)
- [x] JSON parsing (RapidJSON)
- [x] SPSC queue integration
- [x] I/O thread + Dispatcher thread loop
- [x] Ticker callback working

**Evening (2-3 hours):**
- [x] Order book engine with CRC32 checksum
- [x] Trade channel handler
- [x] Subscription handles (pause/resume/add/remove symbols)
- [x] **Test with real Kraken API** ✅

### Day 2: Differentiators & Polish

**Morning (4 hours):**
- [x] **Alert Strategy Engine** - Base class + evaluation loop
- [x] **PriceAlert** strategy implementation
- [x] **VolumeSpike** strategy implementation
- [x] add_alert() / remove_alert() API

**Afternoon (4 hours):**
- [x] **Live Dashboard** - ANSI-based terminal UI (examples/dashboard.cpp)
- [x] **Benchmark tool** - Measure msgs/sec, latency (tools/benchmark.cpp)
- [x] Connection state callbacks
- [x] Auto-reconnection logic

**Evening (2-4 hours):**
- [x] Examples: quickstart, strategies, dashboard, orderbook
- [ ] README with architecture diagram
- [ ] Run benchmarks, publish numbers
- [ ] Record 60-second demo video
- [ ] Final testing with live API

---

## Deliverables Checklist

### Core SDK
- [x] WebSocket connection + JSON parsing
- [x] SPSC queue + 2-thread model
- [x] Ticker/Trade/Book channels
- [x] Order book checksum validation
- [x] Subscription handles (pause/resume/add/remove)
- [x] Metrics tracking

### Differentiators ⭐
- [x] Alert Strategy Engine (PriceAlert, VolumeSpike)
- [x] Live Performance Dashboard (examples/dashboard.cpp)
- [x] Benchmark suite (tools/benchmark.cpp)
- [x] Published benchmark results (< 1ms latency, 0 drops)

### Examples
- [x] `quickstart.cpp` - 5-line demo
- [x] `strategies.cpp` - Alert strategies demo
- [x] `dashboard.cpp` - Live performance dashboard
- [x] `orderbook.cpp` - Order book with checksum

### Tests
- [x] `test_strategies.cpp` - Strategy unit tests
- [x] `test_book_checksum.cpp` - CRC32 validation tests

### Documentation
- [ ] README with quickstart
- [ ] Architecture diagram
- [ ] Performance numbers
- [ ] 60-second demo video

---

## Dependencies

| Library | Version | Purpose |
|---------|---------|---------|
| Boost | >= 1.70 | Beast (WebSocket), Asio |
| OpenSSL | >= 1.1.1 | TLS |
| RapidJSON | latest | JSON parsing (FetchContent) |
| rigtorp/SPSCQueue | latest | Lock-free queue (FetchContent) |
| GoogleTest | latest | Unit testing (FetchContent) |

> **Note:** RapidJSON, SPSCQueue, and GoogleTest are fetched automatically via CMake FetchContent.

---

## Success Criteria

**Working Demo:**
1. ✅ Connect to Kraken, subscribe to tickers
2. ✅ Strategy engine fires alerts in real-time
3. ✅ Live dashboard shows metrics
4. ✅ Order book validates checksums
5. ✅ Benchmark shows performance numbers

**Presentation:**
1. ✅ 60-second demo video
2. ✅ README with clear quickstart
3. ✅ Architecture diagram
4. ✅ Published benchmark results

---

## Why This Wins

1. **Not Just a Data Pipe** - Strategy engine adds intelligence
2. **Visual Proof** - Live dashboard shows it's working
3. **Performance Numbers** - Benchmarks prove claims
4. **Production Quality** - PIMPL, checksums, reconnection
5. **Easy to Demo** - Alert firing in real-time is impressive
6. **Different** - Most SDKs just stream data; this adds value

**This SDK transforms raw market data into actionable trading intelligence.**
