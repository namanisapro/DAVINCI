#pragma once

#include "Config.h"
#include <string>
#include <vector>
#include <chrono>

namespace hft {
namespace utils {

// Price formatting
std::string formatPrice(double price, int decimals = 2);

// Time formatting
std::string formatTimestamp(const std::chrono::system_clock::time_point& time);

// Mathematical utilities
double roundToTick(double price, double tick_size = TICK_SIZE);
double calculateBasisPoints(double price1, double price2);
double calculatePercentage(double value, double base);

// Random number generation
double generateRandomDouble(double min, double max);
int generateRandomInt(int min, int max);

// String utilities
std::vector<std::string> splitString(const std::string& str, char delimiter);
std::string trimString(const std::string& str);

// File utilities
bool fileExists(const std::string& filename);
std::string getCurrentDirectory();
void createDirectory(const std::string& path);

} // namespace utils
} // namespace hft
