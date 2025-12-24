# Code Refactoring Plan

## ✅ Completed Phases

### Phase 1: Strategies Module Refactoring ✅
- Split `strategies.hpp` (912 lines) into modular structure:
  - `strategies/base.hpp` - Base classes and Alert struct
  - `strategies/price_alert.hpp` - PriceAlert strategy
  - `strategies/volume_spike.hpp` - VolumeSpike strategy
  - `strategies/spread_alert.hpp` - SpreadAlert strategy
  - `strategies/composite.hpp` - CompositeStrategy (AND/OR logic)
  - `strategies/presets.hpp` - StrategyPresets factory
  - `strategies/strategy_config.hpp` - Configuration support (moved from root)
  - `strategies/strategies.hpp` - Main include file

### Phase 2: Client Implementation Refactoring ✅
- Split `client_impl.cpp` (876 lines) into 8 focused modules:
  - `src/client/lifecycle.cpp` - Construction, connection, event loop (~200 lines)
  - `src/client/callbacks.cpp` - Callback registration (~70 lines)
  - `src/client/subscriptions.cpp` - Subscription management (~110 lines)
  - `src/client/strategies.cpp` - Strategy management (~45 lines)
  - `src/client/dispatch.cpp` - Message dispatch and I/O loops (~270 lines)
  - `src/client/reconnect.cpp` - Reconnection logic (~100 lines)
  - `src/client/snapshots.cpp` - Data snapshots (~40 lines)
  - `src/client/metrics.cpp` - Metrics collection (~45 lines)
- Extracted `SubscriptionImpl` to `src/client/subscription_impl.cpp`
- Extracted `StrategyEngine` to `src/client/strategy_engine_impl.cpp`

**Results:**
- `client_impl.cpp`: 876 → ~30 lines (97% reduction)
- All 25 tests passing
- Improved code organization and maintainability
- Better separation of concerns

---

## ✅ Refactoring Complete

All four phases of refactoring have been completed successfully:

1. ✅ **Phase 1: Strategies Module** - Split into modular structure
2. ✅ **Phase 2: Client Implementation** - Split into 8 focused modules
3. ✅ **Phase 3: Telemetry Module** - Split into modular structure
4. ✅ **Phase 4: Project Structure** - Reorganized into module-based directories

**Results:**
- Improved code organization and maintainability
- Faster compilation (smaller, focused headers)
- Better separation of concerns
- Easier to navigate and understand
- All 25 test suites passing
- Documentation updated

**Current Structure:**
- Modular organization: `core/`, `strategies/`, `telemetry/`, `connection/`
- Each module in its own directory with focused files
- Clear separation of public API and implementation

---

## Proposed Refactoring

### 1. Split Strategies Module

**Current:** `include/kraken/strategies.hpp` (912 lines)

**Proposed Structure:**
```
include/kraken/strategies/
├── base.hpp              # AlertStrategy base class, Alert struct
├── price_alert.hpp       # PriceAlert class
├── volume_spike.hpp      # VolumeSpike class
├── spread_alert.hpp      # SpreadAlert class
├── composite.hpp         # CompositeStrategy class
├── presets.hpp           # StrategyPresets class
└── strategies.hpp        # Main include (includes all above)
```

**Benefits:**
- Each strategy in its own file (~100-200 lines each)
- Easy to find and understand individual strategies
- Faster compilation (only include what you need)
- Better organization

**Migration:**
- Keep `strategies.hpp` as main include for backward compatibility
- Move classes to separate files
- Update includes in existing code

---

### 2. Split Client Implementation

**Current:** `src/client_impl.cpp` (1152 lines)

**Proposed Structure:**
```
src/client/
├── client_impl_core.cpp       # Core client lifecycle (constructor, run, stop)
├── client_impl_subscriptions.cpp  # Subscription management
├── client_impl_strategies.cpp     # Strategy engine integration
├── client_impl_callbacks.cpp     # Callback handling and dispatch
├── client_impl_snapshots.cpp     # Latest ticker/book snapshots
└── client_impl.cpp              # Main file (includes all or delegates)
```

**Or better - split by responsibility:**
```
src/client/
├── lifecycle.cpp          # Construction, run, stop, connection management
├── subscriptions.cpp      # subscribe(), unsubscribe(), subscription state
├── strategies.cpp         # add_alert(), remove_alert(), strategy evaluation
├── callbacks.cpp          # on_ticker(), on_book(), callback dispatch
├── snapshots.cpp          # latest_ticker(), latest_book()
└── client_impl.cpp       # Main PIMPL class (delegates to above)
```

**Benefits:**
- Each file has single responsibility (~200-300 lines)
- Easy to locate specific functionality
- Better testability
- Faster compilation

---

### 3. Split Telemetry Module

**Current:** `src/telemetry.cpp` (511 lines), `include/kraken/telemetry.hpp` (441 lines)

**Proposed Structure:**
```
include/kraken/telemetry/
├── telemetry.hpp          # Main Telemetry class
├── otlp_exporter.hpp      # OtlpHttpExporter class
└── prometheus_server.hpp  # PrometheusHttpServer class

src/telemetry/
├── telemetry.cpp          # Main Telemetry implementation
├── otlp_exporter.cpp      # OTLP export logic
└── prometheus_server.cpp  # Prometheus server logic
```

---

### 4. Better Project Structure

**Current Structure:**
```
include/kraken/          # All headers flat
src/                     # All implementation flat
src/internal/            # Private headers
```

