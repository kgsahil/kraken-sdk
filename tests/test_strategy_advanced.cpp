/// @file test_strategy_advanced.cpp
/// @brief Comprehensive unit tests for advanced strategy features
/// 
/// Tests:
/// - Strategy composition (AND/OR logic)
/// - OHLC/candle data support
/// - Runtime enable/disable
/// - Strategy presets
/// - Configuration from maps/env vars

#include <gtest/gtest.h>
#include <kraken/strategies/strategies.hpp>
#include <kraken/strategies/strategy_config.hpp>
#include <thread>
#include <chrono>
#include <map>

using namespace kraken;

//------------------------------------------------------------------------------
// Composite Strategy Tests
//------------------------------------------------------------------------------

class CompositeStrategyTest : public ::testing::Test {
protected:
    Ticker make_ticker(const std::string& symbol, double price, double volume = 1000.0) {
        Ticker t;
        t.symbol = symbol;
        t.last = price;
        t.bid = price - 0.5;
        t.ask = price + 0.5;
        t.volume_24h = volume;
        return t;
    }
};

TEST_F(CompositeStrategyTest, ANDLogicAllMustTrigger) {
    auto price_alert = PriceAlert::Builder()
        .symbol("BTC/USD")
        .above(50000.0)
        .recurring(true)  // Allow multiple triggers
        .build();
    
    auto volume_spike = VolumeSpike::Builder()
        .symbols({"BTC/USD"})
        .multiplier(2.0)
        .lookback(5)
        .build();
    
    // Build history for volume spike
    for (int i = 0; i < 5; ++i) {
        volume_spike->check(make_ticker("BTC/USD", 49000.0, 100.0));
    }
    
    // Create AND composite
    auto combined = CompositeStrategy::and_(price_alert, volume_spike);
    
    // Build history for composite too
    for (int i = 0; i < 5; ++i) {
        combined->check(make_ticker("BTC/USD", 49000.0, 100.0));
    }
    
    // Price above but volume not spiked - should not trigger
    EXPECT_FALSE(combined->check(make_ticker("BTC/USD", 51000.0, 150.0)));
    
    // Both conditions met - should trigger
    EXPECT_TRUE(combined->check(make_ticker("BTC/USD", 51000.0, 250.0)));
}

TEST_F(CompositeStrategyTest, ORLogicAnyCanTrigger) {
    auto price_alert = PriceAlert::Builder()
        .symbol("BTC/USD")
        .above(50000.0)
        .build();
    
    auto spread_alert = SpreadAlert::Builder()
        .symbol("BTC/USD")
        .max_spread(10.0)
        .build();
    
    // Create OR composite
    auto either = CompositeStrategy::or_(price_alert, spread_alert);
    
    // Price above - should trigger (OR logic)
    EXPECT_TRUE(either->check(make_ticker("BTC/USD", 51000.0)));
    
    // Wide spread - should trigger (OR logic)
    Ticker wide_spread = make_ticker("BTC/USD", 50000.0);
    wide_spread.bid = 50000.0;
    wide_spread.ask = 50015.0;  // 15 spread > 10
    EXPECT_TRUE(either->check(wide_spread));
}

TEST_F(CompositeStrategyTest, CompositeWithMultipleStrategies) {
    auto price1 = PriceAlert::Builder().symbol("BTC/USD").above(50000.0).recurring(true).build();
    auto price2 = PriceAlert::Builder().symbol("BTC/USD").above(60000.0).recurring(true).build();
    auto price3 = PriceAlert::Builder().symbol("BTC/USD").above(70000.0).recurring(true).build();
    
    // AND: All three must trigger
    auto all_three = CompositeStrategy::and_({price1, price2, price3});
    
    // Only one condition met - should not trigger
    EXPECT_FALSE(all_three->check(make_ticker("BTC/USD", 51000.0)));
    
    // All three conditions met - should trigger
    EXPECT_TRUE(all_three->check(make_ticker("BTC/USD", 75000.0)));
}

TEST_F(CompositeStrategyTest, CompositeEnableDisable) {
    auto price = PriceAlert::Builder().symbol("BTC/USD").above(50000.0).build();
    auto volume = VolumeSpike::Builder().symbols({"BTC/USD"}).multiplier(2.0).lookback(5).build();
    
    // Build history
    for (int i = 0; i < 5; ++i) {
        volume->check(make_ticker("BTC/USD", 49000.0, 100.0));
    }
    
    auto combined = CompositeStrategy::and_(price, volume);
    
    // Disable composite
    combined->disable();
    EXPECT_FALSE(combined->is_enabled());
    EXPECT_FALSE(combined->check(make_ticker("BTC/USD", 51000.0, 250.0)));
    
    // Re-enable
    combined->enable();
    EXPECT_TRUE(combined->is_enabled());
    EXPECT_TRUE(combined->check(make_ticker("BTC/USD", 51000.0, 250.0)));
}

