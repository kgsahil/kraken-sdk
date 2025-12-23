/// @file test_gap_detector.cpp
/// @brief Unit tests for message gap detection

#include <gtest/gtest.h>
#include <kraken/connection/gap_detector.hpp>
#include <vector>
#include <atomic>
#include <thread>

using namespace kraken;

class GapDetectorTest : public ::testing::Test {
protected:
    void SetUp() override {
        gap_count_ = 0;
        last_gap_ = {};
    }
    
    std::atomic<int> gap_count_{0};
    GapInfo last_gap_;
};

//------------------------------------------------------------------------------
// Basic Sequence Tracking Tests
//------------------------------------------------------------------------------

TEST_F(GapDetectorTest, FirstMessageAlwaysValid) {
    SequenceTracker tracker;
    
    // First message should always be valid (establishes baseline)
    EXPECT_TRUE(tracker.check("ticker", "BTC/USD", 100));
    EXPECT_TRUE(tracker.check("ticker", "ETH/USD", 50));  // Different symbol
    EXPECT_TRUE(tracker.check("book", "BTC/USD", 200));   // Different channel
}

TEST_F(GapDetectorTest, ConsecutiveSequences) {
    SequenceTracker tracker;
    
    // Consecutive sequences should be valid
    EXPECT_TRUE(tracker.check("ticker", "BTC/USD", 1));
    EXPECT_TRUE(tracker.check("ticker", "BTC/USD", 2));
    EXPECT_TRUE(tracker.check("ticker", "BTC/USD", 3));
    EXPECT_TRUE(tracker.check("ticker", "BTC/USD", 4));
}

TEST_F(GapDetectorTest, DetectForwardGap) {
    SequenceTracker tracker;
    bool gap_detected = false;
    
    tracker.on_gap([&](const GapInfo& gap) {
        gap_detected = true;
        last_gap_ = gap;
    });
    
    EXPECT_TRUE(tracker.check("ticker", "BTC/USD", 1));
    EXPECT_TRUE(tracker.check("ticker", "BTC/USD", 2));
    
    // Skip 3 and 4, jump to 5 (gap of 2)
    EXPECT_FALSE(tracker.check("ticker", "BTC/USD", 5));
    
    EXPECT_TRUE(gap_detected);
    EXPECT_EQ(last_gap_.channel, "ticker");
    EXPECT_EQ(last_gap_.symbol, "BTC/USD");
    EXPECT_EQ(last_gap_.expected_seq, 3);
    EXPECT_EQ(last_gap_.actual_seq, 5);
    EXPECT_EQ(last_gap_.gap_size, 2);
    EXPECT_FALSE(last_gap_.is_reorder());
}

TEST_F(GapDetectorTest, DetectReorder) {
    SequenceTracker::Config config;
    config.track_reorders = true;
    SequenceTracker tracker(config);
    
    bool gap_detected = false;
    tracker.on_gap([&](const GapInfo& gap) {
        gap_detected = true;
        last_gap_ = gap;
    });
    
    EXPECT_TRUE(tracker.check("ticker", "BTC/USD", 1));
    EXPECT_TRUE(tracker.check("ticker", "BTC/USD", 2));
    EXPECT_TRUE(tracker.check("ticker", "BTC/USD", 3));
    
    // Old message arrives late (reorder)
    EXPECT_FALSE(tracker.check("ticker", "BTC/USD", 2));
    
    EXPECT_TRUE(gap_detected);
    EXPECT_TRUE(last_gap_.is_reorder());
}

TEST_F(GapDetectorTest, ToleranceAllowsSmallGaps) {
    SequenceTracker::Config config;
    config.gap_tolerance = 2;  // Allow up to 2 missing
    SequenceTracker tracker(config);
    
    bool gap_detected = false;
    tracker.on_gap([&](const GapInfo&) {
        gap_detected = true;
    });
    
    EXPECT_TRUE(tracker.check("ticker", "BTC/USD", 1));
    
    // Gap of 1 (missing seq 2) - within tolerance
    EXPECT_TRUE(tracker.check("ticker", "BTC/USD", 3));
    EXPECT_FALSE(gap_detected);
    
    // Gap of 2 (missing seq 4, 5) - within tolerance
    EXPECT_TRUE(tracker.check("ticker", "BTC/USD", 6));
    EXPECT_FALSE(gap_detected);
    
    // Gap of 3 (missing seq 7, 8, 9) - exceeds tolerance
    EXPECT_FALSE(tracker.check("ticker", "BTC/USD", 10));
    EXPECT_TRUE(gap_detected);
}

TEST_F(GapDetectorTest, DisabledTracking) {
    SequenceTracker::Config config;
    config.enabled = false;
    SequenceTracker tracker(config);
    
    bool gap_detected = false;
    tracker.on_gap([&](const GapInfo&) {
        gap_detected = true;
    });
    
    // All sequences should be "valid" when disabled
    EXPECT_TRUE(tracker.check("ticker", "BTC/USD", 1));
    EXPECT_TRUE(tracker.check("ticker", "BTC/USD", 100));  // Big jump
    EXPECT_FALSE(gap_detected);  // No gaps reported
}

//------------------------------------------------------------------------------
// Reset Tests
//------------------------------------------------------------------------------

