/// @file test_offline_replay.cpp
/// @brief Unit tests for offline mode and ReplayEngine API

#include <gtest/gtest.h>
#include <kraken/kraken.hpp>
#include <kraken/strategies/strategies.hpp>
#include <atomic>

using namespace kraken;

class OfflineReplayTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create client with offline mode and no queue for direct dispatch testing
        auto config = ClientConfig::Builder()
            .offline_mode(true)
            .use_queue(false)
            .build();
        client_ = std::make_unique<KrakenClient>(config);
        client_->run();
    }

    void TearDown() override {
        if (client_) {
            client_->stop();
        }
    }

    std::unique_ptr<KrakenClient> client_;
};

TEST_F(OfflineReplayTest, Initialization) {
    EXPECT_FALSE(client_->is_connected());
    auto& engine = client_->get_replay_engine();
    (void)engine; // Ensure we can access it without segfault
}

TEST_F(OfflineReplayTest, InjectedTickerTriggersCallback) {
    std::atomic<int> callback_count{0};
    double last_price = 0.0;

    client_->on_ticker([&](const Ticker& t) {
        callback_count++;
        last_price = t.last;
    });

    auto& engine = client_->get_replay_engine();

    Ticker t;
    t.symbol = "BTC/USD";
    t.last = 50000.0;
    engine.inject_ticker(t);

    EXPECT_EQ(callback_count.load(), 1);
    EXPECT_DOUBLE_EQ(last_price, 50000.0);
}

TEST_F(OfflineReplayTest, InjectedTradeTriggersCallback) {
    std::atomic<int> callback_count{0};
    double last_price = 0.0;

    client_->on_trade([&](const Trade& t) {
        callback_count++;
        last_price = t.price;
    });

    auto& engine = client_->get_replay_engine();

    Trade t;
    t.symbol = "ETH/USD";
    t.price = 3000.0;
    engine.inject_trade(t);

    EXPECT_EQ(callback_count.load(), 1);
    EXPECT_DOUBLE_EQ(last_price, 3000.0);
}

TEST_F(OfflineReplayTest, InjectedTickerTriggersStrategy) {
    std::atomic<int> strategy_triggers{0};

    auto price_alert = PriceAlert::Builder()
        .symbol("BTC/USD")
        .above(50000.0)
        .build();

    client_->add_alert(price_alert, [&](const Alert& alert) {
        strategy_triggers++;
        EXPECT_EQ(alert.strategy_name, "PriceAlert");
    });

    auto& engine = client_->get_replay_engine();

    Ticker t1;
    t1.symbol = "BTC/USD";
    t1.last = 49000.0;
    engine.inject_ticker(t1);

    // Below threshold, no trigger
    EXPECT_EQ(strategy_triggers.load(), 0);

    Ticker t2;
    t2.symbol = "BTC/USD";
    t2.last = 51000.0;
    engine.inject_ticker(t2);

    // Above threshold, should trigger
    EXPECT_EQ(strategy_triggers.load(), 1);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
