# Strategy Configuration Guide

The SDK supports creating trading strategies from configuration files and environment variables, enabling deployment without code changes.

## Overview

The `StrategyConfig` class provides utilities to create strategies from:
- Configuration maps (key-value pairs)
- Environment variables
- Config files (via environment variable loading)

## Supported Strategy Types

### 1. Price Alert (`price_alert`)

```cpp
std::map<std::string, std::string> config = {
    {"STRATEGY_TYPE", "price_alert"},
    {"STRATEGY_SYMBOL", "BTC/USD"},
    {"STRATEGY_ABOVE", "50000.0"},
    {"STRATEGY_BELOW", "40000.0"},
    {"STRATEGY_RECURRING", "true"},
    {"STRATEGY_COOLDOWN_MS", "5000"}
};

auto strategy = StrategyConfig::from_map(config);
```

**Configuration Keys:**
- `STRATEGY_TYPE` (required): `"price_alert"`
- `STRATEGY_SYMBOL` (required): Trading pair symbol
- `STRATEGY_ABOVE` (optional): Price threshold above which to alert
- `STRATEGY_BELOW` (optional): Price threshold below which to alert
- `STRATEGY_RECURRING` (optional): `"true"` or `"false"` (default: `"false"`)
- `STRATEGY_COOLDOWN_MS` (optional): Cooldown period in milliseconds

### 2. Volume Spike (`volume_spike`)

```cpp
std::map<std::string, std::string> config = {
    {"STRATEGY_TYPE", "volume_spike"},
    {"STRATEGY_SYMBOLS", "BTC/USD,ETH/USD"},
    {"STRATEGY_MULTIPLIER", "2.5"},
    {"STRATEGY_LOOKBACK", "50"}
};

auto strategy = StrategyConfig::from_map(config);
```

**Configuration Keys:**
- `STRATEGY_TYPE` (required): `"volume_spike"`
- `STRATEGY_SYMBOLS` (required): Comma-separated list of trading pairs
- `STRATEGY_MULTIPLIER` (optional): Volume multiplier threshold (default: `2.0`)
- `STRATEGY_LOOKBACK` (optional): Number of samples for average calculation (default: `50`)

### 3. Spread Alert (`spread_alert`)

```cpp
std::map<std::string, std::string> config = {
    {"STRATEGY_TYPE", "spread_alert"},
    {"STRATEGY_SYMBOL", "BTC/USD"},
    {"STRATEGY_MIN_SPREAD", "1.0"},
    {"STRATEGY_MAX_SPREAD", "10.0"}
};

auto strategy = StrategyConfig::from_map(config);
```

**Configuration Keys:**
- `STRATEGY_TYPE` (required): `"spread_alert"`
- `STRATEGY_SYMBOL` (required): Trading pair symbol
- `STRATEGY_MIN_SPREAD` (optional): Minimum spread threshold
- `STRATEGY_MAX_SPREAD` (optional): Maximum spread threshold

### 4. Breakout Preset (`breakout`)

```cpp
std::map<std::string, std::string> config = {
    {"STRATEGY_TYPE", "breakout"},
    {"STRATEGY_SYMBOL", "BTC/USD"},
    {"STRATEGY_THRESHOLD", "50000.0"},
    {"STRATEGY_VOLUME_MULTIPLIER", "2.0"}
};

auto strategy = StrategyConfig::from_map(config);
```

**Configuration Keys:**
- `STRATEGY_TYPE` (required): `"breakout"`
- `STRATEGY_SYMBOL` (required): Trading pair symbol
- `STRATEGY_THRESHOLD` (required): Price breakout threshold
- `STRATEGY_VOLUME_MULTIPLIER` (optional): Volume confirmation multiplier (default: `2.0`)

### 5. Support Level Preset (`support_level`)

```cpp
std::map<std::string, std::string> config = {
    {"STRATEGY_TYPE", "support_level"},
    {"STRATEGY_SYMBOL", "BTC/USD"},
    {"STRATEGY_LEVEL", "45000.0"},
    {"STRATEGY_TOLERANCE", "1.0"},
    {"STRATEGY_MIN_LIQUIDITY", "10.0"}
};

auto strategy = StrategyConfig::from_map(config);
```

**Configuration Keys:**
- `STRATEGY_TYPE` (required): `"support_level"`
- `STRATEGY_SYMBOL` (required): Trading pair symbol
- `STRATEGY_LEVEL` (required): Support price level
- `STRATEGY_TOLERANCE` (optional): Price tolerance percentage (default: `1.0`)
- `STRATEGY_MIN_LIQUIDITY` (optional): Minimum bid liquidity required (default: `10.0`)

### 6. Resistance Level Preset (`resistance_level`)

```cpp
std::map<std::string, std::string> config = {
    {"STRATEGY_TYPE", "resistance_level"},
    {"STRATEGY_SYMBOL", "BTC/USD"},
    {"STRATEGY_LEVEL", "50000.0"},
    {"STRATEGY_TOLERANCE", "1.0"},
    {"STRATEGY_MIN_LIQUIDITY", "10.0"}
};

auto strategy = StrategyConfig::from_map(config);
```

**Configuration Keys:**
- `STRATEGY_TYPE` (required): `"resistance_level"`
- `STRATEGY_SYMBOL` (required): Trading pair symbol
- `STRATEGY_LEVEL` (required): Resistance price level
- `STRATEGY_TOLERANCE` (optional): Price tolerance percentage (default: `1.0`)
- `STRATEGY_MIN_LIQUIDITY` (optional): Minimum ask liquidity required (default: `10.0`)

## Using Environment Variables

```cpp
// Set environment variables
setenv("STRATEGY_TYPE", "price_alert", 1);
setenv("STRATEGY_SYMBOL", "BTC/USD", 1);
setenv("STRATEGY_ABOVE", "50000.0", 1);
setenv("STRATEGY_RECURRING", "true", 1);

// Create strategy from environment
auto strategy = StrategyConfig::from_env("STRATEGY_");
if (strategy) {
    client.add_alert(strategy, [](const Alert& alert) {
        std::cout << "Alert: " << alert.message << std::endl;
    });
}
```

## Using Config Files

Config files can be loaded using the existing config file loading mechanism:

```bash
# config.cfg
STRATEGY_TYPE=price_alert
STRATEGY_SYMBOL=BTC/USD
STRATEGY_ABOVE=50000.0
STRATEGY_RECURRING=true
```

```cpp
// Load config file (sets environment variables)
examples::load_config_file("config.cfg");

// Create strategy from environment
auto strategy = StrategyConfig::from_env("STRATEGY_");
```

## Custom Prefix

You can use a custom prefix for configuration keys:

```cpp
std::map<std::string, std::string> config = {
    {"MY_TYPE", "price_alert"},
    {"MY_SYMBOL", "ETH/USD"},
    {"MY_ABOVE", "3000.0"}
};

auto strategy = StrategyConfig::from_map(config, "MY_");
```

## Error Handling

Invalid configurations throw `std::invalid_argument`:

```cpp
try {
    auto strategy = StrategyConfig::from_map(config);
} catch (const std::invalid_argument& e) {
    std::cerr << "Invalid strategy configuration: " << e.what() << std::endl;
}
```

Common errors:
- Missing `STRATEGY_TYPE`
- Missing required keys (e.g., `STRATEGY_SYMBOL`)
- Unknown strategy type
- Invalid numeric values

## Examples

See `examples/strategies.cpp` for a complete example demonstrating strategy configuration and usage.

