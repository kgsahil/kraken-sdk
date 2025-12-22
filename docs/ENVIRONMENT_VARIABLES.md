# Environment Variables Configuration

The Kraken SDK can be configured via environment variables, allowing you to customize behavior without modifying code.

## Usage

```cpp
#include <kraken/kraken.hpp>

int main() {
    // Automatically reads from environment variables
    auto config = kraken::config_from_env();
    kraken::KrakenClient client(config);
    
    // ... rest of your code
}
```

Or use it directly in examples:

```bash
# Set environment variables
export KRAKEN_API_KEY="your-api-key"
export KRAKEN_API_SECRET="your-api-secret"
export ENABLE_SPSC_QUEUE="true"
export SPSC_QUEUE_SIZE="131072"

# Run example
./quickstart
```

---

## Supported Environment Variables

### Authentication

| Variable | Description | Default | Example |
|----------|-------------|---------|---------|
| `KRAKEN_API_KEY` | API key for authenticated endpoints | (none) | `KRAKEN_API_KEY="your-key"` |
| `KRAKEN_API_SECRET` | API secret for authenticated endpoints | (none) | `KRAKEN_API_SECRET="your-secret"` |

**Note:** If both `KRAKEN_API_KEY` and `KRAKEN_API_SECRET` are set, the SDK will automatically authenticate. If either is missing, the SDK runs in public mode.

---

### WebSocket Connection

| Variable | Description | Default | Example |
|----------|-------------|---------|---------|
| `KRAKEN_WS_URL` | WebSocket URL | `wss://ws.kraken.com/v2` | `KRAKEN_WS_URL="wss://ws.kraken.com/v2"` |

---

### SPSC Queue Configuration

| Variable | Description | Default | Example |
|----------|-------------|---------|---------|
| `ENABLE_SPSC_QUEUE` | Enable/disable SPSC queue | `true` | `ENABLE_SPSC_QUEUE="false"` |
| `SPSC_QUEUE_SIZE` | Queue capacity (power of 2 recommended) | `65536` | `SPSC_QUEUE_SIZE="131072"` |

**Note:** If `ENABLE_SPSC_QUEUE="false"`, messages are processed directly in the I/O thread (lower latency, but blocks I/O during callbacks).

---

### Connection Retry Settings

| Variable | Description | Default | Example |
|----------|-------------|---------|---------|
| `WS_CONN_RETRY_DELAY_MS` | Initial retry delay (milliseconds) | `1000` | `WS_CONN_RETRY_DELAY_MS="500"` |
| `WS_CONN_RETRY_MULTIPLIER` | Delay multiplier for exponential backoff | `2.0` | `WS_CONN_RETRY_MULTIPLIER="1.5"` |
| `WS_CONN_RETRY_TIMES` | Maximum retry attempts | `10` | `WS_CONN_RETRY_TIMES="20"` |
| `WS_CONN_RETRY_MAX_DELAY_MS` | Maximum retry delay (milliseconds) | `30000` | `WS_CONN_RETRY_MAX_DELAY_MS="60000"` |
| `WS_CONN_RETRY_JITTER` | Jitter factor (0.0-1.0) | `0.2` | `WS_CONN_RETRY_JITTER="0.3"` |

**Retry Strategy:** Exponential backoff with jitter:
- Delay = `min(initial_delay * (multiplier ^ attempt) * (1 ± jitter), max_delay)`

---

### Data Integrity

| Variable | Description | Default | Example |
|----------|-------------|---------|---------|
| `VALIDATE_CHECKSUMS` | Enable/disable CRC32 checksum validation | `true` | `VALIDATE_CHECKSUMS="false"` |

---

### Gap Detection

| Variable | Description | Default | Example |
|----------|-------------|---------|---------|
| `ENABLE_GAP_DETECTION` | Enable/disable message gap detection | `false` | `ENABLE_GAP_DETECTION="true"` |
| `GAP_TOLERANCE` | Allow up to N missing messages before alerting | `0` | `GAP_TOLERANCE="5"` |

---

### Telemetry/OpenTelemetry

