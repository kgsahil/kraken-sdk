/// @file quickstart.cpp
/// @brief Minimal example - subscribe to tickers in 5 lines
///
/// Usage: 
///   ./quickstart
///   ./quickstart --config=path/to/config.cfg
/// Press Ctrl+C to exit

#include "common.hpp"
#include <iostream>
#include <iomanip>

int main(int argc, char* argv[]) {
    std::cout << "=== Kraken SDK Quickstart ===" << std::endl;
    std::cout << "Press Ctrl+C to exit\n" << std::endl;
    
    // Load config file if provided
    try {
        examples::load_config_from_args(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << "Error loading config file: " << e.what() << std::endl;
        std::cerr << "Usage: " << argv[0] << " [--config=path/to/config.cfg]" << std::endl;
        return 1;
    }
    
    // Create client and setup
    examples::g_client = examples::create_default_client();
    examples::setup_signal_handlers();
    examples::setup_common_callbacks(*examples::g_client);
    
    // Subscribe to ticker updates
    examples::g_client->on_ticker([](const kraken::Ticker& t) {
        std::cout << std::fixed << std::setprecision(2)
                  << t.symbol << ": "
                  << "$" << t.last 
                  << " (bid: $" << t.bid 
                  << ", ask: $" << t.ask << ")"
                  << std::endl;
    });
    
    // Subscribe to BTC and ETH tickers
    examples::g_client->subscribe(kraken::Channel::Ticker, {"BTC/USD", "ETH/USD"});
    
    // Run event loop (blocking)
    examples::g_client->run();
    
    std::cout << "\nGoodbye!" << std::endl;
    return 0;
}

