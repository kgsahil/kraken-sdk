/// @file connection.hpp
/// @brief Internal WebSocket connection implementation
/// 
/// Low-level WebSocket connection using Boost.Beast with TLS support.
/// Handles connection lifecycle, message I/O, and timeout management.
/// 
/// @note This is an internal header. Users should use KrakenClient instead.

#pragma once

#include "kraken/connection/connection_config.hpp"
#include "kraken/rate_limiter.hpp"

#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>

#include <string>
#include <memory>
#include <mutex>

namespace kraken {

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace net = boost::asio;
namespace ssl = net::ssl;
using tcp = net::ip::tcp;

/// @brief WebSocket connection with TLS
/// 
/// Internal WebSocket connection implementation using Boost.Beast.
/// Handles TLS handshake, connection lifecycle, and message I/O.
/// 
/// @note This is an internal class. Users should use KrakenClient instead.
class Connection {
public:
    /// @brief Construct connection with URL and configuration
    /// @param url WebSocket URL (e.g., "wss://ws.kraken.com/v2")
    /// @param timeouts Connection timeout settings
    /// @param security TLS/SSL security settings
    /// @param rate_limiter Rate limiter for outbound messages (may be null)
    explicit Connection(const std::string& url, 
                       const ConnectionTimeouts& timeouts = {},
                       const SecurityConfig& security = {},
                       std::shared_ptr<RateLimiter> rate_limiter = nullptr);
    
    /// @brief Destructor - closes connection if still open
    ~Connection();
    
    // Non-copyable, non-movable
    Connection(const Connection&) = delete;
    Connection& operator=(const Connection&) = delete;
    Connection(Connection&&) = delete;
    Connection& operator=(Connection&&) = delete;
    
    /// @brief Connect to the WebSocket server
    /// 
    /// Establishes TCP connection, performs TLS handshake, and upgrades to WebSocket.
    /// 
    /// @throws ConnectionError on failure (network error, TLS error, etc.)
    void connect();
    
    /// @brief Close the connection
    /// 
    /// Sends close frame and closes the underlying socket gracefully.
    void close();
    
    /// @brief Check if connection is open
    /// @return true if connection is open and ready
    bool is_open() const;
    
    /// @brief Send a message
    /// 
    /// Sends a text message over the WebSocket connection.
    /// 
    /// @param message Message to send (JSON string)
    /// @throws ConnectionError on failure (connection closed, write error, etc.)
    void send(const std::string& message);
    
    /// @brief Receive a message (blocking)
    /// 
    /// Blocks until a message is received or connection is closed.
    /// 
    /// @return The received message, or empty string if connection closed
    std::string receive();
    
private:
    void parse_url(const std::string& url);
    
    std::string host_;
    std::string port_;
    std::string path_;
    
    ConnectionTimeouts timeouts_;
    SecurityConfig security_;
    std::shared_ptr<RateLimiter> rate_limiter_;
    
    net::io_context ioc_;
    ssl::context ssl_ctx_{ssl::context::tlsv12_client};
    
    using stream_type = websocket::stream<beast::ssl_stream<tcp::socket>>;
    std::unique_ptr<stream_type> ws_;
    
    beast::flat_buffer read_buffer_;
    std::mutex send_mutex_;
    std::atomic<bool> is_open_{false};
};

} // namespace kraken

