/// @file telemetry.cpp
/// @brief OpenTelemetry metrics demo
///
/// Demonstrates telemetry configuration and metrics collection.
/// Shows how metrics are automatically collected and can be exported.
///
/// Usage: ./telemetry
/// Press Ctrl+C to exit

#include "common.hpp"
#include <iostream>
#include <iomanip>
#include <thread>
#include <chrono>

void print_metrics(const kraken::Metrics& m) {
    std::cout << "\033[2J\033[H";  // Clear screen
    std::cout << "╔═══════════════════════════════════════════════════════════╗\n";
    std::cout << "║           KRAKEN SDK TELEMETRY METRICS                     ║\n";
    std::cout << "╠═══════════════════════════════════════════════════════════╣\n";
    std::cout << "║ Connection: " << std::left << std::setw(45) 
              << kraken::to_string(m.connection_state) << "║\n";
    std::cout << "║ Uptime:     " << std::setw(45) << m.uptime_string() << "║\n";
    std::cout << "╠═══════════════════════════════════════════════════════════╣\n";
    std::cout << "║ Messages Received:  " << std::right << std::setw(30) 
              << m.messages_received << " ║\n";
    std::cout << "║ Messages Processed:  " << std::setw(30) 
              << m.messages_processed << " ║\n";
    std::cout << "║ Messages Dropped:    " << std::setw(30) 
              << m.messages_dropped << " ║\n";
    std::cout << "║ Queue Depth:         " << std::setw(30) 
              << m.queue_depth << " ║\n";
    std::cout << "╠═══════════════════════════════════════════════════════════╣\n";
    std::cout << "║ Messages/sec:        " << std::setw(30) 
              << std::fixed << std::setprecision(2) << m.messages_per_second() << " ║\n";
    std::cout << "║ Max Latency:         " << std::setw(30) 
              << m.latency_max_us.count() << " μs ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════╝\n";
    std::cout << "\nPress Ctrl+C to exit\n";
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
    std::cout << "=== Kraken SDK Telemetry Demo ===" << std::endl;
    std::cout << "Demonstrating OpenTelemetry metrics collection\n" << std::endl;
    
    //--------------------------------------------------------------------------
    // Configure Telemetry with HTTP Server
    //--------------------------------------------------------------------------
    
    auto config = kraken::ClientConfig::Builder()
        // Enable telemetry with HTTP server for Prometheus scraping
        .telemetry(kraken::TelemetryConfig::Builder()
            .service_name("telemetry-demo")
            .service_version("1.0.0")
            .environment("demo")
            .metrics(true)  // Enable metrics collection
            .http_server(true, 9090)  // Enable HTTP server on port 9090
            .otlp_export(false)  // OTLP export disabled for this demo
            .metrics_interval(std::chrono::seconds(15))
            .build())
        // Enable gap detection to show gap metrics
        .gap_detection(true)
        .on_gap([](const kraken::GapInfo& gap) {
            std::cerr << "⚠️  Gap detected: " << gap.symbol 
                      << " (missed " << gap.gap_size << " messages)" << std::endl;
        })
        .build();
    
    examples::g_client = std::make_unique<kraken::KrakenClient>(config);
    examples::setup_signal_handlers();
    
    std::cout << "Telemetry configured:" << std::endl;
    std::cout << "  - Service: telemetry-demo" << std::endl;
    std::cout << "  - Metrics: Enabled" << std::endl;
    std::cout << "  - HTTP Server: Enabled on port 9090" << std::endl;
    std::cout << "  - Prometheus: http://localhost:9090/metrics" << std::endl;
    std::cout << "  - Health: http://localhost:9090/health" << std::endl;
    std::cout << "  - Gap Detection: Enabled\n" << std::endl;
    
    //--------------------------------------------------------------------------
    // Set up callbacks
    //--------------------------------------------------------------------------
    
    int ticker_count = 0;
    examples::g_client->on_ticker([&ticker_count](const kraken::Ticker& t) {
        // Metrics are automatically collected - no manual tracking needed!
        ticker_count++;
        
        // Show ticker updates (throttled)
        if (ticker_count % 20 == 0) {
            std::cout << "\r" << t.symbol << ": $" << std::fixed << std::setprecision(2)
                      << t.last << " (spread: $" << t.spread() << ")" << std::flush;
        }
    });
    
    examples::g_client->on_error([](const kraken::Error& e) {
        std::cerr << "\nError: " << e.message << std::endl;
    });
    
    examples::g_client->on_connection_state([](kraken::ConnectionState state) {
        std::cout << "\n[Connection: " << kraken::to_string(state) << "]" << std::endl;
    });
    
    //--------------------------------------------------------------------------
    // Subscribe and run
    //--------------------------------------------------------------------------
    
    std::cout << "Subscribing to BTC/USD and ETH/USD tickers..." << std::endl;
    std::cout << "Metrics are automatically collected in the background.\n" << std::endl;
    
    examples::g_client->subscribe(kraken::Channel::Ticker, {"BTC/USD", "ETH/USD"});
    
    // Run client in background
    examples::g_client->run_async();
    
    // Wait for connection
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    //--------------------------------------------------------------------------
    // Display metrics periodically
    //--------------------------------------------------------------------------
    
    std::cout << "\nDisplaying metrics every 3 seconds...\n" << std::endl;
    
    while (examples::g_running && examples::g_client->is_running()) {
        // Get metrics snapshot (reads from Telemetry if enabled)
        auto metrics = examples::g_client->get_metrics();
        
        // Display metrics
        print_metrics(metrics);
        
        // Wait before next update
        std::this_thread::sleep_for(std::chrono::seconds(3));
    }
    
    //--------------------------------------------------------------------------
    // Final metrics summary
    //--------------------------------------------------------------------------
    
    std::cout << "\n\n=== Final Metrics Summary ===" << std::endl;
    auto final_metrics = examples::g_client->get_metrics();
    
    std::cout << "Total Messages Received:  " << final_metrics.messages_received << std::endl;
    std::cout << "Total Messages Processed: " << final_metrics.messages_processed << std::endl;
    std::cout << "Messages Dropped:         " << final_metrics.messages_dropped << std::endl;
    std::cout << "Average Throughput:       " << std::fixed << std::setprecision(2)
              << final_metrics.messages_per_second() << " msg/sec" << std::endl;
    std::cout << "Max Latency:              " << final_metrics.latency_max_us.count() 
              << " μs" << std::endl;
    std::cout << "Gaps Detected:            " << examples::g_client->gap_count() << std::endl;
    
    std::cout << "\n=== Metrics Export ===" << std::endl;
    std::cout << "Metrics are available via:" << std::endl;
    std::cout << "  - HTTP Server: http://localhost:9090/metrics (Prometheus format)" << std::endl;
    std::cout << "  - Health Check: http://localhost:9090/health" << std::endl;
    std::cout << "  - OTLP Export: Configured via telemetry config" << std::endl;
    std::cout << "  - JSON format: metrics.to_json() method" << std::endl;
    std::cout << "\nTry: curl http://localhost:9090/metrics" << std::endl;
    std::cout << "See docs/METRICS.md for details.\n" << std::endl;
    
    std::cout << "Goodbye!" << std::endl;
    return 0;
}

