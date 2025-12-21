# Benchmark Suite

This SDK includes comprehensive benchmarking using [Google Benchmark](https://github.com/google/benchmark).

---

## Quick Run

```bash
# Build in Release mode (required for accurate benchmarks)
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DKRAKEN_BUILD_TOOLS=ON
make -j$(nproc)

# Run all benchmarks
./bench_parser      # JSON parsing
./bench_queue       # Lock-free queue
./bench_orderbook   # Order book operations
./bench_checksum    # CRC32 checksum
./benchmark_integration 30  # Live API test (30 seconds)
```

---

## Benchmark Suites

### 1. Parser Benchmarks (`bench_parser`)

Tests JSON parsing performance with realistic Kraken message payloads.

| Benchmark | What it measures |
|-----------|------------------|
| `BM_ParseTicker` | Parse ticker JSON to `Ticker` struct |
| `BM_ParseTrade` | Parse trade JSON to `Trade` struct |
| `BM_ParseBook` | Parse order book JSON to `OrderBook` |
| `BM_BuildSubscribeMessage` | Build subscription request JSON |
| `BM_BuildUnsubscribeMessage` | Build unsubscribe request JSON |

**Expected Results:** 1-3 μs per parse, 100+ MB/s throughput

---

### 2. Queue Benchmarks (`bench_queue`)

Tests the lock-free SPSC queue (rigtorp/SPSCQueue).

| Benchmark | What it measures |
|-----------|------------------|
| `BM_QueuePush` | Time to push a message |
| `BM_QueuePop` | Time to pop a message |

**Expected Results:** 10-15 ns per operation, 70-90 million ops/sec

---

### 3. Order Book Benchmarks (`bench_orderbook`)

Tests order book management with realistic workloads.

| Benchmark | What it measures |
|-----------|------------------|
| `BM_BookEngineApplySnapshot/N` | Apply N-level snapshot |
| `BM_BookEngineApplyUpdate/N` | Apply N incremental updates |
| `BM_BookEngineGet/N` | Get order book with N levels |

**Expected Results:** 
- Single update: ~50 ns
- 100 updates: ~2 μs
- O(log n) complexity verified

---

### 4. Checksum Benchmarks (`bench_checksum`)

Tests CRC32 checksum calculation for order book validation.

| Benchmark | What it measures |
|-----------|------------------|
| `BM_CalculateChecksum/N` | Calculate checksum for N-level book |
| `BM_CalculateChecksumLargeBook` | Checksum for 100+ level book |

**Expected Results:** ~24 μs for standard 10-level book

---

### 5. Integration Benchmark (`benchmark_integration`)

Tests the complete SDK with live Kraken API.

```bash
./benchmark_integration 60  # Run for 60 seconds
```

**Measures:**
- End-to-end latency (WebSocket → callback)
- Messages per second
- Queue depth under load
- Message drop rate

**Expected Results:**
- Latency: < 1 ms (typically 200-500 μs)
- Drop rate: 0%
- Queue depth: 0-2 (no backpressure)

---

## Interpreting Results

### Time Units

| Unit | Meaning |
|------|---------|
| ns | Nanoseconds (10⁻⁹ seconds) |
| μs | Microseconds (10⁻⁶ seconds) |
| ms | Milliseconds (10⁻³ seconds) |

### Throughput Metrics

| Metric | Meaning |
|--------|---------|
| items_per_second | Operations per second |
| bytes_per_second | Data throughput |
| M/s | Million per second |
| k/s | Thousand per second |

---

## Optimization Tips

### For Accurate Benchmarks

1. **Use Release build** - Debug builds are 10-100x slower
2. **Close other applications** - Minimize CPU contention
3. **Run multiple times** - Check for consistency
4. **Use `taskset`** - Pin to single CPU for reproducibility

```bash
# Pin benchmark to CPU 0
taskset -c 0 ./bench_parser
```

### Environment Variables

```bash
# Disable CPU frequency scaling (requires root)
echo performance | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor

# Run with higher priority
sudo nice -n -20 ./bench_parser
```

---

## Latest Results

See [BENCHMARK_RESULTS.md](BENCHMARK_RESULTS.md) for detailed benchmark output.

---

## Adding New Benchmarks

```cpp
#include <benchmark/benchmark.h>
#include "your_component.hpp"

static void BM_YourOperation(benchmark::State& state) {
    // Setup (outside timing loop)
    YourComponent component;
    
    for (auto _ : state) {
        // Timed code
        component.do_operation();
    }
    
    // Report custom metrics
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK(BM_YourOperation);
```
