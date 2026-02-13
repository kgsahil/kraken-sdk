# Chapter 1: Modern C++17 Features

> Every C++17 feature the Kraken SDK relies on, explained with theory and project examples.

---

## Table of Contents
- [1.1 std::variant — Type-Safe Unions](#11-stdvariant--type-safe-unions)
- [1.2 std::optional — Nullable Values](#12-stdoptional--nullable-values)
- [1.3 std::function & Lambdas — Callbacks](#13-stdfunction--lambdas--callbacks)
- [1.4 Smart Pointers — Ownership Semantics](#14-smart-pointers--ownership-semantics)
- [1.5 std::chrono — Time & Duration](#15-stdchrono--time--duration)
- [1.6 Scoped Enums (enum class)](#16-scoped-enums-enum-class)
- [1.7 std::array vs C-Style Arrays](#17-stdarray-vs-c-style-arrays)
- [1.8 Structured Bindings](#18-structured-bindings)
- [1.9 Modern Class Control (= delete, = default)](#19-modern-class-control--delete--default)
- [1.10 Move Semantics](#110-move-semantics)

---

## 1.1 `std::variant` — Type-Safe Unions

### What Is It?
`std::variant` is a **type-safe union**. It can hold one value from a fixed set of types at any time, but — unlike a raw C `union` — it knows *which* type it's currently holding and will call the correct destructor.

### Why It Matters
In the SDK, every message from the WebSocket can be a Ticker, Trade, OrderBook, OHLC, Error, etc. Before C++17, you'd need either:
- An unsafe `union` (no destructors, no type tracking)
- A base class pointer with `dynamic_cast` (heap allocation, virtual dispatch overhead)

`std::variant` gives you **zero overhead** type safety on the stack.

### Where It's Used

📄 **File:** `src/internal/client_impl.hpp` (lines 63–77)

```cpp
using MessageData = std::variant<
    std::monostate,    // Default/empty state
    Ticker,
    Trade,
    OrderBook,
    OHLC,
    Order,
    OwnTrade,
    std::unordered_map<std::string, Balance>,
    Error,
    SubscribedMsg,
    UnsubscribedMsg,
    HeartbeatMsg
>;
```

### Key Companion Utilities

| Utility | Purpose | Example |
|---------|---------|---------|
| `std::get<T>(v)` | Extract value (throws if wrong type) | `auto& ticker = std::get<Ticker>(msg.data);` |
| `std::holds_alternative<T>(v)` | Check which type is stored | `if (msg.holds<Ticker>()) { ... }` |
| `std::monostate` | Represents "empty" state | First type in the variant (default) |
| `std::visit` | Apply a visitor function to the active type | Pattern matching on message types |

### The `Message` Wrapper

The SDK wraps the variant in a `Message` struct with helper templates:

```cpp
struct Message {
    MessageType type = MessageType::Heartbeat;
    MessageData data;
    std::chrono::steady_clock::time_point receive_time;

    // Type-safe access helpers
    template<typename T>
    const T& get() const { return std::get<T>(data); }

    template<typename T>
    bool holds() const { return std::holds_alternative<T>(data); }
};
```

### 💡 Key Insight
The `Message` struct uses **both** an `enum MessageType` and the variant. The enum is a fast O(1) type check (an integer comparison), while the variant holds the actual data. This avoids the overhead of `std::visit` on the hot path.

---

## 1.2 `std::optional` — Nullable Values

### What Is It?
`std::optional<T>` is a wrapper that either contains a value of type `T` or contains **nothing** (`std::nullopt`). It's C++'s answer to null pointers, but for value types.

### Why It Matters
When you call `latest_ticker("BTC/USD")`, the SDK might not have received any ticker data yet. Before C++17, you'd return a pointer (null if missing) or use a sentinel value. `std::optional` is explicit and safe.

### Where It's Used

📄 **File:** `include/kraken/core/client.hpp` (lines 235–240)

```cpp
/// Get the latest ticker for a symbol (thread-safe copy)
std::optional<Ticker> latest_ticker(const std::string& symbol) const;

/// Get the latest order book for a symbol (thread-safe copy)
std::optional<OrderBook> latest_book(const std::string& symbol) const;
```

### Usage Pattern

```cpp
auto ticker = client.latest_ticker("BTC/USD");

if (ticker) {                              // Check if value exists
    std::cout << ticker->last << std::endl; // Access via ->
    Ticker t = *ticker;                     // Or dereference with *
} else {
    std::cout << "No data yet" << std::endl;
}

// Or use value_or() for a default:
double last_price = client.latest_ticker("BTC/USD")
                         .value_or(Ticker{}).last;
```

### 💡 Key Insight
`std::optional` is a **value type** — it stores the object inline (no heap allocation). When the SDK returns `std::optional<Ticker>`, it's copying the Ticker into the optional on the stack, which is important for thread safety (the copy is your own, not shared state).

---

## 1.3 `std::function` & Lambdas — Callbacks

### What Is It?
`std::function<void(const Ticker&)>` is a **type-erased callable wrapper**. It can hold any callable thing — a lambda, a function pointer, a functor object — as long as the signature matches.

### Why It Matters
The entire SDK is callback-driven. Users register handler functions that get called when data arrives. `std::function` allows the SDK to store *any* kind of callable without knowing its concrete type.

### Where It's Used

📄 **File:** `include/kraken/core/types.hpp` (lines 412–439)

```cpp
using TickerCallback   = std::function<void(const Ticker&)>;
using TradeCallback    = std::function<void(const Trade&)>;
using BookCallback     = std::function<void(const std::string& symbol, const OrderBook&)>;
using OHLCCallback     = std::function<void(const OHLC&)>;
using OrderCallback    = std::function<void(const Order&)>;
using OwnTradeCallback = std::function<void(const OwnTrade&)>;
using BalanceCallback  = std::function<void(const std::unordered_map<std::string, Balance>&)>;
```

### Lambda Capture Modes

```cpp
// By value (safe, each callback has its own copy)
int threshold = 50000;
client.on_ticker([threshold](const Ticker& t) {
    if (t.last > threshold) { /* ... */ }
});

// By reference (careful! reference must outlive the callback)
client.on_ticker([&threshold](const Ticker& t) {
    if (t.last > threshold) { /* ... */ }
});

// Move capture (transfer ownership into lambda)
auto logger = std::make_unique<Logger>();
client.on_ticker([log = std::move(logger)](const Ticker& t) {
    log->info(t.symbol);
});
```

### ⚠️ Performance Note
`std::function` has a cost: it uses **type erasure** internally, which typically means a heap allocation for the stored callable (unless it fits in the Small Buffer Optimization, usually ~16-32 bytes). For this SDK, this is fine because callbacks are registered once during setup, not on the hot path.

---

## 1.4 Smart Pointers — Ownership Semantics

### The Three Smart Pointers

| Pointer | Ownership | Use Case in SDK |
|---------|-----------|-----------------|
| `std::unique_ptr<T>` | **Exclusive** — one owner | PIMPL (`impl_`), BackoffStrategy, MessageQueue |
| `std::shared_ptr<T>` | **Shared** — reference counted | AlertStrategy (shared between client and engine) |
| `std::weak_ptr<T>` | **Non-owning** observer | (Not used in this project) |

### `unique_ptr` — The PIMPL Backbone

📄 **File:** `include/kraken/core/client.hpp` (lines 252–254)

```cpp
class KrakenClient {
private:
    class Impl;                    // Forward declaration only
    std::unique_ptr<Impl> impl_;   // Exclusive ownership
};
```

**Why `unique_ptr` here?** Because:
1. The client **exclusively owns** its implementation — no shared ownership needed
2. `unique_ptr` can hold an **incomplete type** (forward-declared `Impl`), which is essential for PIMPL
3. Zero overhead compared to `new`/`delete` — same performance, but exception-safe

### `shared_ptr` — Shared Strategies

📄 **File:** `include/kraken/core/client.hpp` (line 159)

```cpp
int add_alert(std::shared_ptr<AlertStrategy> strategy, AlertCallback callback);
```

**Why `shared_ptr` here?** Because the strategy could be shared:
- A `CompositeStrategy` holds `shared_ptr`s to sub-strategies
- The user might want to keep a reference to enable/disable the strategy later
- The `StrategyEngine` needs to own it to evaluate it

### `make_unique` and `make_shared`

```cpp
// Preferred: exception-safe, single allocation
auto config = std::make_unique<ClientConfig>(/* ... */);
auto strategy = std::make_shared<PriceAlert>("BTC/USD", 50000.0);

// Avoid: two allocations, not exception-safe
auto config = std::unique_ptr<ClientConfig>(new ClientConfig(/* ... */));
```

---

## 1.5 `std::chrono` — Time & Duration

### What Is It?
`std::chrono` is C++'s type-safe time library. Instead of passing raw integers that might be milliseconds, seconds, or minutes (a common bug source), you pass strongly-typed durations.

### Where It's Used

📄 **File:** `include/kraken/connection/backoff.hpp`

```cpp
// Type-safe durations — the compiler prevents unit confusion
std::chrono::milliseconds initial_delay_{1000};   // 1 second
std::chrono::milliseconds max_delay_{60000};       // 60 seconds

// Builder accepts chrono types directly
builder.initial_delay(std::chrono::seconds(1));    // Auto-converts to ms
builder.max_delay(std::chrono::minutes(2));         // Auto-converts to ms
```

### Key Types

| Type | Purpose | Example |
|------|---------|---------|
| `steady_clock` | Monotonic clock (never goes backward) | Measuring latency |
| `system_clock` | Wall-clock time | Timestamps for alerts |
| `milliseconds`, `seconds`, etc. | Duration types | Backoff delays, timeouts |
| `time_point` | A specific moment in time | `receive_time` in `Message` |

### Duration Arithmetic

```cpp
using namespace std::chrono;

auto start = steady_clock::now();
// ... do work ...
auto elapsed = steady_clock::now() - start;
auto ms = duration_cast<milliseconds>(elapsed).count();
```

### 💡 Key Insight
The SDK uses `steady_clock` for internal timing (latency, backoff) because it's monotonic — it never jumps backward if the system clock is adjusted. `system_clock` is only used for user-facing timestamps.

---

## 1.6 Scoped Enums (`enum class`)

### What Is It?
`enum class` creates a **scoped, strongly-typed** enumeration. Unlike plain `enum`, the values don't leak into the enclosing scope and don't implicitly convert to integers.

### Where It's Used

📄 **File:** `include/kraken/core/types.hpp` (lines 24–72)

```cpp
enum class Channel : std::uint8_t {
    Ticker,
    Trade,
    Book,
    OHLC,
    OwnTrades,
    OpenOrders,
    Balances
};

enum class ConnectionState {
    Disconnected,
    Connecting,
    Connected,
    Reconnecting
};
```

### Why `: std::uint8_t`?

Specifying the underlying type (`: std::uint8_t`) gives you:
1. **Guaranteed size** — exactly 1 byte (instead of compiler-dependent `int`)
2. **Memory efficiency** — important when stored in frequently-created structs
3. **ABI stability** — same binary layout across compilers

### Comparison: Plain `enum` vs `enum class`

```cpp
// ❌ Plain enum — leaks into scope, implicit int conversion
enum Color { Red, Green, Blue };
int x = Red;              // Compiles — dangerous!
if (Red == 0) { ... }     // Compiles — confusing!

// ✅ Scoped enum — safe, explicit
enum class Color { Red, Green, Blue };
int x = Color::Red;       // Error! No implicit conversion
Color c = Color::Red;     // Must use scope prefix
```

---

## 1.7 `std::array` vs C-Style Arrays

### Where It's Used

📄 **File:** `include/kraken/core/types.hpp` (line 111)

```cpp
std::string to_json() const {
    std::array<char, 512> buf{};   // Stack-allocated, bounds-checked
    const int result = snprintf(buf.data(), buf.size(), ...);
    return {buf.data()};
}
```

### Why `std::array`?
- `.size()` — knows its own size (no separate variable needed)
- `.data()` — returns a raw pointer for C API compatibility (like `snprintf`)
- Value semantics — can be copied, passed by value, stored in containers
- `{}` initialization — zero-initializes all elements
- No heap allocation — lives entirely on the stack

### ⚠️ NOLINT Comments
You'll see `NOLINT` comments next to `snprintf` calls — these suppress clang-tidy warnings about C-style varargs. The SDK uses `snprintf` for JSON serialization because it's faster than `std::ostringstream` and doesn't allocate.

---

## 1.8 Structured Bindings

### What Is It?
Structured bindings (C++17) let you decompose a struct or pair into named variables in a single declaration.

### Where It's Used

📄 **File:** `README.md` (example code, line 164)

```cpp
// Iterating over a map of balances
for (const auto& [currency, balance] : balances) {
    std::cout << currency << ": " << balance.available << std::endl;
}
```

Without structured bindings (C++14):
```cpp
for (const auto& pair : balances) {
    std::cout << pair.first << ": " << pair.second.available << std::endl;
}
```

### 💡 Key Insight
Structured bindings work with `std::pair`, `std::tuple`, `std::array`, and any aggregate struct. They make map iteration dramatically more readable.

---

## 1.9 Modern Class Control (`= delete`, `= default`)

### Where It's Used

📄 **File:** `include/kraken/core/client.hpp` (lines 58–66)

```cpp
class KrakenClient {
public:
    KrakenClient();
    ~KrakenClient();

    // Non-copyable (deleted)
    KrakenClient(const KrakenClient&) = delete;
    KrakenClient& operator=(const KrakenClient&) = delete;

    // Movable (defaulted)
    KrakenClient(KrakenClient&&) noexcept;
    KrakenClient& operator=(KrakenClient&&) noexcept;
};
```

### The Rule of Five

In C++, if you define *any* of these five special member functions, you should define *all* of them:

| Function | `KrakenClient` | Why |
|----------|----------------|-----|
| Destructor | Defined | Must clean up `impl_` |
| Copy Constructor | `= delete` | Can't copy a WebSocket connection |
| Copy Assignment | `= delete` | Same reason |
| Move Constructor | Defaulted (`noexcept`) | Transfer ownership of `impl_` |
| Move Assignment | Defaulted (`noexcept`) | Same |

### Why `= delete`?
Copying a `KrakenClient` would mean two objects sharing the same WebSocket connection, threads, and callbacks — a recipe for data races. By deleting the copy operations, the compiler will **refuse to compile** any code that accidentally copies a client.

### Why `noexcept`?
Move operations are marked `noexcept` because:
1. STL containers (like `std::vector`) will only use move if it's `noexcept`
2. Moving a `unique_ptr` can never throw (it's just a pointer swap)

---

## 1.10 Move Semantics

### What Is It?
Move semantics allow **transferring** resources from one object to another instead of copying them. A moved-from object is left in a valid but unspecified state.

### Where It's Used

📄 **File:** `src/core/client.cpp` (lines 25–27)

```cpp
void KrakenClient::on_ticker(TickerCallback callback) {
    impl_->on_ticker(std::move(callback));  // Transfer callback ownership
}
```

### Why `std::move` Here?
A `TickerCallback` (which is a `std::function`) may internally hold a lambda with captured data. Copying it would duplicate that data. Moving it **transfers** the internal state without any allocation or copy.

### Move in the Message Queue

Messages are moved through the SPSC queue to avoid copying large data structures:

```cpp
// Producer (network thread)
Message msg = parse_message(raw_json);
queue.try_push(std::move(msg));  // Move into queue — no copy

// Consumer (dispatcher thread)
Message* front = queue.front();
dispatch(*front);                // Use the data
queue.pop();                     // Destroy the message
```

### 💡 Key Insight
After `std::move(obj)`, the object `obj` is in a **valid but unspecified state**. You can only assign to it or destroy it — don't read from it. This is a common source of bugs for beginners.

---

## Summary

| Feature | What It Replaces | Where Used |
|---------|------------------|------------|
| `std::variant` | Raw union / base class pointers | `MessageData` in client_impl |
| `std::optional` | Nullable pointers / sentinel values | `latest_ticker()`, `latest_book()` |
| `std::function` | Function pointers | All callbacks |
| `unique_ptr` | Raw `new`/`delete` | PIMPL, queue, backoff |
| `shared_ptr` | Manual reference counting | Strategies |
| `std::chrono` | Raw integer milliseconds | Backoff, timestamps |
| `enum class` | Plain enums | Channel, Side, ErrorCode |
| `std::array` | C-style arrays | JSON serialization buffers |
| Structured bindings | `pair.first` / `pair.second` | Map iteration |
| `= delete` / `= default` | Boilerplate constructors | Class declarations |
| Move semantics | Deep copies | Queue push, callback registration |

---

**Next:** [Chapter 2: Design Patterns →](02_DESIGN_PATTERNS.md)
