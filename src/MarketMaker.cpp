#include "MarketMaker.h"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <chrono>
#include <iomanip>
#include <thread>

namespace hft {

MarketMaker::MarketMaker(std::shared_ptr<OrderBook> ob, std::shared_ptr<PriceGenerator> pg, 
                         const MarketMakerConfig& cfg)
    : order_book(ob), price_generator(pg), config(cfg), current_position(0.0), 
      current_inventory(0.0), max_loss_limit(cfg.max_loss_limit), 
      stop_loss_threshold(cfg.stop_loss_threshold), emergency_stop(false),
      start_time(std::chrono::system_clock::now()), total_orders_placed(0), 
      total_trades_executed(0) {
}

void MarketMaker::runMarketMakingLoop() {
    std::cout << "Starting market making loop...\n";
    
    while (!emergency_stop && isRunning()) {
        step();
        
        // Sleep for the configured interval
        std::this_thread::sleep_for(std::chrono::milliseconds(config.order_refresh_ms));
    }
}

void MarketMaker::step() {
    try {
        // Check risk limits first
        checkRiskLimits();
        if (emergency_stop) {
            return;
        }
        
        // Place orders based on current market conditions
        placeOrders();
        
        // Manage inventory and position
        manageInventory();
        
        // Update PnL
        updatePnL();
        
        // Update performance metrics
        updatePerformanceMetrics();
        
    } catch (const std::exception& e) {
        std::cerr << "Error in market making step: " << e.what() << "\n";
        emergencyShutdown();
    }
}

void MarketMaker::placeOrders() {
    // Cancel existing orders first
    cancelAllOrders();
    
    // Calculate optimal prices
    double bid_price = calculateBidPrice();
    double ask_price = calculateAskPrice();
    
    // Place new orders
    if (bid_price > 0 && !shouldReduceExposure()) {
        placeBuyOrder(bid_price);
    }
    
    if (ask_price > 0 && !shouldReduceExposure()) {
        placeSellOrder(ask_price);
    }
}

void MarketMaker::cancelAllOrders() {
    // Cancel all active buy orders
    for (uint64_t order_id : active_buy_orders) {
        order_book->cancelOrder(order_id);
    }
    active_buy_orders.clear();
    
    // Cancel all active sell orders
    for (uint64_t order_id : active_sell_orders) {
        order_book->cancelOrder(order_id);
    }
    active_sell_orders.clear();
}

void MarketMaker::refreshOrders() {
    // This method can be called to refresh orders without canceling
    // For now, we'll just place new orders
    placeOrders();
}

double MarketMaker::calculateBidPrice() const {
    double mid_price = order_book->getMidPrice();
    if (mid_price <= 0) {
        mid_price = price_generator->getCurrentPrice();
    }
    
    double spread = calculateDynamicSpread();
    double bid_price = mid_price - (spread / 2.0);
    
    // Ensure bid price is reasonable
    return std::max(bid_price, 0.01);
}

double MarketMaker::calculateAskPrice() const {
    double mid_price = order_book->getMidPrice();
    if (mid_price <= 0) {
        mid_price = price_generator->getCurrentPrice();
    }
    
    double spread = calculateDynamicSpread();
    double ask_price = mid_price + (spread / 2.0);
    
    return ask_price;
}

double MarketMaker::calculateDynamicSpread() const {
    if (!config.dynamic_spread) {
        return config.base_spread_bps / 10000.0; // Convert basis points to decimal
    }
    
    // Base spread
    double spread = config.base_spread_bps / 10000.0;
    
    // Volatility adjustment
    double volatility = price_generator->calculateRealizedVolatility();
    double vol_adjustment = volatility * config.volatility_multiplier;
    spread += vol_adjustment;
    
    // Position-based adjustment
    double position_adjustment = std::abs(current_position) / config.max_position_size;
    spread += position_adjustment * 0.001; // 1 basis point per position unit
    
    // Clamp to configured limits
    double min_spread = config.min_spread_bps / 10000.0;
    double max_spread = config.max_spread_bps / 10000.0;
    
    return std::clamp(spread, min_spread, max_spread);
}

void MarketMaker::updatePosition(double trade_quantity, double trade_price) {
    current_position += trade_quantity;
    current_inventory += trade_quantity * trade_price;
    
    // Log the trade
    logTrade(trade_price, trade_quantity);
    total_trades_executed++;
}

void MarketMaker::manageInventory() {
    if (std::abs(current_position) > config.position_limit) {
        // Reduce exposure by adjusting spreads or canceling orders
        // For now, we'll just log this condition
        std::cout << "Position limit exceeded: " << current_position << "\n";
    }
}

bool MarketMaker::shouldReduceExposure() const {
    return std::abs(current_position) > config.position_limit;
}

void MarketMaker::checkRiskLimits() {
    // Check stop loss
    if (checkStopLoss()) {
        std::cout << "Stop loss triggered!\n";
        emergencyShutdown();
        return;
    }
    
    // Check maximum loss limit
    if (total_pnl.load() < max_loss_limit) {
        std::cout << "Maximum loss limit exceeded!\n";
        emergencyShutdown();
        return;
    }
    
    // Check position limits
    if (std::abs(current_position) > config.max_position_size) {
        std::cout << "Maximum position size exceeded!\n";
        emergencyShutdown();
        return;
    }
}

void MarketMaker::emergencyShutdown() {
    std::cout << "EMERGENCY SHUTDOWN TRIGGERED!\n";
    emergency_stop = true;
    cancelAllOrders();
}

bool MarketMaker::isRiskLimitExceeded() const {
    return emergency_stop || 
           total_pnl.load() < max_loss_limit ||
           std::abs(current_position) > config.max_position_size;
}

void MarketMaker::updatePnL() {
    // This would typically be done by the PnL calculator
    // For now, we'll just update the unrealized PnL
    double current_price = price_generator->getCurrentPrice();
    double unrealized = (current_price - (current_inventory / std::max(current_position, 0.001))) * current_position;
    unrealized_pnl.store(unrealized);
}

double MarketMaker::calculateUnrealizedPnL() const {
    return unrealized_pnl.load();
}

double MarketMaker::calculateRealizedPnL() const {
    return realized_pnl.load();
}

void MarketMaker::printStatus() const {
    std::cout << getStatusString();
}

std::string MarketMaker::getStatusString() const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2);
    
    oss << "\n=== Market Maker Status ===\n";
    oss << "Current Position: " << current_position << "\n";
    oss << "Current Inventory: " << current_inventory << "\n";
    oss << "Total PnL: " << total_pnl.load() << "\n";
    oss << "Realized PnL: " << realized_pnl.load() << "\n";
    oss << "Unrealized PnL: " << unrealized_pnl.load() << "\n";
    oss << "Active Buy Orders: " << active_buy_orders.size() << "\n";
    oss << "Active Sell Orders: " << active_sell_orders.size() << "\n";
    oss << "Total Orders Placed: " << total_orders_placed << "\n";
    oss << "Total Trades Executed: " << total_trades_executed << "\n";
    oss << "Emergency Stop: " << (emergency_stop ? "YES" : "NO") << "\n";
    oss << "Risk Limit Exceeded: " << (isRiskLimitExceeded() ? "YES" : "NO") << "\n";
    
    // Current market conditions
    double mid_price = order_book->getMidPrice();
    double spread = order_book->getSpread();
    oss << "Mid Price: " << mid_price << "\n";
    oss << "Market Spread: " << spread << "\n";
    oss << "Our Spread: " << (calculateDynamicSpread() * 10000) << " bps\n";
    
    oss << "======================\n";
    
    return oss.str();
}

