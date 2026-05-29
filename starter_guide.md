# C_Structures — Starter Guide

## What This Project Does

C_Structures is a real-time 3D structural analysis simulator. You can build a truss-like structure by placing nodes and connecting them with beams, apply forces to free nodes, then run a static force solver (finite-element method, direct stiffness) to see displacements and beam stress coloured in real time.

**Blue beams** = tension. **Red beams** = compression. **Gray beams** = near-zero force.
**Red spheres** = fixed supports. **White spheres** = free nodes.

---

## Prerequisites

| Dependency | Version | Install |
|---|---|---|
| CMake | ≥ 3.12 | `winget install cmake` / apt |
| SDL2 | ≥ 2.0 | `vcpkg install sdl2` |
| GLEW | any | `vcpkg install glew` |
| Eigen3 | ≥ 3.4 | `vcpkg install eigen3` |
| GLM | ≥ 0.9.9 | `vcpkg install glm` |
| GTest | any | `vcpkg install gtest` |
| C++17 compiler | — | MSVC 2019+ / GCC 9+ / Clang 9+ |

---

## Building

```bash
# From the C_Structures/ project directory:
cmake -B build -DCMAKE_TOOLCHAIN_FILE=<path/to/vcpkg>/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Release
```

On Linux/Mac without vcpkg, ensure the libraries are system-installed and run:
```bash
cmake -B build && cmake --build build
```

---

## Running the Application

```bash
./build/C_Structures         # Linux/Mac
build\Release\C_Structures.exe  # Windows
```

A window opens with a test structure pre-loaded (two beams with a downward force applied).

---

## Running the Tests

```bash
cd build
ctest --verbose
# or directly:
./tests        # Linux/Mac
Release\tests.exe  # Windows
```

All tests should pass.

---

## Controls

| Action | Input |
|---|---|
| Orbit camera | Right-click + drag |
| Zoom | Scroll wheel |
| Switch to Node tool | Click **Nodes** button or press `N` |
| Switch to Beam tool | Click **Beams** button or press `B` |
| Switch to Force tool | Click **Forces** button or press `F` |
| Place a node | Node tool + left-click on 3D grid |
| Create a beam | Beam tool + left-click first node, then second |
| Apply force | Force tool + left-click a free node |
| Re-run solver | Press `Enter` |
| Cancel beam selection | Right-click or press `Escape` |
| Quit | Press `Escape` (when no beam selected) |

---

## Loading a Structure from CSV

Place a CSV file in the project directory with this format:

```
NODE 0.0 0.0 0.0 1
NODE 2.0 0.0 0.0 0
BEAM 0 1 200000000000 0.01
```

- `NODE x y z fixed` — fixed=1 pins the node, fixed=0 leaves it free
- `BEAM startIdx endIdx E A` — E in Pa, A in m²

CSV loading is currently triggered programmatically via `CSVHandler::loadStructure()`.
A UI button for load/save is on the roadmap (see `todo.md`).

---

## Project Structure

```
C_Structures/
├── include/
│   ├── graphics/       Shader.hpp, Camera.hpp
│   ├── model/          Node.hpp, Beam.hpp
│   ├── physics/        Simulator.hpp
│   ├── ui/             UIHandler.hpp
│   └── visualization/  ForceRenderer.hpp
├── src/
│   ├── graphics/       Camera.cpp
│   ├── model/          Node.cpp, Beam.cpp
│   ├── physics/        Simulator.cpp
│   ├── ui/             UIHandler.cpp
│   ├── visualization/  ForceRenderer.cpp, RendererUtils.cpp
│   └── main.cpp
├── shaders/            force_vertex.glsl, force_fragment.glsl
├── tests/              ModelTests.cpp, PhysicsTests.cpp, test_main.cpp
├── CMakeLists.txt
├── todo.md
├── log.md
└── starter_guide.md    (this file)
```

---

## Testing Individual Components

```cpp
// Test Node force accumulation
Node n(0,0,0);
n.applyForce({100,0,0});
assert(n.getAppliedForce().x == 100.0f);

// Test Simulator
Simulator sim(nodes, beams);
sim.solveStaticForces();
auto disp = sim.getNodeDisplacements(); // vector of glm::vec3

// Test CSV round-trip
CSVHandler::saveStructure("out.csv", nodes, beams);
CSVHandler::loadStructure("out.csv", nodes2, beams2);
```
