# C_Structures — Project Review

Author of review: prepared for Colile Sibanda
Date: 2026-05-30
Scope: full codebase, architecture, physics correctness, reliability, build/test/CI, documentation, and usability for technical and non-technical users.

---

## 1. Verdict at a glance

C_Structures is a genuinely solid MVP. In the last two days the project went from "does not compile" to a working real-time 3D viewer with an Eigen-based solver, undo/redo, material presets, six joint types, and a tested CSV pipeline. The architecture is cleanly layered and the development log is exemplary.

The single most important finding is a **gap between what the project promises and what the engine computes.** The README and user stories promise bending moments and shear forces; the solver is a 3-DOF-per-node **axial truss/bar model** that computes only tension and compression. The stored moment of inertia `I` is never used. Everything else flows from this: the joint-type system, the load types implied by the new icon set, and the educational pitch all assume a frame model that does not exist yet.

The good news: the truss math that *is* implemented is **correct** (verified below), and the upgrade path to a real frame element is well understood and the natural next step.

| Dimension | Rating | One-line summary |
|-----------|:------:|------------------|
| Architecture | Strong | Clean layered separation, good docs of intent. |
| Physics correctness (as a truss) | Strong | Method independently verified against closed-form solutions. |
| Physics scope vs. promises | Weak | Bending/shear/moment promised but not implemented. |
| Reliability / memory safety | At risk | Raw `Node*` pointers can dangle on vector reallocation. |
| Testing | Good (core) / Gap (UI+app) | Model/physics/CSV well tested; no end-to-end app coverage. |
| Documentation accuracy | Mixed | Excellent log; README/architecture describe features that differ from the code. |
| Non-technical usability | Weak | Requires understanding of DOFs, E, A, I; no plain-language layer. |

---

## 2. What is working well

The layered architecture (`main` → UI/Camera → rendering → physics → model/IO) is real, not aspirational: layers communicate downward only, and the `architecture.md` "how to add a feature" section is the kind of thing most solo projects never write. The development log is the project's strongest asset — every entry records what, why, and impact, which is exactly what `rules.md` asks for and makes the project auditable.

The solver rewrite (2026-05-30 20:50) replacing penalty boundary conditions and ConjugateGradient with **static condensation + SparseLU** was the right call. Penalty BCs mix diagonal magnitudes of 1 and ~1e7, wrecking conditioning; extracting the free-DOF sub-matrix `K_FF` and solving it directly is the textbook-correct approach and eliminated the divergence. The recent additions — joint-type enum, material presets, snapshot undo/redo, Font Awesome icons — are well-scoped and logged.

The test suite covers the parts that matter most for correctness: model behavior, the physics solver, CSV round-trips, and integration of the three. The `requirements.sh` / `start.sh` scripts and the GitHub Actions Linux workflow show the deployment story was taken seriously.

## 3. Physics correctness

**The implemented method is correct.** I re-implemented the exact algorithm from `Simulator.cpp` (3 DOF/node, element stiffness `K_e = (AE/L)·t·tᵀ` where `t` is the unit axis, static condensation of fixed DOFs, member force `= (AE/L)·(u_J − u_I)·t̂`) independently in NumPy and tested it:

- **Single steel bar** (L=2 m, A=1×10⁻⁴ m², E=200 GPa, F=1 kN axial): computed elongation = 1.000×10⁻⁴ m, matching the closed-form `F·L/(A·E)` to full precision; member force = +1000 N (tension). ✔
- **Symmetric two-bar truss**: both members return identical axial force, as symmetry requires. ✔

So the numbers the app produces for axial/truss problems can be trusted. (Note: the actual C++ could not be compiled inside the review sandbox because SDL2/OpenGL/Eigen/glm are not installed there; the verification mirrors the algorithm, and I recommend porting these two checks into `PhysicsTests.cpp` as permanent regression tests — see the plan.)

**The scope problem.** Because each node has only 3 *translational* DOFs and no rotational DOFs:

- **No bending moments, no shear, no deflected-shape curvature.** The README ("Learn shear forces and bending moments interactively") and the first user story ("see color-coded tension/compression results so I can visualize bending moments") describe behavior the engine cannot produce. A truss member carries axial force only.
- **`Beam::momentOfInertia (I)` is dead data** — stored, shown in the UI, saved to CSV, but never read by the solver. This will mislead users into thinking section choice affects bending results.
- **Joint semantics are conceptually muddled.** In a pure truss every connection is a pin by definition, so `FIXED` vs `PIN_XY` make no physical difference to the result (both just constrain translations). The six-way `JointType` enum implies frame behavior (rotational restraint) that the math does not model.
- **Loads are nodal point forces only.** The new icon set (UDL, triangular load, moment, self-weight) advertises load types the engine cannot apply. These need either a real frame solver or a load-to-node consistent-load conversion.
- **Reactions are not computed.** There is no `getNodeReaction()`, so support reactions (a basic teaching output, and one of the new force icons) cannot be displayed.

## 4. Reliability and safety

**Raw `Node*` pointers are the top risk.** `Beam` stores `Node*`, and `Simulator` recovers node indices by pointer arithmetic (`beam.getStart() - &nodes[0]`). This is only valid while the `std::vector<Node>` is never reallocated. `architecture.md` documents the hazard, but the UI lets users **add nodes interactively at any time**, and a `push_back`/`emplace_back` that grows the vector will invalidate every beam pointer → undefined behavior, wrong indices, or a crash. This is a latent bug waiting for a user to place "one node too many." The robust fix is to store **integer node indices** in `Beam` instead of pointers.

