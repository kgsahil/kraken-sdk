#pragma once

/// @file config_env.hpp
/// @brief Environment variable configuration helper
/// 
/// Provides utilities to build ClientConfig from environment variables.
/// This allows users to configure the SDK without modifying code.

#include "config.hpp"
#include <string>
#include <cstdlib>

namespace kraken {

/// Build ClientConfig from environment variables
/// 
/// Reads configuration from environment variables and builds a ClientConfig.
/// Environment variables are optional - if not set, defaults are used.
/// 
/// Supported environment variables:
/// 
/// **Authentication:**
/// - `KRAKEN_API_KEY` - API key for authentication
/// - `KRAKEN_API_SECRET` - API secret for authentication
/// 
/// **Connection:**
/// - `KRAKEN_WS_URL` - WebSocket URL (default: wss://ws.kraken.com/v2)
/// 
/// **Queue:**
/// - `ENABLE_SPSC_QUEUE` - Enable/disable SPSC queue (true/false, default: true)
/// - `SPSC_QUEUE_SIZE` - SPSC queue capacity (default: 65536)
/// 
/// **Retry:**
/// - `WS_CONN_RETRY_DELAY_MS` - Initial retry delay in milliseconds (default: 1000)
/// - `WS_CONN_RETRY_MULTIPLIER` - Retry delay multiplier (default: 2.0)
/// - `WS_CONN_RETRY_TIMES` - Maximum retry attempts (default: 10)
/// - `WS_CONN_RETRY_MAX_DELAY_MS` - Maximum retry delay in milliseconds (default: 30000)
/// - `WS_CONN_RETRY_JITTER` - Jitter factor 0.0-1.0 (default: 0.2)
/// 
/// **Data Integrity:**
/// - `VALIDATE_CHECKSUMS` - Enable/disable checksum validation (true/false, default: true)
/// - `ENABLE_GAP_DETECTION` - Enable/disable gap detection (true/false, default: false)
/// - `GAP_TOLERANCE` - Gap tolerance count (default: 0)
/// 
/// **Telemetry:**
/// - `ENABLE_TELEMETRY` - Enable/disable telemetry (true/false, default: false)
/// - `TELEMETRY_SERVICE_NAME` - Service name for telemetry (default: "kraken-sdk")
/// - `TELEMETRY_SERVICE_VERSION` - Service version (default: "1.0.0")
/// - `TELEMETRY_ENVIRONMENT` - Environment name (default: "production")
/// - `TELEMETRY_ENABLE_METRICS` - Enable metrics collection (default: true)
/// - `TELEMETRY_ENABLE_TRACES` - Enable tracing (default: false)
/// - `TELEMETRY_ENABLE_LOGS` - Enable log export (default: false)
/// - `TELEMETRY_HTTP_SERVER` - Enable HTTP server for Prometheus (default: false)
/// - `TELEMETRY_HTTP_PORT` - HTTP server port (default: 9090)
/// - `TELEMETRY_OTLP_EXPORT` - Enable OTLP export (default: false)
/// - `TELEMETRY_OTLP_ENDPOINT` - OTLP endpoint URL (default: "http://localhost:4318")
/// - `TELEMETRY_METRICS_INTERVAL_SEC` - Metrics export interval in seconds (default: 15)
/// - `TELEMETRY_OTLP_RETRIES` - OTLP export retry attempts (default: 3)
/// - `TELEMETRY_OTLP_TIMEOUT_MS` - OTLP export timeout in milliseconds (default: 5000)
/// 
/// **Logging:**
/// - `LOG_LEVEL` - Log level: trace, debug, info, warn, error, critical, off (default: "info")
/// - `LOG_CONSOLE` - Enable console logging (default: true)
/// - `LOG_FILE` - Log file path (empty = disabled, default: "")
/// 
/// **Connection Timeouts:**
/// - `WS_CONN_TIMEOUT_MS` - Connection timeout in milliseconds (default: 10000)
/// - `WS_READ_TIMEOUT_MS` - Read timeout in milliseconds (default: 30000)
/// - `WS_WRITE_TIMEOUT_MS` - Write timeout in milliseconds (default: 10000)
/// - `WS_PING_INTERVAL_SEC` - WebSocket ping interval in seconds (default: 30)
/// - `WS_PONG_TIMEOUT_SEC` - Pong timeout in seconds (default: 10)
/// 
/// **Security:**
/// - `TLS_VERIFY_PEER` - Verify TLS certificates (default: true)
/// - `TLS_CA_CERT_PATH` - Custom CA certificate path (default: system default)
/// - `TLS_CLIENT_CERT_PATH` - Client certificate path (optional)
/// - `TLS_CLIENT_KEY_PATH` - Client private key path (optional, required if cert provided)
/// - `TLS_CIPHER_SUITES` - Allowed cipher suites (default: system default)
/// - `ALLOW_INSECURE` - Allow insecure connections (default: false, dev only)
/// 
/// **Rate Limiting:**
/// - `RATE_LIMIT_ENABLED` - Enable/disable rate limiting (default: false)
/// - `RATE_LIMIT_REQUESTS_PER_SEC` - Rate limit in requests per second (default: 10.0)
/// - `RATE_LIMIT_BURST_SIZE` - Maximum burst capacity (default: 20)
/// 
/// @example
/// ```cpp
/// auto config = kraken::config_from_env();
/// kraken::KrakenClient client(config);
/// ```
/// 
/// @return ClientConfig built from environment variables
auto config_from_env() -> ClientConfig;

/// Helper to get environment variable with default
inline std::string get_env(const char* name, const std::string& default_value = "") {
    const char* value = std::getenv(name);
    return (value != nullptr) ? std::string(value) : default_value;
}

/// Helper to parse boolean from environment variable
inline bool get_env_bool(const char* name, bool default_value = false) {
    const char* value = std::getenv(name);
    if (!value) return default_value;
    std::string str(value);
    // Convert to lowercase for comparison
    for (char& c : str) {
        if (c >= 'A' && c <= 'Z') {
            c = static_cast<char>(static_cast<unsigned char>(c - 'A' + 'a'));
        }
    }
    return str == "true" || str == "1" || str == "yes" || str == "on";
}

/// Helper to parse integer from environment variable
inline int get_env_int(const char* name, int default_value = 0) {
    const char* value = std::getenv(name);
    if (!value) return default_value;
    try {
        return std::stoi(value);
    } catch (...) {
        return default_value;
    }
}

/// Helper to parse size_t from environment variable
inline size_t get_env_size_t(const char* name, size_t default_value = 0) {
    const char* value = std::getenv(name);
    if (!value) return default_value;
    try {
        return static_cast<size_t>(std::stoull(value));
    } catch (...) {
        return default_value;
    }
}

/// Helper to parse double from environment variable
inline double get_env_double(const char* name, double default_value = 0.0) {
    const char* value = std::getenv(name);
    if (!value) return default_value;
    try {
        return std::stod(value);
    } catch (...) {
        return default_value;
    }
}

} // namespace kraken

