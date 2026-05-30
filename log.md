# C_Structures — Development Log

Author: Colile Sibanda
Format: [YYYY-MM-DD HH:MM]

---

[2026-05-31 01:00]
**Docs updated — starter_guide.md, .gitignore, log.md brought up to date**
`starter_guide.md`: replaced the stale vcpkg-based build instructions with the verified MSYS2/MinGW64 workflow (Chocolatey → pacman → cmake/ninja). Added Linux/macOS apt and Homebrew equivalents. Updated Controls table to match the current UIHandler (Select/Node/Beam/Force + Undo/Redo/Delete/Ctrl+N). Updated CSV format section to document all 6 JointType integer values (was binary 0/1 only). Added `getBeamForce` to the code examples. Expanded project tree to include `resources/icons/`, `build_win/`, `build_windows.sh`, `REVIEW.md`, `IMPROVEMENT_PLAN.md`, `CSVHandlerTests.cpp`, `IntegrationTests.cpp`, `py_logic_tests.py`.
`.gitignore`: added `build_win/`, `build_linux/`, `build_mac/` alongside `build/`; added `DartConfiguration.tcl`, `*.pdb`, `*.lib`, `*.obj`; named CSV temporaries with exact patterns used by the test suite; removed `log.md` and `starter_guide.md` from the ignore list (they should be committed).

---

[2026-05-31 00:30]
**Full build verified on Windows — 22/22 C++ tests passing, 0 failures**
Installed: MSYS2 + MinGW64 toolchain via Chocolatey/pacman (g++ 16.1.0, cmake 4.3.3, ninja 1.13.2, SDL2, Eigen3, glm, GLEW, GTest). Fixed stale Linux CMakeCache from source root and configured a fresh build in `build_win/`. Fixed one additional compile error: `int main()` → `int main(int, char*[])` required by SDL2's `SDL_main` macro on Windows. All 4 test suites passed: ModelTests (6), PhysicsTests (2), CSVTests (6 incl. new JointTypeRoundTrip), IntegrationTests (8 incl. NodeVectorReallocationKeepsConnectivity). Executable: `build_win/C_Structures.exe`.

---

[2026-05-30 23:45]
**Bugfixes — 3 correctness bugs found and fixed; 28/28 Python logic tests passing**
Fixed:
1. `CSVHandlerTests.cpp:19` — still used the removed `Beam(Node*, Node*, E, A)` pointer constructor (would have caused a compile error). Changed to `Beam(0, 1, ...)` / `Beam(1, 2, ...)`.
2. `UIHandler.cpp` — `beamStart` used as a C++ int-truth expression in the beam-creation hint label. In C++, `-1` (no selection) is nonzero → truthy, so the label showed "Click end node" when no start was selected, and "Click start node" when node 0 was selected. Both wrong. Fixed to `beamStart >= 0`.
3. `CSVHandler.cpp` / `.hpp` — saved JointType as `isFixed() ? 1 : 0`, silently collapsing ROLLER_X, ROLLER_Y, ROLLER_Z, PIN_XY to FREE (0) on every save/load round-trip. Fixed to write/read the full `static_cast<int>(getJointType())` int (0–5). Added a `JointTypeRoundTrip` test covering all six types.
Verification: 28 Python tests run (no C++ compiler available locally): CSV parse/save (5 cases), eraseNodeAt reindex (7 cases incl. chain-delete), physics math against closed-form (10 cases incl. both REVIEW.md benchmarks + integration benchmark), beamStart truthiness (4 cases), SVG XML validation (96 icons). All 28 passed.
Note: full SDL2/OpenGL/ImGui build validation still needs a machine with those deps installed.

---

