#pragma once

#include <string>
#include <chrono>
#include <cstdint>

namespace hft {

enum class OrderSide {
    BUY,
    SELL
};

enum class OrderType {
    MARKET,
    LIMIT,
    STOP
};

enum class OrderStatus {
    PENDING,
    PARTIALLY_FILLED,
    FILLED,
    CANCELLED,
    REJECTED
};

struct Order {
    uint64_t order_id;
    std::string symbol;
    OrderSide side;
    OrderType type;
    double price;
    double quantity;
    double filled_quantity;
    OrderStatus status;
    std::chrono::system_clock::time_point timestamp;
    std::chrono::system_clock::time_point created_time;
    
    // Constructor
    Order(uint64_t id, const std::string& sym, OrderSide s, OrderType t, 
          double p, double qty);
    
    // Check if order is active (can be filled)
    bool isActive() const;
    
    // Check if order is fully filled
    bool isFilled() const;
    
    // Get remaining quantity
    double getRemainingQuantity() const;
    
    // Update filled quantity
    void updateFill(double fill_qty);
    
    // Cancel order
    void cancel();
    
    // Get age in milliseconds
    uint64_t getAgeMs() const;
};

} // namespace hft
