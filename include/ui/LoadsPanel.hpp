// Copyright (c) 2026 Colile Sibanda. All rights reserved.
// Proprietary — see LICENSE for terms. Unauthorised use prohibited.
#pragma once
#include <vector>
#include "../physics/DistributedLoad.hpp"
#include "../physics/FrameSimulator.hpp"
#include "../model/Node.hpp"
#include "../model/Beam.hpp"

// renderLoadsPanel
// Purpose: ImGui panel for adding/removing distributed and moment loads and
//          toggling member self-weight. Only shown when frame mode is active.
// Inputs:  loads — user load list (mutated); selfWeight — self-weight toggle
//          (mutated); frameSim/beams — current model for live updates.
// Returns: true if any load or the self-weight toggle changed (caller re-solves).
bool renderLoadsPanel(std::vector<DistributedLoad>& loads,
                      bool& selfWeight,
                      FrameSimulator& frameSim,
                      const std::vector<Beam>& beams);
