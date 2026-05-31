// Copyright (c) 2026 Colile Sibanda. All rights reserved.
// Proprietary — see LICENSE for terms. Unauthorised use prohibited.
#pragma once
#include <vector>
#include "../model/Node.hpp"
#include "../model/Beam.hpp"
#include "../physics/Simulator.hpp"

// renderGlassBoxPanel
// Engineer-mode panel showing the global stiffness matrix and the solve steps:
// DOF labelling, free-DOF extraction, and solution vector.
// Limited to models with ≤ 6 nodes to keep the table readable.
void renderGlassBoxPanel(const std::vector<Node>& nodes,
                         const std::vector<Beam>& beams,
                         const Simulator& physics,
                         bool* pOpen);
