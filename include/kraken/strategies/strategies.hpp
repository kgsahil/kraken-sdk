/// @file strategies.hpp
/// @brief Main include for all trading strategies
/// 
/// This is the main header that includes all strategy classes.
/// For backward compatibility, users can include this file.
/// 
/// For better compilation times, include specific strategy headers:
/// - `strategies/base.hpp` - Base classes
/// - `strategies/price_alert.hpp` - PriceAlert
/// - `strategies/volume_spike.hpp` - VolumeSpike
/// - `strategies/spread_alert.hpp` - SpreadAlert
/// - `strategies/composite.hpp` - CompositeStrategy
/// - `strategies/presets.hpp` - StrategyPresets

#pragma once

// Include all strategy components
#include "base.hpp"
#include "price_alert.hpp"
#include "volume_spike.hpp"
#include "spread_alert.hpp"
#include "composite.hpp"
#include "presets.hpp"
#include "strategy_config.hpp"

// Re-export everything for convenience
namespace kraken {
    // All classes are already in namespace kraken via includes above
}

