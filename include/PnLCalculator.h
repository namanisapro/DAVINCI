#pragma once

#include <vector>
#include <deque>
#include <chrono>
#include <atomic>
#include <mutex>
#include <string>

namespace hft {

struct Trade {
    std::chrono::system_clock::time_point timestamp;
    double price;
    double quantity;
    double side;  // 1.0 for buy, -1.0 for sell
    double trade_value;
    uint64_t trade_id;
};

struct PnLSnapshot {
    std::chrono::system_clock::time_point timestamp;
    double realized_pnl;
    double unrealized_pnl;
    double total_pnl;
    double position;
    double mark_price;
    double daily_pnl;
    double cumulative_pnl;
};

class PnLCalculator {
private:
    // Trade history
    std::deque<Trade> trade_history;
    std::deque<PnLSnapshot> pnl_history;
    
    // Current state
    double current_position;
    double average_cost;
    double mark_price;
    
    // PnL tracking
    std::atomic<double> realized_pnl{0.0};
    std::atomic<double> unrealized_pnl{0.0};
    std::atomic<double> total_pnl{0.0};
    
    // Daily tracking
    std::chrono::system_clock::time_point last_reset;
    double daily_pnl;
    double daily_high;
    double daily_low;
    
    // Performance metrics
    double max_drawdown;
    double peak_value;
    std::vector<double> returns;
    
    // Configuration
    size_t max_history_size;
    bool track_daily_metrics;
    
    // Thread safety
    mutable std::mutex pnl_mutex;

public:
    explicit PnLCalculator(size_t history_size = 10000, bool daily_tracking = true);
    ~PnLCalculator() = default;
    
    // Trade recording
    void recordTrade(double price, double quantity, double side);
    void recordTrade(const Trade& trade);
    
    // PnL updates
    void updateMarkPrice(double new_price);
    void updatePnL();
    void calculateRealizedPnL();
    void calculateUnrealizedPnL();
    
    // Position management
    void updatePosition(double quantity, double price);
    double getCurrentPosition() const;
    double getAverageCost() const;
    
    // PnL queries
    double getRealizedPnL() const { return realized_pnl.load(); }
    double getUnrealizedPnL() const { return unrealized_pnl.load(); }
    double getTotalPnL() const { return total_pnl.load(); }
    double getMarkToMarketPnL() const;
    
    // Performance metrics
    double getSharpeRatio(size_t lookback = 252) const;
    double getMaxDrawdown() const;
    double getVolatility(size_t lookback = 252) const;
    double getWinRate() const;
    double getProfitFactor() const;
    
    // Daily metrics
    double getDailyPnL() const;
    double getDailyHigh() const;
    double getDailyLow() const;
    void resetDailyMetrics();
    
    // History and analysis
    std::vector<PnLSnapshot> getPnLHistory() const;
    std::vector<Trade> getTradeHistory() const;
    std::vector<double> getReturns() const;
    
    // Export and reporting
    void exportToCSV(const std::string& filename) const;
    std::string generateReport() const;
    
    // Utility functions
    void clear();
    void setMaxHistorySize(size_t size);
    size_t getTradeCount() const;
    bool isEmpty() const;

private:
    // Helper functions
    void addToHistory(const Trade& trade);
    void addToPnLHistory(const PnLSnapshot& snapshot);
    void updateDailyMetrics();
    void updateDrawdownMetrics();
    double calculateReturn(double current_value, double previous_value) const;
    void trimHistory();
    
    // Performance calculations
    double calculateVolatility(const std::vector<double>& returns) const;
    double calculateSharpeRatio(const std::vector<double>& returns) const;
    double calculateMaxDrawdown(const std::vector<double>& values) const;
};

} // namespace hft
