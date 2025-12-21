# Benchmarks

This SDK includes microbenchmarks using [Google Benchmark](https://github.com/google/benchmark).

## Running Benchmarks

```bash
# Build in Release mode
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DKRAKEN_BUILD_TOOLS=ON
make -j$(nproc)

# Run benchmarks
./bench_parser      # JSON parsing
./bench_queue       # Lock-free queue
./bench_orderbook   # Order book operations
./bench_checksum    # CRC32 checksum
```

## Results

**Environment:** Ubuntu 22.04, GCC 11.4, 6-core CPU @ 2.37 GHz

### Parser (`bench_parser`)

| Benchmark | Time | Throughput |
|-----------|------|------------|
| Parse Ticker | 3.1 μs | 88 MB/s |
| Parse Trade | 2.0 μs | 95 MB/s |
| Parse Book | 1.8 μs | 118 MB/s |
| Build Subscribe | 218 ns | - |

### Queue (`bench_queue`)

| Benchmark | Time | Throughput |
|-----------|------|------------|
| Push | 11 ns | 88M ops/s |
| Pop | 13 ns | 75M ops/s |

### Order Book (`bench_orderbook`)

| Benchmark | Time | Throughput |
|-----------|------|------------|
| Apply Update (1 level) | 51 ns | 39M ops/s |
| Apply Update (8 levels) | 139 ns | 115M ops/s |
| Apply Update (100 levels) | 1.9 μs | 106M ops/s |
| Snapshot (10 levels) | 1.1 μs | 18.5M ops/s |

### Checksum (`bench_checksum`)

| Benchmark | Time |
|-----------|------|
| CRC32 (10 levels) | 24 μs |

## Integration Benchmark

Test with live Kraken API:

```bash
./benchmark_integration 30  # Run for 30 seconds
```

Reports end-to-end latency, message rate, and queue depth.
