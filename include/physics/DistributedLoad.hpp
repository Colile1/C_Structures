// Copyright (c) 2026 Colile Sibanda. All rights reserved.
// Proprietary — see LICENSE for terms. Unauthorised use prohibited.
#pragma once
#include <array>
#include <vector>
#include <glm/glm.hpp>

// physics/DistributedLoad.hpp : consistent equivalent nodal loads (CENL) for
// distributed and moment loads on 3D frame members. These are added to the
// global force vector before assembly so FrameSimulator needs no changes.
//
// All intensities are in the member's LOCAL coordinate system (local y = w_y,
// local z = w_z, local x = axial/torsional). The caller specifies the global
// direction of the load and we project into the member's local frame.

enum class LoadType {
    UDL,         // uniform distributed load (w per unit length, N/m)
    TRIANGULAR,  // linearly varying: w_start at node-i, w_end at node-j (N/m)
    MOMENT,      // concentrated moment at a fractional position along the member
};

struct DistributedLoad {
    int       beamIdx;      // index into the beams vector
    LoadType  type;
    glm::vec3 direction;    // global unit direction of the load (+Y = gravity down with -ve w)
    float     w;            // intensity: UDL/TRIANGULAR = N/m; MOMENT = N·m
    float     w2;           // second intensity for TRIANGULAR (w at node-j end); unused for others
    float     pos;          // fractional position [0,1] for MOMENT loads
};

// consistentNodalLoads
// Purpose: return the 12-element consistent equivalent nodal load vector for a
//          single distributed load on a member.  The result is in GLOBAL
//          coordinates and maps directly into the FrameSimulator's force vector.
// Inputs:  dl — the load descriptor; p1, p2 — beam end positions; L — length.
// Output:  12-vector [f_i(6), f_j(6)] global equivalent nodal forces/moments.
std::array<double, 12> consistentNodalLoads(const DistributedLoad& dl,
                                             const glm::vec3& p1,
                                             const glm::vec3& p2,
                                             float L);

// applyDistributedLoads
// Purpose: add CENL contributions to an existing 6n global force vector
//          (same layout as FrameSimulator: DOF order [ux,uy,uz,rx,ry,rz] per node).
// Inputs:  loads — the load list; nodes/beams — scene geometry; F — 6n force vector.
void applyDistributedLoads(const std::vector<DistributedLoad>& loads,
                           const std::vector<glm::vec3>& positions,
                           const std::vector<std::pair<int,int>>& connectivity,
                           std::vector<double>& F);
