#pragma once

#include "kraken/types.hpp"
#include <unordered_map>
#include <string>
#include <cstdint>

namespace kraken {

/// Order book engine with CRC32 checksum validation
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
    
    /// Get current order book for a symbol
    const OrderBook* get(const std::string& symbol) const;
    
    /// Clear all order books
    void clear();
    
    /// Remove a specific symbol's book
    void remove(const std::string& symbol);
    
    /// Calculate CRC32 checksum for an order book (Kraken format)
    static uint32_t calculate_checksum(const OrderBook& book);
    
private:
    /// Merge update into existing book
    void merge_update(OrderBook& book,
                      const std::vector<PriceLevel>& bids,
                      const std::vector<PriceLevel>& asks);
    
    std::unordered_map<std::string, OrderBook> books_;
};

} // namespace kraken

