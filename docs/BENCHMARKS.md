# Performance Benchmarks

## Overview

The SDK includes two types of benchmarks:

1. **Integration Benchmark** (`tools/benchmark.cpp`) - End-to-end performance with live Kraken API
2. **Microbenchmarks** (Google Benchmark) - Component-level performance metrics

---

## Integration Benchmark

**Purpose**: Measure real-world performance with live Kraken WebSocket API.

**Usage**:
```bash
cd build
./benchmark [duration_seconds]
```

**What it measures**:
- Total messages processed
- Messages per second
- Messages dropped
- Max latency
- Queue depth
- Channel breakdown (ticker/trade/book)

**Example Output**:
```
╔═══════════════════════════════════════════════════════════════╗
║                       BENCHMARK RESULTS                       ║
╠═══════════════════════════════════════════════════════════════╣
║ Duration:                 30.0 seconds                        ║
║ Total Messages:             450                                ║
║ Throughput:                 15.0 msg/sec                       ║
║ Messages Dropped:             0                                ║
║ Max Latency:                371 µs                             ║
╚═══════════════════════════════════════════════════════════════╝
```

---

## Microbenchmarks (Google Benchmark)

**Purpose**: Measure individual component performance in isolation.

### Available Benchmarks

#### 1. `bench_parser` - JSON Parsing Performance

Measures:
- Ticker message parsing
- Trade message parsing
- Book message parsing
- Subscribe/unsubscribe message building

**Usage**:
```bash
cd build
./bench_parser
```

**Metrics**:
- Time per operation (nanoseconds)
- Throughput (operations/second)
- Bytes processed

#### 2. `bench_queue` - SPSC Queue Performance

Measures:
- Queue push performance
- Queue pop performance
- Queue throughput (producer/consumer)

**Usage**:
```bash
cd build
./bench_queue
```

**Metrics**:
- Latency per operation
- Throughput (messages/second)
- Scalability with different queue sizes

#### 3. `bench_orderbook` - Order Book Operations

Measures:
- Snapshot application
- Incremental update application
- Book retrieval

**Usage**:
```bash
cd build
./bench_orderbook
```

**Metrics**:
- Time per operation
- Scalability with book size
- O(log n) update performance

#### 4. `bench_checksum` - CRC32 Checksum Calculation

Measures:
- Checksum calculation time
- Scalability with book size

**Usage**:
```bash
cd build
./bench_checksum
```

**Metrics**:
- Checksum calculation latency
- Performance with large order books

---

## Running All Benchmarks

```bash
cd build

# Integration benchmark (requires live API)
./benchmark 30

# Microbenchmarks (no API needed)
./bench_parser
./bench_queue
./bench_orderbook
./bench_checksum
```

---

## Benchmark Results (Example)

### JSON Parsing
```
Benchmark                    Time             CPU   Iterations
---------------------------------------------------------------
BM_ParseTicker            1234 ns         1234 ns       567890
BM_ParseTrade             1456 ns         1456 ns       456789
BM_ParseBook              2345 ns         2345 ns       234567
BM_BuildSubscribeMessage    567 ns          567 ns      1234567
```

### Queue Performance
```
Benchmark                    Time             CPU   Iterations
---------------------------------------------------------------
BM_QueuePush/64             456 ns          456 ns      1234567
BM_QueuePush/1024           512 ns          512 ns      1098765
BM_QueuePush/65536          678 ns          678 ns       987654
BM_QueuePop/64              234 ns          234 ns      2345678
BM_QueueThroughput        12345 ns        12345 ns        56789
```

### Order Book Operations
```
Benchmark                    Time             CPU   Iterations
---------------------------------------------------------------
BM_BookEngineApplySnapshot/10    1234 ns         1234 ns       567890
BM_BookEngineApplySnapshot/100   5678 ns         5678 ns       123456
BM_BookEngineApplyUpdate/1       234 ns          234 ns      3456789
BM_BookEngineApplyUpdate/10     1234 ns         1234 ns       567890
BM_BookEngineGet/10              123 ns          123 ns      5678901
```

---

## Performance Targets (Verified Results)

| Component | Target | Measured | Status |
|-----------|--------|----------|--------|
| JSON Parsing | < 2 µs | **1.0-1.75 µs** | ✅ |
| Queue Push | < 1 µs | **11-13 ns** | 🚀 100x faster |
| Queue Pop | < 0.5 µs | **11-17 ns** | 🚀 30x faster |
| Book Update (10 levels) | < 5 µs | **671 ns** | ✅ |
| Book Update (100 levels) | < 5 µs | **1.9 µs** | ✅ |
| Checksum (100 levels) | < 10 µs | **15 µs** | ⚠️ Slightly over |

*See [BENCHMARK_RESULTS.md](BENCHMARK_RESULTS.md) for complete verified results.*

---

## Benchmarking Best Practices

1. **Run in Release Mode**: Always use `-DCMAKE_BUILD_TYPE=Release`
2. **Warm Up**: First few iterations may be slower due to cache warming
3. **Multiple Runs**: Run benchmarks multiple times for consistency
4. **System Load**: Run on idle system for accurate results
5. **CPU Affinity**: Pin to specific CPU cores for consistent results

---

## Continuous Benchmarking

For CI/CD integration:

```bash
# Run all microbenchmarks and save results
cd build
./bench_parser --benchmark_format=json > parser_results.json
./bench_queue --benchmark_format=json > queue_results.json
./bench_orderbook --benchmark_format=json > orderbook_results.json
./bench_checksum --benchmark_format=json > checksum_results.json
```

---

## Comparing Results

To compare before/after changes:

```bash
# Baseline
./bench_parser --benchmark_out=baseline.json

# After changes
./bench_parser --benchmark_out=current.json

# Compare (requires benchmark tools)
benchmark/tools/compare.py baseline.json current.json
```

---

## Notes

- **Integration benchmark** requires live Kraken API connection
- **Microbenchmarks** run in isolation (no network required)
- Results may vary based on:
  - CPU architecture
  - Compiler optimizations
  - System load
  - Cache state

