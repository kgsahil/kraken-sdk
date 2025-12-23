/// @file volume_spike.hpp
/// @brief Volume spike detection strategy
/// 
/// Detects unusual volume spikes by comparing current volume to historical average.

#pragma once

#include "base.hpp"
#include <deque>
#include <unordered_map>

namespace kraken {

/// @brief Alert when volume exceeds N× the average
/// 
/// Monitors trading volume and triggers when current volume exceeds
/// a multiple of the historical average.
/// 
/// @example
/// @code
/// auto alert = VolumeSpike::Builder()
///     .symbols({"BTC/USD", "ETH/USD"})
///     .multiplier(2.5)
///     .lookback(50)
///     .build();
/// @endcode
class VolumeSpike : public AlertStrategy {
public:
    /// @brief Builder for VolumeSpike configuration
    class Builder {
    public:
        Builder& symbols(const std::vector<std::string>& syms) {
            symbols_ = syms;
            return *this;
        }
        
        Builder& multiplier(double mult) {
            multiplier_ = mult;
            return *this;
        }
        
        Builder& lookback(size_t n) {
            lookback_ = n;
            return *this;
        }
        
        std::shared_ptr<VolumeSpike> build() {
            return std::make_shared<VolumeSpike>(symbols_, multiplier_, lookback_);
        }
        
    private:
        std::vector<std::string> symbols_;
        double multiplier_ = 2.0;
        size_t lookback_ = 50;
    };
    
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    VolumeSpike(std::vector<std::string> symbols, double multiplier, size_t lookback)
        : symbols_(std::move(symbols))
        , multiplier_(multiplier)
        , lookback_(lookback) {}
    
    bool check(const Ticker& ticker) override {
        auto& state = states_[ticker.symbol];
        
        // Add current volume to history
        state.history.push_back(ticker.volume_24h);
        
        // Keep only lookback samples
        while (state.history.size() > lookback_) {
            state.history.pop_front();
        }
        
        // Need enough samples to calculate average
        if (state.history.size() < lookback_ / 2) {
            return false;
        }
        
        // Calculate average (excluding current)
        double sum = 0.0;
        for (size_t i = 0; i < state.history.size() - 1; ++i) {
            sum += state.history[i];
        }
        double avg = sum / (state.history.size() - 1);
        
        // Check if current volume exceeds threshold
        return ticker.volume_24h > avg * multiplier_;
    }
    
    std::string name() const override { return "VolumeSpike"; }
    
    std::vector<std::string> symbols() const override { 
        return symbols_; 
    }
    
    void reset() override {
        states_.clear();
    }
    
private:
    std::vector<std::string> symbols_;
    double multiplier_;
    size_t lookback_;
    
    struct SymbolState {
        std::deque<double> history;
    };
    std::unordered_map<std::string, SymbolState> states_;
};

} // namespace kraken

