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

# Determine which files to check
if [ -n "$CI" ] || [ -n "$GITHUB_ACTIONS" ]; then
    # In CI/CD: check files changed in the PR/commit
    echo "📋 CI mode: Checking files changed in this PR/commit..."
    CHANGED_FILES=""
    
    if [ -n "$GITHUB_BASE_REF" ]; then
        # Pull request - compare against base branch
        echo "Comparing against base branch: $GITHUB_BASE_REF"
        CHANGED_FILES=$(git diff --name-only --diff-filter=ACMR origin/$GITHUB_BASE_REF...HEAD 2>/dev/null | grep -E '\.(cpp|hpp)$' || true)
    elif [ -n "$GITHUB_SHA" ]; then
        # Push to branch - try to get parent commit
        # First, fetch to ensure we have history
        git fetch --depth=2 origin HEAD 2>/dev/null || true
        # Try to compare with previous commit
        if git rev-parse --verify HEAD~1 >/dev/null 2>&1; then
            echo "Comparing against previous commit"
            CHANGED_FILES=$(git diff --name-only --diff-filter=ACMR HEAD~1 HEAD 2>/dev/null | grep -E '\.(cpp|hpp)$' || true)
        else
            # No previous commit - check all files in this commit
            echo "No previous commit found, checking all files in current commit"
            CHANGED_FILES=$(git diff-tree --no-commit-id --name-only --diff-filter=ACMR -r HEAD 2>/dev/null | grep -E '\.(cpp|hpp)$' || true)
        fi
    else
        # Fallback: check all source files
        echo "⚠️  Could not determine changed files, checking all source files"
        CHANGED_FILES=""
    fi
    
    if [ -z "$CHANGED_FILES" ]; then
        echo "ℹ️  No C++ files changed in this PR/commit. Skipping clang-tidy."
        exit 0
    fi
    SOURCES=$(echo "$CHANGED_FILES" | grep -E '^(src|include)/' | grep -v '_deps' | sort | uniq)
else
    # Local development: check modified and untracked files
    echo "📋 Local mode: Checking modified and newly added files..."
    MODIFIED=$(git diff --name-only --diff-filter=ACMR | grep -E '\.(cpp|hpp)$' || true)
    UNTRACKED=$(git ls-files --others --exclude-standard | grep -E '\.(cpp|hpp)$' || true)
    CHANGED_FILES=$(echo -e "$MODIFIED\n$UNTRACKED" | grep -E '^(src|include)/' | grep -v '_deps' | sort | uniq)
    
    if [ -z "$CHANGED_FILES" ] && [ "$CHECK_ALL" = false ]; then
        echo "ℹ️  No modified or new C++ files found."
        echo "   Run with --all flag to check all files: ./scripts/check_clang_tidy.sh --all"
        exit 0
    fi
    
    if [ "$CHECK_ALL" = true ]; then
        # Check all files
        SOURCES=$(find ../src ../include -name '*.cpp' -o -name '*.hpp' | grep -v '_deps' | sort)
    elif [ -n "$CHANGED_FILES" ]; then
        SOURCES="$CHANGED_FILES"
    else
        SOURCES=""
    fi
fi

if [ -z "$SOURCES" ]; then
    echo "⚠️  No source files to check."
    exit 0
fi

echo "📝 Files to check:"
echo "$SOURCES" | sed 's/^/   /'
echo ""

ERROR_COUNT=0
WARNING_COUNT=0

for file in $SOURCES; do
    # Convert relative path from git to absolute path
    if [[ "$file" =~ ^(src|include)/ ]]; then
        file_path="../$file"
    else
        file_path="$file"
    fi
    
    if [ ! -f "$file_path" ]; then
        echo "⚠️  Skipping: $file (file not found)"
        continue
    fi
    
    echo "Checking: $file"
    OUTPUT=$(clang-tidy "$file_path" -p . --config-file=../.clang-tidy --extra-arg=-std=c++17 2>&1 || true)
    
    ERRORS=$(echo "$OUTPUT" | grep -c "error:" 2>/dev/null || echo "0")
    WARNINGS=$(echo "$OUTPUT" | grep -c "warning:" 2>/dev/null || echo "0")
    
    # Convert to integers (handle empty strings and newlines)
    ERRORS=$(echo "$ERRORS" | tr -d '\n' | grep -o '[0-9]*' || echo "0")
    WARNINGS=$(echo "$WARNINGS" | tr -d '\n' | grep -o '[0-9]*' || echo "0")
    ERRORS=${ERRORS:-0}
    WARNINGS=${WARNINGS:-0}
    
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

