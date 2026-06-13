# C_Structures — Architecture

## Overview

C_Structures uses a layered architecture. Each layer has a single responsibility and communicates downward — the UI layer calls the model layer; the physics layer reads the model; the rendering layer reads both. No layer calls upward.

```
┌─────────────────────────────────────────────┐
│                  main.cpp                   │  Orchestration only
├──────────────┬──────────────────────────────┤
│  UIHandler   │  Panel suite (Dear ImGui)    │  Input & UI overlay
├──────────────┴──────────────────────────────┤
│        ForceRenderer  +  Geometry           │  3D rendering
├─────────────────────────────────────────────┤
│    Simulator / FrameSimulator               │  Physics (truss FEM · frame FEM)
├──────────────────────┬──────────────────────┤
│    Node / Beam       │    CSVHandler        │  Data model & I/O
└──────────────────────┴──────────────────────┘
```

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
| `Simulator.hpp/.cpp` | Pin-jointed truss solver (3 DOF/node). Assembles sparse K, applies BCs, solves K·u = F with Eigen's `SparseLU`. Returns `SolveResult`. |
| `FrameSimulator.hpp/.cpp` | Rigid-jointed frame solver (6 DOF/node). Assembles from `FrameElement` blocks, applies distributed loads via static condensation, solves with `SparseLU`. Returns `SolveResult`. |
| `FrameElement.hpp/.cpp` | 12-DOF element stiffness matrix and local-to-global transformation for a single frame member. |
| `MemberForces.hpp/.cpp` | Internal-force diagram reconstruction: recovers axial, shear, moment, and torsion along each member from the nodal displacement solution and any span loads. |
| `DistributedLoad.hpp/.cpp` | Consistent equivalent nodal loads for uniform and linearly varying distributed loads. Used by `FrameSimulator` during assembly. |
| `Determinacy.hpp/.cpp` | Kinematic classification: reports whether the structure is statically determinate, indeterminate, or a mechanism, for both truss (3 DOF/node) and frame (6 DOF/node) mode. |
| `SolveResult.hpp` | POD struct returned by `solve()`: status enum (`OK`, `MECHANISM`, `ILL_CONDITIONED`), user-facing message string, displacement vector, and reaction vector. |

**Truss solving pipeline (`Simulator::solve()`):**

```
populateForceVector()             reads node.getAppliedForce() → m_forces
assembleGlobalStiffnessMatrix()   builds sparse K from beam AE/L blocks (3 DOF/node)
applySupportConstraints()         zeros fixed-DOF rows/cols, sets diagonal = 1
SparseLU.solve()                  computes m_displacements = K⁻¹ · m_forces
```

**Frame solving pipeline (`FrameSimulator::solve()`):**

```
assembleDistributedLoads()        converts span loads to equivalent nodal loads
assembleGlobalStiffnessMatrix()   builds sparse K from FrameElement 12×12 blocks (6 DOF/node)
applySupportConstraints()         zeros fixed-DOF rows/cols (pinned or fixed support)
SparseLU.solve()                  computes m_displacements = K⁻¹ · m_forces
recoverReactions()                back-substitutes to get support forces/moments
```

**DOF layout:**
- Truss: 3 DOF/node — (x=3i, y=3i+1, z=3i+2)
- Frame: 6 DOF/node — (ux=6i, uy=6i+1, uz=6i+2, θx=6i+3, θy=6i+4, θz=6i+5)

---

### Rendering (`include/visualization/`, `include/graphics/`)

| File | Responsibility |
|------|----------------|
| `Shader.hpp` | Header-only. Compiles and links GLSL programs, exposes `setMat4`, `setVec3`, `setFloat` uniform setters. |
| `Camera.hpp/.cpp` | Orbit camera. Converts (yaw, pitch, radius) spherical coordinates to `glm::lookAt` view matrix. Right-drag = orbit, scroll = zoom. |
| `ForceRenderer.hpp/.cpp` | VAO-based arrow renderer for applied force vectors. Generates cylinder+cone meshes on init. `getBeamColor()` maps axial force to blue (tension) / red (compression) / grey (neutral). |

**Geometry in `main.cpp`:** sphere (UV sphere, 8×12) and cylinder (12-segment) VAOs are built once at startup by `buildSphereVAO()` / `buildCylinderVAO()`. The cylinder model matrix is constructed per-beam to align the unit Y-axis to the beam direction.

**Shader files** (`shaders/`): loaded at runtime from the `shaders/` directory next to the executable. The `start.sh` copies them there during the build step.

---

### UI (`include/ui/`, `src/ui/`)

The entire UI is rendered with **Dear ImGui** (SDL2 + OpenGL backend). There is no hand-rolled OpenGL toolbar.

| File | Responsibility |
|------|----------------|
| `UIHandler.hpp/.cpp` | ImGui event-loop host. Initialises the SDL2+OpenGL ImGui backend, processes SDL events, and delegates rendering to panel classes. Handles node/beam creation and tool-mode switching. |
| `LoadsPanel.hpp/.cpp` | ImGui panel: apply/edit point forces, point moments, and distributed loads on selected members. |
| `ReactionsPanel.hpp/.cpp` | ImGui panel: tabular display of support reactions (force + moment per node) and ΣF equilibrium badge. |
| `ResultsPanel.hpp/.cpp` | ImGui panel: member-force summary table (axial, shear, moment per beam). |
| `ModelCheckPanel.hpp/.cpp` | ImGui panel: solver status messages, determinacy classification, and model-validation warnings. Routes solver errors to the user rather than silently failing. |
| `GlassBoxPanel.hpp/.cpp` | ImGui panel (optional): shows the assembled stiffness matrix and solve steps for the "show the math" teaching mode. |
| `Templates.hpp/.cpp` | One-click structural templates (simple beam, simple truss, portal frame). Loads a pre-built node/beam set into the model. |

---

### Data I/O (`include/data/`, `src/data/`)

| File | Responsibility |
|------|----------------|
| `CSVHandler.hpp/.cpp` | Static `loadStructure()` and `saveStructure()`. Format: `NODE x y z fixed` and `BEAM startIdx endIdx E A`. |

CSV files use 0-based integer indices, not pointers, so they are position-independent.

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

Model change → Simulator(nodes, beams).solve()        [truss]
            or FrameSimulator(nodes, beams, loads).solve()  [frame]
                     │
                     └─► SolveResult { status, displacements, reactions }

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

- **Solver re-use:** `Simulator` and `FrameSimulator` store references to `nodes` and `beams`. If you add new nodes/beams after construction, create a new solver instance (`main.cpp` does this on every model change).
- **Shaders at runtime:** GLSL files are loaded from disk at startup. The executable must be run from the `build/` directory (or wherever `shaders/` was copied), not from the project root.
- **OpenGL context required for ForceRenderer/UIHandler:** `initialize()` must be called after the OpenGL context is created. Do not construct these objects before `SDL_GL_CreateContext`.
- **Solver purity:** The physics layer (`Simulator`, `FrameSimulator`, `FrameElement`, `MemberForces`) has no dependency on SDL, OpenGL, or ImGui. All unit tests run headless.
