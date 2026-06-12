// Copyright (c) 2026 Colile Sibanda. All rights reserved.
// Proprietary — see LICENSE for terms. Unauthorised use prohibited.
#pragma once
#include <array>
#include <vector>

// physics/MemberForces.hpp : derive internal-force diagrams (axial, shear,
// bending moment, torsion) along a frame member from its local end forces and
// any span load the member carries. Pure functions — no solving, no I/O. With a
// span load the shear varies linearly and the moment is parabolic (UDL) / cubic
// (triangular); a concentrated moment adds a step to the moment diagram.

struct InternalForces {
    float N  = 0.0f; // axial (tension positive)
    float Vy = 0.0f; // shear in local y
    float Vz = 0.0f; // shear in local z
    float T  = 0.0f; // torsion about local x
    float My = 0.0f; // bending moment about local y
    float Mz = 0.0f; // bending moment about local z
};

// Concentrated internal moment at fractional position a in [0,1] along the
// member, about the local z axis. Produces a step in the Mz diagram.
struct PointMoment {
    double a  = 0.0;
    double mz = 0.0;
};

// Local-frame description of a member's span loading, used to reconstruct the
// true internal-force diagram between the member ends.
struct SpanLoad {
    // Transverse intensity q(s) = q0 + (qL - q0)*s/L (linear: covers UDL when
    // q0==qL and triangular otherwise). Signed in the member's local frame,
    // matching DistributedLoad::udlLocalY (downward load => negative intensity).
    double qy0 = 0.0, qyL = 0.0;        // local y
    double qz0 = 0.0, qzL = 0.0;        // local z
    std::vector<PointMoment> moments;   // concentrated internal moments (about local z)
};

// memberInternalAt
// Purpose: internal forces at local station x from a left-end free body of the
//          local end-force vector p, superposing the member's span load so the
//          shear varies and the moment curves between the ends.
// Inputs:  p — [N1,Vy1,Vz1,T1,My1,Mz1, ...] true local end forces; L length;
//          x in [0,L]; load — local-frame span load (default: none).
// Output:  InternalForces at x.
InternalForces memberInternalAt(const std::array<float, 12>& p, float L, float x,
                                const SpanLoad& load = {});

// sampleMember
// Purpose: evenly sample the internal-force diagram along the member.
// Inputs:  p, L; samples — number of stations (>= 2); load — span load (default none).
// Output:  vector of InternalForces from x=0 to x=L.
std::vector<InternalForces> sampleMember(const std::array<float, 12>& p,
                                         float L, int samples,
                                         const SpanLoad& load = {});
