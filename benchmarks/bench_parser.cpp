/// @file bench_parser.cpp
/// @brief Microbenchmark for JSON parsing performance

#include <benchmark/benchmark.h>
#include "parser.hpp"
#include <string>
#include <vector>

using namespace kraken;

// Sample JSON messages from Kraken
const std::string TICKER_JSON = R"({
    "channel": "ticker",
    "data": [{
        "symbol": "BTC/USD",
        "bid": "50000.5",
        "ask": "50001.0",
        "last": "50000.75",
        "volume": "1234.56",
        "high": "51000.0",
        "low": "49000.0",
        "timestamp": "2024-01-01T00:00:00Z"
    }]
})";

const std::string TRADE_JSON = R"({
    "channel": "trade",
    "data": [{
        "symbol": "BTC/USD",
        "price": "50000.5",
        "qty": "0.1",
        "side": "buy",
        "timestamp": "2024-01-01T00:00:00Z"
    }]
})";

const std::string BOOK_JSON = R"({
    "channel": "book",
    "data": [{
        "symbol": "BTC/USD",
        "bids": [["50000.0", "1.5"], ["49999.0", "2.0"]],
        "asks": [["50001.0", "1.2"], ["50002.0", "3.0"]],
        "checksum": 1234567890
    }]
})";

static void BM_ParseTicker(benchmark::State& state) {
    for (auto _ : state) {
        auto msg = parse_message(TICKER_JSON);
        benchmark::DoNotOptimize(msg);
    }
    state.SetBytesProcessed(state.iterations() * TICKER_JSON.size());
}
BENCHMARK(BM_ParseTicker);

static void BM_ParseTrade(benchmark::State& state) {
    for (auto _ : state) {
        auto msg = parse_message(TRADE_JSON);
        benchmark::DoNotOptimize(msg);
    }
    state.SetBytesProcessed(state.iterations() * TRADE_JSON.size());
}
BENCHMARK(BM_ParseTrade);

static void BM_ParseBook(benchmark::State& state) {
    for (auto _ : state) {
        auto msg = parse_message(BOOK_JSON);
        benchmark::DoNotOptimize(msg);
    }
    state.SetBytesProcessed(state.iterations() * BOOK_JSON.size());
}
BENCHMARK(BM_ParseBook);

static void BM_BuildSubscribeMessage(benchmark::State& state) {
    std::vector<std::string> symbols = {"BTC/USD", "ETH/USD", "SOL/USD"};
    for (auto _ : state) {
        auto msg = build_subscribe_message(Channel::Ticker, symbols, 0);
        benchmark::DoNotOptimize(msg);
    }
}
BENCHMARK(BM_BuildSubscribeMessage);

static void BM_BuildUnsubscribeMessage(benchmark::State& state) {
    std::vector<std::string> symbols = {"BTC/USD", "ETH/USD", "SOL/USD"};
    for (auto _ : state) {
        auto msg = build_unsubscribe_message(Channel::Ticker, symbols);
        benchmark::DoNotOptimize(msg);
    }
}
BENCHMARK(BM_BuildUnsubscribeMessage);

BENCHMARK_MAIN();