| Variable | Description | Default | Example |
|----------|-------------|---------|---------|
| `ENABLE_TELEMETRY` | Enable/disable telemetry collection | `false` | `ENABLE_TELEMETRY="true"` |
| `TELEMETRY_SERVICE_NAME` | Service name for telemetry | `"kraken-sdk"` | `TELEMETRY_SERVICE_NAME="my-trading-bot"` |
| `TELEMETRY_SERVICE_VERSION` | Service version | `"1.0.0"` | `TELEMETRY_SERVICE_VERSION="2.1.0"` |
| `TELEMETRY_ENVIRONMENT` | Environment name | `"production"` | `TELEMETRY_ENVIRONMENT="staging"` |
| `TELEMETRY_ENABLE_METRICS` | Enable metrics collection | `true` | `TELEMETRY_ENABLE_METRICS="true"` |
| `TELEMETRY_ENABLE_TRACES` | Enable distributed tracing | `false` | `TELEMETRY_ENABLE_TRACES="true"` |
| `TELEMETRY_ENABLE_LOGS` | Enable log export | `false` | `TELEMETRY_ENABLE_LOGS="true"` |
| `TELEMETRY_HTTP_SERVER` | Enable HTTP server for Prometheus scraping | `false` | `TELEMETRY_HTTP_SERVER="true"` |
| `TELEMETRY_HTTP_PORT` | HTTP server port | `9090` | `TELEMETRY_HTTP_PORT="9090"` |
| `TELEMETRY_OTLP_EXPORT` | Enable OTLP export | `false` | `TELEMETRY_OTLP_EXPORT="true"` |
| `TELEMETRY_OTLP_ENDPOINT` | OTLP endpoint URL | `"http://localhost:4318"` | `TELEMETRY_OTLP_ENDPOINT="https://collector:4318"` |
| `TELEMETRY_METRICS_INTERVAL_SEC` | Metrics export interval (seconds) | `15` | `TELEMETRY_METRICS_INTERVAL_SEC="30"` |
| `TELEMETRY_OTLP_RETRIES` | OTLP export retry attempts | `3` | `TELEMETRY_OTLP_RETRIES="5"` |
| `TELEMETRY_OTLP_TIMEOUT_MS` | OTLP export timeout (milliseconds) | `5000` | `TELEMETRY_OTLP_TIMEOUT_MS="10000"` |

---

### Logging

| Variable | Description | Default | Example |
|----------|-------------|---------|---------|
| `LOG_LEVEL` | Log level: trace, debug, info, warn, error, critical, off | `"info"` | `LOG_LEVEL="debug"` |
| `LOG_CONSOLE` | Enable console logging | `true` | `LOG_CONSOLE="true"` |
| `LOG_FILE` | Log file path (empty = disabled) | `""` | `LOG_FILE="/var/log/kraken-sdk.log"` |

**Log Levels:**
- `trace` - Most verbose (all messages)
- `debug` - Debug information
- `info` - Informational messages (default)
- `warn` - Warnings
- `error` - Errors only
- `critical` - Critical errors only
- `off` - Disable logging

---

### Connection Timeouts

| Variable | Description | Default | Example |
|----------|-------------|---------|---------|
| `WS_CONN_TIMEOUT_MS` | Connection timeout in milliseconds | `10000` | `WS_CONN_TIMEOUT_MS="5000"` |
| `WS_READ_TIMEOUT_MS` | Read timeout in milliseconds | `30000` | `WS_READ_TIMEOUT_MS="60000"` |
| `WS_WRITE_TIMEOUT_MS` | Write timeout in milliseconds | `10000` | `WS_WRITE_TIMEOUT_MS="5000"` |
| `WS_PING_INTERVAL_SEC` | WebSocket ping interval in seconds | `30` | `WS_PING_INTERVAL_SEC="20"` |
| `WS_PONG_TIMEOUT_SEC` | Pong timeout in seconds | `10` | `WS_PONG_TIMEOUT_SEC="5"` |

**Note:** All timeout values must be positive. Invalid values will cause configuration errors.

---

### Security/TLS

