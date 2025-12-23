/// @file client_impl.cpp
/// @brief Main client implementation file
/// 
/// This file now serves as the main entry point that includes all client modules.
/// Implementation has been split into focused modules in src/client/:
/// - lifecycle.cpp - Construction, connection, event loop
/// - callbacks.cpp - Callback registration
/// - subscriptions.cpp - Subscription management
/// - strategies.cpp - Strategy management
/// - dispatch.cpp - Message dispatch and I/O loops
/// - reconnect.cpp - Reconnection logic
/// - snapshots.cpp - Data snapshots
/// - metrics.cpp - Metrics collection

#include "internal/client_impl.hpp"
#include "queue.cpp"  // NOLINT(bugprone-suspicious-include) - Intentional for template instantiation

// Include all client implementation modules
// These are included here rather than compiled separately to maintain
// access to private members of KrakenClient::Impl

// Note: In a production system, we might use friend classes or expose
// these as public methods. For now, we include the implementations here.

namespace kraken {
    // All implementations are in separate files in src/client/
    // This file exists to maintain the build structure
}

