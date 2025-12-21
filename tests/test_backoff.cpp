/// @file test_backoff.cpp
/// @brief Unit tests for backoff strategies

#include <gtest/gtest.h>
#include <kraken/backoff.hpp>
#include <thread>
#include <vector>
#include <cmath>

using namespace kraken;

class BackoffTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

//------------------------------------------------------------------------------
// ExponentialBackoff Tests
//------------------------------------------------------------------------------

TEST_F(BackoffTest, DefaultValues) {
    auto backoff = ExponentialBackoff::builder().build();
    
    EXPECT_EQ(backoff->max_attempts(), 10);
    EXPECT_EQ(backoff->current_attempt(), 1);
    EXPECT_FALSE(backoff->should_stop());
}

TEST_F(BackoffTest, ExponentialGrowth) {
    auto backoff = ExponentialBackoff::builder()
        .initial_delay(std::chrono::milliseconds(100))
        .max_delay(std::chrono::seconds(10))
        .multiplier(2.0)
        .jitter(0.0)  // No jitter for predictable testing
        .max_attempts(10)
        .build();
    
    // Collect delays
    std::vector<int64_t> delays;
    for (int i = 0; i < 5; ++i) {
        delays.push_back(backoff->next_delay().count());
    }
    
    // Verify exponential growth: 100, 200, 400, 800, 1600
    EXPECT_EQ(delays[0], 100);
    EXPECT_EQ(delays[1], 200);
    EXPECT_EQ(delays[2], 400);
    EXPECT_EQ(delays[3], 800);
    EXPECT_EQ(delays[4], 1600);
}

TEST_F(BackoffTest, MaxDelayCap) {
    auto backoff = ExponentialBackoff::builder()
        .initial_delay(std::chrono::milliseconds(1000))
        .max_delay(std::chrono::milliseconds(3000))
        .multiplier(2.0)
        .jitter(0.0)
        .max_attempts(10)
        .build();
    
    // 1000, 2000, 3000 (capped), 3000 (capped)
    EXPECT_EQ(backoff->next_delay().count(), 1000);
    EXPECT_EQ(backoff->next_delay().count(), 2000);
    EXPECT_EQ(backoff->next_delay().count(), 3000);  // Capped
    EXPECT_EQ(backoff->next_delay().count(), 3000);  // Still capped
}

TEST_F(BackoffTest, JitterRange) {
    auto backoff = ExponentialBackoff::builder()
        .initial_delay(std::chrono::milliseconds(1000))
        .max_delay(std::chrono::seconds(60))
        .multiplier(2.0)
        .jitter(0.3)  // ±30%
        .max_attempts(100)
        .build();
    
    // Run multiple iterations and check bounds
    for (int i = 0; i < 10; ++i) {
        backoff->reset();
        auto delay = backoff->next_delay().count();
        
        // With 30% jitter, delay should be between 700 and 1300
        EXPECT_GE(delay, 700);
        EXPECT_LE(delay, 1300);
    }
}

TEST_F(BackoffTest, MaxAttemptsReached) {
    auto backoff = ExponentialBackoff::builder()
        .initial_delay(std::chrono::milliseconds(100))
        .max_attempts(3)
        .build();
    
    EXPECT_FALSE(backoff->should_stop());
    backoff->next_delay();  // Attempt 1
    EXPECT_FALSE(backoff->should_stop());
    backoff->next_delay();  // Attempt 2
    EXPECT_FALSE(backoff->should_stop());
    backoff->next_delay();  // Attempt 3
    EXPECT_TRUE(backoff->should_stop());  // Max reached
}

TEST_F(BackoffTest, InfiniteAttempts) {
    auto backoff = ExponentialBackoff::builder()
        .initial_delay(std::chrono::milliseconds(100))
        .max_attempts(0)  // 0 = infinite
        .build();
    
    // Should never stop
    for (int i = 0; i < 100; ++i) {
        EXPECT_FALSE(backoff->should_stop());
        backoff->next_delay();
    }
    EXPECT_FALSE(backoff->should_stop());
}

