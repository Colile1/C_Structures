# C_Structures — Current Glitches & Fix Plan

Date: 2026-06-12
Author: prepared for Colile Sibanda
Method: full source read of `model/`, `physics/`, `ui/`, `data/`, `visualization/`; build and run of the full unit-test suite (40 tests) in a Linux sandbox using the project's own headers (Eigen 3.4, glm, GoogleTest); git/CMake/`.gitignore` inspection.

This document supersedes the *stability* sections of the 2026-05-30 `REVIEW.md`. Since that review the project has advanced a great deal: beams now store integer node indices (the dangling-pointer hazard is gone), and a 6-DOF frame solver, support reactions, determinacy check, member-force diagrams, distributed loads, and a beginner/engineer mode have all been added. The glitches below are the issues that remain *today*, found against the current code — not the old ones.

---

## 1. Test result: the tested core is healthy

The full suite builds and passes: **40/40 tests across 12 suites** — model, truss physics, reactions, CSV round-trips, integration, determinacy, frame element, member forces, and distributed loads. The single-bar `F·L/AE` check, the symmetric-truss split, the frame cantilever tip deflection, and the UDL cantilever deflection all pass. So the numerical kernels that have tests can be trusted.

The glitches are therefore concentrated where there is **no test coverage**: the UI/orchestration layer, the build/CI plumbing, and a few physics paths that are computed but never asserted.

---

## 2. Glitches found

Severity: **High** = wrong results or broken process a user/marker will hit; **Medium** = misleading output or real friction; **Low** = hygiene/compliance.

| ID | Severity | Area | Glitch | Evidence |
|----|:--------:|------|--------|----------|
| G1 | High | CI / process | GitHub Actions CI is effectively **disabled**. `.gitignore` ignores the whole `.github/` directory, so `.github/workflows/*.yml` are untracked — `git ls-files .github/` returns nothing. The README advertises "CI/CD: GitHub Actions" that is not actually in the repo, so no test runs on push. | `.gitignore` line `.github/`; `git ls-files .github/` empty |
| G2 | High | Physics (diagrams) | Internal-force **diagrams ignore intra-span distributed load**. `MemberForces::memberInternalAt` reconstructs the diagram from the member's two end forces only, so axial/shear come out constant and moment **linear**. A member under a UDL actually has linearly-varying shear and a **parabolic** moment. End values and nodal deflections are right (hence the passing deflection test), but the drawn SFD/BMD between the ends is wrong — and the diagram is the headline teaching output. | `src/physics/MemberForces.cpp` `memberInternalAt`; code comment "With only nodal loads…"; `drawForceDiagrams` in `src/main.cpp` samples via `sampleMember` |
| G3 | Medium | Physics (determinacy) | The **determinacy / stability check is truss-only** (`dof = 3·nodes`, counts translational reactions only) but is shown even in 6-DOF frame mode. In frame mode it uses the wrong DOF count and ignores rotational restraint, so the determinate/mechanism verdict can be misleading. | `src/physics/Determinacy.cpp` `res.dof = 3*res.nodes`, `countReactions` translational only |
| G4 | Medium | Physics (loads) | **Concentrated nodal moments cannot be applied.** `FrameSimulator::populateForces` leaves the three moment slots per node at zero with the comment "via Node::applyMoment (Phase 2.3+)", and `Node` has no moment field or `applyMoment`. Frame mode silently cannot represent a point moment at a joint. | `src/physics/FrameSimulator.cpp:37`; `grep applyMoment` → only the comment |
| G5 | Medium | Reliability / UX | **Solver failures are invisible in the GUI.** `Simulator` and `FrameSimulator` print "factorization failed (mechanism…)" to `std::cerr`. A GUI user who builds an under-constrained model sees nothing happen — the deformed shape just doesn't update. The 2026-05-30 plan's "surface errors to the user" item is only partly done (there is a model-check panel, but the solve path does not feed it). | `src/physics/Simulator.cpp` / `FrameSimulator.cpp` `std::cerr` on `solver.info()!=Success` |
| G6 | Medium | Repo hygiene | **Build artifacts are committed to git.** 28 files under `build_win/` — including `C_Structures.exe`, `SDL2.dll`, `glew32.dll`, `libimgui_lib.a`, and `*.obj` — are tracked even though `build_win/` is git-ignored (they were added before the ignore rule). This bloats the repo, ships platform binaries, and makes clones heavy. | `git ls-files \| grep build_win` → 28 files |
| G7 | Medium | UX (rendering) | **Deformed-shape scale is a fixed magic number** `dispScale = 50.0f`. Real steel-member displacements are ~1e-4 m, so ×50 is still sub-pixel (deflection looks like nothing); for a soft/large model the same factor can blow the shape off-screen. The force *diagrams* auto-scale (`0.25·avgL/maxAbs`) but the deformed shape does not. | `src/main.cpp:438` `float dispScale = 50.0f`; auto-scale only in `drawForceDiagrams` |
| G8 | Low–Med | Physics (joints) | **Joint rotational semantics are coarse.** `FrameSimulator::isDofConstrained` restrains rotations only for `FIXED`; every other support releases *all* rotations, and there is no per-member internal-hinge / partial release. For a rigid multi-member joint that also has a roller/pin support, the rotational restraint is over-simplified. | `src/physics/FrameSimulator.cpp` `isDofConstrained` |
| G9 | Low | Docs drift | **README and architecture.md describe things that don't exist.** README still leads with a "RESTful endpoints" layer (no API exists), names Eigen's `ConjugateGradient` (the solver is now `SparseLU` with static condensation), and frames the project as a 3-week Trello MVP. `architecture.md` still describes the UI as a hand-rolled "orthographic OpenGL" toolbar though the build uses **Dear ImGui**. | `README.md` API/ER/Trello sections; `architecture.md` UI table |
| G10 | Low | Build hygiene | **Stale, contradictory `tests/CMakeLists.txt`.** It declares its own `add_executable(tests test_main.cpp # Create this later)` linking only GTest — contradicting the real, complete `tests` target in the root `CMakeLists.txt`. It is currently dead (root never calls `add_subdirectory(tests)`), but it will collide the moment anyone does, and it misleads contributors. | `tests/CMakeLists.txt` vs root `CMakeLists.txt` test target |
| G11 | Low | Dead code | **`RendererUtils` is still compiled no-op stubs.** `drawArrow/drawCylinder/drawCone` are empty `(void)`-casts, kept "so existing call sites compile", and the file is still in `CMakeLists.txt`. The 2026-05-30 review already recommended removing it. | `src/visualization/RendererUtils.cpp` |
| G12 | Low | Rules compliance | **Component folders have no `README.md`.** `rules.md` requires each major component folder to carry one; `model/ physics/ graphics/ visualization/ ui/ data/` are all missing it. | `ls */README.md` → all missing |

