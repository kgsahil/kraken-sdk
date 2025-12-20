# Building Kraken SDK

## Prerequisites

### Linux (Ubuntu/Debian) - Recommended

```bash
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    libssl-dev \
    libboost-system-dev
```

**Minimum Versions:**
- GCC 9+ or Clang 10+ (C++17 support)
- CMake 3.16+
- Boost >= 1.70 (system component)
- OpenSSL >= 1.1.1

### WSL (Windows Subsystem for Linux)

```bash
# Use the automated setup script
bash setup_wsl.sh
```

Or manually:
```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake libssl-dev libboost-system-dev
```

### Windows (vcpkg)

```powershell
# Install vcpkg if you haven't
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat

# Install dependencies
.\vcpkg install boost-system:x64-windows openssl:x64-windows

# Configure with vcpkg toolchain
cmake .. -DCMAKE_TOOLCHAIN_FILE=path/to/vcpkg/scripts/buildsystems/vcpkg.cmake
```

### macOS (Homebrew)

```bash
brew install boost openssl cmake
cmake .. -DOPENSSL_ROOT_DIR=$(brew --prefix openssl)
```

---

## Building

### Quick Build

```bash
git clone https://github.com/kgsahil/kraken-sdk.git
cd kraken-sdk
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

### Build Options

| Option | Default | Description |
|--------|---------|-------------|
| `KRAKEN_BUILD_EXAMPLES` | ON | Build example programs |
| `KRAKEN_BUILD_TESTS` | ON | Build test suite |
| `KRAKEN_BUILD_TOOLS` | ON | Build benchmark tool |
| `CMAKE_BUILD_TYPE` | Debug | Release/Debug/RelWithDebInfo |

Example:
```bash
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DKRAKEN_BUILD_TESTS=OFF
```

---

## Running Examples

After building, examples are in the `build/` directory:

```bash
cd build

# Quickstart (5-line demo)
./quickstart

# Strategy demo (alerts)
./strategies

# Order book with checksum
./orderbook

# Live performance dashboard
./dashboard

# Performance benchmark (30 seconds)
./benchmark 30
```

---

## Running Tests

```bash
cd build
ctest --output-on-failure
```

Or run individual tests:
```bash
./test_strategies
./test_book_checksum
```

---

## Installation

Install to system (optional):

```bash
cd build
sudo cmake --install . --prefix /usr/local
```

Then use in your CMake project:
```cmake
find_package(KrakenSDK REQUIRED)
target_link_libraries(your_app PRIVATE kraken::kraken)
```

---

## Troubleshooting

### Boost Version Issues

**Error:** `Could NOT find Boost (missing: Boost_INCLUDE_DIR system)`

**Solution:** Ensure Boost 1.70+ is installed:
```bash
# Ubuntu/Debian
sudo apt-get install libboost-system-dev

# Check version
dpkg -l | grep libboost
```

The SDK requires Boost 1.70+ but works best with 1.81+.

### OpenSSL Not Found

**macOS:**
```bash
cmake .. -DOPENSSL_ROOT_DIR=$(brew --prefix openssl)
```

**Linux:** Ensure `libssl-dev` is installed:
```bash
sudo apt-get install libssl-dev
```

### Compiler Not Found

Ensure you have a C++17-compatible compiler:
```bash
# Check GCC version
gcc --version  # Should be 9+

# Check Clang version
clang++ --version  # Should be 10+
```

### CMake Version Too Old

```bash
# Ubuntu 20.04/22.04
sudo apt-get install cmake

# Or build from source
wget https://github.com/Kitware/CMake/releases/download/v3.24.0/cmake-3.24.0.tar.gz
tar -xzf cmake-3.24.0.tar.gz
cd cmake-3.24.0
./bootstrap && make && sudo make install
```

---

## Dependencies

The SDK uses **FetchContent** to automatically download:
- [rigtorp/SPSCQueue](https://github.com/rigtorp/SPSCQueue) v1.1 - Lock-free queue
- [RapidJSON](https://github.com/Tencent/rapidjson) v1.1.0 - JSON parsing
- [GoogleTest](https://github.com/google/googletest) - Testing framework

These are downloaded automatically during CMake configuration. No manual installation needed.

---

## Development Build

For development with debug symbols:

```bash
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j$(nproc)
```

Enable sanitizers (optional):
```bash
cmake .. \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_CXX_FLAGS="-fsanitize=address -fsanitize=undefined"
```

---

## Cross-Compilation

For cross-compilation, set the toolchain:

```bash
cmake .. -DCMAKE_TOOLCHAIN_FILE=path/to/toolchain.cmake
```

---

## See Also

- [BUILDING_WINDOWS.md](BUILDING_WINDOWS.md) - Detailed Windows instructions
- [README.md](README.md) - Project overview and API documentation
