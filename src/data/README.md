# data/

Persistence layer: serialisation and deserialisation of project files.

| File | Purpose |
|------|---------|
| `CSVHandler.cpp` / `CSVHandler.hpp` | Saves and loads nodes, beams, supports, and concentrated nodal moments as CSV (the lightweight interchange format). Round-trip fidelity is verified by `tests/CSVHandlerTests.cpp`. |
| `JSONHandler.cpp` / `JSONHandler.hpp` | Richer project format (`saveProject`/`loadProject`): nodes, beams, distributed loads, and view-mode preferences (`ViewPrefs`). Built on nlohmann/json; round-trip verified by `tests/JSONHandlerTests.cpp`. |

CSV remains the canonical interchange format; JSON is the full project format. Both store beam connectivity as integer node indices, so files survive position edits.
