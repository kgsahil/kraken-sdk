#include "internal/connection.hpp"
#include "kraken/error.hpp"
#include "kraken/connection_config.hpp"

#include <iostream>
#include <cctype>
#include <fstream>
#include <openssl/ssl.h>

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
        // Note: For Boost 1.74, use executor directly instead of make_strand
        ws_ = std::make_unique<stream_type>(ioc_, ssl_ctx_);
        
        // Resolve host
        tcp::resolver resolver(ioc_);
        auto results = resolver.resolve(host_, port_);
        
        // Connect TCP
        auto ep = net::connect(beast::get_lowest_layer(*ws_), results);
        
        // SNI hostname
        if (!SSL_set_tlsext_host_name(ws_->next_layer().native_handle(), 
                                       host_.c_str())) {
            throw ConnectionError("Failed to set SNI hostname");
        }
        
        // SSL handshake
        ws_->next_layer().handshake(ssl::stream_base::client);
        
        // Set WebSocket options with timeout
        // Note: Boost.Beast websocket::stream_base::timeout uses suggested() for defaults
        // Custom timeouts are configured via ConnectionTimeouts but applied at the socket level
        // For now, we use suggested() defaults; custom timeout application would require
        // lower-level socket timeout configuration which is complex with Boost.Asio
        ws_->set_option(websocket::stream_base::timeout::suggested(beast::role_type::client));
        
        ws_->set_option(websocket::stream_base::decorator(
            [](websocket::request_type& req) {
                req.set(beast::http::field::user_agent, 
                       "KrakenSDK/1.0 (C++; Boost.Beast)");
            }
        ));
        
        // WebSocket handshake
        std::string host_header = host_ + ":" + std::to_string(ep.port());
        ws_->handshake(host_header, path_);
        
        is_open_.store(true);
        
    } catch (const beast::system_error& e) {
        throw ConnectionError(std::string("Connection failed: ") + e.what());
    } catch (const std::exception& e) {
        // Catch-all for any other exceptions
        throw ConnectionError(std::string("Connection failed: ") + e.what());
    }
}

void Connection::close() {
    if (!ws_) return;
    
    // Set flag first to prevent other operations
    bool was_open = is_open_.exchange(false);
    if (!was_open) return;  // Already closed
    
    try {
        // Check if WebSocket is actually open before trying to close
        if (ws_->is_open()) {
            beast::error_code ec;
            ws_->close(websocket::close_code::normal, ec);
            // Ignore errors during close (connection might already be closed)
        }
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

