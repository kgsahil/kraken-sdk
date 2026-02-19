/// @file client.hpp
/// @brief Main Kraken WebSocket API client
/// 
/// Provides the primary KrakenClient interface for connecting to Kraken,
/// subscribing to channels, managing strategies, and handling callbacks.

#pragma once

#include "types.hpp"
#include "error.hpp"
#include "config.hpp"
#include "../metrics.hpp"
#include "../subscription.hpp"
#include "../strategies/base.hpp"

#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>
#include <string>
#include <cstddef>
#include <utility>

namespace kraken {

struct Telemetry;

/**
 * @brief Kraken WebSocket API client
 * 
 * A high-performance SDK for streaming real-time market data from Kraken
 * with built-in trading strategies and performance monitoring.
 * 
 * Thread-safe for:
 * - Callback registration (on_ticker, on_error, etc.)
 * - Subscriptions (subscribe, subscribe_book)
 * - Alert strategies (add_alert, remove_alert)
 * - Connection state queries (is_connected)
 * - Metrics (get_metrics)
 * 
 * @example
 * @code
 * kraken::KrakenClient client;
 * client.on_ticker([](const auto& t) { std::cout << t.last << std::endl; });
 * client.subscribe(kraken::Channel::Ticker, {"BTC/USD"});
 * client.run();
 * @endcode
 */
class KrakenClient {
public:
    //--------------------------------------------------------------------------
    // Construction
    //--------------------------------------------------------------------------
    
    /// Create client with default configuration
    KrakenClient();
    
    /// Create client with custom configuration
    /// @throws std::invalid_argument if config is invalid
    explicit KrakenClient(ClientConfig config);
    
    /// Destructor - stops event loop, closes connection
    ~KrakenClient();
    
    // Non-copyable
    KrakenClient(const KrakenClient&) = delete;
    KrakenClient& operator=(const KrakenClient&) = delete;
    
    // Movable
    KrakenClient(KrakenClient&&) noexcept;
    KrakenClient& operator=(KrakenClient&&) noexcept;
    
    //--------------------------------------------------------------------------
    // Callback Registration (thread-safe)
    //--------------------------------------------------------------------------
    
    /// Set ticker update callback
    void on_ticker(TickerCallback callback);
    
    /// Set trade update callback
    void on_trade(TradeCallback callback);
    
    /// Set order book update callback
    void on_book(BookCallback callback);
    
    /// Set OHLC update callback
    void on_ohlc(OHLCCallback callback);
    
    /// Set order update callback (private channel - requires authentication)
    /// @note Requires API key/secret to be configured
    void on_order(OrderCallback callback);
    
    /// Set own trade callback (private channel - requires authentication)
    /// @note Requires API key/secret to be configured
    void on_own_trade(OwnTradeCallback callback);
    
    /// Set balance update callback (private channel - requires authentication)
    /// @note Requires API key/secret to be configured
    void on_balance(BalanceCallback callback);
    
    /// Set error callback (for runtime errors)
    void on_error(ErrorCallback callback);
    
    /// Set connection state callback
    void on_connection_state(ConnectionStateCallback callback);
    
    /// Set subscription confirmation callback
    /// @param callback Called when server confirms a subscription
    ///                  with channel name and subscribed symbols
    void on_subscribed(SubscribedCallback callback);
    
    //--------------------------------------------------------------------------
    // Connection
    //--------------------------------------------------------------------------
    
    /// Connect to Kraken WebSocket
    /// @throws ConnectionError if connection fails
    void connect();
    
    /// Disconnect from Kraken
    void disconnect();
    
    /// Check if connected
    bool is_connected() const;
    
    /// Get current connection state
    ConnectionState connection_state() const;
    
    //--------------------------------------------------------------------------
    // Subscriptions
    //--------------------------------------------------------------------------
    
    /// Subscribe to a channel
    /// @param channel The channel to subscribe to
    /// @param symbols List of trading pairs (e.g., "BTC/USD")
    /// @return Subscription handle for lifecycle management
    /// @throws std::invalid_argument if symbols is empty
    Subscription subscribe(Channel channel, const std::vector<std::string>& symbols);
    