| Variable | Description | Default | Example |
|----------|-------------|---------|---------|
| `TLS_VERIFY_PEER` | Verify TLS certificates | `true` | `TLS_VERIFY_PEER="true"` |
| `TLS_CA_CERT_PATH` | Custom CA certificate path | `""` (system default) | `TLS_CA_CERT_PATH="/etc/ssl/certs/ca-certificates.crt"` |
| `TLS_CLIENT_CERT_PATH` | Client certificate path | `""` | `TLS_CLIENT_CERT_PATH="/path/to/client.crt"` |
| `TLS_CLIENT_KEY_PATH` | Client private key path | `""` | `TLS_CLIENT_KEY_PATH="/path/to/client.key"` |
| `TLS_CIPHER_SUITES` | Allowed cipher suites | `""` (system default) | `TLS_CIPHER_SUITES="ECDHE-RSA-AES256-GCM-SHA384"` |
| `ALLOW_INSECURE` | Allow insecure connections | `false` | `ALLOW_INSECURE="false"` |

**Security Notes:**
- ⚠️ `ALLOW_INSECURE="true"` cannot be used with secure URLs (`wss://` or `https://`)
- If `TLS_CLIENT_CERT_PATH` is set, `TLS_CLIENT_KEY_PATH` is **required** (and vice versa)
- `TLS_VERIFY_PEER="false"` disables certificate verification (not recommended for production)
- Custom CA certificates are useful for corporate proxies or internal networks

---

## Examples

### Example 1: Public Mode (No Authentication)

```bash
# Use defaults - no environment variables needed
./quickstart
```

### Example 2: Authenticated Mode

```bash
export KRAKEN_API_KEY="your-api-key"
export KRAKEN_API_SECRET="your-api-secret"
./quickstart
```

### Example 3: Custom Queue Size

```bash
export SPSC_QUEUE_SIZE="131072"  # 128K messages
./quickstart
```

### Example 4: Disable Queue (Direct Processing)

```bash
export ENABLE_SPSC_QUEUE="false"
./quickstart
```

### Example 5: Aggressive Retry Strategy

```bash
export WS_CONN_RETRY_DELAY_MS="100"
export WS_CONN_RETRY_MULTIPLIER="1.5"
export WS_CONN_RETRY_TIMES="30"
export WS_CONN_RETRY_MAX_DELAY_MS="10000"
./quickstart
```

### Example 6: Enable Gap Detection

```bash
export ENABLE_GAP_DETECTION="true"
export GAP_TOLERANCE="3"
./quickstart
```

### Example 7: Enable Telemetry

```bash
export ENABLE_TELEMETRY="true"
export TELEMETRY_SERVICE_NAME="trading-bot"
export TELEMETRY_HTTP_SERVER="true"
export TELEMETRY_HTTP_PORT="9090"
export TELEMETRY_OTLP_EXPORT="true"
export TELEMETRY_OTLP_ENDPOINT="http://localhost:4318"
./quickstart
```

### Example 8: Configure Logging

```bash
export LOG_LEVEL="debug"
export LOG_CONSOLE="true"
export LOG_FILE="/var/log/kraken-sdk.log"
./quickstart
```

### Example 9: Connection Timeouts

```bash
export WS_CONN_TIMEOUT_MS="5000"
export WS_READ_TIMEOUT_MS="60000"
export WS_WRITE_TIMEOUT_MS="5000"
export WS_PING_INTERVAL_SEC="20"
export WS_PONG_TIMEOUT_SEC="5"
./quickstart
```

### Example 10: Security Configuration

```bash
# Custom CA certificate (for corporate proxies)
export TLS_CA_CERT_PATH="/etc/ssl/certs/custom-ca.crt"

# Client certificate authentication
export TLS_CLIENT_CERT_PATH="/path/to/client.crt"
export TLS_CLIENT_KEY_PATH="/path/to/client.key"

# Custom cipher suites
export TLS_CIPHER_SUITES="ECDHE-RSA-AES256-GCM-SHA384:ECDHE-RSA-AES128-GCM-SHA256"

./quickstart
```

### Example 11: Complete Enterprise Configuration