---

## 3. Fix plan (prioritised, sequenced)

The order is chosen so that *process and honesty* are restored first (cheap, unblocks everything and makes every later fix verifiable through CI), then the *wrong-output* physics bugs, then UX and hygiene.

### Tier A — restore the safety net (½ day, do first)

**A1 — Re-enable CI (fixes G1).** In `.gitignore`, replace the blanket `.github/` ignore with narrower rules that keep the workflows tracked. `git add .github/workflows/*.yml`, commit, and confirm a green run on push. *Done when:* a pushed commit shows an Actions run that builds and runs `ctest`.

**A2 — Make CI actually run the tests (supports G1).** Ensure the workflow installs the test deps and runs `ctest --output-on-failure`. The sandbox proved the test target builds without SDL2/OpenGL (only Eigen/glm/GTest), so the CI test job can be lightweight and need not build the GUI. *Done when:* CI fails if any unit test fails.

**A3 — Purge committed build artifacts (fixes G6).** `git rm -r --cached build_win build` and commit; the existing `.gitignore` already excludes them going forward. *Done when:* `git ls-files | grep -E 'build|\.exe|\.dll|\.obj'` is empty.

### Tier B — fix wrong or misleading output (2–3 days)

**B1 — Correct internal-force diagrams under span loads (fixes G2).** Extend `MemberForces::memberInternalAt` to take the member's distributed load and superpose the span term onto the end-force solution: for a UDL `w`, `V(x) = V_end + w·x` and `M(x) = M_end + V·x + ½w·x²` (parabolic). Route the member's `DistributedLoad` (already known to `FrameSimulator`) into `sampleMember`. *Done when:* a new test reproduces the textbook simply-supported-UDL diagram — max moment `wL²/8` at mid-span, zero shear at mid-span — within tolerance.

**B2 — Make determinacy frame-aware (fixes G3).** When frame mode is active, compute determinacy with `dof = 6·nodes` and count rotational restraints; otherwise keep the truss formula. Simplest robust route: branch the DOF count on the selected solver, or report stability directly from whether `Kff` factorises. *Done when:* a propped cantilever (frame) and a simple truss are each classified correctly, with a test for each.

**B3 — Surface solver failures in the UI (fixes G5).** Have `solve()`/`solveStaticForces()` return a status (enum or bool + message) instead of only printing; in `main.cpp`, push that into the existing model-check/status panel ("Structure is a mechanism — add supports"). *Done when:* loading an under-constrained model shows an on-screen message, not a silent no-op.

**B4 — Auto-scale the deformed shape (fixes G7).** Replace the fixed `dispScale = 50` default with an auto factor (target max nodal displacement ≈ a fixed fraction of average member length, as `drawForceDiagrams` already does), keeping the slider as a manual override multiplied on top. *Done when:* both a stiff steel truss and a soft timber model show a visible, sensibly-sized deflected shape with the slider at its default.

### Tier C — fill the modelling gaps (3–5 days, optional for MVP)

**C1 — Concentrated nodal moments (fixes G4).** Add `appliedMoment` + `applyMoment()` to `Node`, populate the three moment DOFs in `FrameSimulator::populateForces`, add a UI control + CSV field. *Done when:* a cantilever with a tip moment `M` reproduces `θ = ML/EI` in a test.

**C2 — Per-joint rotational releases (fixes G8).** Introduce an explicit member-end release / internal-hinge flag and wire the existing `internal_hinge` / `rigid` icons to it, rather than inferring rotation restraint from support type alone. *Done when:* a portal frame with a hinged base matches the textbook result.

### Tier D — hygiene & docs (½ day, batch anytime)

**D1 — Reconcile docs (fixes G9):** update README (drop the API/ER/Trello fiction, name the real `SparseLU` + frame solver, label truss-vs-frame modes) and `architecture.md` (Dear ImGui, the new physics modules).
**D2 — Delete the stale test stub (fixes G10):** remove `tests/CMakeLists.txt` (or make it the real target and have root `add_subdirectory`).
**D3 — Remove `RendererUtils` (fixes G11):** delete the file, its header, the call sites, and the `CMakeLists` entry.
**D4 — Add the six component READMEs (fixes G12)** per `rules.md`.

---

## 4. One-line summary

The engine that has tests is correct; the current glitches live in the **plumbing** (CI is silently off, binaries are in git), in **one real physics-output bug** (diagrams are wrong under distributed load), and in **honesty/UX gaps** (silent solver failures, fixed deflection scale, stale docs). Tier A is half a day and restores the safety net; Tier B is the part that changes what a user actually sees.
