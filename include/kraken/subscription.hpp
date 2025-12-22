/// @file subscription.hpp
/// @brief Subscription handle for managing WebSocket subscriptions
/// 
/// Provides a RAII-style handle for managing subscription lifecycle including
/// pause, resume, and dynamic symbol management.

#pragma once

#include "types.hpp"
#include <memory>
#include <vector>
#include <string>

namespace kraken {

// Forward declarations
class KrakenClient;
class SubscriptionImpl; // Internal implementation detail (defined in client_impl.hpp)

/// @brief Handle for managing subscription lifecycle
/// 
/// Provides a RAII-style handle for WebSocket subscriptions with lifecycle management.
/// Allows pausing, resuming, and dynamically adding/removing symbols.
/// 
/// @example
/// @code
/// auto sub = client.subscribe(Channel::Ticker, {"BTC/USD"});
/// sub.pause();  // Temporarily stop updates
/// sub.add_symbols({"ETH/USD"});  // Add more symbols
/// sub.resume();  // Resume updates
/// @endcode
class Subscription {
public:
    /// @brief Destructor - automatically unsubscribes if still active
    ~Subscription();
    
    // Move-only
    Subscription(Subscription&&) noexcept;
    Subscription& operator=(Subscription&&) noexcept;
    Subscription(const Subscription&) = delete;
    Subscription& operator=(const Subscription&) = delete;
    
    //--------------------------------------------------------------------------
    // Lifecycle Control
    //--------------------------------------------------------------------------
    
    /// @brief Pause receiving updates
    /// 
    /// Sends an unsubscribe message to the server but keeps the subscription
    /// handle active. Use resume() to re-enable updates.
    void pause();
    
    /// @brief Resume receiving updates
    /// 
    /// Re-subscribes to the channel and symbols after a pause().
    void resume();
    
    /// @brief Permanently unsubscribe and invalidate the handle
    /// 
    /// Sends unsubscribe message and marks the subscription as inactive.
    /// The handle becomes invalid after this call.
    void unsubscribe();
    
    //--------------------------------------------------------------------------
    // Dynamic Symbol Management
    //--------------------------------------------------------------------------
    
    /// @brief Add symbols to this subscription
    /// 
    /// Dynamically adds new trading pairs to an existing subscription.
    /// The new symbols are immediately subscribed to.
    /// 
    /// @param symbols List of trading pairs to add (e.g., {"ETH/USD", "LTC/USD"})
    void add_symbols(const std::vector<std::string>& symbols);
    
    /// @brief Remove symbols from this subscription
    /// 
    /// Removes trading pairs from the subscription. Unsubscribes from
    /// the removed symbols on the server.
    /// 
    /// @param symbols List of trading pairs to remove
    void remove_symbols(const std::vector<std::string>& symbols);
    
    //--------------------------------------------------------------------------
    // Query
    //--------------------------------------------------------------------------
    
    /// @brief Check if subscription is active
    /// @return true if subscription is active and not paused
    bool is_active() const;
    
    /// @brief Check if subscription is paused
    /// @return true if subscription is paused
    bool is_paused() const;
    
    /// @brief Get subscription channel
    /// @return The channel this subscription is for
    Channel channel() const;
    
    /// @brief Get currently subscribed symbols
    /// @return List of trading pairs currently subscribed
    std::vector<std::string> symbols() const;
    
    /// @brief Get subscription ID
    /// @return Unique subscription identifier
    int id() const;
    
private:
    friend class KrakenClient;
    
    class Impl;
    std::shared_ptr<Impl> impl_;
    
    explicit Subscription(std::shared_ptr<Impl> impl);
    
    // Factory function for creating Subscription::Impl
    // Forward declared - defined in subscription.cpp
    static std::shared_ptr<Impl> create_impl_from_internal(std::shared_ptr<SubscriptionImpl> internal_impl);
};

} // namespace kraken

