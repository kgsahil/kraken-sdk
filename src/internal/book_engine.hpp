/// @file book_engine.hpp
/// @brief Internal order book engine
/// 
/// Maintains order book state using incremental updates with CRC32 checksum
/// validation. Uses std::map for O(log n) price level operations.
/// 
/// @note This is an internal header. Users access order books via callbacks.

#pragma once

#include "kraken/types.hpp"
#include <unordered_map>
#include <map>
#include <string>
#include <cstdint>

namespace kraken {

/// @brief Order book engine with CRC32 checksum validation
/// 
/// Maintains order book state for multiple trading pairs using incremental updates.
/// Uses std::map internally for O(log n) price level updates. Validates data
/// integrity using CRC32 checksums as provided by Kraken.
/// 
/// @note This is an internal class. Users access order books via KrakenClient callbacks.
class BookEngine {
public:
    /// @brief Default constructor
    BookEngine() = default;
    
    /// @brief Apply a snapshot or update to the order book
    /// 
    /// Processes order book updates:
    /// - Snapshots: Replace entire book
    /// - Updates: Apply incremental changes (price level additions/modifications/deletions)
    /// 
    /// Validates CRC32 checksum if provided to detect data corruption.
    /// 
    /// @param symbol The trading pair (e.g., "BTC/USD")
    /// @param bids New bid levels (price, quantity pairs)
    /// @param asks New ask levels (price, quantity pairs)
    /// @param is_snapshot True if this is a full snapshot (replace), false if incremental update
    /// @param expected_checksum Expected CRC32 checksum (0 to skip validation)
    /// @return true if checksum validates (or no validation requested), false on mismatch
    bool apply(const std::string& symbol,
               const std::vector<PriceLevel>& bids,
               const std::vector<PriceLevel>& asks,
               bool is_snapshot,
               uint32_t expected_checksum = 0);
    
    /// @brief Get current order book for a symbol
    /// 
    /// Creates a snapshot from internal map structures and returns it.
    /// The returned pointer is valid until the next apply() call for this symbol.
    /// 
    /// @param symbol The trading pair
    /// @return Pointer to OrderBook, or nullptr if symbol not found
    const OrderBook* get(const std::string& symbol) const;
    
    /// @brief Clear all order books
    /// 
    /// Removes all tracked order books. Useful for cleanup or reset.
    void clear();
    
    /// @brief Remove a specific symbol's book
    /// @param symbol The trading pair to remove
    void remove(const std::string& symbol);
    
    /// @brief Calculate CRC32 checksum for an order book (Kraken format)
    /// 
    /// Computes CRC32 checksum according to Kraken's specification:
    /// - Format: "price:quantity" for each level
    /// - Bids: sorted descending by price
    /// - Asks: sorted ascending by price
    /// 
    /// @param book Order book to checksum
    /// @return CRC32 checksum value
    static uint32_t calculate_checksum(const OrderBook& book);
    
private:
    /// @brief Internal book representation using maps for O(log n) operations
    /// 
    /// Uses std::map for efficient price level lookups and updates.
    struct InternalBook {
        std::string symbol;
        std::map<double, double, std::greater<double>> bids;  ///< Bids: price desc (best bid first)
        std::map<double, double> asks;                        ///< Asks: price asc (best ask first)
        uint32_t checksum = 0;                                 ///< Last validated checksum
        bool is_valid = true;                                  ///< Checksum validation status
    };
    
    /// @brief Apply updates to internal map structure - O(log n) per level
    /// 
    /// Updates price levels in the internal map. Zero quantities remove levels.
    /// 
    /// @param book Internal book to update
    /// @param bids Bid level updates
    /// @param asks Ask level updates
    void apply_updates(InternalBook& book,
                       const std::vector<PriceLevel>& bids,
                       const std::vector<PriceLevel>& asks);
    
    /// @brief Convert internal map to OrderBook vector format
    /// 
    /// Creates a snapshot OrderBook from the internal map representation.
    /// 
    /// @param book Internal book to convert
    /// @return OrderBook with vector-based price levels
    OrderBook to_order_book(const InternalBook& book) const;
    
    std::unordered_map<std::string, InternalBook> books_;
    mutable std::unordered_map<std::string, OrderBook> book_cache_; // Cache for get()
};

} // namespace kraken

