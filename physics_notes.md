# C_Structures — Physics Notes

## What the Simulator Does

The simulator solves **static structural equilibrium** using the **Direct Stiffness Method** (a form of the Finite Element Method). Given a set of nodes, beams, fixed supports, and applied forces, it finds how much each node displaces so that the structure is in balance.

---

## The Core Equation

```
K · u = F
```

| Symbol | Meaning | In code |
|--------|---------|---------|
| K | Global stiffness matrix (n×n, sparse) | `m_globalK` |
| u | Displacement vector (unknown) | `m_displacements` |
| F | Force vector (applied loads) | `m_forces` |
| n | 3 × number of nodes (3 DOFs per node: x, y, z) | `3 * nodes.size()` |

---

## Beam Stiffness

Each beam contributes a stiffness value:

```
k = A·E / L
```

| Symbol | Meaning | Where it comes from |
|--------|---------|---------------------|
| A | Cross-section area (m²) | `Beam::getCrossSection()` |
| E | Young's modulus (Pa) | `Beam::getYoungsModulus()` |
| L | Beam length (m) | `Beam::getLength()` — computed from node positions |

A **stiffer** beam (larger E or A, or shorter length) resists deformation more. Steel has E ≈ 200 GPa (2×10¹¹ Pa). Concrete ≈ 30 GPa.

---

## Assembling the Global Stiffness Matrix

For each beam connecting node `i` to node `j`, the simulator adds a 3×3 identity block scaled by `k` to the global matrix:

```
K[3i:3i+3, 3i:3i+3] += k · I₃
K[3j:3j+3, 3j:3j+3] += k · I₃
K[3i:3i+3, 3j:3j+3] -= k · I₃
K[3j:3j+3, 3i:3i+3] -= k · I₃
```

This is a simplified **axial-only** element — it resists extension and compression along all three axes equally (isotopic). A full 3D frame element would also resist bending and torsion.

---

## Boundary Conditions (Fixed Supports)

Nodes marked `isFixed() == true` cannot move. To enforce this, the solver:

1. Replaces the rows and columns for that node's DOFs with identity rows
2. Sets the corresponding force entries to zero

This ensures the solver returns `u = 0` for fixed DOFs.

---

## Solving

The solver uses Eigen's **Conjugate Gradient** iterative solver, which is efficient for large sparse symmetric positive-definite systems. For small structures (< ~1000 nodes) a direct solver (LU decomposition) would also work.

---

## Reading the Results

### Node Displacements

```cpp
auto disp = simulator.getNodeDisplacements();
// disp[i] is a glm::vec3 — how much node i moved in x, y, z (metres)
```

For a 1m steel bar (E=200GPa, A=0.01m²) with a 10kN axial load:

```
u = F·L / (A·E) = 10,000 × 1 / (0.01 × 200,000,000,000) = 0.000005 m = 5 µm
```

This is very small — real structural displacements are tiny for steel under normal loads.

### Beam Forces

```cpp
float force = simulator.getBeamForce(beam);
// positive = tension (beam being pulled apart)
// negative = compression (beam being pushed together)
```

The force is calculated as:

```
F_axial = k × (u_end - u_start) · unit_axis
```

where `unit_axis` is the normalised direction from beam start to end.

---

## Colour Mapping

| Force | Colour | Meaning |
|-------|--------|---------|
| Positive (tension) | Blue | Beam is being stretched |
| Near zero | Grey | Beam carries almost no load |
| Negative (compression) | Red | Beam is being compressed |

The colour interpolates linearly between grey and full blue/red based on `force / MAX_STRESS` (default MAX_STRESS = 1 MPa).

---

## Limitations of the Current Model

1. **Axial elements only** — beams can only push and pull along their axis. Real beams also resist bending. This is a truss model, not a full frame model.
2. **Linear elastic only** — the model assumes small displacements and linear material behaviour. It does not model buckling, yielding, or fracture.
3. **Static only** — the solver finds the equilibrium state. It does not simulate how the structure deforms over time (no dynamics, no vibration analysis).
4. **Isotropic stiffness blocks** — the 3×3 identity scaling is a simplification. A proper axial element should project stiffness along the beam's local axis using the direction cosines.

---

## Example: Simple Cantilever

```
Fixed wall           Free end
[Node 0] ─── Beam ─── [Node 1] ← 5000 N
```

- Node 0: fixed
- Node 1: free, 5000 N applied in X direction
- Beam: E = 2×10¹¹ Pa, A = 0.01 m², L = 2 m

```
k = A·E/L = 0.01 × 2e11 / 2 = 1,000,000,000 N/m

u₁.x = F / k = 5000 / 1e9 = 5×10⁻⁶ m (5 micrometres)

Beam force = k × u₁.x = 1e9 × 5e-6 = 5000 N (tension, positive)
```

This is what `IntegrationTests.cpp → SingleBeamAxialDisplacement` verifies.
