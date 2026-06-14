# C_Structures — Architecture

## Overview

C_Structures uses a layered architecture. Each layer has a single responsibility and communicates downward — the UI layer calls the model layer; the physics layer reads the model; the rendering layer reads both. No layer calls upward.

```
┌──────────────────────────────────────────────┐
│                  main.cpp                    │  Orchestration only
├──────────────┬────────────────────────────────┤
│  UIHandler   │  Panel suite (Dear ImGui)     │  Input & UI overlay
├──────────────┴────────────────────────────────┤
│  ForceRenderer · IconLibrary · Exporter       │  3D rendering, icons, PNG/PDF
├───────────────────────────────────────────────┤
│  Simulator / FrameSimulator / FrameElement    │  Physics (truss FEM · frame FEM)
│  MemberForces · DistributedLoad · Determinacy  │  diagrams · span loads · stability
├──────────────────────┬────────────────────────┤
│    Node / Beam       │  CSVHandler · JSONHandler│  Data model & I/O
└──────────────────────┴────────────────────────┘
```

The `physics/` core has no SDL/OpenGL/ImGui dependency, so it is unit-tested headlessly (`BUILD_APP=OFF`) and compiled unchanged to WebAssembly via Emscripten (see `wasm/`).

---

## Component Map

### Data Model (`include/model/`, `src/model/`)

| File | Responsibility |
|------|----------------|
| `Node.hpp` | A point in 3D space. Stores position, applied force, applied moment, joint type, and fixed-support flag. All methods inline. |
| `Beam.hpp` / `Beam.cpp` | A structural member between two nodes. Stores Young's modulus (E), cross-section area (A), second moment of area (I), and material label. Computes length and stiffness. |

Nodes are stored as `std::vector<Node>` and beams as `std::vector<Beam>` in `main.cpp`. **Beams reference nodes by integer index** (`startNode`, `endNode`) — never by raw pointer. The node vector may be reallocated freely.

---

### Physics (`include/physics/`, `src/physics/`)

| File | Responsibility |
|------|----------------|
| `Simulator.hpp/.cpp` | Pin-jointed truss solver (3 DOF/node). Assembles sparse K, extracts the free-DOF sub-matrix, solves K_FF·u = F with Eigen's `SparseLU`. `solveStaticForces()` returns `SolveResult`; the factorisation is cached and reused when only loads change. |
| `FrameSimulator.hpp/.cpp` | Rigid-jointed frame solver (6 DOF/node). Assembles from `FrameElement` blocks, adds consistent nodal loads from span/self-weight loads, applies member-end releases (internal hinges) and BCs by static condensation, solves with `SparseLU`. `solve()` returns `SolveResult`; factorisation cached as in the truss. |
| `FrameElement.hpp/.cpp` | 12-DOF element stiffness matrix, local-to-global transformation, and member-end release condensation for a single frame member. |
| `MemberForces.hpp/.cpp` | Internal-force diagram reconstruction: recovers axial, shear, moment, and torsion along each member from the local end forces and any span load, plus peak/end-value stats for annotation. |
| `DistributedLoad.hpp/.cpp` | Consistent equivalent nodal loads for UDL, triangular, and concentrated-moment span loads, plus per-member self-weight (ρ·A·g). Used by `FrameSimulator` during assembly. |
| `Determinacy.hpp/.cpp` | Kinematic classification: reports whether the structure is statically determinate, indeterminate, or a mechanism, for both truss (3 DOF/node) and frame (6 DOF/node) mode. |
| `SolveResult.hpp` | POD struct returned by the solvers: status enum (`OK`, `MECHANISM`, `FAILED`) and a user-facing message string. Displacements/reactions are read back through the solver's getters after the call. |

**Truss solving pipeline (`Simulator::solveStaticForces()`):**

```
populateForceVector()             reads node.getAppliedForce() → m_forces
assembleGlobalStiffnessMatrix()   builds sparse K from beam AE/L blocks (3 DOF/node)
free-DOF extraction               builds K_FF over unconstrained DOFs (static condensation)
SparseLU (cached)                 factorise once, solve u_F = K_FF⁻¹ · F_F
reactions r = K·u − F             cached for getNodeReaction() / checkEquilibrium()
```

