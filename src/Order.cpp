#include "Order.h"
#include <sstream>
#include <iomanip>

namespace hft {

Order::Order(uint64_t id, const std::string& sym, OrderSide s, OrderType t, 
             double p, double qty)
    : order_id(id), symbol(sym), side(s), type(t), price(p), quantity(qty),
      filled_quantity(0.0), status(OrderStatus::PENDING),
      timestamp(std::chrono::system_clock::now()),
      created_time(std::chrono::system_clock::now()) {
}

bool Order::isActive() const {
    return status == OrderStatus::PENDING || status == OrderStatus::PARTIALLY_FILLED;
}

bool Order::isFilled() const {
    return status == OrderStatus::FILLED;
}

double Order::getRemainingQuantity() const {
    return quantity - filled_quantity;
}

void Order::updateFill(double fill_qty) {
    if (fill_qty <= 0 || fill_qty > getRemainingQuantity()) {
        return; // Invalid fill quantity
    }
    
    filled_quantity += fill_qty;
    timestamp = std::chrono::system_clock::now();
    
    if (filled_quantity >= quantity) {
        status = OrderStatus::FILLED;
    } else if (filled_quantity > 0) {
        status = OrderStatus::PARTIALLY_FILLED;
    }
}

void Order::cancel() {
    if (isActive()) {
        status = OrderStatus::CANCELLED;
        timestamp = std::chrono::system_clock::now();
    }
}

uint64_t Order::getAgeMs() const {
    auto now = std::chrono::system_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - created_time);
    return duration.count();
}

// Helper function to convert OrderSide to string
std::string orderSideToString(OrderSide side) {
    switch (side) {
        case OrderSide::BUY: return "BUY";
        case OrderSide::SELL: return "SELL";
        default: return "UNKNOWN";
    }
}

// Helper function to convert OrderType to string
std::string orderTypeToString(OrderType type) {
    switch (type) {
        case OrderType::MARKET: return "MARKET";
        case OrderType::LIMIT: return "LIMIT";
        case OrderType::STOP: return "STOP";
        default: return "UNKNOWN";
    }
}

// Helper function to convert OrderStatus to string
std::string orderStatusToString(OrderStatus status) {
    switch (status) {
        case OrderStatus::PENDING: return "PENDING";
        case OrderStatus::PARTIALLY_FILLED: return "PARTIALLY_FILLED";
        case OrderStatus::FILLED: return "FILLED";
        case OrderStatus::CANCELLED: return "CANCELLED";
        case OrderStatus::REJECTED: return "REJECTED";
        default: return "UNKNOWN";
    }
}

// Stream operator for Order
std::ostream& operator<<(std::ostream& os, const Order& order) {
    os << "Order[ID:" << order.order_id 
       << ", Symbol:" << order.symbol
       << ", Side:" << orderSideToString(order.side)
       << ", Type:" << orderTypeToString(order.type)
       << ", Price:" << std::fixed << std::setprecision(2) << order.price
       << ", Qty:" << order.quantity
       << ", Filled:" << order.filled_quantity
       << ", Status:" << orderStatusToString(order.status)
       << ", Age:" << order.getAgeMs() << "ms]";
    return os;
}

} // namespace hft
