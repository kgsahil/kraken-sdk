/// @file test_strategies.cpp
/// @brief Unit tests for trading strategies

#include <gtest/gtest.h>
#include <kraken/strategies.hpp>
#include <thread>
#include <chrono>

using namespace kraken;

class PriceAlertTest : public ::testing::Test {
protected:
    Ticker make_ticker(const std::string& symbol, double price) {
        Ticker t;
        t.symbol = symbol;
        t.last = price;
        t.bid = price - 0.5;
        t.ask = price + 0.5;
        return t;
    }
};

TEST_F(PriceAlertTest, TriggersWhenAboveThreshold) {
    auto alert = PriceAlert::Builder()
        .symbol("BTC/USD")
        .above(50000.0)
        .build();
    
    // Below threshold - should not trigger
    EXPECT_FALSE(alert->check(make_ticker("BTC/USD", 49000.0)));
    
    // Above threshold - should trigger
    EXPECT_TRUE(alert->check(make_ticker("BTC/USD", 51000.0)));
    
    // Already fired - should not trigger again
    EXPECT_FALSE(alert->check(make_ticker("BTC/USD", 52000.0)));
}

TEST_F(PriceAlertTest, TriggersWhenBelowThreshold) {
    auto alert = PriceAlert::Builder()
        .symbol("BTC/USD")
        .below(40000.0)
        .build();
    
    // Above threshold - should not trigger
    EXPECT_FALSE(alert->check(make_ticker("BTC/USD", 45000.0)));
    
    // Below threshold - should trigger
    EXPECT_TRUE(alert->check(make_ticker("BTC/USD", 39000.0)));
}

TEST_F(PriceAlertTest, ResetAllowsRetrigger) {
    auto alert = PriceAlert::Builder()
        .symbol("BTC/USD")
        .above(50000.0)
        .build();
    
    EXPECT_TRUE(alert->check(make_ticker("BTC/USD", 51000.0)));
    EXPECT_FALSE(alert->check(make_ticker("BTC/USD", 52000.0)));
    
    alert->reset();
    
    EXPECT_TRUE(alert->check(make_ticker("BTC/USD", 53000.0)));
}

TEST_F(PriceAlertTest, RecurringAlerts) {
    auto alert = PriceAlert::Builder()
        .symbol("BTC/USD")
        .above(50000.0)
        .recurring(true)
        .build();
    
    // First trigger
    EXPECT_TRUE(alert->check(make_ticker("BTC/USD", 51000.0)));
    EXPECT_TRUE(alert->has_fired());
    EXPECT_EQ(alert->fire_count(), 1);
    
    // Should trigger again (recurring)
    EXPECT_TRUE(alert->check(make_ticker("BTC/USD", 52000.0)));
    EXPECT_EQ(alert->fire_count(), 2);
    
    // Should trigger again
    EXPECT_TRUE(alert->check(make_ticker("BTC/USD", 53000.0)));
    EXPECT_EQ(alert->fire_count(), 3);
}

TEST_F(PriceAlertTest, CooldownPreventsSpam) {
    auto alert = PriceAlert::Builder()
        .symbol("BTC/USD")
        .above(50000.0)
        .recurring(true)
        .cooldown(std::chrono::milliseconds(100))
        .build();
    
    // First trigger
    EXPECT_TRUE(alert->check(make_ticker("BTC/USD", 51000.0)));
    
    // Immediately after - should be in cooldown
    EXPECT_FALSE(alert->check(make_ticker("BTC/USD", 52000.0)));
    
    // Wait for cooldown
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    
    // Should trigger again after cooldown
    EXPECT_TRUE(alert->check(make_ticker("BTC/USD", 53000.0)));
}

TEST_F(PriceAlertTest, BetterMessagesWithPriceChange) {
    auto alert = PriceAlert::Builder()
        .symbol("BTC/USD")
        .above(50000.0)
        .build();
    
    // First check (no previous price)
    EXPECT_TRUE(alert->check(make_ticker("BTC/USD", 51000.0)));
    std::string msg = alert->last_message();
    EXPECT_NE(msg.find("Price above"), std::string::npos);
    
    // Reset and check again with previous price
    alert->reset();
    alert->check(make_ticker("BTC/USD", 49000.0));  // Set previous price
    EXPECT_TRUE(alert->check(make_ticker("BTC/USD", 51000.0)));
    msg = alert->last_message();
    // Should include price change info
    EXPECT_NE(msg.find("was $"), std::string::npos);
    EXPECT_NE(msg.find("change"), std::string::npos);
}

