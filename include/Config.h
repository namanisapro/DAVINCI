#pragma once

#include <string>
#include <cstdint>

namespace hft {

// Global constants
constexpr double TICK_SIZE = 0.01;
constexpr double MIN_SPREAD_BPS = 1.0;
constexpr double MAX_SPREAD_BPS = 100.0;
constexpr double DEFAULT_VOLATILITY = 0.20;
constexpr double DEFAULT_DRIFT = 0.05;
constexpr size_t DEFAULT_ORDER_BOOK_DEPTH = 10;
constexpr uint64_t DEFAULT_ORDER_REFRESH_MS = 100;

// Configuration structures
struct SystemConfig {
    std::string symbol = "AAPL";
    double initial_price = 100.0;
    double tick_size = TICK_SIZE;
    size_t order_book_depth = DEFAULT_ORDER_BOOK_DEPTH;
    uint64_t simulation_duration_ms = 60000;  // 1 minute
    uint64_t tick_interval_ms = 10;           // 100 ticks per second
    bool enable_logging = true;
    bool enable_csv_export = true;
    std::string log_directory = "logs";
    std::string data_directory = "data";
};

} // namespace hft
