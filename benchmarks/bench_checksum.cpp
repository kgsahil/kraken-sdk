/// @file bench_checksum.cpp
/// @brief Microbenchmark for CRC32 checksum calculation

#include <benchmark/benchmark.h>
#include "book_engine.hpp"
#include <vector>
#include <random>

using namespace kraken;

static OrderBook generate_order_book(size_t bid_count, size_t ask_count) {
    OrderBook book;
    book.symbol = "BTC/USD";
    book.bids.reserve(bid_count);
    book.asks.reserve(ask_count);
    
    std::mt19937 gen(42);
    std::uniform_real_distribution<double> price_dist(50000.0, 51000.0);
    std::uniform_real_distribution<double> qty_dist(0.1, 10.0);
    
    for (size_t i = 0; i < bid_count; ++i) {
        book.bids.push_back({
            price_dist(gen),
            qty_dist(gen)
        });
    }
    
    for (size_t i = 0; i < ask_count; ++i) {
        book.asks.push_back({
            price_dist(gen),
            qty_dist(gen)
        });
    }
    
    return book;
}

static void BM_CalculateChecksum(benchmark::State& state) {
    auto book = generate_order_book(state.range(0), state.range(0));
    
    for (auto _ : state) {
        uint32_t checksum = BookEngine::calculate_checksum(book);
        benchmark::DoNotOptimize(checksum);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_CalculateChecksum)->Range(10, 100);

static void BM_CalculateChecksumLargeBook(benchmark::State& state) {
    auto book = generate_order_book(1000, 1000);
    
    for (auto _ : state) {
        uint32_t checksum = BookEngine::calculate_checksum(book);
        benchmark::DoNotOptimize(checksum);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_CalculateChecksumLargeBook);

BENCHMARK_MAIN();

