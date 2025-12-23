/// @file otlp_exporter.cpp
/// @brief OTLP HTTP exporter implementation

#include "kraken/telemetry/otlp_exporter.hpp"
#include "kraken/telemetry/telemetry.hpp"
#include "kraken/telemetry/metrics_collector.hpp"

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl.hpp>

#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <sstream>
#include <iostream>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

namespace kraken {

class OtlpHttpExporter::Impl {
public:
    explicit Impl(const TelemetryConfig& config)
        : config_(config)
        , running_(false) {}
    
    ~Impl() {
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

OtlpHttpExporter::OtlpHttpExporter(const TelemetryConfig& config)
    : impl_(std::make_unique<Impl>(config)) {}

OtlpHttpExporter::~OtlpHttpExporter() = default;

bool OtlpHttpExporter::start(std::shared_ptr<Telemetry> telemetry) {
    return impl_->start(std::move(telemetry));
}

void OtlpHttpExporter::stop() {
    impl_->stop();
}

bool OtlpHttpExporter::flush() {
    return impl_->flush();
}

} // namespace kraken

