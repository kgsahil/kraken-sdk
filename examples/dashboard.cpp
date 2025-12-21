/// @file dashboard.cpp
/// @brief Live performance dashboard
///
/// Displays real-time metrics and market data in a terminal UI.
///
/// Usage: ./dashboard
/// Press Ctrl+C to exit

#include <kraken/kraken.hpp>
#include <iostream>
#include <iomanip>
#include <csignal>
#include <chrono>
#include <thread>
#include <unordered_map>

std::unique_ptr<kraken::KrakenClient> g_client;
std::atomic<bool> g_running{true};

void signal_handler(int) {
    g_running = false;
    if (g_client) g_client->stop();
}

struct TickerState {
    kraken::Ticker latest;
    double prev_price = 0.0;
};

int main() {
    g_client = std::make_unique<kraken::KrakenClient>();
    std::signal(SIGINT, signal_handler);
    
    // Ticker state
    std::unordered_map<std::string, TickerState> tickers;
    std::mutex ticker_mutex;
    
    // Track tickers
    g_client->on_ticker([&](const kraken::Ticker& t) {
        std::lock_guard<std::mutex> lock(ticker_mutex);
        auto& state = tickers[t.symbol];
        state.prev_price = state.latest.last;
        state.latest = t;
    });
    
    // Connection state
    g_client->on_connection_state([](kraken::ConnectionState state) {
        // Will be shown in dashboard
    });
    
    // Subscribe
    g_client->subscribe(kraken::Channel::Ticker, 
                        {"BTC/USD", "ETH/USD", "SOL/USD", "XRP/USD"});
    
    // Run async
    g_client->run_async();
    
    // Dashboard loop
    while (g_running) {
        // Clear screen
        std::cout << "\033[2J\033[H";
        
        auto metrics = g_client->get_metrics();
        
        // Header
        std::cout << "╔═══════════════════════════════════════════════════════════════╗" << std::endl;
        std::cout << "║               KRAKEN SDK LIVE DASHBOARD                       ║" << std::endl;
        std::cout << "╠═══════════════════════════════════════════════════════════════╣" << std::endl;
        
        // Connection status
        std::cout << "║ Status: " << std::left << std::setw(15) 
                  << kraken::to_string(metrics.connection_state);
        std::cout << " Uptime: " << std::setw(12) << metrics.uptime_string();
        std::cout << "             ║" << std::endl;
        
        // Metrics
        std::cout << "╠═══════════════════════════════════════════════════════════════╣" << std::endl;
        std::cout << "║                       PERFORMANCE METRICS                     ║" << std::endl;
        std::cout << "╠═══════════════════════════════════════════════════════════════╣" << std::endl;
        
        std::cout << "║ Messages Received:  " << std::setw(15) << metrics.messages_received;
        std::cout << " Messages/sec: " << std::setw(10) << std::fixed << std::setprecision(1)
                  << metrics.messages_per_second() << "  ║" << std::endl;
                  
        std::cout << "║ Messages Processed: " << std::setw(15) << metrics.messages_processed;
        std::cout << " Queue Depth:  " << std::setw(10) << metrics.queue_depth << "  ║" << std::endl;
        
        std::cout << "║ Messages Dropped:   " << std::setw(15) << metrics.messages_dropped;
        std::cout << " Max Latency:  " << std::setw(7) << metrics.latency_max_us.count() 
                  << " µs  ║" << std::endl;
        
        // Tickers
        std::cout << "╠═══════════════════════════════════════════════════════════════╣" << std::endl;
        std::cout << "║                          TICKERS                              ║" << std::endl;
        std::cout << "╠═══════════════════════════════════════════════════════════════╣" << std::endl;
        std::cout << "║  Symbol     │    Price    │    Bid      │    Ask      │ Chg  ║" << std::endl;
        std::cout << "╠─────────────┼─────────────┼─────────────┼─────────────┼──────╣" << std::endl;
        
        {
            std::lock_guard<std::mutex> lock(ticker_mutex);
            for (const auto& [symbol, state] : tickers) {
                const auto& t = state.latest;
                
                // Calculate change indicator
                char change = ' ';
                if (state.prev_price > 0) {
                    if (t.last > state.prev_price) change = '+';
                    else if (t.last < state.prev_price) change = '-';
                }
                
                std::cout << "║ " << std::left << std::setw(10) << t.symbol << " │ "
                          << std::right << std::fixed << std::setprecision(2)
                          << "$" << std::setw(10) << t.last << " │ "
                          << "$" << std::setw(10) << t.bid << " │ "
                          << "$" << std::setw(10) << t.ask << " │  "
                          << change << "   ║" << std::endl;
            }
        }
        
        std::cout << "╚═══════════════════════════════════════════════════════════════╝" << std::endl;
        std::cout << "\nPress Ctrl+C to exit..." << std::endl;
        
        // Refresh every 500ms
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    
    std::cout << "\nShutting down..." << std::endl;
    return 0;
}

