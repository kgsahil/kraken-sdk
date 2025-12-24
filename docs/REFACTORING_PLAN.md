# Code Architecture

## Overview

The Kraken SDK uses a modular architecture with clear separation of concerns. The codebase has been organized into focused modules for better maintainability, faster compilation, and easier navigation.

---

## Module Structure

### Core Module (`include/kraken/core/`)

Core SDK functionality:
- `client.hpp` - Main KrakenClient interface
- `config.hpp` - ClientConfig and Builder
- `types.hpp` - Data types and callbacks
- `error.hpp` - Error handling

### Strategies Module (`include/kraken/strategies/`)

Trading strategy engine with modular structure:
- `base.hpp` - Base classes and Alert struct
- `price_alert.hpp` - PriceAlert strategy
- `volume_spike.hpp` - VolumeSpike strategy
- `spread_alert.hpp` - SpreadAlert strategy
- `composite.hpp` - CompositeStrategy (AND/OR logic)
- `presets.hpp` - StrategyPresets factory
- `strategy_config.hpp` - Configuration support
- `strategies.hpp` - Main include file

### Telemetry Module (`include/kraken/telemetry/`)

Observability and monitoring:
- `telemetry.hpp` - Main Telemetry class
- `config.hpp` - TelemetryConfig
- `metrics_collector.hpp` - MetricsCollector
- `prometheus_server.hpp` - PrometheusHttpServer
- `otlp_exporter.hpp` - OtlpHttpExporter

### Connection Module (`include/kraken/connection/`)

Connection management:
- `backoff.hpp` - Reconnection strategies
- `gap_detector.hpp` - Gap detection
- `connection_config.hpp` - Connection configuration

### Client Implementation (`src/client/`)

Client implementation split into focused modules:
- `lifecycle.cpp` - Construction, connection, event loop
- `callbacks.cpp` - Callback registration
- `subscriptions.cpp` - Subscription management
- `strategies.cpp` - Strategy management
- `dispatch.cpp` - Message dispatch and I/O loops
- `reconnect.cpp` - Reconnection logic
- `snapshots.cpp` - Data snapshots
- `metrics.cpp` - Metrics collection

---

## Benefits

### For Users
- **Easier to understand** - Smaller, focused files
- **Faster compilation** - Include only what you need
- **Better navigation** - Clear file structure
- **Backward compatible** - Existing code still works

### For Maintainers
- **Easier to modify** - Changes isolated to specific files
- **Better testing** - Test individual components
- **Clearer responsibilities** - Each file has one job
- **Scalable** - Easy to add new strategies/features

---

## Usage

### Include All Strategies

```cpp
#include <kraken/strategies/strategies.hpp>  // Includes all strategies
```

### Include Specific Strategy

```cpp
#include <kraken/strategies/price_alert.hpp>  // Only PriceAlert
```

### Include Core SDK

```cpp
#include <kraken/kraken.hpp>  // Main include - includes everything
```

---

## Project Structure

```
include/kraken/
├── core/                 # Core SDK functionality
│   ├── client.hpp
│   ├── config.hpp
│   ├── types.hpp
│   └── error.hpp
├── strategies/           # Strategy engine (modular)
│   ├── base.hpp
│   ├── price_alert.hpp
│   ├── volume_spike.hpp
│   ├── spread_alert.hpp
│   ├── composite.hpp
│   ├── presets.hpp
│   ├── strategy_config.hpp
│   └── strategies.hpp
├── telemetry/           # Telemetry (modular)
│   ├── telemetry.hpp
│   ├── config.hpp
│   ├── metrics_collector.hpp
│   ├── prometheus_server.hpp
│   └── otlp_exporter.hpp
├── connection/          # Connection management
│   ├── backoff.hpp
│   ├── gap_detector.hpp
│   └── connection_config.hpp
└── kraken.hpp           # Main include

src/
├── core/                # Core implementation
├── strategies/          # Strategy implementations
├── telemetry/           # Telemetry implementations
├── connection/          # Connection implementations
├── client/              # Client module implementations
└── internal/            # Private implementation details
```
