#!/bin/bash

echo "Building High-Frequency Market Making Simulator..."

# Create bin directory if it doesn't exist
mkdir -p bin

# Compile flags
CXXFLAGS="-std=c++17 -O3 -Wall -Wextra -pthread"
INCLUDES="-Iinclude -Isrc"

# Source files
SOURCES=(
    "src/Order.cpp"
    "src/OrderBook.cpp"
    "src/PriceGenerator.cpp"
    "src/PnLCalculator.cpp"
    "src/MarketMaker.cpp"
    "src/SimulationEngine.cpp"
    "src/utils.cpp"
    "src/main.cpp"
)

# Build main executable
echo "Building main executable..."
g++ $CXXFLAGS $INCLUDES -o bin/HighFrequencyMarketMaker "${SOURCES[@]}"

if [ $? -eq 0 ]; then
    echo "✅ Main executable built successfully!"
    echo "Location: bin/HighFrequencyMarketMaker"
else
    echo "❌ Build failed!"
    exit 1
fi

# Build test executable
echo "Building test executable..."
g++ $CXXFLAGS $INCLUDES -o bin/test_basic src/test_basic.cpp src/Order.cpp src/OrderBook.cpp src/PriceGenerator.cpp src/PnLCalculator.cpp src/MarketMaker.cpp src/SimulationEngine.cpp src/utils.cpp

if [ $? -eq 0 ]; then
    echo "✅ Test executable built successfully!"
    echo "Location: bin/test_basic"
else
    echo "❌ Test build failed!"
fi

echo ""
echo "🎉 Build complete! You can now run:"
echo "  ./bin/HighFrequencyMarketMaker    # Main simulation"
echo "  ./bin/test_basic                  # Run tests"
