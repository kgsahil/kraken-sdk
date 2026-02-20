#include "kraken/kraken.hpp"
#include "kraken/strategies/strategies.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <string>

// Simulates an external data source reading from Aeron IPC or Redpanda
std::vector<kraken::Ticker> fetch_historical_tickers() {
    std::vector<kraken::Ticker> tickers;
    for (int i = 0; i < 5; ++i) {
        kraken::Ticker t;
        t.symbol = "BTC/USD";
        t.last = 50000.0 + (i * 100.0);
        t.volume_24h = 1000.0;
        tickers.push_back(t);
    }
    return tickers;
}

int main() {
    std::cout << "Starting Kraken SDK Offline Replay Example...\n";

    // 1. Configure the Client for P2 Strategy Role / P3 Analysis Role
    //    offline_mode(true) prevents connecting to WebSockets.
    //    use_queue(false) bypasses thread hops since we aren't using the IO thread.
    auto config = kraken::ClientConfig::Builder()
        .offline_mode(true)
        .use_queue(false)
        .build();

    kraken::KrakenClient client(config);

    // 2. Register standard callbacks
    client.on_ticker([](const kraken::Ticker& t) {
        std::cout << "[Callback] Ticker Update -> " << t.symbol 
                  << " | Price: $" << t.last << "\n";
    });

    // 3. Register a Strategy 
    auto price_alert = kraken::PriceAlert::Builder()
                           .symbol("BTC/USD")
                           .above(50250.0)
                           .build();
    client.add_alert(price_alert, [](const kraken::Alert& event) {
        std::cout << "\n============================================\n"
                  << "🚨 STRATEGY TRIGGERED: " << event.strategy_name 
                  << " | " << event.symbol << " | Trigger Price: $" << event.price
                  << "\n============================================\n\n";
    });

    // 4. Run the Client (It will just initialize state and return immediately because offline_mode is true)
    client.run();
    std::cout << "SDK Initialized Offline.\n";

    // 5. Begin Replay Loop (Simulating Aeron / Kafka Polling)
    std::cout << "Simulating Aeron IPC Polling loop and injecting data into ReplayEngine...\n\n";
    
    auto& replay_engine = client.get_replay_engine();
    auto test_market_data = fetch_historical_tickers();

    for (const auto& ticker : test_market_data) {
        std::cout << "Injecting Ticker: $" << ticker.last << "\n";
        
        // Feed data directly into the Strategy Engine & Callbacks
        replay_engine.inject_ticker(ticker);
        
        // Small delay to simulate time passing during backtest/replay
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    std::cout << "\nReplay complete. Exiting.\n";
    client.stop();

    return 0;
}
