/// @file test_telemetry.cpp
/// @brief Unit tests for telemetry/metrics collection

#include <gtest/gtest.h>
#include <kraken/telemetry.hpp>
#include <thread>

using namespace kraken;

class TelemetryTest : public ::testing::Test {
protected:
    void SetUp() override {
        collector_.reset();
    }
    
    MetricsCollector collector_;
};

//------------------------------------------------------------------------------
// Counter Tests
//------------------------------------------------------------------------------

TEST_F(TelemetryTest, IncrementCounters) {
    EXPECT_EQ(collector_.messages_received(), 0);
    
    collector_.increment_messages_received();
    collector_.increment_messages_received();
    collector_.increment_messages_received();
    
    EXPECT_EQ(collector_.messages_received(), 3);
}

TEST_F(TelemetryTest, AllCounters) {
    collector_.increment_messages_received();
    collector_.increment_messages_processed();
    collector_.increment_messages_dropped();
    collector_.increment_reconnect_attempts();
    collector_.increment_checksum_failures();
    collector_.increment_gaps_detected();
    collector_.increment_alerts_triggered();
    
    EXPECT_EQ(collector_.messages_received(), 1);
    EXPECT_EQ(collector_.messages_processed(), 1);
    EXPECT_EQ(collector_.messages_dropped(), 1);
    EXPECT_EQ(collector_.reconnect_attempts(), 1);
    EXPECT_EQ(collector_.checksum_failures(), 1);
    EXPECT_EQ(collector_.gaps_detected(), 1);
    EXPECT_EQ(collector_.alerts_triggered(), 1);
}

//------------------------------------------------------------------------------
// Gauge Tests
//------------------------------------------------------------------------------

TEST_F(TelemetryTest, QueueDepth) {
    EXPECT_EQ(collector_.queue_depth(), 0);
    
    collector_.set_queue_depth(100);
    EXPECT_EQ(collector_.queue_depth(), 100);
    
    collector_.set_queue_depth(50);
    EXPECT_EQ(collector_.queue_depth(), 50);
}

TEST_F(TelemetryTest, ConnectionState) {
    EXPECT_EQ(collector_.connection_state(), 0);
    
    collector_.set_connection_state(1);
    EXPECT_EQ(collector_.connection_state(), 1);
}

//------------------------------------------------------------------------------
// Latency Tests
//------------------------------------------------------------------------------

TEST_F(TelemetryTest, LatencyTracking) {
    EXPECT_EQ(collector_.latency_max_us(), 0);
    EXPECT_EQ(collector_.latency_avg_us(), 0.0);
    
    collector_.record_latency_us(100);
    collector_.record_latency_us(200);
    collector_.record_latency_us(300);
    
    EXPECT_EQ(collector_.latency_max_us(), 300);
    EXPECT_EQ(collector_.latency_avg_us(), 200.0);
}

TEST_F(TelemetryTest, LatencyMaxUpdates) {
    collector_.record_latency_us(500);
    EXPECT_EQ(collector_.latency_max_us(), 500);
    
    collector_.record_latency_us(100);  // Lower, shouldn't change max
    EXPECT_EQ(collector_.latency_max_us(), 500);
    
    collector_.record_latency_us(1000);  // Higher, should update
    EXPECT_EQ(collector_.latency_max_us(), 1000);
}

//------------------------------------------------------------------------------
// Channel Counts Tests
//------------------------------------------------------------------------------

TEST_F(TelemetryTest, ChannelCounts) {
    collector_.increment_messages_received("ticker");
    collector_.increment_messages_received("ticker");
    collector_.increment_messages_received("book");
    
    auto counts = collector_.channel_counts();
    EXPECT_EQ(counts["ticker"], 2);
    EXPECT_EQ(counts["book"], 1);
}

//------------------------------------------------------------------------------
// Reset Tests
//------------------------------------------------------------------------------

TEST_F(TelemetryTest, Reset) {
    collector_.increment_messages_received();
    collector_.increment_messages_processed();
    collector_.set_queue_depth(100);
    collector_.record_latency_us(500);
    
    collector_.reset();
    
    EXPECT_EQ(collector_.messages_received(), 0);
    EXPECT_EQ(collector_.messages_processed(), 0);
    EXPECT_EQ(collector_.queue_depth(), 0);
    EXPECT_EQ(collector_.latency_max_us(), 0);
}

