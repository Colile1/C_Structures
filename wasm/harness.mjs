// Copyright (c) 2026 Colile Sibanda. All rights reserved.
// Proprietary — see LICENSE for terms. Unauthorised use prohibited.
//
// wasm/harness.mjs : Node harness for the WebAssembly solver core. Builds the
// two reference cases (single-bar truss, tip-loaded cantilever frame), solves
// them through the wasm Model, and asserts the results against the closed-form
// values the native tests use. Exits non-zero on any mismatch so CI fails loudly.
//
//   node wasm/harness.mjs build-wasm/solver_core.js

import { pathToFileURL } from 'node:url';
import { resolve } from 'node:path';

const jsPath = process.argv[2] ?? 'build-wasm/solver_core.js';
const mod = await import(pathToFileURL(resolve(jsPath)).href);
const createSolverModule = mod.default;
const Module = await createSolverModule();

let failures = 0;
// Assert |got - want| within max(relTol*|want|, absTol). Logs PASS/FAIL.
function near(name, got, want, relTol = 1e-3, absTol = 1e-9) {
    const tol = Math.max(Math.abs(want) * relTol, absTol);
    const ok = Math.abs(got - want) <= tol;
    if (!ok) failures++;
    console.log(`${ok ? 'PASS' : 'FAIL'}  ${name}: got ${got.toExponential(6)} `
        + `want ${want.toExponential(6)} (tol ${tol.toExponential(2)})`);
}

// ── Case 1: single steel bar in tension (truss) ───────────────────────────────
// Elongation = F·L/(A·E); member force = applied load.
{
    const E = 200e9, A = 1e-4, L = 2.0, F = 1000.0;
    const m = new Module.Model();
    const n0 = m.addNode(0, 0, 0, Module.JointType.FIXED);
    const n1 = m.addNode(L, 0, 0, Module.JointType.FREE);
    const b  = m.addBeam(n0, n1, E, A, 8.33e-9);
    m.applyForce(n1, F, 0, 0);

    const status = m.solveTruss();
    if (status !== Module.SolveStatus.OK) { failures++; console.log('FAIL  truss status', m.lastMessage()); }

    near('single-bar elongation u1.x', m.displacement(n1).x, F * L / (A * E));
    near('single-bar axial force',     m.beamAxialForce(b),  F, 1e-3, 1.0);
    m.delete();
}

// ── Case 2: cantilever with transverse tip load (frame) ───────────────────────
// Tip vy = −PL³/(3EI); tip slope θz = −PL²/(2EI); base reaction P and moment PL.
{
    const E = 200e9, A = 1e-2, I = 1e-5, L = 2.0, P = 1000.0;
    const m = new Module.Model();
    const n0 = m.addNode(0, 0, 0, Module.JointType.FIXED);
    const n1 = m.addNode(L, 0, 0, Module.JointType.FREE);
    const b  = m.addBeam(n0, n1, E, A, I);
    m.applyForce(n1, 0, -P, 0);

    const status = m.solveFrame();
    if (status !== Module.SolveStatus.OK) { failures++; console.log('FAIL  frame status', m.lastMessage()); }

    near('cantilever tip vy',    m.displacement(n1).y, -P * L * L * L / (3 * E * I));
    near('cantilever tip slope', m.rotation(n1).z,     -P * L * L / (2 * E * I));
    near('cantilever reaction Fy', m.reactionForce(n0).y, P, 1e-3, 1e-1);
    near('cantilever reaction |Mz|', Math.abs(m.reactionMoment(n0).z), P * L, 1e-3, 1.0);

    // Internal-force diagram: |Mz| peaks at the fixed base (P·L) and is ~0 at the tip.
    const d = m.memberDiagram(b, 11);
    near('diagram station count', d.size(), 11, 0, 0.5);
    near('diagram |Mz| at base', Math.abs(d.get(0).Mz),         P * L, 1e-2, 1.0);
    near('diagram |Mz| at tip',  Math.abs(d.get(d.size() - 1).Mz), 0,   1e-2, P * L * 1e-2);
    d.delete();
    m.delete();
}

console.log(failures === 0
    ? '\nAll wasm solver checks passed.'
    : `\n${failures} wasm solver check(s) FAILED.`);
process.exit(failures === 0 ? 0 : 1);
