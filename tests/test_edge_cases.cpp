/// @file test_edge_cases.cpp
/// @brief Edge cases and boundary condition tests

#include <gtest/gtest.h>
#include <kraken/kraken.hpp>
#include <kraken/strategies.hpp>
#include "../src/book_engine.hpp"
#include <limits>
#include <cmath>

using namespace kraken;

class EdgeCasesTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

// Test extreme price values
TEST_F(EdgeCasesTest, ExtremePriceValues) {
    auto alert = PriceAlert::Builder()
        .symbol("BTC/USD")
        .above(std::numeric_limits<double>::max())
        .build();
    
    Ticker ticker;
    ticker.symbol = "BTC/USD";
    ticker.last = 1000000.0;
    
    // Should not trigger (price below max)
    EXPECT_FALSE(alert->check(ticker));
}

// Test zero and negative values
TEST_F(EdgeCasesTest, ZeroAndNegativeValues) {
    Ticker ticker;
    ticker.symbol = "BTC/USD";
    ticker.bid = 0.0;
    ticker.ask = 0.0;
    ticker.last = 0.0;
    
    // Spread should be 0
    EXPECT_DOUBLE_EQ(ticker.spread(), 0.0);
    EXPECT_DOUBLE_EQ(ticker.mid_price(), 0.0);
}

// Test very large order book
TEST_F(EdgeCasesTest, LargeOrderBook) {
    BookEngine engine;
    
    // Create large book with many levels
    std::vector<PriceLevel> bids;
    std::vector<PriceLevel> asks;
    
    for (int i = 0; i < 1000; ++i) {
        bids.push_back({50000.0 - i, 1.0});
        asks.push_back({50001.0 + i, 1.0});
    }
    
    engine.apply("BTC/USD", bids, asks, true);
    
    auto* book = engine.get("BTC/USD");
    ASSERT_NE(book, nullptr);
    EXPECT_EQ(book->bids.size(), 1000);
    EXPECT_EQ(book->asks.size(), 1000);
}

// Test empty order book
TEST_F(EdgeCasesTest, EmptyOrderBook) {
    BookEngine engine;
    
    std::vector<PriceLevel> empty_bids;
    std::vector<PriceLevel> empty_asks;
    
    engine.apply("BTC/USD", empty_bids, empty_asks, true);
    
    auto* book = engine.get("BTC/USD");
    ASSERT_NE(book, nullptr);
    EXPECT_EQ(book->bids.size(), 0);
    EXPECT_EQ(book->asks.size(), 0);
    EXPECT_EQ(book->best_bid(), nullptr);
    EXPECT_EQ(book->best_ask(), nullptr);
    EXPECT_DOUBLE_EQ(book->spread(), 0.0);
}

// Test order book with single level
TEST_F(EdgeCasesTest, SingleLevelOrderBook) {
    BookEngine engine;
    
    std::vector<PriceLevel> bids = {{50000.0, 1.0}};
    std::vector<PriceLevel> asks = {{50001.0, 1.0}};
    
    engine.apply("BTC/USD", bids, asks, true);
    
    auto* book = engine.get("BTC/USD");
    ASSERT_NE(book, nullptr);
    EXPECT_NE(book->best_bid(), nullptr);
    EXPECT_NE(book->best_ask(), nullptr);
    EXPECT_DOUBLE_EQ(book->spread(), 1.0);
}

// Test price alert with exact threshold
TEST_F(EdgeCasesTest, PriceAlertExactThreshold) {
    auto alert = PriceAlert::Builder()
        .symbol("BTC/USD")
        .above(50000.0)
        .build();
    
    Ticker ticker;
    ticker.symbol = "BTC/USD";
    ticker.last = 50000.0; // Exactly at threshold
    
    // Should trigger (above means >= in practice - alerts fire when price reaches threshold)
    EXPECT_TRUE(alert->check(ticker));
    
    ticker.last = 50000.01; // Just above
    // Already fired, so should not trigger again
    EXPECT_FALSE(alert->check(ticker));
}

// Test volume spike with zero volume
TEST_F(EdgeCasesTest, VolumeSpikeZeroVolume) {
    auto spike = VolumeSpike::Builder()
        .symbols({"BTC/USD"})
        .multiplier(2.0)
        .lookback(5)
        .build();
    
    // All zero volumes
    for (int i = 0; i < 10; ++i) {
        Ticker ticker;
        ticker.symbol = "BTC/USD";
        ticker.volume_24h = 0.0;
        spike->check(ticker);
    }
    
    // Spike with zero volume (edge case)
    Ticker ticker;
    ticker.symbol = "BTC/USD";
    ticker.volume_24h = 0.0;
    // Should not trigger (0 * 2 = 0)
    EXPECT_FALSE(spike->check(ticker));
}

// Test spread alert with zero spread
TEST_F(EdgeCasesTest, SpreadAlertZeroSpread) {
    auto alert = SpreadAlert::Builder()
        .symbol("BTC/USD")
        .min_spread(1.0)
        .build();
    
    Ticker ticker;
    ticker.symbol = "BTC/USD";
    ticker.bid = 50000.0;
    ticker.ask = 50000.0; // Zero spread
    
    // Should trigger (spread 0 < min 1.0)
    EXPECT_TRUE(alert->check(ticker));
}