double MarketMaker::getSharpeRatio() const {
    // This would require historical return data
    // For now, return a placeholder value
    return 0.0;
}

double MarketMaker::getMaxDrawdown() const {
    // This would require historical PnL data
    // For now, return a placeholder value
    return 0.0;
}

void MarketMaker::updateConfig(const MarketMakerConfig& new_config) {
    std::lock_guard<std::mutex> lock(market_maker_mutex);
    config = new_config;
    
    // Update internal parameters
    max_loss_limit = config.max_loss_limit;
    stop_loss_threshold = config.stop_loss_threshold;
}

bool MarketMaker::isRunning() const {
    return !emergency_stop;
}

void MarketMaker::stop() {
    emergency_stop = true;
    cancelAllOrders();
}

void MarketMaker::reset() {
    emergency_stop = false;
    current_position = 0.0;
    current_inventory = 0.0;
    total_pnl.store(0.0);
    realized_pnl.store(0.0);
    unrealized_pnl.store(0.0);
    total_orders_placed = 0;
    total_trades_executed = 0;
    
    active_buy_orders.clear();
    active_sell_orders.clear();
    trade_history.clear();
    
    start_time = std::chrono::system_clock::now();
}

void MarketMaker::placeBuyOrder(double price) {
    uint64_t order_id = order_book->addOrder(OrderSide::BUY, OrderType::LIMIT, price, config.order_size);
    if (order_id > 0) {
        active_buy_orders.push_back(order_id);
        total_orders_placed++;
    }
}

void MarketMaker::placeSellOrder(double price) {
    uint64_t order_id = order_book->addOrder(OrderSide::SELL, OrderType::LIMIT, price, config.order_size);
    if (order_id > 0) {
        active_sell_orders.push_back(order_id);
        total_orders_placed++;
    }
}

void MarketMaker::manageOrderBook() {
    // This method can be used for more sophisticated order book management
    // For now, it's a placeholder
}

double MarketMaker::calculateOptimalOrderSize() const {
    // Base order size adjusted by position
    double base_size = config.order_size;
    
    // Reduce size if position is large
    if (std::abs(current_position) > config.position_limit * 0.5) {
        base_size *= 0.5;
    }
    
    return std::max(base_size, 1.0);
}

void MarketMaker::logTrade(double price, double quantity) {
    trade_history.emplace_back(price, quantity);
    
    // Keep only recent trades
    if (trade_history.size() > 1000) {
        trade_history.pop_front();
    }
}

void MarketMaker::updatePerformanceMetrics() {
    // Update running statistics
    // This could include order fill rates, latency measurements, etc.
}

double MarketMaker::calculateVaR(double confidence_level) const {
    // Value at Risk calculation
    // This is a simplified implementation
    double volatility = price_generator->calculateRealizedVolatility();
    double position_value = std::abs(current_position) * price_generator->getCurrentPrice();
    
    // Assuming normal distribution, 95% VaR = 1.645 * volatility * position_value
    double var_multiplier = 1.645; // 95% confidence
    if (confidence_level == 0.99) {
        var_multiplier = 2.326; // 99% confidence
    }
    
    return var_multiplier * volatility * position_value;
}

double MarketMaker::calculatePositionRisk() const {
    // Calculate current position risk
    double position_value = std::abs(current_position) * price_generator->getCurrentPrice();
    double max_position_value = config.max_position_size * price_generator->getCurrentPrice();
    
    return position_value / max_position_value;
}

bool MarketMaker::checkStopLoss() const {
    return total_pnl.load() < stop_loss_threshold;
}

} // namespace hft
