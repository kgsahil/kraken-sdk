# Local CI/CD Testing Guide

This guide helps you test and debug CI/CD issues locally without pushing to GitHub.

## Quick Start

### 1. Run Analysis Script
```bash
./analyze_ci_issues.sh
```
This will check your CI/CD configuration and suggest next steps.

### 2. Test Windows CI Locally

**Option A: Using PowerShell Script**
```powershell
powershell -ExecutionPolicy Bypass -File test_ci_windows.ps1
```

This will:
- Check if vcpkg is installed
- Test the `vcpkg list` command
- Verify error handling for "No packages are installed"
- Show what would happen in CI

**Option B: Using GitHub Actions Locally (act)**
```bash
# Install act (if not installed)
# Windows: choco install act-cli
# macOS: brew install act
# Linux: See https://github.com/nektos/act#installation

# List available jobs
act -l

# Test Windows job
act -j windows

# Test macOS job  
act -j macos

# Test Linux job
act -j linux
```

### 3. Test macOS CI Locally

**If you're on macOS:**
```bash
./test_ci_macos.sh
```

This will:
- Check Homebrew and Boost installation
- List all Boost libraries
- Test CMake Boost detection
- Show detailed diagnostics

**If you're on Linux/Windows:**
- You can't test macOS-specific Boost detection locally
- But you can review the logic in `CMakeLists.txt`
- The test script shows what commands CI will run

## Understanding the Issues

### Windows: vcpkg "No packages are installed" Error

**Problem:** When vcpkg has no packages installed, `vcpkg list` returns:
```
No packages are installed. Did you mean `search`?
```

This is written to stderr, causing PowerShell to treat it as an error.

**Solution:** The workflow now:
1. Captures both stdout and stderr with `2>&1`
2. Converts to string with `Out-String`
3. Checks that output contains package name AND doesn't contain error message

**Test it:**
```powershell
# Simulate the CI check
$output = & "C:\Program Files\vcpkg\vcpkg.exe" list boost-system:x64-windows 2>&1 | Out-String
$installed = ($output -match "boost-system" -and $output -notmatch "No packages are installed")
Write-Host "Should install: $(-not $installed)"
```

### macOS: Boost Library Not Found

**Problem:** CMake's `find_library` can't find `boost_system` with standard names.

**Possible Causes:**
1. Homebrew Boost 1.90 uses different library names
2. Library might be `.dylib` or `.a` format
3. Library might have `-mt` suffix or version numbers

**Solution:** The CMakeLists.txt now:
1. Lists all Boost libraries for diagnostics
2. Tries multiple naming patterns
3. Falls back to `file(GLOB)` to find any file with "boost_system" in name

**Test it:**
```bash
# Check what Boost libraries are actually installed
BOOST_ROOT=$(brew --prefix boost)
ls -la "$BOOST_ROOT/lib/" | grep boost

# Find boost_system specifically
find "$BOOST_ROOT/lib" -name "*boost_system*"
```

## Debugging Tips

### 1. Check Workflow Syntax
```bash
# Using GitHub CLI (if installed)
gh workflow view ci.yml

# Or validate YAML syntax
yamllint .github/workflows/ci.yml
```

### 2. Test Individual Commands

**Windows vcpkg:**
```powershell
$env:VCPKG_ROOT = "C:\Program Files\vcpkg"
& "$env:VCPKG_ROOT\vcpkg.exe" list boost-system:x64-windows
```

**macOS Boost:**
```bash
export BOOST_ROOT=$(brew --prefix boost)
ls -la "$BOOST_ROOT/lib/" | grep boost_system
cmake --version  # Check CMake version
```

### 3. Simulate CI Environment

**Windows:**
- Use GitHub Actions runner image: `windows-latest`
- Or use Docker: `docker run -it mcr.microsoft.com/windows/servercore:ltsc2022`

**macOS:**
- Use GitHub Actions runner image: `macos-latest`
- Or use Docker (if on macOS): `docker run -it --platform linux/amd64 ubuntu:22.04`

### 4. Check Logs

If you have access to GitHub Actions logs:
1. Go to: `https://github.com/YOUR_USERNAME/kraken-sdk/actions`
2. Click on the failed workflow run
3. Expand the failed job
4. Look for error messages

## Common Issues and Fixes

### Issue: "vcpkg.exe : No packages are installed"
**Fix:** The workflow now handles this gracefully. Test with `test_ci_windows.ps1`.

### Issue: "Could not find Boost system library"
**Fix:** The CMakeLists.txt now has fallback detection. Test with `test_ci_macos.sh`.

### Issue: "Policy CMP0144 is not known"
**Fix:** Policies are now conditional. Check `CMakeLists.txt` lines 16-20.

### Issue: "CMake Error: Could not find a package configuration file"
**Fix:** This usually means dependencies aren't installed. Check the install steps in CI workflow.

## Next Steps

1. **Run the test scripts** to identify issues locally
2. **Fix any problems** found by the scripts
3. **Test again** before pushing to GitHub
4. **Use act** for full workflow testing (optional but recommended)

## Resources

- [GitHub Actions Documentation](https://docs.github.com/en/actions)
- [act - Run GitHub Actions Locally](https://github.com/nektos/act)
- [vcpkg Documentation](https://vcpkg.io/)
- [CMake FindBoost Documentation](https://cmake.org/cmake/help/latest/module/FindBoost.html)

