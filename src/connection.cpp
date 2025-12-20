#include "connection.hpp"
#include "kraken/error.hpp"

#include <regex>
#include <iostream>

namespace kraken {

Connection::Connection(const std::string& url) {
    parse_url(url);
    
    // Load default CA certificates
    ssl_ctx_.set_default_verify_paths();
    ssl_ctx_.set_verify_mode(ssl::verify_peer);
}

Connection::~Connection() {
    close();
}

void Connection::parse_url(const std::string& url) {
    // Parse WebSocket URL: wss://host:port/path or wss://host/path
    std::regex url_regex(R"(wss?://([^/:]+)(?::(\d+))?(/.*)?)", std::regex::icase);
    std::smatch match;
    
    if (!std::regex_match(url, match, url_regex)) {
        throw std::invalid_argument("Invalid WebSocket URL: " + url);
    }
    
    host_ = match[1].str();
    port_ = match[2].matched ? match[2].str() : "443";
    path_ = match[3].matched ? match[3].str() : "/";
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
        
        // Set WebSocket options
        ws_->set_option(websocket::stream_base::timeout::suggested(
            beast::role_type::client
        ));
        
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