TEST_F(GapDetectorTest, ResetSpecificChannel) {
    SequenceTracker tracker;
    
    EXPECT_TRUE(tracker.check("ticker", "BTC/USD", 10));
    EXPECT_TRUE(tracker.check("ticker", "BTC/USD", 11));
    
    // Reset tracking for this symbol
    tracker.reset("ticker", "BTC/USD");
    
    // Next message should be accepted as first (no gap)
    bool gap_detected = false;
    tracker.on_gap([&](const GapInfo&) {
        gap_detected = true;
    });
    
    EXPECT_TRUE(tracker.check("ticker", "BTC/USD", 50));
    EXPECT_FALSE(gap_detected);
}

TEST_F(GapDetectorTest, ResetAll) {
    SequenceTracker tracker;
    
    EXPECT_TRUE(tracker.check("ticker", "BTC/USD", 10));
    EXPECT_TRUE(tracker.check("book", "ETH/USD", 20));
    
    tracker.reset_all();
    
    // All channels should start fresh
    bool gap_detected = false;
    tracker.on_gap([&](const GapInfo&) {
        gap_detected = true;
    });
    
    EXPECT_TRUE(tracker.check("ticker", "BTC/USD", 100));
    EXPECT_TRUE(tracker.check("book", "ETH/USD", 200));
    EXPECT_FALSE(gap_detected);
}

//------------------------------------------------------------------------------
// Gap Count Tests
//------------------------------------------------------------------------------

TEST_F(GapDetectorTest, GapCount) {
    SequenceTracker tracker;
    
    EXPECT_EQ(tracker.gap_count(), 0);
    
    EXPECT_TRUE(tracker.check("ticker", "BTC/USD", 1));
    EXPECT_FALSE(tracker.check("ticker", "BTC/USD", 5));  // Gap 1
    EXPECT_EQ(tracker.gap_count(), 1);
    
    EXPECT_FALSE(tracker.check("ticker", "BTC/USD", 10)); // Gap 2
    EXPECT_EQ(tracker.gap_count(), 2);
}

//------------------------------------------------------------------------------
// Multi-Symbol Tests
//------------------------------------------------------------------------------

TEST_F(GapDetectorTest, IndependentSymbolTracking) {
    SequenceTracker tracker;
    
    int gap_count = 0;
    tracker.on_gap([&](const GapInfo&) {
        gap_count++;
    });
    
    // BTC starts at 1
    EXPECT_TRUE(tracker.check("ticker", "BTC/USD", 1));
    
    // ETH starts at 100
    EXPECT_TRUE(tracker.check("ticker", "ETH/USD", 100));
    
    // Both continue independently
    EXPECT_TRUE(tracker.check("ticker", "BTC/USD", 2));
    EXPECT_TRUE(tracker.check("ticker", "ETH/USD", 101));
    
    // Gap in BTC only
    EXPECT_FALSE(tracker.check("ticker", "BTC/USD", 5));  // Gap
    EXPECT_TRUE(tracker.check("ticker", "ETH/USD", 102)); // No gap
    
    EXPECT_EQ(gap_count, 1);
}

//------------------------------------------------------------------------------
// JSON Serialization Tests
//------------------------------------------------------------------------------

TEST_F(GapDetectorTest, GapInfoToJson) {
    GapInfo gap;
    gap.channel = "ticker";
    gap.symbol = "BTC/USD";
    gap.expected_seq = 10;
    gap.actual_seq = 15;
    gap.gap_size = 5;
    gap.timestamp = std::chrono::system_clock::now();
    
    std::string json = gap.to_json();
    
    // Verify JSON structure
    EXPECT_NE(json.find("\"channel\":\"ticker\""), std::string::npos);
    EXPECT_NE(json.find("\"symbol\":\"BTC/USD\""), std::string::npos);
    EXPECT_NE(json.find("\"expected\":10"), std::string::npos);
    EXPECT_NE(json.find("\"actual\":15"), std::string::npos);
    EXPECT_NE(json.find("\"gap_size\":5"), std::string::npos);
    EXPECT_NE(json.find("\"is_reorder\":false"), std::string::npos);
}

//------------------------------------------------------------------------------
// Callback Exception Safety Tests
//------------------------------------------------------------------------------

TEST_F(GapDetectorTest, CallbackExceptionSafe) {
    SequenceTracker tracker;
    
    tracker.on_gap([](const GapInfo&) {
        throw std::runtime_error("Callback error");
    });
    
    EXPECT_TRUE(tracker.check("ticker", "BTC/USD", 1));
    
    // Should not crash even if callback throws
    EXPECT_NO_THROW({
        tracker.check("ticker", "BTC/USD", 10);  // Gap
    });
}

//------------------------------------------------------------------------------
// Thread Safety Tests
//------------------------------------------------------------------------------

TEST_F(GapDetectorTest, ConcurrentAccess) {
    SequenceTracker tracker;
    
    std::atomic<int> gap_count{0};
    tracker.on_gap([&](const GapInfo&) {
        gap_count++;
    });
    
    // Simulate concurrent access from multiple threads
    std::vector<std::thread> threads;
    for (int t = 0; t < 4; ++t) {
        threads.emplace_back([&, t]() {
            std::string symbol = "SYM" + std::to_string(t);
            for (uint64_t i = 1; i <= 100; ++i) {
                tracker.check("ticker", symbol, i);
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    // No gaps should be detected (all consecutive)
    EXPECT_EQ(gap_count.load(), 0);
}

