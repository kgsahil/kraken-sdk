# Configuration Options Roadmap

This document lists additional configuration options that could be added to the SDK.

## ✅ Currently Implemented

### Core Configuration
- ✅ Authentication (API key/secret) - HMAC-SHA512
- ✅ WebSocket URL
- ✅ SPSC Queue (enable/disable, size)
- ✅ Connection retry (exponential backoff with all parameters)
- ✅ Checksum validation
- ✅ Gap detection

### Connection & Security
- ✅ Connection timeouts (connect, read, write, ping, pong)
- ✅ TLS/SSL configuration (verify peer, CA certs, client certs)
- ✅ Cipher suites configuration
- ✅ Insecure connection option (dev only)

### Telemetry
- ✅ Telemetry enable/disable
- ✅ Service name, version, environment
- ✅ Metrics, traces, logs enable flags
- ✅ HTTP server (Prometheus) - /health and /metrics endpoints
- ✅ OTLP export (endpoint, retries, timeout) - HTTP exporter
- ✅ Metrics export interval

### Logging
- ✅ Log level (TRACE, DEBUG, INFO, WARN, ERROR, CRITICAL)
- ✅ Console logging
- ✅ File logging with rotation (5MB per file, 3 files)

---

## ✅ Recently Implemented

### Connection Timeouts ✅ **COMPLETE**
| Variable | Description | Status |
|----------|-------------|--------|
| `WS_CONN_TIMEOUT_MS` | Connection timeout | ✅ Implemented |
| `WS_READ_TIMEOUT_MS` | Read timeout | ✅ Implemented |
| `WS_WRITE_TIMEOUT_MS` | Write timeout | ✅ Implemented |
| `WS_PING_INTERVAL_SEC` | WebSocket ping interval | ✅ Implemented |
| `WS_PONG_TIMEOUT_SEC` | Pong timeout | ✅ Implemented |

**Status:** ✅ Complete - All timeout types configurable via environment variables

### Security Configuration ✅ **COMPLETE**
| Variable | Description | Status |
|----------|-------------|--------|
| `TLS_VERIFY_PEER` | Verify TLS certificates | ✅ Implemented |
| `TLS_CA_CERT_PATH` | Custom CA certificate path | ✅ Implemented |
| `TLS_CLIENT_CERT_PATH` | Client certificate path | ✅ Implemented |
| `TLS_CLIENT_KEY_PATH` | Client private key path | ✅ Implemented |
| `TLS_CIPHER_SUITES` | Allowed cipher suites | ✅ Implemented |
| `ALLOW_INSECURE` | Allow insecure connections | ✅ Implemented |

**Status:** ✅ Complete - Full TLS/SSL configuration support

---

### Performance Tuning
| Variable | Description | Use Case |
|----------|-------------|----------|
| `THREAD_PRIORITY_IO` | I/O thread priority | Real-time trading systems |
| `THREAD_PRIORITY_DISPATCHER` | Dispatcher thread priority | Low-latency processing |
| `CPU_AFFINITY_IO` | CPU affinity for I/O thread | NUMA optimization |
| `CPU_AFFINITY_DISPATCHER` | CPU affinity for dispatcher | NUMA optimization |
| `BATCH_SIZE` | Message batch size | Throughput optimization |
| `CALLBACK_TIMEOUT_MS` | Callback execution timeout | Prevent blocking |

**Priority:** Low - Advanced use cases only

---

### Subscription Defaults
| Variable | Description | Use Case |
|----------|-------------|----------|
| `DEFAULT_ORDERBOOK_DEPTH` | Default order book depth | Consistent depth across subscriptions |
| `MAX_SUBSCRIPTIONS` | Maximum concurrent subscriptions | Resource limits |
| `SUBSCRIPTION_TIMEOUT_MS` | Subscription confirmation timeout | Detect subscription failures |

**Priority:** Low - Can be set programmatically

---

### Rate Limiting (Future Feature)
| Variable | Description | Use Case |
|----------|-------------|----------|
| `RATE_LIMIT_ENABLED` | Enable rate limiting | Prevent API bans |
| `RATE_LIMIT_REQUESTS_PER_SEC` | Request rate limit | API compliance |
| `RATE_LIMIT_BURST_SIZE` | Burst allowance | Handle spikes |
| `RATE_LIMIT_STRATEGY` | Strategy: token_bucket, sliding_window | Different algorithms |

**Priority:** High - Important for production

---

