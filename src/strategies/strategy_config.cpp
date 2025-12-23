/// @file strategy_config.cpp
/// @brief Implementation of strategy configuration utilities

#include "kraken/strategies/strategy_config.hpp"
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <sstream>
#include <limits>
#include <chrono>

namespace kraken {

// Declare environ (POSIX systems have it in unistd.h, Windows needs explicit declaration)
#ifdef _WIN32
#include <windows.h>
extern char** _environ;
#define ENVIRON _environ
#else
#include <unistd.h>
#define ENVIRON environ
#endif

std::shared_ptr<AlertStrategy> StrategyConfig::from_env(const std::string& prefix) {
    std::map<std::string, std::string> config;
    
    // Read all environment variables with prefix
    for (char** env = ENVIRON; *env != nullptr; ++env) {
        std::string env_str = *env;
        size_t eq_pos = env_str.find('=');
        if (eq_pos == std::string::npos) continue;
        
        std::string key = env_str.substr(0, eq_pos);
        if (key.find(prefix) == 0) {
            std::string value = env_str.substr(eq_pos + 1);
            config[key] = value;
        }
    }
    
    if (config.empty()) {
        return nullptr;
    }
    
    try {
        return from_map(config, prefix);
    } catch (const std::exception&) {
        return nullptr;
    }
}

std::string StrategyConfig::to_lower(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
        [](unsigned char c) { return std::tolower(c); });
    return result;
}

bool StrategyConfig::parse_bool(const std::string& str) {
    std::string lower = to_lower(str);
    return lower == "true" || lower == "1" || lower == "yes" || lower == "on";
}

double StrategyConfig::parse_double(const std::map<std::string, std::string>& config,
                                     const std::string& key, double default_val) {
    auto it = config.find(key);
    if (it == config.end()) return default_val;
    try {
        return std::stod(it->second);
    } catch (...) {
        return default_val;
    }
}

int StrategyConfig::parse_int(const std::map<std::string, std::string>& config,
                               const std::string& key, int default_val) {
    auto it = config.find(key);
    if (it == config.end()) return default_val;
    try {
        return std::stoi(it->second);
    } catch (...) {
        return default_val;
    }
}

std::string StrategyConfig::get_required(const std::map<std::string, std::string>& config,
                                          const std::string& key) {
    auto it = config.find(key);
    if (it == config.end()) {
        throw std::invalid_argument("Missing required key: " + key);
    }
    return it->second;
}

std::shared_ptr<AlertStrategy> StrategyConfig::create_price_alert(
    const std::map<std::string, std::string>& config,
    const std::string& prefix) {
    
    std::string symbol = get_required(config, prefix + "SYMBOL");
    auto builder = PriceAlert::Builder().symbol(symbol);
    
    double above = parse_double(config, prefix + "ABOVE", std::numeric_limits<double>::max());
    if (above != std::numeric_limits<double>::max()) {
        builder.above(above);
    }
    
    double below = parse_double(config, prefix + "BELOW", std::numeric_limits<double>::lowest());
    if (below != std::numeric_limits<double>::lowest()) {
        builder.below(below);
    }
    
    auto recurring_it = config.find(prefix + "RECURRING");
    bool recurring = (recurring_it != config.end()) ? parse_bool(recurring_it->second) : false;
    builder.recurring(recurring);
    
    int cooldown_ms = parse_int(config, prefix + "COOLDOWN_MS", 0);
    if (cooldown_ms > 0) {
        builder.cooldown(std::chrono::milliseconds(cooldown_ms));
    }
    
    return builder.build();
}

std::shared_ptr<AlertStrategy> StrategyConfig::create_volume_spike(
    const std::map<std::string, std::string>& config,
    const std::string& prefix) {
    
    std::string symbols_str = get_required(config, prefix + "SYMBOLS");
    std::vector<std::string> symbols;
    std::istringstream iss(symbols_str);
    std::string symbol;
    while (std::getline(iss, symbol, ',')) {
        // Trim whitespace
        symbol.erase(0, symbol.find_first_not_of(" \t"));
        symbol.erase(symbol.find_last_not_of(" \t") + 1);
        if (!symbol.empty()) {
            symbols.push_back(symbol);
        }
    }
    
    if (symbols.empty()) {
        throw std::invalid_argument("No symbols provided for volume spike strategy");
    }
    
    double multiplier = parse_double(config, prefix + "MULTIPLIER", 2.0);
    int lookback = parse_int(config, prefix + "LOOKBACK", 50);
    
    return VolumeSpike::Builder()
        .symbols(symbols)
        .multiplier(multiplier)
        .lookback(static_cast<size_t>(lookback))
        .build();
}

std::shared_ptr<AlertStrategy> StrategyConfig::create_spread_alert(
    const std::map<std::string, std::string>& config,
    const std::string& prefix) {
    
    std::string symbol = get_required(config, prefix + "SYMBOL");
    auto builder = SpreadAlert::Builder().symbol(symbol);
    
    double min_spread = parse_double(config, prefix + "MIN_SPREAD", 0.0);
    if (min_spread > 0.0) {
        builder.min_spread(min_spread);
    }
    
    double max_spread = parse_double(config, prefix + "MAX_SPREAD", std::numeric_limits<double>::max());
    if (max_spread != std::numeric_limits<double>::max()) {
        builder.max_spread(max_spread);
    }
    
    return builder.build();
}

std::shared_ptr<AlertStrategy> StrategyConfig::create_breakout(
    const std::map<std::string, std::string>& config,
    const std::string& prefix) {
    
    std::string symbol = get_required(config, prefix + "SYMBOL");
    double threshold = parse_double(config, prefix + "THRESHOLD");
    double volume_multiplier = parse_double(config, prefix + "VOLUME_MULTIPLIER", 2.0);
    
    return StrategyPresets::breakout(symbol, threshold, volume_multiplier);
}

std::shared_ptr<AlertStrategy> StrategyConfig::create_support_level(
    const std::map<std::string, std::string>& config,
    const std::string& prefix) {
    
    std::string symbol = get_required(config, prefix + "SYMBOL");
    double level = parse_double(config, prefix + "LEVEL");
    double tolerance = parse_double(config, prefix + "TOLERANCE", 1.0);
    double min_liquidity = parse_double(config, prefix + "MIN_LIQUIDITY", 10.0);
    
    return StrategyPresets::support_level(symbol, level, tolerance, min_liquidity);
}

std::shared_ptr<AlertStrategy> StrategyConfig::create_resistance_level(
    const std::map<std::string, std::string>& config,
    const std::string& prefix) {
    
    std::string symbol = get_required(config, prefix + "SYMBOL");
    double level = parse_double(config, prefix + "LEVEL");
    double tolerance = parse_double(config, prefix + "TOLERANCE", 1.0);
    double min_liquidity = parse_double(config, prefix + "MIN_LIQUIDITY", 10.0);
    
    return StrategyPresets::resistance_level(symbol, level, tolerance, min_liquidity);
}

} // namespace kraken