    /// Subscribe to order book with specific depth
    /// @param symbols List of trading pairs
    /// @param depth Order book depth (10, 25, 100, 500, 1000)
    /// @return Subscription handle
    Subscription subscribe_book(const std::vector<std::string>& symbols, int depth = 10);  // NOLINT(readability-magic-numbers)
    
    /// Subscribe to own trades (private channel - requires authentication)
    /// @return Subscription handle
    /// @note Requires API key/secret to be configured
    Subscription subscribe_own_trades();
    
    /// Subscribe to open orders (private channel - requires authentication)
    /// @return Subscription handle
    /// @note Requires API key/secret to be configured
    Subscription subscribe_open_orders();
    
    /// Subscribe to balances (private channel - requires authentication)
    /// @return Subscription handle
    /// @note Requires API key/secret to be configured
    Subscription subscribe_balances();
    
    //--------------------------------------------------------------------------
    // Alert Strategies
    //--------------------------------------------------------------------------
    
    /// Add an alert strategy
    /// @param strategy The strategy to evaluate on ticker updates
    /// @param callback Called when strategy condition is met
    /// @return Alert ID (for removal)
    int add_alert(std::shared_ptr<AlertStrategy> strategy, AlertCallback callback);
    
    /// Remove an alert by ID
    void remove_alert(int alert_id);
    
    /// Get number of active alerts
    size_t alert_count() const;
    
    /// @brief Get alert information
    /// 
    /// Returns a list of active alert IDs and their strategy names.
    /// Useful for debugging and monitoring.
    /// 
    /// @return Vector of pairs (alert_id, strategy_name)
    std::vector<std::pair<int, std::string>> get_alerts() const;
    
    /// @brief Enable an alert strategy by ID (without removing)
    /// 
    /// Re-enables a previously disabled strategy. The strategy will
    /// start being evaluated again on market data updates.
    /// 
    /// @param alert_id Alert ID returned from add_alert()
    void enable_alert(int alert_id);
    
    /// @brief Disable an alert strategy by ID (without removing)
    /// 
    /// Temporarily disables a strategy. It remains registered but
    /// will not be evaluated until re-enabled.
    /// 
    /// @param alert_id Alert ID returned from add_alert()
    void disable_alert(int alert_id);
    
    /// @brief Check if an alert strategy is enabled
    /// 
    /// @param alert_id Alert ID returned from add_alert()
    /// @return true if the alert is enabled
    bool is_alert_enabled(int alert_id) const;
    
    //--------------------------------------------------------------------------
    // Event Loop
    //--------------------------------------------------------------------------
    
    /// Run event loop (blocking). Connects if not connected.
    void run();
    
    /// Run event loop in background thread
    void run_async();
    
    /// Stop event loop
    void stop();
    
    /// Check if event loop is running
    bool is_running() const;
    
    //--------------------------------------------------------------------------
    // Metrics
    //--------------------------------------------------------------------------
    
    /// Get current metrics snapshot
    Metrics get_metrics() const;
    
    //--------------------------------------------------------------------------
    // Telemetry
    //--------------------------------------------------------------------------
    
    /// Get the shared telemetry instance (for advanced usage like Prometheus scraping)
    /// @return Shared pointer to the Telemetry instance, or nullptr if not enabled
    std::shared_ptr<Telemetry> get_telemetry_instance() const;
    
    //--------------------------------------------------------------------------
    // Data Snapshots (thread-safe)
    //--------------------------------------------------------------------------
    
    /// Get the latest ticker for a symbol (thread-safe copy)
    /// @param symbol Trading pair (e.g., "BTC/USD")
    /// @return Latest ticker, or nullopt if not available
    std::optional<Ticker> latest_ticker(const std::string& symbol) const;
    
    /// Get the latest order book for a symbol (thread-safe copy)
    /// @param symbol Trading pair (e.g., "BTC/USD")
    /// @return Latest order book, or nullopt if not available
    std::optional<OrderBook> latest_book(const std::string& symbol) const;
    
    /// Get all tracked symbols with their latest tickers
    std::unordered_map<std::string, Ticker> all_tickers() const;
    
    //--------------------------------------------------------------------------
    // Gap Detection
    //--------------------------------------------------------------------------
    
    /// Get total number of gaps detected
    uint64_t gap_count() const;
    
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace kraken

