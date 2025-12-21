# Kraken Forge Hackathon 2025

**Category:** SDK Client  
**Language:** C++17  
**Repository:** https://github.com/kgsahil/kraken-sdk

---

## Overview

A production-grade C++ SDK for streaming real-time market data from Kraken Exchange. Designed for reliability, performance, and extensibility.

---

## Key Features

### Trading Strategy Engine
Built-in alert system for monitoring market conditions:
- `PriceAlert` - Threshold-based price alerts
- `VolumeSpike` - Unusual volume detection
- `SpreadAlert` - Spread monitoring
- Extensible `AlertStrategy` base class

### Lock-Free Architecture
- SPSC queue (rigtorp/SPSCQueue) for zero-contention message passing
- Atomic metrics updates without locks
- Two-thread reactor pattern (I/O + Dispatcher)

### Data Integrity
- CRC32 checksum validation for order books
- Message sequence tracking with gap detection
- Automatic reconnection with exponential backoff

### Production Ready
- PIMPL pattern for ABI stability
- Builder pattern for configuration
- Comprehensive error handling
- 17 test suites (unit, integration, thread safety)

---

## Performance

| Operation | Latency |
|-----------|---------|
| JSON Parsing | 1.8 - 3.1 μs |
| Queue Operations | 11 - 13 ns |
| Order Book Update | 51 ns |

Benchmarked with Google Benchmark in Release mode.

---

## Architecture

```
I/O Thread → Lock-Free Queue → Dispatcher Thread → Callbacks
                                      ↓
                              Strategy Engine → Alerts
```

**Design Patterns:** PIMPL, Builder, Strategy, Reactor, RAII

---

## Quick Demo

```bash
# Build
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)

# Run
./quickstart   # Ticker streaming
./strategies   # Alert strategies
./dashboard    # Live performance UI
```

---

## Technical Stack

| Component | Library |
|-----------|---------|
| WebSocket | Boost.Beast |
| JSON | RapidJSON |
| Queue | rigtorp/SPSCQueue |
| Testing | GoogleTest |
| Benchmarking | Google Benchmark |

---

## Documentation

- [README.md](../README.md) - Full documentation
- [BUILDING.md](../BUILDING.md) - Build instructions
- [BENCHMARKS.md](BENCHMARKS.md) - Performance details
