# visualization/

Renders structural results to the OpenGL framebuffer using VAO/VBO geometry.

| File | Purpose |
|------|---------|
| `ForceRenderer.cpp` / `ForceRenderer.hpp` | Draws force-diagram geometry (axial/shear/moment filled bands, arrow heads) using the force shader pair in `shaders/`. |

**Note:** `RendererUtils` (legacy fixed-function stubs for `drawArrow`/`drawCylinder`/`drawCone`) was removed in Step 9 — all rendering now goes through `ForceRenderer`'s VAO approach.
