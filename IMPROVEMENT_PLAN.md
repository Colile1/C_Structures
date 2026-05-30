# C_Structures — Improvement Plan

Date: 2026-05-30
Companion to: `REVIEW.md`
Goal: make C_Structures correct, reliable, and genuinely usable by both engineering students and non-technical users, without abandoning the real-time 3D interaction that differentiates it.

This plan is sequenced so that **safety and honesty come before new features**: fix the memory hazard and align the claims, then deepen the physics, then build the friendly UI layer on top of the new icon library.

---

## Guiding principles

1. **Don't promise physics you don't compute.** Every feature claim in the README must be backed by the engine, or be clearly labelled "planned."
2. **Two audiences, one model.** A single scene serves both a "Beginner" mode (pictures, plain language, guided templates) and an "Engineer" mode (matrices, numbers, hand-calcs). The new icon set's three view modes (symbol / realistic-2D / realistic-3D) are the visual backbone of Beginner mode.
3. **Glass-box where competitors are black boxes.** Lean into showing the stiffness matrix and step-by-step solve — the niche FTool/Frame3DD/SkyCiv have proven students want.
4. **Test every correctness claim.** A physics result without a regression test is a future bug.

---

## Phase 0 — Stabilise and tell the truth (≈2–3 days)

Low effort, high value. Do these first; nothing below is safe to build on until they are done.

**0.1 Replace `Node*` with integer indices in `Beam`** *(safety, critical)*
Store `int startIdx, endIdx` instead of `Node*`. Update `Beam`, `Simulator` (drop the pointer-arithmetic index recovery), `CSVHandler`, and the UI. This single change removes the vector-reallocation undefined behavior **and** the CSV float-equality save bug. Acceptance: adding 200 nodes then a beam, in any order, never corrupts connectivity; existing tests still pass.

**0.2 Reconcile documentation with reality** *(honesty)*
In README: relabel "bending moments / shear" as **planned (Phase 2)**; state plainly that the current engine is a **3D axial truss/spring solver**; delete the non-existent "RESTful API" layer and the empty ER-diagram section. In `architecture.md`: replace the custom-OpenGL-toolbar description with the actual **Dear ImGui** UI. Acceptance: a new reader can predict the app's behavior from the docs.

**0.3 Surface errors to the user** *(reliability)*
Route solver/CSV/shader failures to an on-screen status bar / toast instead of `std::cerr`. Acceptance: an under-constrained model shows "Structure is a mechanism — add supports," not a silent no-op.

**0.4 Housekeeping** *(rules compliance)*
Add a short `README.md` to each component folder (`model/`, `physics/`, `graphics/`, `visualization/`, `ui/`, `data/`) per `rules.md`. Remove the `RendererUtils` no-op stubs. Port the two `REVIEW.md` verification benchmarks (single bar `F·L/AE`; symmetric truss) into `PhysicsTests.cpp`.

---

## Phase 1 — Correct, complete the truss (≈1 week)

Make the *current* model fully trustworthy and teachable before changing its order.

**1.1 Support reactions.** Implement `Simulator::getNodeReaction(i)` = `(K·u − F)` at constrained DOFs. Display reaction arrows (the `reaction` icon already exists) and a reactions table. Add an equilibrium self-check (Σreactions + Σloads ≈ 0) shown to the user as a green tick — instant credibility.

**1.2 Determinacy / stability pre-check.** Before solving, report static determinacy (`m + r vs 2n`/`3n`) and detect obvious mechanisms; warn in plain language. Acceptance: classic stable/unstable textbook trusses are classified correctly.

**1.3 Better result visualisation.** Multi-stop stress color ramp (blue→white→red) with a numeric legend; a displacement-scale slider (already partly present) with an "× factor" label; per-member force labels toggle. Acceptance: a user can read the largest tension/compression member at a glance.

**1.4 3D nodal loads in the UI.** Expose Fx/Fy/Fz numeric entry (not just the −Y default) so the truss can actually be loaded in 3D.

---

## Phase 2 — Become a real frame solver (≈2–3 weeks, the big one)

This is what makes the bending/shear/moment promise real and lets `I`, the moment icon, UDLs, and the six joint types finally mean something. It is the natural convergence with Frame3DD's capability while keeping the real-time 3D UI.

**2.1 6-DOF-per-node 3D frame element (12-DOF element).** Add 3 rotational DOFs per node. Build the standard 12×12 local frame stiffness matrix (axial + torsion + bending about two axes using `E, A, I` and `G, J`), transform to global with the member rotation matrix, assemble, solve. Keep the truss solver available as a "pin-jointed" mode toggle so the simpler teaching case is still there. Reference: direct-stiffness 3D frame formulation (see sources in `REVIEW.md`).

