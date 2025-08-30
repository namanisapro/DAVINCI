@echo off
echo ========================================
echo High-Frequency Trading Market Maker
echo Build Script for Windows
echo ========================================
echo.

REM Check if CMake is available
cmake --version >nul 2>&1
if errorlevel 1 (
    echo ERROR: CMake is not installed or not in PATH
    echo Please install CMake from https://cmake.org/download/
    pause
    exit /b 1
)

REM Check if Visual Studio is available
where cl >nul 2>&1
if errorlevel 1 (
    echo WARNING: Visual Studio compiler not found in PATH
    echo This may cause build issues
    echo.
)

REM Create build directory
if not exist "build" (
    echo Creating build directory...
    mkdir build
)

REM Change to build directory
cd build

REM Configure with CMake
echo Configuring project with CMake...
cmake -DCMAKE_BUILD_TYPE=Release ..
if errorlevel 1 (
    echo ERROR: CMake configuration failed
    pause
    exit /b 1
)

REM Build the project
echo.
echo Building project...
cmake --build . --config Release
if errorlevel 1 (
    echo ERROR: Build failed
    pause
    exit /b 1
)

echo.
echo ========================================
echo Build completed successfully!
echo ========================================
echo.
echo Executable location: build\bin\HighFrequencyMarketMaker.exe
echo.
echo To run the application:
echo 1. Navigate to build\bin
echo 2. Run HighFrequencyMarketMaker.exe
echo.
pause
