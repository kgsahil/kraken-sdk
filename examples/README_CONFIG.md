# Configuration File Guide

All examples support loading configuration from a file using the `--config=` command-line argument.

## Quick Start

1. **Copy the example config file:**
   ```bash
   cp examples/config.cfg.example myconfig.cfg
   ```

2. **Edit the config file with your settings:**
   ```bash
   nano myconfig.cfg
   ```

3. **Run an example with the config file:**
   ```bash
   ./dashboard --config=myconfig.cfg
   ./quickstart --config=myconfig.cfg
   ```

## Config File Format

The config file uses a simple `KEY=VALUE` format:

```
# This is a comment
KRAKEN_WS_URL=wss://ws.kraken.com/v2
KRAKEN_API_KEY=your-api-key
ENABLE_SPSC_QUEUE=true
SPSC_QUEUE_SIZE=131072
```

### Rules:
- One `KEY=VALUE` pair per line
- Lines starting with `#` are comments (ignored)
- Empty lines are ignored
- Values can be quoted (optional): `KEY="value with spaces"`
- Whitespace around `=` is ignored
- Environment variables are set when the file is loaded

## Available Configuration Variables

See [docs/ENVIRONMENT_VARIABLES.md](../docs/ENVIRONMENT_VARIABLES.md) for a complete list of all supported environment variables.

### Essential (Required)
- `KRAKEN_WS_URL` - WebSocket URL (required)

### Common Settings
- `KRAKEN_API_KEY` - API key for authentication
- `KRAKEN_API_SECRET` - API secret for authentication
- `ENABLE_SPSC_QUEUE` - Enable/disable SPSC queue
- `SPSC_QUEUE_SIZE` - Queue size (if enabled)
- `LOG_LEVEL` - Logging level (trace, debug, info, warn, error, critical)

### Connection Settings
- `WS_CONN_RETRY_DELAY_MS` - Initial retry delay
- `WS_CONN_RETRY_MULTIPLIER` - Retry delay multiplier
- `WS_CONN_RETRY_TIMES` - Maximum retry attempts
- `WS_CONN_TIMEOUT_MS` - Connection timeout
- `WS_READ_TIMEOUT_MS` - Read timeout
- `WS_WRITE_TIMEOUT_MS` - Write timeout

### Telemetry Settings
- `ENABLE_TELEMETRY` - Enable telemetry collection
- `TELEMETRY_SERVICE_NAME` - Service name for telemetry
- `TELEMETRY_HTTP_SERVER` - Enable HTTP server for Prometheus
- `TELEMETRY_HTTP_PORT` - HTTP server port
- `TELEMETRY_OTLP_EXPORT` - Enable OTLP export
- `TELEMETRY_OTLP_ENDPOINT` - OTLP endpoint URL

## Example Config Files

### Minimal Config
```bash
# Minimal configuration - just connection
KRAKEN_WS_URL=wss://ws.kraken.com/v2
LOG_LEVEL=info
```

### Production Config
```bash
# Production configuration
KRAKEN_WS_URL=wss://ws.kraken.com/v2
KRAKEN_API_KEY=your-production-key
KRAKEN_API_SECRET=your-production-secret

ENABLE_SPSC_QUEUE=true
SPSC_QUEUE_SIZE=262144

WS_CONN_RETRY_DELAY_MS=1000
WS_CONN_RETRY_MULTIPLIER=2.0
WS_CONN_RETRY_TIMES=20

VALIDATE_CHECKSUMS=true
ENABLE_GAP_DETECTION=true

ENABLE_TELEMETRY=true
TELEMETRY_SERVICE_NAME=production-bot
TELEMETRY_HTTP_SERVER=true
TELEMETRY_HTTP_PORT=9090

LOG_LEVEL=info
LOG_FILE=/var/log/kraken-sdk.log
```

### Development Config
```bash
# Development configuration
KRAKEN_WS_URL=wss://ws.kraken.com/v2

ENABLE_SPSC_QUEUE=true
SPSC_QUEUE_SIZE=65536

WS_CONN_RETRY_DELAY_MS=500
WS_CONN_RETRY_TIMES=5

LOG_LEVEL=debug
LOG_CONSOLE=true
```

## Command-Line Usage

### Format 1: `--config=path`
```bash
./dashboard --config=./myconfig.cfg
```

### Format 2: `--config path`
```bash
./dashboard --config ./myconfig.cfg
```

### Multiple Arguments
```bash
# Config file + other arguments
./data_collector output.csv --config=myconfig.cfg
./web_backend http://api.example.com --config=myconfig.cfg
```

## Environment Variable Precedence

1. **Config file** (if `--config=` is provided)
2. **System environment variables** (if not in config file)
3. **SDK defaults** (if neither config file nor env vars are set)

Config file values **override** system environment variables.

## Error Handling

If the config file cannot be read, the example will:
1. Print an error message
2. Show usage information
3. Exit with code 1

Example:
```bash
$ ./dashboard --config=nonexistent.cfg
Error loading config file: Cannot open config file: nonexistent.cfg
Usage: ./dashboard [--config=path/to/config.cfg]
```

## Creating Config Files Programmatically

You can also create config files using the SDK:

```cpp
#include "examples/config.hpp"

// Print template to stdout
examples::print_config_template();

// Create a sample config file
examples::create_sample_config("myconfig.cfg");
```

## Validation

The config file format is validated when loaded:
- Each non-comment line must contain `=`
- Empty keys are reported as warnings
- Invalid lines are skipped with warnings

## Security Notes

⚠️ **Important:** Config files may contain sensitive information (API keys, secrets).

- **Never commit config files to version control**
- **Use appropriate file permissions:** `chmod 600 myconfig.cfg`
- **Store production configs securely**
- **Consider using environment variables in production instead**

## See Also

- [Environment Variables Documentation](../docs/ENVIRONMENT_VARIABLES.md)
- [Configuration Validation](../docs/CONFIGURATION_VALIDATION.md)
- [Examples README](README.md)

