#include <gtest/gtest.h>
#include "kraken/rate_limiter.hpp"
#include <thread>
#include <chrono>
#include <vector>

using namespace kraken;

class RateLimiterTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Set up test fixtures
    }
    
    void TearDown() override {
        // Clean up
    }
};

// Test basic token acquisition
TEST_F(RateLimiterTest, BasicAcquire) {
    RateLimiter limiter(10.0, 20, true);  // 10 req/sec, burst 20
    
    // Should be able to acquire tokens up to burst size
    for (size_t i = 0; i < 20; ++i) {
        EXPECT_TRUE(limiter.acquire()) << "Failed to acquire token " << i;
    }
    
    // Should be rate limited after burst
    EXPECT_FALSE(limiter.acquire());
}

// Test token refill over time
TEST_F(RateLimiterTest, TokenRefill) {
    RateLimiter limiter(10.0, 20, true);  // 10 req/sec
    
    // Consume all tokens
    for (size_t i = 0; i < 20; ++i) {
        limiter.acquire();
    }
    
    // Wait for tokens to refill (should get ~1 token per 100ms at 10 req/sec)
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    
    // Should be able to acquire at least 1 token
    EXPECT_TRUE(limiter.acquire());
}

// Test disabled rate limiter
TEST_F(RateLimiterTest, DisabledLimiter) {
    RateLimiter limiter(10.0, 20, false);  // Disabled
    
    // Should always allow requests when disabled
    for (size_t i = 0; i < 100; ++i) {
        EXPECT_TRUE(limiter.acquire()) << "Failed at iteration " << i;
    }
}

// Test blocking acquire
TEST_F(RateLimiterTest, BlockingAcquire) {
    RateLimiter limiter(10.0, 20, true);
    
    // Consume all tokens
    for (size_t i = 0; i < 20; ++i) {
        limiter.acquire();
    }
    
    // Blocking acquire should wait and eventually succeed
    auto start = std::chrono::steady_clock::now();
    bool acquired = limiter.acquire_blocking(std::chrono::milliseconds(200));
    auto elapsed = std::chrono::steady_clock::now() - start;
    
    EXPECT_TRUE(acquired);
    // Should have waited at least 100ms (time for 1 token at 10 req/sec)
    EXPECT_GE(std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count(), 90);
}

// Test blocking acquire timeout
TEST_F(RateLimiterTest, BlockingAcquireTimeout) {
    RateLimiter limiter(0.1, 1, true);  // Very slow rate (0.1 req/sec = 10 seconds per token)
    
    // Consume the only token
    limiter.acquire();
    
    // Blocking acquire with short timeout should fail
    bool acquired = limiter.acquire_blocking(std::chrono::milliseconds(100));
    EXPECT_FALSE(acquired);
}

// Test wait time calculation
TEST_F(RateLimiterTest, WaitTime) {
    RateLimiter limiter(10.0, 20, true);
    
    // Consume all tokens
    for (size_t i = 0; i < 20; ++i) {
        limiter.acquire();
    }
    
    // Wait time should be > 0 when no tokens available
    auto wait = limiter.wait_time();
    EXPECT_GT(wait.count(), 0);
    
    // Wait for token to refill
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    
    // Wait time should be 0 or very small when token available
    wait = limiter.wait_time();
    EXPECT_LE(wait.count(), 50);  // Allow some margin
}

// Test token count
TEST_F(RateLimiterTest, TokenCount) {
    RateLimiter limiter(10.0, 20, true);
    
    // Initially should have max tokens
    EXPECT_GE(limiter.tokens(), 19.0);  // Allow some margin for timing
    
    // After consuming, tokens should decrease
    limiter.acquire();
    EXPECT_LT(limiter.tokens(), 20.0);
}

// Test reset
TEST_F(RateLimiterTest, Reset) {
    RateLimiter limiter(10.0, 20, true);
    
    // Consume all tokens
    for (size_t i = 0; i < 20; ++i) {
        limiter.acquire();
    }
    
    EXPECT_FALSE(limiter.acquire());
    
    // Reset should restore tokens
    limiter.reset();
    EXPECT_TRUE(limiter.acquire());
}

// Test enable/disable
TEST_F(RateLimiterTest, EnableDisable) {
    RateLimiter limiter(10.0, 20, true);
    
    // Consume all tokens
    for (size_t i = 0; i < 20; ++i) {
        limiter.acquire();
    }
    
    EXPECT_FALSE(limiter.acquire());
    
    // Disable should allow requests
    limiter.set_enabled(false);
    EXPECT_TRUE(limiter.acquire());
    EXPECT_TRUE(limiter.is_enabled() == false);
    
    // Re-enable should restore rate limiting
    limiter.set_enabled(true);
    limiter.reset();  // Reset to get tokens back
    EXPECT_TRUE(limiter.acquire());
}

// Test statistics
TEST_F(RateLimiterTest, Statistics) {
    RateLimiter limiter(10.0, 20, true);
    
    // Make some requests
    for (size_t i = 0; i < 15; ++i) {
        limiter.acquire();
    }
    
    // Should be rate limited for remaining
    for (size_t i = 0; i < 10; ++i) {
        limiter.acquire();  // These should be rate limited
    }
    
    auto stats = limiter.get_stats();
    EXPECT_EQ(stats.total_requests, 25);
    EXPECT_EQ(stats.allowed_requests, 15);
    EXPECT_EQ(stats.rate_limited, 10);
    EXPECT_LE(stats.current_tokens, 5.0);
}

// Test high rate
TEST_F(RateLimiterTest, HighRate) {
    RateLimiter limiter(100.0, 200, true);  // 100 req/sec
    
    // Should be able to acquire burst
    for (size_t i = 0; i < 200; ++i) {
        EXPECT_TRUE(limiter.acquire()) << "Failed at " << i;
    }
    
    // Should be rate limited after burst
    EXPECT_FALSE(limiter.acquire());
}

// Test low rate
TEST_F(RateLimiterTest, LowRate) {
    RateLimiter limiter(1.0, 5, true);  // 1 req/sec
    
    // Should be able to acquire burst
    for (size_t i = 0; i < 5; ++i) {
        EXPECT_TRUE(limiter.acquire());
    }
    
    // Should be rate limited
    EXPECT_FALSE(limiter.acquire());
    
    // Wait for token to refill (1 second)
    std::this_thread::sleep_for(std::chrono::milliseconds(1100));
    
    // Should be able to acquire 1 token
    EXPECT_TRUE(limiter.acquire());
}

// Test thread safety (basic)
TEST_F(RateLimiterTest, ThreadSafety) {
    RateLimiter limiter(100.0, 1000, true);
    
    std::vector<std::thread> threads;
    std::atomic<size_t> success_count{0};
    std::atomic<size_t> fail_count{0};
    
    // Launch multiple threads trying to acquire tokens
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([&]() {
            for (int j = 0; j < 200; ++j) {
                if (limiter.acquire()) {
                    success_count.fetch_add(1);
                } else {
                    fail_count.fetch_add(1);
                }
            }
        });
    }
    
    // Wait for all threads
    for (auto& t : threads) {
        t.join();
    }
    
    // Total should be 2000 requests
    EXPECT_EQ(success_count.load() + fail_count.load(), 2000);
    
    // Should have some successes and some failures
    EXPECT_GT(success_count.load(), 0);
    EXPECT_GT(fail_count.load(), 0);
}

