/// @file test_parser.cpp
/// @brief Unit tests for JSON message parsing

#include <gtest/gtest.h>
#include "../src/internal/parser.hpp"
#include "../src/internal/client_impl.hpp"

using namespace kraken;

class ParserTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

// Test valid ticker message
TEST_F(ParserTest, ParseValidTicker) {
    std::string json = R"({
        "channel": "ticker",
        "data": [{
            "symbol": "BTC/USD",
            "bid": 50000.0,
            "ask": 50001.0,
            "last": 50000.5,
            "volume": 1234.56,
            "high": 51000.0,
            "low": 49000.0,
            "timestamp": "2025-01-01T00:00:00Z"
        }]
    })";
    
    Message msg = parse_message(json);
    EXPECT_EQ(msg.type, MessageType::Ticker);
    EXPECT_TRUE(msg.holds<Ticker>());
    
    const auto& ticker = msg.get<Ticker>();
    EXPECT_EQ(ticker.symbol, "BTC/USD");
    EXPECT_DOUBLE_EQ(ticker.bid, 50000.0);
    EXPECT_DOUBLE_EQ(ticker.ask, 50001.0);
    EXPECT_DOUBLE_EQ(ticker.last, 50000.5);
}

// Test valid trade message
TEST_F(ParserTest, ParseValidTrade) {
    std::string json = R"({
        "channel": "trade",
        "data": [{
            "symbol": "BTC/USD",
            "price": 50000.0,
            "qty": 0.5,
            "side": "buy",
            "timestamp": "2025-01-01T00:00:00Z"
        }]
    })";
    
    Message msg = parse_message(json);
    EXPECT_EQ(msg.type, MessageType::Trade);
    EXPECT_TRUE(msg.holds<Trade>());
    
    const auto& trade = msg.get<Trade>();
    EXPECT_EQ(trade.symbol, "BTC/USD");
    EXPECT_DOUBLE_EQ(trade.price, 50000.0);
    EXPECT_DOUBLE_EQ(trade.quantity, 0.5);
    EXPECT_EQ(trade.side, Side::Buy);
}

// Test valid order book message
TEST_F(ParserTest, ParseValidOrderBook) {
    std::string json = R"({
        "channel": "book",
        "data": [{
            "symbol": "BTC/USD",
            "bids": [{"price": 50000.0, "qty": 1.0}],
            "asks": [{"price": 50001.0, "qty": 1.0}],
            "checksum": 12345678
        }]
    })";
    
    Message msg = parse_message(json);
    EXPECT_EQ(msg.type, MessageType::Book);
    EXPECT_TRUE(msg.holds<OrderBook>());
    
    const auto& book = msg.get<OrderBook>();
    EXPECT_EQ(book.symbol, "BTC/USD");
    EXPECT_EQ(book.bids.size(), 1);
    EXPECT_EQ(book.asks.size(), 1);
}

// Test invalid JSON
TEST_F(ParserTest, ParseInvalidJSON) {
    std::string json = "{ invalid json }";
    
    Message msg = parse_message(json);
    EXPECT_EQ(msg.type, MessageType::Error);
    EXPECT_TRUE(msg.holds<Error>());
    
    const auto& error = msg.get<Error>();
    EXPECT_EQ(error.code, ErrorCode::ParseError);
}

// Test empty message
TEST_F(ParserTest, ParseEmptyMessage) {
    Message msg = parse_message("");
    // Empty string should result in parse error or heartbeat
    // Depending on parser implementation
    EXPECT_TRUE(msg.type == MessageType::Heartbeat || 
                msg.type == MessageType::Error);
}

// Test heartbeat message
TEST_F(ParserTest, ParseHeartbeat) {
    std::string json = R"({"channel": "heartbeat"})";
    
    Message msg = parse_message(json);
    EXPECT_EQ(msg.type, MessageType::Heartbeat);
    EXPECT_TRUE(msg.holds<HeartbeatMsg>());
}

// Test subscribe response
TEST_F(ParserTest, ParseSubscribeSuccess) {
    std::string json = R"({
        "method": "subscribe",
        "success": true
    })";
    
    Message msg = parse_message(json);
    EXPECT_EQ(msg.type, MessageType::Subscribed);
    EXPECT_TRUE(msg.holds<SubscribedMsg>());
}

// Test subscribe failure
TEST_F(ParserTest, ParseSubscribeFailure) {
    std::string json = R"({
        "method": "subscribe",
        "success": false,
        "error": "Invalid symbol"
    })";
    
    Message msg = parse_message(json);
    EXPECT_EQ(msg.type, MessageType::Error);
    EXPECT_TRUE(msg.holds<Error>());
    
    const auto& error = msg.get<Error>();
    EXPECT_EQ(error.code, ErrorCode::InvalidSymbol);
}

// Test unsubscribe response
TEST_F(ParserTest, ParseUnsubscribeSuccess) {
    std::string json = R"({
        "method": "unsubscribe",
        "success": true
    })";
    
    Message msg = parse_message(json);
    EXPECT_EQ(msg.type, MessageType::Unsubscribed);
    EXPECT_TRUE(msg.holds<UnsubscribedMsg>());
}

// Test message with missing data field
TEST_F(ParserTest, ParseMessageWithoutData) {
    std::string json = R"({"channel": "ticker"})";
    
    Message msg = parse_message(json);
    EXPECT_EQ(msg.type, MessageType::Heartbeat);
}

// Test message with empty data array
TEST_F(ParserTest, ParseMessageWithEmptyData) {
    std::string json = R"({
        "channel": "ticker",
        "data": []
    })";
    
    Message msg = parse_message(json);
    EXPECT_EQ(msg.type, MessageType::Heartbeat);
}

// Test build subscribe message
TEST_F(ParserTest, BuildSubscribeMessage) {
    std::string msg = build_subscribe_message(Channel::Ticker, {"BTC/USD", "ETH/USD"}, 0);
    
    EXPECT_FALSE(msg.empty());
    EXPECT_NE(msg.find("subscribe"), std::string::npos);
    EXPECT_NE(msg.find("ticker"), std::string::npos);
    EXPECT_NE(msg.find("BTC/USD"), std::string::npos);
    EXPECT_NE(msg.find("ETH/USD"), std::string::npos);
}

// Test build subscribe book message with depth
TEST_F(ParserTest, BuildSubscribeBookMessage) {
    std::string msg = build_subscribe_message(Channel::Book, {"BTC/USD"}, 10);
    
    EXPECT_FALSE(msg.empty());
    EXPECT_NE(msg.find("book"), std::string::npos);
    EXPECT_NE(msg.find("depth"), std::string::npos);
}

// Test build unsubscribe message
TEST_F(ParserTest, BuildUnsubscribeMessage) {
    std::string msg = build_unsubscribe_message(Channel::Ticker, {"BTC/USD"});
    
    EXPECT_FALSE(msg.empty());
    EXPECT_NE(msg.find("unsubscribe"), std::string::npos);
    EXPECT_NE(msg.find("ticker"), std::string::npos);
}

// Test parsing numeric strings
TEST_F(ParserTest, ParseNumericStrings) {
    std::string json = R"({
        "channel": "ticker",
        "data": [{
            "symbol": "BTC/USD",
            "bid": "50000.0",
            "ask": "50001.0",
            "last": "50000.5"
        }]
    })";
    
    Message msg = parse_message(json);
    EXPECT_EQ(msg.type, MessageType::Ticker);
    
    const auto& ticker = msg.get<Ticker>();
    EXPECT_DOUBLE_EQ(ticker.bid, 50000.0);
    EXPECT_DOUBLE_EQ(ticker.ask, 50001.0);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