[2026-05-30 22:30]
**Phase 0.1 — Replaced raw `Node*` with integer indices in `Beam` (safety, critical)**
Why: `Beam` stored `Node*` and `Simulator`/UI recovered indices by pointer arithmetic (`getStart() - &nodes[0]`). Interactively adding a node could reallocate the `std::vector<Node>`, dangling every beam pointer → undefined behaviour. The CSV save also matched nodes by float-equality of positions, breaking after any position edit.
Impact: `Beam` now holds `int startNode/endNode`; constructors take indices; `getLength`/`getStiffness` take the node vector. `Simulator` and `main` use `getStartIdx()/getEndIdx()` directly (no pointer math). `CSVHandler` writes/reads indices directly (float-equality scan removed). `UIHandler` selection state (`selectedNode/selectedBeam/beamStart`) is now `int` (-1 = none); a new `eraseNodeAt` helper deletes a node and re-indexes surviving beams so connectivity stays valid. This removes the reallocation UB and the CSV mapping bug at once.
Verification: ported the two REVIEW.md benchmarks into `PhysicsTests.cpp` as permanent regression tests (single-bar `F·L/AE` elongation + tension; symmetric two-bar truss equal forces) and added an `Integration` test that adds 300 nodes after a beam to prove reallocation no longer corrupts connectivity. Truss math re-confirmed in NumPy (elongation 1.0e-4 m, member force +1000 N; symmetric members equal & compressive). The `eraseNodeAt` re-index logic was unit-checked in a standalone harness. Note: the full SDL2/OpenGL/ImGui GUI still needs a build+test run on a machine with those deps installed — the headless core (model/physics/CSV) is what was verifiable here.
Why: The review surfaced findings that needed an actionable sequence, not just a list of problems.
Impact: Five-phase plan with effort/impact table and a dependency-ordered execution diagram. Phase 0 (replace raw `Node*` with integer indices, reconcile docs, surface errors, per-folder READMEs) before anything else; Phase 1 completes a trustworthy truss (reactions, determinacy check, better colour ramp); Phase 2 is the 6-DOF frame-element upgrade that finally makes bending/shear/moment and `I` real; Phase 3 builds the dual-audience UI on top of the new icon library; Phase 4 covers robustness/export/WASM reach.

---

[2026-05-30 04:10]
**Added: REVIEW.md — full project review report**
Why: Requested audit of correctness, reliability, UI, and dual technical/non-technical usefulness.
Impact: Documents the headline finding — the engine is a 3D axial **truss/bar** solver (3 DOF/node) while the README promises bending/shear/moment and stores an unused `I`. Independently re-implemented the `Simulator.cpp` algorithm in NumPy and verified it against closed-form solutions (single bar matches F·L/AE to full precision; symmetric truss gives equal member forces) — the implemented method is correct. Flags the raw-`Node*` reallocation hazard, CSV float-equality bug, missing reactions, doc drift (architecture.md describes a custom OpenGL toolbar but the app uses Dear ImGui; README cites a non-existent REST API), and the non-technical-usability gap. Grounded against FTool / MASTAN2 / SkyCiv / Frame3DD.

---

[2026-05-30 04:00]
**Reviewed: full codebase, physics, build, docs, and tests**
Why: Establish an evidence-based baseline before recommending changes.
Impact: Confirmed axial-only truss scope (`I` never read by the solver), verified solver correctness numerically, identified the `Beam`-holds-`Node*` UB risk on vector reallocation, and catalogued documentation inconsistencies. Findings feed REVIEW.md and IMPROVEMENT_PLAN.md.

---

[2026-05-30 03:40]
**Added: resources/icons/ — coordinated icon library (96 SVGs)**
Why: The UI labelled components with enum names (ROLLER_X, etc.) and ASCII — unreadable for non-technical users — and there was no visual asset for joints, sections, or loads.
Impact: New `resources/icons/` with the two requested trees — `symbols/` (traditional civil-engineering line symbols) and `realistic/` (`2d/` shaded elevations + `3d/` isometric renders) — so users can pick a view mode. Covers 8 joints (the six `JointType` enum values plus internal-hinge and rigid connection), 15 beams (9 cross-sections: I, H, channel, angle, tee, box/RHS, pipe/CHS, solid rect, solid round; 6 structural types: simply-supported, cantilever, fixed-fixed, continuous, overhanging, truss), and 9 forces (point, UDL, triangular, moment, tension, compression, shear, reaction, self-weight) — 32 components × 3 variants. All 96 SVGs validated as renderable. Includes a self-contained `index.html` gallery with a Symbols/2D/3D toggle and name filter, a `manifest.json`, a `README.md`, and the deterministic `generate.py` so the whole set can be restyled and regenerated in sync.

---

[2026-05-30 22:30]
**Added: Proprietary LICENSE + per-file copyright notices**
Why: No legal protection existed. Anyone could copy or use the code without permission.
Impact: `LICENSE` (project root) asserts all rights reserved under South African law and prohibits all use without written approval from colilesibanda@gmail.com. The two-line copyright comment was prepended to all 12 source/header files so the claim travels with any individual file even if separated from the repo.



[2026-05-30 21:55]
**Implemented: Font Awesome 6 icon font in toolbar**
Why: ASCII placeholder labels ([S], [+], [=], [v]) gave the toolbar an unfinished look and wasted space.
Impact: `fa-solid-900.ttf` is merged into ImGui's default font at startup. Toolbar buttons and Undo/Redo now display FontAwesome icons (arrow-pointer, circle-plus, ruler, bolt, rotate-left/right). Falls back silently if the font file is missing.

