/// @file test_circuit_breaker.cpp
/// @brief Unit tests for Circuit Breaker pattern

#include <gtest/gtest.h>
#include <kraken/connection/circuit_breaker.hpp>
#include <thread>
#include <chrono>

using namespace kraken;

class CircuitBreakerTest : public ::testing::Test {
protected:
    CircuitBreakerConfig make_config(uint32_t failure_threshold = 3,
                                     uint32_t success_threshold = 2,
                                     std::chrono::milliseconds timeout = std::chrono::milliseconds(100)) {
        CircuitBreakerConfig config;
        config.failure_threshold = failure_threshold;
        config.success_threshold = success_threshold;
        config.min_open_time_ms = timeout;
        return config;
    }
};

TEST_F(CircuitBreakerTest, StartsClosed) {
    CircuitBreaker cb;
    EXPECT_EQ(cb.state(), CircuitState::Closed);
    EXPECT_TRUE(cb.can_attempt());
}

TEST_F(CircuitBreakerTest, OpensAfterFailureThreshold) {
    CircuitBreakerConfig config = make_config(3, 2, std::chrono::milliseconds(100));
    CircuitBreaker cb(config);
    
    // Record failures up to threshold
    cb.record_failure();
    EXPECT_EQ(cb.state(), CircuitState::Closed);
    EXPECT_EQ(cb.failure_count(), 1);
    
    cb.record_failure();
    EXPECT_EQ(cb.state(), CircuitState::Closed);
    EXPECT_EQ(cb.failure_count(), 2);
    
    cb.record_failure();
    EXPECT_EQ(cb.state(), CircuitState::Open);
    EXPECT_FALSE(cb.can_attempt());
}

TEST_F(CircuitBreakerTest, RejectsWhenOpen) {
    CircuitBreakerConfig config = make_config(2, 1, std::chrono::milliseconds(50));
    CircuitBreaker cb(config);
    
    cb.record_failure();
    cb.record_failure();
    
    EXPECT_EQ(cb.state(), CircuitState::Open);
    EXPECT_FALSE(cb.can_attempt());
}

TEST_F(CircuitBreakerTest, TransitionsToHalfOpenAfterTimeout) {
    CircuitBreakerConfig config = make_config(2, 1, std::chrono::milliseconds(50));
    CircuitBreaker cb(config);
    
    // Open the circuit
    cb.record_failure();
    cb.record_failure();
    EXPECT_EQ(cb.state(), CircuitState::Open);
    
    // Wait for timeout
    std::this_thread::sleep_for(std::chrono::milliseconds(60));
    
    // Should transition to half-open
    EXPECT_TRUE(cb.can_attempt());  // This triggers transition
    EXPECT_EQ(cb.state(), CircuitState::HalfOpen);
}

TEST_F(CircuitBreakerTest, ClosesAfterSuccessThreshold) {
    CircuitBreakerConfig config = make_config(2, 2, std::chrono::milliseconds(50));
    CircuitBreaker cb(config);
    
    // Open the circuit
    cb.record_failure();
    cb.record_failure();
    
    // Wait and transition to half-open
    std::this_thread::sleep_for(std::chrono::milliseconds(60));
    cb.can_attempt();  // Triggers transition to half-open
    
    // Record successes
    cb.record_success();
    EXPECT_EQ(cb.state(), CircuitState::HalfOpen);
    
    cb.record_success();
    EXPECT_EQ(cb.state(), CircuitState::Closed);
    EXPECT_EQ(cb.failure_count(), 0);
}

TEST_F(CircuitBreakerTest, ReopensOnFailureInHalfOpen) {
    CircuitBreakerConfig config = make_config(2, 2, std::chrono::milliseconds(50));
    CircuitBreaker cb(config);
    
    // Open the circuit
    cb.record_failure();
    cb.record_failure();
    
    // Wait and transition to half-open
    std::this_thread::sleep_for(std::chrono::milliseconds(60));
    cb.can_attempt();
    
    // Any failure in half-open immediately opens
    cb.record_failure();
    EXPECT_EQ(cb.state(), CircuitState::Open);
}

TEST_F(CircuitBreakerTest, Reset) {
    CircuitBreakerConfig config = make_config(3, 1, std::chrono::milliseconds(100));
    CircuitBreaker cb(config);
    
    cb.record_failure();
    cb.record_failure();
    cb.record_failure();
    
    EXPECT_EQ(cb.state(), CircuitState::Open);
    
    cb.reset();
    
    EXPECT_EQ(cb.state(), CircuitState::Closed);
    EXPECT_EQ(cb.failure_count(), 0);
    EXPECT_EQ(cb.success_count(), 0);
    EXPECT_TRUE(cb.can_attempt());
}

TEST_F(CircuitBreakerTest, SuccessResetsFailureWindow) {
    CircuitBreakerConfig config = make_config(3, 1, std::chrono::milliseconds(100));
    config.failure_window_ms = std::chrono::milliseconds(1000);
    CircuitBreaker cb(config);
    
    cb.record_failure();
    cb.record_failure();
    EXPECT_EQ(cb.failure_count(), 2);
    
    cb.record_success();
    EXPECT_EQ(cb.failure_count(), 0);
    EXPECT_EQ(cb.state(), CircuitState::Closed);
}

TEST_F(CircuitBreakerTest, ConfigurableThresholds) {
    CircuitBreakerConfig config;
    config.failure_threshold = 5;
    config.success_threshold = 3;
    config.min_open_time_ms = std::chrono::milliseconds(200);
    
    CircuitBreaker cb(config);
    
    // Need 5 failures to open
    for (int i = 0; i < 4; ++i) {
        cb.record_failure();
        EXPECT_EQ(cb.state(), CircuitState::Closed);
    }
    
    cb.record_failure();
    EXPECT_EQ(cb.state(), CircuitState::Open);
}

