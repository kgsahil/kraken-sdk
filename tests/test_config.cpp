/// @file test_config.cpp
/// @brief Unit tests for configuration builder

#include <gtest/gtest.h>
#include <kraken/core/config.hpp>
#include <kraken/connection/backoff.hpp>

using namespace kraken;

class ConfigTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

// Test default config
TEST_F(ConfigTest, DefaultConfig) {
    auto config = ClientConfig::Builder().build();
    
    EXPECT_EQ(config.url(), "wss://ws.kraken.com/v2");
    EXPECT_EQ(config.queue_capacity(), 65536);
    EXPECT_EQ(config.reconnect_attempts(), 10);
    EXPECT_TRUE(config.validate_checksums());
    // Authentication logic tested in test_authentication.cpp
}

// Test URL setting
TEST_F(ConfigTest, SetURL) {
    auto config = ClientConfig::Builder()
        .url("wss://beta-ws.kraken.com/v2")
        .build();
    
    EXPECT_EQ(config.url(), "wss://beta-ws.kraken.com/v2");
}

// Test queue capacity
TEST_F(ConfigTest, SetQueueCapacity) {
    auto config = ClientConfig::Builder()
        .queue_capacity(131072)
        .build();
    
    EXPECT_EQ(config.queue_capacity(), 131072);
}

// Test reconnect attempts
TEST_F(ConfigTest, SetReconnectAttempts) {
    auto config = ClientConfig::Builder()
        .reconnect_attempts(5)
        .build();
    
    EXPECT_EQ(config.reconnect_attempts(), 5);
}

// Test backoff strategy (replaces legacy reconnect_delay)
TEST_F(ConfigTest, SetBackoffStrategy) {
    auto backoff_ptr = ExponentialBackoff::builder()
        .initial_delay(std::chrono::milliseconds(500))
        .max_delay(std::chrono::seconds(30))
        .multiplier(1.5)
        .jitter(0.2)
        .max_attempts(5)
        .build();
    
    auto config = ClientConfig::Builder()
        .backoff(std::shared_ptr<BackoffStrategy>(std::move(backoff_ptr)))
        .build();
    
    auto backoff = config.backoff_strategy();
    ASSERT_NE(backoff, nullptr);
    EXPECT_EQ(backoff->max_attempts(), 5);
    
    // Test that backoff produces increasing delays
    auto delay1 = backoff->next_delay();
    auto delay2 = backoff->next_delay();
    // With multiplier 1.5 and some jitter, delay2 should be roughly 1.5x delay1
    EXPECT_GT(delay2.count(), delay1.count() * 1.0);  // At least same (with jitter)
    EXPECT_LT(delay2.count(), delay1.count() * 2.5);  // But not too much more
}

// Test checksum validation
TEST_F(ConfigTest, SetValidateChecksums) {
    auto config = ClientConfig::Builder()
        .validate_checksums(false)
        .build();
    
    EXPECT_FALSE(config.validate_checksums());
}

// Test API key/secret (authentication logic tested in test_authentication.cpp)
TEST_F(ConfigTest, SetAPIKey) {
    auto config = ClientConfig::Builder()
        .api_key("test_key")
        .build();
    
    EXPECT_EQ(config.api_key(), "test_key");
}

TEST_F(ConfigTest, SetAPISecret) {
    auto config = ClientConfig::Builder()
        .api_secret("test_secret")
        .build();
    
    EXPECT_EQ(config.api_secret(), "test_secret");
}

// Test fluent builder
TEST_F(ConfigTest, FluentBuilder) {
    auto config = ClientConfig::Builder()
        .url("wss://ws.kraken.com/v2")
        .queue_capacity(32768)
        .reconnect_attempts(5)
        .reconnect_delay(std::chrono::milliseconds(1000))
        .validate_checksums(true)
        .api_key("key")
        .api_secret("secret")
        .build();
    
    EXPECT_EQ(config.url(), "wss://ws.kraken.com/v2");
    EXPECT_EQ(config.queue_capacity(), 32768);
    EXPECT_EQ(config.reconnect_attempts(), 5);
    EXPECT_TRUE(config.validate_checksums());
    EXPECT_TRUE(config.is_authenticated());
}

// Test multiple builds create independent configs
TEST_F(ConfigTest, MultipleBuilds) {
    auto builder = ClientConfig::Builder();
    
    auto config1 = builder.queue_capacity(1000).build();
    auto config2 = builder.queue_capacity(2000).build();
    
    // Builder should reset or reuse
    EXPECT_EQ(config1.queue_capacity(), 1000);
    EXPECT_EQ(config2.queue_capacity(), 2000);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

