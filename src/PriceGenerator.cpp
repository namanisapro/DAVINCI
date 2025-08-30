#include "PriceGenerator.h"
#include <cmath>
#include <algorithm>
#include <stdexcept>
#include <mutex>

namespace hft {

PriceGenerator::PriceGenerator(double initial_p, double drift_rate, double vol, 
                             double time_step_years, size_t history_size)
    : initial_price(initial_p), current_price(initial_p), drift(drift_rate), 
      volatility(vol), time_step(time_step_years), history_window(history_size) {
    
    // Initialize random number generator with current time
    auto now = std::chrono::system_clock::now();
    auto time_since_epoch = now.time_since_epoch();
    auto seed = static_cast<uint32_t>(time_since_epoch.count());
    rng.seed(seed);
    
    // Initialize normal distribution
    normal_dist = std::normal_distribution<double>(0.0, 1.0);
    
    // Initialize price history
    price_history.push_back(initial_price);
}

double PriceGenerator::generateNextPrice() {
    std::lock_guard<std::mutex> lock(price_mutex);
    
    double random_shock = getRandomShock();
    double new_price = calculateGBMPrice(current_price, drift, volatility, time_step, random_shock);
    
    current_price = new_price;
    updatePriceStatistics(new_price);
    addToHistory(new_price);
    ticks_generated++;
    
    return new_price;
}

double PriceGenerator::generateNextPrice(double current_p) {
    std::lock_guard<std::mutex> lock(price_mutex);
    
    double random_shock = getRandomShock();
    double new_price = calculateGBMPrice(current_p, drift, volatility, time_step, random_shock);
    
    current_price = new_price;
    updatePriceStatistics(new_price);
    addToHistory(new_price);
    ticks_generated++;
    
    return new_price;
}

std::vector<double> PriceGenerator::generatePriceSeries(size_t count) {
    std::vector<double> prices;
    prices.reserve(count);
    
    for (size_t i = 0; i < count; ++i) {
        prices.push_back(generateNextPrice());
    }
    
    return prices;
}

double PriceGenerator::calculateRealizedVolatility(size_t lookback) const {
    std::lock_guard<std::mutex> lock(price_mutex);
    
    if (price_history.size() < lookback + 1) {
        return 0.0;
    }
    
    std::vector<double> returns;
    returns.reserve(lookback);
    
    for (size_t i = 1; i <= lookback; ++i) {
        double current = price_history[price_history.size() - i];
        double previous = price_history[price_history.size() - i - 1];
        
        if (previous > 0) {
            double ret = std::log(current / previous);
            returns.push_back(ret);
        }
    }
    
    if (returns.empty()) return 0.0;
    
    // Calculate mean return
    double mean_return = 0.0;
    for (double ret : returns) {
        mean_return += ret;
    }
    mean_return /= returns.size();
    
    // Calculate variance
    double variance = 0.0;
    for (double ret : returns) {
        double diff = ret - mean_return;
        variance += diff * diff;
    }
    variance /= returns.size();
    
    // Annualize volatility (assuming time_step is in years)
    double daily_vol = std::sqrt(variance);
    double annual_vol = daily_vol * std::sqrt(1.0 / time_step);
    
    return annual_vol;
}

double PriceGenerator::calculateRollingVolatility(size_t window) const {
    return calculateRealizedVolatility(window);
}

double PriceGenerator::getCurrentPrice() const {
    std::lock_guard<std::mutex> lock(price_mutex);
    return current_price;
}

void PriceGenerator::updateDrift(double new_drift) {
    std::lock_guard<std::mutex> lock(price_mutex);
    drift = new_drift;
}

void PriceGenerator::updateVolatility(double new_vol) {
    std::lock_guard<std::mutex> lock(price_mutex);
    volatility = new_vol;
}

void PriceGenerator::updateTimeStep(double new_time_step) {
    std::lock_guard<std::mutex> lock(price_mutex);
    time_step = new_time_step;
}

void PriceGenerator::reset(double new_initial_price) {
    std::lock_guard<std::mutex> lock(price_mutex);
    
    initial_price = new_initial_price;
    current_price = new_initial_price;
    price_history.clear();
    price_history.push_back(new_initial_price);
    
    ticks_generated = 0;
    min_price = std::numeric_limits<double>::max();
    max_price = std::numeric_limits<double>::lowest();
}

void PriceGenerator::setSeed(uint32_t seed) {
    std::lock_guard<std::mutex> lock(price_mutex);
    rng.seed(seed);
}

double PriceGenerator::calculateGBMPrice(double current_price, double drift, 
                                        double volatility, double time_step, double random_shock) {
    // Geometric Brownian Motion formula:
    // S(t+dt) = S(t) * exp((μ - 0.5*σ²)*dt + σ*√dt*Z)
    // where:
    // S(t) = current price
    // μ = drift rate
    // σ = volatility
    // dt = time step
    // Z = random shock (standard normal)
    
    double drift_term = (drift - 0.5 * volatility * volatility) * time_step;
    double volatility_term = volatility * std::sqrt(time_step) * random_shock;
    
    double new_price = current_price * std::exp(drift_term + volatility_term);
    
    // Ensure price doesn't go negative
    return std::max(new_price, 0.01);
}

void PriceGenerator::updatePriceStatistics(double price) {
    if (price < min_price.load()) {
        min_price.store(price);
    }
    if (price > max_price.load()) {
        max_price.store(price);
    }
}

double PriceGenerator::calculatePriceChange(double current_p) {
    double random_shock = getRandomShock();
    return calculateGBMPrice(current_p, drift, volatility, time_step, random_shock);
}

void PriceGenerator::addToHistory(double price) {
    price_history.push_back(price);
    
    // Maintain history window size
    if (price_history.size() > history_window) {
        price_history.pop_front();
    }
}

double PriceGenerator::getRandomShock() {
    return normal_dist(rng);
}

} // namespace hft