---

[2026-05-30 21:45]
**Added: Undo/redo system (Ctrl+Z / Ctrl+Y / Ctrl+Shift+Z)**
Why: Every destructive action (place node, draw beam, apply force, delete, move, change joint/material) was irreversible, making experimentation risky.
Impact: Snapshot-based undo stack with 50-level depth. Every mutation captures the full scene state (node positions, joint types, forces; beam connectivity, material, E, A, I). Undo/Redo buttons visible in toolbar with depth counter in status bar. Edit menu added to menu bar.

---

[2026-05-30 21:30]
**Added: Beam material presets and section properties (E, A, I)**
Why: All beams shared one hardcoded stiffness value with no way to model different materials or sections.
Impact: `BeamMaterial` enum adds Steel (200 GPa), Aluminum (70 GPa), Concrete (30 GPa), Timber (12 GPa), Custom presets. `Beam` now stores second moment of area `I` (m⁴) for future frame-element solver. Selecting a beam in the Properties panel shows/edits Material, E (GPa), A (cm²), I (cm⁴), length (m), and derived AE/L.

---

[2026-05-30 21:10]
**Added: JointType enum — 6 support conditions per node**
Why: Only a binary "fixed/free" flag existed, making it impossible to model rollers, pins, or partial constraints needed for realistic structures.
Impact: `JointType` enum: FREE, FIXED, PIN_XY, ROLLER_X/Y/Z. Each type constrains specific translational DOFs. `isDOFConstrained(d)` drives the solver directly. UI shows a "Joint Type" combo in the node properties panel. Nodes are colour-coded by type (red=fixed, orange=pin, cyan/green/purple=rollers).

---

[2026-05-30 20:50]
**Fixed: Solver divergence — replaced ConjugateGradient + penalty BCs with static condensation + SparseLU**
Why: `SimplicialLDLT` (and before it `ConjugateGradient`) failed on the triangle truss. Root cause: penalty BCs mix diagonal=1 (fixed DOFs) with diagonal≈1e7 (stiffness DOFs), creating a scale mismatch that broke iterative convergence and confused the Cholesky SPD check.
Impact: `solveStaticForces` now uses proper static condensation: identifies free DOFs, extracts the `K_FF` sub-matrix directly (no matrix modification), and solves with `Eigen::SparseLU`. This is both numerically correct and the standard FEM approach. "Decomposition failed" spam eliminated.

---

[2026-05-30 20:40]
**Updated: .gitignore**
Why: `imgui.ini`, temp test CSV files, `.exe`/`.dll` Windows binaries, and several doc-only files were not excluded, creating noise in `git status`.
Impact: Build artifacts, runtime temporaries, and internal documentation files are now ignored.

---

[2026-05-29]
**Fixed: Node.hpp duplicate method definitions**
Why: `applyForce()` and `getAppliedForce()` were defined twice in the header, causing a redefinition compile error.
Impact: Removes a hard compilation blocker; Node now compiles cleanly.

---

[2026-05-29]
**Added: Beam::getYoungsModulus() and Beam::getCrossSection()**
Why: CSVHandler::saveStructure() was writing derived values (stiffness, length) instead of the original material inputs. The load function expects E and A, so save must write the same.
Impact: CSV round-trip is now consistent; templates created by save can be correctly reloaded.

---

[2026-05-29]
**Created: include/graphics/Shader.hpp**
Why: ForceRenderer referenced a Shader class that did not exist anywhere in the project. The header-only approach avoids a separate .cpp compile unit and keeps the class self-contained.
Impact: ForceRenderer now compiles; all shader programs across the app use a unified API.

---

[2026-05-29]
**Fixed: Simulator.hpp return type + added getBeamForce()**
Why: getNodeDisplacements() was declared as vector<float> but main.cpp treated each element as a glm::vec3. main.cpp also called getBeamForce() which did not exist. Force vector was never populated from node applied forces, so the solver always solved K*0=0.
Impact: Physics solver now reads node forces into the Eigen vector before solving. Displacements are returned correctly as vec3. Axial beam forces can be queried after solving.

---

[2026-05-29]
**Rewrote: UIHandler.cpp**
Why: The file was corrupted with 5-6x duplicate function definitions and broken brace nesting caused by repeated copy-paste. All UI draw calls were empty stubs.
Impact: UIHandler now compiles and renders a working 2D toolbar overlay using a dedicated orthographic OpenGL shader. Users can switch tool modes by clicking buttons or pressing N/B/F.

---

