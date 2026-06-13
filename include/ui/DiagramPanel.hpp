// Copyright (c) 2026 Colile Sibanda. All rights reserved.
// Proprietary — see LICENSE for terms. Unauthorised use prohibited.
#pragma once
#include <vector>
#include "../model/Node.hpp"
#include "../model/Beam.hpp"
#include "../physics/FrameSimulator.hpp"

// renderDiagramPanel
// Floating ImGui window annotating the internal-force diagram currently overlaid
// in frame mode: per-member end values and the signed peak with its location, in
// the units of the selected quantity, plus that quantity's sign convention and a
// headline maximum. Call only in frame mode while a diagram is shown.
// Inputs: solved model + frameSim, and diagramType 0..5 (matches the UI switch).
void renderDiagramPanel(const std::vector<Node>& nodes,
                        const std::vector<Beam>& beams,
                        const FrameSimulator& frameSim,
                        int diagramType);