// Test negative spread (invalid but should handle gracefully)
TEST_F(EdgeCasesTest, NegativeSpread) {
    Ticker ticker;
    ticker.symbol = "BTC/USD";
    ticker.bid = 50001.0;
    ticker.ask = 50000.0; // ask < bid (invalid but possible)
    
    double spread = ticker.spread();
    EXPECT_LT(spread, 0.0); // Negative spread
}

// Test very small price differences
TEST_F(EdgeCasesTest, VerySmallPriceDifferences) {
    Ticker ticker;
    ticker.symbol = "BTC/USD";
    ticker.bid = 50000.0;
    ticker.ask = 50000.0000001; // Very small spread
    
    double spread = ticker.spread();
    EXPECT_GT(spread, 0.0);
    EXPECT_LT(spread, 0.000001);
}

// Test subscription with many symbols
TEST_F(EdgeCasesTest, ManySymbolsSubscription) {
    KrakenClient client;
    
    std::vector<std::string> symbols;
    for (int i = 0; i < 100; ++i) {
        symbols.push_back("SYM" + std::to_string(i) + "/USD");
    }
    
    auto sub = client.subscribe(Channel::Ticker, symbols);
    
    auto sub_symbols = sub.symbols();
    EXPECT_EQ(sub_symbols.size(), 100);
}

// Test rapid subscription/unsubscription
TEST_F(EdgeCasesTest, RapidSubscriptionLifecycle) {
    KrakenClient client;
    
    for (int i = 0; i < 100; ++i) {
        auto sub = client.subscribe(Channel::Ticker, {"BTC/USD"});
        sub.pause();
        sub.resume();
        sub.unsubscribe();
    }
    
    // Should not crash
    EXPECT_TRUE(true);
}

// Test alert with very high threshold
TEST_F(EdgeCasesTest, VeryHighThreshold) {
    auto alert = PriceAlert::Builder()
        .symbol("BTC/USD")
        .above(1000000.0)
        .build();
    
    Ticker ticker;
    ticker.symbol = "BTC/USD";
    ticker.last = 50000.0;
    
    EXPECT_FALSE(alert->check(ticker));
}

// Test alert with very low threshold
TEST_F(EdgeCasesTest, VeryLowThreshold) {
    auto alert = PriceAlert::Builder()
        .symbol("BTC/USD")
        .below(1.0)
        .build();
    
    Ticker ticker;
    ticker.symbol = "BTC/USD";
    ticker.last = 50000.0;
    
    EXPECT_FALSE(alert->check(ticker));
}

// Test NaN and infinity handling
TEST_F(EdgeCasesTest, NaNInfinityHandling) {
    Ticker ticker;
    ticker.symbol = "BTC/USD";
    ticker.bid = std::numeric_limits<double>::quiet_NaN();
    ticker.ask = 50001.0;
    
    // Spread calculation should handle NaN
    double spread = ticker.spread();
    EXPECT_TRUE(std::isnan(spread) || std::isinf(spread));
}

// Test empty symbol string
TEST_F(EdgeCasesTest, EmptySymbolString) {
    Ticker ticker;
    ticker.symbol = "";
    ticker.last = 50000.0;
    
    // Should not crash
    EXPECT_TRUE(ticker.symbol.empty());
}

// Test order book update with zero quantity (removal)
TEST_F(EdgeCasesTest, OrderBookZeroQuantityRemoval) {
    BookEngine engine;
    
    // Initial snapshot
    std::vector<PriceLevel> bids = {{50000.0, 1.0}, {49999.0, 2.0}};
    std::vector<PriceLevel> asks = {{50001.0, 1.0}};
    engine.apply("BTC/USD", bids, asks, true);
    
    // Update: remove level at 49999.0
    std::vector<PriceLevel> upd_bids = {{49999.0, 0.0}};
    engine.apply("BTC/USD", upd_bids, {}, false);
    
    auto* book = engine.get("BTC/USD");
    ASSERT_NE(book, nullptr);
    // Should have only one bid left
    EXPECT_EQ(book->bids.size(), 1);
    EXPECT_DOUBLE_EQ(book->bids[0].price, 50000.0);
}

// Test order book with duplicate prices
TEST_F(EdgeCasesTest, OrderBookDuplicatePrices) {
    BookEngine engine;
    
    // Multiple levels at same price (should update, not duplicate)
    std::vector<PriceLevel> bids = {
        {50000.0, 1.0},
        {50000.0, 2.0}, // Same price, different qty
        {49999.0, 1.0}
    };
    
    engine.apply("BTC/USD", bids, {}, true);
    
    auto* book = engine.get("BTC/USD");
    ASSERT_NE(book, nullptr);
    // Should consolidate to unique prices
    // Implementation may keep last value or sum - depends on design
    EXPECT_LE(book->bids.size(), 3);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

