#!/usr/bin/env bash
# start.sh : builds C_Structures and launches the application.
# Usage:  ./start.sh [--clean] [--tests-only] [--no-run]
#   --clean       wipe the build directory before configuring
#   --tests-only  build and run tests only (no GUI)
#   --no-run      build only, do not launch the app

set -e

# ── Colour helpers ─────────────────────────────────────────────────────────────
RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33f'; CYAN='\033[0;36m'; NC='\033[0m'
ok()   { echo -e "${GREEN}[OK]${NC}    $1"; }
info() { echo -e "${CYAN}[..]${NC}    $1"; }
warn() { echo -e "${YELLOW}[!!]${NC}    $1"; }
fail() { echo -e "${RED}[XX]${NC}    $1"; exit 1; }

# ── Parse arguments ────────────────────────────────────────────────────────────
CLEAN=0; TESTS_ONLY=0; NO_RUN=0
for arg in "$@"; do
    case "$arg" in
        --clean)       CLEAN=1 ;;
        --tests-only)  TESTS_ONLY=1 ;;
        --no-run)      NO_RUN=1 ;;
        -h|--help)
            echo "Usage: ./start.sh [--clean] [--tests-only] [--no-run]"
            exit 0 ;;
        *) warn "Unknown argument: $arg" ;;
    esac
done

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"

echo ""
echo "══════════════════════════════════════════════"
echo "  C_Structures — Build & Launch"
echo "══════════════════════════════════════════════"
echo ""

# ── Check requirements ─────────────────────────────────────────────────────────
if [[ ! -f "$SCRIPT_DIR/requirements.sh" ]]; then
    fail "requirements.sh not found. Are you in the project directory?"
fi

info "Running dependency check ..."
bash "$SCRIPT_DIR/requirements.sh"

# ── Clean build directory ──────────────────────────────────────────────────────
if [[ $CLEAN -eq 1 ]]; then
    warn "Cleaning build directory ..."
    rm -rf "$BUILD_DIR"
    ok "Build directory cleaned"
fi

mkdir -p "$BUILD_DIR"

# ── Detect vcpkg toolchain (optional) ─────────────────────────────────────────
CMAKE_EXTRA=""
if [[ -n "$VCPKG_ROOT" ]]; then
    CMAKE_EXTRA="-DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
    info "Using vcpkg toolchain: $VCPKG_ROOT"
elif [[ -f "$HOME/vcpkg/scripts/buildsystems/vcpkg.cmake" ]]; then
    CMAKE_EXTRA="-DCMAKE_TOOLCHAIN_FILE=$HOME/vcpkg/scripts/buildsystems/vcpkg.cmake"
    info "Using vcpkg toolchain: $HOME/vcpkg"
fi

# ── Auto-clean stale CMake cache (different source path) ──────────────────────
CACHE_FILE="$BUILD_DIR/CMakeCache.txt"
if [[ -f "$CACHE_FILE" ]]; then
    CACHED_SRC=$(grep -m1 "^CMAKE_HOME_DIRECTORY:INTERNAL=" "$CACHE_FILE" | cut -d= -f2-)
    if [[ -n "$CACHED_SRC" && "$CACHED_SRC" != "$SCRIPT_DIR" ]]; then
        warn "Stale cache detected (was: $CACHED_SRC) — cleaning build directory ..."
        rm -rf "$BUILD_DIR"
        mkdir -p "$BUILD_DIR"
        ok "Build directory cleaned"
    fi
fi

# ── CMake configure ────────────────────────────────────────────────────────────
info "Configuring with CMake (may download ImGui on first run) ..."
cmake -S "$SCRIPT_DIR" -B "$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE=Release \
    $CMAKE_EXTRA \
    2>&1 | grep -v "^--" | grep -v "^$" || true

CMAKE_EXIT=${PIPESTATUS[0]}
if [[ $CMAKE_EXIT -ne 0 ]]; then
    fail "CMake configuration failed. Check that all dependencies are installed."
fi
ok "CMake configured"

# ── Build ──────────────────────────────────────────────────────────────────────
NPROC=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 2)
info "Building with $NPROC parallel jobs ..."
cmake --build "$BUILD_DIR" --config Release --parallel "$NPROC"
ok "Build succeeded"

# ── Copy shaders next to executable (needed at runtime) ───────────────────────
SHADER_SRC="$SCRIPT_DIR/shaders"
SHADER_DST="$BUILD_DIR/shaders"
if [[ -d "$SHADER_SRC" ]]; then
    mkdir -p "$SHADER_DST"
    cp "$SHADER_SRC"/*.glsl "$SHADER_DST/" 2>/dev/null || true
    ok "Shaders copied to $SHADER_DST"
fi

# ── Run tests ──────────────────────────────────────────────────────────────────
info "Running unit tests ..."
echo ""
cd "$BUILD_DIR"

if [[ -f "./tests" ]]; then
    TEST_BIN="./tests"
elif [[ -f "./Release/tests.exe" ]]; then
    TEST_BIN="./Release/tests.exe"
elif [[ -f "./tests.exe" ]]; then
    TEST_BIN="./tests.exe"
else
    warn "Test binary not found — skipping tests."
    TEST_BIN=""
fi

if [[ -n "$TEST_BIN" ]]; then
    if "$TEST_BIN" --gtest_color=yes; then
        echo ""
        ok "All tests passed"
    else
        echo ""
        fail "Tests failed — fix errors before launching the app."
    fi
fi

cd "$SCRIPT_DIR"

if [[ $TESTS_ONLY -eq 1 ]]; then
    echo ""
    echo "══════════════════════════════════════════════"
    echo -e "  ${GREEN}Tests complete. Exiting (--tests-only).${NC}"
    echo "══════════════════════════════════════════════"
    echo ""
    exit 0
fi

if [[ $NO_RUN -eq 1 ]]; then
    echo ""
    echo "══════════════════════════════════════════════"
    echo -e "  ${GREEN}Build complete. App not launched (--no-run).${NC}"
    echo -e "  Run manually: $BUILD_DIR/C_Structures"
    echo "══════════════════════════════════════════════"
    echo ""
    exit 0
fi

# ── Launch application ─────────────────────────────────────────────────────────
echo ""
echo "══════════════════════════════════════════════"
echo -e "  ${GREEN}Launching C_Structures ...${NC}"
echo ""
echo "  Controls:"
echo "   Right-drag   Orbit camera"
echo "   Scroll       Zoom"
echo "   N            Node placement mode"
echo "   B            Beam creation mode"
echo "   F            Force application mode"
echo "   Enter        Re-run solver"
echo "   Escape       Quit"
echo "══════════════════════════════════════════════"
echo ""

if [[ -f "$BUILD_DIR/C_Structures" ]]; then
    cd "$BUILD_DIR"
    exec ./C_Structures
elif [[ -f "$BUILD_DIR/Release/C_Structures.exe" ]]; then
    cd "$BUILD_DIR/Release"
    exec ./C_Structures.exe
else
    fail "Executable not found in $BUILD_DIR. Check build output above."
fi
