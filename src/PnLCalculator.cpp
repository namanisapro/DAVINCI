#include "PnLCalculator.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <numeric>
#include <cmath>

namespace hft {

PnLCalculator::PnLCalculator(size_t history_size, bool daily_tracking)
    : current_position(0.0), average_cost(0.0), mark_price(0.0),
      daily_pnl(0.0), daily_high(0.0), daily_low(0.0),
      max_drawdown(0.0), peak_value(0.0),
      max_history_size(history_size), track_daily_metrics(daily_tracking) {
    
    last_reset = std::chrono::system_clock::now();
}

void PnLCalculator::recordTrade(double price, double quantity, double side) {
    Trade trade;
    trade.timestamp = std::chrono::system_clock::now();
    trade.price = price;
    trade.quantity = quantity;
    trade.side = side;
    trade.trade_value = price * quantity;
    trade.trade_id = trade_history.size() + 1;
    
    recordTrade(trade);
}

void PnLCalculator::recordTrade(const Trade& trade) {
    std::lock_guard<std::mutex> lock(pnl_mutex);
    
    addToHistory(trade);
    updatePosition(trade.quantity, trade.price);
    updatePnL();
    
    if (track_daily_metrics) {
        updateDailyMetrics();
    }
}

void PnLCalculator::updateMarkPrice(double new_price) {
    std::lock_guard<std::mutex> lock(pnl_mutex);
    mark_price = new_price;
    updatePnL();
}

void PnLCalculator::updatePnL() {
    calculateRealizedPnL();
    calculateUnrealizedPnL();
    
    total_pnl.store(realized_pnl.load() + unrealized_pnl.load());
    
    // Update drawdown metrics
    updateDrawdownMetrics();
    
    // Add to PnL history
    PnLSnapshot snapshot;
    snapshot.timestamp = std::chrono::system_clock::now();
    snapshot.realized_pnl = realized_pnl.load();
    snapshot.unrealized_pnl = unrealized_pnl.load();
    snapshot.total_pnl = total_pnl.load();
    snapshot.position = current_position;
    snapshot.mark_price = mark_price;
    snapshot.daily_pnl = daily_pnl;
    snapshot.cumulative_pnl = total_pnl.load();
    
    addToPnLHistory(snapshot);
}

void PnLCalculator::calculateRealizedPnL() {
    // Realized PnL is already calculated when trades are recorded
    // This method can be used for additional calculations if needed
}

void PnLCalculator::calculateUnrealizedPnL() {
    if (current_position == 0.0 || mark_price == 0.0) {
        unrealized_pnl.store(0.0);
        return;
    }
    
    // Unrealized PnL = (Mark Price - Average Cost) * Position
    double unrealized = (mark_price - average_cost) * current_position;
    unrealized_pnl.store(unrealized);
}

void PnLCalculator::updatePosition(double quantity, double price) {
    if (quantity <= 0) return;
    
    double trade_value = quantity * price;
    
    if (current_position == 0.0) {
        // First trade
        current_position = quantity;
        average_cost = price;
    } else {
        // Update average cost using weighted average
        double total_value = current_position * average_cost + trade_value;
        current_position += quantity;
        average_cost = total_value / current_position;
    }
}

double PnLCalculator::getCurrentPosition() const {
    std::lock_guard<std::mutex> lock(pnl_mutex);
    return current_position;
}

double PnLCalculator::getAverageCost() const {
    std::lock_guard<std::mutex> lock(pnl_mutex);
    return average_cost;
}

double PnLCalculator::getMarkToMarketPnL() const {
    std::lock_guard<std::mutex> lock(pnl_mutex);
    return total_pnl.load();
}

double PnLCalculator::getSharpeRatio(size_t lookback) const {
    std::lock_guard<std::mutex> lock(pnl_mutex);
    
    if (returns.size() < lookback) {
        return 0.0;
    }
    
    size_t start_idx = returns.size() - lookback;
    std::vector<double> recent_returns(returns.begin() + start_idx, returns.end());
    
    return calculateSharpeRatio(recent_returns);
}

double PnLCalculator::getMaxDrawdown() const {
    std::lock_guard<std::mutex> lock(pnl_mutex);
    return max_drawdown;
}

double PnLCalculator::getVolatility(size_t lookback) const {
    std::lock_guard<std::mutex> lock(pnl_mutex);
    
    if (returns.size() < lookback) {
        return 0.0;
    }
    
    size_t start_idx = returns.size() - lookback;
    std::vector<double> recent_returns(returns.begin() + start_idx, returns.end());
    
    return calculateVolatility(recent_returns);
}

double PnLCalculator::getWinRate() const {
    std::lock_guard<std::mutex> lock(pnl_mutex);
    
    if (trade_history.empty()) {
        return 0.0;
    }
    
    int winning_trades = 0;
    for (const auto& trade : trade_history) {
        // A trade is "winning" if it reduces position or improves average cost
        if (trade.side * (trade.price - average_cost) > 0) {
            winning_trades++;
        }
    }
    
    return static_cast<double>(winning_trades) / trade_history.size();
}

double PnLCalculator::getProfitFactor() const {
    std::lock_guard<std::mutex> lock(pnl_mutex);
    
    if (trade_history.empty()) {
        return 0.0;
    }
    
    double gross_profit = 0.0;
    double gross_loss = 0.0;
    
    for (const auto& trade : trade_history) {
        double trade_pnl = trade.side * (trade.price - average_cost) * trade.quantity;
        if (trade_pnl > 0) {
            gross_profit += trade_pnl;
        } else {
            gross_loss += std::abs(trade_pnl);
        }
    }
    
    if (gross_loss == 0.0) {
        return gross_profit > 0.0 ? std::numeric_limits<double>::max() : 0.0;
    }
    
    return gross_profit / gross_loss;
}

double PnLCalculator::getDailyPnL() const {
    std::lock_guard<std::mutex> lock(pnl_mutex);
    return daily_pnl;
}

double PnLCalculator::getDailyHigh() const {
    std::lock_guard<std::mutex> lock(pnl_mutex);
    return daily_high;
}

double PnLCalculator::getDailyLow() const {
    std::lock_guard<std::mutex> lock(pnl_mutex);
    return daily_low;
}

void PnLCalculator::resetDailyMetrics() {
    std::lock_guard<std::mutex> lock(pnl_mutex);
    
    daily_pnl = 0.0;
    daily_high = 0.0;
    daily_low = 0.0;
    last_reset = std::chrono::system_clock::now();
}

std::vector<PnLSnapshot> PnLCalculator::getPnLHistory() const {
    std::lock_guard<std::mutex> lock(pnl_mutex);
    return std::vector<PnLSnapshot>(pnl_history.begin(), pnl_history.end());
}

std::vector<Trade> PnLCalculator::getTradeHistory() const {
    std::lock_guard<std::mutex> lock(pnl_mutex);
    return std::vector<Trade>(trade_history.begin(), trade_history.end());
}

std::vector<double> PnLCalculator::getReturns() const {
    std::lock_guard<std::mutex> lock(pnl_mutex);
    return returns;
}

void PnLCalculator::exportToCSV(const std::string& filename) const {
    std::lock_guard<std::mutex> lock(pnl_mutex);
    
    std::string actual_filename = filename.empty() ? "pnl_data.csv" : filename;
    
    std::ofstream file(actual_filename);
    if (!file.is_open()) {
        return;
    }
    
    // Write header
    file << "Timestamp,RealizedPnL,UnrealizedPnL,TotalPnL,Position,MarkPrice,DailyPnL,CumulativePnL\n";
    
    // Write data
    for (const auto& snapshot : pnl_history) {
        auto time_t = std::chrono::system_clock::to_time_t(snapshot.timestamp);
        std::tm* tm = std::localtime(&time_t);
        
        file << std::put_time(tm, "%Y-%m-%d %H:%M:%S") << ","
             << snapshot.realized_pnl << ","
             << snapshot.unrealized_pnl << ","
             << snapshot.total_pnl << ","
             << snapshot.position << ","
             << snapshot.mark_price << ","
             << snapshot.daily_pnl << ","
             << snapshot.cumulative_pnl << "\n";
    }
    
    file.close();
}

std::string PnLCalculator::generateReport() const {
    std::lock_guard<std::mutex> lock(pnl_mutex);
    
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2);
    
