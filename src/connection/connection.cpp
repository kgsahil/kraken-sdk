#include "internal/connection.hpp"
#include "kraken/core/error.hpp"
#include "kraken/connection/connection_config.hpp"

#include <iostream>
#include <cctype>
#include <fstream>
#include <openssl/ssl.h>
#include <future>
#include <chrono>

namespace kraken {

Connection::Connection(const std::string& url, 
                       const ConnectionTimeouts& timeouts,
                       const SecurityConfig& security,
                       std::shared_ptr<RateLimiter> rate_limiter)
    : timeouts_(timeouts)
    , security_(security)
    , rate_limiter_(std::move(rate_limiter)) {
    parse_url(url);
    
    // Configure SSL context based on security settings
    if (security_.verify_peer) {
        ssl_ctx_.set_verify_mode(ssl::verify_peer);
    } else {
        ssl_ctx_.set_verify_mode(ssl::verify_none);
    }
    
    // Load CA certificates
    if (!security_.ca_cert_path.empty()) {
        // Load custom CA certificate
        ssl_ctx_.load_verify_file(security_.ca_cert_path);
    } else {
        // Load default system CA certificates
        ssl_ctx_.set_default_verify_paths();
    }
    
    // Load client certificate and key if provided
    if (!security_.client_cert_path.empty() && !security_.client_key_path.empty()) {
        ssl_ctx_.use_certificate_file(security_.client_cert_path, ssl::context::pem);
        ssl_ctx_.use_private_key_file(security_.client_key_path, ssl::context::pem);
    }
    
    // Set cipher suites if provided
    if (!security_.cipher_suites.empty()) {
        SSL_CTX_set_cipher_list(ssl_ctx_.native_handle(), security_.cipher_suites.c_str());
    }
}

Connection::~Connection() {
    close();
}

void Connection::parse_url(const std::string& url) {
    // Optimized URL parsing without std::regex (faster construction)
    // Parse WebSocket URL: wss://host:port/path or wss://host/path
    
    // Find scheme
    size_t scheme_end = url.find("://");
    if (scheme_end == std::string::npos) {
        throw std::invalid_argument("Invalid WebSocket URL: " + url);
    }
    
    std::string scheme = url.substr(0, scheme_end);
    // Convert to lowercase for comparison
    for (char& c : scheme) c = std::tolower(static_cast<unsigned char>(c));
    
    if (scheme != "wss" && scheme != "ws") {
        throw std::invalid_argument("Invalid WebSocket URL scheme: " + url);
    }
    
    size_t host_start = scheme_end + 3;  // Skip "://"
    
    // Find host end (either ':', '/', or end of string)
    size_t host_end = url.find_first_of(":/", host_start);
    if (host_end == std::string::npos) {
        host_end = url.length();
    }
    
    if (host_end == host_start) {
        throw std::invalid_argument("Invalid WebSocket URL: missing host: " + url);
    }
    
    host_ = url.substr(host_start, host_end - host_start);
    
    // Parse port if present
    if (host_end < url.length() && url[host_end] == ':') {
        size_t port_start = host_end + 1;
        size_t port_end = url.find('/', port_start);
        if (port_end == std::string::npos) {
            port_end = url.length();
        }
        port_ = url.substr(port_start, port_end - port_start);
        host_end = port_end;
    } else {
        port_ = (scheme == "wss") ? "443" : "80";
    }
    
    // Parse path
    if (host_end < url.length() && url[host_end] == '/') {
        path_ = url.substr(host_end);
    } else {
        path_ = "/";
    }
}

void Connection::connect() {
    try {
        // Create WebSocket stream
        ws_ = std::make_unique<stream_type>(ioc_, ssl_ctx_);
        
        // Resolve host
        tcp::resolver resolver(ioc_);
        auto results = resolver.resolve(host_, port_);
        
        // Connect TCP with timeout
        // Use async_connect + run_for to implement timeout
        std::promise<void> connect_promise;
        auto connect_future = connect_promise.get_future();
        
        // Run IO context for limited time
        // Use a small timeout for connection establishment
        auto timeout = timeouts_.connect_timeout > std::chrono::milliseconds(0) 
                     ? timeouts_.connect_timeout 
                     : std::chrono::seconds(10);
                     
        ioc_.restart();
        
        // Setup timeout timer
        net::steady_timer timer(ioc_, timeout);
        timer.async_wait([this](const beast::error_code& ec) {
            if (!ec) { // Timer expired
                beast::error_code ignored;
                if(ws_) beast::get_lowest_layer(*ws_).cancel(ignored);
            }
        });
        
        // Extend connect handler to cancel timer
        net::async_connect(
            beast::get_lowest_layer(*ws_),
            results,
            [&connect_promise, &timer](const beast::error_code& ec, const tcp::endpoint&) {
                timer.cancel(); // Cancel timeout
                if (ec && ec != net::error::operation_aborted) {
                     try {
                        throw ConnectionError(std::string("Connect failed: ") + ec.message());
                    } catch (...) {
                        connect_promise.set_exception(std::current_exception());
                    }
                } else if (!ec) {
                    connect_promise.set_value();
                }
            }
        );
        
        ioc_.run();

        // Check if connected
        if (connect_future.wait_for(std::chrono::seconds(0)) != std::future_status::ready) {
             throw ConnectionError("Connection validation failed (timeout or aborted)");
        }
        
        // Propagate exceptions
        connect_future.get();
        if (ioc_.stopped()) ioc_.restart(); // potential race if stopped

        // SNI hostname
        if (!SSL_set_tlsext_host_name(ws_->next_layer().native_handle(), 
                                       host_.c_str())) {
            throw ConnectionError("Failed to set SNI hostname");
        }
        
        // SSL handshake
        ws_->next_layer().handshake(ssl::stream_base::client);
        
        // Set WebSocket options
        ws_->set_option(websocket::stream_base::timeout::suggested(beast::role_type::client));
        
        ws_->set_option(websocket::stream_base::decorator(
            [](websocket::request_type& req) {
                req.set(beast::http::field::user_agent, 
                       "KrakenSDK/1.0 (C++; Boost.Beast)");
            }
        ));
        
        // WebSocket handshake
        std::string host_header = host_ + ":" + std::to_string(80); // simplified port logic for now or parse from ep
        ws_->handshake(host_header, path_);
        
        is_open_.store(true);
        
    } catch (const beast::system_error& e) {
        throw ConnectionError(std::string("Connection failed: ") + e.what());
    } catch (const std::exception& e) {
        throw ConnectionError(std::string("Connection failed: ") + e.what());
    }
}

void Connection::close() {
    if (!ws_) return;
    
    // Set flag first to prevent other operations
    bool was_open = is_open_.exchange(false);
    if (!was_open) return;  // Already closed
    
    try {
        // Close underlying socket to interrupt blocking reads immediately
        // Note: ws_->close() is not thread-safe if read() is blocking in another thread
        if (ws_) {
            beast::error_code ec;
            beast::get_lowest_layer(*ws_).close(ec);
        }
        
        // Stop the io_context to interrupt any other blocking operations (e.g. resolve)
        ioc_.stop();
    } catch (...) {
        // Ignore all exceptions during close
        // WebSocket might be in various states (closing, closed, etc.)
    }
}

bool Connection::is_open() const {
    return is_open_.load() && ws_ && ws_->is_open();
}

void Connection::send(const std::string& message) {
    if (!is_open()) {
        throw ConnectionError("Not connected");
    }
    
    // Apply rate limiting if enabled
    if (rate_limiter_) {
        // Block until token is available (with reasonable timeout)
        if (!rate_limiter_->acquire_blocking(std::chrono::seconds(30))) {
            throw ConnectionError("Rate limit: timeout waiting for token");
        }
    }
    
    std::lock_guard<std::mutex> lock(send_mutex_);
    
    try {
        ws_->write(net::buffer(message));
    } catch (const beast::system_error& e) {
        is_open_.store(false);
        throw ConnectionError(std::string("Send failed: ") + e.what());
    }
}

std::string Connection::receive() {
    if (!is_open()) {
        return "";
    }
    
    try {
        read_buffer_.clear();
        ws_->read(read_buffer_);
        
        return beast::buffers_to_string(read_buffer_.data());
        
    } catch (const beast::system_error& e) {
        if (e.code() == websocket::error::closed) {
            is_open_.store(false);
            return "";
        }
        is_open_.store(false);
        throw ConnectionError(std::string("Receive failed: ") + e.what());
    }
}

} // namespace kraken

