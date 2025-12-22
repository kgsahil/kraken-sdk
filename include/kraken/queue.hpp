/// @file queue.hpp
/// @brief Abstract interface for single-producer, single-consumer queues
///
/// Allows users to provide custom queue implementations for message passing
/// between I/O and dispatcher threads.

#pragma once

#include <cstddef>
#include <memory>

namespace kraken {

//------------------------------------------------------------------------------
// Message Queue Interface
//------------------------------------------------------------------------------

/// Abstract interface for SPSC (Single Producer, Single Consumer) queues
/// 
/// This interface allows users to provide custom queue implementations.
/// The default implementation uses rigtorp/SPSCQueue, but users can
/// implement their own for specific requirements (e.g., priority queues,
/// bounded/unbounded, different memory layouts).
///
/// @tparam T The message type to queue
template<typename T>
class MessageQueue {
public:
    virtual ~MessageQueue() = default;
    
    /// Try to push an element (non-blocking)
    /// @param value The value to push
    /// @return true if pushed successfully, false if queue is full
    virtual bool try_push(T value) = 0;
    
    /// Get a pointer to the front element (without removing it)
    /// @return Pointer to front element, or nullptr if queue is empty
    virtual T* front() = 0;
    
    /// Remove the front element
    virtual void pop() = 0;
    
    /// Get the current number of elements in the queue
    /// @return Queue size (approximate, may be stale)
    virtual size_t size() const = 0;
};

//------------------------------------------------------------------------------
// Default Implementation (rigtorp::SPSCQueue)
//------------------------------------------------------------------------------

/// Default queue implementation using rigtorp::SPSCQueue
template<typename T>
class DefaultMessageQueue : public MessageQueue<T> {
public:
    explicit DefaultMessageQueue(size_t capacity);
    ~DefaultMessageQueue() override = default;
    
    // Non-copyable, movable
    DefaultMessageQueue(const DefaultMessageQueue&) = delete;
    DefaultMessageQueue& operator=(const DefaultMessageQueue&) = delete;
    DefaultMessageQueue(DefaultMessageQueue&&) noexcept = default;
    DefaultMessageQueue& operator=(DefaultMessageQueue&&) noexcept = default;
    
    bool try_push(T value) override;
    T* front() override;
    void pop() override;
    size_t size() const override;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace kraken