Secondary reliability items:

- **CSV save uses floating-point equality** to find node indices by scanning positions; any position edit before save breaks the mapping. Storing/using indices removes this too.
- **No structure validation before solve.** Under-constrained mechanisms are only caught when SparseLU fails; the message goes to `std::cerr` (invisible in a GUI app). There is no determinacy/stability pre-check and no on-screen feedback.
- **Error handling is minimal** — solver failures, malformed CSV, and missing shader files mostly print to stderr or fail silently. A GUI user sees nothing.
- **Single-precision force pipeline.** Node forces and positions are `float`; the solver promotes to `double`. Fine for an MVP but worth noting for large/stiff models.

## 5. Code quality vs. your own rules

`rules.md` requires each major component folder to contain a `README.md` explaining its purpose. The code is nicely modularized into `model/`, `physics/`, `graphics/`, `visualization/`, `ui/`, `data/`, but **none of these folders has a README** — a straightforward compliance gap. `RendererUtils` is described in the architecture as "legacy stubs / no-ops" and should be removed rather than carried. Otherwise file sizes and single-responsibility are respected, and naming is consistent.

## 6. Documentation accuracy

The log is excellent; the *design* docs have drifted from the code:

- **UI technology mismatch.** `architecture.md` describes a hand-rolled orthographic-OpenGL toolbar, but `main.cpp` and the build use **Dear ImGui** (`imgui_impl_sdl2`/`imgui_impl_opengl3`). The architecture doc should be updated to match.
- **README describes things that don't exist:** a "RESTful endpoints / API & Communication" layer (there is none — it's a desktop app), and an Entity Relationship Diagram section that is empty.
- **README feature claims** (bending/shear) overstate the engine, as above.

These are easy wins and matter because the README is the project's shop window.

## 7. Usability — technical vs. non-technical users

For a **technical** user (engineering student), the tool is already useful for truss intuition once they learn the controls. For a **non-technical** user it is currently hard: the UI speaks in DOFs, Young's modulus (GPa), cross-section (cm²), second moment of area (cm⁴), and six joint types with no explanation, no tooltips, no legend, no guided templates surfaced in the UI, and no plain-language interpretation of results. There is no on-screen help, no units toggle, and no "what am I looking at" layer.

This is exactly the gap the **new icon library** is built to close: every joint, section, and force now has a traditional engineering symbol *and* a shaded realistic 2D/3D picture, so a palette and property panel can show a beginner a recognizable picture of a roller or an I-beam instead of the word "ROLLER_X." The icons are necessary but not sufficient — they need to be wired into a friendlier UI (see the plan).

## 8. How C_Structures compares (context for the plan)

The teaching-tool landscape points clearly at the upgrade path. **FTool** is the long-standing gold standard for teaching plane-frame behavior through fast, interactive modelling. **MASTAN2** offers full pre/analysis/post in a deliberately simple package. **SkyCiv's** education angle is its **step-by-step hand calculations** (method of joints/sections) — a "glass-box" that shows the math, echoed by a 2026 web-based direct-stiffness teaching tool and a Streamlit "Professional Truss Suite" that deliberately expose the element matrices and global assembly. **Frame3DD** is the open-source reference for exactly the engine C_Structures should grow into: static 2D/3D **frames** computing deflections, **reactions, internal element forces**, and mode shapes.

The lesson: C_Structures' real-time 3D interaction is already a differentiator FTool and Frame3DD lack; its biggest opportunities are (a) becoming a true frame solver and (b) leaning into the "glass-box" educational niche — show the stiffness matrix and the hand-calc, which no real-time 3D tool does well.

## 9. Prioritized findings

1. **(Correctness/expectations)** Engine is axial-truss only; bending/shear/moment and `I` are promised/stored but unused. Either deliver a frame element or correct the claims.
2. **(Safety)** Replace raw `Node*` in `Beam` with integer indices to remove the reallocation UB and the CSV float-equality bug at once.
3. **(Reliability)** Add pre-solve structure validation + on-screen error/status feedback (no more silent `cerr`).
4. **(Usability)** Wire the new icon library into a beginner-friendly palette/property panel with tooltips, a legend, units, and plain-language result readouts.
5. **(Correctness output)** Implement support reactions and a clearer multi-stop stress color ramp.
6. **(Docs)** Reconcile `architecture.md`/README with the actual ImGui UI and truss scope; remove the phantom REST/API and empty ER sections.
7. **(Process)** Add per-folder READMEs (rules.md), port the two verification benchmarks into the test suite, remove `RendererUtils` no-ops.

Full sequencing, effort, and acceptance criteria are in `IMPROVEMENT_PLAN.md`.

---

### Sources
- [Frame3DD — static/dynamic analysis of 2D/3D frames (open source reference)](https://frame3dd.sourceforge.net/)
- [SkyCiv Truss & Frame — step-by-step hand calculations for education](https://skyciv.com/structural-software/truss-and-frame/)
- [MASTAN2 — interactive teaching analysis program](https://www.thestructuralengineer.info/software/mastan2)
- [Development of an Interactive Web-Based Tool for 2D Truss Analysis Using the Direct Stiffness Method (Wiley, 2026)](https://onlinelibrary.wiley.com/doi/10.1002/cae.70183?af=R)
- [Direct stiffness method — Wikipedia (3D frame = 6 DOF/node, 12-DOF element)](https://en.wikipedia.org/wiki/Direct_stiffness_method)
