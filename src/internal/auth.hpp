/// @file auth.hpp
/// @brief Internal authentication helpers
/// 
/// Provides utilities for generating authentication tokens and HMAC signatures
/// required for authenticated WebSocket connections.
/// 
/// @note This is an internal header. Users configure authentication via ClientConfig.

#pragma once

#include <string>
#include <openssl/hmac.h>
#include <openssl/sha.h>
#include <iomanip>
#include <sstream>

namespace kraken {

/// @brief Authentication helper for Kraken WebSocket API
/// 
/// Provides utilities for generating authentication tokens and HMAC signatures
/// required for authenticated WebSocket connections.
/// 
/// @note This is an internal class. Users should configure API keys via ClientConfig.
class Auth {
public:
    /// @brief Generate authentication token for WebSocket connection
    /// 
    /// Creates a signed authentication token using HMAC-SHA512 as required
    /// by Kraken's WebSocket API v2.
    /// 
    /// @param api_key API key
    /// @param api_secret API secret
    /// @return Authentication token as JSON string ready to send
    static std::string generate_auth_token(const std::string& api_key, 
                                           const std::string& api_secret);
    
    /// @brief Generate HMAC-SHA512 signature
    /// 
    /// Computes HMAC-SHA512 signature of a message using a secret key.
    /// 
    /// @param message Message to sign
    /// @param secret Secret key
    /// @return Hex-encoded signature (lowercase)
    static std::string hmac_sha512(const std::string& message, 
                                   const std::string& secret);
    
    /// @brief Generate nonce (timestamp in milliseconds)
    /// 
    /// Creates a unique nonce based on current timestamp for authentication.
    /// 
    /// @return Nonce as string (milliseconds since epoch)
    static std::string generate_nonce();
};

} // namespace kraken

