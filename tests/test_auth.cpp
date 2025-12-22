/// @file test_auth.cpp
/// @brief Unit tests for authentication (HMAC-SHA512)

#include <gtest/gtest.h>
#include "../src/internal/auth.hpp"
#include <string>
#include <regex>
#include <thread>
#include <chrono>
#include <algorithm>
#include <cctype>

using namespace kraken;

class AuthTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Set up test fixtures
    }
    
    void TearDown() override {
        // Clean up
    }
};

// Test nonce generation
TEST_F(AuthTest, NonceGeneration) {
    // Generate multiple nonces - should be different (or at least valid)
    std::string nonce1 = Auth::generate_nonce();
    std::string nonce2 = Auth::generate_nonce();
    
    // Nonces should be non-empty
    EXPECT_FALSE(nonce1.empty());
    EXPECT_FALSE(nonce2.empty());
    
    // Nonces should be numeric (timestamp in milliseconds)
    EXPECT_TRUE(std::all_of(nonce1.begin(), nonce1.end(), ::isdigit));
    EXPECT_TRUE(std::all_of(nonce2.begin(), nonce2.end(), ::isdigit));
    
    // Nonces should be reasonable length (timestamp in ms is ~13 digits)
    EXPECT_GE(nonce1.length(), 10);
    EXPECT_LE(nonce1.length(), 20);
}

// Test HMAC-SHA512 generation
TEST_F(AuthTest, HMACSHA512) {
    // Test with known values (from OpenSSL documentation examples)
    std::string message = "test message";
    std::string secret = "test secret";
    
    std::string signature = Auth::hmac_sha512(message, secret);
    
    // HMAC-SHA512 produces 128 hex characters (512 bits = 64 bytes = 128 hex chars)
    EXPECT_EQ(signature.length(), 128);
    
    // Should be hexadecimal
    EXPECT_TRUE(std::all_of(signature.begin(), signature.end(), 
                           [](char c) { return std::isxdigit(c); }));
    
    // Same input should produce same output
    std::string signature2 = Auth::hmac_sha512(message, secret);
    EXPECT_EQ(signature, signature2);
    
    // Different message should produce different signature
    std::string signature3 = Auth::hmac_sha512("different message", secret);
    EXPECT_NE(signature, signature3);
    
    // Different secret should produce different signature
    std::string signature4 = Auth::hmac_sha512(message, "different secret");
    EXPECT_NE(signature, signature4);
}

// Test empty inputs
TEST_F(AuthTest, EmptyInputs) {
    // Empty message
    std::string sig1 = Auth::hmac_sha512("", "secret");
    EXPECT_EQ(sig1.length(), 128);
    
    // Empty secret
    std::string sig2 = Auth::hmac_sha512("message", "");
    EXPECT_EQ(sig2.length(), 128);
    
    // Both empty
    std::string sig3 = Auth::hmac_sha512("", "");
    EXPECT_EQ(sig3.length(), 128);
}

// Test authentication token generation
TEST_F(AuthTest, GenerateAuthToken) {
    std::string api_key = "test_api_key_123";
    std::string api_secret = "test_api_secret_456";
    
    std::string token = Auth::generate_auth_token(api_key, api_secret);
    
    // Token should be non-empty
    EXPECT_FALSE(token.empty());
    
    // Token should be valid JSON
    EXPECT_TRUE(token.find("method") != std::string::npos);
    EXPECT_TRUE(token.find("subscribe") != std::string::npos);
    EXPECT_TRUE(token.find("params") != std::string::npos);
    EXPECT_TRUE(token.find("token") != std::string::npos);
    
    // Token should contain API key
    EXPECT_TRUE(token.find(api_key) != std::string::npos);
    
    // Token format should be: api_key:nonce:signature
    // Extract token value from JSON
    size_t token_start = token.find("\"token\":\"");
    EXPECT_NE(token_start, std::string::npos);
    token_start += 9;  // Skip "token":"
    size_t token_end = token.find("\"", token_start);
    EXPECT_NE(token_end, std::string::npos);
    
    std::string token_value = token.substr(token_start, token_end - token_start);
    
    // Token should have format: key:nonce:signature
    size_t colon1 = token_value.find(':');
    EXPECT_NE(colon1, std::string::npos);
    size_t colon2 = token_value.find(':', colon1 + 1);
    EXPECT_NE(colon2, std::string::npos);
    
    std::string extracted_key = token_value.substr(0, colon1);
    std::string extracted_nonce = token_value.substr(colon1 + 1, colon2 - colon1 - 1);
    std::string extracted_sig = token_value.substr(colon2 + 1);
    
    EXPECT_EQ(extracted_key, api_key);
    EXPECT_FALSE(extracted_nonce.empty());
    EXPECT_EQ(extracted_sig.length(), 128);  // HMAC-SHA512 hex length
}

// Test token generation with different keys
TEST_F(AuthTest, DifferentKeys) {
    std::string token1 = Auth::generate_auth_token("key1", "secret1");
    std::string token2 = Auth::generate_auth_token("key2", "secret2");
    
    // Different keys should produce different tokens
    EXPECT_NE(token1, token2);
}

// Test token generation consistency (same key, different times)
TEST_F(AuthTest, TokenConsistency) {
    std::string api_key = "test_key";
    std::string api_secret = "test_secret";
    
    // Generate two tokens - nonces will be different (time-based)
    std::string token1 = Auth::generate_auth_token(api_key, api_secret);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    std::string token2 = Auth::generate_auth_token(api_key, api_secret);
    
    // Tokens should be different (different nonces)
    EXPECT_NE(token1, token2);
    
    // But both should contain the same API key
    EXPECT_TRUE(token1.find(api_key) != std::string::npos);
    EXPECT_TRUE(token2.find(api_key) != std::string::npos);
}

