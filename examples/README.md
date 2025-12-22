# Examples

This directory contains examples demonstrating how to use the Kraken WebSocket SDK.

## Usage

### Basic Usage

```bash
./quickstart
./dashboard
./strategies
```

### With Configuration File

All examples support loading configuration from a file:

```bash
./dashboard --config=path/to/config.cfg
./quickstart --config=./myconfig.cfg
```

The config file uses `KEY=VALUE` format (one per line). See `config.cfg.example` for a template.

**Config File Format:**
```
KRAKEN_WS_URL=wss://ws.kraken.com/v2
KRAKEN_API_KEY=your-key
KRAKEN_API_SECRET=your-secret
ENABLE_SPSC_QUEUE=true
SPSC_QUEUE_SIZE=131072
```

Lines starting with `#` are treated as comments. Empty lines are ignored.

See [README_CONFIG.md](README_CONFIG.md) for detailed configuration file documentation.

---

## Quick Start Examples

### `quickstart.cpp`
**Purpose:** Minimal example showing basic SDK usage  
**Use Case:** Learning the SDK API  
**Shows:** Basic ticker subscription and callback handling

```bash
./quickstart
```

### `strategies.cpp`
**Purpose:** Demonstrates the trading strategy engine  
**Use Case:** Understanding alert strategies  
**Shows:** Price alerts, volume spikes, custom strategies

```bash
./strategies
```

## Demo Examples (Feature Showcases)

### `dashboard.cpp`
**Purpose:** Terminal UI showing real-time metrics and market data  
**Use Case:** Demonstrating SDK capabilities  
**Shows:** Live performance dashboard, telemetry integration

```bash
./dashboard
# Prometheus metrics: http://localhost:9090/metrics
```

### `orderbook.cpp`
**Purpose:** Terminal UI showing order book reconstruction  
**Use Case:** Demonstrating order book features  
**Shows:** Real-time order book updates, checksum validation

```bash
./orderbook
```

### `telemetry.cpp`
**Purpose:** Demonstrates OpenTelemetry integration  
**Use Case:** Understanding metrics collection  
**Shows:** Telemetry configuration, Prometheus export

```bash
./telemetry
# Prometheus metrics: http://localhost:9090/metrics
```

## Real-World Examples (Practical Use Cases)

### `data_collector.cpp`
**Purpose:** Collect and store market data to file  
**Use Case:** Data collection, historical analysis, backtesting  
**Shows:** File I/O, data persistence, CSV export

```bash
./data_collector market_data.csv
```

**Real-world extensions:**
- Store to database (PostgreSQL, InfluxDB, TimescaleDB)
- Store to time-series database
- Store to cloud storage (S3, GCS)
- Data compression and batching

### `trading_bot.cpp`
**Purpose:** Simple trading bot with decision logic  
**Use Case:** Automated trading, algorithmic trading  
**Shows:** Moving averages, trading signals, position tracking

```bash
./trading_bot
```

**Real-world extensions:**
- Execute trades via Kraken REST API
- Risk management
- Portfolio management
- Multiple strategies
- Backtesting integration

### `web_backend.cpp`
**Purpose:** Send market data to web backend API  
**Use Case:** Web dashboards, mobile apps, microservices  
**Shows:** HTTP integration, message queuing, batching

```bash
./web_backend http://localhost:8080
```

**Real-world extensions:**
- WebSocket server integration
- REST API integration
- Message queue (RabbitMQ, Kafka)
- GraphQL subscriptions
- gRPC streaming

## Example Comparison

| Example | Type | Use Case | Complexity |
|---------|------|----------|------------|
| `quickstart` | Learning | Basic usage | ⭐ |
| `strategies` | Learning | Alert system | ⭐⭐ |
| `dashboard` | Demo | Feature showcase | ⭐⭐⭐ |
| `orderbook` | Demo | Feature showcase | ⭐⭐ |
| `telemetry` | Demo | Feature showcase | ⭐⭐⭐ |
| `data_collector` | **Practical** | Data collection | ⭐⭐ |
| `trading_bot` | **Practical** | Trading automation | ⭐⭐⭐ |
| `web_backend` | **Practical** | Web integration | ⭐⭐⭐ |

## Building Examples

```bash
cd build
cmake .. -DKRAKEN_BUILD_EXAMPLES=ON
make
```

## Common Patterns

### Pattern 1: Data Collection
```cpp
client.on_ticker([](const Ticker& t) {
    // Store to database/file/queue
    database.insert(t);
});
```

### Pattern 2: Trading Logic
```cpp
client.on_ticker([](const Ticker& t) {
    // Analyze and make decisions
    if (should_buy(t)) {
        execute_buy_order(t.symbol);
    }
});
```

### Pattern 3: Web Integration
```cpp
client.on_ticker([](const Ticker& t) {
    // Send to web backend
    http_client.post("/api/tickers", t.to_json());
});
```

### Pattern 4: Real-time Processing
```cpp
client.on_book([](const std::string& symbol, const OrderBook& book) {
    // Calculate indicators
    double imbalance = book.imbalance(10);
    if (imbalance > threshold) {
        // Trigger action
    }
});
```

## Next Steps

1. **For data collection:** Extend `data_collector.cpp` to use your database
2. **For trading:** Extend `trading_bot.cpp` with Kraken REST API integration
3. **For web apps:** Extend `web_backend.cpp` with your HTTP/WebSocket server
4. **For analysis:** Combine multiple examples for comprehensive solutions

