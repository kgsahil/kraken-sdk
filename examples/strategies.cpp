/// @file strategies.cpp
/// @brief Trading strategy engine demo
///
/// Demonstrates the alert strategy system with price and volume alerts.
///
/// Usage: ./strategies
/// Press Ctrl+C to exit

#include <kraken/kraken.hpp>
#include <iostream>
#include <iomanip>
#include <csignal>

std::unique_ptr<kraken::KrakenClient> g_client;

void signal_handler(int) {
    if (g_client) g_client->stop();
}

int main() {
    std::cout << "=== Kraken SDK Strategy Engine Demo ===" << std::endl;
    std::cout << "Monitoring BTC/USD and ETH/USD for trading signals\n" << std::endl;
    
    g_client = std::make_unique<kraken::KrakenClient>();
    std::signal(SIGINT, signal_handler);
    
    //--------------------------------------------------------------------------
    // Strategy 1: Price Alert
    //--------------------------------------------------------------------------
    
    // Alert when BTC goes above $100,000 or below $90,000
    auto btc_price = kraken::PriceAlert::Builder()
        .symbol("BTC/USD")
        .above(100000.0)
        .below(90000.0)
        .build();
    
    g_client->add_alert(btc_price, [](const kraken::Alert& alert) {
        std::cout << "\n🚨 PRICE ALERT: " << alert.symbol << std::endl;
        std::cout << "   " << alert.message << std::endl;
        std::cout << "   Current price: $" << std::fixed << std::setprecision(2) 
                  << alert.price << std::endl;
    });
    
    //--------------------------------------------------------------------------
    // Strategy 2: ETH Price Alert
    //--------------------------------------------------------------------------
    
    auto eth_price = kraken::PriceAlert::Builder()
        .symbol("ETH/USD")
        .above(4000.0)
        .below(3000.0)
        .build();
    
    g_client->add_alert(eth_price, [](const kraken::Alert& alert) {
        std::cout << "\n🚨 PRICE ALERT: " << alert.symbol << std::endl;
        std::cout << "   " << alert.message << std::endl;
        std::cout << "   Current price: $" << std::fixed << std::setprecision(2)
                  << alert.price << std::endl;
    });
    
    //--------------------------------------------------------------------------
    // Strategy 3: Volume Spike Detection
    //--------------------------------------------------------------------------
    
    auto volume_spike = kraken::VolumeSpike::Builder()
        .symbols({"BTC/USD", "ETH/USD"})
        .multiplier(2.0)  // Alert when volume is 2x average
        .lookback(50)     // Compare to last 50 samples
        .build();
    
    g_client->add_alert(volume_spike, [](const kraken::Alert& alert) {
        std::cout << "\n📊 VOLUME SPIKE: " << alert.symbol << std::endl;
        std::cout << "   Unusual volume detected!" << std::endl;
    });
    
    //--------------------------------------------------------------------------
    // Ticker Display
    //--------------------------------------------------------------------------
    
    int count = 0;
    g_client->on_ticker([&count](const kraken::Ticker& t) {
        // Show every 10th ticker to reduce noise
        if (++count % 10 == 0) {
            std::cout << t.symbol << ": $" << std::fixed << std::setprecision(2)
                      << t.last << " | Vol: " << t.volume_24h << std::endl;
        }
    });
    
    //--------------------------------------------------------------------------
    // Error Handling
    //--------------------------------------------------------------------------
    
    g_client->on_error([](const kraken::Error& e) {
        std::cerr << "Error: " << e.message << std::endl;
    });
    
    g_client->on_connection_state([](kraken::ConnectionState state) {
        std::cout << "[" << kraken::to_string(state) << "]" << std::endl;
    });
    
    //--------------------------------------------------------------------------
    // Run
    //--------------------------------------------------------------------------
    
    std::cout << "Active strategies: " << g_client->alert_count() << std::endl;
    std::cout << "Subscribing to BTC/USD and ETH/USD tickers...\n" << std::endl;
    
    g_client->subscribe(kraken::Channel::Ticker, {"BTC/USD", "ETH/USD"});
    g_client->run();
    
    std::cout << "\nGoodbye!" << std::endl;
    return 0;
}

