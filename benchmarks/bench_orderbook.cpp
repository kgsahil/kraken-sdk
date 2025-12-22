/// @file bench_orderbook.cpp
/// @brief Microbenchmark for order book operations

#include <benchmark/benchmark.h>
#include "internal/book_engine.hpp"
#include <vector>
#include <random>

using namespace kraken;

static std::vector<PriceLevel> generate_levels(size_t count, double base_price) {
    std::vector<PriceLevel> levels;
    levels.reserve(count);
    
    std::mt19937 gen(42);
    std::uniform_real_distribution<double> price_dist(0.1, 1.0);
    std::uniform_real_distribution<double> qty_dist(0.1, 10.0);
    
    for (size_t i = 0; i < count; ++i) {
        levels.push_back({
            base_price + price_dist(gen),
            qty_dist(gen)
        });
    }
    
    return levels;
}

static void BM_BookEngineApplySnapshot(benchmark::State& state) {
    BookEngine engine;
    auto bids = generate_levels(state.range(0), 50000.0);
    auto asks = generate_levels(state.range(0), 50001.0);
    
    for (auto _ : state) {
        engine.apply("BTC/USD", bids, asks, true, 0);
    }
    state.SetItemsProcessed(state.iterations() * state.range(0) * 2);
}
BENCHMARK(BM_BookEngineApplySnapshot)->Range(10, 1000);

static void BM_BookEngineApplyUpdate(benchmark::State& state) {
    BookEngine engine;
    
    // Initial snapshot
    auto bids = generate_levels(100, 50000.0);
    auto asks = generate_levels(100, 50001.0);
    engine.apply("BTC/USD", bids, asks, true, 0);
    
    // Updates
    auto update_bids = generate_levels(state.range(0), 50000.0);
    auto update_asks = generate_levels(state.range(0), 50001.0);
    
    for (auto _ : state) {
        engine.apply("BTC/USD", update_bids, update_asks, false, 0);
    }
    state.SetItemsProcessed(state.iterations() * state.range(0) * 2);
}
BENCHMARK(BM_BookEngineApplyUpdate)->Range(1, 100);

static void BM_BookEngineGet(benchmark::State& state) {
    BookEngine engine;
    
    // Setup book with many levels
    auto bids = generate_levels(state.range(0), 50000.0);
    auto asks = generate_levels(state.range(0), 50001.0);
    engine.apply("BTC/USD", bids, asks, true, 0);
    
    for (auto _ : state) {
        const OrderBook* book = engine.get("BTC/USD");
        benchmark::DoNotOptimize(book);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_BookEngineGet)->Range(10, 1000);

BENCHMARK_MAIN();

