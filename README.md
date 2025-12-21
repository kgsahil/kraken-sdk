# Kraken WebSocket SDK

A production-grade C++ SDK for real-time market data streaming from Kraken Exchange.

```cpp
#include <kraken/kraken.hpp>

int main() {
    kraken::KrakenClient client;
    
    client.on_ticker([](const kraken::Ticker& t) {
        std::cout << t.symbol << ": $" << t.last << std::endl;
    });
    
    client.subscribe(kraken::Channel::Ticker, {"BTC/USD", "ETH/USD"});
    client.run();
}
```

---

## Features

### Real-Time Data Streaming
Subscribe to tickers, trades, order books, and OHLC candles with automatic reconnection and resubscription.

### Trading Strategy Engine
Built-in alert system that monitors market conditions and triggers callbacks:

```cpp
// Price threshold alert
auto alert = kraken::PriceAlert::Builder()
    .symbol("BTC/USD")
    .above(100000.0)
    .build();

client.add_alert(alert, [](const kraken::Alert& a) {
    std::cout << "Alert: " << a.message << std::endl;
});
```

Available strategies: `PriceAlert`, `VolumeSpike`, `SpreadAlert`, and extensible `AlertStrategy` base class.

### Order Book with Checksum Validation
Reconstructs order books from incremental updates with CRC32 checksum verification:

```cpp
client.on_book([](const std::string& symbol, const kraken::OrderBook& book) {
    if (!book.is_valid) {
        std::cerr << "Checksum mismatch - data corrupted" << std::endl;
        return;
    }
    
    double imbalance = book.imbalance(10);  // Bid/ask ratio
    std::cout << "Spread: $" << book.spread() << std::endl;
});
```

