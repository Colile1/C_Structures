#!/usr/bin/env bash
# requirements.sh : checks required dependencies and installs missing ones.
# Supports: Ubuntu/Debian (apt), Fedora/RHEL (dnf), Arch (pacman), macOS (brew).
# On Windows Git Bash without WSL, prints vcpkg instructions.

set -e

# ── Colour helpers ─────────────────────────────────────────────────────────────
RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'; NC='\033[0m'
ok()   { echo -e "${GREEN}[OK]${NC}    $1"; }
warn() { echo -e "${YELLOW}[SKIP]${NC}  $1"; }
info() { echo -e "        $1"; }
fail() { echo -e "${RED}[FAIL]${NC}  $1"; exit 1; }

echo ""
echo "══════════════════════════════════════════════"
echo "  C_Structures — Dependency Checker"
echo "══════════════════════════════════════════════"
echo ""

# ── Detect platform ────────────────────────────────────────────────────────────
detect_os() {
    if [[ "$OSTYPE" == "darwin"* ]]; then
        echo "macos"
    elif grep -qi microsoft /proc/version 2>/dev/null; then
        echo "wsl"
    elif [[ -f /etc/debian_version ]]; then
        echo "debian"
    elif [[ -f /etc/fedora-release ]] || [[ -f /etc/redhat-release ]]; then
        echo "fedora"
    elif [[ -f /etc/arch-release ]]; then
        echo "arch"
    elif [[ "$OSTYPE" == "msys" ]] || [[ "$OSTYPE" == "cygwin" ]]; then
        echo "windows_bash"
    else
        echo "unknown"
    fi
}

OS=$(detect_os)
info "Detected platform: ${OS}"
echo ""

# ── Windows Git Bash (no package manager available) ───────────────────────────
if [[ "$OS" == "windows_bash" ]]; then
    echo -e "${YELLOW}Running in Windows Git Bash. Cannot auto-install packages.${NC}"
    echo ""
    echo "Please install dependencies via vcpkg:"
    echo "  git clone https://github.com/microsoft/vcpkg"
    echo "  cd vcpkg && bootstrap-vcpkg.bat"
    echo "  vcpkg install sdl2 glew eigen3 glm gtest"
    echo ""
    echo "Then build with:"
    echo "  cmake -B build -DCMAKE_TOOLCHAIN_FILE=<vcpkg_root>/scripts/buildsystems/vcpkg.cmake"
    echo "  cmake --build build --config Release"
    exit 0
fi

# ── Package manager setup ─────────────────────────────────────────────────────
install_pkg() {
    local pkg="$1"
    case "$OS" in
        macos)
            brew install "$pkg" ;;
        debian|wsl)
            sudo apt-get install -y "$pkg" ;;
        fedora)
            sudo dnf install -y "$pkg" ;;
        arch)
            sudo pacman -S --noconfirm "$pkg" ;;
        *)
            fail "Unknown OS — cannot install $pkg automatically." ;;
    esac
}

update_pkg_cache() {
    case "$OS" in
        debian|wsl)  sudo apt-get update -qq ;;
        fedora)      sudo dnf check-update -q || true ;;
        macos)       brew update -q ;;
        arch)        sudo pacman -Sy --noconfirm ;;
    esac
}

# ── Check a command exists, install if not ────────────────────────────────────
check_cmd() {
    local cmd="$1"; local pkg="$2"; local label="${3:-$pkg}"
    if command -v "$cmd" &>/dev/null; then
        ok "$label ($(command -v "$cmd"))"
    else
        warn "$label not found — installing $pkg ..."
        install_pkg "$pkg"
        ok "$label installed"
    fi
}

# ── Check a pkg-config library, install if not ────────────────────────────────
check_lib() {
    local pc="$1"; local pkg="$2"; local label="${3:-$pkg}"
    if pkg-config --exists "$pc" 2>/dev/null; then
        ok "$label ($(pkg-config --modversion "$pc" 2>/dev/null || echo 'found'))"
    else
        warn "$label not found — installing $pkg ..."
        install_pkg "$pkg"
        ok "$label installed"
    fi
}

# ── Check a header exists without pkg-config ──────────────────────────────────
check_header() {
    local header="$1"; local pkg="$2"; local label="${3:-$pkg}"
    if echo "#include <$header>" | ${CXX:-g++} -x c++ -fsyntax-only - 2>/dev/null; then
        ok "$label (header found)"
    else
        warn "$label not found — installing $pkg ..."
        install_pkg "$pkg"
        ok "$label installed"
    fi
}

