# Stress Testing & Failure Injection

**Purpose:** Comprehensive stress tests and failure injection tests designed to uncover potential bugs, memory leaks, race conditions, and failure scenarios that could break production systems.

---

## Overview

The stress test suite (`test_stress_failure.cpp`) contains **50+ test cases** that intentionally push the SDK to its limits and test failure scenarios that normal unit tests don't cover.

---

## Test Categories

### 1. Queue Stress Tests

**Purpose:** Test queue behavior under extreme conditions

| Test | What It Tests | Potential Issues Found |
|------|---------------|------------------------|
| `QueueSaturation` | Fill queue to capacity rapidly | Memory issues, overflow handling |
| `QueueProducerConsumerMismatch` | Fast producer, slow consumer | Message drops, deadlocks, resource leaks |
| `QueueBurstPattern` | Rapid bursts followed by quiet periods | Memory fragmentation, performance degradation |

**Key Scenarios:**
- Queue filled to capacity (1024+ messages)
- Producer/consumer speed mismatch (10,000 messages, slow consumer)
- Burst patterns (100 bursts of 100 messages each)

---

### 2. Parser Stress Tests - Malformed Data

**Purpose:** Test parser resilience against invalid/corrupted input

| Test | What It Tests | Potential Issues Found |
|------|---------------|------------------------|
| `ParserMalformedJSON` | Extremely malformed JSON | Crashes, memory corruption, infinite loops |
| `ParserLargeJSON` | Very large valid JSON (1000+ items) | Memory exhaustion, stack overflow |
| `ParserDeepNesting` | Deeply nested JSON (100 levels) | Stack overflow, recursion limits |
| `ParserBinaryData` | Binary/non-UTF-8 data | Buffer overflows, encoding issues |
| `ParserMissingFields` | Missing required fields | Null pointer dereferences |
| `ParserWrongTypes` | Wrong data types in JSON | Type conversion errors, crashes |
| `ParserUnicodeSpecialChars` | Unicode and control characters | Encoding issues, buffer overflows |

**Key Scenarios:**
- Empty, incomplete, or invalid JSON structures
- Extremely large JSON payloads (1000+ order book levels)
- Deeply nested structures (100+ levels)
- Binary data injection
- Missing required fields
- Type mismatches (string instead of number, etc.)
- Unicode and control characters

---

### 3. Rate Limiter Stress Tests

**Purpose:** Test rate limiter under extreme load

| Test | What It Tests | Potential Issues Found |
|------|---------------|------------------------|
| `RateLimiterExtremeLoad` | 50,000 rapid requests | Token bucket overflow, precision issues |
| `RateLimiterConcurrentStress` | 20 threads, 1000 requests each | Race conditions, deadlocks |

**Key Scenarios:**
- 50,000 rapid-fire requests
- 20 concurrent threads all acquiring tokens
- High rate (1000 req/sec) with large burst (10,000)

---

### 4. Memory Stress Tests

**Purpose:** Detect memory leaks and excessive memory usage

| Test | What It Tests | Potential Issues Found |
|------|---------------|------------------------|
| `RapidObjectLifecycle` | Create/destroy 1000 clients | Memory leaks, resource leaks |
| `ManySubscriptions` | 1000 concurrent subscriptions | Memory growth, resource exhaustion |
| `ManyOrderBooks` | 1000 order books for different symbols | Memory leaks, map growth issues |

**Key Scenarios:**
- 1000 rapid client create/destroy cycles
- 1000 concurrent subscriptions
- 1000 order books for different symbols

---

### 5. Threading Stress Tests

**Purpose:** Find race conditions and thread safety issues

| Test | What It Tests | Potential Issues Found |
|------|---------------|------------------------|
| `ConcurrentOperationsStress` | 10 threads doing operations simultaneously | Race conditions, deadlocks |
| `RapidStartStop` | 100 rapid start/stop cycles | Resource leaks, thread cleanup issues |
| `CallbackRegistrationRace` | Callback changes during operation | Use-after-free, race conditions |
| `MetricsAccessRace` | Metrics access during high activity | Data races, incorrect values |

