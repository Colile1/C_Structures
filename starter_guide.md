# C_Structures — Starter Guide

## What This Project Does

C_Structures is a real-time 3D structural analysis simulator. You can build a truss-like structure by placing nodes and connecting them with beams, apply forces to free nodes, then run a static force solver (finite-element method, direct stiffness) to see displacements and beam stress coloured in real time.

**Blue beams** = tension. **Red beams** = compression. **Gray beams** = near-zero force.
**Red spheres** = fixed supports. **White spheres** = free nodes.

---

## Prerequisites

### Windows (MSYS2 — recommended, tested)

1. Install [Chocolatey](https://chocolatey.org/install) if you don't have it, then:

```powershell
choco install msys2 -y
```

2. Open **MSYS2 MinGW64** shell (`C:\tools\msys64\msys2_shell.cmd -mingw64`) and run:

```bash
pacman -Sy --noconfirm
pacman -S --noconfirm --needed \
  mingw-w64-x86_64-gcc \
  mingw-w64-x86_64-cmake \
  mingw-w64-x86_64-ninja \
  mingw-w64-x86_64-SDL2 \
  mingw-w64-x86_64-eigen3 \
  mingw-w64-x86_64-glm \
  mingw-w64-x86_64-glew \
  mingw-w64-x86_64-gtest
```

### Linux (apt)

```bash
sudo apt install build-essential cmake ninja-build \
  libsdl2-dev libeigen3-dev libglm-dev libglew-dev libgtest-dev
```

### macOS (Homebrew)

```bash
brew install cmake ninja sdl2 eigen glm glew googletest
```

| Dependency | Version | Notes |
|---|---|---|
| CMake | ≥ 3.14 | Required by FetchContent |
| SDL2 | ≥ 2.0 | Windowing and input |
| GLEW | any | OpenGL extension loader |
| Eigen3 | ≥ 3.4 | Sparse matrix solver |
| GLM | ≥ 0.9.9 | Vector/matrix math |
| GTest | any | Unit test runner |
| C++17 compiler | — | GCC 9+ / Clang 9+ / MSVC 2019+ |

ImGui v1.91.5 is fetched automatically by CMake at configure time — no manual install needed.

---

## Building

### Windows (MSYS2 MinGW64 shell)

```bash
cd /c/Users/<you>/Documents/claude/Projects/C_Structures/C_Structures
mkdir build_win && cd build_win
cmake .. -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_COMPILER=g++ \
  -DCMAKE_PREFIX_PATH=/mingw64
ninja -j4
```

Or just run the helper script from the project root:

```bash
bash build_windows.sh
```

### Linux / macOS

```bash
mkdir build && cd build
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release
ninja -j$(nproc)
```

---

## Running the Application

**Windows — from the VS Code PowerShell terminal** (open the project in VS Code, then
open a new terminal with `` Ctrl+` `` and make sure it says **PowerShell** in the dropdown):

```powershell
.\build_win\C_Structures.exe
```

Or if the terminal is not already in the project folder:

```powershell
cd "C:\Users\Colile\Documents\claude\Projects\C_Structures\C_Structures"
.\build_win\C_Structures.exe
```

You can also double-click `build_win\C_Structures.exe` in Windows Explorer.

The required DLLs (`SDL2.dll`, `glew32.dll`, `libgcc_s_seh-1.dll`, `libstdc++-6.dll`,
`libwinpthread-1.dll`) are already copied into `build_win/` — no extra setup needed.

> **Do NOT run from the VS Code WSL/Git Bash terminal** — those cannot open native Win32
> OpenGL windows and the process will silently exit. Always use the **PowerShell** terminal.

**Linux/macOS:**

```bash
./build/C_Structures
```

A window opens with a test structure pre-loaded (two beams with a downward force applied).

---

## Running the Tests

```bash
# Windows (MSYS2 shell)
cd build_win && ctest --output-on-failure -V

# Linux/macOS
cd build && ctest --output-on-failure -V
```

Expected output: **100% tests passed, 0 tests failed out of 4** (22 individual tests across
ModelTests, PhysicsTests, CSVTests, IntegrationTests).

---

## Controls

| Action | Input |
|---|---|
| Orbit camera | Right-click + drag |
| Zoom | Scroll wheel |
| Select mode | Click **Select** button or press `1` |
| Node placement mode | Click **Node** button or press `N` |
| Beam creation mode | Click **Beam** button or press `B` |
| Force application mode | Click **Force** button or press `F` |
| Place a node | Node tool + left-click on 3D grid |
| Create a beam | Beam tool + left-click first node, then second node |
| Apply force | Force tool + left-click a free node |
| Drag a node | Select tool + left-click and drag |
| Delete selected | `Delete` key |
| Undo | `Ctrl+Z` |
| Redo | `Ctrl+Y` or `Ctrl+Shift+Z` |
| Re-run solver | Press `Enter` (or menu Simulation → Run Solver) |
| New structure | `Ctrl+N` |
| Cancel beam selection | Right-click or `Escape` |

---

## CSV File Format

Save and load structures with `CSVHandler`. The format is:

```
NODE x y z joint
BEAM startIdx endIdx E A
```

**joint** encodes the support type as an integer:

| Value | Type | Description |
|---|---|---|
| 0 | FREE | No constraint (internal node) |
| 1 | FIXED | All three translations fixed |
| 2 | PIN_XY | Ux = Uy = 0, Uz free |
| 3 | ROLLER_X | Ux = 0 only |
| 4 | ROLLER_Y | Uy = 0 only |
| 5 | ROLLER_Z | Uz = 0 only |

Example:

```
NODE 0.0 0.0 0.0 1
NODE 2.0 0.0 0.0 0
NODE 4.0 0.0 0.0 0
BEAM 0 1 200000000000 0.01
BEAM 1 2 200000000000 0.01
```

`BEAM` connectivity is stored as node **indices** (not positions), so it survives position
edits and CSV round-trips exactly.

---

## Project Structure

```
C_Structures/
├── include/
│   ├── data/           CSVHandler.hpp
│   ├── graphics/       Shader.hpp, Camera.hpp
│   ├── model/          Node.hpp, Beam.hpp
│   ├── physics/        Simulator.hpp
│   ├── ui/             UIHandler.hpp
│   └── visualization/  ForceRenderer.hpp
├── src/
│   ├── data/           CSVHandler.cpp
│   ├── graphics/       Camera.cpp
│   ├── model/          Node.cpp, Beam.cpp
│   ├── physics/        Simulator.cpp
│   ├── ui/             UIHandler.cpp
│   ├── visualization/  ForceRenderer.cpp, RendererUtils.cpp
│   └── main.cpp
├── resources/
│   ├── icons/          SVG icon library (symbols/, realistic/2d/, realistic/3d/)
│   └── IconsFontAwesome6.h
├── tests/
│   ├── ModelTests.cpp
│   ├── PhysicsTests.cpp
│   ├── CSVHandlerTests.cpp
│   ├── IntegrationTests.cpp
│   ├── py_logic_tests.py
│   └── test_main.cpp
├── build_win/          Windows build output (git-ignored)
├── build/              Linux build output (git-ignored)
├── CMakeLists.txt
├── build_windows.sh    One-shot Windows build helper
├── REVIEW.md           Full project audit
├── IMPROVEMENT_PLAN.md Five-phase improvement roadmap
├── log.md
└── starter_guide.md    (this file)
```

---

## Quick Code Examples

```cpp
// Force accumulation on a node
Node n(0, 0, 0);
n.applyForce({100, 0, 0});
assert(n.getAppliedForce().x == 100.0f);

// Run the solver
Simulator sim(nodes, beams);
sim.solveStaticForces();
auto disp = sim.getNodeDisplacements(); // vector of glm::vec3
float axialForce = sim.getBeamForce(beams[0]); // + = tension, - = compression

// CSV round-trip
CSVHandler::saveStructure("out.csv", nodes, beams);
CSVHandler::loadStructure("out.csv", nodes2, beams2);
```
