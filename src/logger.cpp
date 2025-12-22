#include "kraken/logger.hpp"

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>

namespace kraken {

std::shared_ptr<spdlog::logger> Logger::logger_ = nullptr;
bool Logger::initialized_ = false;

void Logger::init() {
    init("info", true, "");
}

void Logger::init(const std::string& level, bool console, const std::string& file_path) {
    if (initialized_) {
        shutdown();
    }
    
    std::vector<spdlog::sink_ptr> sinks;
    
    // Console sink
    if (console) {
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");
        sinks.push_back(console_sink);
    }
    
    // File sink (rotating, 5MB per file, 3 files)
    if (!file_path.empty()) {
        auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            file_path, 5 * 1024 * 1024, 3);
        file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");
        sinks.push_back(file_sink);
    }
    
    // Create logger with multiple sinks
    logger_ = std::make_shared<spdlog::logger>("kraken", sinks.begin(), sinks.end());
    logger_->set_level(spdlog::level::from_str(level));
    logger_->flush_on(spdlog::level::warn);
    
    spdlog::register_logger(logger_);
    initialized_ = true;
}

std::shared_ptr<spdlog::logger> Logger::get() {
    if (!initialized_) {
        init();  // Auto-initialize with defaults
    }
    return logger_;
}

void Logger::shutdown() {
    if (logger_) {
        logger_->flush();
        spdlog::drop("kraken");
        logger_.reset();
    }
    initialized_ = false;
}

void Logger::set_level(const std::string& level) {
    if (logger_) {
        logger_->set_level(spdlog::level::from_str(level));
    }
}

bool Logger::is_initialized() {
    return initialized_ && logger_ != nullptr;
}

} // namespace kraken

