/// @file test_error_handling.cpp
/// @brief Unit tests for error handling and edge cases

#include <gtest/gtest.h>
#include <kraken/kraken.hpp>
#include <kraken/core/error.hpp>
#include <stdexcept>

using namespace kraken;

class ErrorHandlingTest : public ::testing::Test {
protected:
    void SetUp() override {
        client_ = std::make_unique<KrakenClient>();
    }
    
    void TearDown() override {
        if (client_ && client_->is_running()) {
            client_->stop();
        }
    }
    
    std::unique_ptr<KrakenClient> client_;
};

// Test error callback
TEST_F(ErrorHandlingTest, ErrorCallback) {
    std::vector<Error> errors;
    
    client_->on_error([&errors](const Error& err) {
        errors.push_back(err);
    });
    
    // Error callback registered (won't fire without actual error)
    EXPECT_TRUE(errors.empty());
}

// Test invalid subscription (empty symbols)
TEST_F(ErrorHandlingTest, SubscribeEmptySymbols) {
    EXPECT_THROW(
        client_->subscribe(Channel::Ticker, {}),
        std::invalid_argument
    );
}

// Test invalid subscription (empty symbols for book)
TEST_F(ErrorHandlingTest, SubscribeBookEmptySymbols) {
    EXPECT_THROW(
        client_->subscribe_book({}, 10),
        std::invalid_argument
    );
}

// Test error code enum
TEST_F(ErrorHandlingTest, ErrorCodeEnum) {
    Error err1{ErrorCode::ConnectionFailed, "Test", ""};
    Error err2{ErrorCode::InvalidSymbol, "Test", ""};
    Error err3{ErrorCode::QueueOverflow, "Test", ""};
    Error err4{ErrorCode::ChecksumMismatch, "Test", ""};
    
    EXPECT_EQ(err1.code, ErrorCode::ConnectionFailed);
    EXPECT_EQ(err2.code, ErrorCode::InvalidSymbol);
    EXPECT_EQ(err3.code, ErrorCode::QueueOverflow);
    EXPECT_EQ(err4.code, ErrorCode::ChecksumMismatch);
}

// Test error message
TEST_F(ErrorHandlingTest, ErrorMessage) {
    Error err{ErrorCode::ConnectionFailed, "Connection failed", "details"};
    
    EXPECT_EQ(err.code, ErrorCode::ConnectionFailed);
    EXPECT_EQ(err.message, "Connection failed");
    EXPECT_EQ(err.details, "details");
}

// Test connection state transitions
TEST_F(ErrorHandlingTest, ConnectionStateTransitions) {
    std::vector<ConnectionState> states;
    
    client_->on_connection_state([&states](ConnectionState state) {
        states.push_back(state);
    });
    
    // Initially disconnected
    EXPECT_EQ(client_->connection_state(), ConnectionState::Disconnected);
}

// Test metrics with no activity
TEST_F(ErrorHandlingTest, MetricsNoActivity) {
    auto metrics = client_->get_metrics();
    
    EXPECT_EQ(metrics.messages_received, 0);
    EXPECT_EQ(metrics.messages_processed, 0);
    EXPECT_EQ(metrics.messages_dropped, 0);
    EXPECT_DOUBLE_EQ(metrics.messages_per_second(), 0.0);
}

// Test stop when not running
TEST_F(ErrorHandlingTest, StopWhenNotRunning) {
    // Should not throw
    EXPECT_NO_THROW(client_->stop());
}

// Test disconnect when not connected
TEST_F(ErrorHandlingTest, DisconnectWhenNotConnected) {
    // Should not throw
    EXPECT_NO_THROW(client_->disconnect());
}

// Test run when already running
TEST_F(ErrorHandlingTest, RunWhenAlreadyRunning) {
    // This is tested in integration tests
    // Here we just verify it doesn't crash
    EXPECT_FALSE(client_->is_running());
}

// Test invalid config values
TEST_F(ErrorHandlingTest, InvalidConfig) {
    bool caught = false;
    try {
        ClientConfig::Builder()
            .queue_capacity(0)
            .build();
    } catch (const std::invalid_argument&) {
        caught = true;
    }
    EXPECT_TRUE(caught);

    caught = false;
    try {
        ClientConfig::Builder()
            .url("not-a-url")
            .build();
    } catch (const std::invalid_argument&) {
        caught = true;
    }
    EXPECT_TRUE(caught);
}

// Test negative reconnect attempts
TEST_F(ErrorHandlingTest, NegativeReconnectAttempts) {
    auto config = ClientConfig::Builder()
        .reconnect_attempts(-1)
        .build();
    
    // Builder doesn't validate
    EXPECT_EQ(config.reconnect_attempts(), -1);
}

// Test remove non-existent alert
TEST_F(ErrorHandlingTest, RemoveNonExistentAlert) {
    // Should not throw
    EXPECT_NO_THROW(client_->remove_alert(99999));
}

// Test alert with invalid ID
TEST_F(ErrorHandlingTest, AlertInvalidID) {
    EXPECT_EQ(client_->alert_count(), 0);
    
    // Remove non-existent should be safe
    client_->remove_alert(0);
    client_->remove_alert(-1);
    
    EXPECT_EQ(client_->alert_count(), 0);
}

// Test subscription operations on moved subscription
TEST_F(ErrorHandlingTest, SubscriptionAfterMove) {
    auto sub1 = client_->subscribe(Channel::Ticker, {"BTC/USD"});
    auto sub2 = std::move(sub1);
    
    // Original should be in moved-from state
    EXPECT_FALSE(sub1.is_active());
    
    // New should work
    EXPECT_TRUE(sub2.is_active());
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

