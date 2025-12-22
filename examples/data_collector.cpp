/// @file data_collector.cpp
/// @brief Real-world example: Collect and store market data
///
/// Demonstrates a practical use case: collecting ticker data and storing it
/// to a file (could be extended to database, time-series DB, etc.)
///
/// Usage: ./data_collector [output_file.csv]
/// Press Ctrl+C to exit

#include "common.hpp"
#include <fstream>
#include <iomanip>
#include <sstream>
#include <mutex>
#include <thread>
#include <chrono>
#include <ctime>
#include <cstring>
#include <atomic>

std::mutex file_mutex;
std::ofstream data_file;

void write_ticker_to_file(const kraken::Ticker& ticker) {
    std::lock_guard<std::mutex> lock(file_mutex);
    
    if (!data_file.is_open()) return;
    
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    
    // Thread-safe: use localtime_r (POSIX) or localtime_s (Windows)
    // For portability, format manually
    std::tm tm_buf;
    #ifdef _WIN32
        localtime_s(&tm_buf, &time_t);
    #else
        localtime_r(&time_t, &tm_buf);
    #endif
    
    char time_str[32];
    std::strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", &tm_buf);
    
    data_file << time_str
              << "," << ticker.symbol
              << "," << std::fixed << std::setprecision(2) << ticker.last
              << "," << ticker.bid
              << "," << ticker.ask
              << "," << ticker.volume_24h
              << "," << ticker.high_24h
              << "," << ticker.low_24h
              << "\n";
    data_file.flush();  // Ensure data is written immediately
}

int main(int argc, char* argv[]) {
    // Load config file if provided
    try {
        examples::load_config_from_args(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << "Error loading config file: " << e.what() << std::endl;
        std::cerr << "Usage: " << argv[0] << " [output.csv] [--config=path/to/config.cfg]" << std::endl;
        return 1;
    }
    
    // Parse output file argument (skip --config if present)
    std::string output_file = "market_data.csv";
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg.find("--config=") != 0 && arg != "--config") {
            output_file = arg;
            break;
        }
        if (arg == "--config" && i + 1 < argc) {
            i++;  // Skip the config path
            continue;
        }
    }
    
    std::cout << "=== Market Data Collector ===" << std::endl;
    std::cout << "Collecting ticker data to: " << output_file << std::endl;
    std::cout << "Press Ctrl+C to stop\n" << std::endl;
    
    // Open output file
    data_file.open(output_file, std::ios::out | std::ios::app);
    if (!data_file.is_open()) {
        std::cerr << "Error: Cannot open file " << output_file << std::endl;
        return 1;
    }
    
    // Write CSV header if file is new
    if (data_file.tellp() == 0) {
        data_file << "timestamp,symbol,last,bid,ask,volume_24h,high_24h,low_24h\n";
    }
    
    // Create client
    examples::g_client = examples::create_default_client();
    examples::setup_signal_handlers();
    examples::setup_minimal_callbacks(*examples::g_client);
    
    // Collect ticker data
    std::atomic<int> ticker_count{0};
    examples::g_client->on_ticker([&ticker_count](const kraken::Ticker& t) {
        write_ticker_to_file(t);
        int count = ticker_count.fetch_add(1, std::memory_order_relaxed) + 1;
        
        // Progress indicator every 100 tickers
        if (count % 100 == 0) {
            std::cout << "\rCollected " << count << " ticker updates..." << std::flush;
        }
    });
    
    // Subscribe to multiple symbols
    examples::g_client->subscribe(kraken::Channel::Ticker, {
        "BTC/USD", "ETH/USD", "SOL/USD", "XRP/USD", "ADA/USD"
    });
    
    // Run
    examples::g_client->run_async();
    
    // Wait for shutdown
    while (examples::g_running && examples::g_client->is_running()) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    // Stop client to ensure no more callbacks
    examples::g_client->stop();
    
    // Cleanup - close file (safe to call even if already closed)
    {
        std::lock_guard<std::mutex> lock(file_mutex);
        if (data_file.is_open()) {
            data_file.close();
        }
    }
    
    std::cout << "\n\nTotal tickers collected: " << ticker_count.load() << std::endl;
    std::cout << "Data saved to: " << output_file << std::endl;
    
    return 0;
}

