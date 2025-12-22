/// @file common.hpp
/// @brief Common utilities for example programs
///
/// Provides reusable components to reduce boilerplate in examples:
/// - Signal handling
/// - Client lifecycle management
/// - Common callback setup
/// - Config file loading

#pragma once

#include <kraken/kraken.hpp>
#include "config.hpp"
#include <csignal>
#include <memory>
#include <atomic>
#include <iostream>

namespace examples {

//------------------------------------------------------------------------------
// Signal Handler & Client Lifecycle
//------------------------------------------------------------------------------

/// Global client instance (for signal handler)
extern std::unique_ptr<kraken::KrakenClient> g_client;
extern std::atomic<bool> g_running;

/// Signal handler for graceful shutdown
void signal_handler(int signal);

/// Setup signal handlers for graceful shutdown
void setup_signal_handlers();

//------------------------------------------------------------------------------
// Common Callback Setup
//------------------------------------------------------------------------------

/// Setup common error and connection state callbacks
/// @param client The client to configure
/// @param verbose If true, print connection state changes
void setup_common_callbacks(kraken::KrakenClient& client, bool verbose = true);

/// Setup minimal callbacks (errors only, no connection state)
void setup_minimal_callbacks(kraken::KrakenClient& client);

//------------------------------------------------------------------------------
// Client Factory
//------------------------------------------------------------------------------

/// Create a client with default configuration
std::unique_ptr<kraken::KrakenClient> create_default_client();

/// Create a client with telemetry enabled
/// @param service_name Service name for telemetry
/// @param http_port HTTP server port (0 to disable)
std::unique_ptr<kraken::KrakenClient> create_telemetry_client(
    const std::string& service_name = "example-client",
    uint16_t http_port = 9090
);

//------------------------------------------------------------------------------
// Utility Functions
//------------------------------------------------------------------------------

/// Print a section header
void print_section(const std::string& title);

/// Print a separator line
void print_separator();

/// Wait for connection with timeout
/// @param client The client to wait for
/// @param timeout_seconds Maximum time to wait
/// @return true if connected, false on timeout
bool wait_for_connection(kraken::KrakenClient& client, int timeout_seconds = 5);

} // namespace examples

