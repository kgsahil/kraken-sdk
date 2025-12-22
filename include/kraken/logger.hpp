#pragma once

/// @file logger.hpp
/// @brief Structured logging interface for the Kraken SDK
/// 
/// Provides a lightweight logging abstraction using spdlog.
/// Logs are structured with levels (DEBUG, INFO, WARN, ERROR) and
/// can be configured to output to console, file, or both.

#include <string>
#include <memory>
#include <spdlog/spdlog.h>
#include <spdlog/logger.h>

namespace kraken {

/// Logger interface for the SDK
/// 
/// Provides structured logging with levels and configurable output.
/// By default, logs to console with INFO level and above.
class Logger {
public:
    /// Initialize logger with default settings (console, INFO level)
    static void init();
    
    /// Initialize logger with custom settings
    /// @param level Log level (trace, debug, info, warn, error, critical, off)
    /// @param console Enable console logging
    /// @param file_path Optional file path for file logging (empty = disabled)
    static void init(const std::string& level, bool console = true, const std::string& file_path = "");
    
    /// Get the logger instance
    static std::shared_ptr<spdlog::logger> get();
    
    /// Shutdown logger (flush and close)
    static void shutdown();
    
    /// Set log level
    static void set_level(const std::string& level);
    
    /// Check if logger is initialized
    static bool is_initialized();

private:
    static std::shared_ptr<spdlog::logger> logger_;
    static bool initialized_;
};

// Convenience macros for logging
#define KRAKEN_LOG_TRACE(...)    if (kraken::Logger::is_initialized()) kraken::Logger::get()->trace(__VA_ARGS__)
#define KRAKEN_LOG_DEBUG(...)    if (kraken::Logger::is_initialized()) kraken::Logger::get()->debug(__VA_ARGS__)
#define KRAKEN_LOG_INFO(...)     if (kraken::Logger::is_initialized()) kraken::Logger::get()->info(__VA_ARGS__)
#define KRAKEN_LOG_WARN(...)     if (kraken::Logger::is_initialized()) kraken::Logger::get()->warn(__VA_ARGS__)
#define KRAKEN_LOG_ERROR(...)    if (kraken::Logger::is_initialized()) kraken::Logger::get()->error(__VA_ARGS__)
#define KRAKEN_LOG_CRITICAL(...) if (kraken::Logger::is_initialized()) kraken::Logger::get()->critical(__VA_ARGS__)

} // namespace kraken

