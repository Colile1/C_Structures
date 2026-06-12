# C_Structures — Improvement Plan (June 2026 refresh)

Date: 2026-06-12
Supersedes: the forward-looking sections of `IMPROVEMENT_PLAN.md` (2026-05-30)
Companion to: `GLITCHES_AND_FIX_PLAN.md` (fix-first work) and `IMPLEMENTATION_PLAN.md` (build order)

## Where the project actually is now

The 2026-05-30 plan defined Phases 0–4. Reading the current code, most of Phases 0–2 are **done**: beams use integer indices (memory hazard closed), there is a real 6-DOF frame solver (`FrameSimulator` + `FrameElement`), support reactions with an equilibrium self-check, a determinacy module, member internal-force diagrams, consistent equivalent nodal loads for UDL/triangular/moment, material presets, and a beginner/engineer UI split with results, reactions, model-check, loads, glass-box, and templates panels. The test suite (40 tests) covers and confirms the core math.

So this refresh is **not** "build the frame solver" — that exists. It is about (a) finishing the things that are 80% done, (b) making the existing physics *trustworthy at the diagram level*, and (c) the reach/polish that turns a working solver into a product students and a marker will actually rate highly. The genuine bugs are tracked separately in `GLITCHES_AND_FIX_PLAN.md`; this document is the *improvement* layer that sits on top once those are fixed.

## Guiding principles (unchanged, still right)

1. Don't promise physics you don't compute — every README/UI claim must be backed by the engine or labelled "planned".
2. Two audiences, one model — Beginner mode (pictures, plain language, templates) and Engineer mode (matrices, numbers, hand-calcs) over one scene.
3. Glass-box where competitors are black boxes — show the stiffness matrix and the solve; this is the real differentiator versus SkyCiv/Frame3DD.
4. Every correctness claim gets a regression test.

---

## Improvement themes

### Theme 1 — Make the teaching outputs correct and complete (highest value)

The diagrams are the product's reason to exist, so they must be right and readable.

- **Correct diagrams under span loads** (also tracked as glitch G2): superpose the distributed-load term so UDL shear is linear and moment parabolic. Without this, the BMD/SFD — the single most-looked-at output — is wrong for any member with a UDL.
- **Numeric annotations on diagrams**: peak value and its location (`M_max = 12.5 kN·m @ x = 2.0 m`), end values, and zero-crossings. SkyCiv's appeal is precisely that it labels these; matching it is cheap once the sampling is correct.
- **Diagram per quantity, switchable**: axial / shear-y / shear-z / torsion / moment-y / moment-z, with the convention (tension-side, sagging-positive) stated on screen.
- **Reactions table with the equilibrium tick**: the `checkEquilibrium` result is already computed — show it as a green "ΣF ≈ 0 ✓" badge. Instant credibility for a marker.

### Theme 2 — Close the modelling gaps so the icons don't lie

The icon set advertises load and joint types the engine should fully support.

- **Concentrated nodal moments** (glitch G4): the `moment` icon needs a real input path (`Node::applyMoment`, frame moment DOFs, CSV field).
- **Self-weight load**: density × area × g as an automatic UDL — the `self_weight` icon. Add a density to `BeamMaterial`.
- **Internal hinges / member-end releases** (glitch G8): wire the `internal_hinge` / `rigid` icons to real DOF releases so pin-vs-rigid joints behave differently.
- **3D nodal loads in the UI**: expose Fx/Fy/Fz (and the new Mx/My/Mz) numeric entry, not just the −Y default, so a structure can be loaded in 3D.

### Theme 3 — Trustworthy feedback loop

- **On-screen solver status** (glitch G5): mechanism / singular / converged, never silent.
- **Frame-aware determinacy** (glitch G3): correct DOF count and rotational restraint when in frame mode.
- **Auto-scaled deformed shape** (glitch G7) with an on-screen "×N" label so the exaggeration is honest.
- **Re-solve only on change**: cache the factorisation and re-solve when the model changes rather than on every Enter — matches the original latency-mitigation goal and makes interaction feel live.

### Theme 4 — Dual-audience UX, built on the icon library

