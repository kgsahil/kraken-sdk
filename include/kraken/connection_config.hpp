#pragma once

/// @file connection_config.hpp
/// @brief Connection timeout and security configuration
/// 
/// Provides configuration structures for WebSocket connection timeouts
/// and TLS/SSL security settings.

#include <string>
#include <chrono>

namespace kraken {

namespace {
    // Default timeout constants (in milliseconds/seconds)
    constexpr std::chrono::milliseconds DEFAULT_CONNECT_TIMEOUT_MS{10000};  // 10 seconds
    constexpr std::chrono::milliseconds DEFAULT_READ_TIMEOUT_MS{30000};     // 30 seconds
    constexpr std::chrono::milliseconds DEFAULT_WRITE_TIMEOUT_MS{10000};     // 10 seconds
    constexpr std::chrono::seconds DEFAULT_PING_INTERVAL_SEC{30};           // 30 seconds
    constexpr std::chrono::seconds DEFAULT_PONG_TIMEOUT_SEC{10};            // 10 seconds
}

/// @brief Connection timeout configuration
/// 
/// Configures various timeouts for WebSocket connection operations.
struct ConnectionTimeouts {
    std::chrono::milliseconds connect_timeout{DEFAULT_CONNECT_TIMEOUT_MS};  ///< Connection establishment timeout (default: 10s)
    std::chrono::milliseconds read_timeout{DEFAULT_READ_TIMEOUT_MS};        ///< Read operation timeout (default: 30s)
    std::chrono::milliseconds write_timeout{DEFAULT_WRITE_TIMEOUT_MS};      ///< Write operation timeout (default: 10s)
    std::chrono::seconds ping_interval{DEFAULT_PING_INTERVAL_SEC};           ///< WebSocket ping interval (default: 30s)
    std::chrono::seconds pong_timeout{DEFAULT_PONG_TIMEOUT_SEC};            ///< Pong response timeout (default: 10s)
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