TEST_F(PriceAlertTest, IgnoresOtherSymbols) {
    auto alert = PriceAlert::Builder()
        .symbol("BTC/USD")
        .above(50000.0)
        .build();
    
    auto symbols = alert->symbols();
    EXPECT_EQ(symbols.size(), 1);
    EXPECT_EQ(symbols[0], "BTC/USD");
}

class VolumeSpikeTest : public ::testing::Test {
protected:
    Ticker make_ticker(const std::string& symbol, double volume) {
        Ticker t;
        t.symbol = symbol;
        t.volume_24h = volume;
        t.last = 50000.0;
        return t;
    }
};

TEST_F(VolumeSpikeTest, RequiresEnoughSamples) {
    auto spike = VolumeSpike::Builder()
        .symbols({"BTC/USD"})
        .multiplier(2.0)
        .lookback(10)
        .build();
    
    // Not enough samples
    for (int i = 0; i < 4; ++i) {
        EXPECT_FALSE(spike->check(make_ticker("BTC/USD", 100.0)));
    }
}

TEST_F(VolumeSpikeTest, DetectsSpike) {
    auto spike = VolumeSpike::Builder()
        .symbols({"BTC/USD"})
        .multiplier(2.0)
        .lookback(10)
        .build();
    
    // Build up history with normal volume
    for (int i = 0; i < 10; ++i) {
        spike->check(make_ticker("BTC/USD", 100.0));
    }
    
    // Spike! (3x normal should trigger 2x threshold)
    EXPECT_TRUE(spike->check(make_ticker("BTC/USD", 300.0)));
}

TEST_F(VolumeSpikeTest, NoSpikeWithNormalVolume) {
    auto spike = VolumeSpike::Builder()
        .symbols({"BTC/USD"})
        .multiplier(2.0)
        .lookback(10)
        .build();
    
    // Build history
    for (int i = 0; i < 10; ++i) {
        spike->check(make_ticker("BTC/USD", 100.0));
    }
    
    // Normal volume (1.5x - below 2x threshold)
    EXPECT_FALSE(spike->check(make_ticker("BTC/USD", 150.0)));
}

TEST_F(VolumeSpikeTest, MultipleSymbols) {
    auto spike = VolumeSpike::Builder()
        .symbols({"BTC/USD", "ETH/USD"})
        .multiplier(2.0)
        .lookback(5)
        .build();
    
    // Build history for both symbols
    for (int i = 0; i < 5; ++i) {
        spike->check(make_ticker("BTC/USD", 100.0));
        spike->check(make_ticker("ETH/USD", 50.0));
    }
    
    // Spike on BTC
    EXPECT_TRUE(spike->check(make_ticker("BTC/USD", 250.0)));
    
    // Spike on ETH
    EXPECT_TRUE(spike->check(make_ticker("ETH/USD", 120.0)));
}

TEST_F(VolumeSpikeTest, IgnoresOtherSymbols) {
    auto spike = VolumeSpike::Builder()
        .symbols({"BTC/USD"})
        .multiplier(2.0)
        .lookback(5)
        .build();
    
    // Build history
    for (int i = 0; i < 5; ++i) {
        spike->check(make_ticker("BTC/USD", 100.0));
    }
    
    // Different symbol - should not affect
    EXPECT_FALSE(spike->check(make_ticker("ETH/USD", 1000.0)));
}

class SpreadAlertTest : public ::testing::Test {
protected:
    Ticker make_ticker(double bid, double ask) {
        Ticker t;
        t.symbol = "BTC/USD";
        t.bid = bid;
        t.ask = ask;
        t.last = (bid + ask) / 2.0;
        return t;
    }
};

TEST_F(SpreadAlertTest, TriggersWhenSpreadTooWide) {
    auto alert = SpreadAlert::Builder()
        .symbol("BTC/USD")
        .max_spread(10.0)
        .build();
    
    // Normal spread
    EXPECT_FALSE(alert->check(make_ticker(50000.0, 50005.0)));
    
    // Wide spread
    EXPECT_TRUE(alert->check(make_ticker(50000.0, 50015.0)));
}

TEST_F(SpreadAlertTest, TriggersWhenSpreadTooNarrow) {
    auto alert = SpreadAlert::Builder()
        .symbol("BTC/USD")
        .min_spread(1.0)
        .build();
    
    // Normal spread
    EXPECT_FALSE(alert->check(make_ticker(50000.0, 50005.0)));
    
    // Too narrow (suspicious)
    EXPECT_TRUE(alert->check(make_ticker(50000.0, 50000.5)));
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

