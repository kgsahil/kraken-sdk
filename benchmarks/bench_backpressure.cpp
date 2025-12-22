/// @file bench_backpressure.cpp
/// @brief Backpressure and throughput benchmark
///
/// Measures SDK throughput and latency under various conditions.
///
/// Usage: ./benchmark [duration_seconds]

#include <kraken/kraken.hpp>
#include <iostream>
#include <iomanip>
#include <chrono>
#include <atomic>
#include <csignal>
#include <thread>

std::unique_ptr<kraken::KrakenClient> g_client;
std::atomic<bool> g_running{true};

void signal_handler(int) {
    g_running = false;
    if (g_client) g_client->stop();
}

int main(int argc, char* argv[]) {
    int duration_secs = 30;
    if (argc > 1) {
        duration_secs = std::atoi(argv[1]);
        if (duration_secs < 5) duration_secs = 5;
    }
    
    std::cout << "╔═══════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║              KRAKEN SDK PERFORMANCE BENCHMARK                 ║" << std::endl;
    std::cout << "╚═══════════════════════════════════════════════════════════════╝" << std::endl;
    std::cout << "\nDuration: " << duration_secs << " seconds" << std::endl;
    std::cout << "Subscribing to maximum channels for stress test...\n" << std::endl;
    
    // Large queue for benchmarking
    auto config = kraken::ClientConfig::Builder()
        .queue_capacity(262144)
        .build();
    
    g_client = std::make_unique<kraken::KrakenClient>(config);
    std::signal(SIGINT, signal_handler);
    
    // Counters
    std::atomic<uint64_t> ticker_count{0};
    std::atomic<uint64_t> trade_count{0};
    std::atomic<uint64_t> book_count{0};
    
    // Track latency samples
    std::atomic<int64_t> latency_sum_us{0};
    std::atomic<int64_t> latency_max_us{0};
    
    g_client->on_ticker([&](const kraken::Ticker& t) {
        ticker_count++;
    });
    
    g_client->on_trade([&](const kraken::Trade& t) {
        trade_count++;
    });
    
    g_client->on_book([&](const std::string&, const kraken::OrderBook&) {
        book_count++;
    });
    
    // Subscribe to many pairs
    std::vector<std::string> symbols = {
        "BTC/USD", "ETH/USD", "SOL/USD", "XRP/USD", "ADA/USD",
        "DOGE/USD", "AVAX/USD", "DOT/USD", "LINK/USD", "MATIC/USD"
    };
    
    g_client->subscribe(kraken::Channel::Ticker, symbols);
    g_client->subscribe(kraken::Channel::Trade, {"BTC/USD", "ETH/USD"});
    g_client->subscribe_book({"BTC/USD"}, 10);
    
    // Run async
    g_client->run_async();
    
    auto start = std::chrono::steady_clock::now();
    auto end = start + std::chrono::seconds(duration_secs);
    
    std::cout << "Running benchmark..." << std::endl;
    
    // Progress updates
    int last_progress = 0;
    while (g_running && std::chrono::steady_clock::now() < end) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        auto elapsed = std::chrono::steady_clock::now() - start;
        int progress = std::chrono::duration_cast<std::chrono::seconds>(elapsed).count();
        
        if (progress != last_progress) {
            last_progress = progress;
            auto m = g_client->get_metrics();
            std::cout << "[" << std::setw(3) << progress << "s] "
                      << "Msgs: " << m.messages_processed 
                      << " | Rate: " << std::fixed << std::setprecision(1)
                      << m.messages_per_second() << "/s"
                      << " | Queue: " << m.queue_depth
                      << std::endl;
        }
    }
    
    g_client->stop();
    
    auto metrics = g_client->get_metrics();
    auto actual_duration = std::chrono::steady_clock::now() - start;
    double secs = std::chrono::duration<double>(actual_duration).count();
    
    // Results
    std::cout << "\n╔═══════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║                       BENCHMARK RESULTS                       ║" << std::endl;
    std::cout << "╠═══════════════════════════════════════════════════════════════╣" << std::endl;
    
    std::cout << "║ Duration:           " << std::setw(10) << std::fixed << std::setprecision(1)
              << secs << " seconds                        ║" << std::endl;
    
    std::cout << "║ Total Messages:     " << std::setw(10) << metrics.messages_processed
              << "                                ║" << std::endl;
    
    std::cout << "║ Throughput:         " << std::setw(10) << std::fixed << std::setprecision(1)
              << metrics.messages_per_second() << " msg/sec                        ║" << std::endl;
    
    std::cout << "║ Messages Dropped:   " << std::setw(10) << metrics.messages_dropped
              << "                                ║" << std::endl;
    
    std::cout << "║ Max Latency:        " << std::setw(10) << metrics.latency_max_us.count()
              << " µs                             ║" << std::endl;
    
    std::cout << "╠═══════════════════════════════════════════════════════════════╣" << std::endl;
    std::cout << "║                      CHANNEL BREAKDOWN                        ║" << std::endl;
    std::cout << "╠═══════════════════════════════════════════════════════════════╣" << std::endl;
    
    std::cout << "║ Tickers:            " << std::setw(10) << ticker_count.load()
              << " (" << std::setprecision(1) << (ticker_count.load() / secs) << "/s)";
    std::cout << std::string(20 - std::to_string((int)(ticker_count.load()/secs)).length(), ' ')
              << "║" << std::endl;
    
    std::cout << "║ Trades:             " << std::setw(10) << trade_count.load()
              << " (" << std::setprecision(1) << (trade_count.load() / secs) << "/s)";
    std::cout << std::string(20 - std::to_string((int)(trade_count.load()/secs)).length(), ' ')
              << "║" << std::endl;
    
    std::cout << "║ Book Updates:       " << std::setw(10) << book_count.load()
              << " (" << std::setprecision(1) << (book_count.load() / secs) << "/s)";
    std::cout << std::string(20 - std::to_string((int)(book_count.load()/secs)).length(), ' ')
              << "║" << std::endl;
    
    std::cout << "╚═══════════════════════════════════════════════════════════════╝" << std::endl;
    
    // Performance rating
    double rate = metrics.messages_per_second();
    std::cout << "\nPerformance Rating: ";
    if (rate > 1000) std::cout << "🏆 EXCELLENT";
    else if (rate > 500) std::cout << "✅ GOOD";
    else if (rate > 100) std::cout << "⚠️  MODERATE";
    else std::cout << "❌ LOW (check connection)";
    std::cout << std::endl;
    
    return 0;
}