    oss << "=== PnL Calculator Report ===\n";
    oss << "Current Position: " << current_position << "\n";
    oss << "Average Cost: " << average_cost << "\n";
    oss << "Mark Price: " << mark_price << "\n";
    oss << "Realized PnL: " << realized_pnl.load() << "\n";
    oss << "Unrealized PnL: " << unrealized_pnl.load() << "\n";
    oss << "Total PnL: " << total_pnl.load() << "\n";
    oss << "Daily PnL: " << daily_pnl << "\n";
    oss << "Daily High: " << daily_high << "\n";
    oss << "Daily Low: " << daily_low << "\n";
    oss << "Max Drawdown: " << max_drawdown << "\n";
    oss << "Sharpe Ratio: " << getSharpeRatio() << "\n";
    oss << "Volatility: " << getVolatility() << "\n";
    oss << "Win Rate: " << (getWinRate() * 100.0) << "%\n";
    oss << "Profit Factor: " << getProfitFactor() << "\n";
    oss << "Total Trades: " << trade_history.size() << "\n";
    oss << "======================\n";
    
    return oss.str();
}

void PnLCalculator::clear() {
    std::lock_guard<std::mutex> lock(pnl_mutex);
    
    trade_history.clear();
    pnl_history.clear();
    returns.clear();
    
    current_position = 0.0;
    average_cost = 0.0;
    mark_price = 0.0;
    
    realized_pnl.store(0.0);
    unrealized_pnl.store(0.0);
    total_pnl.store(0.0);
    
    daily_pnl = 0.0;
    daily_high = 0.0;
    daily_low = 0.0;
    
    max_drawdown = 0.0;
    peak_value = 0.0;
}

