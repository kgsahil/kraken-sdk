/// @file trading_bot.cpp
/// @brief Real-world example: Simple trading bot with decision logic
///
/// Demonstrates a practical trading bot that:
/// - Monitors price movements
/// - Tracks moving averages
/// - Makes trading decisions based on strategy
/// - Logs decisions (could be extended to execute trades)
///
/// Usage: ./trading_bot
/// Press Ctrl+C to exit

#include "common.hpp"
#include <deque>
#include <unordered_map>
#include <mutex>
#include <thread>
#include <algorithm>
#include <iomanip>
#include <atomic>

struct SymbolState {
    std::deque<double> prices;  // Price history for moving average
    double moving_average = 0.0;
    double last_price = 0.0;
    bool position = false;  // false = no position, true = long position
    double entry_price = 0.0;
};

std::unordered_map<std::string, SymbolState> symbol_states;
std::mutex state_mutex;
const size_t MA_PERIOD = 20;  // 20-period moving average

void update_moving_average(SymbolState& state, double price) {
    state.prices.push_back(price);
    if (state.prices.size() > MA_PERIOD) {
        state.prices.pop_front();
    }
    
    if (state.prices.size() == MA_PERIOD) {
        double sum = 0.0;
        for (double p : state.prices) {
            sum += p;
        }
        state.moving_average = sum / MA_PERIOD;
    }
}

void process_trading_signal(const std::string& symbol, SymbolState& state) {
    if (state.moving_average == 0.0) return;  // Not enough data
    
    // Simple strategy: Buy when price crosses above MA, sell when below
    if (!state.position && state.last_price > state.moving_average) {
        // Buy signal
        state.position = true;
        state.entry_price = state.last_price;
        
        std::cout << "\n[BUY SIGNAL] " << symbol 
                  << " @ $" << std::fixed << std::setprecision(2) << state.last_price
                  << " (MA: $" << state.moving_average << ")" << std::endl;
        
        // In real bot, this would execute a buy order via Kraken REST API
        
    } else if (state.position && state.last_price < state.moving_average) {
        // Sell signal
        double profit = state.last_price - state.entry_price;
        double profit_pct = (profit / state.entry_price) * 100.0;
        
        std::cout << "\n[SELL SIGNAL] " << symbol
                  << " @ $" << std::fixed << std::setprecision(2) << state.last_price
                  << " | Entry: $" << state.entry_price
                  << " | P&L: $" << profit << " (" << profit_pct << "%)" << std::endl;
        
        state.position = false;
        state.entry_price = 0.0;
        
        // In real bot, this would execute a sell order via Kraken REST API
    }
}

int main(int argc, char* argv[]) {
    // Load config file if provided
    try {
        examples::load_config_from_args(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << "Error loading config file: " << e.what() << std::endl;
        std::cerr << "Usage: " << argv[0] << " [--config=path/to/config.cfg]" << std::endl;
        return 1;
    }
    std::cout << "=== Simple Trading Bot ===" << std::endl;
    std::cout << "Strategy: Moving Average Crossover (20-period)" << std::endl;
    std::cout << "Monitoring: BTC/USD, ETH/USD\n" << std::endl;
    
    examples::g_client = examples::create_default_client();
    examples::setup_signal_handlers();
    examples::setup_minimal_callbacks(*examples::g_client);
    
    // Process ticker updates
    std::atomic<int> update_count{0};
    examples::g_client->on_ticker([&update_count](const kraken::Ticker& ticker) {
        std::lock_guard<std::mutex> lock(state_mutex);
        
        auto& state = symbol_states[ticker.symbol];
        state.last_price = ticker.last;
        
        update_moving_average(state, ticker.last);
        process_trading_signal(ticker.symbol, state);
        
        int count = update_count.fetch_add(1, std::memory_order_relaxed) + 1;
        
        // Show status every 50 updates
        if (count % 50 == 0) {
            std::cout << "\rUpdates: " << count 
                      << " | BTC: $" << std::fixed << std::setprecision(2)
                      << (symbol_states.count("BTC/USD") ? symbol_states["BTC/USD"].last_price : 0.0)
                      << " | ETH: $" 
                      << (symbol_states.count("ETH/USD") ? symbol_states["ETH/USD"].last_price : 0.0)
                      << std::flush;
        }
    });
    
    // Subscribe to symbols
    examples::g_client->subscribe(kraken::Channel::Ticker, {"BTC/USD", "ETH/USD"});
    
    // Run
    examples::g_client->run_async();
    
    // Wait for shutdown
    while (examples::g_running && examples::g_client->is_running()) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    std::cout << "\n\nBot stopped. Total updates processed: " << update_count.load() << std::endl;
    
    return 0;
}

