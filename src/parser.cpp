#include "parser.hpp"

#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

#include <stdexcept>
#include <cstring>

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

// Parse ticker from Kraken v2 format
Ticker parse_ticker(const rapidjson::Value& data, const std::string& symbol) {
    Ticker t;
    t.symbol = symbol;
    t.bid = get_double(data, "bid");
    t.ask = get_double(data, "ask");
    t.last = get_double(data, "last");
    t.volume_24h = get_double(data, "volume");
    t.high_24h = get_double(data, "high");
    t.low_24h = get_double(data, "low");
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
    for (auto& item : arr.GetArray()) {
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

} // namespace

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
    
    // Check message type
    std::string method = get_string(doc, "method");
    std::string channel = get_string(doc, "channel");
    
    // Handle system messages
    if (method == "subscribe" || method == "unsubscribe") {
        if (doc.HasMember("success") && doc["success"].IsBool()) {
            if (doc["success"].GetBool()) {
                msg.type = (method == "subscribe") ? 
                    MessageType::Subscribed : MessageType::Unsubscribed;
                msg.data = (method == "subscribe") ? 
                    MessageData{SubscribedMsg{}} : MessageData{UnsubscribedMsg{}};
            } else {
                msg.type = MessageType::Error;
                msg.data = Error{
                    ErrorCode::InvalidSymbol,
                    get_string(doc, "error", "Subscription failed"),
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
    
    // Route by channel type - store in variant
    if (channel == "ticker") {
        msg.type = MessageType::Ticker;
        msg.data = parse_ticker(data, symbol);
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
        
        writer.Key("symbol");
        writer.StartArray();
        for (const auto& sym : symbols) {
            writer.String(sym.c_str());
        }
        writer.EndArray();
        
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