**2.2 Element internal forces.** Compute and plot axial, shear, bending-moment, and torsion **diagrams** along each member — the headline teaching output. The `i_beam`/`channel`/etc. section icons now genuinely affect results via `I`.

**2.3 Distributed & moment loads.** Implement UDL, triangular, and applied-moment loads via consistent equivalent nodal loads. The UDL / triangular / moment icons become functional. Add self-weight from material density × section area.

**2.4 Now the joint types are meaningful.** `FIXED` restrains rotations, `PIN`/internal-hinge release them, rollers free a translation — wire each `JointType` (and the new `internal_hinge` / `rigid` icons) to the correct DOF releases. Validate against FTool/textbook cantilever and portal-frame results.

Acceptance for Phase 2: a propped cantilever with a UDL reproduces textbook reactions, max moment (`wL²/...`), and deflection within a small tolerance, as an automated test.

---

## Phase 3 — Dual-audience UX, built on the icon library (≈1–2 weeks, parallel to Phase 2)

**3.1 Mode switch: "Beginner" vs "Engineer."** One toggle changes vocabulary and density. Beginner hides DOFs/E/I behind friendly choices; Engineer exposes everything.

**3.2 Visual palette from the new icons.** Replace text labels with the icon library: a left palette of joints / sections / loads showing each component's icon, with a per-user **view-mode preference** (symbol / realistic-2D / realistic-3D) exactly as the `icons/index.html` toggle demonstrates. Property panels show the chosen component's picture, not just its enum name. (Rasterise SVGs to ImGui textures at 32/48 px, or render the SVG in any HTML side-panel.)

**3.3 Plain-language everywhere.** Tooltips on every control ("A roller lets the beam slide this way but holds it down"); a results panel that says "This member is being stretched (tension), 4.2 kN" alongside the number; units shown and a metric/imperial toggle.

**3.4 Guided templates in the UI.** Surface the prebuilt CSV templates (simple beam, triangle truss, portal frame) as one-click "Start from an example" cards with thumbnails — the fastest way for any user to see a correct result immediately.

**3.5 Glass-box panel (the differentiator).** An optional "Show the math" panel that displays the global stiffness matrix, the free-DOF reduction, and the solve — turning the tool into the real-time 3D answer to SkyCiv's step-by-step hand calculations.

---

## Phase 4 — Robustness, reach, polish (ongoing)

- **Save/Load from the UI** (file dialogs) and a richer project format (JSON) capturing joint types, materials, loads, and view-mode preference — CSV stays as the interchange format.
- **Export**: PNG screenshot, and a one-page PDF results report (reactions, member forces, diagrams) for assignments.
- **CI hardening**: run `ctest` on every push (already scaffolded), add an ASan/Valgrind job to catch exactly the pointer class of bug from Phase 0.
- **Performance**: only re-solve on change (not every Enter), cache factorisation, LOD for large models — matches the original latency-mitigation plan.
- **Accessibility**: colour-blind-safe force ramp (don't rely on red/blue alone — add +/− labels), keyboard navigation.
- **Web reach (stretch)**: the solver core is plain C++/Eigen; a WebAssembly build would put C_Structures in the browser alongside SkyCiv and the 2026 web tools, with the SVG icon set already web-native.

---

## Suggested order of execution

```
Phase 0  (safety + honesty)         ──►  do immediately, blocks everything
Phase 1  (trustworthy truss)        ──►  next; ships visible value fast
Phase 2  (frame solver)  ┐
Phase 3  (dual-audience) ┘          ──►  run in parallel; 3 consumes the icons, 2 the physics
Phase 4  (polish/reach)             ──►  continuous
```

## Effort & impact summary

| Phase | Effort | Primary payoff | Risk if skipped |
|-------|:------:|----------------|-----------------|
| 0 | Low | Removes crash-class bug; docs honest | Latent UB; misleading README |
| 1 | Low–Med | Reactions + trustworthy, readable truss | Core outputs incomplete |
| 2 | High | Real bending/shear/moment — the actual promise | Product never matches its pitch |
| 3 | Med | Non-technical users can actually use it | Stays expert-only |
| 4 | Med (ongoing) | Reach, robustness, shareable output | Fragile, desktop-only |

The first two phases are small and high-leverage; do them before adding anything else. Phase 2 is the project's defining investment. Phase 3 is where the icon library you just added pays off.
