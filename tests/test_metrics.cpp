/// @file test_metrics.cpp
/// @brief Unit tests for metrics tracking

#include <gtest/gtest.h>
#include <kraken/kraken.hpp>
#include <kraken/metrics.hpp>
#include <thread>
#include <chrono>

using namespace kraken;

class MetricsTest : public ::testing::Test {
protected:
    void SetUp() override {
        client_ = std::make_unique<KrakenClient>();
    }
    
    void TearDown() override {
        if (client_ && client_->is_running()) {
            client_->stop();
        }
    }
    
    std::unique_ptr<KrakenClient> client_;
};

// Test initial metrics
TEST_F(MetricsTest, InitialMetrics) {
    auto metrics = client_->get_metrics();
    
    EXPECT_EQ(metrics.messages_received, 0);
    EXPECT_EQ(metrics.messages_processed, 0);
    EXPECT_EQ(metrics.messages_dropped, 0);
    EXPECT_EQ(metrics.queue_depth, 0);
    EXPECT_EQ(metrics.connection_state, ConnectionState::Disconnected);
    EXPECT_EQ(metrics.latency_max_us.count(), 0);
}

// Test messages per second calculation
TEST_F(MetricsTest, MessagesPerSecond) {
    auto metrics = client_->get_metrics();
    
    // Initially zero
    EXPECT_DOUBLE_EQ(metrics.messages_per_second(), 0.0);
}

// Test uptime calculation
TEST_F(MetricsTest, Uptime) {
    auto metrics = client_->get_metrics();
    
    auto uptime = metrics.uptime();
    EXPECT_GE(uptime.count(), 0);
}

// Test uptime string format
TEST_F(MetricsTest, UptimeString) {
    auto metrics = client_->get_metrics();
    
    std::string uptime_str = metrics.uptime_string();
    EXPECT_FALSE(uptime_str.empty());
    // Should be in HH:MM:SS format
    EXPECT_EQ(uptime_str.length(), 8); // "00:00:00"
    EXPECT_EQ(uptime_str[2], ':');
    EXPECT_EQ(uptime_str[5], ':');
}

// Test metrics are thread-safe (can be called from multiple threads)
TEST_F(MetricsTest, ThreadSafeMetrics) {
    std::atomic<bool> done{false};
    std::vector<std::thread> threads;
    
    // Spawn multiple threads reading metrics
    for (int i = 0; i < 4; ++i) {
        threads.emplace_back([this, &done]() {
            while (!done) {
                auto metrics = client_->get_metrics();
                // Just verify we can read without crashing
                (void)metrics.messages_received;
                (void)metrics.messages_processed;
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        });
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    done = true;
    
    for (auto& t : threads) {
        t.join();
    }
}

// Test connection state in metrics
TEST_F(MetricsTest, ConnectionStateInMetrics) {
    auto metrics = client_->get_metrics();
    
    EXPECT_EQ(metrics.connection_state, ConnectionState::Disconnected);
}

// Test queue depth
TEST_F(MetricsTest, QueueDepth) {
    auto metrics = client_->get_metrics();
    
    EXPECT_EQ(metrics.queue_depth, 0);
}

// Test latency tracking
TEST_F(MetricsTest, LatencyTracking) {
    auto metrics = client_->get_metrics();
    
    EXPECT_EQ(metrics.latency_max_us.count(), 0);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

