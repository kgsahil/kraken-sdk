/// @file strategy_config.hpp
/// @brief Configuration support for trading strategies
/// 
/// Provides utilities to create strategies from configuration files and
/// environment variables, enabling strategy deployment without code changes.

#pragma once

#include "strategies.hpp"
#include <string>
#include <memory>
#include <map>
#include <sstream>
#include <stdexcept>
#include <limits>
#include <algorithm>
#include <cctype>
#include <vector>

namespace kraken {

/// @brief Strategy configuration parser
/// 
/// Parses strategy configurations from key-value pairs (e.g., from config files
/// or environment variables) and creates strategy instances.
/// 
/// @example
/// @code
/// std::map<std::string, std::string> config = {
///     {"STRATEGY_TYPE", "price_alert"},
///     {"STRATEGY_SYMBOL", "BTC/USD"},
///     {"STRATEGY_ABOVE", "50000.0"},
///     {"STRATEGY_RECURRING", "true"}
/// };
/// auto strategy = StrategyConfig::from_map(config);
/// @endcode
class StrategyConfig {
public:
    /// @brief Create a strategy from configuration map
    /// 
    /// Supports the following strategy types:
    /// - `price_alert`: Price threshold alerts
    /// - `volume_spike`: Volume spike detection
    /// - `spread_alert`: Spread monitoring
    /// - `breakout`: Breakout detection (preset)
    /// - `support_level`: Support level detection (preset)
    /// - `resistance_level`: Resistance level detection (preset)
    /// 
    /// @param config Configuration map (key-value pairs)
    /// @param prefix Optional prefix for keys (e.g., "STRATEGY_")
    /// @return Shared pointer to created strategy
    /// @throws std::invalid_argument if configuration is invalid
    static std::shared_ptr<AlertStrategy> from_map(
        const std::map<std::string, std::string>& config,
        const std::string& prefix = "STRATEGY_") {
        
        std::string type_key = prefix + "TYPE";
        auto type_it = config.find(type_key);
        if (type_it == config.end()) {
            throw std::invalid_argument("Missing " + type_key + " in strategy configuration");
        }
        
        std::string type = type_it->second;
        std::transform(type.begin(), type.end(), type.begin(),
            [](unsigned char c) { return std::tolower(c); });
        
        if (type == "price_alert") {
            return create_price_alert(config, prefix);
        } else if (type == "volume_spike") {
            return create_volume_spike(config, prefix);
        } else if (type == "spread_alert") {
            return create_spread_alert(config, prefix);
        } else if (type == "breakout") {
            return create_breakout(config, prefix);
        } else if (type == "support_level") {
            return create_support_level(config, prefix);
        } else if (type == "resistance_level") {
            return create_resistance_level(config, prefix);
        } else {
            throw std::invalid_argument("Unknown strategy type: " + type);
        }
    }
    
    /// @brief Create a strategy from environment variables
    /// 
    /// Reads strategy configuration from environment variables with given prefix.
    /// 
    /// @param prefix Prefix for environment variables (e.g., "STRATEGY_")
    /// @return Shared pointer to created strategy, or nullptr if not configured
    static std::shared_ptr<AlertStrategy> from_env(const std::string& prefix = "STRATEGY_");
    
private:
    static std::string to_lower(const std::string& str);
    static bool parse_bool(const std::string& str);
    static double parse_double(const std::map<std::string, std::string>& config,
                               const std::string& key, double default_val = 0.0);
    static int parse_int(const std::map<std::string, std::string>& config,
                         const std::string& key, int default_val = 0);
    static std::string get_required(const std::map<std::string, std::string>& config,
                                     const std::string& key);
    static std::shared_ptr<AlertStrategy> create_price_alert(
        const std::map<std::string, std::string>& config,
        const std::string& prefix);
    static std::shared_ptr<AlertStrategy> create_volume_spike(
        const std::map<std::string, std::string>& config,
        const std::string& prefix);
    static std::shared_ptr<AlertStrategy> create_spread_alert(
        const std::map<std::string, std::string>& config,
        const std::string& prefix);
    static std::shared_ptr<AlertStrategy> create_breakout(
        const std::map<std::string, std::string>& config,
        const std::string& prefix);
    static std::shared_ptr<AlertStrategy> create_support_level(
        const std::map<std::string, std::string>& config,
        const std::string& prefix);
    static std::shared_ptr<AlertStrategy> create_resistance_level(
        const std::map<std::string, std::string>& config,
        const std::string& prefix);
    
};

} // namespace kraken

