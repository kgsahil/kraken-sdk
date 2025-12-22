#include "kraken/config_env.hpp"
#include "kraken/backoff.hpp"
#include "kraken/telemetry.hpp"
#include "kraken/logger.hpp"
#include "kraken/connection_config.hpp"
#include <chrono>
#include <stdexcept>

namespace kraken {

ClientConfig config_from_env() {
    ClientConfig::Builder builder;
    
    // WebSocket URL (ESSENTIAL - will throw if invalid)
    std::string ws_url = get_env("KRAKEN_WS_URL", "wss://ws.kraken.com/v2");
    if (ws_url.empty()) {
        throw std::runtime_error("KRAKEN_WS_URL is required but not set or empty");
    }
    builder.url(ws_url);
    
    // API credentials
    std::string api_key = get_env("KRAKEN_API_KEY");
    std::string api_secret = get_env("KRAKEN_API_SECRET");
    if (!api_key.empty() && !api_secret.empty()) {
        builder.api_key(api_key);
        builder.api_secret(api_secret);
    }
    
    // SPSC Queue settings
    bool enable_queue = get_env_bool("ENABLE_SPSC_QUEUE", true);
    builder.use_queue(enable_queue);
    
    if (enable_queue) {
        size_t queue_size = get_env_size_t("SPSC_QUEUE_SIZE", 65536);
        builder.queue_capacity(queue_size);
    }
    
    // Connection retry settings (exponential backoff)
    int retry_delay_ms = get_env_int("WS_CONN_RETRY_DELAY_MS", 1000);
    double retry_multiplier = get_env_double("WS_CONN_RETRY_MULTIPLIER", 2.0);
    int retry_times = get_env_int("WS_CONN_RETRY_TIMES", 10);
    int retry_max_delay_ms = get_env_int("WS_CONN_RETRY_MAX_DELAY_MS", 30000);
    double retry_jitter = get_env_double("WS_CONN_RETRY_JITTER", 0.2);
    
    // Build exponential backoff strategy
    auto backoff = ExponentialBackoff::Builder()
        .initial_delay(std::chrono::milliseconds(retry_delay_ms))
        .max_delay(std::chrono::milliseconds(retry_max_delay_ms))
        .max_attempts(retry_times)
        .multiplier(retry_multiplier)
        .jitter(retry_jitter)
        .build();
    
    builder.backoff(std::unique_ptr<BackoffStrategy>(backoff.release()));
    
    // Checksum validation
    bool validate_checksums = get_env_bool("VALIDATE_CHECKSUMS", true);
    builder.validate_checksums(validate_checksums);
    
    // Gap detection
    bool enable_gap_detection = get_env_bool("ENABLE_GAP_DETECTION", false);
    builder.gap_detection(enable_gap_detection);
    
    if (enable_gap_detection) {
        int gap_tolerance = get_env_int("GAP_TOLERANCE", 0);
        builder.gap_tolerance(gap_tolerance);
    }
    
    // Telemetry configuration
    bool enable_telemetry = get_env_bool("ENABLE_TELEMETRY", false);
    if (enable_telemetry) {
        std::string service_name = get_env("TELEMETRY_SERVICE_NAME", "kraken-sdk");
        std::string service_version = get_env("TELEMETRY_SERVICE_VERSION", "1.0.0");
        std::string environment = get_env("TELEMETRY_ENVIRONMENT", "production");
        
        bool enable_metrics = get_env_bool("TELEMETRY_ENABLE_METRICS", true);
        bool enable_traces = get_env_bool("TELEMETRY_ENABLE_TRACES", false);
        bool enable_logs = get_env_bool("TELEMETRY_ENABLE_LOGS", false);
        
        // HTTP server for Prometheus
        bool enable_http_server = get_env_bool("TELEMETRY_HTTP_SERVER", false);
        int http_server_port = get_env_int("TELEMETRY_HTTP_PORT", 9090);
        
        // OTLP export
        bool enable_otlp = get_env_bool("TELEMETRY_OTLP_EXPORT", false);
        std::string otlp_endpoint = get_env("TELEMETRY_OTLP_ENDPOINT", "http://localhost:4318");
        int metrics_interval = get_env_int("TELEMETRY_METRICS_INTERVAL_SEC", 15);
        int otlp_retries = get_env_int("TELEMETRY_OTLP_RETRIES", 3);
        int otlp_timeout_ms = get_env_int("TELEMETRY_OTLP_TIMEOUT_MS", 5000);
        
        auto telemetry_config = TelemetryConfig::Builder()
            .service_name(service_name)
            .service_version(service_version)
            .environment(environment)
            .metrics(enable_metrics)
            .traces(enable_traces)
            .logs(enable_logs)
            .http_server(enable_http_server, static_cast<uint16_t>(http_server_port))
            .otlp_export(enable_otlp)
            .otlp_endpoint(otlp_endpoint)
            .metrics_interval(std::chrono::seconds(metrics_interval))
            .export_retries(otlp_retries)
            .export_timeout(std::chrono::milliseconds(otlp_timeout_ms))
            .build();
        
        builder.telemetry(telemetry_config);
    }
    
    // Connection timeouts (non-essential - use defaults if not set)
    int conn_timeout_ms = get_env_int("WS_CONN_TIMEOUT_MS", 10000);
    int read_timeout_ms = get_env_int("WS_READ_TIMEOUT_MS", 30000);
    int write_timeout_ms = get_env_int("WS_WRITE_TIMEOUT_MS", 10000);
    int ping_interval_sec = get_env_int("WS_PING_INTERVAL_SEC", 30);
    int pong_timeout_sec = get_env_int("WS_PONG_TIMEOUT_SEC", 10);
    
    ConnectionTimeouts timeouts;
    timeouts.connect_timeout = std::chrono::milliseconds(conn_timeout_ms);
    timeouts.read_timeout = std::chrono::milliseconds(read_timeout_ms);
    timeouts.write_timeout = std::chrono::milliseconds(write_timeout_ms);
    timeouts.ping_interval = std::chrono::seconds(ping_interval_sec);
    timeouts.pong_timeout = std::chrono::seconds(pong_timeout_sec);
    builder.connection_timeouts(timeouts);
    
    // Security configuration (non-essential - use defaults if not set)
    SecurityConfig security;
    security.verify_peer = get_env_bool("TLS_VERIFY_PEER", true);
    security.ca_cert_path = get_env("TLS_CA_CERT_PATH", "");
    security.client_cert_path = get_env("TLS_CLIENT_CERT_PATH", "");
    security.client_key_path = get_env("TLS_CLIENT_KEY_PATH", "");
    security.cipher_suites = get_env("TLS_CIPHER_SUITES", "");
    security.allow_insecure = get_env_bool("ALLOW_INSECURE", false);
    
    // Validate security config: if client cert is provided, key must also be provided
    if (!security.client_cert_path.empty() && security.client_key_path.empty()) {
        throw std::runtime_error(
            "TLS_CLIENT_KEY_PATH is required when TLS_CLIENT_CERT_PATH is set");
    }
    if (!security.client_key_path.empty() && security.client_cert_path.empty()) {
        throw std::runtime_error(
            "TLS_CLIENT_CERT_PATH is required when TLS_CLIENT_KEY_PATH is set");
    }
    
    builder.security(security);
    
    // Rate limiting configuration (non-essential - disabled by default)
    bool enable_rate_limiting = get_env_bool("RATE_LIMIT_ENABLED", false);
    if (enable_rate_limiting) {
        double requests_per_sec = get_env_double("RATE_LIMIT_REQUESTS_PER_SEC", 10.0);
        size_t burst_size = get_env_size_t("RATE_LIMIT_BURST_SIZE", 20);
        builder.rate_limiting(true, requests_per_sec, burst_size);
    }
    
    // Initialize logger from environment variables
    std::string log_level = get_env("LOG_LEVEL", "info");
    bool log_console = get_env_bool("LOG_CONSOLE", true);
    std::string log_file = get_env("LOG_FILE", "");
    
    if (!log_file.empty() || log_console) {
        Logger::init(log_level, log_console, log_file);
    }
    
    return builder.build();
}

} // namespace kraken

