#pragma once

#include "kraken/types.hpp"
#include <unordered_map>
#include <map>
#include <string>
#include <cstdint>

namespace kraken {

/// Order book engine with CRC32 checksum validation
/// Uses std::map internally for O(log n) operations
class BookEngine {
public:
    BookEngine() = default;
    
    /// Apply a snapshot or update to the order book
    /// @param symbol The trading pair
    /// @param bids New bid levels
    /// @param asks New ask levels
    /// @param is_snapshot True if this is a full snapshot
    /// @param expected_checksum Expected CRC32 checksum (0 to skip validation)
    /// @return true if checksum validates (or no validation requested)
    bool apply(const std::string& symbol,
               const std::vector<PriceLevel>& bids,
               const std::vector<PriceLevel>& asks,
               bool is_snapshot,
               uint32_t expected_checksum = 0);
    
    /// Get current order book for a symbol (creates snapshot from maps)
    const OrderBook* get(const std::string& symbol) const;
    
    /// Clear all order books
    void clear();
    
    /// Remove a specific symbol's book
    void remove(const std::string& symbol);
    
    /// Calculate CRC32 checksum for an order book (Kraken format)
    static uint32_t calculate_checksum(const OrderBook& book);
    
private:
    /// Internal book representation using maps for O(log n) operations
    struct InternalBook {
        std::string symbol;
        std::map<double, double, std::greater<double>> bids;  // Price desc (best bid first)
        std::map<double, double> asks;                        // Price asc (best ask first)
        uint32_t checksum = 0;
        bool is_valid = true;
    };
    
    /// Apply updates to internal map structure - O(log n) per level
    void apply_updates(InternalBook& book,
                       const std::vector<PriceLevel>& bids,
                       const std::vector<PriceLevel>& asks);
    
    /// Convert internal map to OrderBook vector format
    OrderBook to_order_book(const InternalBook& book) const;
    
    std::unordered_map<std::string, InternalBook> books_;
    mutable std::unordered_map<std::string, OrderBook> book_cache_; // Cache for get()
};

} // namespace kraken

