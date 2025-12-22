/// @file queue.cpp
/// @brief Default queue implementation using rigtorp::SPSCQueue

#include "kraken/queue.hpp"
#include <rigtorp/SPSCQueue.h>

namespace kraken {

//------------------------------------------------------------------------------
// DefaultMessageQueue Implementation
//------------------------------------------------------------------------------

template<typename T>
class DefaultMessageQueue<T>::Impl {
public:
    explicit Impl(size_t capacity)
        : queue_(capacity) {}
    
    rigtorp::SPSCQueue<T> queue_;
};

template<typename T>
DefaultMessageQueue<T>::DefaultMessageQueue(size_t capacity)
    : impl_(std::make_unique<Impl>(capacity)) {}

template<typename T>
bool DefaultMessageQueue<T>::try_push(T value) {
    return impl_->queue_.try_push(std::move(value));
}

template<typename T>
T* DefaultMessageQueue<T>::front() {
    return impl_->queue_.front();
}

template<typename T>
void DefaultMessageQueue<T>::pop() {
    impl_->queue_.pop();
}

template<typename T>
size_t DefaultMessageQueue<T>::size() const {
    return impl_->queue_.size();
}

// Note: Explicit instantiation for Message type is done in client_impl.cpp
// since Message is defined there

} // namespace kraken