```bash
# Essential: WebSocket URL
export KRAKEN_WS_URL="wss://ws.kraken.com/v2"

# Authentication
export KRAKEN_API_KEY="your-key"
export KRAKEN_API_SECRET="your-secret"

# Queue settings
export ENABLE_SPSC_QUEUE="true"
export SPSC_QUEUE_SIZE="262144"

# Retry settings
export WS_CONN_RETRY_DELAY_MS="500"
export WS_CONN_RETRY_MULTIPLIER="2.0"
export WS_CONN_RETRY_TIMES="20"
export WS_CONN_RETRY_MAX_DELAY_MS="30000"
export WS_CONN_RETRY_JITTER="0.2"

# Connection timeouts
export WS_CONN_TIMEOUT_MS="10000"
export WS_READ_TIMEOUT_MS="30000"
export WS_WRITE_TIMEOUT_MS="10000"
export WS_PING_INTERVAL_SEC="30"
export WS_PONG_TIMEOUT_SEC="10"

# Security
export TLS_VERIFY_PEER="true"
export TLS_CA_CERT_PATH="/etc/ssl/certs/ca-certificates.crt"

# Data integrity
export VALIDATE_CHECKSUMS="true"
export ENABLE_GAP_DETECTION="true"
export GAP_TOLERANCE="5"

# Telemetry
export ENABLE_TELEMETRY="true"
export TELEMETRY_SERVICE_NAME="production-bot"
export TELEMETRY_ENVIRONMENT="production"
export TELEMETRY_HTTP_SERVER="true"
export TELEMETRY_HTTP_PORT="9090"
export TELEMETRY_OTLP_EXPORT="true"
export TELEMETRY_OTLP_ENDPOINT="https://collector:4318"

# Logging
export LOG_LEVEL="info"
export LOG_CONSOLE="true"
export LOG_FILE="/var/log/kraken-sdk.log"

# Run
./quickstart
```

---

## Programmatic Usage

You can also mix environment variables with programmatic configuration:

```cpp
#include <kraken/kraken.hpp>

int main() {
    // Start with environment variables
    auto config = kraken::config_from_env();
    
    // Override specific settings programmatically
    auto builder = kraken::ClientConfig::Builder(config);
    builder.queue_capacity(262144);  // Override queue size
    
    kraken::KrakenClient client(builder.build());
    // ...
}
```

**Note:** The `ClientConfig::Builder` constructor that takes a `ClientConfig` doesn't exist yet. You would need to manually set values or use the builder pattern directly.

---

## Boolean Values

Boolean environment variables accept:
- `true`, `1`, `yes`, `on` → `true`
- `false`, `0`, `no`, `off` → `false`
- Case-insensitive

---

## Validation

### Essential Configuration
- **`KRAKEN_WS_URL`** - Must be set and non-empty, must be a valid URL format
  - Throws `std::runtime_error` if missing or empty
  - Throws `std::invalid_argument` if URL format is invalid

### Non-Essential Configuration
- Invalid numeric values fall back to defaults
- Invalid boolean values default to `false`
- Empty strings are treated as "not set" (use defaults)

### Security Validation
- If `TLS_CLIENT_CERT_PATH` is set, `TLS_CLIENT_KEY_PATH` is **required** (throws error if missing)
- If `TLS_CLIENT_KEY_PATH` is set, `TLS_CLIENT_CERT_PATH` is **required** (throws error if missing)
- `ALLOW_INSECURE="true"` cannot be used with secure URLs (`wss://` or `https://`) - throws error

### Timeout Validation
- All timeout values must be positive (> 0)
- Throws `std::invalid_argument` if any timeout is <= 0

### Error Handling
```cpp
try {
    auto config = kraken::config_from_env();
    kraken::KrakenClient client(config);
} catch (const std::runtime_error& e) {
    // Essential config missing (e.g., KRAKEN_WS_URL)
    std::cerr << "Configuration error: " << e.what() << std::endl;
} catch (const std::invalid_argument& e) {
    // Invalid configuration value
    std::cerr << "Invalid configuration: " << e.what() << std::endl;
}
```

---

## Security Notes

⚠️ **Never commit API keys to version control!**

Use environment variables or secure credential storage:

```bash
# Good: Use environment variables
export KRAKEN_API_KEY="your-key"
export KRAKEN_API_SECRET="your-secret"

# Better: Use a secrets manager or .env file (not committed)
# .env file (add to .gitignore)
KRAKEN_API_KEY=your-key
KRAKEN_API_SECRET=your-secret
```

---

## See Also

- [README.md](../README.md) - Main documentation
- [BUILDING.md](../BUILDING.md) - Build instructions
- [examples/README.md](../examples/README.md) - Example programs

