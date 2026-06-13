# ui/

Dear ImGui panels and the top-level UI orchestrator. Each panel owns one logical region of the interface.

| File | Purpose |
|------|---------|
| `UIHandler.cpp` | Orchestrates the full Dear ImGui frame: calls each panel, owns the main menu bar and mode toggles. |
| `ModelCheckPanel.cpp` | Displays determinacy verdict, solver status messages, and error/warning badges. |
| `LoadsPanel.cpp` | Controls for adding/editing nodal forces and distributed span loads. |
| `ReactionsPanel.cpp` | Table of support reactions and equilibrium summary (ΣF ≈ 0). |
| `ResultsPanel.cpp` | Member-force summary table (peak axial/shear/moment per member). |
| `GlassBoxPanel.cpp` | "Show the math" glass-box: step-by-step stiffness assembly and solve trace. |
| `Templates.cpp` | One-click template cards that load a correctly-defined example model. |

**No physics here.** Panels read solver output and write user-specified loads/geometry; they never assemble matrices or call Eigen directly.