void PnLCalculator::setMaxHistorySize(size_t size) {
    std::lock_guard<std::mutex> lock(pnl_mutex);
    max_history_size = size;
    trimHistory();
}

size_t PnLCalculator::getTradeCount() const {
    std::lock_guard<std::mutex> lock(pnl_mutex);
    return trade_history.size();
}

bool PnLCalculator::isEmpty() const {
    std::lock_guard<std::mutex> lock(pnl_mutex);
    return trade_history.empty();
}

void PnLCalculator::addToHistory(const Trade& trade) {
    trade_history.push_back(trade);
    
    // Calculate return if we have a previous PnL value
    if (!pnl_history.empty()) {
        double current_value = total_pnl.load();
        double previous_value = pnl_history.back().total_pnl;
        double ret = calculateReturn(current_value, previous_value);
        returns.push_back(ret);
    }
    
    trimHistory();
}

void PnLCalculator::addToPnLHistory(const PnLSnapshot& snapshot) {
    pnl_history.push_back(snapshot);
    trimHistory();
}

void PnLCalculator::updateDailyMetrics() {
    double current_total = total_pnl.load();
    
    if (current_total > daily_high) {
        daily_high = current_total;
    }
    if (current_total < daily_low || daily_low == 0.0) {
        daily_low = current_total;
    }
    
    daily_pnl = current_total;
}

void PnLCalculator::updateDrawdownMetrics() {
    double current_value = total_pnl.load();
    
    if (current_value > peak_value) {
        peak_value = current_value;
    }
    
    double drawdown = peak_value - current_value;
    if (drawdown > max_drawdown) {
        max_drawdown = drawdown;
    }
}

double PnLCalculator::calculateReturn(double current_value, double previous_value) const {
    if (previous_value == 0.0) {
        return 0.0;
    }
    return (current_value - previous_value) / std::abs(previous_value);
}

void PnLCalculator::trimHistory() {
    if (trade_history.size() > max_history_size) {
        trade_history.erase(trade_history.begin(), 
                           trade_history.begin() + (trade_history.size() - max_history_size));
    }
    
    if (pnl_history.size() > max_history_size) {
        pnl_history.erase(pnl_history.begin(), 
                         pnl_history.begin() + (pnl_history.size() - max_history_size));
    }
    
    if (returns.size() > max_history_size) {
        returns.erase(returns.begin(), 
                     returns.begin() + (returns.size() - max_history_size));
    }
}

double PnLCalculator::calculateVolatility(const std::vector<double>& returns) const {
    if (returns.empty()) return 0.0;
    
    double mean = std::accumulate(returns.begin(), returns.end(), 0.0) / returns.size();
    
    double variance = 0.0;
    for (double ret : returns) {
        double diff = ret - mean;
        variance += diff * diff;
    }
    variance /= returns.size();
    
    return std::sqrt(variance);
}

double PnLCalculator::calculateSharpeRatio(const std::vector<double>& returns) const {
    if (returns.empty()) return 0.0;
    
    double volatility = calculateVolatility(returns);
    if (volatility == 0.0) return 0.0;
    
    double mean_return = std::accumulate(returns.begin(), returns.end(), 0.0) / returns.size();
    
    // Assuming risk-free rate is 0 for simplicity
    return mean_return / volatility;
}

double PnLCalculator::calculateMaxDrawdown(const std::vector<double>& values) const {
    if (values.empty()) return 0.0;
    
    double max_dd = 0.0;
    double peak = values[0];
    
    for (double value : values) {
        if (value > peak) {
            peak = value;
        }
        
        double drawdown = peak - value;
        if (drawdown > max_dd) {
            max_dd = drawdown;
        }
    }
    
    return max_dd;
}

} // namespace hft
