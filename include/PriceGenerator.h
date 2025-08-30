#pragma once

#include <random>
#include <chrono>
#include <vector>
#include <deque>
#include <atomic>
#include <mutex>

namespace hft {

class PriceGenerator {
private:
    // Random number generation
    std::mt19937 rng;
    std::normal_distribution<double> normal_dist;
    
    // Price simulation parameters
    double initial_price;
    double current_price;
    double drift;           // Annual drift rate (e.g., 0.05 for 5%)
    double volatility;      // Annual volatility (e.g., 0.20 for 20%)
    double time_step;       // Time step in years (e.g., 1/252 for daily)
    
    // Price history for volatility calculation
    std::deque<double> price_history;
    size_t history_window;
    
    // Statistics
    std::atomic<uint64_t> ticks_generated{0};
    std::atomic<double> min_price{std::numeric_limits<double>::max()};
    std::atomic<double> max_price{std::numeric_limits<double>::lowest()};
    
    // Thread safety
    mutable std::mutex price_mutex;

public:
    PriceGenerator(double initial_p, double drift_rate, double vol, 
                   double time_step_years = 1.0/252.0, size_t history_size = 100);
    
    // Price generation
    double generateNextPrice();
    double generateNextPrice(double current_p);
    
    // Batch price generation
    std::vector<double> generatePriceSeries(size_t count);
    
    // Volatility calculation
    double calculateRealizedVolatility(size_t lookback = 20) const;
    double calculateRollingVolatility(size_t window = 20) const;
    
    // Price statistics
    double getCurrentPrice() const;
    double getMinPrice() const { return min_price.load(); }
    double getMaxPrice() const { return max_price.load(); }
    uint64_t getTicksGenerated() const { return ticks_generated.load(); }
    
    // Parameter updates
    void updateDrift(double new_drift);
    void updateVolatility(double new_vol);
    void updateTimeStep(double new_time_step);
    
    // Reset and utility
    void reset(double new_initial_price);
    void setSeed(uint32_t seed);
    
    // Geometric Brownian Motion formula
    static double calculateGBMPrice(double current_price, double drift, 
                                   double volatility, double time_step, double random_shock);

private:
    // Helper functions
    void updatePriceStatistics(double price);
    double calculatePriceChange(double current_p);
    void addToHistory(double price);
    double getRandomShock();
};

} // namespace hft
