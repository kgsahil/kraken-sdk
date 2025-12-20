#!/bin/bash
# Setup script for building Kraken SDK in WSL

set -e

echo "=== Installing dependencies ==="
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    git \
    libboost-dev \
    libboost-system-dev \
    libboost-filesystem-dev \
    libssl-dev

echo ""
echo "=== Verifying Boost installation ==="
if [ -d "/usr/include/boost" ]; then
    echo "✓ Boost headers found at /usr/include/boost"
    ls -d /usr/include/boost/version.hpp 2>/dev/null && echo "✓ Boost version header found"
else
    echo "✗ Boost headers not found!"
    echo "Try: sudo apt-get install --reinstall libboost-dev"
    exit 1
fi

echo ""
echo "=== Configuring CMake ==="
cd "$(dirname "$0")"
mkdir -p build
cd build
rm -f CMakeCache.txt

cmake .. || {
    echo ""
    echo "CMake configuration failed. Trying with explicit Boost path..."
    cmake .. -DBoost_NO_BOOST_CMAKE=ON -DBOOST_ROOT=/usr -DBoost_INCLUDE_DIR=/usr/include
}

echo ""
echo "=== Building ==="
cmake --build . -j$(nproc)

echo ""
echo "=== Build complete! ==="
echo "Run examples from: build/examples/"

