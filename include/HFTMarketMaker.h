#pragma once

// Main header file for High-Frequency Trading Market Maker
// Include all necessary components

#include "Config.h"
#include "Utils.h"
#include "Order.h"
#include "OrderBook.h"
#include "PriceGenerator.h"
#include "MarketMaker.h"
#include "PnLCalculator.h"

// Additional includes for the complete system
#include <iostream>
#include <memory>
#include <thread>
#include <atomic>
#include <chrono>
#include <string>
#include <vector>
#include <deque>
#include <map>
#include <unordered_map>
#include <mutex>
#include <condition_variable>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <random>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <functional>

namespace hft {

// Forward declarations
class SimulationEngine;
class DataLogger;
class PerformanceMonitor;

// Configuration structures and utilities are now in Config.h and Utils.h

// MarketMakerConfig is defined in MarketMaker.h

// SimulationEngine class is defined in SimulationEngine.h

} // namespace hft
