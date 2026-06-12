// Copyright (c) 2026 Colile Sibanda. All rights reserved.
// Proprietary — see LICENSE for terms. Unauthorised use prohibited.
#pragma once
#include <vector>
#include "../model/Node.hpp"
#include "../model/Beam.hpp"
#include "../physics/SolveResult.hpp"

// ui/ModelCheckPanel.hpp : Dear ImGui window showing the determinacy/stability
// pre-check (counts + plain-language verdict) and the last solver status.
// frame: true to classify with the 6-DOF frame model, matching the active solver.
// result: pointer to the most recent SolveResult; nullptr = not yet solved.
void renderModelCheckPanel(const std::vector<Node>& nodes,
                           const std::vector<Beam>& beams,
                           bool frame = false,
                           const SolveResult* result = nullptr);
