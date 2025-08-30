#include "SimulationEngine.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <thread>

namespace hft {

SimulationEngine::SimulationEngine(const SystemConfig& sys_cfg, const MarketMakerConfig& mm_cfg)
    : system_config(sys_cfg), mm_config(mm_cfg) {
    
    // Initialize components
    order_book = std::make_shared<OrderBook>(system_config.symbol);
    price_generator = std::make_shared<PriceGenerator>(
        system_config.initial_price, 
        DEFAULT_DRIFT, 
        DEFAULT_VOLATILITY,
        1.0 / 252.0,  // Daily time step
        100            // History window
    );
    
    market_maker = std::make_shared<MarketMaker>(order_book, price_generator, mm_config);
    pnl_calculator = std::make_shared<PnLCalculator>(10000, true);
    
    start_time = std::chrono::system_clock::now();
}

SimulationEngine::~SimulationEngine() {
    stop();
    if (simulation_thread.joinable()) {
        simulation_thread.join();
    }
}

void SimulationEngine::start() {
    if (running.load()) {
        std::cout << "Simulation is already running!\n";
        return;
    }
    
    std::cout << "Starting High-Frequency Trading Simulation...\n";
    std::cout << "Symbol: " << system_config.symbol << "\n";
    std::cout << "Initial Price: " << system_config.initial_price << "\n";
    std::cout << "Duration: " << system_config.simulation_duration_ms << " ms\n";
    std::cout << "Tick Interval: " << system_config.tick_interval_ms << " ms\n";
    
    running.store(true);
    simulation_thread = std::thread(&SimulationEngine::runSimulation, this);
}

void SimulationEngine::stop() {
    if (!running.load()) {
        return;
    }
    
    std::cout << "Stopping simulation...\n";
    running.store(false);
    
    if (simulation_thread.joinable()) {
        simulation_thread.join();
    }
    
    std::cout << "Simulation stopped.\n";
}

void SimulationEngine::pause() {
    // For now, we'll just log this
    std::cout << "Pause functionality not yet implemented.\n";
}

void SimulationEngine::resume() {
    // For now, we'll just log this
    std::cout << "Resume functionality not yet implemented.\n";
}

void SimulationEngine::updateSystemConfig(const SystemConfig& new_config) {
    system_config = new_config;
}

void SimulationEngine::updateMarketMakerConfig(const MarketMakerConfig& new_config) {
    mm_config = new_config;
    if (market_maker) {
        market_maker->updateConfig(new_config);
    }
}

void SimulationEngine::printStatus() const {
    std::cout << getStatusString();
}

std::string SimulationEngine::getStatusString() const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2);
    
    auto now = std::chrono::system_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time);
    
    oss << "\n=== Simulation Engine Status ===\n";
    oss << "Running: " << (running.load() ? "YES" : "NO") << "\n";
    oss << "Elapsed Time: " << elapsed.count() << " ms\n";
    oss << "Total Ticks Processed: " << total_ticks_processed << "\n";
    oss << "Total Volume Processed: " << total_volume_processed << "\n";
    oss << "Ticks per Second: " << (total_ticks_processed > 0 ? 
                                   (total_ticks_processed * 1000.0 / elapsed.count()) : 0.0) << "\n";
    
    // Order book status
    if (order_book) {
        oss << "\n--- Order Book Status ---\n";
        oss << "Bid Levels: " << order_book->getBidLevels() << "\n";
        oss << "Ask Levels: " << order_book->getAskLevels() << "\n";
        oss << "Total Orders: " << order_book->getTotalOrders() << "\n";
        oss << "Total Fills: " << order_book->getTotalFills() << "\n";
        oss << "Best Bid: " << order_book->getBestBid() << "\n";
        oss << "Best Ask: " << order_book->getBestAsk() << "\n";
        oss << "Spread: " << order_book->getSpread() << "\n";
        oss << "Mid Price: " << order_book->getMidPrice() << "\n";
    }
    
    // Price generator status
    if (price_generator) {
        oss << "\n--- Price Generator Status ---\n";
        oss << "Current Price: " << price_generator->getCurrentPrice() << "\n";
        oss << "Min Price: " << price_generator->getMinPrice() << "\n";
        oss << "Max Price: " << price_generator->getMaxPrice() << "\n";
        oss << "Ticks Generated: " << price_generator->getTicksGenerated() << "\n";
        oss << "Realized Volatility: " << price_generator->calculateRealizedVolatility() << "\n";
    }
    
    // Market maker status
    if (market_maker) {
        oss << "\n--- Market Maker Status ---\n";
        oss << "Current Position: " << market_maker->getConfig().max_position_size << "\n";
        oss << "Emergency Stop: " << (market_maker->isRiskLimitExceeded() ? "YES" : "NO") << "\n";
    }
    
    // PnL status
    if (pnl_calculator) {
        oss << "\n--- PnL Status ---\n";
        oss << "Total PnL: " << pnl_calculator->getTotalPnL() << "\n";
        oss << "Realized PnL: " << pnl_calculator->getRealizedPnL() << "\n";
        oss << "Unrealized PnL: " << pnl_calculator->getUnrealizedPnL() << "\n";
        oss << "Current Position: " << pnl_calculator->getCurrentPosition() << "\n";
        oss << "Trade Count: " << pnl_calculator->getTradeCount() << "\n";
    }
    
    oss << "==============================\n";
    
    return oss.str();
}

