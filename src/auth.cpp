#include "internal/auth.hpp"

#include <chrono>
#include <cstring>

namespace kraken {

std::string Auth::generate_nonce() {
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count();
    return std::to_string(ms);
}

std::string Auth::hmac_sha512(const std::string& message, const std::string& secret) {
    unsigned char* digest = HMAC(EVP_sha512(), 
                                  secret.c_str(), static_cast<int>(secret.length()),
                                  reinterpret_cast<const unsigned char*>(message.c_str()),
                                  static_cast<int>(message.length()),
                                  nullptr, nullptr);
    
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (int i = 0; i < SHA512_DIGEST_LENGTH; ++i) {
        oss << std::setw(2) << static_cast<unsigned int>(digest[i]);
    }
    return oss.str();
}

std::string Auth::generate_auth_token(const std::string& api_key, 
                                     const std::string& api_secret) {
    // Kraken WebSocket v2 authentication format
    // Generate nonce (timestamp)
    std::string nonce = generate_nonce();
    
    // Create message to sign: nonce + api_key
    std::string message = nonce + api_key;
    
    // Generate signature
    std::string signature = hmac_sha512(message, api_secret);
    
    // Build authentication message
    // Format: {"method":"subscribe","params":{"token":"<token>"}}
    // Token format: <api_key>:<nonce>:<signature>
    std::string token = api_key + ":" + nonce + ":" + signature;
    
    // Return authentication subscribe message
    return R"({"method":"subscribe","params":{"token":")" + token + R"("}})";
}

} // namespace kraken

