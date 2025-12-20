# Building on Windows

## Option 1: Using Visual Studio (Recommended)

### Prerequisites
1. **Visual Studio 2022** (Community, Professional, or Enterprise)
   - Install with "Desktop development with C++" workload
   - Includes MSVC compiler and CMake support

2. **vcpkg** (for dependencies)
   ```powershell
   # Install vcpkg
   git clone https://github.com/microsoft/vcpkg.git
   cd vcpkg
   .\bootstrap-vcpkg.bat
   
   # Install dependencies
   .\vcpkg install boost-beast:x64-windows openssl:x64-windows
   
   # Set environment variable (or add to system PATH)
   $env:VCPKG_ROOT = "C:\path\to\vcpkg"
   ```

### Build Steps

**Method A: Using Visual Studio**
1. Open Visual Studio 2022
2. File → Open → CMake...
3. Select `CMakeLists.txt` in the project root
4. Visual Studio will automatically configure and build

**Method B: Using Command Line**
```powershell
# Open "Developer Command Prompt for VS 2022" or run:
& "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"

# Configure
cd C:\Users\Sahil\source\repos\Kraken
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64 -DCMAKE_TOOLCHAIN_FILE=$env:VCPKG_ROOT\scripts\buildsystems\vcpkg.cmake

# Build
cmake --build . --config Release
```

## Option 2: Using MinGW-w64

### Prerequisites
1. **MSYS2** (includes MinGW-w64 and pacman package manager)
   - Download from: https://www.msys2.org/
   - Install and update: `pacman -Syu`

2. **Install dependencies in MSYS2 terminal:**
   ```bash
   pacman -S mingw-w64-x86_64-gcc
   pacman -S mingw-w64-x86_64-cmake
   pacman -S mingw-w64-x86_64-boost
   pacman -S mingw-w64-x86_64-openssl
   ```

3. **Build in MSYS2 terminal:**
   ```bash
   cd /c/Users/Sahil/source/repos/Kraken
   mkdir build && cd build
   cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
   cmake --build . -j4
   ```

## Option 3: Using WSL (Linux environment on Windows)

See `BUILDING.md` for Linux instructions. This is often the easiest option if you're comfortable with Linux.

## Quick Check: Verify CMakeLists.txt

To check if CMakeLists.txt syntax is correct (without building):
```powershell
cmake .. --dry-run  # If supported
# Or just try configure and see errors
cmake .. -G "Visual Studio 17 2022" -A x64
```

## Troubleshooting

### "Could not find Boost"
- Install Boost via vcpkg: `vcpkg install boost-beast:x64-windows`
- Or set `BOOST_ROOT` environment variable to Boost installation path

### "Could not find OpenSSL"
- Install OpenSSL via vcpkg: `vcpkg install openssl:x64-windows`
- Or set `OPENSSL_ROOT_DIR` environment variable

### "CMAKE_CXX_COMPILER not set"
- Open Visual Studio Developer Command Prompt
- Or manually set: `$env:CC = "cl.exe"; $env:CXX = "cl.exe"`

### "Generator Visual Studio 17 2022 could not find any instance"
- Make sure Visual Studio 2022 is installed
- Try: `cmake .. -G "Visual Studio 16 2019" -A x64` (for VS 2019)
- Or use: `cmake .. -G "NMake Makefiles"` and set up environment manually

## Recommended Setup for This Project

For the fastest setup:
1. **Install vcpkg** in a known location (e.g., `C:\vcpkg`)
2. **Install dependencies**: `vcpkg install boost-beast:x64-windows openssl:x64-windows`
3. **Use Visual Studio 2022** with CMake integration (easiest)
4. Or use **WSL** if you prefer Linux toolchain

