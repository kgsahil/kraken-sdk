#pragma once

#include "kraken/client.hpp"
#include "kraken/types.hpp"
#include "kraken/error.hpp"
#include "kraken/config.hpp"
#include "kraken/metrics.hpp"
#include "kraken/strategies.hpp"
#include "kraken/telemetry.hpp"

#include <rigtorp/SPSCQueue.h>

#include <thread>
#include <atomic>
#include <mutex>
#include <shared_mutex>
#include <condition_variable>
#include <unordered_map>
#include <memory>
#include <functional>
#include <variant>
#include <optional>

namespace kraken {

// Forward declarations
class Connection;
class SubscriptionImpl;

//------------------------------------------------------------------------------
// Internal Message Type (optimized with std::variant)
//------------------------------------------------------------------------------

enum class MessageType {
    Ticker,
    Trade,
    Book,
    OHLC,
    Error,
    Subscribed,
    Unsubscribed,
    Heartbeat
};

// Empty types for control messages
struct SubscribedMsg {};
struct UnsubscribedMsg {};
struct HeartbeatMsg {};

// Variant holding only one message type at a time (memory optimization)
using MessageData = std::variant<
    std::monostate,    // Default/empty state
    Ticker,
    Trade,
    OrderBook,
    OHLC,
    Error,
    SubscribedMsg,
    UnsubscribedMsg,
    HeartbeatMsg
>;

struct Message {
    MessageType type = MessageType::Heartbeat;
    MessageData data;
    std::chrono::steady_clock::time_point receive_time;
    
    // Gap detection fields
    std::string channel;        // e.g., "ticker", "book"
    std::string symbol;         // e.g., "BTC/USD"
    uint64_t sequence = 0;      // Sequence number from Kraken (0 = not present)
    bool has_sequence = false;  // Whether message had a sequence number
    
    // Helper accessors with compile-time safety
    template<typename T>
    const T& get() const { return std::get<T>(data); }
    
    template<typename T>
    T& get() { return std::get<T>(data); }
    
    template<typename T>
    bool holds() const { return std::holds_alternative<T>(data); }
};

//------------------------------------------------------------------------------
// Strategy Engine
//------------------------------------------------------------------------------

class StrategyEngine {
public:
    int add(std::shared_ptr<AlertStrategy> strategy, AlertCallback callback);
    void remove(int id);
    void evaluate(const Ticker& ticker);
    size_t count() const;
    
private:
    struct Entry {
        std::shared_ptr<AlertStrategy> strategy;
        AlertCallback callback;
    };
    
    std::unordered_map<int, Entry> strategies_;
    mutable std::mutex mutex_;
    std::atomic<int> next_id_{1};
};

//------------------------------------------------------------------------------
// Client Implementation
//------------------------------------------------------------------------------

class KrakenClient::Impl {
public:
    explicit Impl(ClientConfig config);
    ~Impl();
    
    // Non-copyable, non-movable (owns threads)
    Impl(const Impl&) = delete;
    Impl& operator=(const Impl&) = delete;
    Impl(Impl&&) = delete;
    Impl& operator=(Impl&&) = delete;
    
    //--- Callbacks ---
    void on_ticker(TickerCallback callback);
    void on_trade(TradeCallback callback);
    void on_book(BookCallback callback);
    void on_ohlc(OHLCCallback callback);
    void on_error(ErrorCallback callback);
    void on_connection_state(ConnectionStateCallback callback);
    
    //--- Connection ---
    void connect();
    void disconnect();
    bool is_connected() const;
    ConnectionState connection_state() const;
    
    //--- Subscriptions ---
    Subscription subscribe(Channel channel, const std::vector<std::string>& symbols);
    Subscription subscribe_book(const std::vector<std::string>& symbols, int depth);
    void send_subscribe(Channel channel, const std::vector<std::string>& symbols, int depth = 0);
    void send_unsubscribe(Channel channel, const std::vector<std::string>& symbols);
    
    //--- Strategies ---
    int add_alert(std::shared_ptr<AlertStrategy> strategy, AlertCallback callback);
    void remove_alert(int alert_id);
    size_t alert_count() const;
    
    //--- Event Loop ---
    void run();
    void run_async();
    void stop();
    bool is_running() const;
    
    //--- Metrics ---
    Metrics get_metrics() const;
    
    //--- Data Snapshots ---
    std::optional<Ticker> latest_ticker(const std::string& symbol) const;
    std::optional<OrderBook> latest_book(const std::string& symbol) const;
    std::unordered_map<std::string, Ticker> all_tickers() const;
    
