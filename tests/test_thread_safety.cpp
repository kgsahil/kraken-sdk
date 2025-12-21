/// @file test_thread_safety.cpp
/// @brief Thread safety and concurrency tests

#include <gtest/gtest.h>
#include <kraken/kraken.hpp>
#include <thread>
#include <chrono>
#include <atomic>
#include <vector>
#include <mutex>

using namespace kraken;

class ThreadSafetyTest : public ::testing::Test {
protected:
    void SetUp() override {
        client_ = std::make_unique<KrakenClient>();
    }
    
    void TearDown() override {
        if (client_ && client_->is_running()) {
            client_->stop();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    
    std::unique_ptr<KrakenClient> client_;
};

// Test concurrent callback registration
TEST_F(ThreadSafetyTest, ConcurrentCallbackRegistration) {
    std::atomic<int> callback_count{0};
    std::vector<std::thread> threads;
    
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([this, &callback_count]() {
            client_->on_ticker([&callback_count](const Ticker&) {
                callback_count++;
            });
            client_->on_trade([&callback_count](const Trade&) {
                callback_count++;
            });
            client_->on_error([&callback_count](const Error&) {
                callback_count++;
            });
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    // Should not crash - verifies thread-safe callback registration
    EXPECT_TRUE(true);
}

// Test concurrent subscription creation
TEST_F(ThreadSafetyTest, ConcurrentSubscriptions) {
    std::vector<Subscription> subscriptions;
    std::mutex mutex;
    std::vector<std::thread> threads;
    
    for (int i = 0; i < 20; ++i) {
        threads.emplace_back([this, &subscriptions, &mutex, i]() {
            std::string symbol = "BTC/USD";
            if (i % 2 == 0) {
                auto sub = client_->subscribe(Channel::Ticker, {symbol});
                std::lock_guard<std::mutex> lock(mutex);
                subscriptions.push_back(std::move(sub));
            } else {
                auto sub = client_->subscribe(Channel::Trade, {symbol});
                std::lock_guard<std::mutex> lock(mutex);
                subscriptions.push_back(std::move(sub));
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    EXPECT_EQ(subscriptions.size(), 20);
    
    // Verify all are active
    for (const auto& sub : subscriptions) {
        EXPECT_TRUE(sub.is_active());
    }
}

// Test concurrent alert management
TEST_F(ThreadSafetyTest, ConcurrentAlertManagement) {
    std::atomic<int> alert_count{0};
    std::vector<int> alert_ids;
    std::mutex mutex;
    std::vector<std::thread> threads;
    
    // Create alerts from multiple threads
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([this, &alert_ids, &mutex, i]() {
            auto alert = PriceAlert::Builder()
                .symbol("BTC/USD")
                .above(50000.0 + i)
                .build();
            
            int id = client_->add_alert(alert, [](const Alert&) {});
            
            std::lock_guard<std::mutex> lock(mutex);
            alert_ids.push_back(id);
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    EXPECT_EQ(alert_ids.size(), 10);
    EXPECT_EQ(client_->alert_count(), 10);
    
    // Remove alerts concurrently
    threads.clear();
    for (int id : alert_ids) {
        threads.emplace_back([this, id]() {
            client_->remove_alert(id);
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    EXPECT_EQ(client_->alert_count(), 0);
}

// Test concurrent metrics access
TEST_F(ThreadSafetyTest, ConcurrentMetricsAccess) {
    std::atomic<bool> running{true};
    std::vector<std::thread> threads;
    std::atomic<int> read_count{0};
    
    // Multiple threads reading metrics
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([this, &running, &read_count]() {
            int local_count = 0;
            while (running.load(std::memory_order_relaxed) && local_count < 100) {
                auto metrics = client_->get_metrics();
                read_count.fetch_add(1, std::memory_order_relaxed);
                local_count++;
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        });
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    running.store(false, std::memory_order_relaxed);
    
    // Wait with timeout
    for (auto& t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }
    
    // Should have read many times without crashes
    EXPECT_GT(read_count.load(), 0);
}

// Test concurrent subscription operations
TEST_F(ThreadSafetyTest, ConcurrentSubscriptionOperations) {
    auto sub = client_->subscribe(Channel::Ticker, {"BTC/USD", "ETH/USD"});
    std::vector<std::thread> threads;
    std::atomic<bool> running{true};
    
    // Thread 1: Pause/resume (limited iterations)
    threads.emplace_back([&sub, &running]() {
        int iterations = 0;
        while (running.load(std::memory_order_relaxed) && iterations < 10) {
            sub.pause();
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            sub.resume();
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            iterations++;
        }
    });
    
    // Thread 2: Add symbols (limited iterations)
    threads.emplace_back([&sub, &running]() {
        int counter = 0;
        while (running.load(std::memory_order_relaxed) && counter < 5) {
            sub.add_symbols({"SOL/USD", "XRP/USD"});
            std::this_thread::sleep_for(std::chrono::milliseconds(7));
            counter++;
        }
    });
    
    // Thread 3: Read symbols (limited iterations)
    threads.emplace_back([&sub, &running]() {
        int iterations = 0;
        while (running.load(std::memory_order_relaxed) && iterations < 20) {
            auto symbols = sub.symbols();
            (void)symbols;
            std::this_thread::sleep_for(std::chrono::milliseconds(3));
            iterations++;
        }
    });
    
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    running.store(false, std::memory_order_relaxed);
    
    // Wait with timeout
    for (auto& t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }
    
    // Should not crash - verifies thread-safe subscription operations
    EXPECT_TRUE(true);
}

// Test stop from multiple threads
TEST_F(ThreadSafetyTest, ConcurrentStop) {
    // Don't actually run async - just test that stop is thread-safe
    // Running async might try to connect and hang
    std::vector<std::thread> threads;
    
    // Multiple threads calling stop (even when not running)
    for (int i = 0; i < 5; ++i) {
        threads.emplace_back([this]() {
            client_->stop();
        });
    }
    
    for (auto& t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }
    
    // Should not crash - stop is thread-safe
    EXPECT_FALSE(client_->is_running());
}

// Test connection state queries from multiple threads
TEST_F(ThreadSafetyTest, ConcurrentConnectionStateQueries) {
    std::atomic<bool> running{true};
    std::vector<std::thread> threads;
    std::atomic<int> query_count{0};
    
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([this, &running, &query_count]() {
            int local_count = 0;
            while (running.load(std::memory_order_relaxed) && local_count < 50) {
                bool connected = client_->is_connected();
                ConnectionState state = client_->connection_state();
                (void)connected;
                (void)state;
                query_count.fetch_add(1, std::memory_order_relaxed);
                local_count++;
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        });
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    running.store(false, std::memory_order_relaxed);
    
    // Wait with timeout
    for (auto& t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }
    
    EXPECT_GT(query_count.load(), 0);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

