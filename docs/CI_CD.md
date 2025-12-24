# CI/CD Pipeline Documentation

## Overview

The Kraken SDK uses GitHub Actions for continuous integration and continuous deployment. The CI/CD pipeline automatically builds and tests the SDK on every push and pull request.

## Workflow Configuration

**Location:** `.github/workflows/ci.yml`

**Triggers:**
- Push to `main` or `develop` branches
- Pull requests targeting `main` or `develop` branches

## Jobs

### 1. Linux Build and Test
- **Platform:** Ubuntu Latest
- **Compiler:** GCC (default)
- **Build Type:** Release
- **Tests:** All 25 GoogleTest suites (328 test cases)
- **Parallel Execution:** Uses all CPU cores (`nproc`)

### 2. Windows Build and Test
- **Platform:** Windows Latest
- **Compiler:** MSVC
- **Build Type:** Release
- **Tests:** All 25 GoogleTest suites (328 test cases)
- **Dependencies:** Installed via Chocolatey (CMake, Boost, OpenSSL)

### 3. macOS Build and Test
- **Platform:** macOS Latest
- **Compiler:** Clang
- **Build Type:** Release
- **Tests:** All 25 GoogleTest suites (328 test cases)
- **Dependencies:** Installed via Homebrew (CMake, Boost, OpenSSL)

### 4. Code Quality Checks
- **Platform:** Ubuntu Latest
- **Tool:** clang-tidy
- **Scope:** All `.cpp` and `.hpp` files in `src/` and `include/`
- **Note:** Non-blocking (continues on error for warnings)

### 5. Test Results Summary
- **Platform:** Ubuntu Latest
- **Purpose:** Aggregates test results from all platforms
- **Output:** GitHub Actions step summary with test statistics

## Test Execution

### GoogleTest Integration

All tests are executed using `ctest`, which runs all GoogleTest executables:

```bash
ctest --output-on-failure --verbose -j$(nproc)
```

**Test Suites (24 total):**
1. `test_strategies` - Trading strategy engine tests
2. `test_book_checksum` - Order book checksum validation
3. `test_connection` - WebSocket connection tests
4. `test_parser` - JSON message parsing tests
5. `test_client` - KrakenClient API tests
6. `test_config` - Configuration builder tests
7. `test_subscription` - Subscription management tests
8. `test_error_handling` - Error handling and recovery tests
9. `test_metrics` - Metrics collection tests
10. `test_integration` - End-to-end integration tests
11. `test_message_flow` - Message flow and queue tests
12. `test_thread_safety` - Concurrency and thread safety tests
13. `test_edge_cases` - Boundary conditions and edge cases
14. `test_exception_safety` - Exception safety and RAII tests
15. `test_rate_limiter` - Rate limiting algorithm tests
16. `test_backoff` - Exponential backoff strategy tests
17. `test_gap_detector` - Message gap detection tests
18. `test_telemetry` - OpenTelemetry integration tests
19. `test_auth` - Authentication (HMAC-SHA512) tests
20. `test_logger` - Structured logging tests
21. `test_queue` - SPSC queue implementation tests
22. `test_config_env` - Environment variable configuration tests
23. `test_connection_config` - Connection timeout and security config tests
24. `test_stress_failure` - Stress and failure injection tests (40+ cases)

**Total Test Cases:** 328

### Test Failure Handling

- If any test fails, the CI job fails
- Test output is captured with `--output-on-failure` flag
- Verbose output (`--verbose`) shows detailed test results
- Test summaries are added to GitHub Actions step summaries

## Build Configuration

### CMake Options

```cmake
-DCMAKE_BUILD_TYPE=Release
-DKRAKEN_BUILD_TESTS=ON
-DKRAKEN_BUILD_EXAMPLES=ON
```

### Dependencies

**Linux:**
- `build-essential` (GCC, make)
- `cmake`
- `libssl-dev` (OpenSSL)
- `libboost-system-dev` (Boost.Beast)
- `pkg-config`

**Windows:**
- CMake (via Chocolatey)
- Boost (via Chocolatey)
- OpenSSL (via Chocolatey)

**macOS:**
- CMake (via Homebrew)
- Boost (via Homebrew)
- OpenSSL@3 (via Homebrew)
- pkg-config (via Homebrew)

## Code Quality

### clang-tidy

The CI pipeline includes static analysis using `clang-tidy`:

- **Scope:** All source files (`src/*.cpp`, `include/**/*.hpp`)
- **Configuration:** Uses `compile_commands.json` from CMake
- **Behavior:** Non-blocking (warnings don't fail the build)

To run locally:
```bash
mkdir -p build
cd build
cmake .. -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cd ..
clang-tidy src/*.cpp include/**/*.hpp -p build
```

## Status Badges

Add to your README.md:

```markdown
![CI](https://github.com/YOUR_USERNAME/kraken-sdk/workflows/CI/badge.svg)
```

## Local Testing

To replicate CI/CD locally:

### Linux
```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake libssl-dev libboost-system-dev pkg-config
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DKRAKEN_BUILD_TESTS=ON
cmake --build . -j$(nproc)
ctest --output-on-failure --verbose -j$(nproc)
```

### Windows (WSL)
```bash
# Same as Linux
```

### macOS
```bash
brew install cmake boost openssl@3 pkg-config
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DKRAKEN_BUILD_TESTS=ON -DOPENSSL_ROOT_DIR=$(brew --prefix openssl@3)
cmake --build . -j$(sysctl -n hw.ncpu)
ctest --output-on-failure --verbose -j$(sysctl -n hw.ncpu)
```

## Future Enhancements

### Planned Additions

1. **Code Coverage**
   - Generate coverage reports using gcov/lcov
   - Upload to Codecov or similar service
   - Coverage badges in README

2. **Performance Benchmarks**
   - Run Google Benchmark suite in CI
   - Track performance regressions
   - Compare against baseline

3. **Dependency Scanning**
   - Dependabot for dependency updates
   - Snyk for vulnerability scanning
   - Automated security alerts

4. **Release Automation**
   - Automatic version tagging
   - Release notes generation
   - Artifact publishing

5. **Multi-Compiler Testing**
   - GCC (multiple versions)
   - Clang (multiple versions)
   - MSVC (multiple versions)

6. **Sanitizer Testing**
   - AddressSanitizer (ASan)
   - ThreadSanitizer (TSan)
   - UndefinedBehaviorSanitizer (UBSan)

## Troubleshooting

### Common Issues

1. **Tests fail on Windows**
   - Check OpenSSL installation path
   - Verify Boost library paths
   - Ensure CMake can find dependencies

2. **Tests fail on macOS**
   - Verify OpenSSL@3 is installed
   - Check `OPENSSL_ROOT_DIR` is set correctly
   - Ensure Homebrew paths are in PATH

3. **clang-tidy errors**
   - These are non-blocking
   - Review warnings locally
   - Fix critical issues before merging

4. **Build timeouts**
   - Reduce parallel jobs if needed
   - Check for dependency download issues
   - Verify network connectivity

## Best Practices

1. **Always run tests locally before pushing**
   ```bash
   cd build
   ctest --output-on-failure --verbose
   ```

2. **Keep tests fast**
   - Unit tests should complete in seconds
   - Integration tests may take longer
   - Avoid long-running tests in CI

3. **Test on multiple platforms**
   - Test on Linux before pushing
   - Verify Windows compatibility
   - Check macOS if available

4. **Monitor CI regularly**
   - Check CI status before merging PRs
   - Fix failing tests immediately
   - Keep CI green

---


