#pragma once

#include "OrderBook.h"
#include "PriceGenerator.h"
#include <memory>
#include <vector>
#include <deque>
#include <atomic>
#include <chrono>

namespace hft {

struct MarketMakerConfig {
    double base_spread_bps;        // Base spread in basis points
    double min_spread_bps;         // Minimum spread in basis points
    double max_spread_bps;         // Maximum spread in basis points
    double volatility_multiplier;  // Multiplier for volatility-based spread adjustment
    double max_position_size;      // Maximum position size (long/short)
    double position_limit;         // Position limit before reducing exposure
    uint64_t order_refresh_ms;     // Order refresh interval in milliseconds
    double order_size;             // Size of each order placed
    bool dynamic_spread;           // Whether to use dynamic spread adjustment
    bool risk_management;          // Whether to enable risk management
    double max_loss_limit;         // Maximum loss limit before emergency stop
    double stop_loss_threshold;    // Stop loss threshold
};

class MarketMaker {
private:
    // Core components
    std::shared_ptr<OrderBook> order_book;
    std::shared_ptr<PriceGenerator> price_generator;
    
    // Configuration
    MarketMakerConfig config;
    
    // State tracking
    double current_position;
    double current_inventory;
    std::atomic<double> total_pnl{0.0};
    std::atomic<double> realized_pnl{0.0};
    std::atomic<double> unrealized_pnl{0.0};
    
    // Order management
    std::vector<uint64_t> active_buy_orders;
    std::vector<uint64_t> active_sell_orders;
    std::deque<std::pair<double, double>> trade_history;  // price, quantity
    
    // Risk management
    double max_loss_limit;
    double stop_loss_threshold;
    bool emergency_stop;
    
    // Performance tracking
    std::chrono::system_clock::time_point start_time;
    uint64_t total_orders_placed;
    uint64_t total_trades_executed;
    
    // Thread safety
    mutable std::mutex market_maker_mutex;

public:
    MarketMaker(std::shared_ptr<OrderBook> ob, std::shared_ptr<PriceGenerator> pg, 
                const MarketMakerConfig& cfg);
    ~MarketMaker() = default;
    
    // Main market making loop
    void runMarketMakingLoop();
    void step();  // Single step of market making
    
    // Order placement
    void placeOrders();
    void cancelAllOrders();
    void refreshOrders();
    
    // Strategy functions
    double calculateBidPrice() const;
    double calculateAskPrice() const;
    double calculateDynamicSpread() const;
    
    // Position management
    void updatePosition(double trade_quantity, double trade_price);
    void manageInventory();
    bool shouldReduceExposure() const;
    
    // Risk management
    void checkRiskLimits();
    void emergencyShutdown();
    bool isRiskLimitExceeded() const;
    
    // PnL calculation
    void updatePnL();
    double calculateUnrealizedPnL() const;
    double calculateRealizedPnL() const;
    
    // Statistics and reporting
    void printStatus() const;
    std::string getStatusString() const;
    double getSharpeRatio() const;
    double getMaxDrawdown() const;
    
    // Configuration
    void updateConfig(const MarketMakerConfig& new_config);
    MarketMakerConfig getConfig() const { return config; }
    
    // Utility functions
    bool isRunning() const;
    void stop();
    void reset();

private:
    // Helper functions
    void placeBuyOrder(double price);
    void placeSellOrder(double price);
    void manageOrderBook();
    double calculateOptimalOrderSize() const;
    void logTrade(double price, double quantity);
    void updatePerformanceMetrics();
    
    // Risk calculations
    double calculateVaR(double confidence_level = 0.95) const;
    double calculatePositionRisk() const;
    bool checkStopLoss() const;
};

} // namespace hft
