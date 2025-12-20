#pragma once

#include "client_impl.hpp"
#include <string>

namespace kraken {

/// Parse a JSON message from Kraken WebSocket
/// @param raw_json The raw JSON string
/// @return Parsed Message structure
Message parse_message(const std::string& raw_json);

/// Build a subscribe message
/// @param channel The channel to subscribe to
/// @param symbols The symbols to subscribe to
/// @param depth Order book depth (for book channel, 0 = default)
/// @return JSON string for subscription
std::string build_subscribe_message(Channel channel, 
                                     const std::vector<std::string>& symbols,
                                     int depth = 0);

/// Build an unsubscribe message
/// @param channel The channel to unsubscribe from
/// @param symbols The symbols to unsubscribe from
/// @return JSON string for unsubscription
std::string build_unsubscribe_message(Channel channel,
                                       const std::vector<std::string>& symbols);

} // namespace kraken

