// Copyright (c) 2026 Colile Sibanda. All rights reserved.
// Proprietary — see LICENSE for terms. Unauthorised use prohibited.
// physics/Determinacy.cpp : count-based determinacy + obvious-mechanism check.
#include "physics/Determinacy.hpp"

// countReactions
// Purpose: total number of constrained translational DOFs across all nodes.
// Inputs:  nodes — the scene nodes.
// Output:  reaction-component count r.
static int countReactions(const std::vector<Node>& nodes) {
    int r = 0;
    for (const Node& nd : nodes)
        for (int d = 0; d < 3; ++d)
            if (nd.isDOFConstrained(d)) ++r;
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
                                     const std::vector<Beam>& beams) {
    DeterminacyResult res;
    res.nodes     = static_cast<int>(nodes.size());
    res.dof       = 3 * res.nodes;
    res.reactions = countReactions(nodes);

    std::vector<int> degree;
    res.members = countValidMembers(nodes, beams, degree);

    // Obvious mechanism: a free (unsupported) node held by fewer than 2 members
    // can rotate or translate freely.
    for (int i = 0; i < res.nodes; ++i)
        if (nodes[i].getJointType() == JointType::FREE && degree[i] < 2)
            res.hasMechanismHint = true;

    res.degree = (res.members + res.reactions) - res.dof;

    if (res.nodes == 0) {
        res.stability = Stability::UNSTABLE;
        res.message   = "Empty model — add nodes, members, and supports.";
        return res;
    }

    if (res.degree < 0 || res.hasMechanismHint) {
        res.stability = Stability::UNSTABLE;
        if (res.degree < 0)
            res.message = "Unstable: too few members/supports (m + r < 3n). "
                          "Add bracing or supports.";
        else
            res.message = "Unstable: a free joint is held by fewer than two "
                          "members and can move. Add a member or support.";
    } else if (res.degree == 0) {
        res.stability = Stability::DETERMINATE;
        res.message   = "Statically determinate (m + r = 3n). Solvable by statics.";
    } else {
        res.stability = Stability::INDETERMINATE;
        res.message   = "Statically indeterminate to degree "
                        + std::to_string(res.degree)
                        + " (m + r > 3n). Solvable, has redundant members.";
    }
    return res;
}
