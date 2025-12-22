/// @file dashboard.cpp
/// @brief Live performance dashboard with telemetry
///
/// Displays real-time metrics, market data, and telemetry information
/// in a terminal UI. Includes HTTP server for Prometheus scraping.
///
/// Usage: 
///   ./dashboard
///   ./dashboard --config=path/to/config.cfg
/// Press Ctrl+C to exit
///
/// Prometheus metrics: http://localhost:9090/metrics

#include "common.hpp"
#include <iostream>
#include <iomanip>
#include <chrono>
#include <thread>
#include <unordered_map>

std::shared_ptr<kraken::Telemetry> g_telemetry;

struct TickerState {
    kraken::Ticker latest;
    double prev_price = 0.0;
};

int main(int argc, char* argv[]) {
    // Load config file if provided
    try {
        examples::load_config_from_args(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << "Error loading config file: " << e.what() << std::endl;
        std::cerr << "Usage: " << argv[0] << " [--config=path/to/config.cfg]" << std::endl;
        return 1;
    }
    // Configure client with telemetry and HTTP server
    auto config = kraken::ClientConfig::Builder()
        .telemetry(kraken::TelemetryConfig::Builder()
            .service_name("dashboard-demo")
            .service_version("1.0.0")
            .environment("demo")
            .metrics(true)
            .http_server(true, 9090)  // Enable HTTP server for Prometheus
            .otlp_export(false)  // Disable OTLP export for this demo
            .build())
        .gap_detection(true)
        .build();
    
    examples::g_client = std::make_unique<kraken::KrakenClient>(config);
    g_telemetry = examples::g_client->get_telemetry_instance();
    examples::setup_signal_handlers();
    
    // Ticker state
    std::unordered_map<std::string, TickerState> tickers;
    std::mutex ticker_mutex;
    
    // Track tickers
    examples::g_client->on_ticker([&](const kraken::Ticker& t) {
        std::lock_guard<std::mutex> lock(ticker_mutex);
        auto& state = tickers[t.symbol];
        state.prev_price = state.latest.last;
        state.latest = t;
    });
    
    // Subscribe
    examples::g_client->subscribe(kraken::Channel::Ticker, 
                        {"BTC/USD", "ETH/USD", "SOL/USD", "XRP/USD"});
    
    // Run async
    examples::g_client->run_async();
    
    // Dashboard loop
    while (examples::g_running) {
        // Clear screen
        std::cout << "\033[2J\033[H";
        
        auto metrics = examples::g_client->get_metrics();
        
        // Header
        std::cout << "╔═══════════════════════════════════════════════════════════════╗" << std::endl;
        std::cout << "║           KRAKEN SDK LIVE DASHBOARD + TELEMETRY              ║" << std::endl;
        std::cout << "╠═══════════════════════════════════════════════════════════════╣" << std::endl;
        
        // Connection status and telemetry info
        std::cout << "║ Status: " << std::left << std::setw(15) 
                  << kraken::to_string(metrics.connection_state);
        std::cout << " Uptime: " << std::setw(12) << metrics.uptime_string();
        std::cout << "             ║" << std::endl;
        
        // Telemetry HTTP server status
        if (g_telemetry && g_telemetry->is_http_server_running()) {
            std::cout << "║ Telemetry: HTTP Server running on port " 
                      << std::setw(4) << g_telemetry->http_server_port()
                      << " (Prometheus: /metrics)        ║" << std::endl;
        } else {
            std::cout << "║ Telemetry: Disabled                                          ║" << std::endl;
        }
        
        // Performance Metrics
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
        
        // Telemetry-specific metrics (if available)
        if (g_telemetry) {
            const auto& tel_metrics = g_telemetry->metrics();
            double avg_latency = tel_metrics.latency_avg_us();
            
            std::cout << "╠═══════════════════════════════════════════════════════════════╣" << std::endl;
            std::cout << "║                      TELEMETRY METRICS                       ║" << std::endl;
            std::cout << "╠═══════════════════════════════════════════════════════════════╣" << std::endl;
            
            std::cout << "║ Avg Latency:        " << std::setw(15) << std::fixed << std::setprecision(2)
                      << avg_latency << " µs";
            std::cout << " Reconnects:   " << std::setw(10) << tel_metrics.reconnect_attempts() << "  ║" << std::endl;
            
            std::cout << "║ Checksum Failures:  " << std::setw(15) << tel_metrics.checksum_failures();
            std::cout << " Gaps Detected: " << std::setw(10) << tel_metrics.gaps_detected() << "  ║" << std::endl;
            
            std::cout << "║ Alerts Triggered:  " << std::setw(15) << tel_metrics.alerts_triggered();
            std::cout << "                              ║" << std::endl;
        }
        
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
        
        // Footer with telemetry info
        if (g_telemetry && g_telemetry->is_http_server_running()) {
            std::cout << "\n📊 Prometheus metrics: http://localhost:" 
                      << g_telemetry->http_server_port() << "/metrics" << std::endl;
        }
        std::cout << "Press Ctrl+C to exit..." << std::endl;
        
        // Refresh every 500ms
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    
    std::cout << "\nShutting down..." << std::endl;
    return 0;
}

