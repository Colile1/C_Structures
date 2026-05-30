// Copyright (c) 2026 Colile Sibanda. All rights reserved.
// Proprietary — see LICENSE for terms. Unauthorised use prohibited.
#pragma once
#include <vector>
#include "../model/Node.hpp"

class Simulator;

// ui/ReactionsPanel.hpp : Dear ImGui window showing support reactions and the
// global equilibrium self-check. Display-only; reads results from the solver.
void renderReactionsPanel(const std::vector<Node>& nodes, const Simulator& sim);
