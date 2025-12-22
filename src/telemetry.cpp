#include "kraken/telemetry.hpp"

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/ssl.hpp>

#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <sstream>
#include <iostream>
#include <iomanip>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

namespace kraken {

//------------------------------------------------------------------------------
// HTTP Server for Prometheus Metrics (PIMPL)
//------------------------------------------------------------------------------

class MetricsHttpServer {
public:
    MetricsHttpServer(std::shared_ptr<Telemetry> telemetry, uint16_t port)
        : telemetry_(std::move(telemetry))
        , port_(port)
        , acceptor_(ioc_)
        , socket_(ioc_)
        , running_(false) {}
    
    ~MetricsHttpServer() {
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

//------------------------------------------------------------------------------
// OTLP HTTP Exporter
//------------------------------------------------------------------------------

class OtlpHttpExporter {
public:
    OtlpHttpExporter(const TelemetryConfig& config)
        : config_(config)
        , running_(false) {}
    
    ~OtlpHttpExporter() {
        stop();
    }
    
    bool start(std::shared_ptr<Telemetry> telemetry) {
        if (running_.exchange(true)) return true;
        
        telemetry_ = std::move(telemetry);
        export_thread_ = std::thread([this]() { export_loop(); });
        return true;
    }
    
    void stop() {
        if (!running_.exchange(false)) return;
        
        cv_.notify_all();
        if (export_thread_.joinable()) {
            export_thread_.join();
        }
    }
    
    bool flush() {
        if (!telemetry_ || !running_.load()) return false;
        return export_metrics();
    }

private:
    void export_loop() {
        while (running_.load()) {
            std::unique_lock<std::mutex> lock(mutex_);
            // Wait for interval or until stopped
            bool stopped = cv_.wait_for(lock, config_.metrics_interval, [this] { 
                return !running_.load(); 
            });
            
            // If we timed out (not stopped), export metrics
            if (!stopped && running_.load()) {
                export_metrics();
            }
        }
    }
    
    bool export_metrics() {
        if (!telemetry_ || !config_.enable_otlp_export) return false;
        
        try {
            // Parse OTLP endpoint URL
            std::string endpoint = config_.otlp_endpoint;
            if (endpoint.empty()) {
                return false;
            }
            
            // Ensure endpoint ends with /v1/metrics
            if (endpoint.back() != '/') {
                endpoint += "/";
            }
            if (endpoint.find("/v1/metrics") == std::string::npos) {
                endpoint += "v1/metrics";
            }
            
            // Extract host, port, and path from endpoint
            std::string host, port, path;
            size_t scheme_end = endpoint.find("://");
            if (scheme_end == std::string::npos) {
                return false;
            }
            
            size_t host_start = scheme_end + 3;
            size_t host_end = endpoint.find_first_of(":/", host_start);
            if (host_end == std::string::npos) {
                host_end = endpoint.length();
            }
            
            host = endpoint.substr(host_start, host_end - host_start);
            
            if (host_end < endpoint.length() && endpoint[host_end] == ':') {
                size_t port_start = host_end + 1;
                size_t port_end = endpoint.find('/', port_start);
                if (port_end == std::string::npos) {
                    port_end = endpoint.length();
                }
                port = endpoint.substr(port_start, port_end - port_start);
                host_end = port_end;
            } else {
                port = (endpoint.substr(0, scheme_end) == "https") ? "443" : "80";
            }
            
            if (host_end < endpoint.length()) {
                path = endpoint.substr(host_end);
            } else {
                path = "/v1/metrics";
            }
            
            // Determine if HTTPS
            bool is_https = endpoint.substr(0, scheme_end) == "https";
            
            // Create HTTP client and send metrics
            net::io_context ioc;
            tcp::resolver resolver(ioc);
            auto results = resolver.resolve(host, port);
            
            const auto& metrics = telemetry_->metrics();
            std::string json_body = build_otlp_json(metrics);
            
            if (is_https) {
                // HTTPS connection
                net::ssl::context ctx(net::ssl::context::tlsv12_client);
                ctx.set_default_verify_paths();
                ctx.set_verify_mode(net::ssl::verify_peer);
                
                beast::ssl_stream<beast::tcp_stream> stream(ioc, ctx);
                
                // Set SNI hostname
                if (!SSL_set_tlsext_host_name(stream.native_handle(), host.c_str())) {
                    return false;
                }
                
                beast::get_lowest_layer(stream).connect(results);
                stream.handshake(net::ssl::stream_base::client);
                
                // Build request
                http::request<http::string_body> req{http::verb::post, path, 11};
                req.set(http::field::host, host + ":" + port);
                req.set(http::field::content_type, "application/json");
                req.set(http::field::user_agent, "KrakenSDK/1.0");
                req.body() = json_body;
                req.prepare_payload();
                
                // Send and receive
                http::write(stream, req);
                beast::flat_buffer buffer;
                http::response<http::string_body> res;
                http::read(stream, buffer, res);
                
                // Close
                beast::error_code ec;
                stream.shutdown(ec);
                
                return res.result() == http::status::ok || 
                       res.result() == http::status::accepted;
            } else {
                // HTTP connection
                beast::tcp_stream stream(ioc);
                stream.connect(results);
                
                // Build request
                http::request<http::string_body> req{http::verb::post, path, 11};
                req.set(http::field::host, host + ":" + port);
                req.set(http::field::content_type, "application/json");
                req.set(http::field::user_agent, "KrakenSDK/1.0");
                req.body() = json_body;
                req.prepare_payload();
                
                // Send and receive
                http::write(stream, req);
                beast::flat_buffer buffer;
                http::response<http::string_body> res;
                http::read(stream, buffer, res);
                
                // Close
                beast::error_code ec;
                stream.socket().shutdown(tcp::socket::shutdown_both, ec);
                
                return res.result() == http::status::ok || 
                       res.result() == http::status::accepted;
            }
            
        } catch (const std::exception& e) {
            // Log error but don't throw (export failures shouldn't crash)
            return false;
        }
    }
    
    std::string build_otlp_json(const MetricsCollector& metrics) {
        // Build OTLP JSON format for metrics
        // Simplified version - sends metrics as OTLP ResourceMetrics
        std::ostringstream json;
        json << R"({"resourceMetrics":[{"resource":{)"
             << R"("attributes":[{"key":"service.name","value":{"stringValue":")"
             << config_.service_name << R"("}},)"
             << R"({"key":"service.version","value":{"stringValue":")"
             << config_.service_version << R"("}},)"
             << R"({"key":"deployment.environment","value":{"stringValue":")"
             << config_.environment << R"("}}]},)"
             << R"("scopeMetrics":[{"metrics":[)";
        
        // Add counter metrics
        json << R"({"name":"kraken.messages.received","sum":{"dataPoints":[{"asInt":")"
             << metrics.messages_received() << R"("}],"aggregationTemporality":2,"isMonotonic":true}},)";
        
        json << R"({"name":"kraken.messages.processed","sum":{"dataPoints":[{"asInt":")"
             << metrics.messages_processed() << R"("}],"aggregationTemporality":2,"isMonotonic":true}},)";
        
        json << R"({"name":"kraken.messages.dropped","sum":{"dataPoints":[{"asInt":")"
             << metrics.messages_dropped() << R"("}],"aggregationTemporality":2,"isMonotonic":true}},)";
        
        json << R"({"name":"kraken.reconnect.attempts","sum":{"dataPoints":[{"asInt":")"
             << metrics.reconnect_attempts() << R"("}],"aggregationTemporality":2,"isMonotonic":true}},)";
        
        json << R"({"name":"kraken.checksum.failures","sum":{"dataPoints":[{"asInt":")"
             << metrics.checksum_failures() << R"("}],"aggregationTemporality":2,"isMonotonic":true}},)";
        
        json << R"({"name":"kraken.gaps.detected","sum":{"dataPoints":[{"asInt":")"
             << metrics.gaps_detected() << R"("}],"aggregationTemporality":2,"isMonotonic":true}},)";
        
        json << R"({"name":"kraken.alerts.triggered","sum":{"dataPoints":[{"asInt":")"
             << metrics.alerts_triggered() << R"("}],"aggregationTemporality":2,"isMonotonic":true}},)";
        
        // Add gauge metrics
        json << R"({"name":"kraken.queue.depth","gauge":{"dataPoints":[{"asInt":")"
             << metrics.queue_depth() << R"("}]}},)";
        
        json << R"({"name":"kraken.connection.state","gauge":{"dataPoints":[{"asInt":")"
             << metrics.connection_state() << R"("}]}},)";
        
        json << R"({"name":"kraken.latency.max.us","gauge":{"dataPoints":[{"asInt":")"
             << metrics.latency_max_us() << R"("}]}},)";
        
        json << R"({"name":"kraken.latency.avg.us","gauge":{"dataPoints":[{"asDouble":)"
             << metrics.latency_avg_us() << R"(}}]}})";
        
        json << R"(]}]}]})";
        
        return json.str();
    }
    
    TelemetryConfig config_;
    std::shared_ptr<Telemetry> telemetry_;
    std::atomic<bool> running_;
    std::thread export_thread_;
    std::mutex mutex_;
    std::condition_variable cv_;
};

