/// @file subscription_impl.cpp
/// @brief SubscriptionImpl implementation

#include "internal/client_impl.hpp"
#include <algorithm>

namespace kraken {

// Helper to safely invoke subscription callbacks
namespace {  // NOLINT(google-readability-namespace-comments)
    template<typename Fn, typename... Args>
    void safe_invoke_subscription_callback(Fn&& fn, Args&&... args) {
        if (fn) {
            try {
                fn(std::forward<Args>(args)...);
            } catch (...) {
                // Callback exception - ignore to prevent crash
            }
        }
    }
}

SubscriptionImpl::SubscriptionImpl(int id, Channel channel, 
                                   std::vector<std::string> symbols,
                                   SubscribeFn on_subscribe,
                                   UnsubscribeFn on_unsubscribe)
    : id_(id)
    , channel_(channel)
    , symbols_(std::move(symbols))
    , subscribe_fn_(std::move(on_subscribe))
    , unsubscribe_fn_(std::move(on_unsubscribe)) {}

void SubscriptionImpl::pause() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!active_ || paused_) return;
    paused_ = true;
    safe_invoke_subscription_callback(unsubscribe_fn_, channel_, symbols_);
}

void SubscriptionImpl::resume() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!active_ || !paused_) return;
    paused_ = false;
    safe_invoke_subscription_callback(subscribe_fn_, channel_, symbols_, depth_);
}

void SubscriptionImpl::unsubscribe() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!active_) return;
    active_ = false;
    paused_ = false;
    safe_invoke_subscription_callback(unsubscribe_fn_, channel_, symbols_);
}

void SubscriptionImpl::add_symbols(const std::vector<std::string>& new_symbols) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!active_) return;
    
    for (const auto& sym : new_symbols) {
        if (std::find(symbols_.begin(), symbols_.end(), sym) == symbols_.end()) {
            symbols_.push_back(sym);
        }
    }
    
    if (!paused_) {
        safe_invoke_subscription_callback(subscribe_fn_, channel_, new_symbols, depth_);
    }
}

void SubscriptionImpl::remove_symbols(const std::vector<std::string>& rem_symbols) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!active_) return;
    
    for (const auto& sym : rem_symbols) {
        symbols_.erase(
            std::remove(symbols_.begin(), symbols_.end(), sym),
            symbols_.end()
        );
    }
    
    if (!paused_) {
        safe_invoke_subscription_callback(unsubscribe_fn_, channel_, rem_symbols);
    }
}

std::vector<std::string> SubscriptionImpl::symbols() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return symbols_;
}

} // namespace kraken
