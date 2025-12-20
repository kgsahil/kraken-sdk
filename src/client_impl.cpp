#include "client_impl.hpp"
#include "connection.hpp"
#include "parser.hpp"

#include <iostream>
#include <algorithm>

namespace kraken {

//------------------------------------------------------------------------------
// SubscriptionImpl - Clean implementation with type-safe callbacks
//------------------------------------------------------------------------------

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
    if (unsubscribe_fn_) {
        unsubscribe_fn_(channel_, symbols_);
    }
}

void SubscriptionImpl::resume() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!active_ || !paused_) return;
    paused_ = false;
    if (subscribe_fn_) {
        subscribe_fn_(channel_, symbols_, depth_);
    }
}

void SubscriptionImpl::unsubscribe() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!active_) return;
    active_ = false;
    paused_ = false;
    if (unsubscribe_fn_) {
        unsubscribe_fn_(channel_, symbols_);
    }
}

void SubscriptionImpl::add_symbols(const std::vector<std::string>& new_symbols) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!active_) return;
    
    for (const auto& sym : new_symbols) {
        if (std::find(symbols_.begin(), symbols_.end(), sym) == symbols_.end()) {
            symbols_.push_back(sym);
        }
    }
    
    if (!paused_ && subscribe_fn_) {
        subscribe_fn_(channel_, new_symbols, depth_);
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
    
    if (!paused_ && unsubscribe_fn_) {
        unsubscribe_fn_(channel_, rem_symbols);
    }
}

std::vector<std::string> SubscriptionImpl::symbols() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return symbols_;
}

//------------------------------------------------------------------------------
// StrategyEngine
//------------------------------------------------------------------------------

int StrategyEngine::add(std::shared_ptr<AlertStrategy> strategy, AlertCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    int id = next_id_++;
    strategies_[id] = {std::move(strategy), std::move(callback)};
    return id;
}

void StrategyEngine::remove(int id) {
    std::lock_guard<std::mutex> lock(mutex_);
    strategies_.erase(id);
}

void StrategyEngine::evaluate(const Ticker& ticker) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (auto& pair : strategies_) {
        auto& entry = pair.second;
        // Check if strategy applies to this symbol
        auto syms = entry.strategy->symbols();
        if (std::find(syms.begin(), syms.end(), ticker.symbol) == syms.end()) {
            continue;
        }
        
        // Check if condition is met
        if (entry.strategy->check(ticker)) {
            Alert alert;
            alert.strategy_name = entry.strategy->name();
            alert.symbol = ticker.symbol;
            alert.price = ticker.last;
            alert.message = "Strategy condition met";
            alert.timestamp = std::chrono::system_clock::now();
            
            // Fire callback
            entry.callback(alert);
        }
    }
}

size_t StrategyEngine::count() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return strategies_.size();
}

//------------------------------------------------------------------------------
// KrakenClient::Impl
//------------------------------------------------------------------------------

KrakenClient::Impl::Impl(ClientConfig config)
    : config_(std::move(config))
    , queue_(std::make_unique<rigtorp::SPSCQueue<Message>>(config_.queue_capacity()))
    , start_time_(std::chrono::steady_clock::now()) {
}

KrakenClient::Impl::~Impl() {
    stop();
}

//------------------------------------------------------------------------------
// Callbacks
//------------------------------------------------------------------------------

void KrakenClient::Impl::on_ticker(TickerCallback callback) {
    std::unique_lock lock(callbacks_mutex_);
    ticker_callback_ = std::move(callback);
}

void KrakenClient::Impl::on_trade(TradeCallback callback) {
    std::unique_lock lock(callbacks_mutex_);
    trade_callback_ = std::move(callback);
}

void KrakenClient::Impl::on_book(BookCallback callback) {
    std::unique_lock lock(callbacks_mutex_);
    book_callback_ = std::move(callback);
}

void KrakenClient::Impl::on_ohlc(OHLCCallback callback) {
    std::unique_lock lock(callbacks_mutex_);
    ohlc_callback_ = std::move(callback);
}

void KrakenClient::Impl::on_error(ErrorCallback callback) {
    std::unique_lock lock(callbacks_mutex_);
    error_callback_ = std::move(callback);
}

void KrakenClient::Impl::on_connection_state(ConnectionStateCallback callback) {
    std::unique_lock lock(callbacks_mutex_);
    state_callback_ = std::move(callback);
}

