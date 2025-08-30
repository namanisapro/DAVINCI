#include "OrderBook.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <stdexcept>

namespace hft {

OrderBook::OrderBook(const std::string& sym) : symbol(sym) {
}

uint64_t OrderBook::addOrder(OrderSide side, OrderType type, double price, double quantity) {
    std::lock_guard<std::mutex> lock(order_book_mutex);
    
    uint64_t order_id = generateOrderId();
    auto order = std::make_shared<Order>(order_id, symbol, side, type, price, quantity);
    
    // Add to appropriate price level map
    if (side == OrderSide::BUY) {
        bids[price].push_back(order);
    } else {
        asks[price].push_back(order);
    }
    
    // Add to order lookup
    order_lookup[order_id] = order;
    
    total_orders_processed++;
    total_volume_processed += quantity;
    
    return order_id;
}

bool OrderBook::cancelOrder(uint64_t order_id) {
    std::lock_guard<std::mutex> lock(order_book_mutex);
    
    auto it = order_lookup.find(order_id);
    if (it == order_lookup.end()) {
        return false;
    }
    
    auto order = it->second;
    if (!order->isActive()) {
        return false;
    }
    
    // Cancel the order
    order->cancel();
    
    // Remove from price level maps
    if (order->side == OrderSide::BUY) {
        removeOrderFromPriceLevel(bids, order->price, order_id);
    } else {
        removeOrderFromPriceLevel(asks, order->price, order_id);
    }
    
    // Remove from order lookup
    order_lookup.erase(it);
    
    // Clean up empty price levels
    cleanupEmptyPriceLevels();
    
    return true;
}

bool OrderBook::modifyOrder(uint64_t order_id, double new_price, double new_quantity) {
    std::lock_guard<std::mutex> lock(order_book_mutex);
    
    auto it = order_lookup.find(order_id);
    if (it == order_lookup.end()) {
        return false;
    }
    
    auto order = it->second;
    if (!order->isActive()) {
        return false;
    }
    
    // Cancel old order
    cancelOrder(order_id);
    
    // Add new order
    addOrder(order->side, order->type, new_price, new_quantity);
    
    return true;
}

double OrderBook::getBestBid() const {
    std::lock_guard<std::mutex> lock(order_book_mutex);
    return getBestBidUnsafe();
}

double OrderBook::getBestAsk() const {
    std::lock_guard<std::mutex> lock(order_book_mutex);
    return getBestAskUnsafe();
}

double OrderBook::getMidPrice() const {
    std::lock_guard<std::mutex> lock(order_book_mutex);
    
    double best_bid = getBestBidUnsafe();
    double best_ask = getBestAskUnsafe();
    
    if (best_bid <= 0 || best_ask <= 0) {
        return 0.0;
    }
    
    return (best_bid + best_ask) / 2.0;
}

double OrderBook::getSpread() const {
    std::lock_guard<std::mutex> lock(order_book_mutex);
    
    double best_bid = getBestBidUnsafe();
    double best_ask = getBestAskUnsafe();
    
    if (best_bid <= 0 || best_ask <= 0) {
        return 0.0;
    }
    
    return best_ask - best_bid;
}

double OrderBook::getBidVolume() const {
    std::lock_guard<std::mutex> lock(order_book_mutex);
    
    double total_volume = 0.0;
    for (const auto& [price, orders] : bids) {
        for (const auto& order : orders) {
            if (order->isActive()) {
                total_volume += order->getRemainingQuantity();
            }
        }
    }
    return total_volume;
}

double OrderBook::getAskVolume() const {
    std::lock_guard<std::mutex> lock(order_book_mutex);
    
    double total_volume = 0.0;
    for (const auto& [price, orders] : asks) {
        for (const auto& order : orders) {
            if (order->isActive()) {
                total_volume += order->getRemainingQuantity();
            }
        }
    }
    return total_volume;
}

std::vector<std::pair<double, double>> OrderBook::getTopBids(int levels) const {
    std::lock_guard<std::mutex> lock(order_book_mutex);
    
    std::vector<std::pair<double, double>> result;
    int count = 0;
    
    for (const auto& [price, orders] : bids) {
        if (count >= levels) break;
        
        double total_volume = 0.0;
        for (const auto& order : orders) {
            if (order->isActive()) {
                total_volume += order->getRemainingQuantity();
            }
        }
        
        if (total_volume > 0) {
            result.emplace_back(price, total_volume);
            count++;
        }
    }
    
    return result;
}

std::vector<std::pair<double, double>> OrderBook::getTopAsks(int levels) const {
    std::lock_guard<std::mutex> lock(order_book_mutex);
    
    std::vector<std::pair<double, double>> result;
    int count = 0;
    
    for (const auto& [price, orders] : asks) {
        if (count >= levels) break;
        
        double total_volume = 0.0;
        for (const auto& order : orders) {
            if (order->isActive()) {
                total_volume += order->getRemainingQuantity();
            }
        }
        
        if (total_volume > 0) {
            result.emplace_back(price, total_volume);
            count++;
        }
    }
    
    return result;
}

void OrderBook::printOrderBook(int levels) const {
    std::cout << getOrderBookString(levels);
}

std::string OrderBook::getOrderBookString(int levels) const {
    std::lock_guard<std::mutex> lock(order_book_mutex);
    
    std::ostringstream oss;
    oss << "\n=== Order Book: " << symbol << " ===\n";
    oss << std::fixed << std::setprecision(2);
    
    // Print asks (descending order)
    auto ask_levels = getTopAsks(levels);
    std::reverse(ask_levels.begin(), ask_levels.end());
    
    for (const auto& [price, volume] : ask_levels) {
        oss << std::setw(10) << price << " | " << std::setw(10) << volume << "\n";
    }
    
    oss << "-------------------\n";
    
    // Print bids
    auto bid_levels = getTopBids(levels);
    for (const auto& [price, volume] : bid_levels) {
        oss << std::setw(10) << price << " | " << std::setw(10) << volume << "\n";
    }
    
    oss << "===================\n";
    oss << "Best Bid: " << getBestBidUnsafe() << " | Best Ask: " << getBestAskUnsafe() << "\n";
    oss << "Spread: " << (getBestAskUnsafe() - getBestBidUnsafe()) << "\n";
    oss << "Total Orders: " << total_orders_processed << "\n";
    
    return oss.str();
}

bool OrderBook::processMarketOrder(OrderSide side, double quantity) {
    std::lock_guard<std::mutex> lock(order_book_mutex);
    
    if (side == OrderSide::BUY) {
        // Market buy order - match against asks
        double remaining_qty = quantity;
        
        for (auto& [price, orders] : asks) {
            if (remaining_qty <= 0) break;
            
            for (auto& order : orders) {
                if (!order->isActive()) continue;
                
                double fill_qty = std::min(remaining_qty, order->getRemainingQuantity());
                order->updateFill(fill_qty);
                remaining_qty -= fill_qty;
                
                if (order->isFilled()) {
                    total_orders_filled++;
                }
                
                if (remaining_qty <= 0) break;
            }
        }
        
        return remaining_qty <= 0;
    } else {
        // Market sell order - match against bids
        double remaining_qty = quantity;
        
        for (auto& [price, orders] : bids) {
            if (remaining_qty <= 0) break;
            
            for (auto& order : orders) {
                if (!order->isActive()) continue;
                
                double fill_qty = std::min(remaining_qty, order->getRemainingQuantity());
                order->updateFill(fill_qty);
                remaining_qty -= fill_qty;
                
                if (order->isFilled()) {
                    total_orders_filled++;
                }
                
                if (remaining_qty <= 0) break;
            }
        }
        
        return remaining_qty <= 0;
    }
}

void OrderBook::updatePrice(double new_price) {
    // This method can be used to update the current market price
    // In a real implementation, this might trigger order matching
    // For now, it's a placeholder
    (void)new_price; // Suppress unused parameter warning
}

bool OrderBook::isEmpty() const {
    std::lock_guard<std::mutex> lock(order_book_mutex);
    return bids.empty() && asks.empty();
}

void OrderBook::clear() {
    std::lock_guard<std::mutex> lock(order_book_mutex);
    bids.clear();
    asks.clear();
    order_lookup.clear();
    total_orders_processed = 0;
    total_orders_filled = 0;
    total_volume_processed = 0.0;
}

template<typename Compare>
void OrderBook::removeOrderFromPriceLevel(std::map<double, std::vector<std::shared_ptr<Order>>, Compare>& price_levels, 
                                         double price, uint64_t order_id) {
    auto it = price_levels.find(price);
    if (it == price_levels.end()) return;
    
    auto& orders = it->second;
    orders.erase(std::remove_if(orders.begin(), orders.end(),
                               [order_id](const std::shared_ptr<Order>& order) {
                                   return order->order_id == order_id;
                               }), orders.end());
}

void OrderBook::cleanupEmptyPriceLevels() {
    // Remove empty bid levels
    for (auto it = bids.begin(); it != bids.end();) {
        if (it->second.empty()) {
            it = bids.erase(it);
        } else {
            ++it;
        }
    }
    
    // Remove empty ask levels
    for (auto it = asks.begin(); it != asks.end();) {
        if (it->second.empty()) {
            it = asks.erase(it);
        } else {
            ++it;
        }
    }
}

void OrderBook::updateOrderStatus(std::shared_ptr<Order> order, OrderStatus status) {
    order->status = status;
    order->timestamp = std::chrono::system_clock::now();
}

uint64_t OrderBook::generateOrderId() {
    return next_order_id++;
}

double OrderBook::getBestBidUnsafe() const {
    if (bids.empty()) return 0.0;
    return bids.begin()->first;
}

double OrderBook::getBestAskUnsafe() const {
    if (asks.empty()) return 0.0;
    return asks.begin()->first;
}

// Template instantiations
template void hft::OrderBook::removeOrderFromPriceLevel(std::map<double, std::vector<std::shared_ptr<Order>>, std::greater<double>>&, double, uint64_t);
template void hft::OrderBook::removeOrderFromPriceLevel(std::map<double, std::vector<std::shared_ptr<Order>>, std::less<double>>&, double, uint64_t);

} // namespace hft
