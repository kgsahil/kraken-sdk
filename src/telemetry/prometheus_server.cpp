/// @file prometheus_server.cpp
/// @brief HTTP server implementation for Prometheus metrics

#include "kraken/telemetry/prometheus_server.hpp"
#include "kraken/telemetry/telemetry.hpp"

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>

#include <thread>
#include <atomic>
#include <iostream>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

namespace kraken {

class MetricsHttpServer::Impl {
public:
    Impl(std::shared_ptr<Telemetry> telemetry, uint16_t port)
        : telemetry_(std::move(telemetry))
        , port_(port)
        , acceptor_(ioc_)
        , socket_(ioc_)
        , running_(false) {}
    
    ~Impl() {
        stop();
    }
    
    bool start() {
        if (running_.exchange(true)) return true;
        
        try {
            tcp::endpoint endpoint(tcp::v4(), port_);
            acceptor_.open(endpoint.protocol());
            acceptor_.set_option(net::socket_base::reuse_address(true));
            acceptor_.bind(endpoint);
            acceptor_.listen();
            
            server_thread_ = std::thread([this]() { run(); });
            return true;
        } catch (const std::exception& e) {
            running_ = false;
            std::cerr << "Failed to start metrics server on port " << port_ 
                      << ": " << e.what() << std::endl;
            return false;
        }
    }
    
    void stop() {
        if (!running_.exchange(false)) return;
        
        ioc_.stop();
        if (server_thread_.joinable()) {
            server_thread_.join();
        }
    }
    
    bool is_running() const { return running_.load(); }
    uint16_t port() const { return port_; }

private:
    void run() {
        accept_connection();
        ioc_.run();
    }
    
    void accept_connection() {
        if (!running_.load()) return;
        
        acceptor_.async_accept(socket_, [this](beast::error_code ec) {
            if (!ec && running_.load()) {
                std::make_shared<Session>(std::move(socket_), telemetry_)->start();
            }
            if (running_.load()) {
                accept_connection();
            }
        });
    }
    
    class Session : public std::enable_shared_from_this<Session> {
    public:
        Session(tcp::socket socket, std::shared_ptr<Telemetry> telemetry)
            : socket_(std::move(socket))
            , telemetry_(std::move(telemetry)) {}
        
        void start() {
            read_request();
        }
        
    private:
        void read_request() {
            auto self = shared_from_this();
            http::async_read(socket_, buffer_, request_,
                [self](beast::error_code ec, std::size_t) {
                    if (!ec) {
                        self->handle_request();
                    }
                });
        }
        
        void handle_request() {
            std::string target = std::string(request_.target());
            
            if (target == "/metrics" || target == "/metrics/") {
                send_metrics();
            } else if (target == "/health" || target == "/health/") {
                send_health();
            } else {
                send_not_found();
            }
        }
        
        // Helper to build and send HTTP response
        void send_response(http::status status, const std::string& content_type, 
                          const std::string& body) {
            response_.result(status);
            response_.set(http::field::content_type, content_type);
            response_.set(http::field::server, "KrakenSDK/1.0");
            response_.body() = body;
            response_.prepare_payload();
            
            auto self = shared_from_this();
            http::async_write(socket_, response_,
                [self](beast::error_code ec, std::size_t) {
                    self->socket_.shutdown(tcp::socket::shutdown_send, ec);
                });
        }
        
        void send_metrics() {
            std::string body = telemetry_ ? telemetry_->prometheus_metrics() : "# No metrics available\n";
            send_response(http::status::ok, "text/plain; version=0.0.4", body);
        }
        
        void send_health() {
            send_response(http::status::ok, "application/json", R"({"status":"healthy"})");
        }
        
        void send_not_found() {
            send_response(http::status::not_found, "text/plain", "Not Found\n");
        }
        
        tcp::socket socket_;
        std::shared_ptr<Telemetry> telemetry_;
        beast::flat_buffer buffer_;
        http::request<http::string_body> request_;
        http::response<http::string_body> response_;
    };
    
    std::shared_ptr<Telemetry> telemetry_;
    uint16_t port_;
    net::io_context ioc_;
    tcp::acceptor acceptor_;
    tcp::socket socket_;
    std::atomic<bool> running_;
    std::thread server_thread_;
};

MetricsHttpServer::MetricsHttpServer(std::shared_ptr<Telemetry> telemetry, uint16_t port)
    : impl_(std::make_unique<Impl>(std::move(telemetry), port)) {}

MetricsHttpServer::~MetricsHttpServer() = default;

bool MetricsHttpServer::start() {
    return impl_->start();
}

void MetricsHttpServer::stop() {
    impl_->stop();
}

bool MetricsHttpServer::is_running() const {
    return impl_->is_running();
}

uint16_t MetricsHttpServer::port() const {
    return impl_->port();
}

} // namespace kraken

