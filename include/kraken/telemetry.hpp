#pragma once

/// @file telemetry.hpp
/// @brief Main telemetry include (backward compatibility)
/// 
/// This file includes all telemetry components for backward compatibility.
/// For better compilation times, include specific headers:
/// - `telemetry/config.hpp` - TelemetryConfig
/// - `telemetry/metrics_collector.hpp` - MetricsCollector
/// - `telemetry/telemetry.hpp` - Telemetry class
/// - `telemetry/prometheus_server.hpp` - MetricsHttpServer
/// - `telemetry/otlp_exporter.hpp` - OtlpHttpExporter

#include "telemetry/telemetry.hpp"
