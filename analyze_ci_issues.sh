#!/bin/bash
# Analyze CI/CD issues without pushing to GitHub
# This script helps identify potential issues in CI/CD configuration

echo "╔═══════════════════════════════════════════════════════════════╗"
echo "║         CI/CD Issue Analysis Tool                            ║"
echo "╚═══════════════════════════════════════════════════════════════╝"
echo ""

# Check if we're in the right directory
if [ ! -f ".github/workflows/ci.yml" ]; then
    echo "❌ Error: .github/workflows/ci.yml not found"
    echo "   Please run this script from the project root"
    exit 1
fi

echo "✅ Found CI/CD workflow file"
echo ""

# Analyze Windows issues
echo "=== Windows CI Analysis ==="
echo ""

# Check vcpkg command syntax
echo "Checking vcpkg command in workflow..."
if grep -q "vcpkg.exe list" .github/workflows/ci.yml; then
    echo "✅ Found vcpkg list command"
    
    # Check if error handling is correct
    if grep -q "No packages are installed" .github/workflows/ci.yml; then
        echo "✅ Error handling for 'No packages are installed' found"
    else
        echo "⚠️  Warning: No explicit handling for 'No packages are installed' message"
    fi
    
    # Check PowerShell syntax
    if grep -q "Out-String" .github/workflows/ci.yml; then
        echo "✅ Using Out-String for output capture (correct)"
    else
        echo "⚠️  Warning: May need Out-String for proper output capture"
    fi
else
    echo "❌ vcpkg list command not found"
fi
echo ""

# Analyze macOS issues
echo "=== macOS CI Analysis ==="
echo ""

# Check Boost detection logic
echo "Checking Boost detection in CMakeLists.txt..."
if grep -q "file(GLOB BOOST_LIBS" CMakeLists.txt; then
    echo "✅ Found file(GLOB) diagnostic code"
else
    echo "⚠️  Warning: No file(GLOB) diagnostic code found"
fi

if grep -q "file(GLOB BOOST_SYSTEM_CANDIDATES" CMakeLists.txt; then
    echo "✅ Found fallback glob search for boost_system"
else
    echo "⚠️  Warning: No fallback glob search found"
fi

# Check if we can test locally (if on macOS or Linux)
if [[ "$OSTYPE" == "darwin"* ]]; then
    echo ""
    echo "✅ Running on macOS - can test Boost detection"
    echo "   Run: ./test_ci_macos.sh"
elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
    echo ""
    echo "ℹ️  Running on Linux - macOS Boost detection cannot be tested here"
    echo "   But you can check the logic in CMakeLists.txt"
fi
echo ""

# Check for common issues
echo "=== Common Issues Check ==="
echo ""

# Check for hardcoded paths
if grep -q "/opt/homebrew/opt/boost" CMakeLists.txt; then
    echo "✅ Found Homebrew path handling"
else
    echo "⚠️  Warning: No explicit Homebrew path handling"
fi

# Check CMake policies
if grep -q "POLICY CMP0144" CMakeLists.txt; then
    echo "✅ CMake policy CMP0144 is conditional"
else
    echo "⚠️  Warning: CMake policy CMP0144 may not be conditional"
fi

if grep -q "POLICY CMP0167" CMakeLists.txt; then
    echo "✅ CMake policy CMP0167 is conditional"
else
    echo "⚠️  Warning: CMake policy CMP0167 may not be conditional"
fi
echo ""

# Suggest next steps
echo "=== Recommended Next Steps ==="
echo ""
echo "1. For Windows:"
echo "   - Run: powershell -ExecutionPolicy Bypass -File test_ci_windows.ps1"
echo "   - This will test vcpkg commands locally"
echo ""
echo "2. For macOS:"
echo "   - Run: ./test_ci_macos.sh"
echo "   - This will test Boost detection and CMake configuration"
echo ""
echo "3. Alternative: Use 'act' to run GitHub Actions locally"
echo "   - Install: https://github.com/nektos/act"
echo "   - Run: act -l  # List workflows"
echo "   - Run: act -j windows  # Test Windows job"
echo "   - Run: act -j macos   # Test macOS job"
echo ""
echo "4. Check workflow syntax:"
echo "   - Visit: https://github.com/kgsahil/kraken-sdk/actions/workflows/ci.yml"
echo "   - Or use: gh workflow view ci.yml (if GitHub CLI is installed)"
echo ""

# Check if act is available
if command -v act &> /dev/null; then
    echo "✅ 'act' is installed - you can run GitHub Actions locally!"
    echo "   Try: act -j windows or act -j macos"
else
    echo "ℹ️  'act' not installed - consider installing for local testing"
    echo "   See: https://github.com/nektos/act"
fi
echo ""

echo "=== Analysis Complete ==="

