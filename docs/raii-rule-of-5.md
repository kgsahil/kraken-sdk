# RAII and Rule of 5 in the SDK

## Overview

The SDK follows RAII (Resource Acquisition Is Initialization) and Rule of 5 throughout. This document details the ownership and lifecycle of each resource-managing class.

---

## KrakenClient (PIMPL)

```cpp
class KrakenClient {
public:
    KrakenClient();
    explicit KrakenClient(ClientConfig config);
    ~KrakenClient();
    
    // Non-copyable (unique ownership of implementation)
    KrakenClient(const KrakenClient&) = delete;
    KrakenClient& operator=(const KrakenClient&) = delete;
    
    // Movable (transfer ownership)
    KrakenClient(KrakenClient&&) noexcept;
    KrakenClient& operator=(KrakenClient&&) noexcept;
    
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
```

**Destructor responsibilities:**
1. Call `stop()` to signal threads
2. Join `io_thread_`
3. Join `dispatcher_thread_` (if Queued mode)
4. Close WebSocket connection
5. `unique_ptr<Impl>` cleans up implementation

**Why movable:** Allows `auto client = create_client();` pattern.

---

## KrakenClient::Impl

```cpp
class KrakenClient::Impl {
public:
    explicit Impl(ClientConfig config);
    ~Impl();
    
    // Non-copyable, non-movable (owns threads)
    Impl(const Impl&) = delete;
    Impl& operator=(const Impl&) = delete;
    Impl(Impl&&) = delete;
    Impl& operator=(Impl&&) = delete;
    
private:
    std::unique_ptr<Connection> connection_;
    std::unique_ptr<rigtorp::SPSCQueue<Message>> queue_;
    std::thread io_thread_;
    std::thread dispatcher_thread_;
    std::atomic<bool> running_{false};
    // ...
};
```

**Why non-movable:** Threads and atomics reference member data. Moving would invalidate references.

---

## Subscription (Shared Ownership)

```cpp
class Subscription {
public:
    ~Subscription();
    
    void pause();
    void resume();
    void unsubscribe();
    
private:
    friend class KrakenClient;
    class Impl;
    std::shared_ptr<Impl> impl_;
    
    Subscription(std::shared_ptr<Impl> impl);
};
```

**Why shared_ptr:**
- User holds `Subscription` handle
- `KrakenClient::Impl` also holds reference for internal management
- Subscription survives as long as either exists

**Destructor:** If this is the last reference, unsubscribes automatically.

---

## Connection

```cpp
class Connection {
public:
    explicit Connection(boost::asio::io_context& ctx, const std::string& url);
    ~Connection();
    
    // Non-copyable, non-movable (socket ownership)
    Connection(const Connection&) = delete;
    Connection& operator=(const Connection&) = delete;
    Connection(Connection&&) = delete;
    Connection& operator=(Connection&&) = delete;
    
private:
    boost::beast::websocket::stream<
        boost::beast::ssl_stream<boost::asio::ip::tcp::socket>
    > ws_;
};
```

**Destructor:** Closes WebSocket gracefully, then TCP socket.

---

## OrderBook

```cpp
struct OrderBook {
    std::string symbol;
    std::vector<PriceLevel> bids;
    std::vector<PriceLevel> asks;
    bool is_valid;
    
    // Rule of 0 - all members are trivially copyable/movable
    // Compiler-generated special members are correct
};
```

**Why Rule of 0:** No custom resource management, just data.

---

## SPSCQueue (rigtorp)

We use `rigtorp::SPSCQueue` which is:
- Non-copyable
- Non-movable
- Contains atomics and aligned storage

Wrapped in `std::unique_ptr` inside `KrakenClient::Impl`.

---

## Summary

| Class | Copyable | Movable | Destructor | Why |
|-------|----------|---------|------------|-----|
| KrakenClient | ❌ | ✅ | Stops threads, closes connection | PIMPL with unique_ptr |
| KrakenClient::Impl | ❌ | ❌ | Joins threads | Owns threads |
| Subscription | ✅ (shared) | ✅ | Unsubscribes if last ref | Shared ownership |
| Connection | ❌ | ❌ | Closes socket | Socket ownership |
| OrderBook | ✅ | ✅ | Default | Data only (Rule of 0) |

---

## Exception Safety

### Strong Guarantee (Constructor)

```cpp
KrakenClient::KrakenClient(ClientConfig config) {
    if (config.url().empty()) {
        throw std::invalid_argument("URL cannot be empty");
    }
    // validate before allocating
    impl_ = std::make_unique<Impl>(std::move(config));
    // if Impl constructor throws, unique_ptr is never assigned
    // no cleanup needed
}
```

### Basic Guarantee (Operations)

```cpp
void KrakenClient::Impl::connect() {
    connection_ = std::make_unique<Connection>(io_ctx_, config_.url());
    // if connect() throws, connection_ is reset
    connection_->connect();
}
```

### No-Throw (Hot Path)

```cpp
void KrakenClient::Impl::dispatch(const Message& msg) noexcept {
    // No exceptions in hot path
    // Errors reported via callback
    std::shared_lock lock(callbacks_mutex_);
    if (ticker_callback_ && msg.type == MessageType::Ticker) {
        ticker_callback_(msg.as_ticker());
    }
}
```

---

## Thread Ownership

| Thread | Owner | Lifetime |
|--------|-------|----------|
| I/O thread | KrakenClient::Impl | Created in `run()`, joined in destructor |
| Dispatcher thread | KrakenClient::Impl | Created in `run()` (Queued mode), joined in destructor |

Both threads check `running_` atomic and exit cleanly when `stop()` is called.
