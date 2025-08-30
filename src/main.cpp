#include "Config.h"
#include "HFTMarketMaker.h"
#include "SimulationEngine.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <string>
#include <limits>

using namespace hft;

void printBanner() {
    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                                                              ║\n";
    std::cout << "║        HIGH-FREQUENCY TRADING MARKET MAKER SIMULATOR        ║\n";
    std::cout << "║                                                              ║\n";
    std::cout << "║                    Version 1.0.0                            ║\n";
    std::cout << "║                                                              ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════╝\n";
    std::cout << "\n";
}

void printMenu() {
    std::cout << "\n=== Main Menu ===\n";
    std::cout << "1. Run Quick Simulation (30 seconds)\n";
    std::cout << "2. Run Standard Simulation (2 minutes)\n";
    std::cout << "3. Run Extended Simulation (5 minutes)\n";
    std::cout << "4. Custom Simulation Settings\n";
    std::cout << "5. View System Status\n";
    std::cout << "6. Export Data\n";
    std::cout << "7. Performance Test\n";
    std::cout << "8. Exit\n";
    std::cout << "================\n";
    std::cout << "Enter your choice: ";
}

void runSimulation(SimulationEngine& engine, uint64_t duration_ms) {
    std::cout << "\nStarting simulation for " << (duration_ms / 1000) << " seconds...\n";
    std::cout << "Press Ctrl+C to stop early.\n\n";
    
    try {
        engine.start();
        
        // Wait for simulation to complete
        auto start_time = std::chrono::system_clock::now();
        auto end_time = start_time + std::chrono::milliseconds(duration_ms);
        
        while (engine.isRunning() && std::chrono::system_clock::now() < end_time) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
            // Print status every 5 seconds
            static auto last_status = std::chrono::system_clock::now();
            if (std::chrono::system_clock::now() - last_status > std::chrono::seconds(5)) {
                engine.printStatus();
                last_status = std::chrono::system_clock::now();
            }
        }
        
        engine.stop();
        
        std::cout << "\nSimulation completed!\n";
        engine.printStatus();
        
        // Generate report
        std::string report_filename = "simulation_report_" + 
                                    std::to_string(std::chrono::duration_cast<std::chrono::seconds>(
                                        std::chrono::system_clock::now().time_since_epoch()).count()) + ".txt";
        engine.generateReport(report_filename);
        
    } catch (const std::exception& e) {
        std::cerr << "Error during simulation: " << e.what() << "\n";
        engine.stop();
    }
}

void customSimulation(SimulationEngine& engine) {
    std::cout << "\n=== Custom Simulation Settings ===\n";
    
    SystemConfig sys_config = engine.getSystemConfig();
    MarketMakerConfig mm_config = engine.getMarketMakerConfig();
    
    // System configuration
    std::cout << "Current initial price: " << sys_config.initial_price << "\n";
    std::cout << "Enter new initial price (or press Enter to keep current): ";
    std::string input;
    std::getline(std::cin, input);
    if (!input.empty()) {
        try {
            sys_config.initial_price = std::stod(input);
        } catch (...) {
            std::cout << "Invalid price, keeping current value.\n";
        }
    }
    
    std::cout << "Current simulation duration: " << (sys_config.simulation_duration_ms / 1000) << " seconds\n";
    std::cout << "Enter new duration in seconds (or press Enter to keep current): ";
    std::getline(std::cin, input);
    if (!input.empty()) {
        try {
            sys_config.simulation_duration_ms = std::stoul(input) * 1000;
        } catch (...) {
            std::cout << "Invalid duration, keeping current value.\n";
        }
    }
    
    // Market maker configuration
    std::cout << "Current base spread: " << mm_config.base_spread_bps << " bps\n";
    std::cout << "Enter new base spread in bps (or press Enter to keep current): ";
    std::getline(std::cin, input);
    if (!input.empty()) {
        try {
            mm_config.base_spread_bps = std::stod(input);
        } catch (...) {
            std::cout << "Invalid spread, keeping current value.\n";
        }
    }
    
    std::cout << "Current order size: " << mm_config.order_size << "\n";
    std::cout << "Enter new order size (or press Enter to keep current): ";
    std::getline(std::cin, input);
    if (!input.empty()) {
        try {
            mm_config.order_size = std::stod(input);
        } catch (...) {
            std::cout << "Invalid order size, keeping current value.\n";
        }
    }
    
    // Update configurations
    engine.updateSystemConfig(sys_config);
    engine.updateMarketMakerConfig(mm_config);
    
    std::cout << "\nConfiguration updated!\n";
    
    // Run simulation
    runSimulation(engine, sys_config.simulation_duration_ms);
}

