# graphics/

Low-level OpenGL camera and projection utilities.

| File | Purpose |
|------|---------|
| `Camera.cpp` / `Camera.hpp` | Orbital camera: view matrix, perspective projection, pan/zoom/rotate input handling. |

This layer knows only about matrices and viewport; it has no knowledge of structural data. Kept separate so the solver can be built without it (`BUILD_APP=OFF`).
