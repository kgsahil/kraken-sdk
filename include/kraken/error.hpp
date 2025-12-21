#pragma once

#include "types.hpp"
#include <string>
#include <stdexcept>
#include <functional>

namespace kraken {

//------------------------------------------------------------------------------
// Error Information
//------------------------------------------------------------------------------

/// Error information passed to callbacks
struct Error {
    ErrorCode code = ErrorCode::None;
    std::string message;
    std::string details;
    
    /// Check if this represents an error
    explicit operator bool() const { return code != ErrorCode::None; }
};

/// Error callback type
using ErrorCallback = std::function<void(const Error&)>;

/// Connection state callback type
using ConnectionStateCallback = std::function<void(ConnectionState)>;

//------------------------------------------------------------------------------
// Exception Types
//------------------------------------------------------------------------------

/// Base exception for Kraken SDK errors
class KrakenException : public std::runtime_error {
public:
    explicit KrakenException(const std::string& message)
        : std::runtime_error(message) {}
    
    KrakenException(ErrorCode code, const std::string& message)
        : std::runtime_error(message), code_(code) {}
    
    ErrorCode code() const { return code_; }
    
private:
    ErrorCode code_ = ErrorCode::None;
};

/// Connection failed exception
class ConnectionError : public KrakenException {
public:
    explicit ConnectionError(const std::string& message)
        : KrakenException(ErrorCode::ConnectionFailed, message) {}
};

/// Authentication failed exception
class AuthenticationError : public KrakenException {
public:
    explicit AuthenticationError(const std::string& message)
        : KrakenException(ErrorCode::AuthenticationFailed, message) {}
};

/// Parse error exception
class ParseError : public KrakenException {
public:
    explicit ParseError(const std::string& message)
        : KrakenException(ErrorCode::ParseError, message) {}
};

} // namespace kraken

