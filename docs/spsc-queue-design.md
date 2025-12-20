# SPSC Queue Design

## Overview

The SDK uses [rigtorp/SPSCQueue](https://github.com/rigtorp/SPSCQueue) - a battle-tested, header-only lock-free queue used in HFT systems.

**Why not custom implementation?**
- Lock-free code is hard to get right (memory ordering bugs are silent)
- rigtorp is proven in production HFT systems (2.5k GitHub stars)
- Saves development time for hackathon
- Header-only, MIT license, easy to integrate

---

## The Architecture (Reactor Pattern)

```
┌──────────────────┐                    ┌──────────────────┐
│   I/O THREAD     │                    │ DISPATCHER THREAD│
│   (PRODUCER)     │   rigtorp::SPSC    │ (CONSUMER)       │
│                  │                    │                  │
│  • Recv from WS  │                    │  • Pop from queue│
│  • Parse JSON    │  ─── try_push ───► │  • Route message │
│  • try_push      │                    │  • Call callback │
│                  │                    │                  │
└──────────────────┘                    └──────────────────┘
         │                                       │
         │                                       ▼
         │                              ┌────────────────────┐
         │                              │   USER CALLBACKS   │
         │                              │   (same thread)    │
         │                              └────────────────────┘
```

**Critical constraint: 1 producer, 1 consumer. Never violated.**

---

## Integration

```cpp
#include <rigtorp/SPSCQueue.h>

class KrakenClient {
private:
    // Queue created with configured capacity
    std::unique_ptr<rigtorp::SPSCQueue<Message>> queue_;
    
public:
    explicit KrakenClient(ClientConfig config) {
        if (config.delivery_mode == DeliveryMode::Queued) {
            queue_ = std::make_unique<rigtorp::SPSCQueue<Message>>(
                config.queue_capacity
            );
        }
    }
};
```

### CMake

```cmake
include(FetchContent)

FetchContent_Declare(
    SPSCQueue
    GIT_REPOSITORY https://github.com/rigtorp/SPSCQueue.git
    GIT_TAG v1.1
)
FetchContent_MakeAvailable(SPSCQueue)

target_link_libraries(kraken PRIVATE SPSCQueue)
```

---

## Overflow Handling

**The key decision: What happens when the queue is full?**

| Option | Effect | For Trading SDK |
|--------|--------|-----------------|
| Overwrite oldest | Lose old data | ❌ Miss confirmations |
| Drop newest | Lose new data | ⚠️ Acceptable with warning |
| Block producer | Stall I/O | ❌ Very bad |

**Our choice: Drop newest + error callback**

```cpp
// I/O thread (producer)
void on_message_received(Message msg) {
    if (!queue_->try_push(std::move(msg))) {
        // Queue full - don't block, drop with warning
        if (config_.on_error) {
            config_.on_error("Queue overflow - message dropped");
        }
        ++stats_.dropped_messages;
    }
}
```

**User mitigation:**
1. Increase `queue_capacity` (default 65536)
2. Speed up callbacks
3. Use Direct mode if callbacks are fast

---

## Wait Strategy

### Consumer (Dispatcher Thread)

```cpp
void dispatcher_loop() {
    while (running_) {
        Message* msg = queue_->front();
        if (msg) {
            dispatch(*msg);
            queue_->pop();
        } else {
            // Queue empty - brief pause
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
    }
}
```

### Why Not Condition Variable?

```cpp
// CV approach - DON'T DO THIS
std::unique_lock lock(mutex_);
cv_.wait(lock, [&] { return !queue_->empty() || !running_; });
```

Issues:
1. **Mutex overhead** - CV requires `std::mutex`, defeating lock-free benefits
2. **Syscall overhead** - `wait()` and `notify_one()` are syscalls
3. **Lost wakeup bug** - Race between `empty()` check and `wait()`

### Why Short Sleep Works

- **No locks** - Queue remains lock-free
- **Low CPU** - Sleep prevents busy-spin
- **Simple** - No complex synchronization
- **Acceptable latency** - 100μs max delay is fine (market data arrives at ~10-100ms)

### For True HFT (Optional)

```cpp
void dispatcher_loop_hft() {
    while (running_) {
        Message* msg;
        while (!(msg = queue_->front())) {
            _mm_pause();  // CPU hint, prevents pipeline stall
        }
        dispatch(*msg);
        queue_->pop();
    }
}
```

This burns CPU but achieves minimum latency.

---

## Why SPSC Works Here

### The Trap (What NOT to Do)

> "Use SPSC queue for 1000 ticker subscriptions with 1000 consumer threads"

This is **wrong**. SPSC = **S**ingle **P**roducer **S**ingle **C**onsumer.

Multiple consumers = race conditions, crashes.

### The Solution (What We Do)

| Subscriptions | Threads | Pattern |
|---------------|---------|---------|
| 1000 tickers | 2 total | Reactor |

How:
1. **I/O thread** pushes ALL messages to ONE queue
2. **Dispatcher thread** pops and routes by symbol
3. **Callback runs** on Dispatcher thread (no new thread)

---

## Direct Mode (No Queue)

Direct mode bypasses the queue entirely:

```
┌──────────────────┐
│   I/O THREAD     │
│                  │
│  • Recv from WS  │
│  • Parse JSON    │
│  • Call callback │  ← Directly, no queue
│                  │
└──────────────────┘
```

Use Direct mode when callbacks are fast (< 1ms).

---

## Summary

| Decision | Choice | Why |
|----------|--------|-----|
| Queue library | rigtorp/SPSCQueue | Battle-tested, saves time |
| Overflow | Drop + callback | Never block I/O |
| Wait strategy | Sleep 100μs | Simple, low CPU |
| Thread count | 2 | I/O + Dispatcher |
| Routing | Hash map | O(1) symbol lookup |
