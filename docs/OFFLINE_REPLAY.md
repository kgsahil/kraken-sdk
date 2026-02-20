# Offline Mode & Replay Engine

## Overview

The Kraken C++ SDK is designed to be highly modular. By default, it operates as a monolithic "P1" node: it handles its own secure WebSocket connections, manages I/O threads, and evaluates its own trading strategies. 

However, in advanced, distributed Infrastructure-as-Code (IaC) environments, you may want to separate market data ingestion from strategy execution. For example:
- **P1 (Ingestion Nodes):** Connect to Kraken WebSockets, parse JSON, and forward to a low-latency IPC bus (e.g., Aeron) or message broker (e.g., Redpanda).
- **P2 (Strategy Nodes):** Consume parsed C++ structs from Aeron and evaluate trading strategies without any external internet connection.
- **P3 (Analytics Nodes):** Replay historical data from Redpanda into the SDK's internal engines for deterministic backtesting.

The **Offline Mode** and **Replay Engine** APIs were built precisely for P2 and P3 topologies.

## Configuring Offline Mode

To prevent the SDK from automatically attempting to connect to kraken.com and spinning up background network thread loops, enable `offline_mode` in the configuration. 

For P2 and P3 nodes, it is also highly recommended to disable the internal SPSC queue (`use_queue(false)`). Because network threads are disabled, the queue overhead is unnecessary; data can simply be injected directly on the calling thread.

```cpp
#include <kraken/kraken.hpp>

auto config = kraken::ClientConfig::Builder()
    .offline_mode(true)
    .use_queue(false) // Disable thread hops for direct injection
    .build();

kraken::KrakenClient client(config);

// Run will initialize the internal state machine but return immediately
// since no network connections are being established.
client.run(); 
```

## Using the Replay Engine

Once the client is running in offline mode, you can retrieve the `ReplayEngine` interface. This interface provides direct access to the `dispatcher` loop, bypassing the network stack entirely.

Injecting data via `ReplayEngine` propagates it through the entire SDK ecosystem exactly as if it arrived over a WebSocket. This includes:
1. Triggering user-defined `.on_ticker()`, `.on_book()`, etc. callbacks.
2. Incrementing internal atomic metrics and OpenTelemetry/Prometheus telemetry.
3. Engaging the **Strategy Engine** to evaluate active `AlertStrategy` configurations.

### Injection Example

```cpp
// 1. Setup a strategy
auto price_alert = kraken::PriceAlert::Builder()
                       .symbol("BTC/USD")
                       .above(50250.0)
                       .build();

client.add_alert(price_alert, [](const kraken::Alert& event) {
    std::cout << "🚨 Strategy Triggered! Price passed $" << event.price << "\n";
});

auto& replay_engine = client.get_replay_engine();

// 2. Wait for data from external source (e.g., Aeron IPC polling)
kraken::Ticker historical_ticker;
historical_ticker.symbol = "BTC/USD";
historical_ticker.last = 51000.0;

// 3. Inject it!
replay_engine.inject_ticker(historical_ticker);
```

### Supported Injection Types
The `ReplayEngine` supports the core volume-heavy data types utilized by the Strategy Engine:
- `inject_ticker(const Ticker& ticker)`
- `inject_trade(const Trade& trade)`
- `inject_book(const OrderBook& book)`

## Example Project

See `examples/offline_replay.cpp` for a fully compilable, end-to-end demonstration of the offline mode and data injection APIs.
