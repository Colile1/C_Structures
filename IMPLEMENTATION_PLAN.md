# C_Structures — Implementation Plan

Date: 2026-06-12
Built with the `project-start` workflow (adapted for an existing codebase).
Inputs: `GLITCHES_AND_FIX_PLAN.md` (what's broken) and `IMPROVEMENT_PLAN_2026-06.md` (what to build next).

This plan turns the fix list and the improvement themes into a single **numbered, one-step-at-a-time build order**, each step with an objective "done when". It is meant to be executed by a fresh Claude Code (or solo) session per step, committing one step at a time. Code is written per step — not all at once.

---

## 1. Scope (fixed — do not drift)

**Purpose.** C_Structures is a real-time, interactive 3D structural analysis tool for engineering students and educators: build a truss or frame, apply loads and supports, and see reactions, deformed shape, and axial/shear/moment/torsion diagrams, with an optional "show the math" glass-box.

**Fixed design decisions (changing these is a design change, not a step):**
- Language/stack: **C++17**, **Eigen** (sparse, `SparseLU` + static condensation), **glm**, **SDL2 + OpenGL + GLEW**, **Dear ImGui** for UI, **GoogleTest** for tests, **CMake** build.
- Two solvers kept side by side: pin-jointed **truss** (`Simulator`, 3 DOF/node) and rigid-jointed **frame** (`FrameSimulator`, 6 DOF/node). Frame mode is a toggle; the truss is the simple teaching case.
- Data model: `Node` (position, joint type, applied force/moment), `Beam` (integer node indices, E/A/I, material). **Beams reference nodes by index, never by pointer.**
- Interchange format stays **CSV**; a richer **JSON** project format may be added but does not replace CSV.
- No dynamic/seismic analysis, no plasticity, no nonlinearity — static linear elastic only (matches the README's stated non-goals).

**Correctness-critical components** (subtle errors here silently invalidate results — these get the high-reasoning model tier and their own test step):
1. `FrameElement` / `FrameSimulator` — element stiffness, transformation, assembly, BC handling.
2. `MemberForces` — internal-force diagram reconstruction (the headline output).
3. `DistributedLoad` — consistent equivalent nodal loads.
4. `Simulator` — truss solve, reactions, equilibrium.

**Deliverables / "done".** All glitches in `GLITCHES_AND_FIX_PLAN.md` Tiers A–B closed; CI green on push; diagrams correct and labelled; no silent solver failure; docs reconciled. Themes from the improvement plan are then additive build steps.

---

## 2. Build order

Steps are built and committed **one at a time, in order**. ⚑ marks correctness-critical steps (use the top model tier; never start one without confirming the tier — see §4).

| # | Step | Files touched | Verification (done when) |
|:-:|------|---------------|--------------------------|
| 1 | **Re-enable CI** | `.gitignore`, `.github/workflows/*.yml` | A pushed commit triggers an Actions run that configures and runs `ctest`; the run is visible on GitHub. (Fixes G1) |
| 2 | **CI runs the unit tests headless** | `.github/workflows/ci.yml` | CI installs Eigen/glm/GTest only, builds the `tests` target (no SDL/GL), runs `ctest --output-on-failure`; a deliberately broken test makes CI red. (G1) |
| 3 | **Purge committed build artifacts** | git index, `.gitignore` | `git rm -r --cached build build_win`; `git ls-files \| grep -E 'build_win/\|\.exe\|\.dll\|\.obj'` is empty; clean clone still builds. (Fixes G6) |
| 4 ⚑ | **Correct internal-force diagrams under span loads** | `physics/MemberForces.{hpp,cpp}`, caller in `main.cpp` | New `tests/MemberForcesTests` case: simply-supported beam under UDL `w` gives `M_max = wL²/8` at mid-span and `V = 0` at mid-span, within tol; existing 40 tests still pass. (Fixes G2) |
| 5 ⚑ | **Frame-aware determinacy** | `physics/Determinacy.{hpp,cpp}`, call site | `tests/DeterminacyTests`: a propped cantilever (frame, 6 DOF/node) and a simple truss are each classified correctly; mechanism hint fires on an unsupported frame. (Fixes G3) |
| 6 | **Surface solver status in the UI** | `physics/Simulator.*`, `physics/FrameSimulator.*`, `ui/ModelCheckPanel.*`, `main.cpp` | `solve()` returns a status+message; an under-constrained model shows an on-screen "mechanism — add supports" message instead of a silent no-op (manual smoke + a unit test on the returned status). (Fixes G5) |
| 7 | **Auto-scale the deformed shape** | `main.cpp` (replace `dispScale=50`), `ui/UIHandler.*` | Stiff steel truss and soft timber model both show a visibly sensible deflected shape at the default slider; on-screen "×N" label reflects the auto factor. (Fixes G7) |
| 8 | **Docs reconciliation** | `README.md`, `architecture.md` | README names `SparseLU`+frame/truss modes and drops the REST/ER/Trello fiction; `architecture.md` says Dear ImGui and lists the new physics modules; a new reader can predict app behaviour from the docs. (Fixes G9) |
| 9 | **Hygiene batch** | delete `tests/CMakeLists.txt`, remove `RendererUtils.*` + call sites + CMake entry, add `README.md` to `model/ physics/ graphics/ visualization/ ui/ data/` | Project builds with `RendererUtils` gone; no stale test stub; each component folder has a README per `rules.md`. (Fixes G10–G12) |
| 10 ⚑ | **Concentrated nodal moments** | `model/Node.hpp` (`applyMoment`), `physics/FrameSimulator.cpp` (moment DOFs), `data/CSVHandler.*`, `ui/LoadsPanel.*` | Cantilever with tip moment `M` reproduces `θ = ML/EI` within tol (new `FrameTests` case); CSV round-trips the moment. (Fixes G4) |
| 11 ⚑ | **Self-weight load** | `model/Beam.hpp` (density per material), `physics/DistributedLoad.*`, `ui` toggle | Enabling self-weight on a horizontal beam adds reactions equal to `ρ·A·g·L` (equilibrium tick stays green); test asserts total vertical reaction. |
| 12 ⚑ | **Internal hinges / member-end releases** | `model/Beam.hpp` (release flags), `physics/FrameSimulator.*`, wire `internal_hinge`/`rigid` icons | Portal frame with a hinged base matches the textbook reaction/moment within tol (new test). (Fixes G8) |
| 13 | **Diagram annotations + quantity switch** | `main.cpp` `drawForceDiagrams`, `ui` | Each diagram shows peak value+location and end values; user can switch axial/shear/moment/torsion; convention is labelled on screen. |
| 14 | **Reactions table + equilibrium badge** | `ui/ReactionsPanel.*` | Panel lists each support's force/moment and shows "ΣF ≈ 0 ✓" from `checkEquilibrium`; matches a hand-checked example. |
| 15 | **Re-solve only on change + cache factorisation** | `main.cpp`, solver classes | Editing the model re-solves automatically without pressing Enter; large model stays interactive; results identical to the Enter-triggered path (regression test on a fixed model). |
| 16 | **Visual icon palette + view-mode toggle** | `ui/*`, rasterise `resources/icons/*.svg` to ImGui textures | Left palette shows joint/section/load icons; the symbol/realistic-2D/realistic-3D toggle changes the rendered icons as in `resources/icons/index.html`. |
| 17 | **Plain-language layer + templates as cards** | `ui/*`, `ui/Templates.*` | Tooltips on every control; results read as sentences beside numbers; templates appear as one-click cards that load a correct model. |
| 18 | **Shareable output: PNG + one-page PDF report** | new `export/` module, `ui` | "Export report" produces a PDF with model summary, reactions, member-force table, and diagrams; "Export image" saves a PNG of the view/diagram. |
| 19 | **JSON project format** | new `data/JSONHandler.*` | Save/Load round-trips joint types, materials, loads, and view-mode preference; CSV still works as interchange. |
| 20 (stretch) ⚑ | **WebAssembly build of the solver core** | new `wasm/` CMake target, Emscripten toolchain | `emcc` builds `Simulator`+`FrameSimulator`+`FrameElement` to Wasm; a browser harness solves the single-bar and cantilever cases and matches the native results. |

Ordering logic: **safety net first** (1–3 restore CI and a clean repo so every later step is verified automatically), then the **wrong-output fixes** (4–7), then **honesty/hygiene** (8–9), then **modelling completeness** (10–12, all correctness-critical), then **UX and output** (13–19), with **Wasm** as the de-risked stretch (20). Tests for each correctness-critical module are written *in the same step* as the module, never deferred.

---

## 3. Coding standard (restate, enforce per step)

One responsibility per file; ~120-line soft limit per file (split when exceeded). Every source file starts with the existing copyright header and a one-line purpose. Every non-trivial function carries a Purpose / Inputs / Output comment (the physics files already do this — match the style). Keep `main.cpp` thin: orchestration only, no physics or assembly logic. Errors must be specific and actionable and reach the user (no `std::cerr`-only failures in GUI paths). The solver core stays pure and deterministic so it is unit-testable without SDL/OpenGL. Small, meaningful commits — one build step per commit.

---

## 4. Model & mode policy (for Claude Code sessions)

At the start of every session and every build step, check the step against this table and switch model (`/model`) if the tier doesn't match. **Never start a ⚑ correctness-critical step without confirming the top tier first.**

| Step type | Examples | Model tier |
|-----------|----------|-----------|
| Correctness-critical physics ⚑ | Steps 4, 5, 10, 11, 12, 20 | **Top tier (Fable / Opus)** — subtle stiffness/sign/BC errors silently invalidate results |
| Integration / UI with logic | Steps 6, 7, 13, 14, 15, 16, 18, 19 | Mid tier (Opus / Sonnet) |
| Scaffolding / docs / hygiene | Steps 1, 2, 3, 8, 9, 17 | **Sonnet** — save usage on routine work |

Reverse reminder: if you find yourself on the top tier doing doc edits or `.gitignore` changes, drop to Sonnet.

---

## 5. Per-step working rhythm

1. **`/clear`** — fresh session at the start of each build step.
2. **Plan Mode first** — read this plan + `CLAUDE.md` (if present) + the relevant folder README, plan the step, confirm the model tier from §4.
3. **Accept Edits** — implement the approved plan.
4. **Verify** — run `ctest --output-on-failure` (or the step's named test), review `git diff`, commit with a short message naming the step.
5. **State the next step** before ending the session.

---

## 6. First action

Start with **Step 1 (Re-enable CI)** — it is half an hour and makes every subsequent step self-verifying. Suggested first Claude Code prompt:

> "Read `IMPLEMENTATION_PLAN.md` and `GLITCHES_AND_FIX_PLAN.md`, then do build step 1: re-enable CI. Confirm the model tier from the policy table first."

Recommended: open `C_Structures/C_Structures/` itself as the workspace root so a future `CLAUDE.md` auto-loads. (Creating that `CLAUDE.md` — carrying §1 scope guards, §3 standard, §2 build order, §4 policy — is a sensible Step 0 if you want the contract to persist across sessions.)
