#pragma once

#include "types.hpp"
#include "error.hpp"
#include "config.hpp"
#include "metrics.hpp"
#include "subscription.hpp"
#include "strategies.hpp"

#include <memory>
#include <future>

namespace kraken {

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
    
    /// Set error callback (for runtime errors)
    void on_error(ErrorCallback callback);
    
    /// Set connection state callback
    void on_connection_state(ConnectionStateCallback callback);
    
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
    Subscription subscribe_book(const std::vector<std::string>& symbols, int depth = 10);
    
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
    
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace kraken

