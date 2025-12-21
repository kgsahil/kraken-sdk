/// @file quickstart.cpp
/// @brief Minimal example - subscribe to tickers in 5 lines
///
/// Usage: ./quickstart
/// Press Ctrl+C to exit

#include <kraken/kraken.hpp>
#include <iostream>
#include <iomanip>
#include <csignal>

// Global client for signal handler
std::unique_ptr<kraken::KrakenClient> g_client;

void signal_handler(int) {
    if (g_client) g_client->stop();
}

int main() {
    std::cout << "=== Kraken SDK Quickstart ===" << std::endl;
    std::cout << "Press Ctrl+C to exit\n" << std::endl;
    
    // Create client
    g_client = std::make_unique<kraken::KrakenClient>();
    
    // Set up signal handler
    std::signal(SIGINT, signal_handler);
    
    // Subscribe to ticker updates
    g_client->on_ticker([](const kraken::Ticker& t) {
        std::cout << std::fixed << std::setprecision(2)
                  << t.symbol << ": "
                  << "$" << t.last 
                  << " (bid: $" << t.bid 
                  << ", ask: $" << t.ask << ")"
                  << std::endl;
    });
    
    // Handle errors
    g_client->on_error([](const kraken::Error& e) {
        std::cerr << "Error: " << e.message << std::endl;
    });
    
    // Connection state
    g_client->on_connection_state([](kraken::ConnectionState state) {
        std::cout << "[Connection: " << kraken::to_string(state) << "]" << std::endl;
    });
    
    // Subscribe to BTC and ETH tickers
    g_client->subscribe(kraken::Channel::Ticker, {"BTC/USD", "ETH/USD"});
    
    // Run event loop (blocking)
    g_client->run();
    
    std::cout << "\nGoodbye!" << std::endl;
    return 0;
}

