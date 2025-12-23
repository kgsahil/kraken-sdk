#pragma once

/// @file gap_detector.hpp
/// @brief Message gap detection for WebSocket streams
/// 
/// Detects missing or out-of-order messages by tracking sequence numbers.
/// Reports gaps via callback for recovery actions.

#include "types.hpp"
#include <string>
#include <unordered_map>
#include <functional>
#include <mutex>
#include <chrono>
#include <cstdint>
#include <array>
#include <cstdio>

namespace kraken {

//------------------------------------------------------------------------------
// Optimized Key for Channel/Symbol Lookups
//------------------------------------------------------------------------------

/// Composite key for channel+symbol (avoids string concatenation on hot path)
struct ChannelSymbolKey {
    std::string channel;
    std::string symbol;
    
    bool operator==(const ChannelSymbolKey& other) const {
        return channel == other.channel && symbol == other.symbol;
    }
};

/// Hash function for ChannelSymbolKey
struct ChannelSymbolKeyHash {
    size_t operator()(const ChannelSymbolKey& key) const {
        // Combine hashes using FNV-1a style mixing
        size_t h1 = std::hash<std::string>{}(key.channel);
        size_t h2 = std::hash<std::string>{}(key.symbol);
        return h1 ^ (h2 + 0x9e3779b9U + ((h1 << 6U) | (h1 >> 2U)));
    }
};

//------------------------------------------------------------------------------
// Gap Information
//------------------------------------------------------------------------------

/// Information about a detected message gap
struct GapInfo {
    std::string channel;           ///< Channel where gap occurred (e.g., "ticker", "book")
    std::string symbol;            ///< Symbol affected (e.g., "BTC/USD")
    uint64_t expected_seq{0};      ///< Expected sequence number
    uint64_t actual_seq{0};        ///< Actual sequence number received
    uint64_t gap_size{0};          ///< Number of missing messages
    std::chrono::system_clock::time_point timestamp;  ///< When gap was detected
    
    /// Check if this is a reorder (actual < expected) vs a skip (actual > expected)
    bool is_reorder() const { return actual_seq < expected_seq; }
    
    /// Convert to JSON for logging/monitoring
    std::string to_json() const {
        auto ts_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            timestamp.time_since_epoch()).count();
        std::array<char, 512> buf{};
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg,hicpp-vararg) - snprintf is safe here
        const int result = snprintf(buf.data(), buf.size(),
            R"({"channel":"%s","symbol":"%s","expected":%llu,"actual":%llu,"gap_size":%llu,"is_reorder":%s,"timestamp_ms":%lld})",
            channel.c_str(), symbol.c_str(),
            static_cast<unsigned long long>(expected_seq),
            static_cast<unsigned long long>(actual_seq),
            static_cast<unsigned long long>(gap_size),
            is_reorder() ? "true" : "false",
            static_cast<long long>(ts_ms));
        (void)result;  // Suppress unused result warning
        return {buf.data()};
    }
};

/// Callback for gap detection events
using GapCallback = std::function<void(const GapInfo&)>;

//------------------------------------------------------------------------------
// Recovery Action
//------------------------------------------------------------------------------

/// Action to take when a gap is detected
enum class GapRecoveryAction {
    Ignore,      ///< Log and continue (default for non-critical data)
    Reconnect,   ///< Force reconnect to get fresh state
    Snapshot,    ///< Request full snapshot (for order books)
    Callback     ///< Let user decide via callback
};

//------------------------------------------------------------------------------
// Sequence Tracker
//------------------------------------------------------------------------------

/// Tracks sequence numbers per channel/symbol to detect gaps
/// 
/// Thread-safe: Uses internal mutex for concurrent access.
/// 
/// @example
/// SequenceTracker tracker;
/// tracker.on_gap([](const GapInfo& gap) {
///     std::cerr << "Gap detected: " << gap.to_json() << std::endl;
/// });
/// 
/// // In message processing loop:
/// if (!tracker.check("ticker", "BTC/USD", seq_num)) {
///     // Gap was detected and callback fired
/// }
class SequenceTracker {
public:
    /// Configuration for sequence tracking
    struct Config {
        bool enabled = true;           ///< Enable/disable gap detection
        int gap_tolerance = 0;         ///< Allow up to N missing before reporting
        bool track_reorders = true;    ///< Also detect out-of-order messages
        GapRecoveryAction recovery = GapRecoveryAction::Callback;
    };
    
