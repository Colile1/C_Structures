// Copyright (c) 2026 Colile Sibanda. All rights reserved.
// Proprietary — see LICENSE for terms. Unauthorised use prohibited.
#pragma once
#include <string>
#include <vector>
#include "../model/Node.hpp"
#include "../model/Beam.hpp"

// physics/Determinacy.hpp : static determinacy / stability pre-check for a 3D
// pin-jointed truss (3 DOF/node) or rigid-jointed frame (6 DOF/node). Pure
// analysis (no solving, no I/O) so it is unit-testable.

enum class Stability { UNSTABLE, DETERMINATE, INDETERMINATE };

struct DeterminacyResult {
    int  members        = 0;   // m — beams with two distinct valid endpoints
    int  memberUnknowns = 0;   // internal-force unknowns: m (truss) or 6m (frame)
    int  reactions      = 0;   // r — total constrained DOFs across all nodes
    int  nodes          = 0;   // n
    int  dofPerNode     = 3;   // 3 (truss, translations) or 6 (frame, +rotations)
    int  dof            = 0;   // dofPerNode·n — equilibrium equations available
    int  degree         = 0;   // (memberUnknowns + r) − dof; > 0 = indeterminate
    bool frame          = false; // true if classified with the 6-DOF frame model
    bool hasMechanismHint = false; // an obvious rigid-body / loose-joint mechanism
    Stability stability   = Stability::UNSTABLE;
    std::string message;  // plain-language summary for the UI
};

// analyzeDeterminacy
// Purpose: classify a structure as unstable / determinate / indeterminate using
//          the count criterion (m + r vs 3n for a truss; 6m + r vs 6n for a
//          frame), plus an obvious-mechanism check.
// Inputs:  nodes, beams — the scene; frame — true to use the 6-DOF frame model
//          (counts rotational restraint, 6 unknowns/member) instead of the truss.
// Output:  DeterminacyResult with counts, classification, and a message.
DeterminacyResult analyzeDeterminacy(const std::vector<Node>& nodes,
                                     const std::vector<Beam>& beams,
                                     bool frame = false);
