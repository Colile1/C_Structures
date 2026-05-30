#!/usr/bin/env bash
# build_windows.sh — configure, build, and test C_Structures with MSYS2/MinGW64.
# Run from the MSYS2 MinGW64 shell:  bash build_windows.sh
# Or from PowerShell:  C:\tools\msys64\mingw64\bin\bash.exe -l build_windows.sh

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build_win"

export PATH="/mingw64/bin:$PATH"

echo "=== C_Structures Windows Build ==="
echo "Source: $SCRIPT_DIR"
echo "Build:  $BUILD_DIR"
echo "Compiler: $(g++ --version | head -1)"
echo "CMake:    $(cmake --version | head -1)"
echo ""

# Fresh configure (delete old Linux build artefacts)
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

cmake .. \
  -G "Ninja" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_COMPILER=gcc \
  -DCMAKE_CXX_COMPILER=g++ \
  -DCMAKE_PREFIX_PATH="/mingw64" \
  -DCMAKE_MAKE_PROGRAM=ninja

echo ""
echo "=== Building ==="
ninja -j$(nproc)

echo ""
echo "=== Running Tests ==="
ctest --output-on-failure -V

echo ""
echo "=== Done ==="
echo "Executable: $BUILD_DIR/C_Structures.exe"