**Frame solving pipeline (`FrameSimulator::solve()`):**

```
effectiveLoads()                  user span loads + (if enabled) self-weight UDLs
populateForces()                  nodal forces/moments + consistent equivalent nodal loads
assemble()                        sparse K from FrameElement 12×12 blocks (6 DOF/node),
                                  with member-end releases condensed out
free-DOF extraction               K_FF over unconstrained DOFs (static condensation)
SparseLU (cached)                 factorise once, solve u_F = K_FF⁻¹ · F_F
reactions r = K·u − F             support forces/moments for getNodeReactionForce/Moment()
```

**DOF layout:**
- Truss: 3 DOF/node — (x=3i, y=3i+1, z=3i+2)
- Frame: 6 DOF/node — (ux=6i, uy=6i+1, uz=6i+2, θx=6i+3, θy=6i+4, θz=6i+5)

---

### Rendering (`include/visualization/`, `include/graphics/`)

| File | Responsibility |
|------|----------------|
| `Shader.hpp` | Header-only. Compiles and links GLSL programs, exposes `setMat4`, `setVec3`, `setFloat` uniform setters. |
| `Camera.hpp/.cpp` | Orbit camera. Converts (yaw, pitch, radius) spherical coordinates to `glm::lookAt` view matrix. Right-drag = orbit, scroll = zoom; `focusOn()`/`resetToHome()` frame a loaded model. |
| `IconLibrary.hpp/.cpp` | Rasterises the `resources/icons` SVG set to cached OpenGL textures for the ImGui palette, with a symbol / realistic-2D / realistic-3D view toggle (vendored `third_party/nanosvg`). |
| `ForceRenderer.hpp/.cpp` | VAO-based arrow renderer for applied force vectors. Generates cylinder+cone meshes on init. `getBeamColor()` maps axial force to blue (tension) / red (compression) / grey (neutral). |
| `export/Exporter.hpp/.cpp` | Shareable output: PNG capture of the viewport and a one-page PDF report (model summary, reactions, member-force table, diagrams). |

**Geometry in `main.cpp`:** sphere (UV sphere, 8×12) and cylinder (12-segment) VAOs are built once at startup by `buildSphereVAO()` / `buildCylinderVAO()`. The cylinder model matrix is constructed per-beam to align the unit Y-axis to the beam direction.

**Shader files** (`shaders/`): loaded at runtime from the `shaders/` directory next to the executable. The `start.sh` copies them there during the build step.

---

### UI (`include/ui/`, `src/ui/`)

The entire UI is rendered with **Dear ImGui** (SDL2 + OpenGL backend). There is no hand-rolled OpenGL toolbar.

| File | Responsibility |
|------|----------------|
| `UIHandler.hpp/.cpp` | ImGui event-loop host. Initialises the SDL2+OpenGL ImGui backend, processes SDL events, and delegates rendering to panel classes. Handles node/beam creation, tool-mode switching, and the truss/frame and beginner/engineer toggles. |
| `LoadsPanel.hpp/.cpp` | ImGui panel: apply/edit point forces, point moments, distributed (UDL/triangular/moment) loads, and the self-weight toggle. |
| `ReactionsPanel.hpp/.cpp` | ImGui panel: tabular display of support reactions (force + moment per node) and ΣF equilibrium badge. |
| `ResultsPanel.hpp/.cpp` | ImGui panel: member-force summary; plain-language sentences in beginner mode, compact tables in engineer mode. |
| `DiagramPanel.hpp/.cpp` | ImGui panel: annotates the overlaid internal-force diagram — per-member end values, signed peak + location, units, and the sign convention for the selected quantity (N/Vy/Vz/T/My/Mz). |
| `ModelCheckPanel.hpp/.cpp` | ImGui panel: solver status messages, determinacy classification, and model-validation warnings. Routes solver errors to the user rather than silently failing. |
| `GlassBoxPanel.hpp/.cpp` | ImGui panel (optional): shows the assembled stiffness matrix and solve steps for the "show the math" teaching mode. |
| `Templates.hpp/.cpp` | One-click template cards (simple beam, simple truss, portal frame, cantilever). Loads a pre-built node/beam set into the model. |

