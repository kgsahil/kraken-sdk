#include "kraken/rate_limiter.hpp"

#include <algorithm>
#include <thread>

namespace kraken {

RateLimiter::RateLimiter(double requests_per_sec, size_t burst_size, bool enabled)
    : tokens_(static_cast<double>(burst_size))
    , refill_rate_(requests_per_sec)
    , max_tokens_(burst_size)
    , enabled_(enabled)
    , last_refill_(std::chrono::steady_clock::now())
{
    if (requests_per_sec <= 0.0) {
        refill_rate_ = 0.0;
        tokens_ = 0.0;
        enabled_ = true; // keep limiter enabled so acquire() can fail
    }
    if (burst_size == 0) {
        max_tokens_ = 1;  // Minimum burst size
        tokens_ = std::min(tokens_, static_cast<double>(max_tokens_));
    }
}

bool RateLimiter::acquire() {
    if (!enabled_.load(std::memory_order_acquire)) {
        total_requests_.fetch_add(1, std::memory_order_relaxed);
        allowed_requests_.fetch_add(1, std::memory_order_relaxed);
        return true;
    }
    
    total_requests_.fetch_add(1, std::memory_order_relaxed);
    
    std::lock_guard<std::mutex> lock(mutex_);
    refill_tokens();
    
    if (tokens_ >= 1.0 && refill_rate_ > 0.0) {
        tokens_ -= 1.0;
        allowed_requests_.fetch_add(1, std::memory_order_relaxed);
        return true;
    }
    
    rate_limited_.fetch_add(1, std::memory_order_relaxed);
    return false;
}

bool RateLimiter::acquire_blocking(std::chrono::milliseconds timeout) {
    if (!enabled_.load(std::memory_order_acquire)) {
        total_requests_.fetch_add(1, std::memory_order_relaxed);
        allowed_requests_.fetch_add(1, std::memory_order_relaxed);
        return true;
    }
    
    total_requests_.fetch_add(1, std::memory_order_relaxed);
    
    auto start = std::chrono::steady_clock::now();
    
    while (true) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            refill_tokens();
            
            if (tokens_ >= 1.0) {
                tokens_ -= 1.0;
                allowed_requests_.fetch_add(1, std::memory_order_relaxed);
                return true;
            }
        }
        
        // Check timeout
        if (timeout.count() > 0) {
            auto elapsed = std::chrono::steady_clock::now() - start;
            if (elapsed >= timeout) {
                rate_limited_.fetch_add(1, std::memory_order_relaxed);
                return false;
            }
        }
        
        // Sleep briefly to avoid busy-waiting
        auto wait = wait_time();
        if (wait.count() > 0) {
            std::this_thread::sleep_for(std::min(wait, std::chrono::milliseconds(100)));
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
}

std::chrono::milliseconds RateLimiter::wait_time() const {
    if (!enabled_.load(std::memory_order_acquire)) {
        return std::chrono::milliseconds(0);
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    refill_tokens();
    
    if (tokens_ >= 1.0 && refill_rate_ > 0.0) {
        return std::chrono::milliseconds(0);
    }
    
    // Calculate time needed to get 1 token
    double tokens_needed = 1.0 - tokens_;
    if (refill_rate_ <= 0.0) {
        return std::chrono::milliseconds(1000);  // Default 1 second if rate is 0
    }
    
    double seconds_needed = tokens_needed / refill_rate_;
    auto ms = static_cast<int64_t>(seconds_needed * 1000.0);
    return std::chrono::milliseconds(std::max(ms, int64_t(1)));
}

double RateLimiter::tokens() const {
    if (!enabled_.load(std::memory_order_acquire)) {
        return static_cast<double>(max_tokens_);
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    refill_tokens();
    return tokens_;
}

void RateLimiter::set_enabled(bool enabled) {
    enabled_.store(enabled, std::memory_order_release);
}

void RateLimiter::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    tokens_ = static_cast<double>(max_tokens_);
    last_refill_ = std::chrono::steady_clock::now();
}

RateLimiter::Stats RateLimiter::get_stats() const {
    Stats stats;
    stats.total_requests = total_requests_.load(std::memory_order_acquire);
    stats.allowed_requests = allowed_requests_.load(std::memory_order_acquire);
    stats.rate_limited = rate_limited_.load(std::memory_order_acquire);
    
    {
        std::lock_guard<std::mutex> lock(mutex_);
        refill_tokens();
        stats.current_tokens = tokens_;
    }
    
    return stats;
}

void RateLimiter::refill_tokens() const {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - last_refill_).count();
    
    if (elapsed <= 0) {
        return;
    }
    
    // Refill tokens based on elapsed time
    double seconds_elapsed = elapsed / 1000.0;
    double tokens_to_add = refill_rate_ * seconds_elapsed;
    
    tokens_ = std::min(static_cast<double>(max_tokens_), tokens_ + tokens_to_add);
    last_refill_ = now;
}

} // namespace kraken