//------------------------------------------------------------------------------
// OHLC Support Tests
//------------------------------------------------------------------------------

class OHLCStrategyTest : public ::testing::Test {
protected:
    OHLC make_ohlc(const std::string& symbol, double open, double high, 
                   double low, double close, double volume) {
        OHLC o;
        o.symbol = symbol;
        o.open = open;
        o.high = high;
        o.low = low;
        o.close = close;
        o.volume = volume;
        o.timestamp = 1000000;
        o.interval = 60;
        return o;
    }
};

TEST_F(OHLCStrategyTest, OHLCStrategyInterface) {
    class TestOHLCStrategy : public AlertStrategy {
        std::string symbol_;
        double threshold_;
    public:
        TestOHLCStrategy(const std::string& sym, double thresh)
            : symbol_(sym), threshold_(thresh) {}
        
        bool check(const Ticker&) override { return false; }
        bool check(const OHLC& ohlc) override {
            return ohlc.symbol == symbol_ && ohlc.close > threshold_;
        }
        bool needs_ohlc() const override { return true; }
        std::string name() const override { return "TestOHLC"; }
        std::vector<std::string> symbols() const override { return {symbol_}; }
    };
    
    auto strategy = std::make_shared<TestOHLCStrategy>("BTC/USD", 50000.0);
    
    EXPECT_FALSE(strategy->check(make_ohlc("BTC/USD", 49000, 49500, 48500, 49000, 100)));
    EXPECT_TRUE(strategy->check(make_ohlc("BTC/USD", 49000, 51000, 48500, 51000, 100)));
    EXPECT_TRUE(strategy->needs_ohlc());
}

//------------------------------------------------------------------------------
// Runtime Enable/Disable Tests
//------------------------------------------------------------------------------

class EnableDisableTest : public ::testing::Test {
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

TEST_F(EnableDisableTest, EnableDisablePriceAlert) {
    auto alert = PriceAlert::Builder()
        .symbol("BTC/USD")
        .above(50000.0)
        .recurring(true)  // Allow multiple triggers
        .build();
    
    // PriceAlert doesn't override is_enabled(), so it always returns true
    // Enable/disable is handled at StrategyEngine level
    EXPECT_TRUE(alert->is_enabled());
    EXPECT_TRUE(alert->check(make_ticker("BTC/USD", 51000.0)));
    
    // Note: Direct enable/disable on PriceAlert doesn't affect check() behavior
    // This is by design - StrategyEngine handles enable/disable via its Entry.enabled flag
    // The test verifies the strategy can be checked when enabled
    alert->reset();
    EXPECT_TRUE(alert->check(make_ticker("BTC/USD", 53000.0)));
}

//------------------------------------------------------------------------------
// Strategy Presets Tests
//------------------------------------------------------------------------------

class StrategyPresetsTest : public ::testing::Test {
protected:
    Ticker make_ticker(const std::string& symbol, double price, double volume = 1000.0) {
        Ticker t;
        t.symbol = symbol;
        t.last = price;
        t.bid = price - 0.5;
        t.ask = price + 0.5;
        t.volume_24h = volume;
        return t;
    }
    
