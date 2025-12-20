/// @file orderbook.cpp
/// @brief Order book with CRC32 checksum validation demo
///
/// Demonstrates real-time order book tracking with Kraken's checksum validation.
///
/// Usage: ./orderbook
/// Press Ctrl+C to exit

#include <kraken/kraken.hpp>
#include <iostream>
#include <iomanip>
#include <csignal>

std::unique_ptr<kraken::KrakenClient> g_client;

void signal_handler(int) {
    if (g_client) g_client->stop();
}

void print_book(const kraken::OrderBook& book) {
    std::cout << "\n╔═══════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║             ORDER BOOK: " << std::left << std::setw(30) << book.symbol << "║" << std::endl;
    std::cout << "╠══════════════════════════╦════════════════════════════╣" << std::endl;
    std::cout << "║          BIDS            ║           ASKS             ║" << std::endl;
    std::cout << "╠══════════════════════════╬════════════════════════════╣" << std::endl;
    
    size_t depth = 5;
    for (size_t i = 0; i < depth; ++i) {
        // Bid side
        if (i < book.bids.size()) {
            std::cout << "║ " << std::fixed << std::setprecision(2)
                      << std::setw(10) << book.bids[i].quantity << " @ "
                      << std::setw(10) << book.bids[i].price << " ";
        } else {
            std::cout << "║                          ";
        }
        
        std::cout << "║ ";
        
        // Ask side
        if (i < book.asks.size()) {
            std::cout << std::setw(10) << book.asks[i].price << " @ "
                      << std::setw(10) << book.asks[i].quantity;
        } else {
            std::cout << "                        ";
        }
        std::cout << " ║" << std::endl;
    }
    
    std::cout << "╠══════════════════════════╩════════════════════════════╣" << std::endl;
    
    // Show spread
    double spread = book.spread();
    double mid = book.mid_price();
    
    std::cout << "║ Spread: $" << std::fixed << std::setprecision(2) 
              << std::setw(8) << spread
              << "     Mid: $" << std::setw(10) << mid << "         ║" << std::endl;
    
    // Show checksum status
    std::cout << "║ Checksum: " << (book.is_valid ? "✓ VALID" : "✗ INVALID")
              << " (0x" << std::hex << book.checksum << std::dec << ")";
    std::cout << std::string(28 - (book.is_valid ? 7 : 9), ' ') << "║" << std::endl;
    
    std::cout << "╚═══════════════════════════════════════════════════════╝" << std::endl;
}

int main() {
    std::cout << "=== Kraken SDK Order Book Demo ===" << std::endl;
    std::cout << "Showing BTC/USD order book with checksum validation\n" << std::endl;
    
    g_client = std::make_unique<kraken::KrakenClient>(
        kraken::ClientConfig::Builder()
            .validate_checksums(true)
            .build()
    );
    
    std::signal(SIGINT, signal_handler);
    
    // Order book callback
    int update_count = 0;
    g_client->on_book([&update_count](const std::string& symbol, const kraken::OrderBook& book) {
        // Print every 5th update to reduce noise
        if (++update_count % 5 == 0) {
            // Clear screen (ANSI escape)
            std::cout << "\033[2J\033[H";
            print_book(book);
            std::cout << "\nUpdates: " << update_count 
                      << " (showing every 5th update)" << std::endl;
        }
    });
    
    // Error handling
    g_client->on_error([](const kraken::Error& e) {
        if (e.code == kraken::ErrorCode::ChecksumMismatch) {
            std::cerr << "⚠️  CHECKSUM MISMATCH - requesting new snapshot" << std::endl;
        } else {
            std::cerr << "Error: " << e.message << std::endl;
        }
    });
    
    g_client->on_connection_state([](kraken::ConnectionState state) {
        std::cout << "[" << kraken::to_string(state) << "]" << std::endl;
    });
    
    // Subscribe to order book with depth 10
    g_client->subscribe_book({"BTC/USD"}, 10);
    
    // Run
    g_client->run();
    
    std::cout << "\nGoodbye!" << std::endl;
    return 0;
}