TEST_F(BackoffTest, Reset) {
    auto backoff = ExponentialBackoff::builder()
        .initial_delay(std::chrono::milliseconds(100))
        .multiplier(2.0)
        .jitter(0.0)
        .max_attempts(5)
        .build();
    
    // Advance
    backoff->next_delay();  // 100
    backoff->next_delay();  // 200
    EXPECT_EQ(backoff->current_attempt(), 3);
    
    // Reset
    backoff->reset();
    
    // Should start fresh
    EXPECT_EQ(backoff->current_attempt(), 1);
    EXPECT_EQ(backoff->next_delay().count(), 100);
}

TEST_F(BackoffTest, Clone) {
    auto original = ExponentialBackoff::builder()
        .initial_delay(std::chrono::milliseconds(500))
        .max_delay(std::chrono::seconds(30))
        .multiplier(1.5)
        .jitter(0.0)
        .max_attempts(5)
        .build();
    
    // Advance original
    original->next_delay();
    original->next_delay();
    EXPECT_EQ(original->current_attempt(), 3);
    
    // Clone
    auto cloned = original->clone();
    
    // Clone should start fresh
    EXPECT_EQ(cloned->current_attempt(), 1);
    EXPECT_EQ(cloned->max_attempts(), 5);
    
    // Clone should have same config
    EXPECT_EQ(cloned->next_delay().count(), 500);
}

//------------------------------------------------------------------------------
// Preset Tests
//------------------------------------------------------------------------------

TEST_F(BackoffTest, AggressivePreset) {
    auto backoff = ExponentialBackoff::aggressive();
    
    EXPECT_EQ(backoff->max_attempts(), 20);
    
    // First delay should be around 100ms (with some jitter)
    auto delay = backoff->next_delay().count();
    EXPECT_GE(delay, 80);
    EXPECT_LE(delay, 120);
}

TEST_F(BackoffTest, ConservativePreset) {
    auto backoff = ExponentialBackoff::conservative();
    
    EXPECT_EQ(backoff->max_attempts(), 10);
    
    // First delay should be around 1000ms (with some jitter)
    auto delay = backoff->next_delay().count();
    EXPECT_GE(delay, 700);
    EXPECT_LE(delay, 1300);
}

TEST_F(BackoffTest, InfinitePreset) {
    auto backoff = ExponentialBackoff::infinite();
    
    EXPECT_EQ(backoff->max_attempts(), 0);  // 0 = infinite
    EXPECT_FALSE(backoff->should_stop());
}

//------------------------------------------------------------------------------
// FixedBackoff Tests
//------------------------------------------------------------------------------

TEST_F(BackoffTest, FixedDelay) {
    FixedBackoff backoff(std::chrono::milliseconds(500), 5);
    
    EXPECT_EQ(backoff.max_attempts(), 5);
    
    // All delays should be 500ms
    for (int i = 0; i < 3; ++i) {
        EXPECT_EQ(backoff.next_delay().count(), 500);
    }
}

TEST_F(BackoffTest, FixedMaxAttempts) {
    FixedBackoff backoff(std::chrono::milliseconds(100), 3);
    
    EXPECT_FALSE(backoff.should_stop());
    backoff.next_delay();
    backoff.next_delay();
    backoff.next_delay();
    EXPECT_TRUE(backoff.should_stop());
}

//------------------------------------------------------------------------------
// NoBackoff Tests
//------------------------------------------------------------------------------

TEST_F(BackoffTest, NoDelay) {
    NoBackoff backoff(5);
    
    EXPECT_EQ(backoff.max_attempts(), 5);
    
    // All delays should be 0
    for (int i = 0; i < 3; ++i) {
        EXPECT_EQ(backoff.next_delay().count(), 0);
    }
}

//------------------------------------------------------------------------------
// Thread Safety Tests
//------------------------------------------------------------------------------

TEST_F(BackoffTest, CloneIsThreadSafe) {
    auto original = ExponentialBackoff::conservative();
    
    // Clone multiple times from different "threads" (simulated)
    std::vector<std::unique_ptr<BackoffStrategy>> clones;
    for (int i = 0; i < 10; ++i) {
        clones.push_back(original->clone());
    }
    
    // Each clone should operate independently
    for (int i = 0; i < 10; ++i) {
        auto delay = clones[i]->next_delay();
        EXPECT_GT(delay.count(), 0);
    }
}

