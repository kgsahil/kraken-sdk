# Configuration Validation

This document explains how the SDK validates configuration values and handles errors.

## Essential vs Non-Essential Configuration

### Essential (Required) - Throws Errors
These values **must** be provided and valid, otherwise the SDK throws an error:

1. **`KRAKEN_WS_URL`**
   - **Required:** Yes
   - **Validation:** Must be non-empty and valid URL format
   - **Error:** `std::runtime_error` if empty, `std::invalid_argument` if invalid format
   - **Example Error:** `"KRAKEN_WS_URL is required but not set or empty"`

### Non-Essential (Optional) - Uses Defaults
All other configuration values are optional. If not provided, the SDK uses sensible defaults:

- Queue settings → Defaults to enabled with 65536 capacity
- Retry settings → Defaults to conservative exponential backoff
- Timeouts → Defaults to 10s connect, 30s read, 10s write
- Security → Defaults to verify peer, system CA certificates
- Telemetry → Defaults to disabled
- Logging → Defaults to INFO level, console only

---

## Validation Rules

### 1. WebSocket URL (`KRAKEN_WS_URL`)
```cpp
// ✅ Valid
KRAKEN_WS_URL="wss://ws.kraken.com/v2"
KRAKEN_WS_URL="ws://localhost:8080"

// ❌ Invalid - throws std::runtime_error
KRAKEN_WS_URL=""  // Empty

// ❌ Invalid - throws std::invalid_argument
KRAKEN_WS_URL="invalid-url"  // Missing ://
KRAKEN_WS_URL="ftp://example.com"  // Invalid scheme
```

### 2. Connection Timeouts
All timeout values must be **positive** (> 0):

```cpp
// ✅ Valid
WS_CONN_TIMEOUT_MS="10000"  // 10 seconds
WS_READ_TIMEOUT_MS="30000"  // 30 seconds

// ❌ Invalid - throws std::invalid_argument
WS_CONN_TIMEOUT_MS="0"      // Zero
WS_CONN_TIMEOUT_MS="-1000"  // Negative
WS_CONN_TIMEOUT_MS="invalid"  // Non-numeric (falls back to default)
```

**Note:** Non-numeric values fall back to defaults (no error thrown).

### 3. Security Configuration

#### Client Certificate Validation
If either `TLS_CLIENT_CERT_PATH` or `TLS_CLIENT_KEY_PATH` is set, **both** must be set:

```cpp
// ✅ Valid
TLS_CLIENT_CERT_PATH="/path/to/cert.crt"
TLS_CLIENT_KEY_PATH="/path/to/key.key"

// ❌ Invalid - throws std::runtime_error
TLS_CLIENT_CERT_PATH="/path/to/cert.crt"
TLS_CLIENT_KEY_PATH=""  // Missing key

// ❌ Invalid - throws std::runtime_error
TLS_CLIENT_KEY_PATH="/path/to/key.key"
TLS_CLIENT_CERT_PATH=""  // Missing cert
```

#### Insecure Connection Validation
`ALLOW_INSECURE="true"` cannot be used with secure URLs:

```cpp
// ✅ Valid
KRAKEN_WS_URL="ws://localhost:8080"
ALLOW_INSECURE="true"

// ❌ Invalid - throws std::invalid_argument
KRAKEN_WS_URL="wss://ws.kraken.com/v2"
ALLOW_INSECURE="true"  // Cannot use insecure with wss://
```

### 4. Numeric Values
Invalid numeric values fall back to defaults (no error):

```cpp
// ✅ Valid - uses value
WS_CONN_RETRY_TIMES="10"

// ⚠️ Falls back to default (10) - no error
WS_CONN_RETRY_TIMES="invalid"  // Not a number
WS_CONN_RETRY_TIMES=""  // Empty
```

### 5. Boolean Values
Invalid boolean values default to `false` (no error):