void SimulationEngine::generateReport(const std::string& filename) const {
    std::string actual_filename = filename.empty() ? "simulation_report.txt" : filename;
    
    std::ofstream file(actual_filename);
    if (!file.is_open()) {
        std::cerr << "Could not open file for writing: " << actual_filename << "\n";
        return;
    }
    
    file << "=== High-Frequency Trading Simulation Report ===\n\n";
    file << "Generated: " << utils::formatTimestamp(std::chrono::system_clock::now()) << "\n\n";
    
    // System configuration
    file << "System Configuration:\n";
    file << "  Symbol: " << system_config.symbol << "\n";
    file << "  Initial Price: " << system_config.initial_price << "\n";
    file << "  Tick Size: " << system_config.tick_size << "\n";
    file << "  Order Book Depth: " << system_config.order_book_depth << "\n";
    file << "  Simulation Duration: " << system_config.simulation_duration_ms << " ms\n";
    file << "  Tick Interval: " << system_config.tick_interval_ms << " ms\n\n";
    
    // Market maker configuration
    file << "Market Maker Configuration:\n";
    file << "  Base Spread: " << mm_config.base_spread_bps << " bps\n";
    file << "  Min Spread: " << mm_config.min_spread_bps << " bps\n";
    file << "  Max Spread: " << mm_config.max_spread_bps << " bps\n";
    file << "  Volatility Multiplier: " << mm_config.volatility_multiplier << "\n";
    file << "  Max Position Size: " << mm_config.max_position_size << "\n";
    file << "  Order Size: " << mm_config.order_size << "\n";
    file << "  Order Refresh: " << mm_config.order_refresh_ms << " ms\n\n";
    
    // Performance metrics
    file << "Performance Metrics:\n";
    file << "  Total Ticks: " << total_ticks_processed << "\n";
    file << "  Total Volume: " << total_volume_processed << "\n";
    file << "  Ticks per Second: " << (total_ticks_processed > 0 ? 
                                       (total_ticks_processed * 1000.0 / 
                                        std::chrono::duration_cast<std::chrono::milliseconds>(
                                            std::chrono::system_clock::now() - start_time).count()) : 0.0) << "\n\n";
    
    // Order book summary
    if (order_book) {
        file << "Order Book Summary:\n";
        file << "  Total Orders: " << order_book->getTotalOrders() << "\n";
        file << "  Total Fills: " << order_book->getTotalFills() << "\n";
        file << "  Bid Levels: " << order_book->getBidLevels() << "\n";
        file << "  Ask Levels: " << order_book->getAskLevels() << "\n\n";
    }
    
    // PnL summary
    if (pnl_calculator) {
        file << "PnL Summary:\n";
        file << "  Total PnL: " << pnl_calculator->getTotalPnL() << "\n";
        file << "  Realized PnL: " << pnl_calculator->getRealizedPnL() << "\n";
        file << "  Unrealized PnL: " << pnl_calculator->getUnrealizedPnL() << "\n";
        file << "  Max Drawdown: " << pnl_calculator->getMaxDrawdown() << "\n";
        file << "  Sharpe Ratio: " << pnl_calculator->getSharpeRatio() << "\n";
        file << "  Volatility: " << pnl_calculator->getVolatility() << "\n";
        file << "  Win Rate: " << (pnl_calculator->getWinRate() * 100.0) << "%\n";
        file << "  Profit Factor: " << pnl_calculator->getProfitFactor() << "\n";
        file << "  Total Trades: " << pnl_calculator->getTradeCount() << "\n\n";
    }
    
    file << "=== End of Report ===\n";
    file.close();
    
    std::cout << "Report generated: " << actual_filename << "\n";
}

