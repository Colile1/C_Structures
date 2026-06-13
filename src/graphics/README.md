# graphics/

Low-level OpenGL camera, projection, and icon-texture utilities.

| File | Purpose |
|------|---------|
| `Camera.cpp` / `Camera.hpp` | Orbital camera: view matrix, perspective projection, pan/zoom/rotate input handling. |
| `IconLibrary.cpp` / `IconLibrary.hpp` | Rasterises the `resources/icons` SVG set (joints / sections / loads) to cached OpenGL textures for ImGui, with a switchable symbol / realistic-2D / realistic-3D view mode. Uses the vendored `third_party/nanosvg` header-only rasteriser. |

This layer knows only about matrices, textures, and the viewport; it has no knowledge of structural data. Kept separate so the solver can be built without it (`BUILD_APP=OFF`).
