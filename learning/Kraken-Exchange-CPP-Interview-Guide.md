# KRAKEN EXCHANGE - C++ SOFTWARE ENGINEER INTERVIEW PREPARATION GUIDE

**Position:** Senior Software Engineer - Exchange Platform Team  
**Company:** Kraken Digital Asset Exchange  
**Interview Format:** Single Fast-Tracked Technical Round (60-90 minutes)  
**Your Context:** Winner of Kraken Forge Hackathon - SDK Client Track (C++)  
**Preparation Time:** 2 Days  

---

## TABLE OF CONTENTS

1. [Executive Summary](#executive-summary)
2. [Interview Structure & Timeline](#interview-structure)
3. [Day 1 Preparation Plan](#day-1-plan)
4. [Day 2 Preparation Plan](#day-2-plan)
5. [Complete Question Bank](#question-bank)
   - C++ Technical Deep-Dive
   - Coding & Algorithm Questions
   - Exchange System Design
   - Your Project Deep-Dive Questions
   - Code Review & Debug Scenarios
   - Production Incident Scenarios
   - Crypto & Culture Questions
6. [Key Concepts Reference](#key-concepts)
7. [Mock Interview Simulation](#mock-interview)
8. [Final Checklist](#final-checklist)

---

<a name="executive-summary"></a>
## 1. EXECUTIVE SUMMARY

### What Kraken Exchange Platform Team Does

The Exchange team at Kraken owns critical trading infrastructure:

- **Matching Engine** - Core order matching logic with price-time priority
- **Market Data Gateways** - Real-time L2/L3 order book feeds and trade data
- **Internal & External APIs** - REST, WebSocket, FIX protocol interfaces
- **Margin & Risk Systems** - Pre-trade and post-trade risk management
- **Trading Services** - Order ingestion, validation, routing, execution

### Technical Stack

- **Primary Language:** Modern C++ (C++17/20) on Linux
- **Supporting Languages:** Go, Rust, Python in parts of the stack
- **Performance Requirements:**
  - Sub-millisecond internal latency
  - 600,000+ operations per second capability
  - 24/7 operation with 99.99%+ uptime
  - Institutional-grade throughput (30,000+ matches per second per instrument)

### What They're Looking For

1. **Modern C++ mastery** - RAII, move semantics, concurrency, memory management
2. **Low-latency systems experience** - Profiling, optimization, cache-awareness
3. **Trading systems knowledge** - Order books, matching engines, FIX protocol
4. **Distributed systems understanding** - Consistency, fault tolerance, scalability
5. **Crypto alignment** - Mission-driven, security-conscious, DeFi awareness
6. **Production mindset** - Debugging, incident response, telemetry

### Your Competitive Advantages

✅ **Kraken Forge C++ SDK Winner** - Demonstrated capability with their API  
✅ **UBS Trading Background** - Real FIX protocol and repo trading experience  
✅ **Order Book & Matching Engine Projects** - Relevant portfolio work  
✅ **Fast-Track from Hackathon** - Already vetted by their team  

---

<a name="interview-structure"></a>
## 2. INTERVIEW STRUCTURE & TIMELINE

### Expected 60-90 Minute Breakdown

| Time | Section | Focus |
|------|---------|-------|
| **0-5 min** | Intro & Warm-up | Brief background, hackathon experience |
| **5-25 min** | Project Deep-Dive | Your Kraken SDK + order book/matching engine work |
| **25-55 min** | Technical Assessment | C++ coding/optimization + concurrency scenarios |
| **55-75 min** | System Design | Exchange component design (matching engine or market data) |
| **75-85 min** | Culture & Crypto | Mission alignment, remote work, crypto interest |
| **85-90 min** | Your Questions | Technical team questions prepared |

### Interview Styles to Expect

**Collaborative, Not Adversarial**  
Kraken culture emphasizes:
- Think-aloud problem-solving
- Trade-off discussions over "perfect" answers
- Real-world production constraints
- Security and reliability focus

**Domain-Specific, Not Generic Puzzles**  
Questions will be anchored in:
- Trading systems context
- Performance-critical scenarios
- Real production failure modes
- Crypto exchange challenges

---

<a name="day-1-plan"></a>
## 3. DAY 1 PREPARATION PLAN (8 Hours)

### Block 1: Modern C++ Fundamentals (2.5 hours)

**Focus Areas:**

#### A. Ownership & Lifetime Management (45 min)
- Rule of 3/5/0 and when to use each
- unique_ptr vs shared_ptr vs weak_ptr trade-offs
- RAII patterns for network/file resources
- Move semantics and copy elision
- Exception safety guarantees (basic/strong/no-throw)

**Practice Questions:**
1. When would you use a custom deleter with unique_ptr in an exchange system?
2. Explain a scenario where shared_ptr creates a performance bottleneck
3. Design RAII wrappers for a FIX session connection

#### B. Concurrency & Threading (45 min)
- std::thread, mutex, condition_variable, atomic
- Memory ordering (relaxed, acquire, release, seq_cst)
- Common race conditions and deadlock patterns
- Lock-free vs lock-based approaches
- False sharing and cache-line alignment

**Practice Questions:**
1. Implement a thread-safe order counter without excessive contention
2. Identify race conditions in multi-threaded order book updates
3. Compare spinlock vs mutex for matching engine hot paths

#### C. Performance & Optimization (45 min)
- Where allocations happen in C++ (implicit and explicit)
- Cache-friendly data layout principles
- Virtual dispatch cost and devirtualization
- Profiling workflow (perf, valgrind, heaptrack)
- Common hot-path anti-patterns

**Practice Questions:**
1. Why is std::vector growth problematic in latency-sensitive code?
2. Optimize a hot function that copies orders unnecessarily
3. Design a zero-allocation order insertion path

**Action Items:**
- [ ] Write out rule of 5 from memory
- [ ] Implement thread-safe counter (atomic vs mutex)
- [ ] List 5 sources of allocations in typical C++ code
- [ ] Practice explaining cache locality with an order book example

---

### Block 2: Your Project Deep-Dive Preparation (3 hours)

**Goal:** Deliver a crisp, technical 10-15 minute walkthrough of your Kraken Forge SDK with architectural clarity.

#### Structured Presentation Outline

**1. Problem Statement (1 min)**
- What the SDK enables for Kraken API users
- Target use case (algo traders, market makers, portfolio management)

**2. High-Level Architecture (3 min)**
Draw this on paper/whiteboard:
```
┌─────────────────────────────────────────┐
│         User Application Layer          │
│   (callbacks: on_ticker, on_trade,      │
│    on_book, on_error, on_alert)         │
└──────────────┬──────────────────────────┘
               │ registers callbacks via
┌──────────────▼──────────────────────────┐
│       KrakenClient (PIMPL → Impl)       │
│   Public API: subscribe(), run(),       │
│   add_alert(), get_metrics()            │
├─────────────────────────────────────────┤
│   Strategy Engine (evaluate alerts)     │
│   Book Engine (maintain local books)    │
│   Sequence Tracker (gap detection)      │
└──────────────┬──────────────────────────┘
               │
┌──────────────▼──────────────────────────┐
│   Dispatcher Thread ◄── SPSC Queue ──┐  │
│   (invokes callbacks on user thread) │  │
│                                      │  │
│   I/O Thread (Boost.Asio event loop) ┘  │
│   (WebSocket read → JSON parse → push)  │
├─────────────────────────────────────────┤
│   Resilience Layer                      │
│   Circuit Breaker · Exponential Backoff │
│   Token Bucket Rate Limiter             │
├─────────────────────────────────────────┤
│   Transport: Boost.Beast WebSocket      │
│   TLS/SSL (OpenSSL) · HMAC-SHA512 Auth  │
│   JSON: RapidJSON                       │
├─────────────────────────────────────────┤
│   Observability                         │
│   spdlog · OpenTelemetry · Prometheus   │
└─────────────────────────────────────────┘
```

Explain:
- Why you chose these layers
- What each layer owns
- How errors propagate (setup: exceptions, runtime: error callbacks)

> 📘 See [System Design chapter](04_SYSTEM_DESIGN.md) for the full event-driven architecture, data flow, and state machine diagrams.

**3. Concurrency Model (2 min)**
- **Two-thread reactor**: I/O thread (Boost.Asio event loop, reads WebSocket, parses JSON) and Dispatcher thread (invokes user callbacks + strategy evaluation)
- **Lock-free SPSC queue** (`rigtorp::SPSCQueue`) for inter-thread communication — ~12ns push/pop
- **Direct mode** option: bypass queue, dispatch callbacks on I/O thread for ultra-low latency
- Lock contention minimized: only `std::mutex` on callback registration (rare), `std::atomic` for metrics counters

> 📘 See [Concurrency chapter](03_CONCURRENCY.md) for the SPSC queue internals, atomic operations, and thread-safety analysis.

**4. Key Technical Decisions (3 min)**
- **PIMPL Pattern:** Clean public API (`client.hpp` has zero heavy includes), ABI stability, compile firewall — [see Design Patterns](02_DESIGN_PATTERNS.md#21-pimpl-pointer-to-implementation)
- **Error Handling:** Dual approach — exceptions during setup (connect/construct fails → halt), callbacks at runtime (never crash) — [see System Design](04_SYSTEM_DESIGN.md#48-error-handling-strategy)
- **Strategy Engine:** Observer + Strategy + Composite patterns — users add alert strategies that auto-evaluate on every tick — [see Design Patterns](02_DESIGN_PATTERNS.md#23-strategy-pattern)
- **Builder Pattern:** `ClientConfig::Builder` with 20+ parameters, fluent API, composable (backoff builder, telemetry builder nested) — [see Design Patterns](02_DESIGN_PATTERNS.md#22-builder-pattern)

**5. Performance Characteristics (2 min)**
- JSON parse: ~1.8 μs per message (RapidJSON)
- SPSC queue push/pop: ~12 ns
- Strategy evaluation: ~100 ns per strategy
- Total SDK overhead: < 5 μs end-to-end
- Queue capacity: 131,072 messages (configurable)

> 📘 See [System Design chapter](04_SYSTEM_DESIGN.md#42-end-to-end-data-flow) for the latency breakdown table.

**6. Testing & Validation (1 min)**
- ~250 unit tests (Google Test) — parser, book engine, config, strategies
- ~50 integration tests — end-to-end message flow
- ~40 stress tests — queue overflow, rapid reconnection, concurrent callbacks, malformed JSON
- Google Benchmark — continuous performance regression testing
- Clang-Tidy static analysis + CI/CD pipeline (GitHub Actions)

> 📘 See [Testing chapter](08_TESTING.md) for the full testing pyramid, stress test categories, and CI/CD pipeline.

#### Anticipated Challenge Questions

Prepare answers for:

1. **"If users complain of intermittent timeouts, how would you debug it?"**
   - Check structured logs (spdlog): connection state transitions, reconnect attempts
   - Check metrics via `get_metrics()`: messages received, queue depth, dropped messages
   - TLS handshake issues? Check OpenSSL cert verification
   - Circuit breaker tripping? Check failure threshold settings
   - Use OpenTelemetry traces if OTLP exporter is enabled

2. **"How would you scale this SDK to 10x message throughput?"**
   - Increase SPSC queue capacity (default 131,072, can go higher)
   - Switch to direct mode (bypass queue) for single-threaded use
   - Pre-size RapidJSON parse buffers
   - Batch subscription requests
   - Profile with `perf` under load to find actual bottlenecks

3. **"What trade-offs did you make between simplicity and performance?"**
   - Queue mode (default) adds ~12ns latency but protects user callbacks from blocking I/O
   - PIMPL adds one pointer indirection but gives ABI stability and clean API
   - `std::function` callbacks (not raw function pointers) for usability, ~small overhead
   - `std::variant` for message types — type-safe but slightly more complex dispatch

4. **"A trading firm wants to integrate your SDK into their production stack. What would you change first?"**
   - Already has: structured logging (spdlog), OpenTelemetry, Prometheus metrics endpoint, circuit breaker, rate limiter, config via env vars
   - Would add: custom allocator for message objects, FIX protocol gateway adapter, historical data replay

**Action Items:**
- [ ] Draw architecture diagram on paper 5 times until you can do it in 90 seconds
- [ ] Record yourself explaining the SDK in 10 minutes, listen back for clarity
- [ ] Write bullet-point answers to all 4 challenge questions
- [ ] Identify 3 specific performance optimizations with before/after metrics

---

### Block 3: Exchange System Design Fundamentals (2.5 hours)

**Goal:** Be able to design one exchange component end-to-end in 15-20 minutes.

#### Core System: Spot Exchange Matching Engine

**Functional Requirements:**
- Accept limit orders and market orders
- Match using price-time priority (FIFO within price level)
- Support cancel and modify operations
- Generate trade events and maintain balances
- Persist order book state for recovery

**Non-Functional Requirements:**
- Sub-millisecond matching latency (P99)
- 100,000+ orders per second throughput
- 99.99% uptime
- Crash recovery with no lost trades

**Component Design:**

```
┌─────────────────────────────────────────────────┐
│              API Gateway Layer                  │
│  (REST/WebSocket/FIX ingress, auth, rate limit) │
└──────────────┬──────────────────────────────────┘
               │
┌──────────────▼──────────────────────────────────┐
│           Order Validation                      │
│  (Balance checks, duplicate detection)          │
└──────────────┬──────────────────────────────────┘
               │
┌──────────────▼──────────────────────────────────┐
│         Matching Engine Core                    │
│  (Per-symbol single-threaded, lock-free)        │
│                                                  │
│  Data Structures:                               │
│  - BID side: std::map<price, std::list<Order>>  │
│    (descending price order)                     │
│  - ASK side: std::map<price, std::list<Order>>  │
│    (ascending price order)                      │
│  - Order lookup: std::unordered_map<OrderID>    │
└──────────────┬──────────────────────────────────┘
               │
┌──────────────▼──────────────────────────────────┐
│         Trade Execution Logger                  │
│  (Write-ahead log for durability)               │
└──────────────┬──────────────────────────────────┘
               │
┌──────────────▼──────────────────────────────────┐
│        Market Data Publisher                    │
│  (WebSocket fans, FIX gateways, L2/L3 feeds)    │
└─────────────────────────────────────────────────┘
```

**Key Design Decisions:**

1. **Single-threaded per symbol** vs multi-threaded global book
   - ✅ Eliminates lock contention, predictable latency
   - ❌ Doesn't scale horizontally without sharding
   - **Choice:** Single-threaded for Kraken-scale volume

2. **In-memory state + persistent log** vs fully persistent
   - ✅ Fast matching in RAM, WAL for durability
   - ❌ Recovery time after crash (replay log)
   - **Choice:** Snapshot every N seconds + incremental log

3. **Data structure for order book:**
   - `std::map<price, std::list<Order>>` for price levels
   - Why map? O(log n) insert/delete, sorted iteration
   - Why list? O(1) FIFO append/remove within price level
   - Alternative: Skip list or custom tree for better cache locality

4. **Backpressure handling:**
   - Bounded queues with drop-tail or reject when full
   - Client-specific rate limits to prevent one user DoS-ing system

**Failure Modes & Mitigations:**

| Failure Mode | Detection | Mitigation |
|--------------|-----------|------------|
| Process crash | Watchdog, heartbeat | Restart + replay WAL from last snapshot |
| Corrupted state | Checksums, invariants | Reject order, alert, manual intervention |
| Slow client blocking | Queue depth metrics | Drop slow clients, separate queues |
| Hot symbol overload | Per-symbol latency monitoring | Dynamic throttling, load shedding |

**Practice Exercise:**
- Draw this system in 10 minutes
- Explain matching algorithm in 5 minutes
- Discuss 3 failure modes and fixes in 5 minutes
- Total: 20 minutes end-to-end

**Action Items:**
- [ ] Draw matching engine architecture 3 times on paper
- [ ] Implement toy order book in C++ (100 lines, bids/asks maps)
- [ ] Write out price-time priority matching algorithm in pseudocode
- [ ] List 5 failure modes with detection + mitigation strategies

---

<a name="day-2-plan"></a>
## 4. DAY 2 PREPARATION PLAN (8 Hours)

### Block 1: Coding & Optimization Drills (3 hours)

**Goal:** Build fluency in domain-relevant coding problems under time pressure.

#### Problem 1: Token Bucket Rate Limiter (25 min)

**Problem Statement:**
Implement a token bucket rate limiter that allows at most `N` operations per second with burst capacity `B`.

```cpp
class RateLimiter {
public:
    RateLimiter(int rate_per_sec, int burst_capacity);
    
    // Returns true if operation is allowed
    bool allow();
    
private:
    // Your implementation
};
```

**Solution Outline:**
```cpp
class RateLimiter {
public:
    RateLimiter(int rate, int burst)
        : rate_per_sec_(rate)
        , capacity_(burst)
        , tokens_(burst)
        , last_refill_(std::chrono::steady_clock::now())
    {}
    
    bool allow() {
        refill();
        if (tokens_ >= 1.0) {
            tokens_ -= 1.0;
            return true;
        }
        return false;
    }
    
private:
    void refill() {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration<double>(now - last_refill_).count();
        tokens_ = std::min(capacity_, tokens_ + elapsed * rate_per_sec_);
        last_refill_ = now;
    }
    
    double rate_per_sec_;
    double capacity_;
    double tokens_;
    std::chrono::steady_clock::time_point last_refill_;
};
```

**Follow-up Questions:**
1. How would you make this thread-safe? (std::mutex around allow() — exactly how the SDK's `RateLimiter` does it: [see Resilience chapter](06_RESILIENCE.md#62-token-bucket-rate-limiter))
2. How to avoid lock contention with high concurrency? (Per-thread limiters, sharding by user ID)
3. What if time goes backward? (Use steady_clock, add monotonicity check)
4. How does your SDK implement this? (Thread-safe `RateLimiter` class with `acquire()`, `acquire_blocking()`, `wait_time()`, and stats tracking — [see rate_limiter.hpp](../include/kraken/rate_limiter.hpp))

---

#### Problem 2: Streaming VWAP Calculator (30 min)

**Problem Statement:**
Maintain Volume-Weighted Average Price (VWAP) for the last 60 seconds of trades.

```cpp
class VWAPCalculator {
public:
    VWAPCalculator(int window_seconds);
    
    // Add new trade
    void add_trade(double price, double quantity, uint64_t timestamp_ms);
    
    // Get current VWAP
    double get_vwap() const;
    
private:
    // Your implementation
};
```

**Solution Outline:**
```cpp
class VWAPCalculator {
public:
    VWAPCalculator(int window_sec)
        : window_ms_(window_sec * 1000)
    {}
    
    void add_trade(double price, double qty, uint64_t ts) {
        trades_.push_back({price, qty, ts});
        total_value_ += price * qty;
        total_volume_ += qty;
        
        // Evict old trades
        uint64_t cutoff = ts - window_ms_;
        while (!trades_.empty() && trades_.front().timestamp < cutoff) {
            total_value_ -= trades_.front().price * trades_.front().quantity;
            total_volume_ -= trades_.front().quantity;
            trades_.pop_front();
        }
    }
    
    double get_vwap() const {
        return total_volume_ > 0 ? total_value_ / total_volume_ : 0.0;
    }
    
private:
    struct Trade {
        double price;
        double quantity;
        uint64_t timestamp;
    };
    
    int window_ms_;
    std::deque<Trade> trades_;
    double total_value_ = 0.0;
    double total_volume_ = 0.0;
};
```

**Follow-up Questions:**
1. How to optimize for very high trade rates? (Ring buffer, pre-allocate)
2. What if trades arrive out-of-order? (Insert sorted, use std::multiset)
3. Memory usage concern for long windows? (Downsample older data)

---

#### Problem 3: Order Book Best Bid/Ask (35 min)

**Problem Statement:**
Implement a simplified order book that supports:
- `add_order(order_id, side, price, quantity)`
- `cancel_order(order_id)`
- `get_best_bid()` - O(1)
- `get_best_ask()` - O(1)

**Solution Outline:**
```cpp
enum class Side { BUY, SELL };

struct Order {
    uint64_t id;
    Side side;
    double price;
    double quantity;
};

class OrderBook {
public:
    void add_order(uint64_t id, Side side, double price, double qty) {
        Order order{id, side, price, qty};
        orders_[id] = order;
        
        if (side == Side::BUY) {
            bids_[price].push_back(order);
        } else {
            asks_[price].push_back(order);
        }
    }
    
    void cancel_order(uint64_t id) {
        auto it = orders_.find(id);
        if (it == orders_.end()) return;
        
        Order& order = it->second;
        auto& levels = (order.side == Side::BUY) ? bids_ : asks_;
        auto& level_orders = levels[order.price];
        
        level_orders.erase(
            std::remove_if(level_orders.begin(), level_orders.end(),
                [id](const Order& o) { return o.id == id; }),
            level_orders.end()
        );
        
        if (level_orders.empty()) {
            levels.erase(order.price);
        }
        
        orders_.erase(id);
    }
    
    std::optional<double> get_best_bid() const {
        if (bids_.empty()) return std::nullopt;
        return bids_.rbegin()->first;  // Highest price
    }
    
    std::optional<double> get_best_ask() const {
        if (asks_.empty()) return std::nullopt;
        return asks_.begin()->first;  // Lowest price
    }
    
private:
    // Price -> Orders at that price (FIFO within level)
    std::map<double, std::vector<Order>, std::greater<>> bids_;  // Descending
    std::map<double, std::vector<Order>> asks_;                  // Ascending
    std::unordered_map<uint64_t, Order> orders_;                 // Fast lookup
};
```

**Follow-up Questions:**
1. How to implement matching? (Check cross: best_bid >= best_ask, execute trades)
2. Performance bottleneck? (std::vector for price level could be list for O(1) remove)
3. How to add modify operation? (Cancel + Add, or update in place)

---

#### Problem 4: Hot Path Optimization Challenge (30 min)

**Given Code:**
```cpp
double calculate_total_value(const std::vector<Order>& orders) {
    double total = 0.0;
    for (int i = 0; i < orders.size(); ++i) {
        total += orders[i].price * orders[i].quantity;
    }
    return total;
}
```

**Optimization Questions:**
1. What's the complexity? (O(n) - unavoidable)
2. What are performance issues? (Implicit copies, bounds checking)
3. Rewrite for better performance:

```cpp
double calculate_total_value(const std::vector<Order>& orders) {
    double total = 0.0;
    const size_t n = orders.size();
    for (size_t i = 0; i < n; ++i) {
        const auto& order = orders[i];  // Avoid copy
        total += order.price * order.quantity;
    }
    return total;
}

// Or with iterators (compiler can optimize better):
double calculate_total_value(const std::vector<Order>& orders) {
    return std::accumulate(orders.begin(), orders.end(), 0.0,
        [](double sum, const Order& o) {
            return sum + o.price * o.quantity;
        });
}
```

4. How to parallelize if orders vector is huge? (std::reduce with execution policy)
5. What if Order has virtual methods? (Cache misses, indirect calls - avoid in hot path)

---

**Action Items for Block 1:**
- [ ] Implement all 4 problems in C++ without looking at solutions
- [ ] Run and test each implementation
- [ ] Practice explaining your approach out loud in 5 minutes per problem
- [ ] Prepare answers to all follow-up questions

---

### Block 2: Production Scenarios & Debugging (2.5 hours)

**Goal:** Demonstrate production engineering judgment and incident response skills.

#### Scenario 1: Latency Spike in Matching Engine

**Situation:**
Monday 10:00 AM UTC, BTC-USD matching engine p99 latency spikes from 0.3ms to 45ms. Other symbols unaffected.

**Your Investigation Process:**

**Step 1: Confirm & Scope (2 min)**
- Check monitoring dashboards: is it sustained or transient?
- Verify single symbol isolation (BTC-USD only)
- Check if other metrics correlated (throughput, CPU, memory)

**Step 2: Immediate Data Gathering (3 min)**
- Per-symbol order rate: BTC-USD seeing 10x normal volume?
- Order book depth: has one side grown abnormally large?
- Market data fan-out queue depth: backlog in publishers?
- Profiling: where is CPU time going? (perf record -p <pid>)

**Step 3: Root Cause Hypotheses (5 min)**

| Hypothesis | How to Check | Likely Fix |
|------------|--------------|------------|
| Hot price level (too many orders at one price) | Count orders per price level | Spread orders across levels, or optimize in-level search |
| Memory allocations in hot path | valgrind/massif heap profile | Pre-allocate, use object pools |
| Lock contention (if multi-threaded) | perf lock contention analysis | Reduce lock scope, go lock-free |
| Slow client blocking system | Check websocket/FIX queue depths per client | Disconnect slow clients, separate queue |
| GC or system pauses | Check dmesg, kernel logs | Tune memory, check NUMA, disable swap |

**Step 4: Mitigation & Fix (10 min)**
- **Short-term:** Rate-limit new orders to BTC-USD, shed load if necessary
- **Medium-term:** Optimize hot code path identified in profiling
- **Long-term:** Architectural change (e.g., shard by price range, batch processing)

**Communication:**
- Alert #oncall and #trading-ops channels immediately
- Post incident report with timeline, root cause, and fix plan

---

#### Scenario 2: Balance Discrepancy Between Ledger and Engine

**Situation:**
User reports their account shows $10,000 in the web UI but only $8,000 in available balance for trading.

**Investigation:**

**Step 1: Identify System of Record**
- Ledger database (PostgreSQL): persistent, source of truth
- Matching engine in-memory state: fast, but derived
- Which is correct? Check transaction logs in ledger

**Step 2: Trace Transactions**
- Pull ledger history for user account: deposits, withdrawals, trades
- Compare with matching engine order history for same user
- Look for missing writes or duplicate entries

**Step 3: Common Root Causes**

| Cause | Symptom | Detection |
|-------|---------|-----------|
| Non-idempotent retries | Duplicate credit/debit | Check for duplicate transaction IDs |
| Race condition in balance update | Occasionally incorrect balance | Happens under high concurrency |
| Failed DB write | Engine has trade, DB doesn't | Check write error logs |
| Snapshot vs real-time mismatch | UI shows old snapshot | Check snapshot timestamp |

**Step 4: Fix Process**
1. Freeze user account temporarily
2. Reconcile manually: compare ledger and engine state line-by-line
3. Correct discrepancy: manual adjustment with audit trail
4. Root cause fix:
   - Add idempotency keys to all balance operations
   - Implement two-phase commit for critical transactions
   - Add continuous reconciliation job (every 5 minutes)

---

#### Scenario 3: Missing Fill Reports (Client Complaint)

**Situation:**
Market maker complains they sent a limit order that should have filled based on market data, but they never received an execution report.

**Investigation:**

**Step 1: Gather Order Details**
- Order ID (ClOrdID from client)
- Symbol, side, price, quantity
- Timestamp of order submission
- Client's view of market at submission time

**Step 2: Trace Through System**
```
Client → API Gateway → Order Validation → Matching Engine → Trade Logger → Execution Report Publisher → Client
```

Check logs at each stage:
- Did API gateway receive the order? (access logs)
- Did it pass validation? (validation logs)
- Did it reach matching engine? (engine order logs)
- Was a trade executed? (trade logs with match ID)
- Was execution report published? (FIX outbound logs, WebSocket outbound logs)

**Step 3: Possible Root Causes**

1. **Order rejected silently**
   - Insufficient balance at validation stage
   - Price out of collar limits
   - Fix: Ensure rejection messages always sent

2. **Execution report dropped**
   - Network issue between engine and client
   - Message queue overflow
   - Fix: Add retry logic, persistent queue

3. **Timing issue (order arrived after market moved)**
   - Client saw stale market data
   - By the time order arrived, price changed
   - Fix: Timestamp analysis, improve market data latency

4. **Engine bug (actually didn't match when it should have)**
   - Incorrect matching logic
   - Fix: Replay orders with test harness, fix bug

**Resolution:**
- Provide detailed timeline to client with logs
- If engine error: credit client, fix bug, deploy patch
- If client error: explain timing/market data delay, suggest improvements

---

**Action Items for Block 2:**
- [ ] Write out investigation steps for all 3 scenarios
- [ ] Practice explaining your approach out loud (5 min per scenario)
- [ ] Identify 2 additional scenarios from your UBS experience
- [ ] Prepare specific questions to ask Kraken about their incident response process

---

### Block 3: Code Review & Bug Hunt (1.5 hours)

**Goal:** Identify bugs, security issues, and performance problems in C++ snippets quickly.

#### Snippet 1: Dangling Reference

```cpp
const std::string& get_symbol(const Order& order) {
    std::string sym = order.base + "/" + order.quote;
    return sym;  // BUG: returning reference to local variable
}
```

**Problems:**
- Undefined behavior: `sym` destroyed at function exit
- Caller gets dangling reference

**Fixes:**
```cpp
// Option 1: Return by value (modern C++ optimizes with RVO/NRVO)
std::string get_symbol(const Order& order) {
    return order.base + "/" + order.quote;
}

// Option 2: Output parameter
void get_symbol(const Order& order, std::string& out) {
    out = order.base + "/" + order.quote;
}

// Option 3: String view if symbol already exists
std::string_view get_symbol(const Order& order) const {
    return order.cached_symbol;  // Member variable
}
```

---

#### Snippet 2: Race Condition

```cpp
class Metrics {
public:
    void record_order() {
        ++total_orders_;  // BUG: non-atomic increment
    }
    
    int get_total() const {
        return total_orders_;  // BUG: non-atomic read
    }
    
private:
    int total_orders_ = 0;
};
```

**Problems:**
- Data race: multiple threads can call `record_order()` concurrently
- Non-atomic read-modify-write on `total_orders_`
- Undefined behavior under C++ memory model

**Fixes:**
```cpp
// Option 1: Atomic
class Metrics {
public:
    void record_order() {
        total_orders_.fetch_add(1, std::memory_order_relaxed);
    }
    
    int get_total() const {
        return total_orders_.load(std::memory_order_relaxed);
    }
    
private:
    std::atomic<int> total_orders_{0};
};

// Option 2: Mutex (if also need to update multiple metrics atomically)
class Metrics {
public:
    void record_order() {
        std::lock_guard<std::mutex> lock(mutex_);
        ++total_orders_;
    }
    
    int get_total() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return total_orders_;
    }
    
private:
    mutable std::mutex mutex_;
    int total_orders_ = 0;
};

// Option 3: Thread-local (if aggregation acceptable)
thread_local int tl_total_orders = 0;

void record_order() {
    ++tl_total_orders;  // No contention
}
```

---

#### Snippet 3: Exception Safety Violation

```cpp
class OrderBook {
public:
    void add_order(const Order& order) {
        orders_.push_back(order);
        index_[order.id] = &orders_.back();
        notify_subscribers(order);  // May throw
    }
    
private:
    std::vector<Order> orders_;
    std::unordered_map<OrderID, Order*> index_;
};
```

**Problems:**
- If `notify_subscribers()` throws:
  - Order is in `orders_` vector
  - Pointer is in `index_`
  - But subscribers weren't notified
  - Inconsistent state: partially complete operation
- Pointer invalidation risk: `orders_.push_back()` may reallocate, invalidating all pointers

**Fixes:**
```cpp
// Option 1: Strong exception safety with rollback
class OrderBook {
public:
    void add_order(const Order& order) {
        size_t old_size = orders_.size();
        
        try {
            orders_.push_back(order);
            index_[order.id] = &orders_.back();
            notify_subscribers(order);  // May throw
        } catch (...) {
            // Rollback
            if (orders_.size() > old_size) {
                orders_.pop_back();
            }
            index_.erase(order.id);
            throw;
        }
    }
    
private:
    std::vector<Order> orders_;
    std::unordered_map<OrderID, Order*> index_;
};

// Option 2: Use stable storage (std::list or std::deque)
class OrderBook {
public:
    void add_order(const Order& order) {
        orders_.push_back(order);
        index_[order.id] = &orders_.back();  // Stable pointer with deque
        notify_subscribers(order);  // Still may throw, but state consistent
    }
    
private:
    std::deque<Order> orders_;  // Stable pointers
    std::unordered_map<OrderID, Order*> index_;
};

// Option 3: Notify first, commit second (if notifications are idempotent)
class OrderBook {
public:
    void add_order(const Order& order) {
        notify_subscribers(order);  // May throw, but state unchanged
        orders_.push_back(order);   // Commit only if notification succeeded
        index_[order.id] = &orders_.back();
    }
    
private:
    std::deque<Order> orders_;
    std::unordered_map<OrderID, Order*> index_;
};
```

---

#### Snippet 4: Memory Leak with shared_ptr

```cpp
struct Node {
    std::shared_ptr<Node> next;
    std::shared_ptr<Node> prev;  // BUG: Circular reference
    Order data;
};

void create_list() {
    auto node1 = std::make_shared<Node>();
    auto node2 = std::make_shared<Node>();
    
    node1->next = node2;
    node2->prev = node1;  // Cycle: node1 -> node2 -> node1
    
    // Memory leak: reference count never reaches zero
}
```

**Problems:**
- Circular shared_ptr creates reference cycle
- Neither object ever destroyed (reference count always > 0)

**Fixes:**
```cpp
// Option 1: Use weak_ptr for back-pointer
struct Node {
    std::shared_ptr<Node> next;
    std::weak_ptr<Node> prev;  // Breaks cycle
    Order data;
};

// Option 2: Use raw pointer for back-pointer (if lifetime is clear)
struct Node {
    std::shared_ptr<Node> next;
    Node* prev;  // Non-owning
    Order data;
};

// Option 3: Use unique_ptr if ownership is unidirectional
struct Node {
    std::unique_ptr<Node> next;  // Owns next node
    Node* prev;                  // Non-owning back-pointer
    Order data;
};
```

---

#### Snippet 5: Performance Issue - Unnecessary Copies

```cpp
std::vector<Order> filter_large_orders(const std::vector<Order>& orders) {
    std::vector<Order> result;
    for (auto order : orders) {  // BUG: copies each Order
        if (order.quantity > 1000) {
            result.push_back(order);  // Another copy
        }
    }
    return result;  // RVO should eliminate this copy
}
```

**Problems:**
- Loop copies each `Order` object unnecessarily
- `push_back` may cause reallocations if capacity not reserved

**Fixes:**
```cpp
// Option 1: Use const reference in loop, reserve capacity
std::vector<Order> filter_large_orders(const std::vector<Order>& orders) {
    std::vector<Order> result;
    result.reserve(orders.size());  // Avoid reallocations
    
    for (const auto& order : orders) {  // No copy
        if (order.quantity > 1000) {
            result.push_back(order);
        }
    }
    return result;
}

// Option 2: If acceptable, return indices instead of copies
std::vector<size_t> filter_large_orders(const std::vector<Order>& orders) {
    std::vector<size_t> indices;
    indices.reserve(orders.size());
    
    for (size_t i = 0; i < orders.size(); ++i) {
        if (orders[i].quantity > 1000) {
            indices.push_back(i);
        }
    }
    return indices;
}

// Option 3: Use std::copy_if with back_inserter
std::vector<Order> filter_large_orders(const std::vector<Order>& orders) {
    std::vector<Order> result;
    result.reserve(orders.size());
    
    std::copy_if(orders.begin(), orders.end(), 
                 std::back_inserter(result),
                 [](const Order& o) { return o.quantity > 1000; });
    
    return result;
}
```

---

**Action Items for Block 3:**
- [ ] Identify all bugs in 5 snippets without looking at answers
- [ ] Write fixes for each in C++
- [ ] Practice explaining what's wrong in 1 minute per snippet
- [ ] Find 2 more buggy snippets from your own codebase to practice

---

### Block 4: Mock Interview Simulation (1 hour)

**Goal:** Run through a full 60-minute interview simulation.

#### Simulation Script

**Set timer for 60 minutes. Speak out loud as if interviewer is present.**

**Minutes 0-5: Introduction**
- "Thanks for having me. I'm excited to discuss the Exchange platform team."
- "Quick background: I won the Kraken Forge SDK track using C++, worked at UBS on repo trading with FIX, and built order book/matching engine projects."
- "I've been fascinated by crypto exchanges since [your story]."

**Minutes 5-20: Project Deep-Dive**
Interviewer: *"Walk me through your Kraken SDK architecture."*

- [Give 10-minute structured walkthrough from Block 2]
- Pause for questions

Interviewer: *"What if we see 10x throughput? How would you scale it?"*

- [Answer from prepared challenge questions]

**Minutes 20-45: Technical Coding**
Interviewer: *"Let's implement a rate limiter for order submission."*

- [Implement token bucket from Block 1, Problem 1]
- [Discuss thread-safety]

Interviewer: *"Now here's some code from our matching engine. Find the bugs."*

- [Review Snippet 2 (race condition) from Block 3]
- [Explain fix]

**Minutes 45-60: System Design**
Interviewer: *"Design market data fan-out for 100K concurrent WebSocket clients."*

```
Your answer structure:
1. Requirements (1 min)
   - L2 vs L3 data
   - Update frequency (real-time vs throttled)
   - Latency budget (10ms P99)

2. Architecture (5 min)
   [Draw diagram with components]:
   - Matching engine publishes to internal bus
   - Market data aggregator subscribes
   - WebSocket farm with load balancer
   - Per-client message queues with backpressure

3. Key decisions (4 min)
   - Throttling: send updates at most 100ms per client
   - Batching: coalesce multiple order book changes
   - Slow client handling: drop connection if queue full

4. Failure modes (2 min)
   - Aggregator crash: redundant instances
   - WebSocket server crash: clients reconnect
   - Network partition: clients use fallback REST
```

**Minutes 60+: Your Questions**
- "What's the typical latency budget for a trade from order submission to execution report?"
- "How does the team handle schema evolution in market data feeds?"
- "What's the on-call rotation like? What are common pages?"
- "Can you walk me through a recent interesting production incident?"
- "What's the tech stack for observability? (OpenTelemetry, Prometheus, Grafana?)"

---

**Action Items for Block 4:**
- [ ] Do full 60-minute simulation, recording yourself if possible
- [ ] Listen back: identify rambling, long pauses, unclear explanations
- [ ] Repeat simulation, improving weak areas
- [ ] Prepare 5 smart questions to ask interviewers

---

<a name="question-bank"></a>
## 5. COMPLETE QUESTION BANK

This section contains all possible question types you might encounter. You don't need to answer every single one, but reviewing each category ensures comprehensive coverage.

### 5.1 C++ Language & Concepts (Deep Technical)

#### Memory Management & Ownership

1. **Explain the difference between stack and heap allocation. When would you prefer heap in a trading system?**

   Answer: Stack allocation is deterministic, fast (just pointer bump), and automatically cleaned up. Heap requires dynamic memory allocator, non-deterministic latency, manual lifetime management. In trading systems, prefer stack for hot-path data (order structs, local buffers) to avoid allocator contention and achieve predictable latency. Use heap only for: 1) data that outlives function scope, 2) large objects that would overflow stack, 3) polymorphic objects needing virtual dispatch.

2. **What is RAII? Give three examples relevant to an exchange system.**

   Answer: Resource Acquisition Is Initialization - resource lifetime tied to object lifetime via constructor/destructor. Examples:
   - `std::unique_ptr<Connection>` managing FIX session: auto-disconnect on scope exit
   - `std::lock_guard<std::mutex>` protecting order book: auto-unlock even if exception
   - `FileLogger` with RAII file handle: ensures flush and close even on crash

3. **Explain move semantics. How does it help in a high-throughput order processing pipeline?**

   Answer: Move semantics transfer ownership/resources instead of copying. `Order&&` allows "stealing" internals (buffer pointers, strings) instead of deep copy. In pipeline:
   ```cpp
   // Without move: 3 copies of Order
   orders.push_back(order);
   
   // With move: 0 copies, just pointer swap
   orders.push_back(std::move(order));
   ```
   Critical for large objects (orders with metadata, market data snapshots) to avoid memcpy overhead.

4. **What is copy elision / Return Value Optimization? Can you rely on it?**

   Answer: Compiler optimization that eliminates temporary copies when returning by value. C++17 guarantees elision in certain cases (prvalue). Can rely on it for:
   ```cpp
   Order create_order() {
       return Order{...};  // Guaranteed elision in C++17
   }
   ```
   Still important: design for move-correctness, elision is optimization not guaranteed everywhere.

5. **Explain the rule of 3/5/0. When would you explicitly delete copy constructor in a trading engine?**

   Answer:
   - Rule of 3 (C++98): If you define destructor, copy ctor, or copy assignment, define all 3
   - Rule of 5 (C++11): Add move ctor and move assignment
   - Rule of 0: Prefer default implementation (use unique_ptr, avoid manual new/delete)
   
   Delete copy constructor for non-copyable resources:
   ```cpp
   class MatchingEngine {
   public:
       MatchingEngine(const MatchingEngine&) = delete;  // Non-copyable
       MatchingEngine(MatchingEngine&&) = default;      // Moveable
   };
   ```
   Prevents accidental copies of expensive resources (network sockets, large in-memory state).

6. **Compare `unique_ptr`, `shared_ptr`, and `weak_ptr`. When would you use each in an order book?**

   Answer:
   - `unique_ptr`: Exclusive ownership, zero overhead. Use for: owned child objects, temporary buffers
   - `shared_ptr`: Shared ownership, reference counted (atomic). Use for: orders referenced by multiple indexes
   - `weak_ptr`: Non-owning observer of shared_ptr. Use for: caching, breaking cycles (e.g., parent/child relationships)
   
   Order book example:
   ```cpp
   std::unordered_map<OrderID, std::shared_ptr<Order>> orders_;  // Shared ownership
   std::map<Price, std::vector<std::weak_ptr<Order>>> bids_;     // Non-owning
   ```

7. **What is a custom deleter for smart pointers? Give a use case.**

   Answer: Lambda/function object specifying how to release resource. Use case:
   ```cpp
   auto conn_deleter = [](Connection* c) {
       c->logout();
       c->disconnect();
       delete c;
   };
   std::unique_ptr<Connection, decltype(conn_deleter)> conn(
       new Connection(), conn_deleter
   );
   ```
   Ensures proper cleanup of FIX sessions (logout before disconnect).

#### Concurrency & Threading

8. **Explain the difference between `std::mutex`, `std::shared_mutex`, and spinlock. When to use each?**

   Answer:
   - `mutex`: Exclusive lock, sleeps waiting thread. Use: moderate contention, longer critical sections
   - `shared_mutex`: Allows multiple readers or one writer. Use: read-heavy workloads (order book queries)
   - Spinlock: Busy-waits, never sleeps. Use: very short critical sections (< 100 cycles), no context switches
   
   Matching engine: spinlock for updating single counter, shared_mutex for order book reads, mutex for rare admin operations.

9. **What is `std::atomic`? Explain memory ordering options.**

   Answer: Lock-free synchronization primitive using CPU atomic instructions. Memory orderings:
   - `memory_order_relaxed`: No ordering constraints, just atomicity (use: counters, stats)
   - `memory_order_acquire`: Reads see all writes before a release (use: consumer in producer-consumer)
   - `memory_order_release`: Writes visible to acquire reads (use: producer)
   - `memory_order_seq_cst`: Sequentially consistent (default, strongest)
   
   Example:
   ```cpp
   std::atomic<uint64_t> order_counter{0};
   order_counter.fetch_add(1, std::memory_order_relaxed);  // Stats collection
   ```

10. **What is a data race? Give an example and fix it.**

    Answer: Two threads accessing same memory, at least one writing, no synchronization. Example:
    ```cpp
    int total = 0;
    // Thread 1 & 2 both execute:
    total++;  // DATA RACE: read-modify-write not atomic
    ```
    
    Fixes:
    ```cpp
    // Option 1: Mutex
    std::mutex mtx;
    {
        std::lock_guard<std::mutex> lock(mtx);
        total++;
    }
    
    // Option 2: Atomic
    std::atomic<int> total{0};
    total.fetch_add(1);
    ```

11. **What is false sharing? How do you prevent it?**

    Answer: Multiple threads writing to different variables on same cache line (typically 64 bytes) causes cache line bouncing. Example:
    ```cpp
    struct Counters {
        std::atomic<int> thread1_count;  // Byte 0-3
        std::atomic<int> thread2_count;  // Byte 4-7 (SAME CACHE LINE!)
    };
    ```
    
    Prevention:
    ```cpp
    struct alignas(64) Counters {
        std::atomic<int> thread1_count;
        char padding[60];  // Pad to 64 bytes
        std::atomic<int> thread2_count;
    };
    
    // Or use C++17:
    struct Counters {
        alignas(std::hardware_destructive_interference_size) std::atomic<int> count;
    };
    ```

12. **Explain producer-consumer pattern with condition variables.**

    Answer:
    ```cpp
    std::queue<Order> queue;
    std::mutex mtx;
    std::condition_variable cv;
    bool done = false;
    
    // Producer
    void produce(Order order) {
        std::unique_lock<std::mutex> lock(mtx);
        queue.push(order);
        cv.notify_one();
    }
    
    // Consumer
    void consume() {
        while (true) {
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, []{ return !queue.empty() || done; });
            
            if (queue.empty()) break;
            Order order = queue.front();
            queue.pop();
            lock.unlock();
            
            process(order);
        }
    }
    ```

13. **What is an ABA problem in lock-free programming?**

    Answer: Thread A reads value V, gets preempted. Thread B changes V to W then back to V. Thread A resumes, sees V unchanged (via compare-and-swap), proceeds incorrectly. Example:
    ```cpp
    Node* head = stack.load();
    Node* next = head->next;
    stack.compare_exchange_strong(head, next);  // May succeed even if stack changed!
    ```
    
    Solutions:
    - Use tagged pointers (extra version counter)
    - Hazard pointers
    - Epoch-based reclamation

14. **How would you implement a thread-safe queue for order messages?**

    Answer:
    ```cpp
    template<typename T>
    class ThreadSafeQueue {
    public:
        void push(T value) {
            std::lock_guard<std::mutex> lock(mutex_);
            queue_.push(std::move(value));
            cv_.notify_one();
        }
        
        bool pop(T& out) {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait(lock, [this]{ return !queue_.empty() || stopped_; });
            
            if (queue_.empty()) return false;
            
            out = std::move(queue_.front());
            queue_.pop();
            return true;
        }
        
        void stop() {
            std::lock_guard<std::mutex> lock(mutex_);
            stopped_ = true;
            cv_.notify_all();
        }
        
    private:
        std::queue<T> queue_;
        std::mutex mutex_;
        std::condition_variable cv_;
        bool stopped_ = false;
    };
    ```

#### Performance & Optimization

15. **Where do allocations happen in C++? List 5 sources.**

    Answer:
    1. `new` / `delete` / `malloc` / `free`
    2. `std::vector::push_back()` when capacity exceeded
    3. `std::string` operations (small-string optimization limit exceeded)
    4. `std::make_shared` / `std::make_unique`
    5. Standard library containers without pre-reserved capacity
    6. Exception handling (allocating exception object)
    7. `std::function` with large captures

16. **How do you profile a C++ application for performance issues?**

    Answer:
    - **CPU profiling**: `perf record -g ./program`, then `perf report` (Linux)
    - **Memory profiling**: `valgrind --tool=massif` or `heaptrack`
    - **Cache misses**: `perf stat -e cache-misses,cache-references`
    - **Lock contention**: `perf lock record`, then `perf lock report`
    - **Custom instrumentation**: RAII timer around hot functions
    
    For matching engine: focus on p99 latency, not average. Use `perf` with `--call-graph dwarf` for accurate stack traces.

17. **What is cache locality? How does it affect order book performance?**

    Answer: CPU caches frequently accessed data. Sequential memory access much faster than random jumps. Order book example:
    ```cpp
    // Bad: vector of pointers (cache misses on every access)
    std::vector<Order*> orders;
    
    // Good: contiguous storage (cache-friendly)
    std::vector<Order> orders;
    
    // Better: struct-of-arrays for hot path
    struct OrderBook {
        std::vector<double> prices;    // Contiguous
        std::vector<double> quantities;
    };
    ```

18. **Explain Small String Optimization (SSO). How to avoid string allocations in hot path?**

    Answer: `std::string` stores short strings (typically ≤15 chars) inline, no heap allocation. For longer strings or hot paths:
    ```cpp
    // Option 1: string_view (non-owning)
    std::string_view symbol = "BTC/USD";
    
    // Option 2: constexpr string for compile-time
    constexpr char symbol[] = "BTC/USD";
    
    // Option 3: intern strings (store once, use pointer)
    const std::string& symbol = symbol_table.get("BTC/USD");
    
    // Option 4: fixed-size buffer for known-small strings
    char symbol[16];
    std::snprintf(symbol, sizeof(symbol), "BTC/USD");
    ```

19. **What is the cost of virtual dispatch? How to avoid it in hot paths?**

    Answer: Virtual function call requires pointer dereference through vtable (~3-5 CPU cycles overhead + prevents inlining). For polymorphic orders:
    ```cpp
    // Slow: virtual dispatch
    class Order {
    public:
        virtual double get_price() const = 0;
    };
    
    // Fast: template static polymorphism (CRTP)
    template<typename Derived>
    class OrderBase {
    public:
        double get_price() const {
            return static_cast<const Derived*>(this)->get_price_impl();
        }
    };
    
    class LimitOrder : public OrderBase<LimitOrder> {
    public:
        double get_price_impl() const { return price_; }
    };
    
    // Or: std::variant for type-safe union (zero virtual overhead)
    using Order = std::variant<LimitOrder, MarketOrder, StopOrder>;
    ```

20. **How would you optimize a function that's called millions of times per second?**

    Answer:
    1. **Profile first**: Confirm it's actually hot with `perf`
    2. **Reduce allocations**: pre-allocate, use object pools, stack buffers
    3. **Inline**: mark `inline` or `constexpr`, keep function small
    4. **Reduce branches**: branchless algorithms, lookup tables instead of if/else
    5. **SIMD**: vectorize with compiler intrinsics if processing arrays
    6. **Cache locality**: struct-of-arrays, pack hot data together
    7. **Avoid syscalls**: no I/O, no locks if possible
    8. **Benchmark**: A/B test with real workload, measure p99 latency

---

### 5.2 Trading Systems & Exchange Domain

#### Order Book & Matching

21. **Explain price-time priority matching algorithm.**

    Answer: Orders matched first by best price (highest bid vs lowest ask), then by arrival time (FIFO within price level). Example:
    ```
    Bids: 100 @ 50.00 (t=1), 200 @ 50.00 (t=2), 150 @ 49.99
    Asks: 100 @ 50.01, 50 @ 50.02
    
    New market sell 250 shares:
    - Match 100 @ 50.00 with oldest bid (t=1)
    - Match 150 @ 50.00 with next bid (t=2), partially fills it
    - Remaining 50 shares of bid (t=2) stay in book
    ```

22. **What data structures would you use to implement an order book?**

    Answer:
    ```cpp
    // Bids: map sorted descending by price
    std::map<double, std::list<Order>, std::greater<>> bids_;
    
    // Asks: map sorted ascending by price
    std::map<double, std::list<Order>> asks_;
    
    // Fast order lookup by ID
    std::unordered_map<OrderID, OrderIterator> order_index_;
    ```
    
    Why:
    - `std::map`: O(log n) insert/delete, sorted iteration, best bid/ask is `begin()`/`rbegin()`
    - `std::list`: O(1) FIFO append/remove within price level, stable iterators
    - `std::unordered_map`: O(1) lookup for cancel/modify by ID

23. **How would you handle modify operations efficiently?**

    Answer:
    Options:
    1. **Cancel + Add**: Simple, loses time priority
    2. **In-place update**: Complex, need to handle price change (move between levels)
    
    Efficient approach:
    ```cpp
    void modify_order(OrderID id, double new_qty) {
        auto& order = order_index_[id];
        
        if (new_qty < order.quantity) {
            // Reduce: keep time priority
            order.quantity = new_qty;
        } else {
            // Increase: lose time priority (cancel + add at end)
            cancel_order(id);
            add_order(id, order.side, order.price, new_qty);
        }
    }
    ```

24. **What is an L2 vs L3 order book feed?**

    Answer:
    - **L2 (Market Depth)**: Aggregated by price level, no individual orders visible
      ```
      Bids: 1000 @ 50.00, 500 @ 49.99
      Asks: 800 @ 50.01, 1200 @ 50.02
      ```
    - **L3 (Full Order Book)**: Every individual order visible with ID
      ```
      Bids: Order-123 200@50.00, Order-456 800@50.00, Order-789 500@49.99
      ```
    
    L2 uses less bandwidth, L3 enables sophisticated strategies (order flow analysis, maker detection).

25. **How would you implement self-trade prevention?**

    Answer: Detect when same account has orders on both sides that would match.
    ```cpp
    bool can_match(const Order& bid, const Order& ask) {
        if (bid.account_id == ask.account_id) {
            // Self-trade detected
            if (self_trade_prevention == CANCEL_NEWEST) {
                cancel_order(ask.id);
                return false;
            } else if (self_trade_prevention == CANCEL_OLDEST) {
                cancel_order(bid.id);
                return false;
            } else if (self_trade_prevention == CANCEL_BOTH) {
                cancel_order(bid.id);
                cancel_order(ask.id);
                return false;
            }
        }
        return true;
    }
    ```

26. **Explain different order types: Market, Limit, Stop, Fill-or-Kill, Immediate-or-Cancel.**

    Answer:
    - **Market**: Execute immediately at best available price (no price specified)
    - **Limit**: Execute at specified price or better (may rest in book)
    - **Stop**: Becomes market/limit order when trigger price reached
    - **Fill-or-Kill (FOK)**: Execute entire quantity immediately or cancel
    - **Immediate-or-Cancel (IOC)**: Execute whatever matches immediately, cancel rest
    - **Post-Only**: Only add to book, cancel if would match (maker-only)

27. **How would you implement a matching engine that handles 100K orders/sec with sub-millisecond latency?**

    Answer:
    - **Single-threaded per symbol**: No locks, predictable latency
    - **Pre-allocated memory**: Object pools for orders, no malloc in hot path
    - **Cache-friendly data**: Contiguous order storage, inline small orders
    - **Lock-free queues**: For cross-thread communication
    - **Affinity**: Pin thread to CPU core, avoid context switches
    - **Profiling**: Continuous p99 latency monitoring, profile regularly
    - **Batch processing**: Process 10 orders at a time, amortize overhead

#### FIX Protocol

28. **What is FIX protocol? What are key message types?**

    Answer: Financial Information eXchange - industry standard for electronic trading. Key messages:
    - **NewOrderSingle (D)**: Submit new order
    - **ExecutionReport (8)**: Order status update (fill, partial fill, cancel, reject)
    - **OrderCancelRequest (F)**: Request to cancel order
    - **OrderCancelReject (9)**: Cancel request rejected
    - **MarketDataRequest (V)**: Subscribe to market data
    - **MarketDataSnapshot (W)**: Market data update

29. **Explain ClOrdID vs OrderID in FIX.**

    Answer:
    - **ClOrdID (Tag 11)**: Client Order ID, assigned by buy-side (client)
    - **OrderID (Tag 37)**: Exchange Order ID, assigned by sell-side (exchange)
    
    Client uses ClOrdID to track orders, exchange uses OrderID. Both included in ExecutionReports for correlation. ClOrdID changes on modify, OrderID stays same.

30. **What is a FIX message structure?**

    Answer:
    ```
    BeginString (8): FIX version (e.g., "FIX.4.4")
    BodyLength (9): Byte count of message body
    MsgType (35): Message type (e.g., "D" for NewOrderSingle)
    SenderCompID (49): Sender identifier
    TargetCompID (56): Receiver identifier
    MsgSeqNum (34): Sequence number
    SendingTime (52): Timestamp
    [Message-specific fields]
    CheckSum (10): Integrity checksum
    ```
    
    Format: `8=FIX.4.4|9=120|35=D|49=CLIENT|56=EXCHANGE|...|10=123|` (| represents SOH character 0x01)

31. **How would you handle FIX message sequence gaps?**

    Answer:
    - Detect gap: received MsgSeqNum 105, expected 103
    - Send ResendRequest (2): "Please resend messages 103-104"
    - Exchange resends missed messages with PossDupFlag=Y
    - Apply messages in order, update sequence number
    - If critical: disconnect and reconnect, trigger full state sync

#### Market Data & APIs

32. **Design a WebSocket market data feed for 100K concurrent clients.**

    Answer:
    ```
    Matching Engine
        ↓ (publish internal events)
    Market Data Aggregator
        ↓ (fan out)
    WebSocket Server Farm (10 servers, 10K clients each)
        ↓
    Clients
    ```
    
    Key decisions:
    - **Throttling**: Max 10 updates/sec per client (100ms batching)
    - **Backpressure**: If client queue > 1000 messages, disconnect slow client
    - **Compression**: zlib for snapshots, delta updates for incremental
    - **Load balancing**: Sticky sessions by client IP
    - **Redundancy**: 2x capacity, instant failover

33. **How would you implement rate limiting in an API gateway?**

    Answer: Token bucket per API key + per IP:
    ```cpp
    class APIGateway {
    public:
        bool allow_request(const std::string& api_key, const std::string& ip) {
            if (!api_key_limiters_[api_key].allow()) return false;
            if (!ip_limiters_[ip].allow()) return false;
            return true;
        }
        
    private:
        std::unordered_map<std::string, RateLimiter> api_key_limiters_;
        std::unordered_map<std::string, RateLimiter> ip_limiters_;
    };
    ```
    
    Return HTTP 429 when limit exceeded, include `Retry-After` header.

34. **Explain the difference between REST and WebSocket for market data.**

    Answer:
    - **REST**: Request-response, polling, stateless. Use for: snapshots, historical data, infrequent queries
    - **WebSocket**: Persistent connection, server push, stateful. Use for: real-time updates, low latency, frequent data
    
    Typical flow: REST for initial snapshot + full order book, then WebSocket for incremental updates.

35. **How would you handle out-of-order messages in market data?**

    Answer:
    - Assign sequence number to each update
    - Client maintains `expected_seq`
    - On message arrival:
      ```cpp
      if (msg.seq == expected_seq) {
          apply(msg);
          expected_seq++;
      } else if (msg.seq > expected_seq) {
          // Gap detected, buffer message
          buffered_msgs_[msg.seq] = msg;
          request_snapshot();  // Or request missed messages
      } else {
          // Duplicate or old, ignore
      }
      ```

---

### 5.3 System Design Questions

36. **Design a distributed matching engine across multiple data centers.**

37. **Design a margin and risk management system for leveraged trading.**

38. **Design an API gateway that handles authentication, rate limiting, and routing.**

39. **Design a real-time settlement system for crypto trades.**

40. **Design a cold storage system for customer crypto assets.**

*(Detailed answers for these would make this document too long, but structure each answer with: requirements, architecture diagram, key decisions, failure modes, and trade-offs.)*

---

### 5.4 Crypto & Blockchain Specific

41. **What is the difference between a centralized exchange (CEX) and decentralized exchange (DEX)?**

    Answer:
    - **CEX (Kraken, Coinbase)**: Custodial, matching engine controlled by company, faster, easier UX, regulated
    - **DEX (Uniswap, dYdX)**: Non-custodial, smart contracts on-chain, slower (blockchain latency), composable
    
    Kraken is CEX but exploring hybrid models.

42. **What is an automated market maker (AMM)? How does it differ from an order book?**

    Answer:
    - **AMM**: Uses liquidity pools and pricing formula (e.g., x*y=k), no discrete orders
    - **Order book**: Discrete limit orders, price-time priority matching
    
    AMM always has liquidity (though price may be poor), order book may have empty levels.

43. **What are the unique challenges of running a 24/7 crypto exchange?**

    Answer:
    - No maintenance windows (traditional markets close overnight)
    - Deploy without downtime: blue-green deployments, canary releases
    - Global on-call coverage across time zones
    - Unpredictable volume spikes (news events)
    - Regulatory uncertainty across jurisdictions

44. **How would you ensure security in a crypto trading system?**

    Answer:
    - **Hot/cold wallet separation**: Majority in cold storage, minimal hot wallet
    - **Multi-sig**: Require M-of-N signatures for withdrawals
    - **Rate limiting**: Detect and block abnormal withdrawal patterns
    - **2FA**: Enforce for all account operations
    - **Audit trail**: Immutable logs of all transactions
    - **Penetration testing**: Regular security audits
    - **Incident response**: Playbooks for security breaches

45. **What is Kraken's mission? Why does it matter for engineering?**

    Answer: Kraken aims to accelerate adoption of cryptocurrency and build a more inclusive financial system. Engineering implications:
    - **Reliability**: Downtime erodes trust in crypto (already volatile)
    - **Performance**: Competitive with TradFi (sub-ms latency)
    - **Security**: Custody of billions, target for attackers
    - **Access**: API-first, support institutional and retail
    - **Innovation**: Push boundaries (Lightning, DeFi integrations)

---

### 5.5 Behavioral & Culture

46. **Tell me about a production incident you debugged.**

47. **Describe a time you optimized a critical system.**

48. **How do you handle disagreement about technical decisions?**

49. **What's your approach to code reviews?**

50. **How do you stay current with C++ and systems programming?**

*(Prepare STAR format stories: Situation, Task, Action, Result. Draw from UBS experience and personal projects.)*

---

<a name="key-concepts"></a>
## 6. KEY CONCEPTS REFERENCE

### C++ Quick Reference

#### Smart Pointers
```cpp
// unique_ptr: Exclusive ownership
std::unique_ptr<Order> order = std::make_unique<Order>();
auto moved = std::move(order);  // Transfer ownership

// shared_ptr: Shared ownership
std::shared_ptr<Order> order = std::make_shared<Order>();
auto copy = order;  // Increment ref count

// weak_ptr: Non-owning observer
std::weak_ptr<Order> weak = order;
if (auto shared = weak.lock()) {
    // Use order if still alive
}
```

#### Concurrency Primitives
```cpp
// Mutex
std::mutex mtx;
{
    std::lock_guard<std::mutex> lock(mtx);
    // Critical section
}

// Atomic
std::atomic<int> counter{0};
counter.fetch_add(1, std::memory_order_relaxed);

// Condition Variable
std::condition_variable cv;
std::unique_lock<std::mutex> lock(mtx);
cv.wait(lock, []{ return ready; });
```

### Order Book Data Structure
```cpp
class OrderBook {
private:
    // Price -> Orders at that price
    std::map<double, std::list<Order>, std::greater<>> bids_;  // Descending
    std::map<double, std::list<Order>> asks_;                  // Ascending
    
    // OrderID -> Iterator for O(1) lookup
    std::unordered_map<OrderID, OrderIterator> order_index_;
};
```

### Matching Algorithm Pseudocode
```
function match_order(new_order):
    if new_order.side == BUY:
        opposite_side = asks_
    else:
        opposite_side = bids_
    
    while new_order.quantity > 0 and not opposite_side.empty():
        best_level = opposite_side.begin()
        
        if not prices_cross(new_order.price, best_level.price):
            break  # No more matches
        
        for order in best_level.orders:
            trade_qty = min(new_order.quantity, order.quantity)
            execute_trade(new_order, order, trade_qty)
            
            new_order.quantity -= trade_qty
            order.quantity -= trade_qty
            
            if order.quantity == 0:
                remove order from book
            
            if new_order.quantity == 0:
                break
    
    if new_order.quantity > 0 and new_order.type == LIMIT:
        add new_order to book
```

### Performance Checklist
- [ ] Profile with `perf` before optimizing
- [ ] Pre-allocate containers (`reserve()`)
- [ ] Avoid copies (use `const&`, move semantics)
- [ ] Minimize allocations (object pools, stack buffers)
- [ ] Use `string_view` instead of `string` where possible
- [ ] Mark hot functions `inline`
- [ ] Avoid virtual dispatch in hot paths
- [ ] Consider cache locality (struct-of-arrays)
- [ ] Use `std::atomic` instead of mutex for simple counters
- [ ] Profile again after changes

---

<a name="mock-interview"></a>
## 7. MOCK INTERVIEW SIMULATION

### Full 60-Minute Script

**Setup:**
- Timer set for 60 minutes
- Whiteboard or paper ready
- Speak out loud as if interviewer is present
- Record yourself if possible

---

**[0:00 - 0:03] Introduction**

Interviewer: *"Thanks for joining us. Can you give a quick intro?"*

You: "Thanks for having me. I'm [your name], currently based in Pune. I recently won the Kraken Forge hackathon in the SDK client track where I built a C++ SDK for your trading API. Before that, I spent time at UBS working on repo trading systems with FIX protocol and trade automation. I'm really excited about Kraken's mission to accelerate crypto adoption, and the Exchange platform team is exactly where I want to contribute - building high-performance, reliable trading infrastructure."

---

**[0:03 - 0:18] Project Deep-Dive**

Interviewer: *"Great. Let's dive into your Kraken SDK. Walk me through the architecture."*

You: [Give 10-minute structured walkthrough]:

"Sure. The SDK is a **production-grade C++ library** that gives traders and algo firms a clean, high-performance way to consume Kraken's WebSocket API v2 — real-time tickers, trades, order book snapshots, and private channels like own trades and balances.

The architecture follows a **two-thread reactor pattern**:

**I/O Thread** runs a Boost.Asio event loop — it reads WebSocket frames via Boost.Beast over TLS, parses JSON with RapidJSON (~1.8μs per message), and pushes parsed messages into a lock-free **SPSC queue** (rigtorp::SPSCQueue, ~12ns push/pop).

**Dispatcher Thread** pops from the queue and invokes user callbacks — `on_ticker()`, `on_trade()`, `on_book()`, etc. It also feeds data into the **Strategy Engine**, which evaluates user-defined alert strategies (price alerts, volume spikes, spread monitors) in real-time.

This separation guarantees the I/O thread is **never blocked** by user callback logic. And for ultra-low-latency cases, there's a **direct mode** that bypasses the queue entirely.

The public API uses the **PIMPL pattern** — `KrakenClient` holds a `unique_ptr<Impl>`. The public header has zero heavy includes, so users compile fast and get ABI stability. All configuration goes through a **fluent Builder** with 20+ parameters.

For **resilience**, I implemented three layers: a **circuit breaker** (tracks consecutive failures, opens to reject connections temporarily), **exponential backoff with jitter** (configurable presets: aggressive/conservative/infinite), and a **token bucket rate limiter** for outbound messages.

Data integrity is verified with **CRC32 checksums** on order book snapshots (computed locally, compared against exchange-provided checksum) and **sequence-based gap detection** across all channels.

For **authentication**, private channels use **HMAC-SHA512** — the SDK generates a nonce, signs `nonce + api_key` with the secret, and sends the token inline.

For **observability**, I integrated structured logging (spdlog), a local metrics API with atomic counters (`get_metrics()`), an OpenTelemetry OTLP exporter, and a built-in Prometheus HTTP endpoint.

Testing covers ~250 unit tests, ~50 integration tests, ~40 stress tests (queue overflow, concurrent callbacks, malformed JSON), plus Google Benchmark for continuous performance regression."

Interviewer: *"Impressive. What if users start seeing intermittent timeouts?"*

You: "I'd approach it systematically:

First, **check the SDK's built-in metrics** via `get_metrics()` — messages received, reconnect count, queue depth, dropped messages. The metrics use atomic counters so they're always available without locking.

Then **check the connection state machine** — the SDK tracks `Disconnected → Connecting → Connected → Reconnecting` transitions. The `on_connection_state` callback logs every transition with timestamps.

**Check structured logs** (spdlog) — they include reconnect attempts, backoff delays, circuit breaker state changes, and TLS handshake timing.

If the circuit breaker is OPEN, that means we've hit the failure threshold and the SDK is deliberately rejecting connections for a cooldown period. I'd check the configured threshold and recovery timeout.

If it's TLS-related, I'd check cert verification settings and the OpenSSL context configuration.

For deeper diagnosis, I'd enable the **OpenTelemetry exporter** to get distributed traces showing exact timing of each stage."

Interviewer: *"How would you scale this SDK to handle 10x throughput?"*

You: "Three main areas:

**1. Queue tuning**: Increase SPSC queue capacity from default 131,072 to 1M+. The lock-free design means throughput scales linearly with capacity — no lock contention.

**2. Direct mode**: For ultra-low-latency, bypass the queue entirely with `use_queue(false)`. This dispatches callbacks directly on the I/O thread — eliminates the ~12ns queue overhead but requires non-blocking callbacks.

**3. Parse optimization**: RapidJSON is already fast (~1.8μs), but for 10x load I'd consider simd-json or pre-allocated document pools to avoid per-message allocations.

I'd also **profile with perf** under load to find actual bottlenecks — could be JSON parsing, callback execution time, or something unexpected in Boost.Asio's epoll loop.

Finally, I'd use the built-in **Prometheus endpoint** to monitor queue depth and message rates in real-time during load testing."

---

**[0:18 - 0:45] Technical Coding**

Interviewer: *"Let's code. Implement a rate limiter that allows at most 5 orders per second with a burst of 10."*

You: [Write on whiteboard or code editor]:

```cpp
#include <chrono>

class RateLimiter {
public:
    RateLimiter(double rate_per_sec, double burst)
        : rate_(rate_per_sec)
        , capacity_(burst)
        , tokens_(burst)
        , last_refill_(std::chrono::steady_clock::now())
    {}
    
    bool allow() {
        refill();
        
        if (tokens_ >= 1.0) {
            tokens_ -= 1.0;
            return true;
        }
        return false;
    }
    
private:
    void refill() {
        auto now = std::chrono::steady_clock::now();
        double elapsed = std::chrono::duration<double>(now - last_refill_).count();
        tokens_ = std::min(capacity_, tokens_ + elapsed * rate_);
        last_refill_ = now;
    }
    
    double rate_;
    double capacity_;
    double tokens_;
    std::chrono::steady_clock::time_point last_refill_;
};
```

"This uses a token bucket: tokens refill continuously at `rate_per_sec`, capped at `capacity_`. Each request consumes 1 token. If tokens available, allow; otherwise, deny."

Interviewer: *"How would you make this thread-safe?"*

You: "Add a mutex around `allow()`:

```cpp
bool allow() {
    std::lock_guard<std::mutex> lock(mutex_);
    refill();
    
    if (tokens_ >= 1.0) {
        tokens_ -= 1.0;
        return true;
    }
    return false;
}

private:
    mutable std::mutex mutex_;
```

If lock contention becomes an issue with high concurrency, I'd shard by API key - each key gets its own limiter, so different users don't contend."

Interviewer: *"Good. Now here's a code snippet from our system. Find the bug."*

```cpp
class Metrics {
public:
    void record_order() {
        ++total_orders_;
    }
    
    int get_total() const {
        return total_orders_;
    }
    
private:
    int total_orders_ = 0;
};
```

You: "This has a **data race**. If multiple threads call `record_order()` concurrently, the increment `++total_orders_` is a read-modify-write that's not atomic. Under the C++ memory model, this is undefined behavior.

Fix with `std::atomic`:

```cpp
class Metrics {
public:
    void record_order() {
        total_orders_.fetch_add(1, std::memory_order_relaxed);
    }
    
    int get_total() const {
        return total_orders_.load(std::memory_order_relaxed);
    }
    
private:
    std::atomic<int> total_orders_{0};
};
```

I used `memory_order_relaxed` because we only care about atomicity, not ordering with other operations. If this counter needs to be consistent with other state, we'd need `acquire`/`release` semantics."

---

**[0:45 - 0:70] System Design**

Interviewer: *"Let's switch to system design. Design market data fan-out for 100,000 concurrent WebSocket clients subscribing to BTC-USD order book updates."*

You: [Draw diagram on whiteboard]:

```
┌─────────────────────────────┐
│   Matching Engine (BTC-USD) │
│   (publishes order book      │
│    events to internal bus)   │
└──────────────┬──────────────┘
               │
               ▼
┌─────────────────────────────┐
│  Market Data Aggregator      │
│  (subscribes to internal     │
│   bus, transforms events)    │
└──────────────┬──────────────┘
               │
               ▼
┌─────────────────────────────┐
│  WebSocket Server Farm       │
│  (10 servers, 10K clients    │
│   each, load balanced)       │
└──────────────┬──────────────┘
               │
               ▼
      ┌────────┴────────┐
      ▼                 ▼
   Clients           Clients
```

"Here's my approach:

**Requirements**:
- 100K concurrent WebSocket connections
- Real-time order book updates (L2 or L3)
- Low latency (p99 < 50ms from event to client)
- Handle slow clients without blocking fast ones

**Architecture**:

1. **Matching engine** publishes order book events (add, cancel, trade) to an internal message bus (e.g., Kafka, Redis Streams, or custom ring buffer).

2. **Market data aggregator** subscribes to the bus. It:
   - Transforms internal format to external API format
   - Aggregates L3 to L2 if needed (sum quantities per price level)
   - Applies throttling: batch updates, send at most 10/sec per client

3. **WebSocket server farm**: 10 servers, each handling 10K connections. Load balancer uses sticky sessions (by client ID) to ensure clients reconnect to same server.

4. **Per-client queue**: Each client has a bounded queue (e.g., 1000 messages). If queue full, disconnect client (slow consumer).

**Key Decisions**:

- **Throttling**: Batch updates over 100ms windows, send snapshot diff. Reduces bandwidth and client processing load.
- **Backpressure**: Drop slow clients to prevent one client blocking the system.
- **Compression**: Use zlib or Brotli for initial snapshot, then delta updates.
- **Redundancy**: Run 2x capacity, so if one server fails, others absorb load.

**Failure Modes**:

| Failure | Detection | Mitigation |
|---------|-----------|------------|
| Aggregator crash | Heartbeat monitor | Standby aggregator takes over, reads from bus offset |
| WebSocket server crash | Health check fails | Load balancer redirects clients to other servers |
| Slow client | Queue depth metric | Disconnect after warning |
| Message bus lag | Consumer lag metric | Scale aggregator horizontally, partition by symbol |

**Scalability**:
- Horizontal: Add more WebSocket servers, partition by symbol
- Vertical: Tune TCP buffer sizes, use epoll for I/O multiplexing

This design handles 100K clients with room to scale to millions by adding servers."

Interviewer: *"How would you handle clients reconnecting and needing a full snapshot?"*

You: "Clients should request a snapshot via REST endpoint first, then subscribe to WebSocket for incremental updates. The snapshot includes a sequence number. WebSocket messages include sequence numbers too. Client applies updates with `seq > snapshot_seq` and buffers any gaps.

If gap detected (missed sequence), client can either:
1. Request snapshot again (simple, but expensive)
2. Request missed messages (if we keep history for 60 seconds)

This is similar to how Kraken actually does it with the `book` subscription."

---

**[0:70 - 0:83] Culture & Wrap-Up**

Interviewer: *"Great. A few quick culture questions. Why Kraken, and why now?"*

You: "Three reasons:

**First, mission alignment**: I believe crypto is the future of finance - more inclusive, transparent, and innovative. Kraken is leading this with a focus on security and institutional adoption. The Forge hackathon showed me your commitment to developer community.

**Second, technical challenge**: The Exchange team builds genuinely hard systems - sub-millisecond matching, 24/7 uptime, massive scale. That's the kind of problem I love. My UBS experience with trading systems prepared me, but crypto's unique challenges (volatility, global 24/7 markets) are exciting.

**Third, timing**: I've been building toward this - my order book projects, Kraken SDK, FIX protocol experience. Winning Forge validated that my skills are a fit. I'm ready to contribute immediately."

Interviewer: *"Tell me about a time you handled a production incident."*

You: [Use STAR format - example from UBS]:

"At UBS, we had a Friday afternoon incident where repo trade bookings started failing. Traders were panicking because EOD was approaching.

**Situation**: Trade booking service was timing out, but logs showed requests reaching the service.

**Task**: I was on-call, needed to restore service and ensure no trades were lost.

**Action**: I checked metrics first - DB query latency had spiked to 30 seconds. Dug into slow query logs, found a missing index on a table that had grown 10x overnight due to a data migration. I added the index (took 5 minutes), and latency dropped back to 50ms. Then I replayed the 23 failed trades from our queue, verified with traders.

**Result**: Service restored in 15 minutes, zero trade loss. Post-incident, I added monitoring for query latency by table, and we changed the migration process to add indexes before bulk inserts. Wrote a detailed post-mortem that became our template."

Interviewer: *"Perfect. Do you have questions for us?"*

You:
1. "What's the typical latency budget for a trade - from order submission via API to execution report back to the client?"
2. "How does the Exchange team handle schema evolution in market data feeds without breaking existing clients?"
3. "Can you walk me through a recent production incident the team handled? What was the root cause and how did you fix it?"
4. "What's the on-call rotation like? What are the most common pages?"
5. "What's your observability stack? OpenTelemetry, Prometheus, Grafana? How do you do distributed tracing?"

Interviewer: *"Great questions. We'll answer those and then wrap up."*

---

**[End of Simulation]**

---

<a name="final-checklist"></a>
## 8. FINAL CHECKLIST

### Day Before Interview

**Technical Readiness:**
- [ ] Can draw your Kraken SDK architecture from memory in 3 minutes
- [ ] Can explain price-time priority matching algorithm in 2 minutes
- [ ] Can code token bucket rate limiter in 15 minutes
- [ ] Can design matching engine end-to-end in 15 minutes
- [ ] Reviewed all 5 code review snippets, can spot bugs in 1 minute each
- [ ] Prepared 3 production incident stories (STAR format)

**Materials Ready:**
- [ ] Laptop/tablet for virtual interview, charged and tested
- [ ] Backup device in case of technical issues
- [ ] Whiteboard or large paper for diagrams
- [ ] Pen/pencil (multiple colors helpful for diagrams)
- [ ] Water nearby
- [ ] Quiet space reserved, no interruptions

**Mental Preparation:**
- [ ] Reviewed Kraken's mission and values
- [ ] Read latest Kraken engineering blog posts
- [ ] Prepared 5 smart questions to ask interviewers
- [ ] Visualization: imagine walking through the interview successfully
- [ ] Sleep: aim for 8 hours

---

### Interview Day

**Morning:**
- [ ] Light review only (no cramming - you're prepared)
- [ ] Skim your SDK architecture diagram
- [ ] Quick review of matching engine design
- [ ] Eat a good meal 2 hours before interview

**30 Minutes Before:**
- [ ] Test camera, microphone, internet connection
- [ ] Join meeting 5 minutes early
- [ ] Have water ready, use bathroom
- [ ] Deep breathing: 4-7-8 technique (4 sec inhale, 7 sec hold, 8 sec exhale)

**During Interview:**
- [ ] Think out loud - explain your reasoning as you work
- [ ] Ask clarifying questions before diving in
- [ ] Draw diagrams - visual communication is powerful
- [ ] If stuck, say so and talk through your approach
- [ ] Watch time - don't spend 30 min on a 10 min problem
- [ ] Be enthusiastic - they want to work with excited people
- [ ] Take notes on their questions/feedback

**After Interview:**
- [ ] Send thank-you email within 24 hours
- [ ] Reference specific discussion points from interview
- [ ] Reiterate interest and excitement
- [ ] Review: what went well? What would you improve?

---

## 9. ADDITIONAL RESOURCES

### Recommended Reading (If Time Permits)

**C++ Performance:**
- "Optimized C++" by Kurt Guntheroth (Chapter 10: Optimize Hot Code)
- "C++ Concurrency in Action" by Anthony Williams (Chapter 5: Memory Model)

**Trading Systems:**
- Kraken Engineering Blog: https://blog.kraken.com/engineering
- "Electronic Trading Systems" concepts (focus on matching engines)

**System Design:**
- "Designing Data-Intensive Applications" by Martin Kleppmann (Chapter 3: Storage and Retrieval)

### Online References

- C++ Reference: https://en.cppreference.com
- Kraken API Documentation: https://docs.kraken.com
- FIX Protocol: https://www.fixtrading.org/standards/
- Linux Performance Tools: http://www.brendangregg.com/linuxperf.html

---

## 10. CONFIDENCE BUILDERS

### Your Strengths

You have several unique advantages going into this interview:

1. **Kraken Forge Winner** - You've already demonstrated capability with their API and C++. They invited you, which means they see potential.

2. **Real Trading Experience** - UBS background with FIX protocol and repo trading is directly applicable. You understand the domain, not just the code.

3. **Relevant Projects** - Order book, matching engine implementations show you've practiced the exact systems they build.

4. **Fast-Track Status** - Single technical round means they're already bought-in on your potential. This is about confirming fit, not starting from zero.

5. **Preparation** - Following this guide puts you ahead of 90% of candidates who wing it.

### Mindset for Success

- **You belong here**: They invited you. They want you to succeed.
- **Collaboration, not interrogation**: They're evaluating if you'll be a good teammate, not trying to trick you.
- **Perfect is the enemy of good**: A working solution explained clearly beats a perfect solution mumbled through.
- **Ask questions**: Clarifying requirements shows engineering judgment.
- **Learn from mistakes**: If you realize an error mid-interview, acknowledge it and fix it. Shows self-awareness.

### If Things Go Sideways

- **Stuck on a problem?** Say: "I'm not immediately seeing the optimal approach. Let me talk through a brute force solution first, then we can optimize."
- **Don't know something?** Say: "I haven't worked with X directly, but here's my understanding based on Y. Can you clarify?"
- **Made a mistake?** Say: "Actually, I see an issue with my approach - [explain]. Let me revise."

**Remember**: They're hiring for potential and problem-solving approach, not trivia knowledge.

---

## 11. FINAL THOUGHTS

You're well-prepared. Trust your preparation, trust your experience, and trust yourself.

The Kraken team is full of people who love building hard systems, who care about crypto's mission, and who value curiosity and craftsmanship. Show them that you're one of those people.

You've got this. Now go build something great.

---

**Good luck! 🚀**

---

## APPENDIX: Quick Reference Cards

### Card 1: SDK Architecture (Memorize This)
```
┌─────────────────────────────────────────┐
│  User Callbacks + Strategy Engine       │ ← on_ticker(), on_trade(), alerts
├─────────────────────────────────────────┤
│  KrakenClient (PIMPL → Impl)            │ ← subscribe(), run(), add_alert()
├─────────────────────────────────────────┤
│  Dispatcher ◄── SPSC Queue ◄── I/O      │ ← Two-thread reactor pattern
├─────────────────────────────────────────┤
│  Circuit Breaker · Backoff · RateLimiter│ ← Resilience layer
├─────────────────────────────────────────┤
│  Boost.Beast WS · TLS · HMAC · RapidJSON│ ← Transport + auth
├─────────────────────────────────────────┤
│  spdlog · OTLP · Prometheus             │ ← Observability
└─────────────────────────────────────────┘
```

### Card 2: Matching Engine Architecture
```
API Gateway → Order Validation → Matching Engine → Trade Logger → Market Data Publisher
                                       ↓
                    std::map<price, std::list<Order>>
                    (bids descending, asks ascending)
```

### Card 3: Common Bugs
1. **Dangling reference**: returning reference to local variable
2. **Data race**: non-atomic increment
3. **Exception safety**: partial state update when exception thrown
4. **Memory leak**: circular shared_ptr
5. **Unnecessary copies**: loop by value instead of const&

### Card 4: Your 30-Second Pitch
"I'm a C++ engineer passionate about high-performance trading systems. I won Kraken Forge building a production-grade C++ SDK for your WebSocket API — featuring a two-thread reactor with lock-free SPSC queues, a real-time strategy engine, circuit breakers with exponential backoff, HMAC-SHA512 auth, and full observability via spdlog, OpenTelemetry, and Prometheus. I have FIX protocol experience from UBS repo trading, and I've built order book and matching engine projects. I'm excited about Kraken's mission and want to contribute to the Exchange platform team."

### Card 5: Learning Docs Quick Reference

For detailed explanations of every concept used in your SDK, see these chapters:

| # | Chapter | Key Topics |
|---|---------|------------|
| 0 | [README — Learning Roadmap](README.md) | Concept map, reading order |
| 1 | [C++17 Features](01_CPP17_FEATURES.md) | variant, optional, function, smart ptrs, move semantics |
| 2 | [Design Patterns](02_DESIGN_PATTERNS.md) | PIMPL, Builder, Strategy, Observer, Composite, RAII |
| 3 | [Concurrency](03_CONCURRENCY.md) | Two-thread reactor, SPSC queue, atomics, mutex, direct mode |
| 4 | [System Design](04_SYSTEM_DESIGN.md) | Event-driven arch, data flow, state machine, dispatch table |
| 5 | [Networking](05_NETWORKING.md) | WebSocket, Boost.Beast, TLS, RapidJSON, HMAC auth |
| 6 | [Resilience](06_RESILIENCE.md) | Circuit breaker, backoff, rate limiter, CRC32, gap detection |
| 7 | [Observability](07_OBSERVABILITY.md) | spdlog, OpenTelemetry, Prometheus, metrics API |
| 8 | [Testing](08_TESTING.md) | Google Test, benchmarks, stress testing, CI/CD |

---

**End of Preparation Guide**

This guide is your roadmap. Follow it, trust the process, and you'll walk into that interview fully prepared.
