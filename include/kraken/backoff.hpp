#pragma once

/// @file backoff.hpp
/// @brief Exponential backoff strategy for connection retries
/// 
/// Implements the Strategy Pattern for flexible retry behavior.
/// Supports exponential backoff with jitter to prevent thundering herd.

#include <chrono>
#include <random>
#include <algorithm>
#include <memory>
#include <cmath>

namespace kraken {

//------------------------------------------------------------------------------
// BackoffStrategy Interface (Strategy Pattern)
//------------------------------------------------------------------------------

/// Abstract base class for backoff strategies
class BackoffStrategy {
public:
    virtual ~BackoffStrategy() = default;
    
    /// Get the next delay duration
    /// @return Duration to wait before next retry
    virtual std::chrono::milliseconds next_delay() = 0;
    
    /// Reset the strategy (call after successful connection)
    virtual void reset() = 0;
    
    /// Check if max attempts reached
    virtual bool should_stop() const = 0;
    
    /// Get current attempt number (1-based)
    virtual int current_attempt() const = 0;
    
    /// Get max attempts (0 = infinite)
    virtual int max_attempts() const = 0;
    
    /// Clone the strategy (Prototype Pattern for thread safety)
    virtual std::unique_ptr<BackoffStrategy> clone() const = 0;
};

//------------------------------------------------------------------------------
// FixedBackoff - Simple fixed delay (for testing/legacy compatibility)
//------------------------------------------------------------------------------

/// Fixed delay between retries (no exponential growth)
class FixedBackoff : public BackoffStrategy {
public:
    explicit FixedBackoff(std::chrono::milliseconds delay, int max_attempts = 10)
        : delay_(delay), max_attempts_(max_attempts) {}
    
    std::chrono::milliseconds next_delay() override {
        attempt_++;
        return delay_;
    }
    
    void reset() override {
        attempt_ = 0;
    }
    
    bool should_stop() const override {
        return max_attempts_ > 0 && attempt_ >= max_attempts_;
    }
    
    int current_attempt() const override {
        return attempt_ + 1;
    }
    
    int max_attempts() const override {
        return max_attempts_;
    }
    
    std::unique_ptr<BackoffStrategy> clone() const override {
        return std::make_unique<FixedBackoff>(delay_, max_attempts_);
    }

private:
    std::chrono::milliseconds delay_;
    int max_attempts_;
    int attempt_ = 0;
};

//------------------------------------------------------------------------------
// NoBackoff - Immediate retry (for testing only)
//------------------------------------------------------------------------------

/// No delay between retries (use with caution!)
class NoBackoff : public BackoffStrategy {
public:
    explicit NoBackoff(int max_attempts = 3) : max_attempts_(max_attempts) {}
    
    std::chrono::milliseconds next_delay() override {
        attempt_++;
        return std::chrono::milliseconds(0);
    }
    
    void reset() override {
        attempt_ = 0;
    }
    
    bool should_stop() const override {
        return max_attempts_ > 0 && attempt_ >= max_attempts_;
    }
    
    int current_attempt() const override {
        return attempt_ + 1;
    }
    
    int max_attempts() const override {
        return max_attempts_;
    }
    
    std::unique_ptr<BackoffStrategy> clone() const override {
        return std::make_unique<NoBackoff>(max_attempts_);
    }

private:
    int max_attempts_;
    int attempt_ = 0;
};

//------------------------------------------------------------------------------
// ExponentialBackoff - Production-grade implementation
//------------------------------------------------------------------------------

/// Exponential backoff with jitter
/// 
/// Formula: delay = min(initial * multiplier^attempt, max) * (1 ± jitter)
/// 
/// @example
/// auto backoff = ExponentialBackoff::builder()
///     .initial_delay(std::chrono::seconds(1))
///     .max_delay(std::chrono::minutes(2))
///     .multiplier(2.0)
///     .jitter(0.3)
///     .max_attempts(10)
///     .build();
class ExponentialBackoff : public BackoffStrategy {
public:
    /// Builder for fluent configuration
    class Builder {
    public:
        Builder() = default;
        
        /// Set initial delay (default: 1 second)
        Builder& initial_delay(std::chrono::milliseconds delay) {
            initial_delay_ = delay;
            return *this;
        }
        
        /// Set maximum delay cap (default: 60 seconds)
        Builder& max_delay(std::chrono::milliseconds delay) {
            max_delay_ = delay;
            return *this;
        }
        
        /// Set multiplier for exponential growth (default: 2.0)
        Builder& multiplier(double mult) {
            multiplier_ = mult;
            return *this;
        }
        
