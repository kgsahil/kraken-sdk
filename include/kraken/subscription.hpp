#pragma once

#include "types.hpp"
#include <memory>
#include <vector>
#include <string>

namespace kraken {

// Forward declarations
class KrakenClient;
class SubscriptionImpl; // Internal implementation detail (defined in client_impl.hpp)

/// Handle for managing subscription lifecycle
class Subscription {
public:
    ~Subscription();
    
    // Move-only
    Subscription(Subscription&&) noexcept;
    Subscription& operator=(Subscription&&) noexcept;
    Subscription(const Subscription&) = delete;
    Subscription& operator=(const Subscription&) = delete;
    
    //--------------------------------------------------------------------------
    // Lifecycle Control
    //--------------------------------------------------------------------------
    
    /// Pause receiving updates (sends unsubscribe)
    void pause();
    
    /// Resume receiving updates (sends subscribe)
    void resume();
    
    /// Permanently unsubscribe
    void unsubscribe();
    
    //--------------------------------------------------------------------------
    // Dynamic Symbol Management
    //--------------------------------------------------------------------------
    
    /// Add symbols to this subscription
    void add_symbols(const std::vector<std::string>& symbols);
    
    /// Remove symbols from this subscription
    void remove_symbols(const std::vector<std::string>& symbols);
    
    //--------------------------------------------------------------------------
    // Query
    //--------------------------------------------------------------------------
    
    /// Check if subscription is active
    bool is_active() const;
    
    /// Check if subscription is paused
    bool is_paused() const;
    
    /// Get subscription channel
    Channel channel() const;
    
    /// Get currently subscribed symbols
    std::vector<std::string> symbols() const;
    
    /// Get subscription ID
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