void SimulationEngine::exportOrderBookData(const std::string& filename) const {
    if (!order_book) return;
    
    std::string actual_filename = filename.empty() ? "orderbook_data.csv" : filename;
    
    std::ofstream file(actual_filename);
    if (!file.is_open()) {
        std::cerr << "Could not open file for writing: " << actual_filename << "\n";
        return;
    }
    
    file << "Timestamp,Side,Price,Quantity,OrderID,Status\n";
    
    // This is a simplified export - in a real implementation, you'd want to track
    // all orders with timestamps
    file.close();
    
    std::cout << "Order book data exported: " << actual_filename << "\n";
}

void SimulationEngine::exportTradeData(const std::string& filename) const {
    if (!pnl_calculator) return;
    
    std::string actual_filename = filename.empty() ? "trade_data.csv" : filename;
    
    std::ofstream file(actual_filename);
    if (!file.is_open()) {
        std::cerr << "Could not open file for writing: " << actual_filename << "\n";
        return;
    }
    
    file << "Timestamp,Price,Quantity,Side,TradeValue,TradeID\n";
    
    auto trades = pnl_calculator->getTradeHistory();
    for (const auto& trade : trades) {
        auto time_t = std::chrono::system_clock::to_time_t(trade.timestamp);
        std::tm* tm = std::localtime(&time_t);
        
        file << std::put_time(tm, "%Y-%m-%d %H:%M:%S") << ","
             << trade.price << ","
             << trade.quantity << ","
             << (trade.side > 0 ? "BUY" : "SELL") << ","
             << trade.trade_value << ","
             << trade.trade_id << "\n";
    }
    
    file.close();
    
    std::cout << "Trade data exported: " << actual_filename << "\n";
}

void SimulationEngine::exportPnLData(const std::string& filename) const {
    if (!pnl_calculator) return;
    
    pnl_calculator->exportToCSV(filename);
}

void SimulationEngine::runSimulation() {
    std::cout << "Simulation thread started.\n";
    
    auto start_time = std::chrono::system_clock::now();
    auto end_time = start_time + std::chrono::milliseconds(system_config.simulation_duration_ms);
    
    while (running.load() && std::chrono::system_clock::now() < end_time) {
        processTick();
        
        // Sleep for the configured tick interval
        std::this_thread::sleep_for(std::chrono::milliseconds(system_config.tick_interval_ms));
    }
    
    std::cout << "Simulation completed.\n";
    running.store(false);
}

void SimulationEngine::processTick() {
    try {
        // Generate new price
        double new_price = price_generator->generateNextPrice();
        
        // Update market data
        updateMarketData();
        
        // Let market maker process the tick
        if (market_maker && market_maker->isRunning()) {
            market_maker->step();
        }
        
        // Update PnL with new mark price
        if (pnl_calculator) {
            pnl_calculator->updateMarkPrice(new_price);
        }
        
        // Update performance metrics
        updatePerformanceMetrics();
        
        total_ticks_processed++;
        
    } catch (const std::exception& e) {
        std::cerr << "Error processing tick: " << e.what() << "\n";
    }
}

void SimulationEngine::updateMarketData() {
    // This method can be used to update market data feeds
    // For now, it's handled by the price generator
}

void SimulationEngine::updatePerformanceMetrics() {
    // Update running performance statistics
    // This could include latency measurements, throughput calculations, etc.
}

void SimulationEngine::logPerformanceData() {
    // Log performance data to files or databases
    // This could include metrics like:
    // - Order processing latency
    // - Memory usage
    // - CPU utilization
    // - Network latency (if applicable)
}

} // namespace hft
