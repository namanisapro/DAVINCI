#include "HFTMarketMaker.h"
#include "SimulationEngine.h"
#include <iostream>
#include <cassert>
#include <memory>

using namespace hft;

void testOrder() {
    std::cout << "Testing Order class...\n";
    
    Order order(1, "AAPL", OrderSide::BUY, OrderType::LIMIT, 150.0, 100.0);
    
    assert(order.order_id == 1);
    assert(order.symbol == "AAPL");
    assert(order.side == OrderSide::BUY);
    assert(order.type == OrderType::LIMIT);
    assert(order.price == 150.0);
    assert(order.quantity == 100.0);
    assert(order.isActive());
    assert(!order.isFilled());
    assert(order.getRemainingQuantity() == 100.0);
    
    order.updateFill(50.0);
    assert(order.filled_quantity == 50.0);
    assert(order.getRemainingQuantity() == 50.0);
    assert(order.status == OrderStatus::PARTIALLY_FILLED);
    
    order.updateFill(50.0);
    assert(order.isFilled());
    assert(order.status == OrderStatus::FILLED);
    
    std::cout << "Order tests passed!\n";
}

void testOrderBook() {
    std::cout << "Testing OrderBook class...\n";
    
    auto order_book = std::make_shared<OrderBook>("AAPL");
    
    // Test adding orders
    uint64_t bid_id = order_book->addOrder(OrderSide::BUY, OrderType::LIMIT, 149.0, 100.0);
    uint64_t ask_id = order_book->addOrder(OrderSide::SELL, OrderType::LIMIT, 151.0, 100.0);
    
    assert(bid_id > 0);
    assert(ask_id > 0);
    
    // Test order book queries
    assert(order_book->getBestBid() == 149.0);
    assert(order_book->getBestAsk() == 151.0);
    assert(order_book->getMidPrice() == 150.0);
    assert(order_book->getSpread() == 2.0);
    
    // Test order cancellation
    assert(order_book->cancelOrder(bid_id));
    assert(order_book->getBestBid() == 0.0);
    
    std::cout << "OrderBook tests passed!\n";
}

void testPriceGenerator() {
    std::cout << "Testing PriceGenerator class...\n";
    
    auto price_gen = std::make_shared<PriceGenerator>(100.0, 0.05, 0.20);
    
    double initial_price = price_gen->getCurrentPrice();
    assert(initial_price == 100.0);
    
    // Generate a few prices
    double price1 = price_gen->generateNextPrice();
    double price2 = price_gen->generateNextPrice();
    
    assert(price1 > 0);
    assert(price2 > 0);
    assert(price_gen->getTicksGenerated() == 2);
    
    // Test volatility calculation
    double vol = price_gen->calculateRealizedVolatility();
    assert(vol >= 0);
    
    std::cout << "PriceGenerator tests passed!\n";
}

void testPnLCalculator() {
    std::cout << "Testing PnLCalculator class...\n";
    
    auto pnl_calc = std::make_shared<PnLCalculator>();
    
    // Test trade recording
    pnl_calc->recordTrade(150.0, 100.0, 1.0);   // Buy 100 at 150
    pnl_calc->recordTrade(151.0, 100.0, -1.0);  // Sell 100 at 151
    
    assert(pnl_calc->getTradeCount() == 2);
    assert(pnl_calc->getCurrentPosition() == 0.0);
    
    // Test mark price update
    pnl_calc->updateMarkPrice(152.0);
    
    std::cout << "PnLCalculator tests passed!\n";
}

void testMarketMaker() {
    std::cout << "Testing MarketMaker class...\n";
    
    auto order_book = std::make_shared<OrderBook>("AAPL");
    auto price_gen = std::make_shared<PriceGenerator>(150.0, 0.05, 0.20);
    
    MarketMakerConfig config;
    config.base_spread_bps = 10.0;
    config.order_size = 100.0;
    
    auto market_maker = std::make_shared<MarketMaker>(order_book, price_gen, config);
    
    // Test basic functionality
    assert(!market_maker->isRunning());
    assert(!market_maker->isRiskLimitExceeded());
    
    std::cout << "MarketMaker tests passed!\n";
}

void testSimulationEngine() {
    std::cout << "Testing SimulationEngine class...\n";
    
    SystemConfig sys_config;
    sys_config.symbol = "AAPL";
    sys_config.initial_price = 150.0;
    sys_config.simulation_duration_ms = 1000;
    
    MarketMakerConfig mm_config;
    mm_config.base_spread_bps = 10.0;
    mm_config.order_size = 100.0;
    
    SimulationEngine engine(sys_config, mm_config);
    
    // Test basic functionality
    assert(!engine.isRunning());
    
    std::cout << "SimulationEngine tests passed!\n";
}

int main() {
    std::cout << "Starting HFT Market Maker Basic Tests...\n";
    std::cout << "========================================\n\n";
    
    try {
        testOrder();
        testOrderBook();
        testPriceGenerator();
        testPnLCalculator();
        testMarketMaker();
        testSimulationEngine();
        
        std::cout << "\n========================================\n";
        std::cout << "All basic tests passed successfully!\n";
        std::cout << "========================================\n";
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << "\n";
        return 1;
    } catch (...) {
        std::cerr << "Test failed with unknown exception\n";
        return 1;
    }
}
