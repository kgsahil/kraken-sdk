# OpenTelemetry (OTEL) Integration

## Overview

The OpenTelemetry (OTEL) interface is fully integrated into the Kraken SDK client. All core metrics are automatically collected when telemetry is enabled via `ClientConfig`, and metrics can be exported via HTTP server (Prometheus scraping) or OTLP push.

---

## Features

### Telemetry Interface (`include/kraken/telemetry/telemetry.hpp`)

- **`TelemetryConfig`** - Configuration structure with builder pattern
  - Service name, version, environment
  - HTTP server configuration (port, enabled/disabled)
  - OTLP export configuration (endpoint, interval, retries)
  - Metrics/traces/logs enable flags

- **`MetricsCollector`** - Lock-free metrics collection with:
  - Atomic counters (messages, drops, reconnects, etc.)
  - Gauges (queue depth, connection state)
  - Latency tracking (max, average)
  - Per-channel message counts
  - Prometheus text format export (`to_prometheus()`)
  - JSON export (`to_json()`)

- **`Telemetry`** - Main interface class with:
  - Factory method `create()` for shared_ptr management
  - HTTP server lifecycle management (`start()`, `stop()`)
  - OTLP export lifecycle management
  - Direct metrics access

### HTTP Server for Prometheus (`src/telemetry/prometheus_server.cpp`)

- Built-in HTTP server using Boost.Beast for exposing Prometheus metrics
- **Endpoints:**
  - `GET /metrics` - Prometheus text format metrics
  - `GET /health` - Health check endpoint (returns JSON)
- Configurable port (default: 9090)
- Automatic start/stop with telemetry lifecycle
- Thread-safe, async request handling
- No external dependencies (uses Boost.Beast already in project)

### OTLP HTTP Exporter (`src/telemetry/otlp_exporter.cpp`)

- Background thread for periodic metric export
- Configurable export interval (default: 15 seconds)
- Retry logic support (configurable max retries)
- OTLP HTTP/JSON format support
- Can be extended to full OTLP protobuf or gRPC

### Client Integration

The `Telemetry` class is fully integrated into `KrakenClient`:

- `ClientConfig::Builder::telemetry()` method for configuration
- `KrakenClient::Impl` instantiates and starts `Telemetry` when configured
- `KrakenClient::Impl::stop()` stops telemetry services
- All metrics automatically collected:
  - Messages received/processed/dropped
  - Queue depth
  - Connection state
  - Latency (max, average)
  - Reconnect attempts
  - Checksum failures
  - Gap detection
  - Strategy alerts triggered
- `get_metrics()` reads from Telemetry when enabled
- `get_telemetry_instance()` exposes Telemetry for advanced use

---

## Usage Example

```cpp
#include <kraken/kraken.hpp>
#include <iostream>

int main() {
    // Configure telemetry with HTTP server for Prometheus scraping
    kraken::TelemetryConfig telemetry_config = kraken::TelemetryConfig::Builder()
        .service_name("my-trading-bot")
        .service_version("1.0.0")
        .environment("production")
        .metrics(true)
        .http_server(true, 9090)  // Enable HTTP server on port 9090
        .otlp_export(true)  // Enable OTLP export (pushes to collector)
        .otlp_endpoint("http://localhost:4318")  // OTLP HTTP endpoint
        .metrics_interval(std::chrono::seconds(15))  // Export every 15 seconds
        .build();

    // Configure client with telemetry
    kraken::ClientConfig client_config = kraken::ClientConfig::Builder()
        .telemetry(telemetry_config)
        .build();

    kraken::KrakenClient client(client_config);

    // Metrics are automatically collected and exported in the background!
    // - HTTP server exposes /metrics endpoint for Prometheus scraping
    // - OTLP exporter pushes metrics to collector every 15 seconds

    // You can still get a snapshot of current metrics
    kraken::Metrics current_metrics = client.get_metrics();
    std::cout << "Messages Processed: " << current_metrics.messages_processed << std::endl;

    // Check if HTTP server is running
    std::shared_ptr<kraken::Telemetry> telemetry = client.get_telemetry_instance();
    if (telemetry && telemetry->is_http_server_running()) {
        std::cout << "Prometheus metrics available at: http://localhost:" 
                  << telemetry->http_server_port() << "/metrics" << std::endl;
    }

    client.subscribe(kraken::Channel::Ticker, {"BTC/USD"});
    client.run();
    return 0;
}
```

---

## Prometheus Integration

Once the HTTP server is running, you can scrape metrics with Prometheus:

### Prometheus Configuration

```yaml
# prometheus.yml
scrape_configs:
  - job_name: 'kraken-sdk'
    scrape_interval: 15s
    static_configs:
      - targets: ['localhost:9090']
```

### Manual Query

```bash
# Get metrics in Prometheus format
curl http://localhost:9090/metrics

# Health check
curl http://localhost:9090/health
```

---

## Future Enhancements

### Full OTLP Protobuf Export (Optional)

- Integrate with the official OpenTelemetry C++ SDK or implement OTLP protobuf serialization
- Full OTLP (OpenTelemetry Protocol) exporter to send metrics, traces, and logs to an OpenTelemetry Collector via gRPC or HTTP
- This would enable seamless integration with various backends like Jaeger (tracing), Grafana (dashboards), and other APM tools

---

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    KrakenClient                              │
│  ┌──────────────────────────────────────────────────────┐  │
│  │              Telemetry (PIMPL)                        │  │
│  │  ┌──────────────────┐  ┌──────────────────────────┐  │  │
│  │  │ MetricsCollector │  │  HTTP Server (Boost.Beast)│  │  │
│  │  │  (Lock-free)     │  │  - GET /metrics           │  │  │
│  │  │                  │  │  - GET /health           │  │  │
│  │  └──────────────────┘  └──────────────────────────┘  │  │
│  │                              │                        │  │
│  │  ┌──────────────────────────┘                        │  │
│  │  │ OTLP HTTP Exporter                                │  │
│  │  │ - Background thread                                │  │
│  │  │ - Periodic export                                  │  │
│  │  │ - Retry logic                                      │  │
│  │  └──────────────────────────────────────────────────┘  │  │
│  └──────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
                    ┌─────────────────┐
                    │  Prometheus     │
                    │  (Scraping)     │
                    └─────────────────┘
                              │
                    ┌─────────────────┐
                    │  OTLP Collector │
                    │  (Push)         │
                    └─────────────────┘
```

---

## Conclusion

The OTEL integration is fully functional and production-ready:
- Metrics collection (lock-free, high-performance)
- HTTP server for Prometheus scraping
- OTLP export framework (ready for protobuf extension)
- Automatic lifecycle management
- Zero external dependencies (uses existing Boost.Beast)
