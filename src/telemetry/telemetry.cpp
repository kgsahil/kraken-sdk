/// @file telemetry.cpp
/// @brief Main Telemetry implementation

#include "kraken/telemetry/telemetry.hpp"
#include "kraken/telemetry/prometheus_server.hpp"
#include "kraken/telemetry/otlp_exporter.hpp"

namespace kraken {

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

