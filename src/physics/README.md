# physics/

Pure, deterministic solvers and analysis routines. No SDL/OpenGL — the entire folder is unit-testable headlessly.

| File | Purpose |
|------|---------|
| `Simulator.cpp` | Pin-jointed truss solver: 3 DOF/node, Eigen `SparseLU`, support reactions. |
| `FrameElement.cpp` | 6-DOF beam-column element stiffness, coordinate transformation, and member-end release condensation (internal hinges). |
| `FrameSimulator.cpp` | Rigid-jointed frame solver: assembles global stiffness from `FrameElement`, applies BCs and member-end moment releases via static condensation, returns nodal displacements and reactions. |
| `MemberForces.cpp` | Reconstructs internal axial/shear/moment/torsion diagrams along each member from end forces and span loads. |
| `DistributedLoad.cpp` | Converts a uniform or linearly-varying span load into consistent equivalent nodal loads for assembly. |
| `Determinacy.cpp` | Classifies a model as statically determinate, indeterminate, or a mechanism; frame-aware (6 DOF/node) vs. truss (3 DOF/node). |

**Correctness-critical:** `FrameElement`, `FrameSimulator`, `MemberForces`, `DistributedLoad`, and `Simulator` are all marked ⚑ in the implementation plan — subtle sign or DOF errors here silently invalidate results.
