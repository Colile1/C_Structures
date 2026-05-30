// Copyright (c) 2026 Colile Sibanda. All rights reserved.
// Proprietary — see LICENSE for terms. Unauthorised use prohibited.
#pragma once
#include <string>
#include <vector>
#include "../model/Node.hpp"
#include "../model/Beam.hpp"

// physics/Determinacy.hpp : static determinacy / stability pre-check for a 3D
// pin-jointed truss. Pure analysis (no solving, no I/O) so it is unit-testable.

enum class Stability { UNSTABLE, DETERMINATE, INDETERMINATE };

struct DeterminacyResult {
    int  members   = 0;   // m — beams with two distinct valid endpoints
    int  reactions = 0;   // r — total constrained DOFs across all nodes
    int  nodes     = 0;   // n
    int  dof       = 0;   // 3n (3 translational DOFs per node)
    int  degree    = 0;   // (m + r) − 3n; > 0 = degree of indeterminacy
    bool hasMechanismHint = false; // a free node held by < 2 members (can swing)
    Stability stability   = Stability::UNSTABLE;
    std::string message;  // plain-language summary for the UI
};

// analyzeDeterminacy
// Purpose: classify a truss as unstable / determinate / indeterminate using the
//          count criterion m + r vs 3n, plus an obvious-mechanism check.
// Inputs:  nodes, beams — the scene.
// Output:  DeterminacyResult with counts, classification, and a message.
DeterminacyResult analyzeDeterminacy(const std::vector<Node>& nodes,
                                     const std::vector<Beam>& beams);
