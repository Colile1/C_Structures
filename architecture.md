# C_Structures — Architecture

## Overview

C_Structures uses a layered architecture. Each layer has a single responsibility and communicates downward — the UI layer calls the model layer; the physics layer reads the model; the rendering layer reads both. No layer calls upward.

```
┌─────────────────────────────────────────────┐
│                  main.cpp                   │  Orchestration only
├──────────────┬──────────────────────────────┤
│  UIHandler   │      Camera                  │  Input & 2D overlay
├──────────────┴──────────────────────────────┤
│        ForceRenderer  +  Geometry           │  3D rendering
├─────────────────────────────────────────────┤
│               Simulator                     │  Physics (FEM solver)
├──────────────────────┬──────────────────────┤
│    Node / Beam       │    CSVHandler        │  Data model & I/O
└──────────────────────┴──────────────────────┘
```

---

## Component Map

### Data Model (`include/model/`, `src/model/`)

| File | Responsibility |
|------|----------------|
| `Node.hpp` | A point in 3D space. Stores position, applied force, and fixed-support flag. All methods inline. |
| `Beam.hpp` / `Beam.cpp` | A structural member between two nodes. Stores Young's modulus (E) and cross-section area (A). Computes length and stiffness (AE/L). |

Nodes and beams are stored as `std::vector<Node>` and `std::vector<Beam>` in `main.cpp`. Beams hold raw pointers into the node vector (`Node*`). This means the vectors must not be reallocated after beams are created (use `reserve()` if adding nodes dynamically).

---

### Physics (`include/physics/`, `src/physics/`)

| File | Responsibility |
|------|----------------|
| `Simulator.hpp/.cpp` | Assembles the global stiffness matrix K, applies boundary conditions, solves K·u = F using Eigen's ConjugateGradient solver. |

**Solving pipeline (called in order by `solveStaticForces()`):**

```
populateForceVector()         reads node.getAppliedForce() → m_forces
assembleGlobalStiffnessMatrix()  builds sparse K from beam stiffness blocks
applySupportConstraints()     zeros fixed-DOF rows/cols, sets diagonal=1
ConjugateGradient.solve()     computes m_displacements = K⁻¹ · m_forces
```

**DOF layout:** each node occupies 3 consecutive entries in the global vectors (x=3i, y=3i+1, z=3i+2).

**`getBeamForce(beam)`** returns the signed axial force: positive = tension, negative = compression. It projects the relative displacement of the two end nodes onto the beam axis and multiplies by stiffness.

---

### Rendering (`include/visualization/`, `include/graphics/`)

| File | Responsibility |
|------|----------------|
| `Shader.hpp` | Header-only. Compiles and links GLSL programs, exposes `setMat4`, `setVec3`, `setFloat` uniform setters. |
| `Camera.hpp/.cpp` | Orbit camera. Converts (yaw, pitch, radius) spherical coordinates to `glm::lookAt` view matrix. Right-drag = orbit, scroll = zoom. |
| `ForceRenderer.hpp/.cpp` | VAO-based arrow renderer for applied force vectors. Generates cylinder+cone meshes on init. `getBeamColor()` maps axial force to blue (tension) / red (compression) / grey (neutral). |
| `RendererUtils.hpp/.cpp` | Legacy stubs — superseded by ForceRenderer. Functions exist but are no-ops. |

**Geometry in `main.cpp`:** sphere (UV sphere, 8×12) and cylinder (12-segment) VAOs are built once at startup by `buildSphereVAO()` / `buildCylinderVAO()`. The cylinder model matrix is constructed per-beam to align the unit Y-axis to the beam direction.

**Shader files** (`shaders/`): loaded at runtime from the `shaders/` directory next to the executable. The `start.sh` copies them there during the build step.

---

### UI (`include/ui/`, `src/ui/`)

| File | Responsibility |
|------|----------------|
| `UIHandler.hpp/.cpp` | 2D toolbar overlay (orthographic OpenGL shader). Handles tool mode switching, node placement, beam creation, and force application. Converts mouse pixel coordinates to world-space XZ-plane position via ray unprojection. |

