/// @file test_connection.cpp
/// @brief Unit tests for WebSocket connection handling

#include <gtest/gtest.h>
#include "../src/internal/connection.hpp"
#include <kraken/core/error.hpp>
#include <thread>
#include <chrono>

using namespace kraken;

class ConnectionTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Use a test URL (will fail but tests error handling)
        conn_ = std::make_unique<Connection>("wss://invalid-host-that-does-not-exist.com/v2");
    }
    
    void TearDown() override {
        if (conn_) {
            conn_->close();
        }
    }
    
    std::unique_ptr<Connection> conn_;
};

TEST_F(ConnectionTest, ParseURL) {
    Connection c("wss://ws.kraken.com/v2");
    // Should not throw
}

TEST_F(ConnectionTest, ParseURLWithPort) {
    Connection c("wss://ws.kraken.com:443/v2");
    // Should not throw
}

TEST_F(ConnectionTest, ParseURLWithoutPath) {
    Connection c("wss://ws.kraken.com");
    // Should not throw
}

TEST_F(ConnectionTest, InvalidURLThrows) {
    EXPECT_THROW(Connection("not-a-url"), std::invalid_argument);
    EXPECT_THROW(Connection("http://ws.kraken.com"), std::invalid_argument);
}

TEST_F(ConnectionTest, InitiallyNotOpen) {
    EXPECT_FALSE(conn_->is_open());
}

TEST_F(ConnectionTest, CloseWhenNotOpen) {
    // Should not throw
    conn_->close();
    EXPECT_FALSE(conn_->is_open());
}

TEST_F(ConnectionTest, SendWhenNotConnectedThrows) {
    EXPECT_THROW(conn_->send("test"), ConnectionError);
}

TEST_F(ConnectionTest, ReceiveWhenNotConnectedReturnsEmpty) {
    std::string msg = conn_->receive();
    EXPECT_TRUE(msg.empty());
}

// Note: Actual connection tests require network access
// These are tested in integration tests

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

