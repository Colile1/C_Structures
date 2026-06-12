// Copyright (c) 2026 Colile Sibanda. All rights reserved.
// Proprietary — see LICENSE for terms. Unauthorised use prohibited.
#pragma once
#include <string>

// physics/SolveResult.hpp : status returned by Simulator::solveStaticForces()
// and FrameSimulator::solve() so GUI callers can surface errors on-screen.
enum class SolveStatus { OK, MECHANISM, FAILED };

struct SolveResult {
    SolveStatus status  = SolveStatus::OK;
    std::string message;   // empty on OK; human-readable error otherwise
};
