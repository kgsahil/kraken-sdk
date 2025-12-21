/// @file test_client.cpp
/// @brief Unit tests for KrakenClient API

#include <gtest/gtest.h>
#include <kraken/kraken.hpp>
#include <thread>
#include <chrono>
#include <atomic>

using namespace kraken;

class ClientTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Use default config
        client_ = std::make_unique<KrakenClient>();
    }
    
    void TearDown() override {
        if (client_ && client_->is_running()) {
            client_->stop();
        }
        // Give threads time to stop
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    std::unique_ptr<KrakenClient> client_;
};

// Test default construction
TEST_F(ClientTest, DefaultConstruction) {
    KrakenClient client;
    EXPECT_FALSE(client.is_connected());
    EXPECT_FALSE(client.is_running());
}

// Test construction with config
TEST_F(ClientTest, ConstructionWithConfig) {
    auto config = ClientConfig::Builder()
        .queue_capacity(32768)
        .reconnect_attempts(5)
        .build();
    
    KrakenClient client(config);
    EXPECT_FALSE(client.is_connected());
}

// Test callback registration
TEST_F(ClientTest, RegisterTickerCallback) {
    bool called = false;
    client_->on_ticker([&called](const Ticker&) {
        called = true;
    });
    
    // Callback registered (won't be called without connection)
    EXPECT_FALSE(called);
}

TEST_F(ClientTest, RegisterTradeCallback) {
    bool called = false;
    client_->on_trade([&called](const Trade&) {
        called = true;
    });
    
    EXPECT_FALSE(called);
}

TEST_F(ClientTest, RegisterBookCallback) {
    bool called = false;
    client_->on_book([&called](const std::string&, const OrderBook&) {
        called = true;
    });
    
    EXPECT_FALSE(called);
}

TEST_F(ClientTest, RegisterErrorCallback) {
    bool called = false;
    client_->on_error([&called](const Error&) {
        called = true;
    });
    
    EXPECT_FALSE(called);
}

TEST_F(ClientTest, RegisterConnectionStateCallback) {
    std::vector<ConnectionState> states;
    client_->on_connection_state([&states](ConnectionState state) {
        states.push_back(state);
    });
    
    // No state changes yet
    EXPECT_TRUE(states.empty());
}

// Test subscription
TEST_F(ClientTest, SubscribeTicker) {
    EXPECT_NO_THROW({
        auto sub = client_->subscribe(Channel::Ticker, {"BTC/USD"});
        EXPECT_TRUE(sub.is_active());
        EXPECT_FALSE(sub.is_paused());
        EXPECT_EQ(sub.channel(), Channel::Ticker);
    });
}

TEST_F(ClientTest, SubscribeBook) {
    EXPECT_NO_THROW({
        auto sub = client_->subscribe_book({"BTC/USD"}, 10);
        EXPECT_TRUE(sub.is_active());
        EXPECT_EQ(sub.channel(), Channel::Book);
    });
}

TEST_F(ClientTest, SubscribeEmptySymbolsThrows) {
    EXPECT_THROW(
        client_->subscribe(Channel::Ticker, {}),
        std::invalid_argument
    );
}

// Test subscription management
TEST_F(ClientTest, SubscriptionPauseResume) {
    auto sub = client_->subscribe(Channel::Ticker, {"BTC/USD"});
    
    EXPECT_TRUE(sub.is_active());
    EXPECT_FALSE(sub.is_paused());
    
    sub.pause();
    EXPECT_TRUE(sub.is_paused());
    
    sub.resume();
    EXPECT_FALSE(sub.is_paused());
}

TEST_F(ClientTest, SubscriptionAddRemoveSymbols) {
    auto sub = client_->subscribe(Channel::Ticker, {"BTC/USD"});
    
    auto symbols = sub.symbols();
    EXPECT_EQ(symbols.size(), 1);
    EXPECT_EQ(symbols[0], "BTC/USD");
    
    sub.add_symbols({"ETH/USD"});
    symbols = sub.symbols();
    EXPECT_EQ(symbols.size(), 2);
    
    sub.remove_symbols({"BTC/USD"});
    symbols = sub.symbols();
    EXPECT_EQ(symbols.size(), 1);
    EXPECT_EQ(symbols[0], "ETH/USD");
}

TEST_F(ClientTest, SubscriptionUnsubscribe) {
    auto sub = client_->subscribe(Channel::Ticker, {"BTC/USD"});
    EXPECT_TRUE(sub.is_active());
    
    sub.unsubscribe();
    EXPECT_FALSE(sub.is_active());
}

// Test strategies
TEST_F(ClientTest, AddAlert) {
    auto alert = PriceAlert::Builder()
        .symbol("BTC/USD")
        .above(50000.0)
        .build();
    
    int id = client_->add_alert(alert, [](const Alert&) {});
    EXPECT_GT(id, 0);
    EXPECT_EQ(client_->alert_count(), 1);
}

TEST_F(ClientTest, RemoveAlert) {
    auto alert = PriceAlert::Builder()
        .symbol("BTC/USD")
        .above(50000.0)
        .build();
    
    int id = client_->add_alert(alert, [](const Alert&) {});
    EXPECT_EQ(client_->alert_count(), 1);
    
    client_->remove_alert(id);
    EXPECT_EQ(client_->alert_count(), 0);
}

TEST_F(ClientTest, MultipleAlerts) {
    auto alert1 = PriceAlert::Builder().symbol("BTC/USD").above(50000.0).build();
    auto alert2 = VolumeSpike::Builder().symbols({"ETH/USD"}).multiplier(2.0).build();
    
    int id1 = client_->add_alert(alert1, [](const Alert&) {});
    int id2 = client_->add_alert(alert2, [](const Alert&) {});
    
    EXPECT_NE(id1, id2);
    EXPECT_EQ(client_->alert_count(), 2);
    
    client_->remove_alert(id1);
    EXPECT_EQ(client_->alert_count(), 1);
}

// Test metrics
TEST_F(ClientTest, GetMetrics) {
    auto metrics = client_->get_metrics();
    
    EXPECT_EQ(metrics.messages_received, 0);
    EXPECT_EQ(metrics.messages_processed, 0);
    EXPECT_EQ(metrics.messages_dropped, 0);
    EXPECT_EQ(metrics.queue_depth, 0);
    EXPECT_EQ(metrics.connection_state, ConnectionState::Disconnected);
}

// Test connection state
TEST_F(ClientTest, InitiallyDisconnected) {
    EXPECT_FALSE(client_->is_connected());
    EXPECT_EQ(client_->connection_state(), ConnectionState::Disconnected);
}

// Test move semantics
TEST_F(ClientTest, MoveConstruction) {
    KrakenClient client1;
    KrakenClient client2 = std::move(client1);
    // Should not crash - moved-from client is in valid state
    EXPECT_FALSE(client2.is_connected());
}

// Test that client is not copyable (compile-time check)
// This test just verifies move works correctly
TEST_F(ClientTest, MoveWorks) {
    KrakenClient client1;
    KrakenClient client2 = std::move(client1);
    EXPECT_FALSE(client2.is_connected());
    EXPECT_FALSE(client2.is_running());
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