        /// Set jitter factor 0.0-1.0 (default: 0.3 = ±30%)
        Builder& jitter(double factor) {
            jitter_factor_ = std::clamp(factor, 0.0, 1.0);
            return *this;
        }
        
        /// Set max attempts, 0 = infinite (default: 10)
        Builder& max_attempts(int attempts) {
            max_attempts_ = attempts;
            return *this;
        }
        
        /// Build the backoff strategy
        std::unique_ptr<ExponentialBackoff> build() {
            auto result = std::make_unique<ExponentialBackoff>();
            result->initial_delay_ = initial_delay_;
            result->max_delay_ = max_delay_;
            result->multiplier_ = multiplier_;
            result->jitter_factor_ = jitter_factor_;
            result->max_attempts_ = max_attempts_;
            return result;
        }
        
    private:
        std::chrono::milliseconds initial_delay_{1000};
        std::chrono::milliseconds max_delay_{60000};
        double multiplier_ = 2.0;
        double jitter_factor_ = 0.3;
        int max_attempts_ = 10;
    };
    
    ExponentialBackoff() = default;
    
    std::chrono::milliseconds next_delay() override {
        if (should_stop()) {
            return max_delay_;  // Return max if exceeded
        }
        
        // Calculate base delay: initial * multiplier^attempt
        double base_ms = static_cast<double>(initial_delay_.count()) 
                       * std::pow(multiplier_, attempt_);
        
        // Cap at max delay
        base_ms = std::min(base_ms, static_cast<double>(max_delay_.count()));
        
        // Apply jitter: delay * (1 + random(-jitter, +jitter))
        double jittered_ms = base_ms;
        if (jitter_factor_ > 0.0) {
            std::uniform_real_distribution<double> dist(-jitter_factor_, jitter_factor_);
            jittered_ms = base_ms * (1.0 + dist(rng_));
        }
        
        // Ensure non-negative
        jittered_ms = std::max(0.0, jittered_ms);
        
        attempt_++;
        return std::chrono::milliseconds(static_cast<int64_t>(jittered_ms));
    }
    
    void reset() override {
        attempt_ = 0;
    }
    
    bool should_stop() const override {
        return max_attempts_ > 0 && attempt_ >= max_attempts_;
    }
    
    int current_attempt() const override {
        return attempt_ + 1;  // 1-based for user display
    }
    
    int max_attempts() const override {
        return max_attempts_;
    }
    
    std::unique_ptr<BackoffStrategy> clone() const override {
        auto copy = std::make_unique<ExponentialBackoff>();
        copy->initial_delay_ = initial_delay_;
        copy->max_delay_ = max_delay_;
        copy->multiplier_ = multiplier_;
        copy->jitter_factor_ = jitter_factor_;
        copy->max_attempts_ = max_attempts_;
        copy->attempt_ = 0;  // Reset attempt count on clone
        return copy;
    }
    
    /// Create a builder for fluent configuration
    static Builder builder() {
        return Builder{};
    }
    
    /// Preset: Aggressive (fast retries for dev/testing)
    static std::unique_ptr<ExponentialBackoff> aggressive() {
        return builder()
            .initial_delay(std::chrono::milliseconds(100))
            .max_delay(std::chrono::seconds(5))
            .multiplier(1.5)
            .jitter(0.1)
            .max_attempts(20)
            .build();
    }
    
    /// Preset: Conservative (production-friendly)
    static std::unique_ptr<ExponentialBackoff> conservative() {
        return builder()
            .initial_delay(std::chrono::seconds(1))
            .max_delay(std::chrono::minutes(2))
            .multiplier(2.0)
            .jitter(0.3)
            .max_attempts(10)
            .build();
    }
    
    /// Preset: Infinite (never give up)
    static std::unique_ptr<ExponentialBackoff> infinite() {
        return builder()
            .initial_delay(std::chrono::seconds(1))
            .max_delay(std::chrono::minutes(5))
            .multiplier(2.0)
            .jitter(0.3)
            .max_attempts(0)  // 0 = infinite
            .build();
    }

private:
    std::chrono::milliseconds initial_delay_{1000};
    std::chrono::milliseconds max_delay_{60000};
    double multiplier_ = 2.0;
    double jitter_factor_ = 0.3;
    int max_attempts_ = 10;
    int attempt_ = 0;
    
    // Thread-local RNG for jitter (each instance gets its own)
    mutable std::mt19937 rng_{std::random_device{}()};
};

} // namespace kraken