### Lock-Free Message Queue
Uses [rigtorp/SPSCQueue](https://github.com/rigtorp/SPSCQueue) for zero-contention message passing between I/O and dispatcher threads.

### Exponential Backoff
Production-ready reconnection with configurable strategy:

```cpp
auto config = kraken::ClientConfig::Builder()
    .backoff(kraken::ExponentialBackoff::Builder()
        .initial_delay(std::chrono::milliseconds(100))
        .max_delay(std::chrono::seconds(30))
        .max_attempts(20)
        .jitter_factor(0.2)
        .build())
    .build();
```

### Message Gap Detection
Tracks sequence numbers per channel/symbol to detect missed messages:

```cpp
auto config = kraken::ClientConfig::Builder()
    .enable_gap_detection(true)
    .on_gap([](const kraken::GapInfo& gap) {
        std::cerr << "Missed " << gap.gap_size << " messages" << std::endl;
    })
    .build();
```

### JSON Serialization
All data types serialize to JSON for web integration:

```cpp
client.on_ticker([](const kraken::Ticker& t) {
    websocket_server.broadcast(t.to_json());
});
```

### Performance Monitoring
Real-time metrics including message counts, latency, and queue depth:

```cpp
auto metrics = client.get_metrics();
std::cout << "Messages/sec: " << metrics.messages_per_second() << std::endl;
std::cout << "Max latency: " << metrics.latency_max_us.count() << " μs" << std::endl;
```

**Two metrics systems:**
- **Local API** (`get_metrics()`) - Immediate access for dashboards and debugging
- **OpenTelemetry** - Interface defined for future integration with Prometheus/Jaeger

See [docs/METRICS.md](docs/METRICS.md) for detailed metrics documentation.

---

## Architecture

```
┌────────────────────────────────────────────────────────────────────┐
│                         KRAKEN SDK                                  │
├────────────────────────────────────────────────────────────────────┤
│                                                                     │
│   ┌──────────────┐     ┌──────────────┐     ┌──────────────┐       │
│   │   WebSocket  │     │  Lock-Free   │     │  Dispatcher  │       │
│   │   I/O Thread │────▶│  SPSC Queue  │────▶│    Thread    │       │
│   │   (Producer) │     │  (88M ops/s) │     │  (Consumer)  │       │
│   └──────────────┘     └──────────────┘     └──────┬───────┘       │
│          │                                         │                │
│          │              ┌──────────────────────────┼────────┐      │
│          │              │                          ▼        │      │
│          │              │   ┌──────────────┐  ┌────────────┐│      │
│          ▼              │   │   Strategy   │  │    User    ││      │
│   ┌──────────────┐      │   │    Engine    │  │  Callbacks ││      │
│   │    Kraken    │      │   │ (Alerts)     │  │            ││      │
│   │   Exchange   │      │   └──────────────┘  └────────────┘│      │
│   └──────────────┘      │         Your Trading Logic        │      │
│                         └───────────────────────────────────┘      │
└────────────────────────────────────────────────────────────────────┘
```

### Design Decisions

| Pattern | Purpose |
|---------|---------|
| **PIMPL** | ABI stability, hide implementation dependencies |
| **Builder** | Fluent, self-documenting configuration |
| **Strategy** | Extensible alert system |
| **Two-Thread Reactor** | I/O never blocks callbacks |
| **RAII** | Automatic resource management |

### Implementation Details

| Component | Implementation |
|-----------|----------------|
| WebSocket | Boost.Beast with TLS |
| JSON Parsing | RapidJSON (zero-copy) |
| Message Queue | rigtorp/SPSCQueue (lock-free) |
| Order Book | `std::map` for O(log n) updates |
| Metrics | `std::atomic` for lock-free updates |
| Threading | `std::condition_variable` for efficient wake-up |

---

## Performance

Benchmarked with Google Benchmark (Release build):

| Operation | Time |
|-----------|------|
| JSON Parsing | 1.8 - 3.1 μs |
| Queue Push/Pop | 11 - 13 ns |
| Order Book Update | 51 ns (single level) |
| Checksum Calculation | 24 μs |

---

## Installation

### Requirements
- C++17 compiler (GCC 9+, Clang 10+)
- CMake 3.16+
- Boost 1.70+ (system component)
- OpenSSL 1.1.1+

### Build

```bash
# Ubuntu/WSL
sudo apt-get install -y build-essential cmake libssl-dev libboost-system-dev

git clone https://github.com/kgsahil/kraken-sdk.git
cd kraken-sdk
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

### Run Examples

```bash
./quickstart     # Basic ticker streaming
./strategies     # Alert strategies demo
./dashboard      # Live performance UI
./orderbook      # Order book with checksum
```

---

## API Overview

### KrakenClient

```cpp
class KrakenClient {
    // Callbacks
    void on_ticker(TickerCallback);
    void on_trade(TradeCallback);
    void on_book(BookCallback);
    void on_ohlc(OHLCCallback);
    void on_error(ErrorCallback);
    void on_connection_state(ConnectionStateCallback);
    
    // Subscriptions
    Subscription subscribe(Channel, std::vector<std::string> symbols);
    Subscription subscribe_book(std::vector<std::string> symbols, int depth = 10);
    
    // Strategies
    int add_alert(std::shared_ptr<AlertStrategy>, AlertCallback);
    void remove_alert(int id);
    
    // Snapshots
    std::optional<Ticker> latest_ticker(const std::string& symbol);
    std::optional<OrderBook> latest_book(const std::string& symbol);
    
    // Event Loop
    void run();       // Blocking
    void run_async(); // Non-blocking
    void stop();
    
    // Metrics
    Metrics get_metrics();
};
```

### Subscription Handle

```cpp
Subscription sub = client.subscribe(Channel::Ticker, {"BTC/USD"});

sub.pause();                      // Stop receiving updates
sub.resume();                     // Resume updates
sub.add_symbols({"ETH/USD"});     // Add symbols
sub.remove_symbols({"BTC/USD"});  // Remove symbols
sub.unsubscribe();                // Unsubscribe completely
```

### Data Types

```cpp
struct Ticker {
    std::string symbol;
    double bid, ask, last;
    double volume_24h, high_24h, low_24h;
    
    double spread();
    double mid_price();
    std::string to_json();
};

struct OrderBook {
    std::vector<PriceLevel> bids, asks;
    bool is_valid;  // CRC32 passed
    
    double spread();
    double imbalance(size_t depth);
    std::string to_json(size_t levels);
};
```

---

## Testing

```bash
cd build
ctest --output-on-failure
```

17 test suites covering unit, integration, thread safety, edge cases, and exception safety.

---

## Project Structure

```
kraken-sdk/
├── include/kraken/    # Public API
│   ├── kraken.hpp     # Main include
│   ├── client.hpp     # KrakenClient
│   ├── config.hpp     # ClientConfig
│   ├── types.hpp      # Data types
│   ├── strategies.hpp # Alert strategies
│   ├── backoff.hpp    # Reconnection strategies
│   └── gap_detector.hpp
├── src/               # Implementation (PIMPL)
├── examples/          # Demo applications
├── tests/             # GoogleTest suites
├── benchmarks/        # Google Benchmark suites
└── docs/              # Documentation
```

---

## License

MIT License - see [LICENSE](LICENSE)

---

## Acknowledgements

- [rigtorp/SPSCQueue](https://github.com/rigtorp/SPSCQueue) - Lock-free queue
- [RapidJSON](https://github.com/Tencent/rapidjson) - JSON parsing
- [Boost.Beast](https://github.com/boostorg/beast) - WebSocket client