    SequenceTracker() = default;
    explicit SequenceTracker(Config config) : config_(config) {}
    
    /// Set gap detection callback
    void on_gap(GapCallback callback) {
        std::lock_guard<std::mutex> lock(mutex_);
        gap_callback_ = std::move(callback);
    }
    
    /// Check sequence number, returns true if OK, false if gap detected
    /// @param channel Channel name (e.g., "ticker", "book")
    /// @param symbol Symbol name (e.g., "BTC/USD")
    /// @param sequence Sequence number from message
    /// @return true if sequence is valid, false if gap detected
    bool check(const std::string& channel, const std::string& symbol, uint64_t sequence) {
        if (!config_.enabled) return true;
        
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Use struct key to avoid string concatenation on hot path
        ChannelSymbolKey key{channel, symbol};
        auto it = last_seq_.find(key);
        
        if (it == last_seq_.end()) {
            // First message for this channel/symbol - move key into map
            last_seq_.emplace(std::move(key), sequence);
            return true;
        }
        
        uint64_t expected = it->second + 1;
        
        if (sequence == expected) {
            // Perfect, update and return
            it->second = sequence;
            return true;
        }
        
        // Gap or reorder detected
        if (sequence > expected) {
            // Forward gap (missed messages)
            uint64_t gap_size = sequence - expected;
            
            if (gap_size <= static_cast<uint64_t>(config_.gap_tolerance)) {
                // Within tolerance, accept
                it->second = sequence;
                return true;
            }
            
            // Gap exceeds tolerance - report
            report_gap(channel, symbol, expected, sequence, gap_size);
            it->second = sequence;  // Accept anyway to avoid cascade
            return false;
        }
        
        if (config_.track_reorders) {
            // Backward gap (reordered message)
            uint64_t gap_size = expected - sequence - 1;
            report_gap(channel, symbol, expected, sequence, gap_size);
            // Don't update last_seq for reorders
            return false;
        }
        
        return true;
    }
    
    /// Reset tracking for a specific channel/symbol (e.g., after snapshot)
    void reset(const std::string& channel, const std::string& symbol) {
        std::lock_guard<std::mutex> lock(mutex_);
        last_seq_.erase(ChannelSymbolKey{channel, symbol});
    }
    
    /// Reset all tracking (e.g., after reconnect)
    void reset_all() {
        std::lock_guard<std::mutex> lock(mutex_);
        last_seq_.clear();
        gap_count_ = 0;
    }
    
    /// Get total number of gaps detected
    uint64_t gap_count() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return gap_count_;
    }
    
    /// Get configuration
    const Config& config() const { return config_; }
    
    /// Update configuration
    void set_config(Config config) {
        std::lock_guard<std::mutex> lock(mutex_);
        config_ = config;
    }

private:
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - Intentional parameter order
    void report_gap(const std::string& channel, const std::string& symbol,
                   uint64_t expected, uint64_t actual, uint64_t gap_size) {
        gap_count_++;
        
        if (gap_callback_) {
            GapInfo info;
            info.channel = channel;
            info.symbol = symbol;
            info.expected_seq = expected;
            info.actual_seq = actual;
            info.gap_size = gap_size;
            info.timestamp = std::chrono::system_clock::now();
            
            try {
                gap_callback_(info);
            } catch (...) {
                // Callback threw - ignore
            }
        }
    }
    
    Config config_;
    mutable std::mutex mutex_;
    std::unordered_map<ChannelSymbolKey, uint64_t, ChannelSymbolKeyHash> last_seq_;
    uint64_t gap_count_ = 0;
    GapCallback gap_callback_;
};

} // namespace kraken