//------------------------------------------------------------------------------
// Export Format Tests
//------------------------------------------------------------------------------

TEST_F(TelemetryTest, PrometheusFormat) {
    collector_.increment_messages_received();
    collector_.increment_messages_received();
    collector_.set_queue_depth(10);
    
    std::string prom = collector_.to_prometheus();
    
    // Verify Prometheus format
    EXPECT_NE(prom.find("# HELP kraken_messages_received_total"), std::string::npos);
    EXPECT_NE(prom.find("# TYPE kraken_messages_received_total counter"), std::string::npos);
    EXPECT_NE(prom.find("kraken_messages_received_total 2"), std::string::npos);
    EXPECT_NE(prom.find("kraken_queue_depth 10"), std::string::npos);
}

TEST_F(TelemetryTest, JSONFormat) {
    collector_.increment_messages_received();
    collector_.set_queue_depth(5);
    
    std::string json = collector_.to_json();
    
    // Verify JSON format
    EXPECT_NE(json.find("\"messages_received\":1"), std::string::npos);
    EXPECT_NE(json.find("\"queue_depth\":5"), std::string::npos);
}

//------------------------------------------------------------------------------
// Telemetry Manager Tests
//------------------------------------------------------------------------------

TEST_F(TelemetryTest, TelemetryManager) {
    auto telemetry = Telemetry::create(
        TelemetryConfig::Builder()
            .service_name("test-service")
            .otlp_endpoint("http://localhost:4317")
            .metrics(true)
            .build()
    );
    
    EXPECT_TRUE(telemetry->is_enabled());
    EXPECT_EQ(telemetry->config().service_name, "test-service");
    
    // Access metrics
    telemetry->metrics().increment_messages_received();
    EXPECT_EQ(telemetry->metrics().messages_received(), 1);
}

TEST_F(TelemetryTest, ConfigBuilder) {
    auto config = TelemetryConfig::Builder()
        .otlp_endpoint("http://otel-collector:4317")
        .service_name("my-bot")
        .service_version("2.0.0")
        .environment("production")
        .metrics(true)
        .traces(true)
        .logs(false)
        .metrics_interval(std::chrono::seconds(30))
        .build();
    
    EXPECT_EQ(config.otlp_endpoint, "http://otel-collector:4317");
    EXPECT_EQ(config.service_name, "my-bot");
    EXPECT_EQ(config.service_version, "2.0.0");
    EXPECT_EQ(config.environment, "production");
    EXPECT_TRUE(config.enable_metrics);
    EXPECT_TRUE(config.enable_traces);
    EXPECT_FALSE(config.enable_logs);
    EXPECT_EQ(config.metrics_interval.count(), 30);
}

//------------------------------------------------------------------------------
// Thread Safety Tests
//------------------------------------------------------------------------------

TEST_F(TelemetryTest, ConcurrentIncrements) {
    const int threads = 4;
    const int increments = 1000;
    
    std::vector<std::thread> workers;
    for (int t = 0; t < threads; ++t) {
        workers.emplace_back([&]() {
            for (int i = 0; i < increments; ++i) {
                collector_.increment_messages_received();
                collector_.increment_messages_processed();
            }
        });
    }
    
    for (auto& t : workers) {
        t.join();
    }
    
    EXPECT_EQ(collector_.messages_received(), threads * increments);
    EXPECT_EQ(collector_.messages_processed(), threads * increments);
}

TEST_F(TelemetryTest, ConcurrentLatencyRecording) {
    const int threads = 4;
    const int records = 1000;
    
    std::vector<std::thread> workers;
    for (int t = 0; t < threads; ++t) {
        workers.emplace_back([&, t]() {
            for (int i = 0; i < records; ++i) {
                collector_.record_latency_us(100 + t * 10 + i % 50);
            }
        });
    }
    
    for (auto& t : workers) {
        t.join();
    }
    
    // Max should be at least 100 + 3*10 + 49 = 179
    EXPECT_GE(collector_.latency_max_us(), 100);
}

