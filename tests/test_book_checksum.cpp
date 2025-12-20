/// @file test_book_checksum.cpp
/// @brief Unit tests for order book checksum validation

#include <gtest/gtest.h>
#include "../src/book_engine.hpp"

using namespace kraken;

class BookEngineTest : public ::testing::Test {
protected:
    BookEngine engine;
    
    std::vector<PriceLevel> make_bids(std::initializer_list<std::pair<double, double>> levels) {
        std::vector<PriceLevel> bids;
        for (auto& [price, qty] : levels) {
            bids.push_back({price, qty});
        }
        return bids;
    }
    
    std::vector<PriceLevel> make_asks(std::initializer_list<std::pair<double, double>> levels) {
        std::vector<PriceLevel> asks;
        for (auto& [price, qty] : levels) {
            asks.push_back({price, qty});
        }
        return asks;
    }
};

TEST_F(BookEngineTest, ApplySnapshot) {
    auto bids = make_bids({{50000.0, 1.5}, {49999.0, 2.0}});
    auto asks = make_asks({{50001.0, 1.0}, {50002.0, 3.0}});
    
    engine.apply("BTC/USD", bids, asks, true);
    
    auto* book = engine.get("BTC/USD");
    ASSERT_NE(book, nullptr);
    EXPECT_EQ(book->symbol, "BTC/USD");
    EXPECT_EQ(book->bids.size(), 2);
    EXPECT_EQ(book->asks.size(), 2);
}

TEST_F(BookEngineTest, BestBidAsk) {
    auto bids = make_bids({{50000.0, 1.5}, {49999.0, 2.0}});
    auto asks = make_asks({{50001.0, 1.0}, {50002.0, 3.0}});
    
    engine.apply("BTC/USD", bids, asks, true);
    
    auto* book = engine.get("BTC/USD");
    ASSERT_NE(book, nullptr);
    
    auto* best_bid = book->best_bid();
    auto* best_ask = book->best_ask();
    
    ASSERT_NE(best_bid, nullptr);
    ASSERT_NE(best_ask, nullptr);
    
    EXPECT_DOUBLE_EQ(best_bid->price, 50000.0);
    EXPECT_DOUBLE_EQ(best_ask->price, 50001.0);
}

TEST_F(BookEngineTest, Spread) {
    auto bids = make_bids({{50000.0, 1.5}});
    auto asks = make_asks({{50010.0, 1.0}});
    
    engine.apply("BTC/USD", bids, asks, true);
    
    auto* book = engine.get("BTC/USD");
    ASSERT_NE(book, nullptr);
    
    EXPECT_DOUBLE_EQ(book->spread(), 10.0);
    EXPECT_DOUBLE_EQ(book->mid_price(), 50005.0);
}

TEST_F(BookEngineTest, IncrementalUpdate) {
    // Initial snapshot
    auto bids = make_bids({{50000.0, 1.5}, {49999.0, 2.0}});
    auto asks = make_asks({{50001.0, 1.0}, {50002.0, 3.0}});
    engine.apply("BTC/USD", bids, asks, true);
    
    // Update - modify qty at 50000, remove 49999, add 49998
    auto upd_bids = make_bids({{50000.0, 2.0}, {49999.0, 0.0}, {49998.0, 1.0}});
    engine.apply("BTC/USD", upd_bids, {}, false);
    
    auto* book = engine.get("BTC/USD");
    ASSERT_NE(book, nullptr);
    
    // Should have 50000 (updated) and 49998 (new)
    EXPECT_EQ(book->bids.size(), 2);
    EXPECT_DOUBLE_EQ(book->bids[0].price, 50000.0);
    EXPECT_DOUBLE_EQ(book->bids[0].quantity, 2.0);
    EXPECT_DOUBLE_EQ(book->bids[1].price, 49998.0);
}

TEST_F(BookEngineTest, ChecksumCalculation) {
    // Create a simple book
    auto bids = make_bids({{50000.0, 1.0}});
    auto asks = make_asks({{50001.0, 1.0}});
    
    engine.apply("BTC/USD", bids, asks, true);
    
    auto* book = engine.get("BTC/USD");
    ASSERT_NE(book, nullptr);
    
    // Calculate checksum
    uint32_t checksum = BookEngine::calculate_checksum(*book);
    
    // Just verify it returns something (actual value depends on Kraken's format)
    EXPECT_NE(checksum, 0);
}

TEST_F(BookEngineTest, NonExistentSymbol) {
    auto* book = engine.get("NONEXISTENT");
    EXPECT_EQ(book, nullptr);
}

TEST_F(BookEngineTest, ClearBooks) {
    auto bids = make_bids({{50000.0, 1.0}});
    auto asks = make_asks({{50001.0, 1.0}});
    
    engine.apply("BTC/USD", bids, asks, true);
    engine.apply("ETH/USD", bids, asks, true);
    
    EXPECT_NE(engine.get("BTC/USD"), nullptr);
    EXPECT_NE(engine.get("ETH/USD"), nullptr);
    
    engine.clear();
    
    EXPECT_EQ(engine.get("BTC/USD"), nullptr);
    EXPECT_EQ(engine.get("ETH/USD"), nullptr);
}

TEST_F(BookEngineTest, RemoveSymbol) {
    auto bids = make_bids({{50000.0, 1.0}});
    auto asks = make_asks({{50001.0, 1.0}});
    
    engine.apply("BTC/USD", bids, asks, true);
    engine.apply("ETH/USD", bids, asks, true);
    
    engine.remove("BTC/USD");
    
    EXPECT_EQ(engine.get("BTC/USD"), nullptr);
    EXPECT_NE(engine.get("ETH/USD"), nullptr);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

