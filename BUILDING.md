# Building Kraken SDK

## Prerequisites

### Windows (vcpkg)
```powershell
# Install vcpkg if you haven't
git clone https://github.com/microsoft/vcpkg.git
.\vcpkg\bootstrap-vcpkg.bat

# Install dependencies
.\vcpkg\vcpkg install boost-beast:x64-windows openssl:x64-windows
```

### Linux (Ubuntu/Debian)
```bash
sudo apt-get update
sudo apt-get install -y libboost-all-dev libssl-dev cmake build-essential
```

### macOS (Homebrew)
```bash
brew install boost openssl cmake
```

## Building

### Configure
```bash
mkdir build
cd build

# Windows with vcpkg
cmake .. -DCMAKE_TOOLCHAIN_FILE=path/to/vcpkg/scripts/buildsystems/vcpkg.cmake

# Linux/macOS
cmake ..
```

### Build
```bash
cmake --build . --config Release
```

### Run Examples
```bash
# Quickstart
./quickstart

# Strategy demo
./strategies

# Order book
./orderbook

# Live dashboard
./dashboard

# Benchmark (30 seconds)
./benchmark 30
```

### Run Tests
```bash
ctest --output-on-failure
```

## Build Options

| Option | Default | Description |
|--------|---------|-------------|
| `KRAKEN_BUILD_EXAMPLES` | ON | Build example programs |
| `KRAKEN_BUILD_TESTS` | ON | Build test suite |
| `KRAKEN_BUILD_TOOLS` | ON | Build benchmark tool |

Example:
```bash
cmake .. -DKRAKEN_BUILD_TESTS=OFF
```

## Troubleshooting

### OpenSSL not found (macOS)
```bash
cmake .. -DOPENSSL_ROOT_DIR=$(brew --prefix openssl)
```

### Boost not found
Make sure Boost is installed with the `system` component:
```bash
# Ubuntu
sudo apt-get install libboost-system-dev

# vcpkg
vcpkg install boost-beast
```