- **Visual palette from the SVG icons**: a left palette of joints / sections / loads, each shown with its icon, with the per-user view-mode toggle (symbol / realistic-2D / realistic-3D) exactly as `resources/icons/index.html` already demonstrates. Property panels show the chosen component's picture, not just its enum name. (Rasterise SVGs to ImGui textures at 32/48 px.)
- **Plain-language everywhere**: tooltips ("A roller lets the beam slide this way but holds it down"); results that read "This member is being stretched (tension), 4.2 kN" beside the number; a metric/imperial toggle.
- **Guided templates as cards**: surface the existing templates (simple beam, triangle truss, portal frame) as one-click "Start from an example" cards with thumbnails — the fastest path to a correct first result.
- **Glass-box panel polish**: the panel exists — make it show the global K, the free-DOF reduction, and the solved `u` for the *current* model, so it tracks what the user built.

### Theme 5 — Shareable output (what a marker/teacher wants to keep)

- **One-page PDF results report**: model summary, reactions, member-force table, and the diagrams — directly usable for an assignment hand-in. (The `pdf` skill / a headless render can produce this.)
- **PNG screenshot export** of the 3D view and of each diagram.
- **Richer project format (JSON)** capturing joint types, materials, loads, and view-mode preference; keep CSV as the interchange format.

### Theme 6 — Reach and robustness

- **WebAssembly build (stretch, high-impact).** The solver core is plain C++/Eigen, and Eigen is header-only and **known to compile under Emscripten** (Eigen ≥ 3.3, used this way by Project Chrono and the `eigen-js` port). A Wasm build would put C_Structures in the browser next to SkyCiv, with the SVG icon set already web-native — a strong differentiator and a great portfolio/demo artefact.
- **CI hardening**: once CI is re-enabled (glitch G1), add an AddressSanitizer/Valgrind job to the test build to catch memory bugs early; run `ctest` on every push.
- **Colour-blind-safe force ramp**: don't rely on red/blue alone — add +/− or T/C labels on members and the legend.
- **Performance/LOD** for large models; only the visible members need full geometry.

---

## What comparable tools do (research basis)

SkyCiv's free Beam and Frame calculators are browser-based and lead on exactly two things: (1) automatically generated **reactions, SFD, BMD, deflection** diagrams, and (2) **full hand-calculations** shown step-by-step "whether you're a student learning… or a professional verifying figures" — i.e. the glass-box idea. Frame3DD offers the same static frame analysis but through CLI/text files with no interactive visuals. C_Structures' defensible niche is the intersection SkyCiv and Frame3DD each half-occupy: **real-time, interactive 3D** *plus* the **glass-box hand-calculation** view, in a free desktop (and eventually browser) tool. Themes 1, 4, and 5 above are what put C_Structures on equal footing with SkyCiv's outputs; Theme 3's glass-box and the live 3D are what make it distinct.

Sources:
- [SkyCiv Beam analysis software](https://skyciv.com/structural-software/beam-analysis-software/)
- [SkyCiv free frame calculator](https://skyciv.com/free-frame-calculator/)
- [SkyCiv beam hand calculations](https://skyciv.com/docs/skyciv-beam/hand-calculations/)
- [SkyCiv shear, moment, torsion & axial post-processing](https://skyciv.com/docs/structural-3d/post-processing/shear-moment-axial-torsion/)
- [Eigen-js: Eigen ported to WebAssembly](https://github.com/BertrandBev/eigen-js)
- [Project Chrono — building with Emscripten/Eigen to Wasm](https://api.chrono.projectchrono.org/tutorial_install_chrono_emscripten.html)
- [Emscripten — Building to WebAssembly](https://emscripten.org/docs/compiling/WebAssembly.html)

---

## Effort & impact summary

| Theme | Effort | Primary payoff | Sequence |
|-------|:------:|----------------|----------|
| 1 Correct/complete diagrams | Low–Med | The headline output becomes trustworthy + readable | First (after Tier A/B fixes) |
| 2 Close modelling gaps | Med | Icons stop advertising features that don't work | After Theme 1 |
| 3 Trustworthy feedback | Low | No silent failures; honest, live visuals | Parallel with Theme 1 |
| 4 Dual-audience UX | Med | Non-technical users can actually use it | After Theme 1; consumes icons |
| 5 Shareable output | Med | A deliverable a marker keeps | After Themes 1–2 |
| 6 Reach (Wasm/CI/a11y) | Med–High | Portfolio reach, robustness | Continuous / stretch |

The cheapest high-value work is Theme 1 + Theme 3: make the diagrams correct, labelled, and never-silent. Theme 6's WebAssembly build is the single most impressive stretch goal and is technically de-risked (the solver is already pure Eigen). The concrete, ordered, testable build steps for all of this are in `IMPLEMENTATION_PLAN.md`.
