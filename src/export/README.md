# export/

Shareable output. Turns a solved model and the live viewport into files a student can hand in or a teacher can post.

| File | Purpose |
|------|---------|
| `Exporter.cpp` / `Exporter.hpp` | PNG capture of the OpenGL viewport, and a one-page PDF report (model summary, support reactions, member-force table, and the force diagrams). |

This layer reads the model and solver output plus the framebuffer; it does not modify the model. Image/PDF writing is self-contained (no extra runtime service).