**Key Scenarios:**
- 10 threads doing 100 operations each
- 100 rapid start/stop cycles
- Callback registration while client is running
- Metrics access during high message throughput

---

### 6. Invalid State Tests

**Purpose:** Test SDK behavior with invalid configurations or states

| Test | What It Tests | Potential Issues Found |
|------|---------------|------------------------|
| `OperationsOnStoppedClient` | Operations after client stopped | Crashes, undefined behavior |
| `InvalidConfiguration` | Zero queue capacity, invalid URLs | Validation errors, crashes |
| `InvalidTimeouts` | Negative or zero timeouts | Timeout handling issues |

**Key Scenarios:**
- Subscribe/unsubscribe after client stopped
- Zero queue capacity
- Invalid URLs
- Negative timeouts

---

### 7. Data Corruption Tests

**Purpose:** Test SDK resilience to corrupted data

| Test | What It Tests | Potential Issues Found |
|------|---------------|------------------------|
| `CorruptedChecksum` | Order book with invalid checksum | Checksum validation failures |
| `CorruptedNumericValues` | Inf, NaN, overflow values | Numeric handling issues, crashes |

**Key Scenarios:**
- Order book updates with invalid checksums
- JSON with `inf`, `-inf`, `nan` values
- Extremely large numbers (1e999)

---

### 8. Long-Running Stability Tests

**Purpose:** Test SDK stability over extended periods

| Test | What It Tests | Potential Issues Found |
|------|---------------|------------------------|
| `MetricsAccumulation` | 1,000,000 metric accesses | Integer overflow, memory leaks |
| `RapidSubscriptionChanges` | 1000 pause/resume cycles | State corruption, resource leaks |

**Key Scenarios:**
- 1,000,000 metric read operations
- 1000 rapid pause/resume cycles

---

### 9. Resource Exhaustion Tests

**Purpose:** Test SDK behavior when resources are limited

| Test | What It Tests | Potential Issues Found |
|------|---------------|------------------------|
| `MinimalQueueSize` | Queue size of 4 | Queue overflow handling |
| `MaximumQueueSize` | Queue size of 1M | Memory allocation issues |
| `ManyConcurrentClients` | 100 concurrent clients | File descriptor exhaustion |

**Key Scenarios:**
- Minimal queue (4 messages)
- Maximum queue (1M messages)
- 100 concurrent client instances

---

### 10. Race Condition Stress Tests

**Purpose:** Find race conditions under concurrent access

| Test | What It Tests | Potential Issues Found |
|------|---------------|------------------------|
| `CallbackRegistrationRace` | Callback changes during operation | Use-after-free, race conditions |
| `MetricsAccessRace` | Metrics access during high activity | Data races, incorrect values |

**Key Scenarios:**
- 1000 callback registrations while client is running
- Continuous metrics access during 100 subscriptions

---

### 11. Invalid Input Stress Tests

**Purpose:** Test SDK with malicious or invalid input

| Test | What It Tests | Potential Issues Found |
|------|---------------|------------------------|
| `ExtremelyLongSymbols` | 10,000 character symbol names | Buffer overflows, memory issues |
| `SpecialCharacterSymbols` | Null bytes, control characters | String handling issues, crashes |
| `NullCallbacks` | Empty/null callbacks | Null pointer dereferences |

**Key Scenarios:**
- 10,000 character symbol names
- Symbols with null bytes, control characters
- Empty callback functions

---

### 12. Order Book Stress Tests

**Purpose:** Test order book with extreme data

| Test | What It Tests | Potential Issues Found |
|------|---------------|------------------------|
| `OrderBookManyLevels` | 10,000 price levels | Memory usage, performance degradation |
| `RapidOrderBookUpdates` | 10,000 rapid updates | State corruption, performance issues |
| `OrderBookDuplicatePrices` | Duplicate price levels | Map insertion issues, data corruption |

**Key Scenarios:**
- 10,000 price levels per side
- 10,000 rapid updates
- Duplicate prices in same update

---

### 13. Configuration Stress Tests

**Purpose:** Test SDK with extreme configuration values

