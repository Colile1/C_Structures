# C_Structures

## Real-Time 3D Structural Analysis Simulator

C_Structures is an interactive 3D structural analysis tool for engineering students and educators. Build a truss or frame, apply loads and supports, and see reactions, the deformed shape, and axial/shear/moment diagrams — with an optional "show the math" glass-box.

**Author:** Colile Sibanda

---

## Technologies

### Core Stack

| Layer | Technology | Purpose |
|-------|------------|---------|
| UI / Rendering | SDL2, OpenGL, GLEW, Dear ImGui | Window, input, 3D scene, 2D panel UI |
| Physics | Eigen (SparseLU + static condensation) | Stiffness assembly and solve |
| Math | GLM | Matrix/vector transforms for rendering |
| Build | CMake | Cross-platform compilation |
| Tests | GoogleTest | Unit tests (headless — no SDL/GL needed) |
| VCS | Git / GitHub, GitHub Actions | Version control and CI |

### Alternatives Evaluated

**OpenGL vs. Vulkan** — OpenGL chosen for faster prototyping; Vulkan's lower-level control is not needed for a teaching tool at this scale.

**Eigen vs. Armadillo** — Eigen chosen for its template-optimized sparse solvers and the ability to run headless in unit tests without an extra runtime dependency.

---

## What C_Structures Does

- **Truss mode** (`Simulator`, 3 DOF/node): pin-jointed members, axial forces only. The simple teaching case.
- **Frame mode** (`FrameSimulator`, 6 DOF/node): rigid-jointed members, shear + moment + torsion. Toggled in the UI.
- Applied loads: point forces, point moments, and distributed (span) loads.
- Output: support reactions, deformed shape (auto-scaled), axial/shear/moment diagrams, equilibrium check.
- Optional glass-box panel: shows the assembled stiffness matrix and the solve steps.
- CSV import/export for node coordinates and beam definitions.

---

## What C_Structures Will NOT Solve

- Dynamic loads (earthquakes, wind time-histories), seismic analysis.
- Material nonlinearity or plasticity.
- A replacement for certified production software (SAP2000, AutoCAD Civil 3D, etc.).

---

## Architecture

```
┌─────────────────────────────────────────────┐
│                  main.cpp                   │  Orchestration only
├──────────────┬──────────────────────────────┤
│  UIHandler   │  Panel suite (ImGui)         │  Input & UI overlay
├──────────────┴──────────────────────────────┤
│        ForceRenderer  +  Geometry           │  3D rendering
├─────────────────────────────────────────────┤
│    Simulator / FrameSimulator               │  Physics (truss FEM · frame FEM)
├──────────────────────┬──────────────────────┤
│    Node / Beam       │    CSVHandler        │  Data model & I/O
└──────────────────────┴──────────────────────┘
```

Data flows top-down: UI panels call into the model; solvers read the model; the renderer reads both. No layer calls upward.

---

## Building

### Linux / WSL

```bash
cmake -S . -B build && cmake --build build -j
ctest --test-dir build --output-on-failure
./build/C_Structures          # run from a directory where shaders/ is present
```

### Windows

See [starter_guide.md](starter_guide.md) and [build_windows.sh](build_windows.sh).

---

## Key Classes / APIs

### `Simulator` — truss solver (3 DOF/node)

```cpp
Simulator sim(nodes, beams);
SolveResult result = sim.solve();          // SparseLU; returns status + displacements
float force = sim.getBeamForce(beamIdx);   // positive = tension
glm::vec3 disp = sim.getNodeDisplacement(nodeIdx);
```

### `FrameSimulator` — frame solver (6 DOF/node)

```cpp
FrameSimulator fsim(nodes, beams, distributedLoads);
SolveResult result = fsim.solve();         // SparseLU + static condensation
```

### `Node`

```cpp
Node n(glm::vec3 pos, JointType type);
n.applyForce(glm::vec3 f);
n.applyMoment(glm::vec3 m);
```

### `Beam`

```cpp
Beam b(int startIdx, int endIdx, double E, double A, double I);
// Indices into the node vector — never raw pointers.
```

### `CSVHandler`

```cpp
CSVHandler::loadStructure("model.csv", nodes, beams);
CSVHandler::saveStructure("model.csv", nodes, beams);
```

CSV format:
```
NODE x y z fixed
BEAM startIdx endIdx E A
```

---

## Data Model

```
Node ──< Beam >── Node
  position (glm::vec3)       startNode (int index)
  jointType                  endNode   (int index)
  appliedForce               E, A, I
  appliedMoment              material label
  isFixed
```

Beams reference nodes by **integer index**, never by pointer. The node vector can grow freely.

---

## Who Benefits

- **Civil engineering students** — learn shear, bending moment, and axial force interactively.
- **Educators** — demonstrate load redistribution and structural behavior in class.
- **Hobbyists** — experiment with basic truss and frame designs.

---

## Existing Solutions

| Tool | Similarities | Differences |
|------|-------------|-------------|
| ANSYS Mechanical | 3D structural analysis | Commercial, complex, expensive |
| Frame3DD | Static truss/beam analysis | CLI/text-file interface, no real-time interaction |
| PhET Simulations | Visualises physics concepts | K-12 level; no engineering-level FEM |

C_Structures bridges the gap between theoretical textbooks and professional tools with real-time 3D design and instant force feedback.

---

## Risks

| Risk | Impact | Mitigation |
|------|--------|-----------|
| Real-time rendering latency | Lag during interaction | Simplified mesh geometry; LOD when zoomed out |
| Numerical instability | Incorrect forces | Eigen's SparseLU with unit-test validation against textbook solutions |
| Solo development bottlenecks | Schedule delays | Strict step-by-step build order; one commit per build step |

---

## Infrastructure

- **Branching:** GitHub Flow — `main` for stable releases, feature branches for development.
- **CI/CD:** GitHub Actions — headless CMake build + `ctest` on every push.
- **Distribution:** Linux executable via CMake; Windows build via MinGW (see `build_windows.sh`).
- **Data:** Predefined structural templates (simple beam, simple truss, portal frame); CSV import/export.
