/// @file test_config_env.cpp
/// @brief Unit tests for environment variable configuration

#include <gtest/gtest.h>
#include "kraken/config_env.hpp"
#include <cstdlib>
#include <string>

#ifdef _WIN32
#include <stdlib.h>
#define setenv(name, value, overwrite) _putenv_s(name, value)
#define unsetenv(name) _putenv_s(name, "")
#endif

using namespace kraken;

class ConfigEnvTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Clear relevant environment variables
        unsetenv("KRAKEN_WS_URL");
        unsetenv("KRAKEN_API_KEY");
        unsetenv("KRAKEN_API_SECRET");
        unsetenv("ENABLE_SPSC_QUEUE");
        unsetenv("SPSC_QUEUE_SIZE");
        unsetenv("RATE_LIMIT_ENABLED");
        unsetenv("RATE_LIMIT_REQUESTS_PER_SEC");
        unsetenv("RATE_LIMIT_BURST_SIZE");
    }
    
    void TearDown() override {
        // Clean up environment variables
        unsetenv("KRAKEN_WS_URL");
        unsetenv("KRAKEN_API_KEY");
        unsetenv("KRAKEN_API_SECRET");
        unsetenv("ENABLE_SPSC_QUEUE");
        unsetenv("SPSC_QUEUE_SIZE");
        unsetenv("RATE_LIMIT_ENABLED");
        unsetenv("RATE_LIMIT_REQUESTS_PER_SEC");
        unsetenv("RATE_LIMIT_BURST_SIZE");
    }
};

// Test default configuration
TEST_F(ConfigEnvTest, DefaultConfig) {
    // Should not throw with defaults
    ClientConfig config = config_from_env();
    
    EXPECT_FALSE(config.url().empty());
    EXPECT_FALSE(config.is_authenticated());  // No keys set
}

// Test WebSocket URL
TEST_F(ConfigEnvTest, WebSocketURL) {
    setenv("KRAKEN_WS_URL", "wss://test.example.com/v2", 1);
    
    ClientConfig config = config_from_env();
    EXPECT_EQ(config.url(), "wss://test.example.com/v2");
}

// Test API credentials
TEST_F(ConfigEnvTest, APICredentials) {
    setenv("KRAKEN_API_KEY", "test_key", 1);
    setenv("KRAKEN_API_SECRET", "test_secret", 1);
    
    ClientConfig config = config_from_env();
    EXPECT_TRUE(config.is_authenticated());
    EXPECT_EQ(config.api_key(), "test_key");
    EXPECT_EQ(config.api_secret(), "test_secret");
}

// Test queue configuration
TEST_F(ConfigEnvTest, QueueConfig) {
    setenv("ENABLE_SPSC_QUEUE", "true", 1);
    setenv("SPSC_QUEUE_SIZE", "131072", 1);
    
    ClientConfig config = config_from_env();
    EXPECT_TRUE(config.use_queue());
    EXPECT_EQ(config.queue_capacity(), 131072);
}

// Test queue disabled
TEST_F(ConfigEnvTest, QueueDisabled) {
    setenv("ENABLE_SPSC_QUEUE", "false", 1);
    
    ClientConfig config = config_from_env();
    EXPECT_FALSE(config.use_queue());
}

// Test rate limiting
TEST_F(ConfigEnvTest, RateLimiting) {
    setenv("RATE_LIMIT_ENABLED", "true", 1);
    setenv("RATE_LIMIT_REQUESTS_PER_SEC", "20.0", 1);
    setenv("RATE_LIMIT_BURST_SIZE", "50", 1);
    
    ClientConfig config = config_from_env();
    EXPECT_TRUE(config.rate_limiting_enabled());
    EXPECT_NE(config.rate_limiter(), nullptr);
}

// Test rate limiting disabled
TEST_F(ConfigEnvTest, RateLimitingDisabled) {
    setenv("RATE_LIMIT_ENABLED", "false", 1);
    
    ClientConfig config = config_from_env();
    EXPECT_FALSE(config.rate_limiting_enabled());
}

// Test get_env helper
TEST_F(ConfigEnvTest, GetEnvHelper) {
    setenv("TEST_VAR", "test_value", 1);
    
    std::string value = get_env("TEST_VAR");
    EXPECT_EQ(value, "test_value");
    
    std::string default_val = get_env("NONEXISTENT_VAR", "default");
    EXPECT_EQ(default_val, "default");
}

// Test get_env_bool helper
TEST_F(ConfigEnvTest, GetEnvBoolHelper) {
    setenv("BOOL_TRUE", "true", 1);
    setenv("BOOL_FALSE", "false", 1);
    setenv("BOOL_1", "1", 1);
    setenv("BOOL_0", "0", 1);
    
    EXPECT_TRUE(get_env_bool("BOOL_TRUE"));
    EXPECT_FALSE(get_env_bool("BOOL_FALSE"));
    EXPECT_TRUE(get_env_bool("BOOL_1"));
    EXPECT_FALSE(get_env_bool("BOOL_0"));
    EXPECT_FALSE(get_env_bool("NONEXISTENT", false));
    EXPECT_TRUE(get_env_bool("NONEXISTENT", true));
}

// Test get_env_int helper
TEST_F(ConfigEnvTest, GetEnvIntHelper) {
    setenv("INT_VAR", "42", 1);
    
    int value = get_env_int("INT_VAR");
    EXPECT_EQ(value, 42);
    
    int default_val = get_env_int("NONEXISTENT", 100);
    EXPECT_EQ(default_val, 100);
}

// Test get_env_double helper
TEST_F(ConfigEnvTest, GetEnvDoubleHelper) {
    setenv("DOUBLE_VAR", "3.14", 1);
    
    double value = get_env_double("DOUBLE_VAR");
    EXPECT_DOUBLE_EQ(value, 3.14);
    
    double default_val = get_env_double("NONEXISTENT", 2.5);
    EXPECT_DOUBLE_EQ(default_val, 2.5);
}

// Test get_env_size_t helper
TEST_F(ConfigEnvTest, GetEnvSizeTHelper) {
    setenv("SIZE_VAR", "65536", 1);
    
    size_t value = get_env_size_t("SIZE_VAR");
    EXPECT_EQ(value, 65536);
    
    size_t default_val = get_env_size_t("NONEXISTENT", 1024);
    EXPECT_EQ(default_val, 1024);
}

// Test invalid URL (should throw)
TEST_F(ConfigEnvTest, InvalidURL) {
    setenv("KRAKEN_WS_URL", "", 1);
    
    // Should throw because URL is required
    EXPECT_THROW(config_from_env(), std::runtime_error);
}

