/// @file strategies.cpp
/// @brief Trading strategy engine demo
///
/// Demonstrates the alert strategy system with price and volume alerts.
///
/// Usage: ./strategies
/// Press Ctrl+C to exit

#include "common.hpp"
#include <iostream>
#include <iomanip>

int main(int argc, char* argv[]) {
    // Load config file if provided
    try {
        examples::load_config_from_args(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << "Error loading config file: " << e.what() << std::endl;
        std::cerr << "Usage: " << argv[0] << " [--config=path/to/config.cfg]" << std::endl;
        return 1;
    }
    std::cout << "=== Kraken SDK Strategy Engine Demo ===" << std::endl;
    std::cout << "Monitoring BTC/USD and ETH/USD for trading signals\n" << std::endl;
    
    examples::g_client = examples::create_default_client();
    examples::setup_signal_handlers();
    examples::setup_common_callbacks(*examples::g_client);
    
    //--------------------------------------------------------------------------
    // Strategy 1: Price Alert
    //--------------------------------------------------------------------------
    
    // Alert when BTC goes above $100,000 or below $90,000
    // Using recurring alerts with 5-second cooldown to prevent spam
    auto btc_price = kraken::PriceAlert::Builder()
        .symbol("BTC/USD")
        .above(100000.0)
        .below(90000.0)
        .recurring(true)  // Fire every time condition is met
        .cooldown(std::chrono::seconds(5))  // But wait 5 seconds between alerts
        .build();
    
    examples::g_client->add_alert(btc_price, [](const kraken::Alert& alert) {
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
    
    examples::g_client->add_alert(eth_price, [](const kraken::Alert& alert) {
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
    
    examples::g_client->add_alert(volume_spike, [](const kraken::Alert& alert) {
        std::cout << "\n📊 VOLUME SPIKE: " << alert.symbol << std::endl;
        std::cout << "   Unusual volume detected!" << std::endl;
    });
    
    //--------------------------------------------------------------------------
    // Ticker Display
    //--------------------------------------------------------------------------
    
    int count = 0;
    examples::g_client->on_ticker([&count](const kraken::Ticker& t) {
        // Show every 10th ticker to reduce noise
        if (++count % 10 == 0) {
            std::cout << t.symbol << ": $" << std::fixed << std::setprecision(2)
                      << t.last << " | Vol: " << t.volume_24h << std::endl;
        }
    });
    
    //--------------------------------------------------------------------------
    // Run
    //--------------------------------------------------------------------------
    
    std::cout << "Active strategies: " << examples::g_client->alert_count() << std::endl;
    std::cout << "Subscribing to BTC/USD and ETH/USD tickers...\n" << std::endl;
    
    examples::g_client->subscribe(kraken::Channel::Ticker, {"BTC/USD", "ETH/USD"});
    examples::g_client->run();
    
    std::cout << "\nGoodbye!" << std::endl;
    return 0;
}

