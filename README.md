# Kraken WebSocket SDK

A **production-grade C++ SDK** for real-time market data streaming with built-in **trading strategies** and **performance monitoring**.

[![Demo Video](https://img.shields.io/badge/Demo-Video-red)](# "Coming Soon")
[![C++17](https://img.shields.io/badge/C++-17-blue.svg)](https://en.cppreference.com/w/cpp/17)
[![License: MIT](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)

---

## 🎯 What Makes This Different

| Feature | Typical SDK | This SDK |
|---------|-------------|----------|
| Data streaming | ✅ | ✅ |
| Lock-free queue | ❌ | ✅ HFT-grade |
| Order book checksum | ❌ | ✅ CRC32 |
| **Alert strategies** | ❌ | ✅ **Built-in** |
| **Performance dashboard** | ❌ | ✅ **Real-time** |
| Subscription lifecycle | ❌ | ✅ Pause/resume |
| ABI stability | ❌ | ✅ PIMPL pattern |

---

## ⚡ Quickstart (5 Lines)

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

## 🚨 Trading Strategies (Key Feature)

Apply strategies to tickers. Get notified when conditions are met.

```cpp
#include <kraken/client.hpp>
#include <kraken/strategies.hpp>

int main() {
    kraken::KrakenClient client;
    
    // Price alert: Notify when BTC > $50,000
    auto price_alert = kraken::PriceAlert::Builder()
        .symbol("BTC/USD")
        .above(50000.0)
        .build();
    
    client.add_alert(price_alert, [](const kraken::Alert& a) {
        std::cout << "🚨 ALERT: " << a.symbol << " hit $" << a.price << "\n";
    });
    
    // Volume spike: Notify on 2x average volume
    auto volume_spike = kraken::VolumeSpike::Builder()
        .symbols({"BTC/USD", "ETH/USD"})
        .multiplier(2.0)
        .build();
    
    client.add_alert(volume_spike, [](const auto& a) {
        std::cout << "📊 Volume spike on " << a.symbol << "\n";
    });
    
    client.subscribe(kraken::Channel::Ticker, {"BTC/USD", "ETH/USD"});
    client.run();
}
```

**Built-in Strategies:**
- `PriceAlert` - Alert when price crosses threshold
- `VolumeSpike` - Alert on unusual volume
- Custom strategies via `AlertStrategy` base class

---

## 📊 Live Performance Dashboard

Real-time terminal dashboard showing SDK performance:

```
┌─────────────────────────────────────────────┐
│         KRAKEN SDK PERFORMANCE              │
├─────────────────────────────────────────────┤
│  Status:       🟢 Connected                 │
│  Messages/sec: 1,234                        │
│  Total msgs:   1,123,456                    │
│  Queue depth:  45 / 65536                   │
│  Dropped:      0                            │
│  Max latency:  2.3ms                        │
└─────────────────────────────────────────────┘
```

Run the dashboard example:
```bash
./examples/dashboard
```

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
│        ▼                                     ▼               │
│  ┌────────────┐                       ┌────────────┐         │
│  │   Kraken   │                       │    User    │         │
│  │  Exchange  │                       │ Callbacks  │         │
│  └────────────┘                       └────────────┘         │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

**Only 2 threads.** I/O thread handles WebSocket, dispatcher thread runs callbacks and strategies.

---

## 📈 Performance

Benchmarked on Ubuntu 22.04, Intel i7:

| Metric | Result |
|--------|--------|
| Messages/sec | 50,000+ |
| Max latency | < 5ms |
| Queue capacity | 65,536 |
| Memory usage | < 50MB |

Run benchmarks yourself:
```bash
./tools/benchmark --symbols BTC/USD,ETH/USD --duration 60
```

---

## 🛠 Installation

### Requirements

- **Platform:** Linux
- **Compiler:** GCC 9+ or Clang 10+ with C++17 support
- **Dependencies:** Boost >= 1.81, OpenSSL >= 1.1.1

### Build

```bash
# Install dependencies
sudo apt-get install -y build-essential cmake libssl-dev libboost-all-dev

# Clone and build
git clone https://github.com/your-org/kraken-sdk.git
cd kraken-sdk
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)

# Run examples
./examples/quickstart
./examples/strategies
./examples/dashboard
```

### CMake Integration

```cmake
find_package(KrakenSDK REQUIRED)
target_link_libraries(your_app PRIVATE kraken::kraken)
```

---

## 📚 Examples

### Order Book with Checksum

```cpp
#include <kraken/client.hpp>

int main() {
    kraken::KrakenClient client;
    
    client.on_book([](const std::string& symbol, const kraken::OrderBook& book) {
        if (!book.is_valid) {
            std::cerr << "⚠️ " << symbol << ": checksum failed!\n";
            return;
        }
        std::cout << symbol << " spread: $" << book.spread() << "\n";
    });
    
    client.subscribe_book({"BTC/USD"}, 10);
    client.run();
}
```

### Custom Strategy

```cpp
class TightSpreadAlert : public kraken::AlertStrategy {
public:
    bool check(const kraken::Ticker& t) override {
        return t.spread() < 5.0;  // Alert when spread < $5
    }
    
    std::string name() const override { return "TightSpread"; }
    std::vector<std::string> symbols() const override { return {"BTC/USD"}; }
};

int main() {
    kraken::KrakenClient client;
    
    auto custom = std::make_shared<TightSpreadAlert>();
    client.add_alert(custom, [](const auto& a) {
        std::cout << "💰 Tight spread on " << a.symbol << "!\n";
    });
    
    client.subscribe(kraken::Channel::Ticker, {"BTC/USD"});
    client.run();
}
```

### Connection State

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
    }
});
```

---

## 🔧 Configuration

```cpp
auto config = kraken::ClientConfig::Builder()
    .url("wss://ws.kraken.com/v2")
    .queue_capacity(131072)
    .reconnect_attempts(10)
    .validate_checksums(true)
    .build();

kraken::KrakenClient client(config);
```

---

## 📖 API Reference

### KrakenClient

```cpp
class KrakenClient {
    // Constructors
    KrakenClient();
    explicit KrakenClient(ClientConfig config);
    
    // Callbacks
    void on_ticker(TickerCallback callback);
    void on_trade(TradeCallback callback);
    void on_book(BookCallback callback);
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
    
    // Event loop
    void run();
    void run_async();
    void stop();
    
    // Metrics
    Metrics get_metrics() const;
};
```

### Data Types

```cpp
struct Ticker {
    std::string symbol;
    double bid, ask, last;
    double volume_24h, high_24h, low_24h;
    double spread() const;
    double mid_price() const;
};

struct OrderBook {
    std::string symbol;
    std::vector<PriceLevel> bids, asks;
    bool is_valid;  // CRC32 checksum passed
    double spread() const;
};

struct Alert {
    std::string strategy_name;
    std::string symbol;
    double price;
    std::string message;
};

enum class Channel { Ticker, Trade, Book, OHLC };
```

---

## 🧵 Thread Safety

| Operation | Thread Safety |
|-----------|---------------|
| `on_ticker()`, `on_error()`, etc. | ✅ Thread-safe |
| `subscribe()` | ✅ Thread-safe |
| `add_alert()` | ✅ Thread-safe |
| `is_connected()` | ✅ Thread-safe |
| `get_metrics()` | ✅ Thread-safe |
| `run()`, `stop()` | ⚠️ Single thread |

---

## 📁 Project Structure

```
kraken-sdk/
├── include/kraken/
│   ├── client.hpp       # Public API
│   ├── types.hpp        # Data types
│   └── strategies.hpp   # Alert strategies
├── src/
│   ├── client_impl.cpp
│   ├── connection.cpp
│   ├── parser.cpp
│   ├── book_engine.cpp
│   └── strategy_engine.cpp
├── examples/
│   ├── quickstart.cpp   # 5-line demo
│   ├── strategies.cpp   # Alert strategies
│   ├── dashboard.cpp    # Performance dashboard
│   └── orderbook.cpp    # Order book example
└── tools/
    └── benchmark.cpp    # Performance benchmark
```

---

## 📄 License

MIT License

---

## 🙏 Acknowledgements

- [rigtorp/SPSCQueue](https://github.com/rigtorp/SPSCQueue) - Lock-free queue
- [RapidJSON](https://github.com/Tencent/rapidjson) - JSON parsing
- [Boost.Beast](https://github.com/boostorg/beast) - WebSocket client

---

**Built for the Kraken Forge Hackathon 2025** 🚀
