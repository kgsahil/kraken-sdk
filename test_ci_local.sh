#!/bin/bash
# Local CI/CD test script
# Tests the build process locally before pushing to GitHub

set -e  # Exit on error

echo "╔═══════════════════════════════════════════════════════════════╗"
echo "║           LOCAL CI/CD TEST - KRAKEN SDK                      ║"
echo "╚═══════════════════════════════════════════════════════════════╝"
echo ""

# Detect OS
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    OS="linux"
elif [[ "$OSTYPE" == "darwin"* ]]; then
    OS="macos"
elif [[ "$OSTYPE" == "msys" ]] || [[ "$OSTYPE" == "cygwin" ]]; then
    OS="windows"
else
    OS="unknown"
fi

echo "Detected OS: $OS"
echo ""

# Test function
test_build() {
    local platform=$1
    local build_dir="build_test_${platform}"
    
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo "Testing: $platform"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    
    # Clean previous test build
    if [ -d "$build_dir" ]; then
        echo "Cleaning previous test build..."
        rm -rf "$build_dir"
    fi
    
    mkdir -p "$build_dir"
    cd "$build_dir"
    
    echo "Step 1: Configuring CMake..."
    if [ "$platform" == "windows" ]; then
        # Windows would use vcpkg, but for local test we'll try without
        cmake .. \
            -DCMAKE_BUILD_TYPE=Release \
            -DKRAKEN_BUILD_TESTS=ON \
            -DKRAKEN_BUILD_EXAMPLES=ON \
            || { echo "❌ CMake configuration failed"; cd ..; return 1; }
    else
        cmake .. \
            -DCMAKE_BUILD_TYPE=Release \
            -DKRAKEN_BUILD_TESTS=ON \
            -DKRAKEN_BUILD_EXAMPLES=ON \
            || { echo "❌ CMake configuration failed"; cd ..; return 1; }
    fi
    
    echo "✅ CMake configuration successful"
    
    echo ""
    echo "Step 2: Building..."
    if [ "$platform" == "windows" ]; then
        cmake --build . --config Release --parallel || { echo "❌ Build failed"; cd ..; return 1; }
    else
        cmake --build . -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4) || { echo "❌ Build failed"; cd ..; return 1; }
    fi
    
    echo "✅ Build successful"
    
    echo ""
    echo "Step 3: Running tests..."
    if [ "$platform" == "windows" ]; then
        ctest --output-on-failure --verbose -C Release --parallel || { echo "❌ Tests failed"; cd ..; return 1; }
    else
        ctest --output-on-failure --verbose -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4) || { echo "❌ Tests failed"; cd ..; return 1; }
    fi
    
    echo "✅ All tests passed"
    
    cd ..
    echo ""
    echo "✅ $platform build test PASSED"
    echo ""
}

# Main test
main() {
    echo "Starting local CI/CD test..."
    echo ""
    
    # Test based on detected OS
    case $OS in
        linux)
            test_build "linux"
            ;;
        macos)
            test_build "macos"
            ;;
        windows)
            echo "⚠️  Windows build test requires vcpkg setup"
            echo "   For full Windows test, use PowerShell and vcpkg"
            echo "   Testing Linux build in WSL instead..."
            test_build "linux"
            ;;
        *)
            echo "⚠️  Unknown OS, testing Linux build..."
            test_build "linux"
            ;;
    esac
    
    echo "╔═══════════════════════════════════════════════════════════════╗"
    echo "║              ✅ LOCAL CI/CD TEST COMPLETE                     ║"
    echo "╚═══════════════════════════════════════════════════════════════╝"
    echo ""
    echo "If all tests passed, you can push to GitHub!"
}

# Run main
main

