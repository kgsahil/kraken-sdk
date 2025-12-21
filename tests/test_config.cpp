/// @file test_config.cpp
/// @brief Unit tests for configuration builder

#include <gtest/gtest.h>
#include <kraken/config.hpp>

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
    EXPECT_FALSE(config.is_authenticated());
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

// Test reconnect delay
TEST_F(ConfigTest, SetReconnectDelay) {
    auto delay = std::chrono::milliseconds(2000);
    auto config = ClientConfig::Builder()
        .reconnect_delay(delay)
        .build();
    
    EXPECT_EQ(config.reconnect_delay(), delay);
}

// Test checksum validation
TEST_F(ConfigTest, SetValidateChecksums) {
    auto config = ClientConfig::Builder()
        .validate_checksums(false)
        .build();
    
    EXPECT_FALSE(config.validate_checksums());
}

// Test API key/secret
TEST_F(ConfigTest, SetAPIKey) {
    auto config = ClientConfig::Builder()
        .api_key("test_key")
        .build();
    
    EXPECT_EQ(config.api_key(), "test_key");
    EXPECT_FALSE(config.is_authenticated()); // No secret
}

TEST_F(ConfigTest, SetAPISecret) {
    auto config = ClientConfig::Builder()
        .api_secret("test_secret")
        .build();
    
    EXPECT_EQ(config.api_secret(), "test_secret");
    EXPECT_FALSE(config.is_authenticated()); // No key
}

TEST_F(ConfigTest, AuthenticatedConfig) {
    auto config = ClientConfig::Builder()
        .api_key("test_key")
        .api_secret("test_secret")
        .build();
    
    EXPECT_TRUE(config.is_authenticated());
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

