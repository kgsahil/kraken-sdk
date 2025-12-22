# Doxygen API Documentation

## Overview

The Kraken WebSocket SDK includes **comprehensive Doxygen documentation** for all public APIs, making it easy for developers to understand and use the SDK effectively. Every class, function, parameter, and return value is documented with clear descriptions, examples, and usage notes.

## Documentation Standards

All documentation follows **Doxygen conventions** for consistency and compatibility with documentation generators:

- `/// @brief` - Brief description of the API
- `/// @param` - Parameter documentation with types and descriptions
- `/// @return` - Return value documentation
- `/// @throws` - Exception documentation
- `/// @note` - Important implementation notes or warnings
- `/// @example` - Code examples showing usage
- `/// @file` - File-level documentation with purpose

**For End Users:** All public APIs include clear descriptions, parameter details, return values, and usage examples to help you integrate the SDK quickly.

## Coverage Summary

### Public API Headers (`include/kraken/`)

✅ **Fully Documented:**
- `client.hpp` - Main KrakenClient class with comprehensive method documentation
- `types.hpp` - All enums, structs, and utility functions documented
- `subscription.hpp` - Subscription handle with lifecycle management docs
- `metrics.hpp` - Metrics struct with all methods documented
- `error.hpp` - Error types and exception classes documented
- `strategies.hpp` - Strategy base class and all built-in strategies documented
- `config.hpp` - ClientConfig and Builder with fluent API docs
- `connection_config.hpp` - Timeout and security configuration documented
- `backoff.hpp` - Backoff strategies documented (already had good coverage)
- `gap_detector.hpp` - Gap detection documented (already had good coverage)
- `telemetry.hpp` - OpenTelemetry integration documented (already had good coverage)
- `queue.hpp` - Message queue interface documented (already had good coverage)
- `logger.hpp` - Structured logging documented (already had good coverage)
- `config_env.hpp` - Environment variable configuration documented (already had good coverage)
- `kraken.hpp` - Main header documented (already had good coverage)

### Internal Headers (`src/internal/`)

✅ **Fully Documented:**
- `connection.hpp` - WebSocket connection implementation documented
- `auth.hpp` - Authentication helpers documented
- `parser.hpp` - JSON message parser documented
- `book_engine.hpp` - Order book engine documented
- `client_impl.hpp` - PIMPL implementation documented

## Documentation Features

### 1. File-Level Documentation
Every header file includes:
- `@file` tag with filename
- `@brief` description of the file's purpose
- Additional context about when/why to use the file

### 2. Class/Struct Documentation
All public classes and structs include:
- Brief description of purpose
- Usage examples where appropriate
- Thread-safety notes where relevant
- Notes about internal vs. public API

### 3. Method Documentation
All public methods include:
- Brief description
- Parameter documentation (`@param`)
- Return value documentation (`@return`)
- Exception documentation (`@throws`) where applicable
- Usage examples for complex methods

### 4. Enum Documentation
All enums include:
- Brief description of purpose
- Documentation for each enum value

### 5. Type Alias Documentation
All callback types and type aliases include:
- Brief description of purpose
- Parameter documentation for callbacks

## Examples

### Class Documentation
```cpp
/// @brief Kraken WebSocket API client
/// 
/// A high-performance SDK for streaming real-time market data from Kraken
/// with built-in trading strategies and performance monitoring.
/// 
/// Thread-safe for:
/// - Callback registration (on_ticker, on_error, etc.)
/// - Subscriptions (subscribe, subscribe_book)
/// - Alert strategies (add_alert, remove_alert)
/// - Connection state queries (is_connected)
/// - Metrics (get_metrics)
/// 
/// @example
/// @code
/// kraken::KrakenClient client;
/// client.on_ticker([](const auto& t) { std::cout << t.last << std::endl; });
/// client.subscribe(kraken::Channel::Ticker, {"BTC/USD"});
/// client.run();
/// @endcode
class KrakenClient {
    // ...
};
```

### Method Documentation
```cpp
/// @brief Subscribe to a channel
/// @param channel The channel to subscribe to
/// @param symbols List of trading pairs (e.g., "BTC/USD")
/// @return Subscription handle for lifecycle management
/// @throws std::invalid_argument if symbols is empty
Subscription subscribe(Channel channel, const std::vector<std::string>& symbols);
```

### Struct Documentation
```cpp
/// @brief Ticker data from Kraken WebSocket
/// 
/// Contains real-time market data for a trading pair including bid, ask, last price,
/// and 24-hour statistics.
struct Ticker {
    std::string symbol;      ///< Trading pair (e.g., "BTC/USD")
    double bid = 0.0;         ///< Best bid price
    double ask = 0.0;         ///< Best ask price
    // ...
};
```

## Generating HTML Documentation

To generate HTML documentation from Doxygen comments:

```bash
# Install Doxygen (if not already installed)
sudo apt-get install doxygen graphviz

# Generate documentation
doxygen Doxyfile

# View documentation
# Open html/index.html in a browser
```

The generated documentation includes:
- **Class/Struct Index** - All data types and classes
- **Function Index** - All public functions and methods
- **File Index** - All header files with descriptions
- **Examples** - Code examples embedded in documentation
- **Search** - Full-text search across all documentation

## Verification

✅ All code compiles successfully
✅ All 17 tests pass
✅ No linter errors
✅ Documentation follows consistent style
✅ All public APIs documented
✅ All internal APIs documented (for maintainability)

## Maintenance

When adding new code:
1. Add `@file` documentation to new headers
2. Document all public classes, structs, and functions
3. Include `@param`, `@return`, `@throws` as appropriate
4. Add `@example` for complex APIs
5. Use `@note` for important implementation details

## Notes

- Internal headers are documented for maintainability, even though they're not part of the public API
- All documentation uses Doxygen-style comments (`///`) for consistency
- Examples are included for complex APIs to aid understanding
- Thread-safety notes are included where relevant