**Proposed Structure:**
```
include/kraken/
├── core/                 # Core SDK functionality
│   ├── client.hpp
│   ├── config.hpp
│   ├── types.hpp
│   └── error.hpp
├── strategies/           # Strategy engine (see above)
├── telemetry/           # Telemetry (see above)
├── connection/          # Connection management
│   ├── backoff.hpp
│   ├── connection_config.hpp
│   └── gap_detector.hpp
└── kraken.hpp           # Main include

src/
├── core/                # Core implementation
│   ├── client/
│   └── config/
├── strategies/          # Strategy implementations
├── telemetry/           # Telemetry implementations
├── connection/          # Connection implementations
└── internal/            # Private implementation details
```

**Benefits:**
- Clear module boundaries
- Easy to navigate
- Better organization
- Scalable structure

---

## Implementation Plan

### Phase 1: Strategies Module (High Priority)
1. Create `include/kraken/strategies/` directory
2. Split `strategies.hpp` into separate files
3. Create `strategies.hpp` that includes all (backward compatible)
4. Update tests and examples
5. Verify compilation

**Estimated Effort:** 2-3 hours

### Phase 2: Client Implementation (High Priority)
1. Create `src/client/` directory
2. Split `client_impl.cpp` by responsibility
3. Update `client_impl.hpp` if needed
4. Update includes
5. Verify compilation and tests

**Estimated Effort:** 3-4 hours

### Phase 3: Telemetry Module Refactoring ✅ **COMPLETED**
- Split `telemetry.hpp` (441 lines) and `telemetry.cpp` (511 lines) into modular structure:
  - `telemetry/telemetry.hpp` - Main Telemetry class
  - `telemetry/config.hpp` - TelemetryConfig and Builder
  - `telemetry/metrics_collector.hpp` - MetricsCollector class
  - `telemetry/prometheus_server.hpp` - PrometheusHttpServer class
  - `telemetry/otlp_exporter.hpp` - OtlpHttpExporter class
  - Corresponding `.cpp` files in `src/telemetry/`
- All tests passing
- All includes updated

### Phase 4: Project Structure Reorganization ✅ **COMPLETED**
- Reorganized into module-based structure:
  - `include/kraken/core/` - Core SDK (client, config, types, error)
  - `include/kraken/strategies/` - Strategy engine (modular)
  - `include/kraken/telemetry/` - Telemetry (modular)
  - `include/kraken/connection/` - Connection management
  - `src/core/`, `src/strategies/`, `src/telemetry/`, `src/connection/` - Implementation modules
- Updated all includes across codebase
- Updated CMakeLists.txt
- All tests passing
- Documentation updated

---

## Backward Compatibility

**Critical:** Maintain backward compatibility during refactoring!

1. **Keep main includes:** `strategies.hpp`, `client.hpp` still work
2. **Forward includes:** Main files include the split files
3. **Gradual migration:** Users can migrate to new includes over time
4. **Documentation:** Update docs to show new structure

---

## Example: Strategies Refactoring

### Before (strategies.hpp - 912 lines):
```cpp
// Everything in one file
class AlertStrategy { ... };
class PriceAlert : public AlertStrategy { ... };
class VolumeSpike : public AlertStrategy { ... };
class SpreadAlert : public AlertStrategy { ... };
class CompositeStrategy : public AlertStrategy { ... };
class StrategyPresets { ... };
```

### After (modular structure):

**include/kraken/strategies/base.hpp:**
```cpp
#pragma once
#include "types.hpp"
// ... AlertStrategy base class, Alert struct
```

**include/kraken/strategies/price_alert.hpp:**
```cpp
#pragma once
#include "base.hpp"
// ... PriceAlert class only
```

**include/kraken/strategies/strategies.hpp:**
```cpp
#pragma once
// Main include - includes all strategies
#include "base.hpp"
#include "price_alert.hpp"
#include "volume_spike.hpp"
#include "spread_alert.hpp"
#include "composite.hpp"
#include "presets.hpp"
```

**Usage (backward compatible):**
```cpp
#include <kraken/strategies.hpp>  // Still works!
// Or use specific includes:
#include <kraken/strategies/price_alert.hpp>  // Only what you need
```

---

## Benefits Summary

### For Users:
- ✅ **Easier to understand** - Smaller, focused files
- ✅ **Faster compilation** - Include only what you need
- ✅ **Better navigation** - Clear file structure
- ✅ **Backward compatible** - Existing code still works

### For Maintainers:
- ✅ **Easier to modify** - Changes isolated to specific files
- ✅ **Better testing** - Test individual components
- ✅ **Clearer responsibilities** - Each file has one job
- ✅ **Scalable** - Easy to add new strategies/features

---

## ✅ Refactoring Summary

All refactoring phases completed successfully. The codebase is now:
- **Modular** - Clear module boundaries (core, strategies, telemetry, connection)
- **Maintainable** - Small, focused files (~100-300 lines each)
- **Scalable** - Easy to add new features and strategies
- **Well-tested** - All 25 test suites passing (260+ test cases)
- **Well-documented** - Documentation updated to reflect new structure

**Key Improvements:**
- Strategies: 912 lines → 7 modular files (~100-200 lines each)
- Client Implementation: 876 lines → 8 focused modules (~40-270 lines each)
- Telemetry: 441+511 lines → 5 modular files (~100-200 lines each)
- Project Structure: Flat → Module-based (core/, strategies/, telemetry/, connection/)

**Next Steps:**
- Continue adding features using the new modular structure
- Maintain code quality and test coverage
- Keep documentation up to date

