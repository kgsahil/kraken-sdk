#!/bin/bash
# Script to run clang-tidy locally (if available) or check for common issues

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"

cd "$PROJECT_ROOT"

echo "🔍 Checking clang-tidy configuration..."
echo ""

# Check if clang-tidy is available
if ! command -v clang-tidy &> /dev/null; then
    echo "⚠️  clang-tidy not found. Installing..."
    echo "   On Ubuntu/WSL: sudo apt-get install -y clang-tidy"
    echo "   On macOS: brew install llvm"
    echo ""
    echo "📋 Common clang-tidy issues to check manually:"
    echo "   1. Missing 'const' on methods that don't modify state"
    echo "   2. Missing 'noexcept' on methods that don't throw"
    echo "   3. Unused variables"
    echo "   4. Missing 'constexpr' where applicable"
    echo "   5. Unnecessary copies (use const& or move)"
    echo "   6. Raw pointers instead of smart pointers"
    echo "   7. Missing 'override' on virtual methods"
    echo ""
    exit 0
fi

# Check if compile_commands.json exists
if [ ! -f "$BUILD_DIR/compile_commands.json" ]; then
    echo "⚠️  compile_commands.json not found."
    echo "   Run: mkdir -p build && cd build && cmake .. -DCMAKE_EXPORT_COMPILE_COMMANDS=ON"
    exit 1
fi

echo "✅ Found compile_commands.json"
echo ""

# Run clang-tidy on source files
echo "🔍 Running clang-tidy on source files..."
echo ""

cd "$BUILD_DIR"

# Find all source files
SOURCES=$(find ../src ../include -name '*.cpp' -o -name '*.hpp' | grep -v '_deps' | sort)

ERROR_COUNT=0
WARNING_COUNT=0

for file in $SOURCES; do
    echo "Checking: $file"
    OUTPUT=$(clang-tidy "$file" -p . --config-file=../.clang-tidy --extra-arg=-std=c++17 2>&1 || true)
    
    ERRORS=$(echo "$OUTPUT" | grep -c "error:" || echo "0")
    WARNINGS=$(echo "$OUTPUT" | grep -c "warning:" || echo "0")
    
    if [ "$ERRORS" -gt 0 ] || [ "$WARNINGS" -gt 0 ]; then
        echo "$OUTPUT" | head -20
        ERROR_COUNT=$((ERROR_COUNT + ERRORS))
        WARNING_COUNT=$((WARNING_COUNT + WARNINGS))
        echo ""
    fi
done

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "📊 Summary:"
echo "   Errors: $ERROR_COUNT"
echo "   Warnings: $WARNING_COUNT"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

if [ "$ERROR_COUNT" -gt 0 ]; then
    echo ""
    echo "❌ Found $ERROR_COUNT errors. Please fix them."
    exit 1
else
    echo ""
    echo "✅ No errors found!"
    exit 0
fi

