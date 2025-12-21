/// @file bench_queue.cpp
/// @brief Microbenchmark for SPSC queue performance

#include <benchmark/benchmark.h>
#include <rigtorp/SPSCQueue.h>
#include <thread>
#include <atomic>
#include <vector>

struct Message {
    int type;
    double data[4];
    char symbol[16];  // Fixed-size to avoid std::string issues
};

static void BM_QueuePush(benchmark::State& state) {
    rigtorp::SPSCQueue<Message> queue(state.range(0));
    Message msg{1, {1.0, 2.0, 3.0, 4.0}, "BTC/USD"};
    
    for (auto _ : state) {
        if (queue.try_push(msg)) {
            Message* popped = queue.front();
            if (popped) {
                queue.pop(); // Pop immediately to keep queue from filling
            }
        }
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_QueuePush)->Range(64, 65536);

static void BM_QueuePop(benchmark::State& state) {
    rigtorp::SPSCQueue<Message> queue(state.range(0));
    Message msg{1, {1.0, 2.0, 3.0, 4.0}, "BTC/USD"};
    
    // Pre-fill queue
    size_t fill_count = std::min(static_cast<size_t>(state.range(0) / 2), size_t(1000));
    for (size_t i = 0; i < fill_count; ++i) {
        queue.try_push(msg);
    }
    
    for (auto _ : state) {
        Message* popped = queue.front();
        if (popped) {
            queue.pop();
            // Re-push to maintain queue level
            if (fill_count > 0) {
                queue.try_push(msg);
            }
        }
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_QueuePop)->Range(64, 65536);

static void BM_QueueThroughput(benchmark::State& state) {
    rigtorp::SPSCQueue<Message> queue(65536);
    Message msg{1, {1.0, 2.0, 3.0, 4.0}, "BTC/USD"};
    std::atomic<bool> running{true};
    
    // Pre-fill queue
    for (int i = 0; i < 1000; ++i) {
        queue.try_push(msg);
    }
    
    // Consumer (this thread)
    for (auto _ : state) {
        Message* popped = queue.front();
        if (popped) {
            queue.pop();
            // Re-push to maintain queue level
            queue.try_push(msg);
        }
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_QueueThroughput);

BENCHMARK_MAIN();

