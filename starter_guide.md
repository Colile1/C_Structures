# C_Structures — Starter Guide

## What This Project Does

C_Structures is a real-time 3D structural analysis simulator. You place nodes, connect them with beams, apply loads and supports, then run a static finite-element (direct stiffness) solver to see displacements, reactions, and internal forces coloured in real time. Two solvers run side by side: a pin-jointed **truss** (3 DOF/node, axial only) and a rigid-jointed **frame** (6 DOF/node, shear + moment + torsion), switched with a UI toggle.

**Blue beams** = tension. **Red beams** = compression. **Gray beams** = near-zero force.
**Red spheres** = fixed supports. **White spheres** = free nodes.

In frame mode you also get annotated axial/shear/moment/torsion diagrams, a reactions table with an equilibrium badge, distributed and self-weight loads, and internal hinges. Models save to CSV (interchange) or JSON (full project), and export to PNG / a one-page PDF report. The pure solver core also builds to WebAssembly (see [wasm/README.md](wasm/README.md)).

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

### WebAssembly (solver core only — runs in a browser / Node)

Install and activate the [Emscripten SDK](https://emscripten.org/docs/getting_started/downloads.html)
so `emcc` / `emcmake` are on PATH (`source ./emsdk/emsdk_env.sh`, or `.\emsdk\emsdk_env.ps1`
in PowerShell), then from the project root:

```bash
emcmake cmake -S wasm -B build-wasm -DCMAKE_BUILD_TYPE=Release
cmake --build build-wasm
node wasm/harness.mjs build-wasm/solver_core.js   # asserts vs native/closed-form
```

This needs no SDL/OpenGL and no host Eigen/glm — the wasm project fetches them. See
[wasm/README.md](wasm/README.md) for the JavaScript API and the in-browser `harness.html`.

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

**Windows (VS Code PowerShell terminal):**

```powershell
cmake --build build_win --target tests
ctest --test-dir build_win --output-on-failure
```

**Linux/macOS:**

```bash
cmake --build build && ctest --test-dir build --output-on-failure
```

Expected output: **100% tests passed, 0 tests failed out of 11** suites — ModelTests,
PhysicsTests, CSVTests, IntegrationTests, DeterminacyTests, FrameTests, MemberForcesTests,
DistributedLoadTests, SolverStatusTests, ResolveCacheTests, JSONHandlerTests. The tests
build with `-DBUILD_APP=OFF` (no SDL/OpenGL needed); CI runs exactly this.

> **cmake/ctest not found?** Add MSYS2 to your PATH once in PowerShell:
> ```powershell
> [Environment]::SetEnvironmentVariable("PATH","C:\tools\msys64\mingw64\bin;$env:PATH","User")
> ```
> Then restart VS Code so the new PATH takes effect.

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
NODE x y z joint [mx my mz]
BEAM startIdx endIdx E A
```

The three trailing `NODE` columns are an optional concentrated nodal moment
(Mx, My, Mz, used by the frame solver); older files omit them and default to zero.
For loads, view preferences, and distributed loads as well, use the JSON project
format (`JSONHandler::saveProject` / `loadProject`).

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
├── include/            Public headers, mirroring src/ (data, graphics, model,
│                       physics, ui, visualization, export)
├── src/
│   ├── data/           CSVHandler.cpp, JSONHandler.cpp
│   ├── graphics/       Camera.cpp, IconLibrary.cpp
│   ├── model/          Node.cpp, Beam.cpp
│   ├── physics/        Simulator.cpp, FrameSimulator.cpp, FrameElement.cpp,
│   │                   MemberForces.cpp, DistributedLoad.cpp, Determinacy.cpp
│   ├── ui/             UIHandler.cpp + panels (Reactions, ModelCheck, Loads,
│   │                   Results, Diagram, GlassBox, Templates)
│   ├── visualization/  ForceRenderer.cpp
│   ├── export/         Exporter.cpp  (PNG capture + one-page PDF report)
│   ├── build_meta.cpp
│   └── main.cpp        Orchestration only
├── wasm/               WebAssembly build of the solver core (SolverBindings.cpp,
│                       CMakeLists.txt, harness.mjs, harness.html, README.md)
├── third_party/        nanosvg/  (vendored SVG rasteriser for the icon palette)
├── resources/
│   ├── icons/          SVG icon library (symbols/, realistic/2d/, realistic/3d/)
│   └── IconsFontAwesome6.h
├── tests/              11 GoogleTest suites (Model, Physics, CSV, JSON,
│                       Integration, Determinacy, Frame, MemberForces,
│                       DistributedLoad, SolverStatus, ResolveCache) + py_logic_tests.py
├── .github/workflows/  ci.yml  (headless ctest + Emscripten wasm harness)
├── build/, build_win/, build-wasm/   Build output (git-ignored)
├── CMakeLists.txt
├── build_windows.sh    One-shot Windows build helper
├── CLAUDE.md           Project contract / build order
├── IMPLEMENTATION_PLAN.md   Numbered build order (A1–A20)
├── GLITCHES_AND_FIX_PLAN.md, IMPROVEMENT_PLAN_2026-06.md, REVIEW.md
├── architecture.md, README.md
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
