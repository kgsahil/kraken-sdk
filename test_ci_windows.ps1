# Test Windows CI/CD locally
# This script simulates the Windows CI environment

Write-Host "=== Testing Windows CI/CD Setup ===" -ForegroundColor Cyan
Write-Host ""

# Check if vcpkg is available
$vcpkgRoot = $env:VCPKG_ROOT
if (-not $vcpkgRoot) {
    Write-Host "❌ VCPKG_ROOT not set. Trying default location..." -ForegroundColor Yellow
    $vcpkgRoot = "C:\Program Files\vcpkg"
    if (-not (Test-Path "$vcpkgRoot\vcpkg.exe")) {
        Write-Host "❌ vcpkg not found at $vcpkgRoot" -ForegroundColor Red
        Write-Host "Please set VCPKG_ROOT or install vcpkg first" -ForegroundColor Yellow
        exit 1
    }
}

Write-Host "✅ vcpkg found at: $vcpkgRoot" -ForegroundColor Green
Write-Host ""

# Test the vcpkg list command
Write-Host "=== Testing vcpkg list command ===" -ForegroundColor Cyan
Write-Host "Command: vcpkg.exe list boost-system:x64-windows" -ForegroundColor Gray

$boostOutput = & "$vcpkgRoot\vcpkg.exe" list boost-system:x64-windows 2>&1 | Out-String
Write-Host "Raw output:" -ForegroundColor Yellow
Write-Host $boostOutput
Write-Host ""

# Check if it contains the package
$boostInstalled = ($boostOutput -match "boost-system" -and $boostOutput -notmatch "No packages are installed")
Write-Host "Contains 'boost-system': $($boostOutput -match 'boost-system')" -ForegroundColor $(if ($boostOutput -match 'boost-system') { 'Green' } else { 'Red' })
Write-Host "Contains 'No packages are installed': $($boostOutput -match 'No packages are installed')" -ForegroundColor $(if ($boostOutput -match 'No packages are installed') { 'Yellow' } else { 'Green' })
Write-Host "Should install: $(-not $boostInstalled)" -ForegroundColor $(if (-not $boostInstalled) { 'Yellow' } else { 'Green' })
Write-Host ""

# Test OpenSSL
Write-Host "=== Testing OpenSSL check ===" -ForegroundColor Cyan
$opensslOutput = & "$vcpkgRoot\vcpkg.exe" list openssl:x64-windows 2>&1 | Out-String
Write-Host "Raw output:" -ForegroundColor Yellow
Write-Host $opensslOutput
Write-Host ""

$opensslInstalled = ($opensslOutput -match "openssl" -and $opensslOutput -notmatch "No packages are installed")
Write-Host "Contains 'openssl': $($opensslOutput -match 'openssl')" -ForegroundColor $(if ($opensslOutput -match 'openssl') { 'Green' } else { 'Red' })
Write-Host "Contains 'No packages are installed': $($opensslOutput -match 'No packages are installed')" -ForegroundColor $(if ($opensslOutput -match 'No packages are installed') { 'Yellow' } else { 'Green' })
Write-Host "Should install: $(-not $opensslInstalled)" -ForegroundColor $(if (-not $opensslInstalled) { 'Yellow' } else { 'Green' })
Write-Host ""

# Summary
Write-Host "=== Summary ===" -ForegroundColor Cyan
if ($boostInstalled -and $opensslInstalled) {
    Write-Host "✅ Both packages are installed - would skip installation" -ForegroundColor Green
} else {
    Write-Host "⚠️  Packages need to be installed" -ForegroundColor Yellow
    if (-not $boostInstalled) {
        Write-Host "   - boost-system:x64-windows" -ForegroundColor Yellow
    }
    if (-not $opensslInstalled) {
        Write-Host "   - openssl:x64-windows" -ForegroundColor Yellow
    }
}

