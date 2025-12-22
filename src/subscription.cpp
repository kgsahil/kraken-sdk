#include "kraken/subscription.hpp"
#include "internal/client_impl.hpp"

namespace kraken {

//------------------------------------------------------------------------------
// Subscription::Impl - PIMPL implementation
//------------------------------------------------------------------------------

class Subscription::Impl {
public:
    // Forward all methods to SubscriptionImpl
    void pause() { impl_->pause(); }
    void resume() { impl_->resume(); }
    void unsubscribe() { impl_->unsubscribe(); }
    void add_symbols(const std::vector<std::string>& symbols) { impl_->add_symbols(symbols); }
    void remove_symbols(const std::vector<std::string>& symbols) { impl_->remove_symbols(symbols); }
    bool is_active() const { return impl_->is_active(); }
    bool is_paused() const { return impl_->is_paused(); }
    Channel channel() const { return impl_->channel(); }
    std::vector<std::string> symbols() const { return impl_->symbols(); }
    int id() const { return impl_->id(); }
    
    Impl(std::shared_ptr<SubscriptionImpl> impl) : impl_(std::move(impl)) {}
    
private:
    std::shared_ptr<SubscriptionImpl> impl_;
};

// Factory function - takes a shared_ptr<SubscriptionImpl>
std::shared_ptr<Subscription::Impl> Subscription::create_impl_from_internal(std::shared_ptr<SubscriptionImpl> impl) {
    return std::make_shared<Impl>(std::move(impl));
}

//------------------------------------------------------------------------------
// Subscription
//------------------------------------------------------------------------------

Subscription::Subscription(std::shared_ptr<Impl> impl)
    : impl_(std::move(impl)) {}

Subscription::~Subscription() = default;

Subscription::Subscription(Subscription&&) noexcept = default;
Subscription& Subscription::operator=(Subscription&&) noexcept = default;

void Subscription::pause() {
    if (impl_) impl_->pause();
}

void Subscription::resume() {
    if (impl_) impl_->resume();
}

void Subscription::unsubscribe() {
    if (impl_) impl_->unsubscribe();
}

void Subscription::add_symbols(const std::vector<std::string>& symbols) {
    if (impl_) impl_->add_symbols(symbols);
}

void Subscription::remove_symbols(const std::vector<std::string>& symbols) {
    if (impl_) impl_->remove_symbols(symbols);
}

bool Subscription::is_active() const {
    return impl_ && impl_->is_active();
}

bool Subscription::is_paused() const {
    return impl_ && impl_->is_paused();
}

Channel Subscription::channel() const {
    return impl_ ? impl_->channel() : Channel::Ticker;
}

std::vector<std::string> Subscription::symbols() const {
    return impl_ ? impl_->symbols() : std::vector<std::string>{};
}

int Subscription::id() const {
    return impl_ ? impl_->id() : -1;
}

} // namespace kraken

