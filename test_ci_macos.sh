#!/bin/bash
# Test macOS CI/CD locally
# This script simulates the macOS CI environment

echo "=== Testing macOS CI/CD Setup ==="
echo ""

# Check if we're on macOS
if [[ "$OSTYPE" != "darwin"* ]]; then
    echo "❌ This script is for macOS only"
    echo "   Current OS: $OSTYPE"
    exit 1
fi

# Check if brew is available
if ! command -v brew &> /dev/null; then
    echo "❌ Homebrew not found. Please install Homebrew first."
    exit 1
fi

echo "✅ Homebrew found"
echo ""

# Get Boost root
BOOST_ROOT=$(brew --prefix boost 2>/dev/null)
if [ -z "$BOOST_ROOT" ]; then
    echo "❌ Boost not installed. Installing..."
    brew install boost || {
        echo "❌ Failed to install Boost"
        exit 1
    }
    BOOST_ROOT=$(brew --prefix boost)
fi

echo "✅ Boost root: $BOOST_ROOT"
echo ""

# Check if Boost lib directory exists
if [ ! -d "$BOOST_ROOT/lib" ]; then
    echo "❌ Boost lib directory not found: $BOOST_ROOT/lib"
    exit 1
fi

echo "=== All files in Boost lib directory ==="
ls -la "$BOOST_ROOT/lib/" | head -20
echo ""

echo "=== All Boost libraries ==="
ls -1 "$BOOST_ROOT/lib/" | grep -i boost | head -20
echo ""

echo "=== Looking for boost_system specifically ==="
find "$BOOST_ROOT/lib" -name "*boost_system*" -type f
echo ""

echo "=== Library file details ==="
if [ -f "$BOOST_ROOT/lib/libboost_system.dylib" ]; then
    echo "✅ Found: libboost_system.dylib"
    file "$BOOST_ROOT/lib/libboost_system.dylib"
elif [ -f "$BOOST_ROOT/lib/libboost_system.a" ]; then
    echo "✅ Found: libboost_system.a"
    file "$BOOST_ROOT/lib/libboost_system.a"
elif [ -f "$BOOST_ROOT/lib/libboost_system-mt.dylib" ]; then
    echo "✅ Found: libboost_system-mt.dylib"
    file "$BOOST_ROOT/lib/libboost_system-mt.dylib"
elif [ -f "$BOOST_ROOT/lib/libboost_system-mt.a" ]; then
    echo "✅ Found: libboost_system-mt.a"
    file "$BOOST_ROOT/lib/libboost_system-mt.a"
else
    echo "❌ No boost_system library found with standard names"
    echo ""
    echo "Available boost libraries (first 20):"
    ls -1 "$BOOST_ROOT/lib/" | grep boost | head -20
    echo ""
    echo "Trying to find any file containing 'system':"
    ls -1 "$BOOST_ROOT/lib/" | grep -i system | head -10
fi
echo ""

# Test CMake detection
echo "=== Testing CMake Boost Detection ==="
mkdir -p test_build
cd test_build

# Set environment variables like CI does
export BOOST_ROOT
export BOOST_INCLUDEDIR="$BOOST_ROOT/include"
export BOOST_LIBRARYDIR="$BOOST_ROOT/lib"
export OPENSSL_ROOT_DIR=$(brew --prefix openssl@3 2>/dev/null || brew --prefix openssl)
export CMAKE_PREFIX_PATH="$BOOST_ROOT:$OPENSSL_ROOT_DIR:$CMAKE_PREFIX_PATH"

# Unset Boost_DIR to force FindBoost
unset Boost_DIR

echo "Environment variables:"
echo "  BOOST_ROOT=$BOOST_ROOT"
echo "  BOOST_INCLUDEDIR=$BOOST_INCLUDEDIR"
echo "  BOOST_LIBRARYDIR=$BOOST_LIBRARYDIR"
echo "  OPENSSL_ROOT_DIR=$OPENSSL_ROOT_DIR"
echo ""

# Try CMake configuration
echo "Running CMake configuration..."
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DKRAKEN_BUILD_TESTS=OFF \
    -DKRAKEN_BUILD_EXAMPLES=OFF \
    -DBOOST_ROOT="$BOOST_ROOT" \
    -DBOOST_INCLUDEDIR="$BOOST_INCLUDEDIR" \
    -DBOOST_LIBRARYDIR="$BOOST_LIBRARYDIR" \
    -DBoost_NO_BOOST_CMAKE=ON \
    -DBoost_USE_STATIC_LIBS=OFF \
    -DBoost_USE_MULTITHREADED=ON \
    -DOPENSSL_ROOT_DIR="$OPENSSL_ROOT_DIR" \
    2>&1 | tee cmake_output.log

CMAKE_EXIT_CODE=$?

if [ $CMAKE_EXIT_CODE -eq 0 ]; then
    echo ""
    echo "✅ CMake configuration succeeded!"
    echo ""
    echo "Boost detection details:"
    grep -i "boost" cmake_output.log | head -20
else
    echo ""
    echo "❌ CMake configuration failed with exit code: $CMAKE_EXIT_CODE"
    echo ""
    echo "Error details:"
    grep -i "error\|boost\|not found" cmake_output.log | head -30
fi

cd ..
rm -rf test_build

echo ""
echo "=== Test Complete ==="