```cpp
// ✅ Valid - true
ENABLE_SPSC_QUEUE="true"
ENABLE_SPSC_QUEUE="1"
ENABLE_SPSC_QUEUE="yes"
ENABLE_SPSC_QUEUE="on"

// ✅ Valid - false
ENABLE_SPSC_QUEUE="false"
ENABLE_SPSC_QUEUE="0"
ENABLE_SPSC_QUEUE="no"
ENABLE_SPSC_QUEUE="off"

// ⚠️ Defaults to false - no error
ENABLE_SPSC_QUEUE="invalid"  // Not recognized
ENABLE_SPSC_QUEUE=""  // Empty
```

---

## Error Handling

### Programmatic Usage

```cpp
#include <kraken/kraken.hpp>
#include <iostream>

int main() {
    try {
        // This can throw std::runtime_error or std::invalid_argument
        auto config = kraken::config_from_env();
        kraken::KrakenClient client(config);
        
        // ... use client
    } catch (const std::runtime_error& e) {
        // Essential config missing (e.g., KRAKEN_WS_URL)
        std::cerr << "Configuration error: " << e.what() << std::endl;
        return 1;
    } catch (const std::invalid_argument& e) {
        // Invalid configuration value
        std::cerr << "Invalid configuration: " << e.what() << std::endl;
        return 1;
    } catch (const std::exception& e) {
        // Other errors
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
```

### Builder Pattern Usage

```cpp
try {
    auto builder = kraken::ClientConfig::Builder();
    builder.url("");  // Empty URL
    
    auto config = builder.build();  // Throws std::invalid_argument
} catch (const std::invalid_argument& e) {
    std::cerr << "Invalid config: " << e.what() << std::endl;
}
```

---

## Validation Summary

| Configuration | Required | Validation | Error Type |
|---------------|----------|------------|------------|
| `KRAKEN_WS_URL` | ✅ Yes | Non-empty, valid URL | `std::runtime_error` / `std::invalid_argument` |
| `WS_CONN_TIMEOUT_MS` | ❌ No | Positive (> 0) | `std::invalid_argument` |
| `WS_READ_TIMEOUT_MS` | ❌ No | Positive (> 0) | `std::invalid_argument` |
| `WS_WRITE_TIMEOUT_MS` | ❌ No | Positive (> 0) | `std::invalid_argument` |
| `TLS_CLIENT_CERT_PATH` | ❌ No* | If set, key must be set | `std::runtime_error` |
| `TLS_CLIENT_KEY_PATH` | ❌ No* | If set, cert must be set | `std::runtime_error` |
| `ALLOW_INSECURE` | ❌ No | Cannot be true with wss:// | `std::invalid_argument` |
| All others | ❌ No | Use defaults if invalid | No error |

\* Required only if the other is set

---

## Best Practices

1. **Always catch exceptions** when using `config_from_env()`
2. **Validate essential config early** in your application startup
3. **Use environment variable validation** in CI/CD pipelines
4. **Document required variables** in your deployment guides
5. **Test with missing/invalid values** to ensure proper error handling

---

## Example: Robust Configuration

```cpp
#include <kraken/kraken.hpp>
#include <iostream>

bool validate_config() {
    try {
        auto config = kraken::config_from_env();
        
        // Additional validation
        if (config.url().empty()) {
            std::cerr << "Error: WebSocket URL is required" << std::endl;
            return false;
        }
        
        // Check if authenticated mode is properly configured
        if (config.is_authenticated()) {
            std::cout << "✓ Authenticated mode enabled" << std::endl;
        } else {
            std::cout << "✓ Public mode (no authentication)" << std::endl;
        }
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Configuration error: " << e.what() << std::endl;
        return false;
    }
}

int main() {
    if (!validate_config()) {
        return 1;
    }
    
    try {
        auto config = kraken::config_from_env();
        kraken::KrakenClient client(config);
        // ... rest of code
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
```

