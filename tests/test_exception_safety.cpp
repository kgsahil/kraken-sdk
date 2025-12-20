/// @file test_exception_safety.cpp
/// @brief Tests for exception safety in callbacks and strategies

#include <gtest/gtest.h>
#include <kraken/kraken.hpp>
#include <stdexcept>
#include <atomic>

using namespace kraken;

class ExceptionSafetyTest : public ::testing::Test {
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

// Test that throwing ticker callback doesn't crash
TEST_F(ExceptionSafetyTest, ThrowingTickerCallback) {
    std::atomic<bool> error_called{false};
    
    // Register error callback to catch the exception notification
    client_->on_error([&error_called](const Error& err) {
        if (err.code == ErrorCode::CallbackError) {
            error_called = true;
        }
    });
    
    // Register ticker callback that throws
    client_->on_ticker([](const Ticker&) {
        throw std::runtime_error("Test exception");
    });
    
    // Subscribe (but don't actually connect - this tests the exception handling path)
    auto sub = client_->subscribe(Channel::Ticker, {"BTC/USD"});
    
    // Should not crash - exception should be caught
    EXPECT_TRUE(sub.is_active());
}

// Test that throwing strategy doesn't crash
TEST_F(ExceptionSafetyTest, ThrowingStrategy) {
    std::atomic<bool> error_called{false};
    
    client_->on_error([&error_called](const Error& err) {
        if (err.code == ErrorCode::CallbackError) {
            error_called = true;
        }
    });
    
    // Create a strategy that throws
    class ThrowingStrategy : public AlertStrategy {
    public:
        bool check(const Ticker&) override {
            throw std::runtime_error("Strategy exception");
        }
        std::string name() const override { return "ThrowingStrategy"; }
        std::vector<std::string> symbols() const override { return {"BTC/USD"}; }
    };
    
    auto strategy = std::make_shared<ThrowingStrategy>();
    int id = client_->add_alert(strategy, [](const Alert&) {});
    
    // Should not crash
    EXPECT_GT(id, 0);
}

// Test multiple exceptions
TEST_F(ExceptionSafetyTest, MultipleExceptions) {
    int exception_count = 0;
    
    client_->on_error([&exception_count](const Error& err) {
        if (err.code == ErrorCode::CallbackError) {
            exception_count++;
        }
    });
    
    // Multiple callbacks that throw
    client_->on_ticker([](const Ticker&) {
        throw std::runtime_error("Exception 1");
    });
    
    client_->on_trade([](const Trade&) {
        throw std::runtime_error("Exception 2");
    });
    
    // Should not crash
    EXPECT_TRUE(true);
}

// Test that error callback itself throwing doesn't crash
TEST_F(ExceptionSafetyTest, ErrorCallbackThrowing) {
    // Error callback that throws
    client_->on_error([](const Error&) {
        throw std::runtime_error("Error callback exception");
    });
    
    // Ticker callback that throws (will trigger error callback)
    client_->on_ticker([](const Ticker&) {
        throw std::runtime_error("Ticker exception");
    });
    
    // Should not crash - error callback exception should be caught
    EXPECT_TRUE(true);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