# ── macOS: ensure Homebrew is present ─────────────────────────────────────────
if [[ "$OS" == "macos" ]]; then
    if ! command -v brew &>/dev/null; then
        warn "Homebrew not found — installing ..."
        /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
        ok "Homebrew installed"
    else
        ok "Homebrew ($(brew --version | head -1))"
    fi
fi

echo "── Updating package cache ──────────────────────"
update_pkg_cache
echo ""

echo "── Build tools ─────────────────────────────────"
check_cmd cmake cmake "CMake"
check_cmd make make "Make"

# Compiler
if command -v g++ &>/dev/null; then
    ok "g++ ($(g++ --version | head -1))"
elif command -v clang++ &>/dev/null; then
    ok "clang++ ($(clang++ --version | head -1))"
else
    warn "No C++ compiler found — installing g++ ..."
    case "$OS" in
        debian|wsl) install_pkg "build-essential" ;;
        fedora)     install_pkg "gcc-c++" ;;
        arch)       install_pkg "gcc" ;;
        macos)      install_pkg "llvm" ;;
    esac
    ok "C++ compiler installed"
fi
echo ""

echo "── Libraries ───────────────────────────────────"

case "$OS" in
    debian|wsl)
        check_lib  "sdl2"    "libsdl2-dev"     "SDL2"
        check_lib  "glew"    "libglew-dev"      "GLEW"
        check_header "Eigen/Sparse" "libeigen3-dev" "Eigen3"
        check_header "glm/glm.hpp"  "libglm-dev"    "GLM"
        # GTest
        if ! dpkg -l libgtest-dev &>/dev/null 2>&1; then
            warn "GTest not found — installing ..."
            sudo apt-get install -y libgtest-dev cmake
            # On older Ubuntu, GTest needs to be compiled manually
            if [[ ! -f /usr/lib/libgtest.a ]] && [[ ! -f /usr/lib/x86_64-linux-gnu/libgtest.a ]]; then
                info "Compiling GTest from source ..."
                pushd /usr/src/gtest > /dev/null
                sudo cmake . -DCMAKE_BUILD_TYPE=Release
                sudo make
                sudo cp lib/libgtest*.a /usr/lib/ 2>/dev/null || \
                sudo cp *.a /usr/lib/ 2>/dev/null || true
                popd > /dev/null
            fi
            ok "GTest installed"
        else
            ok "GTest (installed)"
        fi
        ;;
    fedora)
        check_lib  "sdl2"    "SDL2-devel"       "SDL2"
        check_lib  "glew"    "glew-devel"        "GLEW"
        check_header "Eigen/Sparse" "eigen3-devel"  "Eigen3"
        check_header "glm/glm.hpp"  "glm-devel"     "GLM"
        check_cmd  pkg-config "pkgconf"
        install_pkg "gtest-devel" || true
        ok "GTest"
        ;;
    arch)
        check_lib  "sdl2"    "sdl2"             "SDL2"
        check_lib  "glew"    "glew"              "GLEW"
        check_header "Eigen/Sparse" "eigen"     "Eigen3"
        check_header "glm/glm.hpp"  "glm"       "GLM"
        check_cmd  gtest "gtest"                "GTest"
        ;;
    macos)
        check_lib  "sdl2"    "sdl2"             "SDL2"
        check_lib  "glew"    "glew"              "GLEW"
        check_header "Eigen/Sparse" "eigen"     "Eigen3"
        check_header "glm/glm.hpp"  "glm"       "GLM"
        check_cmd  brew install googletest      "GTest" || true
        ;;
esac
echo ""

echo "── OpenGL ──────────────────────────────────────"
if [[ "$OS" == "macos" ]]; then
    ok "OpenGL (built-in on macOS)"
else
    check_header "GL/glew.h" "" "OpenGL headers"
    if ! dpkg -l libgl1-mesa-dev &>/dev/null 2>&1 && [[ "$OS" == "debian" || "$OS" == "wsl" ]]; then
        warn "Mesa OpenGL dev not found — installing ..."
        sudo apt-get install -y libgl1-mesa-dev libglu1-mesa-dev
        ok "Mesa OpenGL installed"
    else
        ok "OpenGL (mesa)"
    fi
fi
echo ""

echo "══════════════════════════════════════════════"
echo -e "  ${GREEN}All dependencies satisfied.${NC}"
echo "  Run ./start.sh to build and launch."
echo "══════════════════════════════════════════════"
echo ""
