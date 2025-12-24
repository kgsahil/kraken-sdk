/// @file test_authentication.cpp
/// @brief Unit tests for authentication checks and error handling

#include <gtest/gtest.h>
#include <kraken/core/client.hpp>
#include <kraken/core/config.hpp>
#include <kraken/core/error.hpp>

using namespace kraken;

class AuthenticationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create config without authentication
        config_no_auth_ = ClientConfig::Builder()
            .url("wss://ws.kraken.com")
            .build();
        
        // Create config with authentication
        config_with_auth_ = ClientConfig::Builder()
            .url("wss://ws.kraken.com")
            .api_key("test_key")
            .api_secret("test_secret")
            .build();
    }
    
    ClientConfig config_no_auth_;
    ClientConfig config_with_auth_;
};

TEST_F(AuthenticationTest, ConfigIsAuthenticated) {
    EXPECT_FALSE(config_no_auth_.is_authenticated());
    EXPECT_TRUE(config_with_auth_.is_authenticated());
    
    // Partial auth should fail
    ClientConfig partial_key = ClientConfig::Builder()
        .url("wss://ws.kraken.com")
        .api_key("test_key")
        .build();
    EXPECT_FALSE(partial_key.is_authenticated());
    
    ClientConfig partial_secret = ClientConfig::Builder()
        .url("wss://ws.kraken.com")
        .api_secret("test_secret")
        .build();
    EXPECT_FALSE(partial_secret.is_authenticated());
}

TEST_F(AuthenticationTest, SubscribeOwnTradesRequiresAuth) {
    KrakenClient client(config_no_auth_);
    
    // Should throw AuthenticationError when not authenticated
    EXPECT_THROW({
        try {
            client.subscribe_own_trades();
        } catch (const AuthenticationError& e) {
            EXPECT_NE(std::string(e.what()).find("API key"), std::string::npos);
            throw;
        }
    }, AuthenticationError);
    
    // Should not throw when authenticated
    KrakenClient client_auth(config_with_auth_);
    // Note: This will fail at connection time, but subscription should be allowed
    // We can't fully test without a real connection, but the check should pass
    EXPECT_NO_THROW({
        // This will create subscription object, but won't actually connect
        // The authentication check happens before connection
        auto sub = client_auth.subscribe_own_trades();
        (void)sub;  // Suppress unused warning
    });
}

TEST_F(AuthenticationTest, SubscribeOpenOrdersRequiresAuth) {
    KrakenClient client(config_no_auth_);
    
    // Should throw AuthenticationError when not authenticated
    EXPECT_THROW({
        try {
            client.subscribe_open_orders();
        } catch (const AuthenticationError& e) {
            EXPECT_NE(std::string(e.what()).find("API key"), std::string::npos);
            throw;
        }
    }, AuthenticationError);
    
    // Should not throw when authenticated
    KrakenClient client_auth(config_with_auth_);
    EXPECT_NO_THROW({
        auto sub = client_auth.subscribe_open_orders();
        (void)sub;
    });
}

TEST_F(AuthenticationTest, SubscribeBalancesRequiresAuth) {
    KrakenClient client(config_no_auth_);
    
    // Should throw AuthenticationError when not authenticated
    EXPECT_THROW({
        try {
            client.subscribe_balances();
        } catch (const AuthenticationError& e) {
            EXPECT_NE(std::string(e.what()).find("API key"), std::string::npos);
            throw;
        }
    }, AuthenticationError);
    
    // Should not throw when authenticated
    KrakenClient client_auth(config_with_auth_);
    EXPECT_NO_THROW({
        auto sub = client_auth.subscribe_balances();
        (void)sub;
    });
}

TEST_F(AuthenticationTest, PublicChannelsDontRequireAuth) {
    KrakenClient client(config_no_auth_);
    
    // Public channels should work without authentication
    EXPECT_NO_THROW({
        auto sub = client.subscribe(Channel::Ticker, {"BTC/USD"});
        (void)sub;
        
        auto sub2 = client.subscribe(Channel::Trade, {"ETH/USD"});
        (void)sub2;
        
        auto sub3 = client.subscribe_book({"BTC/USD"}, 10);
        (void)sub3;
    });
}

TEST_F(AuthenticationTest, CallbackRegistrationDoesntRequireAuth) {
    KrakenClient client(config_no_auth_);
    
    // Callback registration should work without authentication
    // (they just won't be called if there's no data)
    EXPECT_NO_THROW({
        client.on_order([](const Order&) {});
        client.on_own_trade([](const OwnTrade&) {});
        client.on_balance([](const std::unordered_map<std::string, Balance>&) {});
    });
}

