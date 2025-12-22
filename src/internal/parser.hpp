/// @file parser.hpp
/// @brief Internal JSON message parser
/// 
/// Parses incoming JSON messages from Kraken WebSocket and builds
/// subscription/unsubscription messages.
/// 
/// @note This is an internal header. Users interact via KrakenClient.

#pragma once

#include "client_impl.hpp"  // Same directory
#include <string>

namespace kraken {

/// @brief Parse a JSON message from Kraken WebSocket
/// 
/// Parses incoming JSON messages and extracts ticker, trade, book, or OHLC data.
/// Handles various message formats including snapshots, updates, and control messages.
/// 
/// @param raw_json The raw JSON string from WebSocket
/// @return Parsed Message structure with type and data
/// @throws ParseError if JSON is invalid or message format is unexpected
Message parse_message(const std::string& raw_json);

/// @brief Build a subscribe message
/// 
/// Creates a JSON subscription message in Kraken WebSocket API v2 format.
/// 
/// @param channel The channel to subscribe to (ticker, trade, book, ohlc)
/// @param symbols List of trading pairs (e.g., {"BTC/USD", "ETH/USD"})
/// @param depth Order book depth (for book channel: 10, 25, 100, 500, 1000; 0 = default)
/// @return JSON string ready to send over WebSocket
std::string build_subscribe_message(Channel channel, 
                                     const std::vector<std::string>& symbols,
                                     int depth = 0);

/// @brief Build an unsubscribe message
/// 
/// Creates a JSON unsubscription message in Kraken WebSocket API v2 format.
/// 
/// @param channel The channel to unsubscribe from
/// @param symbols List of trading pairs to unsubscribe from
/// @return JSON string ready to send over WebSocket
std::string build_unsubscribe_message(Channel channel,
                                       const std::vector<std::string>& symbols);

} // namespace kraken

