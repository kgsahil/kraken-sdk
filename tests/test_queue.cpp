/// @file test_queue.cpp
/// @brief Unit tests for message queue (SPSC queue)

#include <gtest/gtest.h>
#include "kraken/queue.hpp"
#include "../src/queue.cpp"  // Include template implementation
#include <thread>
#include <vector>
#include <atomic>

using namespace kraken;

// Test with simple integer type
using TestQueue = DefaultMessageQueue<int>;

class QueueTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Set up test fixtures
    }
    
    void TearDown() override {
        // Clean up
    }
};

// Test basic push/pop
TEST_F(QueueTest, BasicPushPop) {
    TestQueue queue(1024);
    
    // Push some values
    EXPECT_TRUE(queue.try_push(1));
    EXPECT_TRUE(queue.try_push(2));
    EXPECT_TRUE(queue.try_push(3));
    
    // Pop and verify
    int* val = queue.front();
    ASSERT_NE(val, nullptr);
    EXPECT_EQ(*val, 1);
    queue.pop();
    
    val = queue.front();
    ASSERT_NE(val, nullptr);
    EXPECT_EQ(*val, 2);
    queue.pop();
    
    val = queue.front();
    ASSERT_NE(val, nullptr);
    EXPECT_EQ(*val, 3);
    queue.pop();
    
    // Queue should be empty
    EXPECT_EQ(queue.front(), nullptr);
}

// Test queue capacity
TEST_F(QueueTest, QueueCapacity) {
    TestQueue queue(4);  // Small capacity
    
    // Fill queue
    EXPECT_TRUE(queue.try_push(1));
    EXPECT_TRUE(queue.try_push(2));
    EXPECT_TRUE(queue.try_push(3));
    EXPECT_TRUE(queue.try_push(4));
    
    // Next push should fail (queue full)
    EXPECT_FALSE(queue.try_push(5));
    
    // Pop one
    queue.pop();
    
    // Now should be able to push
    EXPECT_TRUE(queue.try_push(5));
}

// Test empty queue
TEST_F(QueueTest, EmptyQueue) {
    TestQueue queue(1024);
    
    EXPECT_EQ(queue.front(), nullptr);
    EXPECT_EQ(queue.size(), 0);
}

// Test size tracking
TEST_F(QueueTest, SizeTracking) {
    TestQueue queue(1024);
    
    EXPECT_EQ(queue.size(), 0);
    
    ASSERT_TRUE(queue.try_push(1));
    EXPECT_GE(queue.size(), 1);
    
    ASSERT_TRUE(queue.try_push(2));
    EXPECT_GE(queue.size(), 2);
    
    queue.pop();
    EXPECT_GE(queue.size(), 1);
    
    queue.pop();
    EXPECT_EQ(queue.size(), 0);
}

// Test SPSC (single producer, single consumer) behavior
TEST_F(QueueTest, SPSCBehavior) {
    TestQueue queue(1024);
    std::atomic<int> produced{0};
    std::atomic<int> consumed{0};
    
    // Producer thread
    std::thread producer([&]() {
        for (int i = 0; i < 1000; ++i) {
            while (!queue.try_push(i)) {
                std::this_thread::yield();
            }
            produced++;
        }
    });
    
    // Consumer thread
    std::thread consumer([&]() {
        for (int i = 0; i < 1000; ++i) {
            int* val = nullptr;
            while ((val = queue.front()) == nullptr) {
                std::this_thread::yield();
            }
            EXPECT_EQ(*val, i);
            queue.pop();
            consumed++;
        }
    });
    
    producer.join();
    consumer.join();
    
    EXPECT_EQ(produced.load(), 1000);
    EXPECT_EQ(consumed.load(), 1000);
    EXPECT_EQ(queue.size(), 0);
}

// Test high throughput
TEST_F(QueueTest, HighThroughput) {
    TestQueue queue(65536);
    const int count = 10000;
    
    // Push many values
    for (int i = 0; i < count; ++i) {
        EXPECT_TRUE(queue.try_push(i));
    }
    
    // Pop all
    for (int i = 0; i < count; ++i) {
        int* val = queue.front();
        ASSERT_NE(val, nullptr);
        EXPECT_EQ(*val, i);
        queue.pop();
    }
    
    EXPECT_EQ(queue.front(), nullptr);
}

// Test move semantics
TEST_F(QueueTest, MoveSemantics) {
    TestQueue queue1(1024);
    queue1.try_push(42);
    
    // Move queue
    TestQueue queue2 = std::move(queue1);
    
    // queue2 should have the value
    int* val = queue2.front();
    ASSERT_NE(val, nullptr);
    EXPECT_EQ(*val, 42);
}

// Test with different types (string)
using StringQueue = DefaultMessageQueue<std::string>;

TEST_F(QueueTest, StringQueue) {
    StringQueue queue(1024);
    
    queue.try_push("hello");
    queue.try_push("world");
    
    std::string* val = queue.front();
    ASSERT_NE(val, nullptr);
    EXPECT_EQ(*val, "hello");
    queue.pop();
    
    val = queue.front();
    ASSERT_NE(val, nullptr);
    EXPECT_EQ(*val, "world");
    queue.pop();
}

