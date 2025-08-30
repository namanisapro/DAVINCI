#pragma once

#include "Order.h"
#include <map>
#include <unordered_map>
#include <vector>
#include <memory>
#include <mutex>
#include <chrono>
#include <atomic>

namespace hft {

class OrderBook {
private:
    // Price level -> vector of orders (bids and asks)
    std::map<double, std::vector<std::shared_ptr<Order>>, std::greater<double>> bids;  // Descending order for bids
    std::map<double, std::vector<std::shared_ptr<Order>>, std::less<double>> asks;     // Ascending order for asks
    
    // Order ID -> Order lookup for fast cancellation
    std::unordered_map<uint64_t, std::shared_ptr<Order>> order_lookup;
    
    // Symbol for this order book
    std::string symbol;
    
    // Thread safety
    mutable std::mutex order_book_mutex;
    
    // Order ID counter
    std::atomic<uint64_t> next_order_id{1};
    
    // Statistics
    uint64_t total_orders_processed{0};
    uint64_t total_orders_filled{0};
    double total_volume_processed{0.0};

public:
    explicit OrderBook(const std::string& sym);
    ~OrderBook() = default;
    
    // Order management
    uint64_t addOrder(OrderSide side, OrderType type, double price, double quantity);
    bool cancelOrder(uint64_t order_id);
    bool modifyOrder(uint64_t order_id, double new_price, double new_quantity);
    
    // Order book queries
    double getBestBid() const;
    double getBestAsk() const;
    double getMidPrice() const;
    double getSpread() const;
    double getBidVolume() const;
    double getAskVolume() const;
    
    // Price level queries
    std::vector<std::pair<double, double>> getTopBids(int levels = 5) const;
    std::vector<std::pair<double, double>> getTopAsks(int levels = 5) const;
    
    // Order book display
    void printOrderBook(int levels = 10) const;
    std::string getOrderBookString(int levels = 10) const;
    
    // Market data
    bool processMarketOrder(OrderSide side, double quantity);
    void updatePrice(double new_price);
    
    // Statistics
    uint64_t getTotalOrders() const { return total_orders_processed; }
    uint64_t getTotalFills() const { return total_orders_filled; }
    double getTotalVolume() const { return total_volume_processed; }
    
    // Utility functions
    bool isEmpty() const;
    void clear();
    size_t getBidLevels() const { return bids.size(); }
    size_t getAskLevels() const { return asks.size(); }
    
private:
    // Helper functions
    template<typename Compare>
    void removeOrderFromPriceLevel(std::map<double, std::vector<std::shared_ptr<Order>>, Compare>& price_levels, 
                                  double price, uint64_t order_id);
    void cleanupEmptyPriceLevels();
    void updateOrderStatus(std::shared_ptr<Order> order, OrderStatus status);
    uint64_t generateOrderId();
    
    // Thread-safe getters
    double getBestBidUnsafe() const;
    double getBestAskUnsafe() const;
};

} // namespace hft