    //--- Gap Detection ---
    uint64_t gap_count() const;
    
private:
    void io_loop();
    void dispatcher_loop();
    void dispatch(Message& msg);
    void set_connection_state(ConnectionState state);
    void handle_reconnect();
    void send_pending_subscriptions();
    
    // Helper methods to reduce code duplication
    void safe_invoke_error_callback(ErrorCode code, const std::string& message, 
                                     const std::string& details = "");
    void safe_send_message(const std::string& message);
    
    // Template helper for safe callback invocation (defined inline)
    template<typename Callback, typename... Args>
    void safe_invoke_callback(Callback&& callback, Args&&... args) {
        try {
            callback(std::forward<Args>(args)...);
        } catch (const std::exception& e) {
            // Callback exception - notify via error callback
            safe_invoke_error_callback(ErrorCode::CallbackError, 
                                      std::string("Callback exception: ") + e.what(), "");
        }
    }
    
    // Configuration
    ClientConfig config_;
    
    // Connection
    std::unique_ptr<Connection> connection_;
    std::atomic<ConnectionState> state_{ConnectionState::Disconnected};
    
    // Threading
    std::thread io_thread_;
    std::thread dispatcher_thread_;
    std::atomic<bool> running_{false};
    std::atomic<bool> stop_requested_{false};
    
    // Message queue with condition variable for efficient wake-up
    std::unique_ptr<rigtorp::SPSCQueue<Message>> queue_;
    std::mutex queue_cv_mutex_;
    std::condition_variable queue_cv_;
    
    // Callbacks (protected by shared_mutex for read-heavy access)
    mutable std::shared_mutex callbacks_mutex_;
    TickerCallback ticker_callback_;
    TradeCallback trade_callback_;
    BookCallback book_callback_;
    OHLCCallback ohlc_callback_;
    ErrorCallback error_callback_;
    ConnectionStateCallback state_callback_;
    
    // Subscriptions
    std::unordered_map<int, std::shared_ptr<SubscriptionImpl>> subscriptions_;
    mutable std::mutex subscriptions_mutex_;
    std::atomic<int> next_sub_id_{1};
    
    // Strategies
    StrategyEngine strategy_engine_;
    
    // Metrics - use atomics for hot-path counters (no mutex needed)
    std::atomic<uint64_t> msg_received_{0};
    std::atomic<uint64_t> msg_processed_{0};
    std::atomic<uint64_t> msg_dropped_{0};
    std::atomic<int64_t> latency_max_us_{0};
    std::chrono::steady_clock::time_point start_time_;
    
    // Reconnection with exponential backoff
    std::unique_ptr<BackoffStrategy> backoff_strategy_;
    
    // Gap detection
    SequenceTracker gap_tracker_;
    
    // Telemetry (optional, created if config has telemetry enabled)
    std::shared_ptr<Telemetry> telemetry_;
    
    // Latest data snapshots (thread-safe)
    mutable std::shared_mutex snapshots_mutex_;
    std::unordered_map<std::string, Ticker> latest_tickers_;
    std::unordered_map<std::string, OrderBook> latest_books_;
    
    // Order book state (for incremental updates)
    std::unordered_map<std::string, OrderBook> order_books_;
    std::mutex books_mutex_;
};

//------------------------------------------------------------------------------
// Subscription Implementation - uses type-safe callbacks
//------------------------------------------------------------------------------

class SubscriptionImpl {
public:
    // Type-safe callbacks for subscribe/unsubscribe operations
    using SubscribeFn = std::function<void(Channel, const std::vector<std::string>&, int)>;
    using UnsubscribeFn = std::function<void(Channel, const std::vector<std::string>&)>;
    
    SubscriptionImpl(int id, Channel channel, std::vector<std::string> symbols,
                     SubscribeFn on_subscribe, UnsubscribeFn on_unsubscribe);
    
    void pause();
    void resume();
    void unsubscribe();
    void add_symbols(const std::vector<std::string>& symbols);
    void remove_symbols(const std::vector<std::string>& symbols);
    
    bool is_active() const { return active_; }
    bool is_paused() const { return paused_; }
    Channel channel() const { return channel_; }
    std::vector<std::string> symbols() const;
    int id() const { return id_; }
    int depth() const { return depth_; }
    void set_depth(int d) { depth_ = d; }
    
private:
    int id_;
    Channel channel_;
    std::vector<std::string> symbols_;
    int depth_ = 10;
    bool active_ = true;
    bool paused_ = false;
    mutable std::mutex mutex_;
    
    // Type-safe callbacks - no void* needed
    SubscribeFn subscribe_fn_;
    UnsubscribeFn unsubscribe_fn_;
};

} // namespace kraken

