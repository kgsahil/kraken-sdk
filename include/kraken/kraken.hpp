#pragma once

/// @file kraken.hpp
/// @brief Main header for Kraken WebSocket SDK
/// 
/// Include this single header to access all SDK functionality.

#include "types.hpp"
#include "error.hpp"
#include "config.hpp"
#include "metrics.hpp"
#include "subscription.hpp"
#include "strategies.hpp"
#include "client.hpp"

/// @mainpage Kraken WebSocket SDK
/// 
/// @section intro Introduction
/// 
/// A high-performance C++ SDK for streaming real-time market data from Kraken.
/// 
/// Key features:
/// - Lock-free SPSC queue for low-latency message delivery
/// - Order book reconstruction with CRC32 checksum validation
/// - Built-in trading strategy engine
/// - Auto-reconnection with resubscription
/// - Thread-safe callbacks
/// 
/// @section quickstart Quickstart
/// 
/// @code
/// #include <kraken/kraken.hpp>
/// #include <iostream>
/// 
/// int main() {
///     kraken::KrakenClient client;
///     
///     client.on_ticker([](const kraken::Ticker& t) {
///         std::cout << t.symbol << ": $" << t.last << std::endl;
///     });
///     
///     client.subscribe(kraken::Channel::Ticker, {"BTC/USD", "ETH/USD"});
///     client.run();  // Blocking
/// }
/// @endcode
/// 
/// @section strategies Trading Strategies
/// 
/// @code
/// // Price alert
/// auto alert = kraken::PriceAlert::Builder()
///     .symbol("BTC/USD")
///     .above(50000.0)
///     .build();
/// 
/// client.add_alert(alert, [](const kraken::Alert& a) {
///     std::cout << "ALERT: " << a.message << std::endl;
/// });
/// @endcode

