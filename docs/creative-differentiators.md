# Delivery Plan: What to Build (2 Days)

This document prioritizes features for maximum impact in the hackathon.

---

## ✅ Core Features (MUST DELIVER)

These are now integrated into the main product. Not optional.

### Day 1: Foundation

| Feature | Time | Priority |
|---------|------|----------|
| WebSocket + TLS | 2-3h | 🔴 Critical |
| JSON parsing (RapidJSON) | 1-2h | 🔴 Critical |
| SPSC queue (rigtorp) | 1-2h | 🔴 Critical |
| I/O + Dispatcher threads | 2-3h | 🔴 Critical |
| Ticker callback working | 1-2h | 🔴 Critical |
| Order book + CRC32 | 2-3h | 🔴 Critical |
| Basic subscription | 1h | 🔴 Critical |

**Day 1 Total: ~10-14 hours**

### Day 2: Differentiators ⭐

| Feature | Time | Priority |
|---------|------|----------|
| **Alert Strategy Engine** | 3-4h | 🔴 Critical |
| PriceAlert strategy | 1h | 🔴 Critical |
| VolumeSpike strategy | 1-2h | 🔴 Critical |
| **Live Dashboard** | 2-3h | 🟡 High |
| **Benchmark tool** | 1-2h | 🟡 High |
| Auto-reconnection | 1-2h | 🟡 High |
| Examples (3) | 1-2h | 🟡 High |
| README + docs | 1h | 🟡 High |

**Day 2 Total: ~12-16 hours**

---

## 🎯 The Three Differentiators

### 1. Trading Strategy Engine ⭐⭐⭐

**Why it's critical:**
- Transforms SDK from "data pipe" to "trading intelligence"
- Easy to demo (show alert firing)
- Shows you understand trader needs
- Extensible (custom strategies)

**Implementation:**
```cpp
// Base class
class AlertStrategy {
    virtual bool check(const Ticker& t) = 0;
};

// Built-in strategies
class PriceAlert : public AlertStrategy { ... };
class VolumeSpike : public AlertStrategy { ... };

// Client integration
client.add_alert(strategy, callback);
```

### 2. Live Performance Dashboard ⭐⭐

**Why it's important:**
- Visual proof SDK works
- Shows you care about observability
- Impressive in demo video

**Implementation:**
```cpp
// ANSI escape codes (no ncurses needed)
std::cout << "\033[2J\033[H";  // Clear screen
std::cout << "┌───────────────────────┐\n";
std::cout << "│ Messages/sec: " << rate << "│\n";
...
```

### 3. Benchmark Suite ⭐⭐

**Why it matters:**
- Proves performance claims
- Shows engineering rigor
- Good for README

**Implementation:**
```cpp
// Subscribe, measure for 60s, output stats
// Output: JSON file + console summary
{
  "messages_per_second": 52847,
  "max_latency_us": 2300,
  "dropped": 0
}
```

---

## 🔄 If Running Behind Schedule

### Minimum Viable Product (MVP)

If you're running out of time, deliver this:

1. ✅ WebSocket connection working
2. ✅ Ticker subscription working
3. ✅ Order book with checksum
4. ✅ **At least PriceAlert strategy working**
5. ✅ One working example
6. ✅ Basic README

**Skip if needed:**
- VolumeSpike strategy (keep PriceAlert only)
- Live dashboard (can show metrics in callback)
- Auto-reconnection (manual reconnect is OK)
- Multiple examples (one is enough)

---

## 🏃 If Ahead of Schedule

### Polish Items (Nice to Have)

| Feature | Time | Impact |
|---------|------|--------|
| SpreadAlert strategy | 30min | Medium |
| Interactive CLI | 2-3h | Medium |
| More examples | 1-2h | Medium |
| Doxygen docs | 1h | Low |
| Unit tests | 2-3h | Low |

---

## 🎬 Demo Video Script (60 seconds)

Record this at the end of Day 2:

**0-10s: Quickstart**
```
"This is the Kraken SDK in 5 lines of code."
[Show quickstart.cpp compiling and running]
```

**10-30s: Strategy Engine**
```
"But what makes this SDK special is the strategy engine."
"Set a price alert - get notified when it triggers."
[Show alert firing in real-time]
```

**30-45s: Performance Dashboard**
```
"Built for performance."
[Show live dashboard with messages/sec]
"50,000+ messages per second, lock-free queue."
```

**45-60s: Close**
```
"Kraken SDK. Production-grade. Trading-ready."
[Show README on screen]
```

---

## 📝 README Must-Haves

1. **Quickstart** - 5-line code example at top
2. **Strategy example** - Show PriceAlert in action
3. **Architecture diagram** - ASCII art showing thread model
4. **Performance numbers** - From benchmark tool
5. **Comparison table** - "This SDK vs Others"

---

## 🚫 What NOT to Do

- ❌ Trading operations (too complex, not demo-able)
- ❌ Web UI (wrong track, time sink)
- ❌ Python bindings (nice but not critical)
- ❌ Machine learning (irrelevant)
- ❌ Perfect documentation (good enough is fine)
- ❌ Full test coverage (a few tests is OK)

---

## 🎯 Success Definition

**You win if:**

1. ✅ SDK connects to Kraken and streams data
2. ✅ Strategy engine fires alerts in real-time
3. ✅ Dashboard shows performance metrics
4. ✅ Order book validates checksums
5. ✅ 60-second demo video is compelling
6. ✅ README makes it easy to understand

**Judges remember:**
- The alert firing in real-time
- The performance numbers
- The clean architecture diagram
- How quickly they can get started

---

## 📊 Time Allocation Summary

| Day | Focus | Hours |
|-----|-------|-------|
| Day 1 AM | Foundation (WebSocket, JSON, queue) | 4-5h |
| Day 1 PM | Core features (ticker, book, thread model) | 4-5h |
| Day 1 Eve | Testing with live API | 2h |
| Day 2 AM | **Strategy Engine** | 4-5h |
| Day 2 PM | Dashboard, benchmark, reconnection | 3-4h |
| Day 2 Eve | Examples, README, demo video | 3-4h |

**Total: ~20-24 hours over 2 days**

---

**Focus. Execute. Ship.**
