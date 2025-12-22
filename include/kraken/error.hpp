/// @file error.hpp
/// @brief Error handling and exception types
/// 
/// Defines error codes, error structures, and exception classes for SDK error handling.

#pragma once

#include "types.hpp"
#include <string>
#include <stdexcept>
#include <functional>

namespace kraken {

//------------------------------------------------------------------------------
// Error Information
//------------------------------------------------------------------------------

/// @brief Error information passed to error callbacks
/// 
/// Contains error code, message, and optional details for debugging.
struct Error {
    ErrorCode code = ErrorCode::None;  ///< Error code category  // NOLINT(misc-non-private-member-variables-in-classes)
    std::string message;                ///< Human-readable error message  // NOLINT(misc-non-private-member-variables-in-classes)
    std::string details;                ///< Additional error details (optional)  // NOLINT(misc-non-private-member-variables-in-classes)
    
    /// @brief Check if this represents an error
    /// @return true if code is not ErrorCode::None
    explicit operator bool() const { return code != ErrorCode::None; }
};

/// @brief Error callback type
/// 
/// Called when runtime errors occur (connection failures, parse errors, etc.).
/// 
/// @param error Error information
using ErrorCallback = std::function<void(const Error&)>;

/// @brief Connection state callback type
/// 
/// Called when the connection state changes (connecting, connected, disconnected, etc.).
/// 
/// @param state New connection state
using ConnectionStateCallback = std::function<void(ConnectionState)>;

//------------------------------------------------------------------------------
// Exception Types
//------------------------------------------------------------------------------

/// @brief Base exception for Kraken SDK errors
/// 
/// All SDK exceptions derive from this class. Provides error code and message.
class KrakenException : public std::runtime_error {
public:
    /// @brief Construct with message only
    /// @param message Error message
    explicit KrakenException(const std::string& message)
        : std::runtime_error(message) {}
    
    /// @brief Construct with error code and message
    /// @param code Error code
    /// @param message Error message
    KrakenException(ErrorCode code, const std::string& message)
        : std::runtime_error(message), code_(code) {}
    
    /// @brief Get the error code
    /// @return Error code
    ErrorCode code() const { return code_; }
    
private:
    ErrorCode code_ = ErrorCode::None;
};

/// @brief Connection failed exception
/// 
/// Thrown when WebSocket connection cannot be established.
class ConnectionError : public KrakenException {
public:
    /// @brief Construct with error message
    /// @param message Error message
    explicit ConnectionError(const std::string& message)
        : KrakenException(ErrorCode::ConnectionFailed, message) {}
};

/// @brief Authentication failed exception
/// 
/// Thrown when API key/secret authentication fails.
class AuthenticationError : public KrakenException {
public:
    /// @brief Construct with error message
    /// @param message Error message
    explicit AuthenticationError(const std::string& message)
        : KrakenException(ErrorCode::AuthenticationFailed, message) {}
};

/// @brief Parse error exception
/// 
/// Thrown when JSON parsing fails or message format is invalid.
class ParseError : public KrakenException {
public:
    /// @brief Construct with error message
    /// @param message Error message
    explicit ParseError(const std::string& message)
        : KrakenException(ErrorCode::ParseError, message) {}
};

} // namespace kraken

