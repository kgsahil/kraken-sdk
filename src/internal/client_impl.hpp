/// @file client_impl.hpp
/// @brief PIMPL implementation of KrakenClient
/// 
/// Contains all implementation details for KrakenClient including WebSocket
/// connection management, message parsing, order book state, strategies,
/// threading, and metrics.
/// 
/// @note This is an internal header. Users interact via KrakenClient.

#pragma once

#include "kraken/client.hpp"
#include "kraken/types.hpp"
#include "kraken/error.hpp"
#include "kraken/config.hpp"
#include "kraken/metrics.hpp"
#include "kraken/strategies.hpp"
#include "kraken/telemetry.hpp"

#include "kraken/queue.hpp"

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

// Forward declarations (defined in internal/)
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

/// @brief Strategy engine for evaluating alert strategies
/// 
/// Manages multiple alert strategies and evaluates them on ticker updates.
/// Thread-safe for concurrent strategy addition/removal and evaluation.
/// 
/// @note This is an internal class. Users add strategies via KrakenClient::add_alert().
class StrategyEngine {
public:
    /// @brief Add a strategy with callback
    /// @param strategy Strategy to evaluate
    /// @param callback Callback to invoke when strategy triggers
    /// @return Strategy ID (for removal)
    int add(std::shared_ptr<AlertStrategy> strategy, AlertCallback callback);
    
    /// @brief Remove a strategy by ID
    /// @param id Strategy ID returned from add()
    void remove(int id);
    
    /// @brief Evaluate all strategies on a ticker update
    /// @param ticker Current ticker data
    void evaluate(const Ticker& ticker);
    
    /// @brief Evaluate all strategies on an order book update
    /// @param book Current order book data
    void evaluate(const OrderBook& book);
    
    /// @brief Evaluate all strategies on a trade update
    /// @param trade Recent trade data
    void evaluate(const Trade& trade);
    
    /// @brief Evaluate all strategies with both ticker and order book
    /// @param ticker Current ticker data
    /// @param book Current order book data
    void evaluate(const Ticker& ticker, const OrderBook& book);
    
    /// @brief Get number of active strategies
    /// @return Strategy count
    size_t count() const;
    
    std::vector<std::pair<int, std::string>> get_alerts() const;
    
    /// @brief Enable a strategy by ID
    /// @param id Strategy ID
    void enable(int id);
    
    /// @brief Disable a strategy by ID (without removing)
    /// @param id Strategy ID
    void disable(int id);
    
    /// @brief Check if strategy is enabled
    /// @param id Strategy ID
    /// @return true if enabled
    bool is_enabled(int id) const;
    
    /// @brief Evaluate all strategies on an OHLC update
    /// @param ohlc Current OHLC candle data
    void evaluate(const OHLC& ohlc);
    
private:
    struct Entry {
        std::shared_ptr<AlertStrategy> strategy;
        AlertCallback callback;
        bool enabled = true;  // Runtime enable/disable flag
    };
    
    std::unordered_map<int, Entry> strategies_;
    mutable std::mutex mutex_;
    std::atomic<int> next_id_{1};
};

//------------------------------------------------------------------------------
// Client Implementation (PIMPL)
//------------------------------------------------------------------------------

/// @brief PIMPL implementation of KrakenClient
/// 
/// Contains all implementation details hidden from the public API.
/// Manages WebSocket connection, message parsing, order book state, strategies,
/// and threading (I/O thread + dispatcher thread with SPSC queue).
/// 
/// @note This is an internal class. Users interact via KrakenClient.
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
    void enable_alert(int alert_id);
    void disable_alert(int alert_id);
    bool is_alert_enabled(int alert_id) const;
    size_t alert_count() const;
    std::vector<std::pair<int, std::string>> get_alerts() const;
    
    //--- Event Loop ---
    void run();
    void run_async();
    void stop();
    bool is_running() const;
    
    //--- Metrics ---
    Metrics get_metrics() const;
    std::shared_ptr<Telemetry> get_telemetry_instance() const;
    
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
    std::unique_ptr<MessageQueue<Message>> queue_;
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

/// @brief Internal subscription implementation
/// 
/// Manages subscription state and lifecycle. Uses type-safe callbacks
/// to communicate with KrakenClient::Impl for subscribe/unsubscribe operations.
/// 
/// @note This is an internal class. Users interact via Subscription handle.
class SubscriptionImpl {
public:
    /// @brief Type-safe callback for subscribe operations
    using SubscribeFn = std::function<void(Channel, const std::vector<std::string>&, int)>;
    
    /// @brief Type-safe callback for unsubscribe operations
    using UnsubscribeFn = std::function<void(Channel, const std::vector<std::string>&)>;
    
    /// @brief Construct subscription
    /// @param id Unique subscription ID
    /// @param channel Channel to subscribe to
    /// @param symbols Initial trading pairs
    /// @param on_subscribe Callback to trigger subscribe
    /// @param on_unsubscribe Callback to trigger unsubscribe
    SubscriptionImpl(int id, Channel channel, std::vector<std::string> symbols,
                     SubscribeFn on_subscribe, UnsubscribeFn on_unsubscribe);
    
    /// @brief Pause subscription (unsubscribe but keep handle)
    void pause();
    
    /// @brief Resume subscription (re-subscribe)
    void resume();
    
    /// @brief Unsubscribe and invalidate handle
    void unsubscribe();
    
    /// @brief Add symbols to subscription
    /// @param symbols Trading pairs to add
    void add_symbols(const std::vector<std::string>& symbols);
    
    /// @brief Remove symbols from subscription
    /// @param symbols Trading pairs to remove
    void remove_symbols(const std::vector<std::string>& symbols);
    
    /// @brief Check if subscription is active
    bool is_active() const { return active_; }
    
    /// @brief Check if subscription is paused
    bool is_paused() const { return paused_; }
    
    /// @brief Get subscription channel
    Channel channel() const { return channel_; }
    
    /// @brief Get subscribed symbols
    std::vector<std::string> symbols() const;
    
    /// @brief Get subscription ID
    int id() const { return id_; }
    
    /// @brief Get order book depth (for book channel)
    int depth() const { return depth_; }
    
    /// @brief Set order book depth
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

