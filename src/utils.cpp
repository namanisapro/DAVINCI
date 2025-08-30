#include "Utils.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <filesystem>
#include <random>
#include <algorithm>
#include <cmath>

namespace hft {
namespace utils {

std::string formatPrice(double price, int decimals) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(decimals) << price;
    return oss.str();
}

std::string formatTimestamp(const std::chrono::system_clock::time_point& time) {
    auto time_t = std::chrono::system_clock::to_time_t(time);
    std::tm* tm = std::localtime(&time_t);
    
    std::ostringstream oss;
    oss << std::put_time(tm, "%Y-%m-%d %H:%M:%S");
    
    // Add milliseconds
    auto duration = time.time_since_epoch();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count() % 1000;
    oss << "." << std::setfill('0') << std::setw(3) << millis;
    
    return oss.str();
}

double roundToTick(double price, double tick_size) {
    if (tick_size <= 0) return price;
    return std::round(price / tick_size) * tick_size;
}

double calculateBasisPoints(double price1, double price2) {
    if (price2 == 0) return 0.0;
    return ((price1 - price2) / price2) * 10000.0;
}

double calculatePercentage(double value, double base) {
    if (base == 0) return 0.0;
    return (value / base) * 100.0;
}

double generateRandomDouble(double min, double max) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<double> dis(min, max);
    return dis(gen);
}

int generateRandomInt(int min, int max) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dis(min, max);
    return dis(gen);
}

std::vector<std::string> splitString(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;
    
    while (std::getline(ss, token, delimiter)) {
        tokens.push_back(token);
    }
    
    return tokens;
}

std::string trimString(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return "";
    
    size_t end = str.find_last_not_of(" \t\n\r");
    return str.substr(start, end - start + 1);
}

bool fileExists(const std::string& filename) {
    std::ifstream file(filename);
    return file.good();
}

std::string getCurrentDirectory() {
    return std::filesystem::current_path().string();
}

void createDirectory(const std::string& path) {
    try {
        std::filesystem::create_directories(path);
    } catch (const std::exception& e) {
        std::cerr << "Error creating directory: " << e.what() << "\n";
    }
}

// Additional utility functions for HFT

double calculateVolatility(const std::vector<double>& returns) {
    if (returns.size() < 2) return 0.0;
    
    double mean = std::accumulate(returns.begin(), returns.end(), 0.0) / returns.size();
    
    double variance = 0.0;
    for (double ret : returns) {
        double diff = ret - mean;
        variance += diff * diff;
    }
    variance /= (returns.size() - 1); // Sample variance
    
    return std::sqrt(variance);
}

double calculateSharpeRatio(const std::vector<double>& returns, double risk_free_rate) {
    if (returns.empty()) return 0.0;
    
    double volatility = calculateVolatility(returns);
    if (volatility == 0.0) return 0.0;
    
    double mean_return = std::accumulate(returns.begin(), returns.end(), 0.0) / returns.size();
    double excess_return = mean_return - risk_free_rate;
    
    return excess_return / volatility;
}

double calculateMaxDrawdown(const std::vector<double>& values) {
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

double calculateVaR(const std::vector<double>& returns, double confidence_level) {
    if (returns.empty()) return 0.0;
    
    std::vector<double> sorted_returns = returns;
    std::sort(sorted_returns.begin(), sorted_returns.end());
    
    size_t index = static_cast<size_t>((1.0 - confidence_level) * sorted_returns.size());
    if (index >= sorted_returns.size()) index = sorted_returns.size() - 1;
    
    return sorted_returns[index];
}

double calculateExpectedShortfall(const std::vector<double>& returns, double confidence_level) {
    if (returns.empty()) return 0.0;
    
    double var = calculateVaR(returns, confidence_level);
    
    double sum = 0.0;
    int count = 0;
    
    for (double ret : returns) {
        if (ret <= var) {
            sum += ret;
            count++;
        }
    }
    
    return count > 0 ? sum / count : 0.0;
}

std::string formatDuration(uint64_t milliseconds) {
    uint64_t seconds = milliseconds / 1000;
    uint64_t minutes = seconds / 60;
    uint64_t hours = minutes / 60;
    
    seconds %= 60;
    minutes %= 60;
    
    std::ostringstream oss;
    if (hours > 0) {
        oss << hours << "h ";
    }
    if (minutes > 0 || hours > 0) {
        oss << minutes << "m ";
    }
    oss << seconds << "s";
    
    return oss.str();
}

std::string formatBytes(uint64_t bytes) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unit_index = 0;
    double size = static_cast<double>(bytes);
    
    while (size >= 1024.0 && unit_index < 4) {
        size /= 1024.0;
        unit_index++;
    }
    
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << size << " " << units[unit_index];
    return oss.str();
}

double calculateCorrelation(const std::vector<double>& x, const std::vector<double>& y) {
    if (x.size() != y.size() || x.size() < 2) return 0.0;
    
    size_t n = x.size();
    
    double sum_x = std::accumulate(x.begin(), x.end(), 0.0);
    double sum_y = std::accumulate(y.begin(), y.end(), 0.0);
    double sum_xy = 0.0;
    double sum_x2 = 0.0;
    double sum_y2 = 0.0;
    
    for (size_t i = 0; i < n; ++i) {
        sum_xy += x[i] * y[i];
        sum_x2 += x[i] * x[i];
        sum_y2 += y[i] * y[i];
    }
    
    double numerator = n * sum_xy - sum_x * sum_y;
    double denominator = std::sqrt((n * sum_x2 - sum_x * sum_x) * (n * sum_y2 - sum_y * sum_y));
    
    if (denominator == 0.0) return 0.0;
    
    return numerator / denominator;
}

std::vector<double> calculateRollingAverage(const std::vector<double>& data, size_t window) {
    if (data.size() < window) return {};
    
    std::vector<double> result;
    result.reserve(data.size() - window + 1);
    
    for (size_t i = window - 1; i < data.size(); ++i) {
        double sum = 0.0;
        for (size_t j = 0; j < window; ++j) {
            sum += data[i - j];
        }
        result.push_back(sum / window);
    }
    
    return result;
}

std::vector<double> calculateRollingVolatility(const std::vector<double>& returns, size_t window) {
    if (returns.size() < window) return {};
    
    std::vector<double> result;
    result.reserve(returns.size() - window + 1);
    
    for (size_t i = window - 1; i < returns.size(); ++i) {
        std::vector<double> window_returns(returns.begin() + i - window + 1, returns.begin() + i + 1);
        result.push_back(calculateVolatility(window_returns));
    }
    
    return result;
}

} // namespace utils
} // namespace hft