### Circuit Breaker (Future Feature)
| Variable | Description | Use Case |
|----------|-------------|----------|
| `CIRCUIT_BREAKER_ENABLED` | Enable circuit breaker | Prevent cascading failures |
| `CIRCUIT_BREAKER_FAILURE_THRESHOLD` | Failure threshold | Trigger open state |
| `CIRCUIT_BREAKER_SUCCESS_THRESHOLD` | Success threshold | Trigger close state |
| `CIRCUIT_BREAKER_TIMEOUT_MS` | Half-open timeout | Recovery testing interval |

**Priority:** Medium - Enterprise resilience

---

## 🔄 Potential Future Additions

### Remaining Security Options
| Variable | Description | Use Case |
|----------|-------------|----------|
| `TLS_CERT_PINNING` | Certificate pinning | Enhanced security |
| `TLS_MIN_VERSION` | Minimum TLS version | Security hardening |

**Priority:** Low - Advanced security features

---

### Debug/Development
| Variable | Description | Use Case |
|----------|-------------|----------|
| `DEBUG_MODE` | Enable debug mode | Development |
| `VERBOSE_OUTPUT` | Verbose logging | Troubleshooting |
| `DUMP_MESSAGES` | Dump raw messages | Debugging |
| `MOCK_MODE` | Use mock server | Testing |

**Priority:** Low - Development only

---

### Metrics/Statistics
| Variable | Description | Use Case |
|----------|-------------|----------|
| `METRICS_ENABLED` | Enable local metrics | Performance monitoring |
| `METRICS_HISTORY_SIZE` | Metrics history buffer | Trend analysis |
| `STATS_INTERVAL_SEC` | Statistics reporting interval | Periodic reports |

**Priority:** Low - Already covered by telemetry

---

### Message Processing
| Variable | Description | Use Case |
|----------|-------------|----------|
| `MESSAGE_BUFFER_SIZE` | Message buffer size | Memory tuning |
| `MAX_MESSAGE_SIZE` | Maximum message size | Prevent DoS |
| `DROP_ON_QUEUE_FULL` | Drop messages when queue full | Backpressure |
| `CALLBACK_EXECUTOR_THREADS` | Number of callback threads | Parallel processing |

**Priority:** Medium - Performance optimization

---

### Order Book Specific
| Variable | Description | Use Case |
|----------|-------------|----------|
| `ORDERBOOK_MAX_LEVELS` | Maximum order book levels | Memory limits |
| `ORDERBOOK_SNAPSHOT_INTERVAL` | Snapshot save interval | Data collection |
| `ORDERBOOK_VALIDATION_STRICT` | Strict validation mode | Data integrity |

**Priority:** Low - Niche use cases

---

## 📊 Priority Matrix

| Category | Priority | Effort | Impact |
|----------|----------|--------|--------|
| Connection Timeouts | Medium | Low | High |
| Rate Limiting | High | Medium | High |
| Circuit Breaker | Medium | Medium | Medium |
| Security | Medium | Low | High |
| Performance Tuning | Low | High | Low |
| Debug/Development | Low | Low | Low |

---

## 🎯 Recommended Next Steps

1. **Connection Timeouts** (Quick Win)
   - Easy to implement
   - High value for production
   - Low risk

2. **Rate Limiting** (High Value)
   - Important for production
   - Prevents API bans
   - Medium complexity

3. **Security Options** (Security)
   - TLS certificate pinning
   - Custom CA paths
   - Security hardening

4. **Circuit Breaker** (Resilience)
   - Enterprise pattern
   - Prevents cascading failures
   - Medium complexity

---

## 💡 Implementation Notes

### Adding New Environment Variables

1. Add to `include/kraken/config_env.hpp` documentation
2. Add parsing logic to `src/config_env.cpp`
3. Update `docs/ENVIRONMENT_VARIABLES.md`
4. Add example to documentation
5. Consider backward compatibility

### Best Practices

- Use clear, descriptive names
- Provide sensible defaults
- Document units (ms, sec, bytes)
- Validate ranges where applicable
- Support both boolean formats (true/false, 1/0, yes/no)

---

## 📝 Summary

**Current Status:** ✅ **30+ environment variables implemented**

**✅ Recently Completed:**
- ✅ Connection timeouts (5 variables) - **COMPLETE**
- ✅ Security options (6 variables) - **COMPLETE**

**⏳ Recommended Additions:**
- ⏳ Rate limiting (4 variables) - High priority
- ⏳ Circuit breaker (4 variables) - Medium priority

**Total Implemented:** ~30 environment variables  
**Total Potential:** ~38 environment variables for complete enterprise configuration

**Status:** ✅ **Production-Ready** - All critical configuration options available

