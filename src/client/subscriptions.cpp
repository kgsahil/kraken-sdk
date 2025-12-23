/// @file subscriptions.cpp
/// @brief Subscription management

#include "internal/client_impl.hpp"
#include "internal/parser.hpp"
#include <algorithm>

namespace kraken {

namespace {
    bool symbols_valid(const std::vector<std::string>& symbols) {
        constexpr size_t kMaxSymbolLen = 256;
        for (const auto& sym : symbols) {
            if (sym.empty() || sym.size() > kMaxSymbolLen) {
                return false;
            }
        }
        return true;
    }
} // namespace

Subscription KrakenClient::Impl::subscribe(Channel channel, 
                                            const std::vector<std::string>& symbols) {
    if (symbols.empty()) {
        throw std::invalid_argument("symbols cannot be empty");
    }
    if (!symbols_valid(symbols)) {
        throw std::invalid_argument("symbols are invalid (empty or too long)");
    }
    
    int id = next_sub_id_++;
    
    // Create subscription with type-safe callbacks
    auto impl = std::make_shared<SubscriptionImpl>(
        id, channel, symbols,
        [this](Channel ch, const std::vector<std::string>& syms, int depth) {
            send_subscribe(ch, syms, depth);
        },
        [this](Channel ch, const std::vector<std::string>& syms) {
            send_unsubscribe(ch, syms);
        }
    );
    
    {
        std::lock_guard<std::mutex> lock(subscriptions_mutex_);
        subscriptions_[id] = impl;
    }
    
    send_subscribe(channel, symbols);
    return Subscription(Subscription::create_impl_from_internal(impl));
}

Subscription KrakenClient::Impl::subscribe_book(const std::vector<std::string>& symbols, 
                                                 int depth) {
    if (symbols.empty()) {
        throw std::invalid_argument("symbols cannot be empty");
    }
    if (!symbols_valid(symbols)) {
        throw std::invalid_argument("symbols are invalid (empty or too long)");
    }
    
    int id = next_sub_id_++;
    
    auto impl = std::make_shared<SubscriptionImpl>(
        id, Channel::Book, symbols,
        [this](Channel ch, const std::vector<std::string>& syms, int d) {
            send_subscribe(ch, syms, d);
        },
        [this](Channel ch, const std::vector<std::string>& syms) {
            send_unsubscribe(ch, syms);
        }
    );
    impl->set_depth(depth);
    
    {
        std::lock_guard<std::mutex> lock(subscriptions_mutex_);
        subscriptions_[id] = impl;
    }
    
    send_subscribe(Channel::Book, symbols, depth);
    return Subscription(Subscription::create_impl_from_internal(impl));
}

void KrakenClient::Impl::send_subscribe(Channel channel, 
                                         const std::vector<std::string>& symbols,
                                         int depth) {
    std::string msg = build_subscribe_message(channel, symbols, depth);
    safe_send_message(msg);
}

void KrakenClient::Impl::send_unsubscribe(Channel channel, 
                                           const std::vector<std::string>& symbols) {
    std::string msg = build_unsubscribe_message(channel, symbols);
    safe_send_message(msg);
}

void KrakenClient::Impl::send_pending_subscriptions() {
    std::lock_guard<std::mutex> lock(subscriptions_mutex_);
    for (auto& pair : subscriptions_) {
        auto& sub = pair.second;
        if (sub->is_active() && !sub->is_paused()) {
            send_subscribe(sub->channel(), sub->symbols(), sub->depth());
        }
    }
}

} // namespace kraken

