# Test Suite

Comprehensive unit tests for the Kraken WebSocket SDK using GoogleTest.

## Test Coverage

### ✅ test_strategies.cpp
- **PriceAlert**: Above/below thresholds, retrigger, symbol filtering
- **VolumeSpike**: Detection with lookback, multiplier thresholds
- **SpreadAlert**: Wide/narrow spread detection

**Tests:** 8 test cases

### ✅ test_book_checksum.cpp
- Snapshot application
- Incremental updates
- Best bid/ask calculation
- Spread calculation
- CRC32 checksum validation
- Multi-symbol management

**Tests:** 8 test cases

### ✅ test_connection.cpp
- URL parsing (valid/invalid formats)
- Connection state management
- Error handling (send/receive when not connected)
- Edge cases

**Tests:** 9 test cases

### ✅ test_parser.cpp
- Valid message parsing (ticker, trade, book, OHLC)
- Invalid JSON handling
- Heartbeat messages
- Subscribe/unsubscribe responses
- Message building (subscribe/unsubscribe)
- Edge cases (empty data, missing fields)

**Tests:** 15 test cases

### ✅ test_client.cpp
- Construction (default, with config)
- Callback registration (all types)
- Subscription management
- Alert strategies (add/remove/multiple)
- Metrics retrieval
- Connection state queries
- Move semantics

**Tests:** 20 test cases

### ✅ test_config.cpp
- Default configuration
- Builder pattern (all options)
- API key/secret handling
- Authentication detection
- Fluent builder chaining

**Tests:** 11 test cases

### ✅ test_subscription.cpp
- Subscription creation (all channels)
- Pause/resume
- Add/remove symbols
- Unsubscribe
- Multiple subscriptions
- Edge cases (duplicates, non-existent symbols, inactive state)

**Tests:** 17 test cases

### ✅ test_error_handling.cpp
- Error callback registration
- Invalid input handling
- Error code enum coverage
- Connection state transitions
- Metrics edge cases
- Stop/disconnect when not running/connected
- Alert management edge cases

**Tests:** 15 test cases

### ✅ test_metrics.cpp
- Initial metrics state
- Messages per second calculation
- Uptime calculation and formatting
- Thread safety
- Connection state tracking
- Queue depth tracking
- Latency tracking

**Tests:** 8 test cases

### ✅ test_integration.cpp
- Full client lifecycle
- Subscription lifecycle flow
- Strategy evaluation flow
- Multiple strategies
- Connection state flow
- Metrics flow
- Error callback flow
- Concurrent operations
- Queue overflow handling

**Tests:** 15 test cases

### ✅ test_message_flow.cpp
- Parser → message creation
- Multiple message types
- Error message flow
- Subscribe/unsubscribe message building
- Strategy with parsed ticker
- Order book parsing flow

**Tests:** 7 test cases

### ✅ test_thread_safety.cpp
- Concurrent callback registration
- Concurrent subscriptions
- Concurrent alert management
- Concurrent metrics access
- Concurrent subscription operations
- Concurrent stop operations
- Concurrent connection state queries

**Tests:** 7 test cases

### ✅ test_edge_cases.cpp
- Extreme price values
- Zero and negative values
- Large order books
- Empty order books
- Exact threshold values
- Zero volume handling
- Negative spreads
- Very small price differences
- Many symbols
- Rapid lifecycle operations
- NaN/infinity handling

**Tests:** 20+ test cases

## Running Tests

```bash
cd build
ctest --output-on-failure
```

Or run individual tests:
```bash
./test_strategies
./test_client
./test_parser
# etc.
```

## Test Statistics

- **Total Test Suites:** 14
- **Total Test Cases:** 166
- **Coverage Areas:**
  - ✅ Happy paths
  - ✅ Error handling
  - ✅ Edge cases
  - ✅ Thread safety (where applicable)
  - ✅ Move semantics
  - ✅ Invalid input handling
  - ✅ Integration flows
  - ✅ Message flow (parse → queue → dispatch)

## Test Philosophy

1. **Isolation**: Each test is independent
2. **Completeness**: Cover both success and failure paths
3. **Clarity**: Test names describe what they test
4. **Speed**: Tests run quickly (< 3 seconds total)
5. **Reliability**: No flaky tests, deterministic results

