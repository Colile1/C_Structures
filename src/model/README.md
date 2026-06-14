# model/

Data structures that describe a structural model.

| File | Purpose |
|------|---------|
| `Node.cpp` / `Node.hpp` | A joint: 3-D position, support type (`JointType`), and applied force + moment vectors. |
| `Beam.cpp` / `Beam.hpp` | A member: integer start/end node indices, section/material properties (E, A, I, density), and member-end moment releases (internal hinges). |

**Rules.** Beams reference nodes by integer index — never by pointer. No physics or rendering logic lives here; this layer is pure data so it can be used by both the solver and the test suite without any SDL/OpenGL dependency.
