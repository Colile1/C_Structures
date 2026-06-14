# wasm/ — WebAssembly build of the solver core

Compiles the pure C_Structures solver core — `Simulator` (pin-jointed truss),
`FrameSimulator` + `FrameElement` (rigid-jointed frame), with `MemberForces` and
`DistributedLoad` — to WebAssembly via Emscripten. The same C++ that runs natively
runs in a browser or under Node, so a web front-end can analyse models with no
server. **No physics lives here**: `SolverBindings.cpp` is a thin Embind facade
over the existing classes (Build step A20).

The SDL/OpenGL/ImGui app, CSV/JSON I/O, and GoogleTest are intentionally excluded
— this target is the deterministic numerical core only.

## Files

| File | Purpose |
|------|---------|
| `SolverBindings.cpp` | Embind facade: a `Model` class you build up, solve, and read back. |
| `CMakeLists.txt` | Standalone Emscripten project; fetches header-only Eigen + glm. |
| `harness.mjs` | Node harness: runs the reference cases and asserts vs closed-form (CI gate). |
| `harness.html` | Browser harness: same cases, shown as PASS/FAIL rows. |

## Build

Requires the [Emscripten SDK](https://emscripten.org/docs/getting_started/downloads.html).
Activate it so `emcc`/`emcmake` are on PATH (`source ./emsdk/emsdk_env.sh`, or
`emsdk_env.bat` on Windows `cmd`, or `.\emsdk\emsdk_env.ps1` in PowerShell):

```bash
emcmake cmake -S wasm -B build-wasm -DCMAKE_BUILD_TYPE=Release
cmake --build build-wasm
```

This produces `build-wasm/solver_core.js` (the loader/glue) and
`build-wasm/solver_core.wasm` (the compiled core). The first configure fetches
Eigen 3.4.0 and glm 1.0.1 and takes a couple of minutes; later builds are fast.

## Verify

**Node** (used in CI):

```bash
node wasm/harness.mjs build-wasm/solver_core.js
```

It builds the single-bar truss (`δ = F·L/AE`, member force `= F`) and the
tip-loaded cantilever (`vy = −PL³/3EI`, slope `= −PL²/2EI`, base reaction `P` and
moment `PL`, plus the bending-moment diagram), and asserts each within a 1e-3
relative tolerance of the closed-form / native value. Exit code is non-zero on
any failure.

**Browser:** copy `harness.html` next to the build output (or serve the repo) and
open it over HTTP — `solver_core.wasm` won't load from a `file://` URL:

```bash
cp wasm/harness.html build-wasm/ && (cd build-wasm && python -m http.server)
# then open http://localhost:8000/harness.html
```

## JS API (`Module.Model`)

`createSolverModule()` returns a promise resolving to the module. All units are
SI; nodes and beams are referenced by the integer index returned at creation.

```js
const Module = await createSolverModule();
const m = new Module.Model();
const n0 = m.addNode(0, 0, 0, Module.JointType.FIXED);
const n1 = m.addNode(2, 0, 0, Module.JointType.FREE);
const b  = m.addBeam(n0, n1, /*E*/200e9, /*A*/1e-2, /*I*/1e-5);
m.applyForce(n1, 0, -1000, 0);          // N
const status = m.solveFrame();          // Module.SolveStatus.OK | MECHANISM | FAILED
if (status !== Module.SolveStatus.OK) console.warn(m.lastMessage());
const tip = m.displacement(n1);         // {x, y, z} in metres
const diag = m.memberDiagram(b, 21);    // sampled internal forces along the member
for (let i = 0; i < diag.size(); i++) console.log(diag.get(i).Mz);
diag.delete();                          // free Embind vectors and models when done
m.delete();
```

| Method | Notes |
|--------|-------|
| `addNode(x, y, z, jointType)` | `jointType` ∈ `Module.JointType` (`FREE`, `FIXED`, `PIN_XY`, `ROLLER_X/Y/Z`). Returns the node index. |
| `applyForce(nodeIdx, fx, fy, fz)` / `applyMoment(nodeIdx, mx, my, mz)` | Concentrated nodal load (N) / moment (N·m). Moments act in frame solves only. |
| `addBeam(startIdx, endIdx, E, A, I)` | Returns the beam index. |
| `setMomentRelease(beamIdx, atStart, atEnd)` | Internal hinges (frame). |
| `addDistributedLoad(beamIdx, type, dx, dy, dz, w, w2, pos)` | `type` ∈ `Module.LoadType` (`UDL`, `TRIANGULAR`, `MOMENT`); `(dx,dy,dz)` global direction; `w`/`w2` intensity (N/m), `pos` fractional position for `MOMENT`. Frame solves only. |
| `setSelfWeight(on)` | Adds each member's ρ·A·g UDL (frame). |
| `solveTruss()` / `solveFrame()` | Returns a `Module.SolveStatus`; `lastMessage()` carries any error text. |
| `displacement(i)` / `rotation(i)` | Translation (m) / rotation (rad) as `{x,y,z}`. Rotation is frame-only. |
| `reactionForce(i)` / `reactionMoment(i)` | Support reactions (N, N·m). Moment is frame-only. |
| `beamAxialForce(i)` | Signed axial force (tension +), truss. |
| `memberDiagram(i, nSamples)` | Vector of `{N,Vy,Vz,T,My,Mz}` stations (local frame), frame. Call `.delete()` on the returned vector. |

`Model` and any returned vector hold C++ memory — call `.delete()` when finished
to avoid leaks (standard Embind ownership).