//------------------------------------------------------------------------------
// Connection
//------------------------------------------------------------------------------

void KrakenClient::Impl::connect() {
    if (state_ == ConnectionState::Connected || 
        state_ == ConnectionState::Connecting) {
        return;
    }
    
    set_connection_state(ConnectionState::Connecting);
    
    try {
        connection_ = std::make_unique<Connection>(config_.url());
        connection_->connect();
        set_connection_state(ConnectionState::Connected);
    } catch (const std::exception& e) {
        set_connection_state(ConnectionState::Disconnected);
        throw ConnectionError(std::string("Failed to connect: ") + e.what());
    }
}

void KrakenClient::Impl::disconnect() {
    if (connection_) {
        connection_->close();
        connection_.reset();
    }
    set_connection_state(ConnectionState::Disconnected);
}

bool KrakenClient::Impl::is_connected() const {
    return state_ == ConnectionState::Connected;
}

ConnectionState KrakenClient::Impl::connection_state() const {
    return state_;
}

void KrakenClient::Impl::set_connection_state(ConnectionState state) {
    state_.store(state, std::memory_order_relaxed);
    
    // Notify callback
    std::shared_lock lock(callbacks_mutex_);
    if (state_callback_) {
        state_callback_(state);
    }
}

//------------------------------------------------------------------------------
// Subscriptions
//------------------------------------------------------------------------------

Subscription KrakenClient::Impl::subscribe(Channel channel, 
                                            const std::vector<std::string>& symbols) {
    if (symbols.empty()) {
        throw std::invalid_argument("symbols cannot be empty");
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
    if (!connection_) return;
    std::string msg = build_subscribe_message(channel, symbols, depth);
    connection_->send(msg);
}

void KrakenClient::Impl::send_unsubscribe(Channel channel, 
                                           const std::vector<std::string>& symbols) {
    if (!connection_) return;
    std::string msg = build_unsubscribe_message(channel, symbols);
    connection_->send(msg);
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

//------------------------------------------------------------------------------
// Strategies
//------------------------------------------------------------------------------

int KrakenClient::Impl::add_alert(std::shared_ptr<AlertStrategy> strategy, 
                                   AlertCallback callback) {
    return strategy_engine_.add(std::move(strategy), std::move(callback));
}

void KrakenClient::Impl::remove_alert(int alert_id) {
    strategy_engine_.remove(alert_id);
}

size_t KrakenClient::Impl::alert_count() const {
    return strategy_engine_.count();
}

//------------------------------------------------------------------------------
// Event Loop
//------------------------------------------------------------------------------

void KrakenClient::Impl::run() {
    if (running_) return;
    
    // Connect if not connected
    if (!is_connected()) {
        connect();
    }
    
    // Send any pending subscriptions
    send_pending_subscriptions();
    
    running_ = true;
    stop_requested_ = false;
    
    // Start I/O thread
    io_thread_ = std::thread([this]() { io_loop(); });
    
    // Run dispatcher in current thread
    dispatcher_loop();
    
    // Wait for I/O thread to finish
    if (io_thread_.joinable()) {
        io_thread_.join();
    }
    
    running_ = false;
}

void KrakenClient::Impl::run_async() {
    if (running_) return;
    
    // Connect if not connected
    if (!is_connected()) {
        connect();
    }
    
    // Send any pending subscriptions
    send_pending_subscriptions();
    
    running_ = true;
    stop_requested_ = false;
    
    // Start I/O thread
    io_thread_ = std::thread([this]() { io_loop(); });
    
    // Start dispatcher thread
    dispatcher_thread_ = std::thread([this]() { dispatcher_loop(); });
}

void KrakenClient::Impl::stop() {
    if (!running_) return;
    
    stop_requested_ = true;
    
    // Wake up dispatcher thread if waiting on condition variable
    queue_cv_.notify_all();
    
    // Close connection to unblock I/O thread
    if (connection_) {
        connection_->close();
    }
    
    // Wait for threads
    if (io_thread_.joinable()) {
        io_thread_.join();
    }
    if (dispatcher_thread_.joinable()) {
        dispatcher_thread_.join();
    }
    
    running_ = false;
}

bool KrakenClient::Impl::is_running() const {
    return running_;
}

void KrakenClient::Impl::io_loop() {
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
            
            // Push to queue
            if (queue_->try_push(std::move(msg))) {
                // Notify dispatcher thread (efficient wake-up)
                queue_cv_.notify_one();
            } else {
                // Queue full - drop message (lock-free)
                msg_dropped_.fetch_add(1, std::memory_order_relaxed);
                
                // Notify via error callback
                std::shared_lock cb_lock(callbacks_mutex_);
                if (error_callback_) {
                    Error err{ErrorCode::QueueOverflow, "Message queue overflow", ""};
                    error_callback_(err);
                }
            }
            
        } catch (const std::exception& e) {
            if (!stop_requested_) {
                // Log error and try to reconnect
                std::shared_lock lock(callbacks_mutex_);
                if (error_callback_) {
                    Error err{ErrorCode::ConnectionClosed, e.what(), ""};
                    error_callback_(err);
                }
                handle_reconnect();
            }
        }
    }
}

