/// @file common.cpp
/// @brief Implementation of common example utilities

#include "common.hpp"
#include <thread>

namespace examples {

//------------------------------------------------------------------------------
// Global State
//------------------------------------------------------------------------------

std::unique_ptr<kraken::KrakenClient> g_client;
std::atomic<bool> g_running{true};

//------------------------------------------------------------------------------
// Signal Handling
//------------------------------------------------------------------------------

void signal_handler(int signal) {
    (void)signal;  // Unused parameter
    g_running = false;
    if (g_client) {
        g_client->stop();
    }
}

void setup_signal_handlers() {
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
}

//------------------------------------------------------------------------------
// Common Callbacks
//------------------------------------------------------------------------------

void setup_common_callbacks(kraken::KrakenClient& client, bool verbose) {
    client.on_error([](const kraken::Error& e) {
        std::cerr << "Error: " << e.message;
        if (e.code != kraken::ErrorCode::None) {
            std::cerr << " (code: " << static_cast<int>(e.code) << ")";
        }
        std::cerr << std::endl;
    });
    
    if (verbose) {
        client.on_connection_state([](kraken::ConnectionState state) {
            std::cout << "[Connection: " << kraken::to_string(state) << "]" << std::endl;
        });
    }
}

void setup_minimal_callbacks(kraken::KrakenClient& client) {
    client.on_error([](const kraken::Error& e) {
        std::cerr << "Error: " << e.message << std::endl;
    });
}

//------------------------------------------------------------------------------
// Client Factory
//------------------------------------------------------------------------------

std::unique_ptr<kraken::KrakenClient> create_default_client() {
    // Build config from environment variables (if set) or use defaults
    // Note: Config file should be loaded before calling this function
    // using examples::load_config_from_args(argc, argv)
    auto config = kraken::config_from_env();
    return std::make_unique<kraken::KrakenClient>(config);
}

std::unique_ptr<kraken::KrakenClient> create_telemetry_client(
    const std::string& service_name,
    uint16_t http_port) {
    
    auto config = kraken::ClientConfig::Builder()
        .telemetry(kraken::TelemetryConfig::Builder()
            .service_name(service_name)
            .service_version("1.0.0")
            .environment("example")
            .metrics(true)
            .http_server(http_port > 0, http_port)
            .otlp_export(false)
            .build())
        .build();
    
    return std::make_unique<kraken::KrakenClient>(config);
}

//------------------------------------------------------------------------------
// Utility Functions
//------------------------------------------------------------------------------

void print_section(const std::string& title) {
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "  " << title << std::endl;
    std::cout << std::string(60, '=') << std::endl << std::endl;
}

void print_separator() {
    std::cout << std::string(60, '-') << std::endl;
}

bool wait_for_connection(kraken::KrakenClient& client, int timeout_seconds) {
    for (int i = 0; i < timeout_seconds * 10; ++i) {
        if (client.is_connected()) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    return false;
}

} // namespace examples

