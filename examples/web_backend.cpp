/// @file web_backend.cpp
/// @brief Real-world example: Send market data to web backend
///
/// Demonstrates sending real-time market data to a REST API endpoint.
/// This is a common pattern for building web dashboards or mobile apps.
///
/// Usage: ./web_backend [api_url]
/// Press Ctrl+C to exit
///
/// Note: This example shows the pattern. In production, you would use
/// a proper HTTP client library (libcurl, cpp-httplib, etc.)

#include "common.hpp"
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <fstream>
#include <iostream>
#include <atomic>

// Simple HTTP client (simulated - in production use libcurl or similar)
class HttpClient {
public:
    HttpClient(const std::string& base_url) : base_url_(base_url) {
        // In production, initialize HTTP client library here
        std::cout << "[HTTP Client] Initialized for: " << base_url_ << std::endl;
    }
    
    bool post(const std::string& endpoint, const std::string& json_data) {
        // In production, this would make an actual HTTP POST request
        // For demo purposes, we'll just log it
        static int request_count = 0;
        request_count++;
        
        if (request_count % 10 == 0) {
            std::cout << "\r[HTTP] POST " << endpoint 
                      << " (total requests: " << request_count << ")" << std::flush;
        }
        
        // Example: In production, you would do:
        // curl_easy_setopt(curl, CURLOPT_URL, (base_url_ + endpoint).c_str());
        // curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_data.c_str());
        // curl_easy_perform(curl);
        
        return true;
    }
    
private:
    std::string base_url_;
};

// Message queue for batching sends
std::queue<std::string> message_queue;
std::mutex queue_mutex;
std::condition_variable queue_cv;
std::atomic<bool> running{true};

void background_sender(HttpClient& client) {
    while (running) {
        std::unique_lock<std::mutex> lock(queue_mutex);
        queue_cv.wait_for(lock, std::chrono::seconds(1), [] { 
            return !message_queue.empty() || !running; 
        });
        
        // Batch send up to 10 messages
        int batch_size = 0;
        while (!message_queue.empty() && batch_size < 10) {
            std::string json = message_queue.front();
            message_queue.pop();
            lock.unlock();
            
            // Send to backend API
            client.post("/api/tickers", json);
            
            batch_size++;
            lock.lock();
        }
    }
}

int main(int argc, char* argv[]) {
    // Load config file if provided
    try {
        examples::load_config_from_args(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << "Error loading config file: " << e.what() << std::endl;
        std::cerr << "Usage: " << argv[0] << " [api_url] [--config=path/to/config.cfg]" << std::endl;
        return 1;
    }
    
    // Parse API URL argument (skip --config if present)
    std::string api_url = "http://localhost:8080";
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg.find("--config=") != 0 && arg != "--config") {
            api_url = arg;
            break;
        }
        if (arg == "--config" && i + 1 < argc) {
            i++;  // Skip the config path
            continue;
        }
    }
    
    std::cout << "=== Web Backend Integration ===" << std::endl;
    std::cout << "Sending market data to: " << api_url << std::endl;
    std::cout << "Press Ctrl+C to stop\n" << std::endl;
    
    HttpClient http_client(api_url);
    
    // Start background sender thread
    std::thread sender_thread([&http_client]() {
        background_sender(http_client);
    });
    
    // Create client
    examples::g_client = examples::create_default_client();
    examples::setup_signal_handlers();
    examples::setup_minimal_callbacks(*examples::g_client);
    
    // Send ticker data to backend
    std::atomic<int> sent_count{0};
    examples::g_client->on_ticker([&sent_count](const kraken::Ticker& ticker) {
        // Convert to JSON
        std::string json = ticker.to_json();
        
        // Queue for background sending
        {
            std::lock_guard<std::mutex> lock(queue_mutex);
            message_queue.push(json);
        }
        queue_cv.notify_one();
        
        int count = sent_count.fetch_add(1, std::memory_order_relaxed) + 1;
        if (count % 50 == 0) {
            std::cout << "\rSent " << count << " ticker updates to backend..." << std::flush;
        }
    });
    
    // Subscribe
    examples::g_client->subscribe(kraken::Channel::Ticker, {
        "BTC/USD", "ETH/USD", "SOL/USD"
    });
    
    // Run
    examples::g_client->run_async();
    
    // Wait for shutdown
    while (examples::g_running && examples::g_client->is_running()) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    // Stop background sender
    running = false;
    queue_cv.notify_all();
    if (sender_thread.joinable()) {
        sender_thread.join();
    }
    
    std::cout << "\n\nTotal updates sent: " << sent_count.load() << std::endl;
    
    return 0;
}

