#include "kraken/core/client.hpp"
#include "internal/client_impl.hpp"

namespace kraken {

//------------------------------------------------------------------------------
// Construction / Destruction
//------------------------------------------------------------------------------

KrakenClient::KrakenClient()
    : impl_(std::make_unique<Impl>(ClientConfig{})) {}

KrakenClient::KrakenClient(ClientConfig config)
    : impl_(std::make_unique<Impl>(std::move(config))) {}

KrakenClient::~KrakenClient() = default;

KrakenClient::KrakenClient(KrakenClient&&) noexcept = default;
KrakenClient& KrakenClient::operator=(KrakenClient&&) noexcept = default;

//------------------------------------------------------------------------------
// Callbacks
//------------------------------------------------------------------------------

void KrakenClient::on_ticker(TickerCallback callback) {
    impl_->on_ticker(std::move(callback));
}

void KrakenClient::on_trade(TradeCallback callback) {
    impl_->on_trade(std::move(callback));
}

void KrakenClient::on_book(BookCallback callback) {
    impl_->on_book(std::move(callback));
}

void KrakenClient::on_ohlc(OHLCCallback callback) {
    impl_->on_ohlc(std::move(callback));
}

void KrakenClient::on_error(ErrorCallback callback) {
    impl_->on_error(std::move(callback));
}

void KrakenClient::on_connection_state(ConnectionStateCallback callback) {
    impl_->on_connection_state(std::move(callback));
}

//------------------------------------------------------------------------------
// Connection
//------------------------------------------------------------------------------

void KrakenClient::connect() {
    impl_->connect();
}

void KrakenClient::disconnect() {
    impl_->disconnect();
}

bool KrakenClient::is_connected() const {
    return impl_->is_connected();
}

ConnectionState KrakenClient::connection_state() const {
    return impl_->connection_state();
}

//------------------------------------------------------------------------------
// Subscriptions
//------------------------------------------------------------------------------

Subscription KrakenClient::subscribe(Channel channel, 
                                      const std::vector<std::string>& symbols) {
    return impl_->subscribe(channel, symbols);
}

Subscription KrakenClient::subscribe_book(const std::vector<std::string>& symbols, 
                                           int depth) {
    return impl_->subscribe_book(symbols, depth);
}

//------------------------------------------------------------------------------
// Strategies
//------------------------------------------------------------------------------

int KrakenClient::add_alert(std::shared_ptr<AlertStrategy> strategy, 
                            AlertCallback callback) {
    return impl_->add_alert(std::move(strategy), std::move(callback));
}

void KrakenClient::remove_alert(int alert_id) {
    impl_->remove_alert(alert_id);
}

size_t KrakenClient::alert_count() const {
    return impl_->alert_count();
}

std::vector<std::pair<int, std::string>> KrakenClient::get_alerts() const {
    return impl_->get_alerts();
}

void KrakenClient::enable_alert(int alert_id) {
    impl_->enable_alert(alert_id);
}

void KrakenClient::disable_alert(int alert_id) {
    impl_->disable_alert(alert_id);
}

bool KrakenClient::is_alert_enabled(int alert_id) const {
    return impl_->is_alert_enabled(alert_id);
}

//------------------------------------------------------------------------------
// Event Loop
//------------------------------------------------------------------------------

void KrakenClient::run() {
    impl_->run();
}

void KrakenClient::run_async() {
    impl_->run_async();
}

void KrakenClient::stop() {
    impl_->stop();
}

bool KrakenClient::is_running() const {
    return impl_->is_running();
}

//------------------------------------------------------------------------------
// Metrics
//------------------------------------------------------------------------------

Metrics KrakenClient::get_metrics() const {
    return impl_->get_metrics();
}

//------------------------------------------------------------------------------
// Telemetry
//------------------------------------------------------------------------------

std::shared_ptr<Telemetry> KrakenClient::get_telemetry_instance() const {
    return impl_->get_telemetry_instance();
}

//------------------------------------------------------------------------------
// Data Snapshots
//------------------------------------------------------------------------------

std::optional<Ticker> KrakenClient::latest_ticker(const std::string& symbol) const {
    return impl_->latest_ticker(symbol);
}

std::optional<OrderBook> KrakenClient::latest_book(const std::string& symbol) const {
    return impl_->latest_book(symbol);
}

std::unordered_map<std::string, Ticker> KrakenClient::all_tickers() const {
    return impl_->all_tickers();
}

//------------------------------------------------------------------------------
// Gap Detection
//------------------------------------------------------------------------------

uint64_t KrakenClient::gap_count() const {
    return impl_->gap_count();
}

} // namespace kraken

