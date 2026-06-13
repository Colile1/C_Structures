# model/

Data structures that describe a structural model.

| File | Purpose |
|------|---------|
| `Node.cpp` / `Node.hpp` | A joint: 3-D position, support type, applied force vector. |
| `Beam.cpp` / `Beam.hpp` | A member: integer start/end node indices, cross-section properties (E, A, I), and material. |

**Rules.** Beams reference nodes by integer index — never by pointer. No physics or rendering logic lives here; this layer is pure data so it can be used by both the solver and the test suite without any SDL/OpenGL dependency.
