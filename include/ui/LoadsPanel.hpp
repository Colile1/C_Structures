// Copyright (c) 2026 Colile Sibanda. All rights reserved.
// Proprietary — see LICENSE for terms. Unauthorised use prohibited.
#pragma once
#include <vector>
#include "../physics/DistributedLoad.hpp"
#include "../physics/FrameSimulator.hpp"
#include "../model/Node.hpp"
#include "../model/Beam.hpp"

// renderLoadsPanel
// Purpose: ImGui panel for adding/removing distributed and moment loads.
//          Only shown when frame mode is active.
// Returns: true if any load was added or removed (caller should re-solve).
bool renderLoadsPanel(std::vector<DistributedLoad>& loads,
                      FrameSimulator& frameSim,
                      const std::vector<Beam>& beams);
