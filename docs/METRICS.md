# Metrics & Observability

The SDK provides two complementary metrics systems for different use cases.

---

## Metrics Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                    SDK Metrics Collection                      │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│   ┌─────────────────────────────────────┐                      │
│   │  Atomic Counters (Lock-Free)         │                      │
│   │  - messages_received                │                      │
│   │  - messages_processed                │                      │
│   │  - messages_dropped                  │                      │
│   │  - latency_max_us                    │                      │
│   └──────────────┬──────────────────────┘                      │
│                  │                                              │
│         ┌────────┴────────┐                                    │
│         │                 │                                    │
│         ▼                 ▼                                    │
│  ┌──────────────┐  ┌──────────────────┐                        │
│  │ Local Access │  │ OTEL Export      │                        │
│  │ get_metrics()│  │ (Future)         │                        │
│  └──────┬───────┘  └──────┬───────────┘                        │
│         │                 │                                    │
└─────────┼─────────────────┼────────────────────────────────────┘
          │                 │
          ▼                 ▼
┌─────────────────┐  ┌──────────────────────────────────────────┐
│  Local Use       │  │  External Infrastructure                 │
│  - Dashboard     │  │  - Prometheus                             │
│  - Debugging     │  │  - Jaeger                                 │
│  - Health checks │  │  - Grafana                                │
│  - JSON export   │  │  - OTLP collectors                        │
└─────────────────┘  └──────────────────────────────────────────┘
```

---

## 1. Local Metrics API (`get_metrics()`)

**Purpose:** Immediate, in-process access to SDK metrics for real-time visibility.

### Usage

```cpp
// Get current metrics snapshot
auto metrics = client.get_metrics();

// Access individual metrics
std::cout << "Messages/sec: " << metrics.messages_per_second() << std::endl;
std::cout << "Total processed: " << metrics.messages_processed << std::endl;
std::cout << "Dropped: " << metrics.messages_dropped << std::endl;
std::cout << "Max latency: " << metrics.latency_max_us.count() << " μs" << std::endl;
std::cout << "Queue depth: " << metrics.queue_depth << std::endl;
std::cout << "Uptime: " << metrics.uptime_string() << std::endl;

// Export as JSON
std::string json = metrics.to_json();
websocket_server.broadcast(json);
```

### Available Metrics

| Metric | Type | Description |
|--------|------|-------------|
| `messages_received` | Counter | Total messages received from WebSocket |
| `messages_processed` | Counter | Total messages processed by dispatcher |
| `messages_dropped` | Counter | Messages dropped due to queue overflow |
| `queue_depth` | Gauge | Current number of messages in queue |
| `connection_state` | Gauge | Current connection state |
| `latency_max_us` | Gauge | Maximum message processing latency |
| `messages_per_second()` | Calculated | Approximate throughput |
| `uptime()` | Calculated | SDK uptime duration |

### Characteristics

- ✅ **Synchronous** - Returns immediately
- ✅ **Zero overhead** - No network calls
- ✅ **Always available** - Works without external dependencies
- ✅ **Snapshot** - Returns current state at call time

### Use Cases

| Scenario | Why Use Local Metrics |
|----------|----------------------|
| **Terminal Dashboard** | Real-time display in same process |
| **Development Debugging** | Quick health checks during development |
| **Application Health Checks** | Monitor SDK state in your application |
| **JSON Export** | Send metrics to your own monitoring system |
| **Single Trading Bot** | Simple monitoring without infrastructure |

---

## 2. OpenTelemetry Integration (Planned)

**Purpose:** Export metrics to centralized monitoring infrastructure (Prometheus, Jaeger, Grafana).

### Current Status

⚠️ **Interface Defined, Not Yet Integrated**

The SDK includes a `Telemetry` interface (`include/kraken/telemetry.hpp`) with:
- `MetricsCollector` - Lock-free metrics collection
- `TelemetryConfig` - Configuration for OTLP endpoints
- `to_prometheus()` - Prometheus text format export
- Builder pattern for configuration

**However**, the `Telemetry` class is not yet integrated into `KrakenClient`. This is planned for future releases.

### Planned Usage

```cpp
// Configure telemetry
auto config = ClientConfig::Builder()
    .telemetry(TelemetryConfig::Builder()
        .otlp_endpoint("http://localhost:4317")
        .service_name("trading-bot")
        .metrics(true)
        .metrics_interval(std::chrono::seconds(15))
        .build())
    .build();

KrakenClient client(config);

// Metrics automatically exported to OTLP collector
// Can be scraped by Prometheus or sent to Jaeger
```

### Characteristics

- ⏳ **Asynchronous** - Background export (when implemented)
- ⏳ **Requires infrastructure** - Needs OTLP collector setup
- ⏳ **Historical data** - Metrics stored in time-series DB
- ⏳ **Cross-service** - Correlate with other services

### Use Cases

| Scenario | Why Use OTEL |
|----------|--------------|
| **Production Monitoring** | Centralized metrics across multiple bots |
| **Alerting** | PagerDuty/OpsGenie integration |
| **Distributed Tracing** | Track requests across services |
| **Long-term Storage** | Historical analysis and trending |
| **Multi-Service Architecture** | Correlate SDK metrics with trading logic |

---

## When to Use Each

| Scenario | Local Metrics | OTEL |
|----------|---------------|------|
| Development debugging | ✅ | ❌ |
| Terminal dashboard | ✅ | ❌ |
| Single trading bot | ✅ | Optional |
| Production monitoring | ✅ | ✅ |
| Multi-bot trading desk | ✅ | ✅ |
| Alerting (PagerDuty) | ❌ | ✅ |
| Distributed tracing | ❌ | ✅ |
| Historical analysis | ❌ | ✅ |

---

## Summary

**They're complementary, not competing:**

1. **Local Metrics (`get_metrics()`)** 
   - ✅ Available now
   - ✅ Zero setup, always works
   - ✅ Perfect for development and simple monitoring

2. **OTEL Export**
   - ⏳ Interface defined, integration pending
   - ⏳ Requires infrastructure setup
   - ⏳ Essential for production at scale

**Best Practice:** Use local metrics for immediate visibility, add OTEL when you need centralized monitoring infrastructure.

---

## Example: Dashboard Using Local Metrics

```cpp
#include <kraken/kraken.hpp>
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    kraken::KrakenClient client;
    client.subscribe(kraken::Channel::Ticker, {"BTC/USD"});
    client.run_async();
    
    // Update dashboard every second
    while (client.is_running()) {
        auto metrics = client.get_metrics();
        
        std::cout << "\033[2J\033[H";  // Clear screen
        std::cout << "╔═══════════════════════════════════╗\n";
        std::cout << "║     KRAKEN SDK METRICS            ║\n";
        std::cout << "╠═══════════════════════════════════╣\n";
        std::cout << "║ Messages/sec: " << metrics.messages_per_second() << "\n";
        std::cout << "║ Queue Depth:  " << metrics.queue_depth << "\n";
        std::cout << "║ Max Latency:   " << metrics.latency_max_us.count() << " μs\n";
        std::cout << "║ Uptime:        " << metrics.uptime_string() << "\n";
        std::cout << "╚═══════════════════════════════════╝\n";
        
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}
```

