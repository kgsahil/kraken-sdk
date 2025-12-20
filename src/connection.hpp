#pragma once

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

/// WebSocket connection with TLS
class Connection {
public:
    explicit Connection(const std::string& url);
    ~Connection();
    
    // Non-copyable, non-movable
    Connection(const Connection&) = delete;
    Connection& operator=(const Connection&) = delete;
    Connection(Connection&&) = delete;
    Connection& operator=(Connection&&) = delete;
    
    /// Connect to the WebSocket server
    /// @throws ConnectionError on failure
    void connect();
    
    /// Close the connection
    void close();
    
    /// Check if connection is open
    bool is_open() const;
    
    /// Send a message
    /// @throws ConnectionError on failure
    void send(const std::string& message);
    
    /// Receive a message (blocking)
    /// @return The received message, or empty string if closed
    std::string receive();
    
private:
    void parse_url(const std::string& url);
    
    std::string host_;
    std::string port_;
    std::string path_;
    
    net::io_context ioc_;
    ssl::context ssl_ctx_{ssl::context::tlsv12_client};
    
    using stream_type = websocket::stream<beast::ssl_stream<tcp::socket>>;
    std::unique_ptr<stream_type> ws_;
    
    beast::flat_buffer read_buffer_;
    std::mutex send_mutex_;
    std::atomic<bool> is_open_{false};
};

} // namespace kraken

