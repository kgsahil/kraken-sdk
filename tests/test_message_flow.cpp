/// @file test_message_flow.cpp
/// @brief Integration tests for complete message flow (parse -> queue -> dispatch)

#include <gtest/gtest.h>
#include "../src/parser.hpp"
#include "../src/client_impl.hpp"
#include <kraken/kraken.hpp>
#include <thread>
#include <chrono>
#include <atomic>
#include <vector>
#include <mutex>

using namespace kraken;

class MessageFlowTest : public ::testing::Test {
protected:
    void SetUp() override {
        client_ = std::make_unique<KrakenClient>();
    }
    
    void TearDown() override {
        if (client_ && client_->is_running()) {
            client_->stop();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    
    std::unique_ptr<KrakenClient> client_;
};

// Test parser -> message creation flow
TEST_F(MessageFlowTest, ParseToMessageFlow) {
    // Valid ticker JSON
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

// Test multiple message types parsing
TEST_F(MessageFlowTest, MultipleMessageTypes) {
    // Ticker
    std::string ticker_json = R"({
        "channel": "ticker",
        "data": [{"symbol": "BTC/USD", "bid": 50000.0, "ask": 50001.0, "last": 50000.5}]
    })";
    Message ticker_msg = parse_message(ticker_json);
    EXPECT_EQ(ticker_msg.type, MessageType::Ticker);
    
    // Trade
    std::string trade_json = R"({
        "channel": "trade",
        "data": [{"symbol": "BTC/USD", "price": 50000.0, "qty": 0.5, "side": "buy"}]
    })";
    Message trade_msg = parse_message(trade_json);
    EXPECT_EQ(trade_msg.type, MessageType::Trade);
    
    // Book
    std::string book_json = R"({
        "channel": "book",
        "data": [{
            "symbol": "BTC/USD",
            "bids": [{"price": 50000.0, "qty": 1.0}],
            "asks": [{"price": 50001.0, "qty": 1.0}]
        }]
    })";
    Message book_msg = parse_message(book_json);
    EXPECT_EQ(book_msg.type, MessageType::Book);
}

// Test error message flow
TEST_F(MessageFlowTest, ErrorMessageFlow) {
    std::string invalid_json = "{ invalid json }";
    
    Message msg = parse_message(invalid_json);
    
    EXPECT_EQ(msg.type, MessageType::Error);
    EXPECT_TRUE(msg.holds<Error>());
    
    const auto& error = msg.get<Error>();
    EXPECT_EQ(error.code, ErrorCode::ParseError);
    EXPECT_FALSE(error.message.empty());
}

// Test subscribe message building flow
TEST_F(MessageFlowTest, SubscribeMessageBuilding) {
    // Build subscribe message
    std::string msg = build_subscribe_message(Channel::Ticker, {"BTC/USD", "ETH/USD"}, 0);
    
    // Parse it back to verify structure
    // (In real flow, this goes to WebSocket)
    EXPECT_FALSE(msg.empty());
    EXPECT_NE(msg.find("subscribe"), std::string::npos);
    EXPECT_NE(msg.find("ticker"), std::string::npos);
    EXPECT_NE(msg.find("BTC/USD"), std::string::npos);
    EXPECT_NE(msg.find("ETH/USD"), std::string::npos);
}

// Test unsubscribe message building
TEST_F(MessageFlowTest, UnsubscribeMessageBuilding) {
    std::string msg = build_unsubscribe_message(Channel::Trade, {"SOL/USD"});
    
    EXPECT_FALSE(msg.empty());
    EXPECT_NE(msg.find("unsubscribe"), std::string::npos);
    EXPECT_NE(msg.find("trade"), std::string::npos);
    EXPECT_NE(msg.find("SOL/USD"), std::string::npos);
}

// Test strategy with parsed ticker
TEST_F(MessageFlowTest, StrategyWithParsedTicker) {
    // Parse a ticker message
    std::string json = R"({
        "channel": "ticker",
        "data": [{
            "symbol": "BTC/USD",
            "bid": 49000.0,
            "ask": 49001.0,
            "last": 51000.0,
            "volume": 1000.0
        }]
    })";
    
    Message msg = parse_message(json);
    EXPECT_EQ(msg.type, MessageType::Ticker);
    
    const auto& ticker = msg.get<Ticker>();
    
    // Create strategy that should trigger
    auto alert = PriceAlert::Builder()
        .symbol("BTC/USD")
        .above(50000.0)
        .build();
    
    // Strategy should trigger (price 51000 > 50000)
    EXPECT_TRUE(alert->check(ticker));
}

// Test order book parsing and structure
TEST_F(MessageFlowTest, OrderBookParsingFlow) {
    std::string json = R"({
        "channel": "book",
        "data": [{
            "symbol": "BTC/USD",
            "bids": [
                {"price": 50000.0, "qty": 1.5},
                {"price": 49999.0, "qty": 2.0}
            ],
            "asks": [
                {"price": 50001.0, "qty": 1.0},
                {"price": 50002.0, "qty": 3.0}
            ],
            "checksum": 12345678
        }]
    })";
    
    Message msg = parse_message(json);
    EXPECT_EQ(msg.type, MessageType::Book);
    EXPECT_TRUE(msg.holds<OrderBook>());
    
    const auto& book = msg.get<OrderBook>();
    EXPECT_EQ(book.symbol, "BTC/USD");
    EXPECT_EQ(book.bids.size(), 2);
    EXPECT_EQ(book.asks.size(), 2);
    EXPECT_DOUBLE_EQ(book.bids[0].price, 50000.0);
    EXPECT_DOUBLE_EQ(book.asks[0].price, 50001.0);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

