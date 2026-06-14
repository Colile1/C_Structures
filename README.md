# C_Structures

## Real-Time 3D Structural Analysis Simulator

C_Structures is an interactive 3D structural analysis tool for engineering students and educators. Build a truss or frame, apply loads and supports, and see reactions, the deformed shape, and axial/shear/moment/torsion diagrams — with an optional "show the math" glass-box.

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
- Applied loads: point forces, point moments, distributed (span) loads (UDL / triangular / moment), and self-weight (ρ·A·g).
- Member-end releases (internal hinges) for pinned bases and three-hinged frames.
- Output: support reactions (force + moment) with an equilibrium badge, auto-scaled deformed shape, and annotated axial / shear / moment / torsion diagrams (peak value + location, end values, labelled sign convention).
- Determinacy check (frame-aware): warns before solving a mechanism; the solver surfaces status messages on screen instead of failing silently.
- Plain-language layer: tooltips on every control and results read as sentences beside the numbers; structural templates load as one-click cards.
- Visual icon palette with a symbol / realistic-2D / realistic-3D view toggle.
- Optional glass-box panel: shows the assembled stiffness matrix and the solve steps.
- Shareable output: PNG capture of the view and a one-page PDF report (model summary, reactions, member forces, diagrams).
- File formats: **CSV** interchange (nodes, beams, supports, nodal moments) and a richer **JSON** project format (also stores distributed loads and view preferences).
- **WebAssembly build** of the solver core (`wasm/`): the same analysis runs in a browser or under Node with no native install.

---

## What C_Structures Will NOT Solve

- Dynamic loads (earthquakes, wind time-histories), seismic analysis.
- Material nonlinearity or plasticity.
- A replacement for certified production software (SAP2000, AutoCAD Civil 3D, etc.).

---

## Architecture

```
┌─────────────────────────────────────────────┐
│                  main.cpp                    │  Orchestration only
├──────────────┬───────────────────────────────┤
│  UIHandler   │  Panel suite (ImGui)          │  Input & UI overlay
├──────────────┴───────────────────────────────┤
│   ForceRenderer · IconLibrary · Exporter     │  3D rendering, icons, PNG/PDF
├──────────────────────────────────────────────┤
│   Simulator / FrameSimulator / FrameElement  │  Physics (truss FEM · frame FEM)
│   MemberForces · DistributedLoad · Determinacy│  Diagrams · span loads · stability
├──────────────────────┬───────────────────────┤
│    Node / Beam       │  CSVHandler · JSONHandler│  Data model & I/O
└──────────────────────┴───────────────────────┘
```

Data flows top-down: UI panels call into the model; solvers read the model; the renderer reads both. No layer calls upward. The `physics/` core is pure and SDL/OpenGL-free, so it is unit-tested headlessly and compiled to WebAssembly (`wasm/`) unchanged.

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

### WebAssembly (solver core)

```bash
emcmake cmake -S wasm -B build-wasm -DCMAKE_BUILD_TYPE=Release
cmake --build build-wasm
node wasm/harness.mjs build-wasm/solver_core.js   # checks vs native results
```

Requires the [Emscripten SDK](https://emscripten.org/). See [wasm/README.md](wasm/README.md) for the JS API.

---

## Key Classes / APIs

### `Simulator` — truss solver (3 DOF/node)

```cpp
Simulator sim(nodes, beams);
SolveResult result = sim.solveStaticForces();   // SparseLU; returns status + message
auto disp = sim.getNodeDisplacements();         // std::vector<glm::vec3>
float force = sim.getBeamForce(beams[i]);        // positive = tension
glm::vec3 r = sim.getNodeReaction(nodeIdx);      // support reaction
```

### `FrameSimulator` — frame solver (6 DOF/node)

```cpp
FrameSimulator fsim(nodes, beams);
fsim.setDistributedLoads(loads);                 // UDL / triangular / moment
fsim.setSelfWeight(true);                        // optional ρ·A·g UDL per member
SolveResult result = fsim.solve();               // SparseLU + static condensation
auto t = fsim.getNodeTranslations();             // per-node translation (m)
auto rot = fsim.getNodeRotations();              // per-node rotation (rad)
std::array<float,12> p = fsim.getMemberEndForces(beams[i]);
```

### `Node`

```cpp
Node n(x, y, z);                 // floats
n.setJointType(JointType::PIN_XY);
n.applyForce(glm::vec3 f);
n.applyMoment(glm::vec3 m);       // acted on by the frame solver only
```

### `Beam`

```cpp
Beam b(startIdx, endIdx, E, A);   // legacy E/A ctor; or (startIdx,endIdx,material,A,I)
b.setMomentOfInertia(I);
b.setStartMomentRelease(true);    // internal hinge (frame)
// Indices into the node vector — never raw pointers.
```

### `CSVHandler` / `JSONHandler`

```cpp
CSVHandler::loadStructure("model.csv", nodes, beams);
CSVHandler::saveStructure("model.csv", nodes, beams);
JSONHandler::saveProject("project.json", nodes, beams, distLoads, prefs);
JSONHandler::loadProject("project.json", nodes, beams, distLoads, prefs);
```

CSV format (the trailing moment columns are optional; older files omit them):
```
NODE x y z joint [mx my mz]      # joint: 0=FREE 1=FIXED 2=PIN_XY 3=ROLLER_X 4=ROLLER_Y 5=ROLLER_Z
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
- **CI/CD:** GitHub Actions — headless CMake build + `ctest` on every push, plus an Emscripten job that builds the solver core to WebAssembly and runs the browser/Node harness.
- **Distribution:** Linux executable via CMake; Windows build via MinGW (see `build_windows.sh`); WebAssembly solver core for the browser (see `wasm/`).
- **Data:** Predefined structural templates (simple beam, simple truss, portal frame, cantilever) loaded as cards; CSV interchange and JSON project files; PNG / one-page PDF report export.