---

### Data I/O (`include/data/`, `src/data/`)

| File | Responsibility |
|------|----------------|
| `CSVHandler.hpp/.cpp` | Static `loadStructure()` / `saveStructure()`. Format: `NODE x y z joint [mx my mz]` (optional nodal-moment columns) and `BEAM startIdx endIdx E A`. The lightweight interchange format. |
| `JSONHandler.hpp/.cpp` | Static `saveProject()` / `loadProject()`. Richer project format: nodes, beams, distributed loads, and view preferences (`ViewPrefs`). CSV remains supported for interchange. |

CSV/JSON files use 0-based integer indices, not pointers, so they are position-independent.

---

## Data Flow: Full Frame

```
SDL_PollEvent
    │
    ├─► Camera.handleMouseDrag / handleScroll    (orbit camera)
    │
    └─► UIHandler.handleEvent(e, nodes, beams, ...)
            │
            ├─► ImGui panel input → model updates (loads, supports, geometry)
            └─► tool-mode click → nodes.emplace_back / beams.emplace_back

Model change → Simulator(nodes, beams).solveStaticForces()   [truss]
            or FrameSimulator(nodes, beams);                 [frame]
               setDistributedLoads(loads); setSelfWeight(b); solve()
                     │
                     └─► SolveResult { status, message }   (u / reactions via getters)

Render:
    geoShader.use()
    drawGrid(...)
    for each beam → getBeamForce() → getBeamColor() → drawBeam(displaced pos, color)
    for each node → drawNode(displaced pos)
    forceRenderer.renderForceVectors(nodes, view, proj)
    ImGui::Render() / ImGui_ImplOpenGL3_RenderDrawData()
    SDL_GL_SwapWindow
```

---

## Adding a New Feature

### Add a new tool mode
1. Add an entry to `enum class ToolMode` in `UIHandler.hpp`
2. Add a case in `UIHandler::handleEvent` switch
3. Add an ImGui button in the relevant panel's `render()` call
4. Add keyboard shortcut in the `SDL_KEYDOWN` block in `main.cpp`

### Add a new material property to Beam
1. Add a private member and getter in `Beam.hpp`
2. Implement getter in `Beam.cpp`
3. Add to `CSVHandler::saveStructure` output
4. Add parsing in `CSVHandler::loadStructure`
5. Add a test in `ModelTests.cpp` and `CSVHandlerTests.cpp`

### Add a new physics quantity
1. Add a method to `Simulator.hpp` and/or `FrameSimulator.hpp`
2. Implement it using `m_displacements` and the assembled stiffness (SparseLU factorisation is reusable)
3. Call it from `main.cpp` render loop or via a results panel
4. Add a test in `PhysicsTests.cpp` or `FrameTests.cpp`

---

## Key Constraints

- **Solver re-use:** `Simulator` and `FrameSimulator` store references to `nodes` and `beams`. The simulators are kept alive across frames and cache their `SparseLU` factorisation, reusing it when only loads change (a stiffness signature detects geometry/connectivity/BC edits and invalidates the cache). If you swap the underlying vectors, construct a new solver.
- **Shaders at runtime:** GLSL files are loaded from disk at startup. The executable must be run from the `build/` directory (or wherever `shaders/` was copied), not from the project root.
- **OpenGL context required for ForceRenderer/UIHandler:** `initialize()` must be called after the OpenGL context is created. Do not construct these objects before `SDL_GL_CreateContext`.
- **Solver purity:** The physics layer (`Simulator`, `FrameSimulator`, `FrameElement`, `MemberForces`) has no dependency on SDL, OpenGL, or ImGui. All unit tests run headless.
