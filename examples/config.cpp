/// @file config.cpp
/// @brief Implementation of config file loader

#include "config.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <cstdlib>
#include <algorithm>
#include <cstring>

namespace examples {

//------------------------------------------------------------------------------
// Internal Utilities
//------------------------------------------------------------------------------

namespace {
    /// Trim whitespace from both ends of a string
    /// Handles spaces, tabs, and line endings (\r, \n) for cross-platform compatibility
    inline std::string trim(const std::string& str) {
        if (str.empty()) return str;
        
        // Trim all whitespace including \r and \n (for Windows CRLF line endings)
        // std::getline removes \n but may leave \r on Windows files
        size_t start = str.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) return "";
        
        size_t end = str.find_last_not_of(" \t\r\n");
        if (end == std::string::npos || end < start) return "";
        
        return str.substr(start, end - start + 1);
    }
    
    /// Remove surrounding quotes from a string if present
    inline std::string unquote(const std::string& str) {
        if (str.size() >= 2) {
            if ((str[0] == '"' && str[str.size() - 1] == '"') ||
                (str[0] == '\'' && str[str.size() - 1] == '\'')) {
                return str.substr(1, str.size() - 2);
            }
        }
        return str;
    }
    
    /// Set environment variable (cross-platform)
    inline void set_env_var(const std::string& key, const std::string& value) {
        #ifdef _WIN32
            _putenv_s(key.c_str(), value.c_str());
        #else
            setenv(key.c_str(), value.c_str(), 1);  // Overwrite existing
        #endif
    }
    
    /// Get the path to the example config file
    /// Tries multiple locations: current dir, examples dir, build dir
    std::string find_example_config_file() {
        // Try current directory first
        std::ifstream test1("config.cfg.example");
        if (test1.good()) {
            return "config.cfg.example";
        }
        
        // Try examples directory (relative to source)
        std::ifstream test2("examples/config.cfg.example");
        if (test2.good()) {
            return "examples/config.cfg.example";
        }
        
        // Try build directory
        std::ifstream test3("../examples/config.cfg.example");
        if (test3.good()) {
            return "../examples/config.cfg.example";
        }
        
        // Return empty if not found (will use fallback)
        return "";
    }
    
    /// Read config template from file or return fallback
    std::string get_config_template() {
        std::string file_path = find_example_config_file();
        
        if (!file_path.empty()) {
            std::ifstream file(file_path);
            if (file.is_open()) {
                std::string content((std::istreambuf_iterator<char>(file)),
                                   std::istreambuf_iterator<char>());
                if (!content.empty()) {
                    return content;
                }
            }
        }
        
        // Fallback: return minimal template if file not found
        // This can happen if examples are installed separately
        return R"(# Kraken SDK Configuration File
# Format: KEY=VALUE (one per line)
# See examples/config.cfg.example for full template

KRAKEN_WS_URL=wss://ws.kraken.com/v2
ENABLE_SPSC_QUEUE=true
SPSC_QUEUE_SIZE=131072
LOG_LEVEL=info
LOG_CONSOLE=true
)";
    }
    
    /// Parse a single line from config file
    /// @return pair of (key, value) or empty strings if line should be skipped
    inline std::pair<std::string, std::string> parse_config_line(
        const std::string& line, int line_num, bool& is_valid) {
        is_valid = false;  // Default to invalid until proven valid
        
        std::string trimmed = trim(line);
        
        // Skip empty lines (silently - no warning)
        // Must check empty FIRST before any other checks
        if (trimmed.empty()) {
            return {"", ""};
        }
        
        // Skip comments (silently - no warning)
        if (trimmed[0] == '#') {
            return {"", ""};
        }
        
        // Parse KEY=VALUE - line has content and is not a comment
        size_t eq_pos = trimmed.find('=');
        if (eq_pos == std::string::npos) {
            // Line has content but no '=' - this is an error worth warning about
            // (This should not happen for properly formatted config files)
            std::cerr << "Warning: Invalid line " << line_num 
                      << " in config file (missing '='): " << trimmed << std::endl;
            return {"", ""};
        }
        
        // Line is valid - has '=' and is not empty/comment
        is_valid = true;
        
        std::string key = trim(trimmed.substr(0, eq_pos));
        std::string value = unquote(trim(trimmed.substr(eq_pos + 1)));
        
        if (key.empty()) {
            std::cerr << "Warning: Empty key on line " << line_num << std::endl;
            is_valid = false;
            return {"", ""};
        }
        
        return {key, value};
    }
}

//------------------------------------------------------------------------------
// Config File Loading
//------------------------------------------------------------------------------

bool load_config_file(const std::string& file_path) {
    // Validate file path
    if (file_path.empty()) {
        throw std::invalid_argument("Config file path cannot be empty");
    }
    
    std::ifstream file(file_path);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open config file: " + file_path);
    }
    
    std::string line;
    int line_num = 0;
    int loaded_count = 0;
    
    while (std::getline(file, line)) {
        line_num++;
        
        bool is_valid = true;
        auto [key, value] = parse_config_line(line, line_num, is_valid);
        
        if (!is_valid || key.empty()) {
            continue;
        }
        
        // Set environment variable
        set_env_var(key, value);
        loaded_count++;
    }
    
    std::cout << "Loaded " << loaded_count << " configuration variables from: " 
              << file_path << std::endl;
    
    return true;
}

std::optional<std::string> parse_config_arg(int argc, char* argv[]) {
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        // Check for --config=path format
        if (arg.find("--config=") == 0) {
            return arg.substr(9);  // Skip "--config="
        }
        
        // Check for --config path format
        if (arg == "--config" && i + 1 < argc) {
            return std::string(argv[i + 1]);
        }
        
        // Check for -c shorthand
        if (arg == "-c" && i + 1 < argc) {
            return std::string(argv[i + 1]);
        }
    }
    
    return std::nullopt;
}

bool load_config_from_args(int argc, char* argv[]) {
    auto config_path = parse_config_arg(argc, argv);
    
    if (!config_path.has_value()) {
        return false;
    }
    
    load_config_file(config_path.value());
    return true;
}

//------------------------------------------------------------------------------
// Config File Utilities
//------------------------------------------------------------------------------

bool validate_config_file(const std::string& file_path) {
    if (file_path.empty()) {
        return false;
    }
    
    std::ifstream file(file_path);
    if (!file.is_open()) {
        return false;
    }
    
    std::string line;
    int line_num = 0;
    
    while (std::getline(file, line)) {
        line_num++;
        
        bool is_valid = true;
        auto [key, value] = parse_config_line(line, line_num, is_valid);
        
        // If line is not a comment/empty and is invalid, validation fails
        if (!is_valid && !trim(line).empty() && trim(line)[0] != '#') {
            return false;
        }
    }
    
    return true;
}

void print_config_template() {
    std::cout << get_config_template();
}

bool create_sample_config(const std::string& file_path) {
    if (file_path.empty()) {
        return false;
    }
    
    std::ofstream file(file_path);
    if (!file.is_open()) {
        return false;
    }
    
    file << get_config_template();
    return true;
}

} // namespace examples

