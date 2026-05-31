// Copyright (c) 2026 Colile Sibanda. All rights reserved.
// Proprietary — see LICENSE for terms. Unauthorised use prohibited.
#pragma once
#include <vector>
#include "../model/Node.hpp"
#include "../model/Beam.hpp"
#include "../physics/Simulator.hpp"

// renderResultsPanel
// Floating ImGui window that describes analysis results in plain English (Beginner
// mode) or compact tabular form (Engineer mode).  Call after solving.
void renderResultsPanel(const std::vector<Node>& nodes,
                        const std::vector<Beam>& beams,
                        const Simulator& physics,
                        bool beginnerMode,
                        float dispScale);
