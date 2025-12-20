/// @file test_subscription.cpp
/// @brief Unit tests for subscription management

#include <gtest/gtest.h>
#include <kraken/kraken.hpp>
#include <vector>

using namespace kraken;

class SubscriptionTest : public ::testing::Test {
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

// Test basic subscription
TEST_F(SubscriptionTest, CreateSubscription) {
    auto sub = client_->subscribe(Channel::Ticker, {"BTC/USD"});
    
    EXPECT_TRUE(sub.is_active());
    EXPECT_FALSE(sub.is_paused());
    EXPECT_EQ(sub.channel(), Channel::Ticker);
    
    auto symbols = sub.symbols();
    EXPECT_EQ(symbols.size(), 1);
    EXPECT_EQ(symbols[0], "BTC/USD");
}

// Test subscription with multiple symbols
TEST_F(SubscriptionTest, MultipleSymbols) {
    auto sub = client_->subscribe(Channel::Ticker, {"BTC/USD", "ETH/USD", "SOL/USD"});
    
    auto symbols = sub.symbols();
    EXPECT_EQ(symbols.size(), 3);
}

// Test pause
TEST_F(SubscriptionTest, Pause) {
    auto sub = client_->subscribe(Channel::Ticker, {"BTC/USD"});
    EXPECT_FALSE(sub.is_paused());
    
    sub.pause();
    EXPECT_TRUE(sub.is_paused());
    EXPECT_TRUE(sub.is_active());
}

// Test resume
TEST_F(SubscriptionTest, Resume) {
    auto sub = client_->subscribe(Channel::Ticker, {"BTC/USD"});
    
    sub.pause();
    EXPECT_TRUE(sub.is_paused());
    
    sub.resume();
    EXPECT_FALSE(sub.is_paused());
}

// Test pause when already paused
TEST_F(SubscriptionTest, PauseWhenAlreadyPaused) {
    auto sub = client_->subscribe(Channel::Ticker, {"BTC/USD"});
    
    sub.pause();
    sub.pause(); // Should be idempotent
    
    EXPECT_TRUE(sub.is_paused());
}

// Test resume when not paused
TEST_F(SubscriptionTest, ResumeWhenNotPaused) {
    auto sub = client_->subscribe(Channel::Ticker, {"BTC/USD"});
    
    sub.resume(); // Should be idempotent
    
    EXPECT_FALSE(sub.is_paused());
}

// Test unsubscribe
TEST_F(SubscriptionTest, Unsubscribe) {
    auto sub = client_->subscribe(Channel::Ticker, {"BTC/USD"});
    EXPECT_TRUE(sub.is_active());
    
    sub.unsubscribe();
    EXPECT_FALSE(sub.is_active());
    EXPECT_FALSE(sub.is_paused());
}

// Test unsubscribe when already unsubscribed
TEST_F(SubscriptionTest, UnsubscribeWhenAlreadyUnsubscribed) {
    auto sub = client_->subscribe(Channel::Ticker, {"BTC/USD"});
    
    sub.unsubscribe();
    sub.unsubscribe(); // Should be idempotent
    
    EXPECT_FALSE(sub.is_active());
}

// Test add symbols
TEST_F(SubscriptionTest, AddSymbols) {
    auto sub = client_->subscribe(Channel::Ticker, {"BTC/USD"});
    
    sub.add_symbols({"ETH/USD"});
    
    auto symbols = sub.symbols();
    EXPECT_EQ(symbols.size(), 2);
    EXPECT_NE(std::find(symbols.begin(), symbols.end(), "BTC/USD"), symbols.end());
    EXPECT_NE(std::find(symbols.begin(), symbols.end(), "ETH/USD"), symbols.end());
}

// Test add duplicate symbols (should not duplicate)
TEST_F(SubscriptionTest, AddDuplicateSymbols) {
    auto sub = client_->subscribe(Channel::Ticker, {"BTC/USD"});
    
    sub.add_symbols({"BTC/USD", "ETH/USD"});
    
    auto symbols = sub.symbols();
    EXPECT_EQ(symbols.size(), 2); // BTC/USD should not be duplicated
}

// Test remove symbols
TEST_F(SubscriptionTest, RemoveSymbols) {
    auto sub = client_->subscribe(Channel::Ticker, {"BTC/USD", "ETH/USD"});
    
    sub.remove_symbols({"BTC/USD"});
    
    auto symbols = sub.symbols();
    EXPECT_EQ(symbols.size(), 1);
    EXPECT_EQ(symbols[0], "ETH/USD");
}

// Test remove non-existent symbol
TEST_F(SubscriptionTest, RemoveNonExistentSymbol) {
    auto sub = client_->subscribe(Channel::Ticker, {"BTC/USD"});
    
    sub.remove_symbols({"ETH/USD"}); // Not in subscription
    
    auto symbols = sub.symbols();
    EXPECT_EQ(symbols.size(), 1);
    EXPECT_EQ(symbols[0], "BTC/USD");
}

// Test add/remove when paused
TEST_F(SubscriptionTest, AddRemoveWhenPaused) {
    auto sub = client_->subscribe(Channel::Ticker, {"BTC/USD"});
    sub.pause();
    
    // Should still work
    sub.add_symbols({"ETH/USD"});
    EXPECT_EQ(sub.symbols().size(), 2);
    
    sub.remove_symbols({"BTC/USD"});
    EXPECT_EQ(sub.symbols().size(), 1);
}

// Test add/remove when unsubscribed
TEST_F(SubscriptionTest, AddRemoveWhenUnsubscribed) {
    auto sub = client_->subscribe(Channel::Ticker, {"BTC/USD"});
    sub.unsubscribe();
    
    // After unsubscribe, subscription is inactive
    EXPECT_FALSE(sub.is_active());
    
    // Add/remove operations should not modify symbols when inactive
    sub.add_symbols({"ETH/USD"});
    auto symbols = sub.symbols();
    // Should remain unchanged (still has BTC/USD)
    EXPECT_EQ(symbols.size(), 1);
    EXPECT_EQ(symbols[0], "BTC/USD");
    
    // Remove should also not work when inactive
    sub.remove_symbols({"BTC/USD"});
    symbols = sub.symbols();
    // Should still have BTC/USD (remove was ignored)
    EXPECT_EQ(symbols.size(), 1);
}

// Test subscription ID
TEST_F(SubscriptionTest, SubscriptionID) {
    auto sub1 = client_->subscribe(Channel::Ticker, {"BTC/USD"});
    auto sub2 = client_->subscribe(Channel::Ticker, {"ETH/USD"});
    
    EXPECT_NE(sub1.id(), sub2.id());
    EXPECT_GT(sub1.id(), 0);
    EXPECT_GT(sub2.id(), 0);
}

// Test book subscription
TEST_F(SubscriptionTest, BookSubscription) {
    auto sub = client_->subscribe_book({"BTC/USD"}, 25);
    
    EXPECT_EQ(sub.channel(), Channel::Book);
    EXPECT_TRUE(sub.is_active());
}

// Test multiple subscriptions
TEST_F(SubscriptionTest, MultipleSubscriptions) {
    auto sub1 = client_->subscribe(Channel::Ticker, {"BTC/USD"});
    auto sub2 = client_->subscribe(Channel::Trade, {"ETH/USD"});
    auto sub3 = client_->subscribe_book({"SOL/USD"}, 10);
    
    EXPECT_EQ(sub1.channel(), Channel::Ticker);
    EXPECT_EQ(sub2.channel(), Channel::Trade);
    EXPECT_EQ(sub3.channel(), Channel::Book);
    
    EXPECT_NE(sub1.id(), sub2.id());
    EXPECT_NE(sub2.id(), sub3.id());
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

