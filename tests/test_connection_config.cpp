/// @file test_connection_config.cpp
/// @brief Unit tests for connection configuration (timeouts, security)

#include <gtest/gtest.h>
#include "kraken/connection/connection_config.hpp"
#include <chrono>

using namespace kraken;

class ConnectionConfigTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Set up test fixtures
    }
    
    void TearDown() override {
        // Clean up
    }
};

// Test default ConnectionTimeouts
TEST_F(ConnectionConfigTest, DefaultTimeouts) {
    ConnectionTimeouts timeouts;
    
    EXPECT_GT(timeouts.connect_timeout.count(), 0);
    EXPECT_GT(timeouts.read_timeout.count(), 0);
    EXPECT_GT(timeouts.write_timeout.count(), 0);
    EXPECT_GT(timeouts.ping_interval.count(), 0);
    EXPECT_GT(timeouts.pong_timeout.count(), 0);
}

// Test custom ConnectionTimeouts
TEST_F(ConnectionConfigTest, CustomTimeouts) {
    ConnectionTimeouts timeouts;
    timeouts.connect_timeout = std::chrono::milliseconds(5000);
    timeouts.read_timeout = std::chrono::milliseconds(30000);
    timeouts.write_timeout = std::chrono::milliseconds(10000);
    timeouts.ping_interval = std::chrono::seconds(20);
    timeouts.pong_timeout = std::chrono::seconds(5);
    
    EXPECT_EQ(timeouts.connect_timeout.count(), 5000);
    EXPECT_EQ(timeouts.read_timeout.count(), 30000);
    EXPECT_EQ(timeouts.write_timeout.count(), 10000);
    EXPECT_EQ(timeouts.ping_interval.count(), 20);
    EXPECT_EQ(timeouts.pong_timeout.count(), 5);
}

// Test default SecurityConfig
TEST_F(ConnectionConfigTest, DefaultSecurity) {
    SecurityConfig security;
    
    EXPECT_TRUE(security.verify_peer);  // Should verify by default
    EXPECT_TRUE(security.ca_cert_path.empty());
    EXPECT_TRUE(security.client_cert_path.empty());
    EXPECT_TRUE(security.client_key_path.empty());
    EXPECT_TRUE(security.cipher_suites.empty());
    EXPECT_FALSE(security.allow_insecure);
}

// Test custom SecurityConfig
TEST_F(ConnectionConfigTest, CustomSecurity) {
    SecurityConfig security;
    security.verify_peer = false;
    security.ca_cert_path = "/path/to/ca.crt";
    security.client_cert_path = "/path/to/client.crt";
    security.client_key_path = "/path/to/client.key";
    security.cipher_suites = "ECDHE-RSA-AES256-GCM-SHA384";
    security.allow_insecure = true;
    
    EXPECT_FALSE(security.verify_peer);
    EXPECT_EQ(security.ca_cert_path, "/path/to/ca.crt");
    EXPECT_EQ(security.client_cert_path, "/path/to/client.crt");
    EXPECT_EQ(security.client_key_path, "/path/to/client.key");
    EXPECT_EQ(security.cipher_suites, "ECDHE-RSA-AES256-GCM-SHA384");
    EXPECT_TRUE(security.allow_insecure);
}

// Test SecurityConfig copy
TEST_F(ConnectionConfigTest, SecurityConfigCopy) {
    SecurityConfig security1;
    security1.verify_peer = false;
    security1.ca_cert_path = "/test/ca.crt";
    
    SecurityConfig security2 = security1;
    
    EXPECT_EQ(security2.verify_peer, security1.verify_peer);
    EXPECT_EQ(security2.ca_cert_path, security1.ca_cert_path);
}

// Test ConnectionTimeouts copy
TEST_F(ConnectionConfigTest, TimeoutsCopy) {
    ConnectionTimeouts timeouts1;
    timeouts1.connect_timeout = std::chrono::milliseconds(1000);
    
    ConnectionTimeouts timeouts2 = timeouts1;
    
    EXPECT_EQ(timeouts2.connect_timeout.count(), timeouts1.connect_timeout.count());
}

// Test edge cases - zero timeouts
TEST_F(ConnectionConfigTest, ZeroTimeouts) {
    ConnectionTimeouts timeouts;
    timeouts.connect_timeout = std::chrono::milliseconds(0);
    timeouts.read_timeout = std::chrono::milliseconds(0);
    timeouts.write_timeout = std::chrono::milliseconds(0);
    
    EXPECT_EQ(timeouts.connect_timeout.count(), 0);
    EXPECT_EQ(timeouts.read_timeout.count(), 0);
    EXPECT_EQ(timeouts.write_timeout.count(), 0);
}

// Test very large timeouts
TEST_F(ConnectionConfigTest, LargeTimeouts) {
    ConnectionTimeouts timeouts;
    timeouts.connect_timeout = std::chrono::hours(1);
    timeouts.read_timeout = std::chrono::hours(2);
    
    EXPECT_EQ(timeouts.connect_timeout.count(), 3600000);  // 1 hour in ms
    EXPECT_EQ(timeouts.read_timeout.count(), 7200000);     // 2 hours in ms
}

