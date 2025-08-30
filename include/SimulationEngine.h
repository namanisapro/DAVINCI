#pragma once

#include "Config.h"
#include "Utils.h"
#include "Order.h"
#include "OrderBook.h"
#include "PriceGenerator.h"
#include "MarketMaker.h"
#include "PnLCalculator.h"
#include <memory>
#include <thread>
#include <atomic>
#include <chrono>
#include <string>

namespace hft {

class SimulationEngine {
private:
    SystemConfig system_config;
    MarketMakerConfig mm_config;
    
    std::shared_ptr<OrderBook> order_book;
    std::shared_ptr<PriceGenerator> price_generator;
    std::shared_ptr<MarketMaker> market_maker;
    std::shared_ptr<PnLCalculator> pnl_calculator;
    
    std::atomic<bool> running{false};
    std::thread simulation_thread;
    
    // Performance tracking
    std::chrono::system_clock::time_point start_time;
    uint64_t total_ticks_processed{0};
    double total_volume_processed{0.0};
    
public:
    SimulationEngine(const SystemConfig& sys_cfg, const MarketMakerConfig& mm_cfg);
    ~SimulationEngine();
    
    // Control functions
    void start();
    void stop();
    void pause();
    void resume();
    bool isRunning() const { return running.load(); }
    
    // Configuration
    void updateSystemConfig(const SystemConfig& new_config);
    void updateMarketMakerConfig(const MarketMakerConfig& new_config);
    SystemConfig getSystemConfig() const { return system_config; }
    MarketMakerConfig getMarketMakerConfig() const { return mm_config; }
    
    // Status and reporting
    void printStatus() const;
    std::string getStatusString() const;
    void generateReport(const std::string& filename = "") const;
    
    // Data export
    void exportOrderBookData(const std::string& filename) const;
    void exportTradeData(const std::string& filename) const;
    void exportPnLData(const std::string& filename) const;
    
private:
    // Main simulation loop
    void runSimulation();
    void processTick();
    void updateMarketData();
    
    // Performance monitoring
    void updatePerformanceMetrics();
    void logPerformanceData();
};

} // namespace hft