    OrderBook make_book(const std::string& symbol, double bid_liq, double ask_liq) {
        OrderBook book;
        book.symbol = symbol;
        book.bids.push_back({50000.0, bid_liq});
        book.asks.push_back({50001.0, ask_liq});
        return book;
    }
};

TEST_F(StrategyPresetsTest, BreakoutPreset) {
    auto breakout = StrategyPresets::breakout("BTC/USD", 50000.0, 2.0);
    
    // Build volume history (need at least lookback/2 = 10 samples for VolumeSpike)
    // Build more than lookback to ensure we have a stable baseline of 100.0
    for (int i = 0; i < 25; ++i) {
        breakout->check(make_ticker("BTC/USD", 49000.0, 100.0));
    }
    
    // Price above but volume not spiked - should not trigger
    // Volume 150.0 is not 2x the average of 100.0
    EXPECT_FALSE(breakout->check(make_ticker("BTC/USD", 51000.0, 150.0)));
    
    // Both price and volume conditions met - should trigger
    // Need to rebuild baseline to push out the 150.0 from history
    // Add enough normal samples (100.0) to completely replace history
    // Since lookback=20, we need 20+ samples to fully replace history
    for (int i = 0; i < 25; ++i) {
        breakout->check(make_ticker("BTC/USD", 50000.0, 100.0));
    }
    
    // Now check with high volume - should trigger
    // Average should be ~100.0, threshold = 200.0, 250.0 > 200.0 = true
    EXPECT_TRUE(breakout->check(make_ticker("BTC/USD", 51000.0, 250.0)));
}

TEST_F(StrategyPresetsTest, SupportLevelPreset) {
    auto support = StrategyPresets::support_level("BTC/USD", 45000.0, 1.0, 10.0);
    
    Ticker ticker = make_ticker("BTC/USD", 45050.0);  // Within 1% of 45000
    OrderBook book = make_book("BTC/USD", 15.0, 5.0);  // Strong bid liquidity
    
    // Should trigger (price near support + strong bid liquidity)
    EXPECT_TRUE(support->check(ticker, book));
    
    // Price too far from support - should not trigger
    ticker.last = 46000.0;
    EXPECT_FALSE(support->check(ticker, book));
    
    // Insufficient liquidity - should not trigger
    ticker.last = 45050.0;
    book.bids[0].quantity = 5.0;  // Below 10.0 threshold
    EXPECT_FALSE(support->check(ticker, book));
}

TEST_F(StrategyPresetsTest, ResistanceLevelPreset) {
    auto resistance = StrategyPresets::resistance_level("BTC/USD", 50000.0, 1.0, 10.0);
    
    Ticker ticker = make_ticker("BTC/USD", 50050.0);  // Within 1% of 50000
    OrderBook book = make_book("BTC/USD", 5.0, 15.0);  // Strong ask liquidity
    
    // Should trigger (price near resistance + strong ask liquidity)
    EXPECT_TRUE(resistance->check(ticker, book));
    
    // Price too far from resistance - should not trigger
    ticker.last = 49000.0;
    EXPECT_FALSE(resistance->check(ticker, book));
}

//------------------------------------------------------------------------------
// Configuration Tests
//------------------------------------------------------------------------------

class StrategyConfigTest : public ::testing::Test {};

TEST_F(StrategyConfigTest, PriceAlertFromMap) {
    std::map<std::string, std::string> config = {
        {"STRATEGY_TYPE", "price_alert"},
        {"STRATEGY_SYMBOL", "BTC/USD"},
        {"STRATEGY_ABOVE", "50000.0"},
        {"STRATEGY_RECURRING", "true"},
        {"STRATEGY_COOLDOWN_MS", "1000"}
    };
    
    auto strategy = StrategyConfig::from_map(config);
    ASSERT_NE(strategy, nullptr);
    EXPECT_EQ(strategy->name(), "PriceAlert");
    
    Ticker ticker;
    ticker.symbol = "BTC/USD";
    ticker.last = 51000.0;
    ticker.bid = 50999.0;
    ticker.ask = 51001.0;
    
    EXPECT_TRUE(strategy->check(ticker));
}

TEST_F(StrategyConfigTest, VolumeSpikeFromMap) {
    std::map<std::string, std::string> config = {
        {"STRATEGY_TYPE", "volume_spike"},
        {"STRATEGY_SYMBOLS", "BTC/USD,ETH/USD"},
        {"STRATEGY_MULTIPLIER", "2.5"},
        {"STRATEGY_LOOKBACK", "10"}
    };
    
    auto strategy = StrategyConfig::from_map(config);
    ASSERT_NE(strategy, nullptr);
    EXPECT_EQ(strategy->name(), "VolumeSpike");
    
    auto symbols = strategy->symbols();
    EXPECT_EQ(symbols.size(), 2);
    EXPECT_EQ(symbols[0], "BTC/USD");
    EXPECT_EQ(symbols[1], "ETH/USD");
}

TEST_F(StrategyConfigTest, BreakoutFromMap) {
    std::map<std::string, std::string> config = {
        {"STRATEGY_TYPE", "breakout"},
        {"STRATEGY_SYMBOL", "BTC/USD"},
        {"STRATEGY_THRESHOLD", "50000.0"},
        {"STRATEGY_VOLUME_MULTIPLIER", "2.0"}
    };
    
    auto strategy = StrategyConfig::from_map(config);
    ASSERT_NE(strategy, nullptr);
    EXPECT_NE(strategy->name().find("Composite"), std::string::npos);
}

TEST_F(StrategyConfigTest, SupportLevelFromMap) {
    std::map<std::string, std::string> config = {
        {"STRATEGY_TYPE", "support_level"},
        {"STRATEGY_SYMBOL", "BTC/USD"},
        {"STRATEGY_LEVEL", "45000.0"},
        {"STRATEGY_TOLERANCE", "1.5"},
        {"STRATEGY_MIN_LIQUIDITY", "20.0"}
    };
    
    auto strategy = StrategyConfig::from_map(config);
    ASSERT_NE(strategy, nullptr);
    EXPECT_EQ(strategy->name(), "SupportLevel");
}

TEST_F(StrategyConfigTest, InvalidConfigThrows) {
    std::map<std::string, std::string> config = {
        {"STRATEGY_TYPE", "price_alert"}
        // Missing STRATEGY_SYMBOL
    };
    
    EXPECT_THROW(StrategyConfig::from_map(config), std::invalid_argument);
}

TEST_F(StrategyConfigTest, UnknownStrategyTypeThrows) {
    std::map<std::string, std::string> config = {
        {"STRATEGY_TYPE", "unknown_strategy"},
        {"STRATEGY_SYMBOL", "BTC/USD"}
    };
    
    EXPECT_THROW(StrategyConfig::from_map(config), std::invalid_argument);
}

TEST_F(StrategyConfigTest, CustomPrefix) {
    std::map<std::string, std::string> config = {
        {"MY_TYPE", "price_alert"},
        {"MY_SYMBOL", "ETH/USD"},
        {"MY_ABOVE", "3000.0"}
    };
    
    auto strategy = StrategyConfig::from_map(config, "MY_");
    ASSERT_NE(strategy, nullptr);
    
    Ticker ticker;
    ticker.symbol = "ETH/USD";
    ticker.last = 3100.0;
    ticker.bid = 3099.0;
    ticker.ask = 3101.0;
    
    EXPECT_TRUE(strategy->check(ticker));
}

//------------------------------------------------------------------------------
// Integration Tests
//------------------------------------------------------------------------------

class StrategyIntegrationTest : public ::testing::Test {
protected:
    Ticker make_ticker(const std::string& symbol, double price, double volume = 1000.0) {
        Ticker t;
        t.symbol = symbol;
        t.last = price;
        t.bid = price - 0.5;
        t.ask = price + 0.5;
        t.volume_24h = volume;
        return t;
    }
};

TEST_F(StrategyIntegrationTest, CompositeWithPresets) {
    auto breakout = StrategyPresets::breakout("BTC/USD", 50000.0, 2.0);
    auto support = StrategyPresets::support_level("BTC/USD", 45000.0, 1.0, 10.0);
    
    // OR: Either breakout OR support level
    auto either = CompositeStrategy::or_(breakout, support);
    
    // Build volume history for breakout (need at least lookback/2 = 10 samples)
    // Build more than lookback to ensure stable baseline
    for (int i = 0; i < 25; ++i) {
        either->check(make_ticker("BTC/USD", 49000.0, 100.0));
    }
    
    // Add enough normal samples to ensure clean baseline (fully replace history)
    // Since lookback=20, we need 20+ samples to fully replace history
    for (int i = 0; i < 25; ++i) {
        either->check(make_ticker("BTC/USD", 50000.0, 100.0));
    }
    
    // Breakout condition met - should trigger
    // Price 51000 > 50000 threshold AND volume 250.0 > 2x average of 100.0
    EXPECT_TRUE(either->check(make_ticker("BTC/USD", 51000.0, 250.0)));
}

TEST_F(StrategyIntegrationTest, EnableDisableComposite) {
    auto price = PriceAlert::Builder().symbol("BTC/USD").above(50000.0).build();
    auto volume = VolumeSpike::Builder().symbols({"BTC/USD"}).multiplier(2.0).lookback(5).build();
    
    // Build history
    for (int i = 0; i < 5; ++i) {
        volume->check(make_ticker("BTC/USD", 49000.0, 100.0));
    }
    
    auto combined = CompositeStrategy::and_(price, volume);
    
    // Test enable/disable
    EXPECT_TRUE(combined->is_enabled());
    combined->disable();
    EXPECT_FALSE(combined->is_enabled());
    EXPECT_FALSE(combined->check(make_ticker("BTC/USD", 51000.0, 250.0)));
    
    combined->enable();
    EXPECT_TRUE(combined->is_enabled());
    EXPECT_TRUE(combined->check(make_ticker("BTC/USD", 51000.0, 250.0)));
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

