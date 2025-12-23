#pragma once

/// @file connection_config.hpp
/// @brief Connection timeout and security configuration
/// 
/// Provides configuration structures for WebSocket connection timeouts
/// and TLS/SSL security settings.

#include <string>
#include <chrono>

namespace kraken {

/// @brief Connection timeout configuration
/// 
/// Configures various timeouts for WebSocket connection operations.
struct ConnectionTimeouts {
    std::chrono::milliseconds connect_timeout{std::chrono::milliseconds(10000)};  ///< Connection establishment timeout (default: 10s)
    std::chrono::milliseconds read_timeout{std::chrono::milliseconds(30000)};      ///< Read operation timeout (default: 30s)
    std::chrono::milliseconds write_timeout{std::chrono::milliseconds(10000)};      ///< Write operation timeout (default: 10s)
    std::chrono::seconds ping_interval{std::chrono::seconds(30)};                  ///< WebSocket ping interval (default: 30s)
    std::chrono::seconds pong_timeout{std::chrono::seconds(10)};                  ///< Pong response timeout (default: 10s)
};

/// @brief TLS/Security configuration
/// 
/// Configures TLS/SSL settings for secure WebSocket connections.
struct SecurityConfig {
    bool verify_peer = true;                    ///< Verify TLS certificates (default: true)
    std::string ca_cert_path;                   ///< Custom CA certificate path (empty = system default)
    std::string client_cert_path;               ///< Client certificate path (optional, for mutual TLS)
    std::string client_key_path;                ///< Client private key path (required if client_cert_path set)
    std::string cipher_suites;                   ///< Allowed cipher suites (empty = system default)
    bool allow_insecure = false;                ///< Allow insecure connections (dev only, default: false)
};

} // namespace kraken