void KrakenClient::Impl::dispatcher_loop() {
    while (!stop_requested_) {
        Message* msg = queue_->front();
        if (msg) {
            // Capture receive time before pop (msg may be invalidated)
            auto recv_time = msg->receive_time;
            
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
        } else {
            // Queue empty - wait efficiently with condition variable
            std::unique_lock<std::mutex> lock(queue_cv_mutex_);
            queue_cv_.wait_for(lock, std::chrono::milliseconds(10), [this] {
                return queue_->front() != nullptr || stop_requested_.load();
            });
        }
    }
}

void KrakenClient::Impl::dispatch(Message& msg) {
    // Dispatch to callbacks under shared lock
    {
        std::shared_lock lock(callbacks_mutex_);
        
        switch (msg.type) {
            case MessageType::Ticker:
                if (ticker_callback_ && msg.holds<Ticker>()) {
                    ticker_callback_(msg.get<Ticker>());
                }
                break;
                
            case MessageType::Trade:
                if (trade_callback_ && msg.holds<Trade>()) {
                    trade_callback_(msg.get<Trade>());
                }
                break;
                
            case MessageType::Book:
                if (book_callback_ && msg.holds<OrderBook>()) {
                    const auto& book = msg.get<OrderBook>();
                    book_callback_(book.symbol, book);
                }
                break;
                
            case MessageType::OHLC:
                if (ohlc_callback_ && msg.holds<OHLC>()) {
                    ohlc_callback_(msg.get<OHLC>());
                }
                break;
                
            case MessageType::Error:
                if (error_callback_ && msg.holds<Error>()) {
                    error_callback_(msg.get<Error>());
                }
                break;
                
            default:
                break;
        }
    }
    
    // Evaluate strategies OUTSIDE the lock to avoid blocking callbacks
    if (msg.type == MessageType::Ticker && msg.holds<Ticker>()) {
        strategy_engine_.evaluate(msg.get<Ticker>());
    }
}

void KrakenClient::Impl::handle_reconnect() {
    if (stop_requested_) return;
    
    set_connection_state(ConnectionState::Reconnecting);
    
    int attempts = 0;
    while (!stop_requested_ && attempts < config_.reconnect_attempts()) {
        attempts++;
        
        std::this_thread::sleep_for(config_.reconnect_delay());
        
        try {
            connection_ = std::make_unique<Connection>(config_.url());
            connection_->connect();
            set_connection_state(ConnectionState::Connected);
            
            // Resubscribe
            std::lock_guard<std::mutex> lock(subscriptions_mutex_);
            for (auto& pair : subscriptions_) {
                auto& sub = pair.second;
                if (sub->is_active() && !sub->is_paused()) {
                    send_subscribe(sub->channel(), sub->symbols(), sub->depth());
                }
            }
            
            return;
        } catch (const std::exception&) {
            // Continue trying
        }
    }
    
    // Failed to reconnect
    set_connection_state(ConnectionState::Disconnected);
}

//------------------------------------------------------------------------------
// Metrics
//------------------------------------------------------------------------------

Metrics KrakenClient::Impl::get_metrics() const {
    Metrics m;
    m.messages_received = msg_received_.load(std::memory_order_relaxed);
    m.messages_processed = msg_processed_.load(std::memory_order_relaxed);
    m.messages_dropped = msg_dropped_.load(std::memory_order_relaxed);
    m.queue_depth = queue_->size();
    m.connection_state = state_.load(std::memory_order_relaxed);
    m.latency_max_us = std::chrono::microseconds(
        latency_max_us_.load(std::memory_order_relaxed)
    );
    m.start_time = start_time_;
    return m;
}

} // namespace kraken

