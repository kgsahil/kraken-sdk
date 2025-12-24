#include "internal/parser.hpp"

#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

#include <cstring>
#include <algorithm>
#include <cctype>
#include <string>

namespace kraken {

namespace {

// Helper to safely get string from JSON
std::string get_string(const rapidjson::Value& obj, const char* key, 
                       const std::string& def = "") {
    if (obj.HasMember(key) && obj[key].IsString()) {
        return obj[key].GetString();
    }
    return def;
}

// Helper to safely get double from JSON
double get_double(const rapidjson::Value& obj, const char* key, double def = 0.0) {
    if (obj.HasMember(key)) {
        if (obj[key].IsNumber()) {
            return obj[key].GetDouble();
        }
        if (obj[key].IsString()) {
            try {
                return std::stod(obj[key].GetString());
            } catch (...) {}
        }
    }
    return def;
}

// Helper to strictly parse numbers; returns false if present but not numeric
bool get_double_strict(const rapidjson::Value& obj, const char* key, double& out, double def = 0.0) {
    if (!obj.HasMember(key)) {
        out = def;
        return true;
    }
    const auto& v = obj[key];
    if (v.IsNumber()) {
        out = v.GetDouble();
        return true;
    }
    if (v.IsString()) {
        try {
            out = std::stod(v.GetString());
            return true;
        } catch (...) {
            return false;
        }
    }
    return false;
}

// Parse ticker from Kraken v2 format
Ticker parse_ticker(const rapidjson::Value& data, const std::string& symbol, bool& ok) {
    Ticker t;
    t.symbol = symbol;
    ok = true;
    ok = ok && get_double_strict(data, "bid", t.bid);
    ok = ok && get_double_strict(data, "ask", t.ask);
    ok = ok && get_double_strict(data, "last", t.last);
    ok = ok && get_double_strict(data, "volume", t.volume_24h);
    ok = ok && get_double_strict(data, "high", t.high_24h);
    ok = ok && get_double_strict(data, "low", t.low_24h);
    t.timestamp = get_string(data, "timestamp");
    return t;
}

// Parse trade from Kraken v2 format
Trade parse_trade(const rapidjson::Value& data, const std::string& symbol) {
    Trade t;
    t.symbol = symbol;
    t.price = get_double(data, "price");
    t.quantity = get_double(data, "qty");
    
    std::string side = get_string(data, "side");
    t.side = (side == "sell") ? Side::Sell : Side::Buy;
    t.timestamp = get_string(data, "timestamp");
    
    return t;
}

// Parse price levels from array
std::vector<PriceLevel> parse_levels(const rapidjson::Value& arr) {
    std::vector<PriceLevel> levels;
    if (!arr.IsArray()) return levels;
    
    levels.reserve(arr.Size());
    for (const auto& item : arr.GetArray()) {
        if (item.IsObject()) {
            PriceLevel level;
            level.price = get_double(item, "price");
            level.quantity = get_double(item, "qty");
            levels.push_back(level);
        }
    }
    return levels;
}

// Parse order book
OrderBook parse_book(const rapidjson::Value& data, const std::string& symbol) {
    OrderBook book;
    book.symbol = symbol;
    
    if (data.HasMember("bids") && data["bids"].IsArray()) {
        book.bids = parse_levels(data["bids"]);
    }
    if (data.HasMember("asks") && data["asks"].IsArray()) {
        book.asks = parse_levels(data["asks"]);
    }
    if (data.HasMember("checksum") && data["checksum"].IsUint()) {
        book.checksum = data["checksum"].GetUint();
    }
    
    return book;
}

// Parse OHLC candle
OHLC parse_ohlc(const rapidjson::Value& data, const std::string& symbol) {
    OHLC ohlc;
    ohlc.symbol = symbol;
    ohlc.open = get_double(data, "open");
    ohlc.high = get_double(data, "high");
    ohlc.low = get_double(data, "low");
    ohlc.close = get_double(data, "close");
    ohlc.volume = get_double(data, "volume");
    
    if (data.HasMember("timestamp") && data["timestamp"].IsInt64()) {
        ohlc.timestamp = data["timestamp"].GetInt64();
    }
    if (data.HasMember("interval") && data["interval"].IsInt()) {
        ohlc.interval = data["interval"].GetInt();
    }
    
    return ohlc;
}

// Parse order (private channel)
Order parse_order(const rapidjson::Value& data) {
    Order order;
    order.order_id = get_string(data, "order_id");
    order.symbol = get_string(data, "symbol");
    order.price = get_double(data, "price");
    order.quantity = get_double(data, "qty");
    order.filled = get_double(data, "filled", 0.0);
    order.remaining = get_double(data, "remaining", order.quantity);
    order.timestamp = get_string(data, "timestamp");
    order.userref = get_string(data, "userref");
    
    std::string side_str = get_string(data, "side");
    order.side = (side_str == "sell") ? Side::Sell : Side::Buy;
    
    std::string type_str = get_string(data, "type");
    order.type = (type_str == "market") ? OrderType::Market : OrderType::Limit;
    
    std::string status_str = get_string(data, "status");
    // Map status string to enum using lookup table
    static const std::unordered_map<std::string, OrderStatus> status_map = {
        {"pending", OrderStatus::Pending},
        {"open", OrderStatus::Open},
        {"closed", OrderStatus::Closed},
        {"cancelled", OrderStatus::Cancelled},
        {"expired", OrderStatus::Expired}
    };
    auto it = status_map.find(status_str);
    order.status = (it != status_map.end()) ? it->second : OrderStatus::Open;
    
    return order;
}

// Parse own trade (private channel)
OwnTrade parse_own_trade(const rapidjson::Value& data) {
    OwnTrade trade;
    trade.trade_id = get_string(data, "trade_id");
    trade.order_id = get_string(data, "order_id");
    trade.symbol = get_string(data, "symbol");
    trade.price = get_double(data, "price");
    trade.quantity = get_double(data, "qty");
    trade.fee = get_double(data, "fee", 0.0);
    trade.fee_currency = get_string(data, "fee_currency");
    trade.timestamp = get_string(data, "timestamp");
    
    std::string side_str = get_string(data, "side");
    trade.side = (side_str == "sell") ? Side::Sell : Side::Buy;
    
    return trade;
}

// Parse balances (private channel)
std::unordered_map<std::string, Balance> parse_balances(const rapidjson::Value& data) {
    std::unordered_map<std::string, Balance> balances;
    
    if (!data.IsObject()) return balances;
    
    for (auto it = data.MemberBegin(); it != data.MemberEnd(); ++it) {
        if (!it->name.IsString() || !it->value.IsObject()) continue;
        
        std::string currency = it->name.GetString();
        const auto& bal_obj = it->value;
        
        Balance balance;
        balance.currency = currency;
        balance.available = get_double(bal_obj, "available", 0.0);
        balance.reserved = get_double(bal_obj, "reserved", 0.0);
        balance.total = balance.available + balance.reserved;
        
        balances[currency] = balance;
    }
    
    return balances;
}

} // namespace

// NOLINTNEXTLINE(readability-function-cognitive-complexity) - Complex message parsing requires many conditionals
Message parse_message(const std::string& raw_json) {
    Message msg;
    msg.type = MessageType::Heartbeat;
    msg.data = HeartbeatMsg{};
    
    rapidjson::Document doc;
    if (doc.Parse(raw_json.c_str()).HasParseError()) {
        msg.type = MessageType::Error;
        msg.data = Error{ErrorCode::ParseError, "Failed to parse JSON", raw_json};
        return msg;
    }

    // Validate basic structure – must be a JSON object
    if (!doc.IsObject()) {
        msg.type = MessageType::Error;
        msg.data = Error{ErrorCode::ParseError, "Invalid message format (not an object)", raw_json};
        return msg;
    }
    
    // Check message type
    std::string method = get_string(doc, "method");
    std::string channel = get_string(doc, "channel");

    // If neither method nor channel is present, treat as malformed
    if (method.empty() && channel.empty()) {
        msg.type = MessageType::Error;
        msg.data = Error{ErrorCode::ParseError, "Invalid message format (missing method/channel)", raw_json};
        return msg;
    }
    
    // Handle system messages
    if (method == "subscribe" || method == "unsubscribe") {
        if (doc.HasMember("success") && doc["success"].IsBool()) {
            if (doc["success"].GetBool()) {
                msg.type = (method == "subscribe") ? 
                    MessageType::Subscribed : MessageType::Unsubscribed;
                msg.data = (method == "subscribe") ? 
                    MessageData{SubscribedMsg{}} : MessageData{UnsubscribedMsg{}};
            } else {
                // Check if error is rate limit or authentication related
                std::string error_msg = get_string(doc, "error", "Subscription failed");
                ErrorCode error_code = ErrorCode::InvalidSymbol;
                
                // Detect error type (case-insensitive)
                std::string error_lower = error_msg;
                std::transform(error_lower.begin(), error_lower.end(), error_lower.begin(),
                              [](unsigned char c) { return std::tolower(c); });
                
                // Check for authentication errors
                const bool has_auth_keyword = (error_lower.find("auth") != std::string::npos ||
                    error_lower.find("unauthorized") != std::string::npos ||
                    error_lower.find("api key") != std::string::npos ||
                    error_lower.find("api secret") != std::string::npos);
                const bool has_token_issue = (error_lower.find("invalid token") != std::string::npos ||
                    (error_lower.find("token") != std::string::npos && 
                     (error_lower.find("invalid") != std::string::npos || 
                      error_lower.find("expired") != std::string::npos ||
                      error_lower.find("missing") != std::string::npos)));
                const bool has_auth_status = (error_lower.find("401") != std::string::npos ||
                    error_lower.find("403") != std::string::npos);
                
                if (has_auth_keyword || has_token_issue || has_auth_status) {
                    error_code = ErrorCode::AuthenticationFailed;
                }
                // Check for rate limit errors
                else if (error_lower.find("rate limit") != std::string::npos ||
                    error_lower.find("ratelimit") != std::string::npos ||
                    error_lower.find("too many") != std::string::npos ||
                    error_lower.find("429") != std::string::npos) {
                    error_code = ErrorCode::RateLimited;
                }
                
                msg.type = MessageType::Error;
                msg.data = Error{
                    error_code,
                    error_msg,
                    raw_json
                };
            }
        }
        return msg;
    }
    
    // Handle heartbeat
    if (channel == "heartbeat") {
        msg.type = MessageType::Heartbeat;
        msg.data = HeartbeatMsg{};
        return msg;
    }
    
    // Handle data messages
    if (!doc.HasMember("data") || !doc["data"].IsArray() || 
        doc["data"].Empty()) {
        return msg;
    }
    
    const rapidjson::Value& data_arr = doc["data"];
    if (data_arr.Size() == 0) {
        return msg;  // Empty data array
    }
    const rapidjson::Value& data = data_arr[0];
    std::string symbol = get_string(data, "symbol");
    
    // Extract sequence number for gap detection (if present)
    msg.channel = channel;
    msg.symbol = symbol;
    if (doc.HasMember("sequence") && doc["sequence"].IsUint64()) {
        msg.sequence = doc["sequence"].GetUint64();
        msg.has_sequence = true;
    }
    
    // Route by channel type - store in variant
    if (channel == "ticker") {
        bool ok = true;
        auto ticker = parse_ticker(data, symbol, ok);
        if (!ok) {
            msg.type = MessageType::Error;
            msg.data = Error{ErrorCode::ParseError, "Ticker contains non-numeric fields", raw_json};
        } else {
            msg.type = MessageType::Ticker;
            msg.data = ticker;
        }
    }
    else if (channel == "trade") {
        msg.type = MessageType::Trade;
        msg.data = parse_trade(data, symbol);
    }
    else if (channel == "book") {
        msg.type = MessageType::Book;
        msg.data = parse_book(data, symbol);
    }
    else if (channel == "ohlc") {
        msg.type = MessageType::OHLC;
        msg.data = parse_ohlc(data, symbol);
    }
    else if (channel == "openOrders") {
        // Private channel: open orders
        msg.type = MessageType::Order;
        msg.data = parse_order(data);
    }
    else if (channel == "ownTrades") {
        // Private channel: own trades
        msg.type = MessageType::OwnTrade;
        msg.data = parse_own_trade(data);
    }
    else if (channel == "balances") {
        // Private channel: balances (data is the balances object directly)
        msg.type = MessageType::Balance;
        if (data.IsObject()) {
            msg.data = parse_balances(data);
        } else {
            // If data is not an object, try parsing from root
            if (doc.HasMember("data") && doc["data"].IsObject()) {
                msg.data = parse_balances(doc["data"]);
            }
        }
    }
    
    return msg;
}

// Helper to build subscription messages (reduces code duplication)
namespace {
    std::string build_subscription_message(const char* method, Channel channel,
                                           const std::vector<std::string>& symbols,
                                           int depth = 0) {
        rapidjson::StringBuffer sb;
        rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
        
        writer.StartObject();
        writer.Key("method");
        writer.String(method);
        
        writer.Key("params");
        writer.StartObject();
        
        writer.Key("channel");
        writer.String(to_string(channel));
        
        // Private channels (ownTrades, openOrders, balances) don't need symbols
        bool is_private = (channel == Channel::OwnTrades || 
                          channel == Channel::OpenOrders || 
                          channel == Channel::Balances);
        
        if (!is_private && !symbols.empty()) {
            writer.Key("symbol");
            writer.StartArray();
            for (const auto& sym : symbols) {
                writer.String(sym.c_str());
            }
            writer.EndArray();
        }
        
        // Add depth for book channel (subscribe only)
        if (std::strcmp(method, "subscribe") == 0 && channel == Channel::Book && depth > 0) {
            writer.Key("depth");
            writer.Int(depth);
        }
        
        writer.EndObject();  // params
        writer.EndObject();  // root
        
        return sb.GetString();
    }
}

std::string build_subscribe_message(Channel channel, 
                                     const std::vector<std::string>& symbols,
                                     int depth) {
    return build_subscription_message("subscribe", channel, symbols, depth);
}

std::string build_unsubscribe_message(Channel channel,
                                       const std::vector<std::string>& symbols) {
    return build_subscription_message("unsubscribe", channel, symbols, 0);
}

} // namespace kraken