**Tool modes:**

| Mode | Key | Left-click action |
|------|-----|-------------------|
| `NODE_PLACEMENT` | N | Adds a node at the XZ-plane intersection of the mouse ray |
| `BEAM_CREATION` | B | First click selects a node; second click connects it to another |
| `FORCE_APPLICATION` | F | Applies `forceVector` to the clicked node |

The toolbar occupies pixels 0–110 on the left. Clicks in that region are consumed by the toolbar and do not reach the 3D scene.

---

### Data I/O (`include/data/`, `src/data/`)

| File | Responsibility |
|------|----------------|
| `CSVHandler.hpp/.cpp` | Static `loadStructure()` and `saveStructure()`. Format: `NODE x y z fixed` and `BEAM startIdx endIdx E A`. |

CSV files use indices (0-based) not pointers, so they are position-independent. `saveStructure` finds node indices by scanning positions — this uses floating-point equality comparison and works correctly only when positions are exact (not modified after saving). For robustness, always save before modifying node positions.

---

## Data Flow: Full Frame

```
SDL_PollEvent
    │
    ├─► Camera.handleMouseDrag / handleScroll    (orbit camera)
    │
    └─► UIHandler.handleEvent(e, nodes, beams, view, proj)
            │
            ├─► toolbar click → change currentTool
            ├─► scene click (NODE_PLACEMENT) → nodes.emplace_back(...)
            ├─► scene click (BEAM_CREATION)  → beams.emplace_back(...)
            └─► scene click (FORCE_APPLICATION) → node.applyForce(...)

[Enter key] → Simulator(nodes, beams).solveStaticForces()
                     │
                     └─► m_displacements, ready for next frame

Render:
    geoShader.use()
    drawGrid(...)
    for each beam → getBeamForce() → getBeamColor() → drawBeam(displaced pos, color)
    for each node → drawNode(displaced pos, red/white)
    forceRenderer.renderForceVectors(nodes, view, proj)
    uiHandler.renderUI(window)     ← 2D overlay on top
    SDL_GL_SwapWindow
```

---

## Adding a New Feature

### Add a new tool mode
1. Add an entry to `enum class ToolMode` in `UIHandler.hpp`
2. Add a case in `UIHandler::handleEvent` switch
3. Add a button call in `UIHandler::renderToolbar`
4. Add keyboard shortcut in the `SDL_KEYDOWN` block

### Add a new material property to Beam
1. Add a private member and getter in `Beam.hpp`
2. Implement getter in `Beam.cpp`
3. Add to `CSVHandler::saveStructure` output
4. Add parsing in `CSVHandler::loadStructure`
5. Add a test in `ModelTests.cpp` and `CSVHandlerTests.cpp`

### Add a new physics quantity
1. Add a method to `Simulator.hpp` (e.g. `getNodeReaction`)
2. Implement it in `Simulator.cpp` using `m_displacements` and `m_globalK`
3. Call it from `main.cpp` render loop as needed
4. Add a test in `PhysicsTests.cpp` or `IntegrationTests.cpp`

---

## Key Constraints

- **Node vector stability:** `std::vector<Beam>` holds `Node*` pointers. Never call `nodes.push_back` or `nodes.emplace_back` after beams have been created, as this can reallocate the vector and invalidate all beam pointers. Use `nodes.reserve(N)` before building the structure if you know the node count in advance.
- **Solver re-use:** `Simulator` stores references to `nodes` and `beams`. If you add new nodes/beams after construction, create a new `Simulator` (main.cpp does this on Enter keypress).
- **Shaders at runtime:** GLSL files are loaded from disk at startup. The executable must be run from the `build/` directory (or wherever `shaders/` was copied), not from the project root.
- **OpenGL context required for ForceRenderer/UIHandler:** `initialize()` must be called after the OpenGL context is created. Do not construct these objects before `SDL_GL_CreateContext`.
