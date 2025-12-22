/// @file config.hpp
/// @brief Configuration file loader for examples
///
/// Supports loading environment variables from config files.
/// Config files use KEY=VALUE format (one per line).
///
/// Usage:
///   ./dashboard --config=path/to/config.cfg
///   ./quickstart --config=./myconfig.cfg

#pragma once

#include <string>
#include <vector>
#include <optional>

namespace examples {

//------------------------------------------------------------------------------
// Config File Loading
//------------------------------------------------------------------------------

/// Load environment variables from a config file
/// Config file format: KEY=VALUE (one per line)
/// Lines starting with # are treated as comments
/// Empty lines are ignored
///
/// @param file_path Path to config file
/// @return true if file was loaded successfully, false otherwise
/// @throws std::runtime_error if file cannot be read
bool load_config_file(const std::string& file_path);

/// Parse command-line arguments for --config option
/// @param argc Argument count
/// @param argv Argument vector
/// @return Path to config file if --config= was provided, empty otherwise
std::optional<std::string> parse_config_arg(int argc, char* argv[]);

/// Load config file from command-line arguments
/// Automatically parses --config= argument and loads the file
/// @param argc Argument count
/// @param argv Argument vector
/// @return true if config file was loaded, false if no --config= provided
/// @throws std::runtime_error if config file cannot be read
bool load_config_from_args(int argc, char* argv[]);

//------------------------------------------------------------------------------
// Config File Utilities
//------------------------------------------------------------------------------

/// Validate config file format
/// @param file_path Path to config file
/// @return true if valid, false otherwise
bool validate_config_file(const std::string& file_path);

/// Print config file template to stdout
void print_config_template();

/// Create a sample config file
/// @param file_path Path where to create the sample config file
/// @return true if created successfully, false otherwise
bool create_sample_config(const std::string& file_path);

} // namespace examples