[2026-05-29]
**Created: include/graphics/Camera.hpp + src/graphics/Camera.cpp**
Why: No camera existed; the render loop had no view or projection matrix setup. Without a camera the scene cannot be rendered at any angle.
Impact: Users can now right-click-drag to orbit and scroll to zoom. The camera is an orbit type centred on the world origin, suitable for inspecting structures from any angle.

---

[2026-05-29]
**Rewrote: src/main.cpp**
Why: Main contained placeholder drawSphere/drawCylinder stubs using deprecated OpenGL fixed-function calls (glPushMatrix), called getBeamForce() which did not exist, used wrong displacement type (float vs vec3), and called renderForceVectors() as a static method with wrong arguments.
Impact: Full working render loop: reference grid, sphere nodes, cylinder beams coloured by force (blue=tension, red=compression), applied force arrows, and 2D UI overlay. Pressing Enter re-solves the simulation.

---

[2026-05-29]
**Fixed: ForceRenderer — static constexpr glm::vec3 and include path**
Why: glm::vec3 is not a literal type and cannot be constexpr. Include path "../graphics/Shader.hpp" was relative to the old include structure.
Impact: ForceRenderer compiles; colour constants are accessible as static class members.

---

[2026-05-29]
**Fixed: All unit tests (ModelTests.cpp, PhysicsTests.cpp)**
Why: ModelTests had orphaned code outside any TEST() block. PhysicsTests chained setFixed() on a void return and accessed displacement vector with wrong element type.
Impact: All tests compile and validate core Node, Beam, and Simulator behaviour.

---

[2026-05-29]
**Fixed: Bad include paths in src/*.cpp files**
Why: CSVHandler.cpp used `#include "include/data/CSVHandler.hpp"` and Beam.cpp used `#include "include/model/Beam.hpp"`. These paths are relative to the source file location, so they resolve to `src/data/include/...` which does not exist. The correct path with `-I include/` in CMake is just `"data/CSVHandler.hpp"`.
Impact: Source files now include headers correctly; the test executable (which also compiles these .cpp files) will build without header-not-found errors.

---

[2026-05-29]
**Fixed: Node.cpp duplicate method definitions**
Why: Node.hpp defines all methods inline in the class body. Node.cpp was additionally defining `getPosition()`, `isFixed()`, `setFixed()`, and `applyForce()`, which causes "multiple definition" linker errors when any translation unit includes Node.hpp.
Impact: Node.cpp is now intentionally empty; only the inline header definitions are compiled.

---

[2026-05-29]
**Added: CSVHandlerTests.cpp**
Why: No tests existed for the CSV import/export pipeline. Silent bugs in saveStructure (previously writing stiffness instead of E and A) would not have been caught by the existing tests.
Impact: Five tests now verify save/load round-trips, fixed-flag preservation, material property preservation, and stiffness consistency after a reload.

---

[2026-05-29]
**Added: IntegrationTests.cpp**
Why: Existing tests checked components in isolation. The most important correctness guarantee (does the physics answer make sense end-to-end?) had no coverage. Specifically: F*L/(A*E) displacement formula, force transmission across multiple beams, compression sign convention, and the full CSV→load→solve pipeline.
Impact: Seven integration tests now verify the complete Node+Beam+Simulator pipeline, including a CSV round-trip followed by a physics solve.

---

[2026-05-29]
**Created: requirements.sh**
Why: Dependencies (SDL2, GLEW, Eigen3, GLM, GTest) must be installed before CMake can configure the project. With no setup script, a new developer had to manually identify and install each library.
Impact: Single script detects the OS (Ubuntu/WSL, Fedora, Arch, macOS) and installs all missing packages automatically. Prints clear instructions for Windows Git Bash users to use vcpkg instead.

---

[2026-05-29]
**Created: start.sh**
Why: Building and launching the app required knowing the correct cmake flags, parallel job count, and shader copy step. No single command existed to go from source to running app.
Impact: `./start.sh` installs dependencies, configures CMake, builds, runs all tests, copies shaders, and launches the app. Flags `--clean`, `--tests-only`, and `--no-run` cover common developer workflows.

---

[2026-05-29]
**Updated: CMakeLists.txt — named test suites and CSVHandler in test build**
Why: The test executable previously had a single catch-all `add_test(NAME Tests ...)`. Adding CSVHandlerTests and IntegrationTests required CSVHandler.cpp to be compiled into the test binary, and named suites allow `ctest -R ModelTests` style filtering.
Impact: `ctest` can now run individual suites: ModelTests, PhysicsTests, CSVTests, IntegrationTests. CSVHandler round-trip is now exercised in the test build without SDL2/OpenGL dependencies.