//------------------------------------------------------------------------------
// Telemetry::Impl (PIMPL)
//------------------------------------------------------------------------------

class Telemetry::Impl {
public:
    explicit Impl(TelemetryConfig config)
        : config_(std::move(config))
        , http_server_(nullptr)
        , otlp_exporter_(config_) {}
    
    ~Impl() {
        stop();
    }
    
    bool start(std::shared_ptr<Telemetry> self) {
        bool success = true;
        
        // Start HTTP server if enabled
        if (config_.enable_http_server) {
            http_server_ = std::make_unique<MetricsHttpServer>(self, config_.http_server_port);
            if (!http_server_->start()) {
                success = false;
            }
        }
        
        // Start OTLP exporter if enabled
        if (config_.enable_otlp_export) {
            if (!otlp_exporter_.start(self)) {
                success = false;
            }
        }
        
        return success;
    }
    
    void stop() {
        if (http_server_) {
            http_server_->stop();
            http_server_.reset();
        }
        otlp_exporter_.stop();
    }
    
    bool flush() {
        return otlp_exporter_.flush();
    }
    
    bool is_http_server_running() const {
        return http_server_ && http_server_->is_running();
    }
    
    uint16_t http_server_port() const {
        return http_server_ ? http_server_->port() : 0;
    }

private:
    TelemetryConfig config_;
    std::unique_ptr<MetricsHttpServer> http_server_;
    OtlpHttpExporter otlp_exporter_;
};

//------------------------------------------------------------------------------
// Telemetry Implementation
//------------------------------------------------------------------------------

Telemetry::Telemetry(TelemetryConfig config)
    : config_(std::move(config))
    , impl_(std::make_unique<Impl>(config_)) {}

Telemetry::~Telemetry() {
    stop();
}

bool Telemetry::start() {
    return impl_->start(shared_from_this());
}

void Telemetry::stop() {
    impl_->stop();
}

bool Telemetry::flush() {
    return impl_->flush();
}

bool Telemetry::is_http_server_running() const {
    return impl_->is_http_server_running();
}

uint16_t Telemetry::http_server_port() const {
    return impl_->http_server_port();
}

} // namespace kraken

