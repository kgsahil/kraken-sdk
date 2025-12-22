# Internal Headers

This directory contains **private implementation headers** that are not part of the public API.

## Purpose

These headers are used internally by the SDK implementation and should **never** be included by user code.

## Headers

- `connection.hpp` - Internal WebSocket connection implementation
- `client_impl.hpp` - PIMPL implementation details for `KrakenClient`
- `auth.hpp` - Internal authentication helpers
- `parser.hpp` - Internal JSON message parsing
- `book_engine.hpp` - Internal order book management

## Usage

These headers are only included by:
- Implementation files in `src/`
- Test files in `tests/` (for unit testing internals)
- Benchmark files in `benchmarks/` (for performance testing)

## Public API

Users should only include headers from `include/kraken/`:
```cpp
#include <kraken/kraken.hpp>  // ✅ Correct
#include <kraken/client.hpp>  // ✅ Correct
```

**Never include internal headers:**
```cpp
#include "internal/client_impl.hpp"  // ❌ Wrong - internal only
```

## Structure Rationale

This directory structure provides:
- **Clear separation** between public and private APIs
- **Better organization** as the project grows
- **Easier maintenance** - internal changes don't affect public API
- **Standard practice** for scalable C++ projects

