// Copyright (c) 2026 Colile Sibanda. All rights reserved.
// Proprietary — see LICENSE for terms. Unauthorised use prohibited.
// physics/Determinacy.cpp : count-based determinacy + obvious-mechanism check
// for both the pin-jointed truss (3 DOF/node) and rigid-jointed frame (6).
#include "physics/Determinacy.hpp"

// nodeIsSupported
// Purpose: true if the node carries any reaction (translational, or a fixed
//          joint's rotational restraint) — i.e. it is held to ground.
// Inputs:  nd — the node; frame — whether the 6-DOF model is in use.
// Output:  bool.
static bool nodeIsSupported(const Node& nd, bool frame) {
    for (int d = 0; d < 3; ++d)
        if (nd.isDOFConstrained(d)) return true;
    if (frame && nd.getJointType() == JointType::FIXED) return true; // rotations
    return false;
}

// countReactions
// Purpose: total number of constrained DOFs across all nodes. In frame mode a
//          FIXED joint also restrains the three rotations (matching
//          FrameSimulator::isDofConstrained — pins/rollers are moment-releases).
// Inputs:  nodes — the scene nodes; frame — whether the 6-DOF model is in use.
// Output:  reaction-component count r.
static int countReactions(const std::vector<Node>& nodes, bool frame) {
    int r = 0;
    for (const Node& nd : nodes) {
        for (int d = 0; d < 3; ++d)
            if (nd.isDOFConstrained(d)) ++r;
        if (frame && nd.getJointType() == JointType::FIXED) r += 3; // Rx, Ry, Rz
    }
    return r;
}

// countValidMembers
// Purpose: count beams whose endpoints are two distinct, in-range nodes, and
//          tally how many members touch each node (for the mechanism check).
// Inputs:  nodes, beams; degree — out param sized to nodes, member counts.
// Output:  valid member count m.
static int countValidMembers(const std::vector<Node>& nodes,
                             const std::vector<Beam>& beams,
                             std::vector<int>& degree) {
    const int n = static_cast<int>(nodes.size());
    degree.assign(n, 0);
    int m = 0;
    for (const Beam& b : beams) {
        const int i = b.getStartIdx(), j = b.getEndIdx();
        if (i < 0 || j < 0 || i >= n || j >= n || i == j) continue;
        ++m; ++degree[i]; ++degree[j];
    }
    return m;
}

DeterminacyResult analyzeDeterminacy(const std::vector<Node>& nodes,
                                     const std::vector<Beam>& beams,
                                     bool frame) {
    DeterminacyResult res;
    res.frame      = frame;
    res.dofPerNode = frame ? 6 : 3;
    res.nodes      = static_cast<int>(nodes.size());
    res.dof        = res.dofPerNode * res.nodes;
    res.reactions  = countReactions(nodes, frame);

    std::vector<int> degree;
    res.members = countValidMembers(nodes, beams, degree);
    // A pin-jointed member carries one axial unknown; a 3D rigid-frame member
    // carries six (axial, two shears, torsion, two bending moments).
    res.memberUnknowns = frame ? 6 * res.members : res.members;

    // Obvious mechanism.
    if (frame) {
        // A rigid joint transfers moment, so a single member can hold a node
        // (a cantilever). The give-away mechanisms are a frame with no supports
        // at all (free to float as a rigid body) or a disconnected, unheld node.
        if (res.nodes > 0 && res.reactions == 0) res.hasMechanismHint = true;
        for (int i = 0; i < res.nodes; ++i)
            if (degree[i] == 0 && !nodeIsSupported(nodes[i], true))
                res.hasMechanismHint = true;
    } else {
        // Pin joints: a free node held by fewer than two members can swing.
        for (int i = 0; i < res.nodes; ++i)
            if (nodes[i].getJointType() == JointType::FREE && degree[i] < 2)
                res.hasMechanismHint = true;
    }

    res.degree = (res.memberUnknowns + res.reactions) - res.dof;

    const char* lhs = frame ? "6m + r" : "m + r";
    const char* rhs = frame ? "6n" : "3n";

    if (res.nodes == 0) {
        res.stability = Stability::UNSTABLE;
        res.message   = "Empty model — add nodes, members, and supports.";
        return res;
    }

    if (res.degree < 0 || res.hasMechanismHint) {
        res.stability = Stability::UNSTABLE;
        if (res.degree < 0) {
            res.message = std::string("Unstable: too few members/supports (")
                          + lhs + " < " + rhs + "). Add bracing or supports.";
        } else if (frame) {
            res.message = "Unstable: the frame has no supports (or a loose joint) "
                          "and can move as a rigid body. Add supports.";
        } else {
            res.message = "Unstable: a free joint is held by fewer than two "
                          "members and can move. Add a member or support.";
        }
    } else if (res.degree == 0) {
        res.stability = Stability::DETERMINATE;
        res.message   = std::string("Statically determinate (") + lhs + " = "
                        + rhs + "). Solvable by statics.";
    } else {
        res.stability = Stability::INDETERMINATE;
        res.message   = "Statically indeterminate to degree "
                        + std::to_string(res.degree) + " (" + lhs + " > " + rhs
                        + "). Solvable, has redundant members/supports.";
    }
    return res;
}
