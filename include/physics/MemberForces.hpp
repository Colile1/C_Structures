// Copyright (c) 2026 Colile Sibanda. All rights reserved.
// Proprietary — see LICENSE for terms. Unauthorised use prohibited.
#pragma once
#include <array>
#include <vector>

// physics/MemberForces.hpp : derive internal-force diagrams (axial, shear,
// bending moment, torsion) along a frame member from its local end forces.
// Pure functions — no solving, no I/O. For members carrying only nodal loads
// (Phase 2.1/2.2) axial/shear/torsion are constant and moments vary linearly.

struct InternalForces {
    float N  = 0.0f; // axial (tension positive)
    float Vy = 0.0f; // shear in local y
    float Vz = 0.0f; // shear in local z
    float T  = 0.0f; // torsion about local x
    float My = 0.0f; // bending moment about local y
    float Mz = 0.0f; // bending moment about local z
};

// memberInternalAt
// Purpose: internal forces at local station x along the member from a left-end
//          free body of the local end-force vector p.
// Inputs:  p — [N1,Vy1,Vz1,T1,My1,Mz1, ...] local end forces; L length; x in [0,L].
// Output:  InternalForces at x.
InternalForces memberInternalAt(const std::array<float, 12>& p, float L, float x);

// sampleMember
// Purpose: evenly sample the internal-force diagram along the member.
// Inputs:  p, L; samples — number of stations (>= 2).
// Output:  vector of InternalForces from x=0 to x=L.
std::vector<InternalForces> sampleMember(const std::array<float, 12>& p,
                                         float L, int samples);