void exportData(SimulationEngine& engine) {
    std::cout << "\n=== Export Data ===\n";
    std::cout << "1. Export Order Book Data\n";
    std::cout << "2. Export Trade Data\n";
    std::cout << "3. Export PnL Data\n";
    std::cout << "4. Export All Data\n";
    std::cout << "5. Back to Main Menu\n";
    std::cout << "Enter your choice: ";
    
    int choice;
    std::cin >> choice;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    
    switch (choice) {
        case 1:
            engine.exportOrderBookData("data/orderbook_data.csv");
            break;
        case 2:
            engine.exportTradeData("data/trade_data.csv");
            break;
        case 3:
            engine.exportPnLData("data/pnl_data.csv");
            break;
        case 4:
            engine.exportOrderBookData("data/orderbook_data.csv");
            engine.exportTradeData("data/trade_data.csv");
            engine.exportPnLData("data/pnl_data.csv");
            std::cout << "All data exported!\n";
            break;
        case 5:
            return;
        default:
            std::cout << "Invalid choice.\n";
            break;
    }
}

void performanceTest() {
    std::cout << "\n=== Performance Test ===\n";
    std::cout << "Running performance benchmark...\n";
    
    // Create a simple order book for testing
    auto order_book = std::make_shared<OrderBook>("TEST");
    
    // Measure order insertion performance
    auto start_time = std::chrono::high_resolution_clock::now();
    
    const int num_orders = 100000;
    for (int i = 0; i < num_orders; ++i) {
        double price = 100.0 + (i % 100) * 0.01;
        double quantity = 100.0 + (i % 50);
        OrderSide side = (i % 2 == 0) ? OrderSide::BUY : OrderSide::SELL;
        
        order_book->addOrder(side, OrderType::LIMIT, price, quantity);
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    
    double orders_per_second = (num_orders * 1000000.0) / duration.count();
    
    std::cout << "Performance Results:\n";
    std::cout << "  Orders inserted: " << num_orders << "\n";
    std::cout << "  Time taken: " << duration.count() << " microseconds\n";
    std::cout << "  Orders per second: " << std::fixed << std::setprecision(0) << orders_per_second << "\n";
    std::cout << "  Microseconds per order: " << std::fixed << std::setprecision(2) 
              << (duration.count() / static_cast<double>(num_orders)) << "\n";
    
    // Test order book queries
    start_time = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < 10000; ++i) {
        order_book->getBestBid();
        order_book->getBestAsk();
        order_book->getMidPrice();
        order_book->getSpread();
    }
    
    end_time = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    
    double queries_per_second = (40000 * 1000000.0) / duration.count();
    
    std::cout << "\nQuery Performance:\n";
    std::cout << "  Queries executed: " << 40000 << "\n";
    std::cout << "  Time taken: " << duration.count() << " microseconds\n";
    std::cout << "  Queries per second: " << std::fixed << std::setprecision(0) << queries_per_second << "\n";
    
    std::cout << "\nPerformance test completed!\n";
}

int main() {
    printBanner();
    
    // Create default configurations
    SystemConfig sys_config;
    sys_config.symbol = "AAPL";
    sys_config.initial_price = 150.0;
    sys_config.simulation_duration_ms = 120000; // 2 minutes
    sys_config.tick_interval_ms = 10;           // 100 ticks per second
    
    MarketMakerConfig mm_config;
    mm_config.base_spread_bps = 15.0;          // 15 basis points
    mm_config.min_spread_bps = 5.0;            // 5 basis points
    mm_config.max_spread_bps = 50.0;           // 50 basis points
    mm_config.volatility_multiplier = 2.0;
    mm_config.max_position_size = 1000.0;
    mm_config.position_limit = 500.0;
    mm_config.order_refresh_ms = 100;           // 100ms refresh
    mm_config.order_size = 100.0;
    mm_config.dynamic_spread = true;
    mm_config.risk_management = true;
    mm_config.max_loss_limit = -10000.0;       // $10,000 max loss
    mm_config.stop_loss_threshold = -5000.0;   // $5,000 stop loss
    
    // Create simulation engine
    SimulationEngine engine(sys_config, mm_config);
    
    std::cout << "System initialized with default configuration.\n";
    std::cout << "Symbol: " << sys_config.symbol << "\n";
    std::cout << "Initial Price: $" << sys_config.initial_price << "\n";
    std::cout << "Base Spread: " << mm_config.base_spread_bps << " basis points\n";
    std::cout << "Order Size: " << mm_config.order_size << " shares\n\n";
    
    int choice;
    bool running = true;
    
    while (running) {
        printMenu();
        std::cin >> choice;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        
        switch (choice) {
            case 1:
                runSimulation(engine, 30000); // 30 seconds
                break;
            case 2:
                runSimulation(engine, 120000); // 2 minutes
                break;
            case 3:
                runSimulation(engine, 300000); // 5 minutes
                break;
            case 4:
                customSimulation(engine);
                break;
            case 5:
                engine.printStatus();
                break;
            case 6:
                exportData(engine);
                break;
            case 7:
                performanceTest();
                break;
            case 8:
                running = false;
                std::cout << "Goodbye!\n";
                break;
            default:
                std::cout << "Invalid choice. Please try again.\n";
                break;
        }
        
        if (running) {
            std::cout << "\nPress Enter to continue...";
            std::cin.get();
        }
    }
    
    return 0;
}
