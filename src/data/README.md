# data/

Persistence layer: serialisation and deserialisation of project files.

| File | Purpose |
|------|---------|
| `CSVHandler.cpp` / `CSVHandler.hpp` | Saves and loads nodes, beams, and loads as CSV (the canonical interchange format). Round-trip fidelity is verified by `tests/CSVHandlerTests.cpp`. |

**Future.** A `JSONHandler` (Step 19) will add a richer project format that preserves material presets, view-mode preference, and joint types; CSV will remain supported as the lightweight interchange.
