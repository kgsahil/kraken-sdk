/// @file dispatch.cpp
/// @brief Message dispatch and I/O loops

#include "internal/client_impl.hpp"
#include "internal/parser.hpp"
#include "internal/connection.hpp"
#include "kraken/strategies/base.hpp"
#include "kraken/core/types.hpp"
#include "kraken/telemetry.hpp"

namespace kraken {

void KrakenClient::Impl::io_loop() { // NOLINT(readability-function-cognitive-complexity)
    while (!stop_requested_) {
        if (!connection_ || !connection_->is_open()) {
            if (!stop_requested_) {
                handle_reconnect();
            }
            continue;
        }
        
        try {
            // Read message from WebSocket
            std::string raw_json = connection_->receive();
            if (raw_json.empty()) continue;
            
            // Record receive time
            auto recv_time = std::chrono::steady_clock::now();
            
            // Parse message
            Message msg = parse_message(raw_json);
            msg.receive_time = recv_time;
            
            // Update metrics (lock-free)
            msg_received_.fetch_add(1, std::memory_order_relaxed);
            
            // Update telemetry if enabled
            if (telemetry_) {
                std::string channel = to_string(static_cast<Channel>(msg.type));
                telemetry_->metrics().increment_messages_received(channel);
            }
            
            // Filter heartbeats BEFORE the queue — saves SPSC slots for real data
            if (msg.type == MessageType::Heartbeat) {
                heartbeats_received_.fetch_add(1, std::memory_order_relaxed);
                last_heartbeat_time_.store(
                    recv_time.time_since_epoch().count(),
                    std::memory_order_relaxed);
                continue;
            }
            
            // Handle subscription acks in I/O thread (don't waste queue slots)
            if (msg.type == MessageType::Subscribed || 
                msg.type == MessageType::Unsubscribed) {
                dispatch(msg);
                continue;
            }
            
            // Process message: either via queue or directly
            if (queue_) {
                // Queue mode: push to queue for dispatcher thread
                if (queue_->try_push(std::move(msg))) {
                    // Notify dispatcher thread (efficient wake-up)
                    queue_cv_.notify_one();
                } else {
                    // Queue full - drop message (lock-free)
                    msg_dropped_.fetch_add(1, std::memory_order_relaxed);
                    
                    // Update telemetry if enabled
                    if (telemetry_) {
                        telemetry_->metrics().increment_messages_dropped();
                    }
                    
                    // Notify via error callback
                    safe_invoke_error_callback(ErrorCode::QueueOverflow, 
                                               "Message queue overflow", "");
                }
            } else {
                // Direct mode: process immediately in I/O thread
                // Check for gaps in sequence numbers
                if (msg.has_sequence) {
                    gap_tracker_.check(msg.channel, msg.symbol, msg.sequence);
                }
                
                // Dispatch directly (blocks I/O during callback execution)
                dispatch(msg);
                
                // Update metrics (lock-free atomics)
                msg_processed_.fetch_add(1, std::memory_order_relaxed);
                
                // Track latency
                auto now = std::chrono::steady_clock::now();
                auto latency_us = std::chrono::duration_cast<std::chrono::microseconds>(
                    now - msg.receive_time
                ).count();
                
                int64_t current_max = latency_max_us_.load(std::memory_order_relaxed);
                while (latency_us > current_max) {
                    if (latency_max_us_.compare_exchange_weak(current_max, latency_us,
                            std::memory_order_relaxed)) {
                        break;
                    }
                }
                
                // Update telemetry if enabled
                if (telemetry_) {
                    telemetry_->metrics().increment_messages_processed();
                    telemetry_->metrics().record_latency_us(latency_us);
                }
            }
            
        } catch (const std::exception& e) {
            if (!stop_requested_) {
                // Log error and try to reconnect
                safe_invoke_error_callback(ErrorCode::ConnectionClosed, e.what(), "");
                handle_reconnect();
            }
        }
    }
}

void KrakenClient::Impl::dispatcher_loop() {
    // Only run if queue is enabled
    if (!queue_) return;
    
    while (!stop_requested_) {
        Message* msg = queue_->front();
        
        // Update queue depth in telemetry if enabled
        if (telemetry_ && queue_) {
            telemetry_->metrics().set_queue_depth(queue_->size());
        }
        
        if (msg) {
            // Capture receive time before pop (msg may be invalidated)
            auto recv_time = msg->receive_time;
            
            // Check for gaps in sequence numbers
            if (msg->has_sequence) {
                gap_tracker_.check(msg->channel, msg->symbol, msg->sequence);
            }
            
            dispatch(*msg);
            queue_->pop();
            
            // Update metrics (lock-free atomics)
            msg_processed_.fetch_add(1, std::memory_order_relaxed);
            
            // Track max latency (lock-free compare-exchange)
            auto now = std::chrono::steady_clock::now();
            auto latency_us = std::chrono::duration_cast<std::chrono::microseconds>(
                now - recv_time
            ).count();
            
            int64_t current_max = latency_max_us_.load(std::memory_order_relaxed);
            while (latency_us > current_max) {
                if (latency_max_us_.compare_exchange_weak(current_max, latency_us,
                        std::memory_order_relaxed)) {
                    break;
                }
            }
            
            // Update telemetry if enabled
            if (telemetry_) {
                telemetry_->metrics().increment_messages_processed();
                telemetry_->metrics().record_latency_us(latency_us);
            }
        } else {
            // Queue empty - wait efficiently with condition variable
            // Use unbounded wait for minimal latency (stop_requested_ will wake us)
            std::unique_lock<std::mutex> lock(queue_cv_mutex_);
            queue_cv_.wait(lock, [this] {
                return queue_->front() != nullptr || stop_requested_.load(std::memory_order_acquire);
            });
        }
    }
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
void KrakenClient::Impl::dispatch(Message& msg) {
    // Dispatch to callbacks under shared lock
    {
        std::shared_lock lock(callbacks_mutex_);
        
        switch (msg.type) {
            case MessageType::Ticker:
                if (msg.holds<Ticker>()) {
                    const auto& ticker = msg.get<Ticker>();
                    
                    // Store latest snapshot (for latest_ticker())
                    {
                        std::unique_lock snap_lock(snapshots_mutex_);
                        latest_tickers_[ticker.symbol] = ticker;
                    }
                    
                    // Invoke callback
                    if (ticker_callback_) {
                        safe_invoke_callback(ticker_callback_, ticker);
                    }
                }
                break;
                
            case MessageType::Trade:
                if (trade_callback_ && msg.holds<Trade>()) {
                    safe_invoke_callback(trade_callback_, msg.get<Trade>());
                }
                break;
                
            case MessageType::Book:
                if (msg.holds<OrderBook>()) {
                    const auto& book = msg.get<OrderBook>();
                    
                    // Track checksum failures in telemetry
                    if (telemetry_ && !book.is_valid) {
                        telemetry_->metrics().increment_checksum_failures();
                    }
                    
                    // Store latest snapshot (for latest_book())
                    {
                        std::unique_lock snap_lock(snapshots_mutex_);
                        latest_books_[book.symbol] = book;
                    }
                    
                    // Invoke callback
                    if (book_callback_) {
                        safe_invoke_callback(book_callback_, book.symbol, book);
                    }
                }
                break;
                
            case MessageType::OHLC:
                if (ohlc_callback_ && msg.holds<OHLC>()) {
                    safe_invoke_callback(ohlc_callback_, msg.get<OHLC>());
                }
                break;
                
            case MessageType::Order:
                if (order_callback_ && msg.holds<Order>()) {
                    safe_invoke_callback(order_callback_, msg.get<Order>());
                }
                break;
                
            case MessageType::OwnTrade:
                if (own_trade_callback_ && msg.holds<OwnTrade>()) {
                    safe_invoke_callback(own_trade_callback_, msg.get<OwnTrade>());
                }
                break;
                
            case MessageType::Balance:
                if (balance_callback_ && msg.holds<std::unordered_map<std::string, Balance>>()) {
                    safe_invoke_callback(balance_callback_, msg.get<std::unordered_map<std::string, Balance>>());
                }
                break;
                
            case MessageType::Error:
                if (error_callback_ && msg.holds<Error>()) {
                    // Error callback itself - don't use safe_invoke_callback to avoid recursion
                    try {
                        error_callback_(msg.get<Error>());
                    } catch (...) {
                        // Error callback threw - ignore to prevent crash
                    }
                }
                break;
                
            case MessageType::Subscribed:
                if (msg.holds<SubscribedMsg>()) {
                    const auto& sub_msg = msg.get<SubscribedMsg>();
                    
                    // Update matching subscription's confirmed state
                    {
                        std::lock_guard<std::mutex> sub_lock(subscriptions_mutex_);
                        for (auto& [id, sub] : subscriptions_) {
                            if (sub->is_active() && !sub->is_confirmed() &&
                                to_string(sub->channel()) == sub_msg.channel) {
                                sub->set_confirmed(true);
                                break;  // One ack per message
                            }
                        }
                    }
                    
                    // Invoke user callback if set
                    if (subscribed_callback_) {
                        safe_invoke_callback(subscribed_callback_, 
                                           sub_msg.channel, sub_msg.symbols);
                    }
                }
                break;
                
            case MessageType::Unsubscribed:
                if (msg.holds<UnsubscribedMsg>()) {
                    const auto& unsub_msg = msg.get<UnsubscribedMsg>();
                    
                    // Update matching subscription's confirmed state
                    {
                        std::lock_guard<std::mutex> sub_lock(subscriptions_mutex_);
                        for (auto& [id, sub] : subscriptions_) {
                            if (sub->is_confirmed() &&
                                to_string(sub->channel()) == unsub_msg.channel) {
                                sub->set_confirmed(false);
                                break;
                            }
                        }
                    }
                }
                break;
                
            default:
                break;
        }
    }
    
    // Evaluate strategies OUTSIDE the lock to avoid blocking callbacks
    try {
        if (msg.type == MessageType::Ticker && msg.holds<Ticker>()) {
            const auto& ticker = msg.get<Ticker>();
            strategy_engine_.evaluate(ticker);
            
            // Also check if any strategy needs both ticker and order book
            // (if we have latest order book for this symbol)
            {
                std::shared_lock snap_lock(snapshots_mutex_);
                auto it = latest_books_.find(ticker.symbol);
                if (it != latest_books_.end()) {
                    strategy_engine_.evaluate(ticker, it->second);
                }
            }
        } else if (msg.type == MessageType::Book && msg.holds<OrderBook>()) {
            const auto& book = msg.get<OrderBook>();
            strategy_engine_.evaluate(book);
            
            // Also check if we have latest ticker for multi-source strategies
            {
                std::shared_lock snap_lock(snapshots_mutex_);
                auto it = latest_tickers_.find(book.symbol);
                if (it != latest_tickers_.end()) {
                    strategy_engine_.evaluate(it->second, book);
                }
            }
        } else if (msg.type == MessageType::Trade && msg.holds<Trade>()) {
            strategy_engine_.evaluate(msg.get<Trade>());
        } else if (msg.type == MessageType::OHLC && msg.holds<OHLC>()) {
            strategy_engine_.evaluate(msg.get<OHLC>());
        }
    } catch (const std::exception& e) {
        // Strategy evaluation exception - notify via error callback
        safe_invoke_error_callback(ErrorCode::CallbackError, 
                                  std::string("Strategy evaluation exception: ") + e.what(), "");
    }
}

} // namespace kraken