| Test | What It Tests | Potential Issues Found |
|------|---------------|------------------------|
| `InvalidRateLimiterConfig` | Zero/negative rates | Division by zero, crashes |
| `ExtremeConfigurationValues` | 1GB queue, size 1 queue | Memory allocation, validation issues |

**Key Scenarios:**
- Zero rate limiter rate
- Zero burst capacity
- Negative rates
- 1GB queue capacity
- Queue size of 1

---

### 14. Integration Stress Tests

**Purpose:** Test all components under stress simultaneously

| Test | What It Tests | Potential Issues Found |
|------|---------------|------------------------|
| `FullSystemStress` | All components stressed together | Component interaction issues, resource contention |

**Key Scenarios:**
- Rate limiter + queue + subscriptions + metrics all under stress
- 100 subscriptions with rapid pause/resume
- 1000 metric accesses
- All happening concurrently

---

## Running Stress Tests

```bash
cd build
cmake .. -DKRAKEN_BUILD_TESTS=ON
make -j$(nproc)
./test_stress_failure
```

Or run with CTest:
```bash
ctest -R test_stress_failure --verbose
```

---

## What These Tests Uncover

### ✅ Issues Found by Stress Tests

1. **Memory Leaks**
   - Rapid object creation/destruction
   - Many concurrent subscriptions
   - Long-running operations

2. **Race Conditions**
   - Concurrent callback registration
   - Metrics access during high activity
   - Concurrent subscription operations

3. **Resource Exhaustion**
   - File descriptor limits
   - Memory allocation failures
   - Queue overflow handling

4. **Data Corruption Handling**
   - Malformed JSON parsing
   - Invalid checksums
   - Corrupted numeric values

5. **Invalid State Handling**
   - Operations on stopped client
   - Invalid configurations
   - Missing required fields

6. **Performance Degradation**
   - Large order books (10,000+ levels)
   - Deep JSON nesting
   - Burst patterns

7. **Edge Cases**
   - Extremely long strings
   - Special characters
   - Binary data injection

---

## Test Coverage Summary

| Category | Test Count | Purpose |
|----------|------------|---------|
| Queue Stress | 3 | Queue overflow, producer/consumer mismatch |
| Parser Stress | 7 | Malformed JSON, large payloads, encoding |
| Rate Limiter | 2 | Extreme load, concurrency |
| Memory Stress | 3 | Memory leaks, resource usage |
| Threading Stress | 4 | Race conditions, thread safety |
| Invalid State | 3 | Invalid configurations, stopped client |
| Data Corruption | 2 | Checksums, numeric values |
| Long-Running | 2 | Stability over time |
| Resource Exhaustion | 3 | Limited resources |
| Race Conditions | 2 | Concurrent access |
| Invalid Input | 3 | Malicious input, edge cases |
| Order Book Stress | 3 | Large books, rapid updates |
| Configuration | 2 | Extreme values |
| Integration | 1 | Full system stress |
| **Total** | **40+** | **Comprehensive failure testing** |

---

## Best Practices

1. **Run Regularly**: Include stress tests in CI/CD pipeline
2. **Monitor Resources**: Watch for memory leaks during long runs
3. **Fix Immediately**: Any failure indicates a potential production issue
4. **Add New Tests**: When bugs are found, add tests to prevent regression

---

## Comparison with Regular Tests

| Aspect | Regular Tests | Stress Tests |
|--------|---------------|--------------|
| **Purpose** | Verify correct behavior | Find breaking scenarios |
| **Input** | Valid, expected data | Invalid, extreme, malicious data |
| **Scale** | Small, focused | Large, comprehensive |
| **Duration** | Fast (< 1 sec) | Slower (seconds to minutes) |
| **Focus** | Happy path + basic errors | Failure modes, edge cases |
| **Goal** | Ensure it works | Ensure it doesn't break |

---

## Conclusion

The stress test suite provides **comprehensive coverage of failure scenarios** that could break the SDK in production. By testing extreme conditions, malformed data, resource exhaustion, and race conditions, we ensure the SDK is **robust and production-ready**.

**All stress tests should pass** - any failure indicates a potential production issue that needs to be fixed.

