#!/bin/bash

# Exit on error
set -e

echo "🚀 Starting Xenon build process..."

# 1. Detect Qt 6 path via Homebrew (check both 'qt' and 'qt@6')
QT_PATH=$(brew --prefix qt 2>/dev/null || brew --prefix qt@6 2>/dev/null)

if [ -z "$QT_PATH" ] || [ ! -d "$QT_PATH" ]; then
    echo "❌ Error: Qt 6 not found. Please run 'brew install qt'"
    exit 1
fi

echo "✅ Found Qt at: $QT_PATH"

# 2. Refresh build directory to avoid cache issues
rm -rf build
mkdir -p build
cd build

# 3. Configure CMake
# We set CMAKE_PREFIX_PATH and also hint Qt6_DIR for extra reliability
echo "🛠 Configuring CMake..."
cmake .. \
  -DCMAKE_PREFIX_PATH="$QT_PATH" \
  -DQt6_DIR="$QT_PATH/lib/cmake/Qt6" \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

# 4. Build the project
echo "🛠 Building Xenon..."
make -j$(sysctl -n hw.ncpu)

# 5. Run the application
echo "✨ Launching Xenon..."
if [[ "$OSTYPE" == "darwin"* ]]; then
    if [ -d "bin/xenon.app" ]; then
        open bin/xenon.app
    else
        ./bin/xenon
    fi
else
    ./bin/xenon
fi
