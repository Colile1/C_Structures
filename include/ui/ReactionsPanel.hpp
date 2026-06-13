// Copyright (c) 2026 Colile Sibanda. All rights reserved.
// Proprietary — see LICENSE for terms. Unauthorised use prohibited.
#pragma once
#include <vector>
#include "../model/Node.hpp"

class Simulator;
class FrameSimulator;

// ui/ReactionsPanel.hpp : Dear ImGui window showing support reactions and the
// global equilibrium self-check. Display-only; reads results from the solver.
// Truss overload lists reaction forces (Rx,Ry,Rz); the frame overload also lists
// reaction moments (Mx,My,Mz), the extra DOFs the 6-DOF solver carries.
void renderReactionsPanel(const std::vector<Node>& nodes, const Simulator& sim);
void renderReactionsPanel(const std::vector<Node>& nodes, const FrameSimulator& sim);
