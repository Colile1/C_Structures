// Copyright (c) 2026 Colile Sibanda. All rights reserved.
// Proprietary — see LICENSE for terms. Unauthorised use prohibited.
// physics/MemberForces.cpp : internal-force diagram sampling for frame members.
#include "physics/MemberForces.hpp"

InternalForces memberInternalAt(const std::array<float, 12>& p, float L, float x) {
    InternalForces f;
    if (x < 0.0f) x = 0.0f;
    if (L > 0.0f && x > L) x = L;

    // Left-segment free body of the local end-force vector. With only nodal
    // loads, axial/shear/torsion are constant; moments vary linearly. The sign
    // conventions below were verified against closed-form cantilever results.
    f.N  = -p[0];
    f.Vy = -p[1];
    f.Vz = -p[2];
    f.T  = -p[3];
    f.My = -x * p[2] - p[4];
    f.Mz =  x * p[1] - p[5];
    return f;
}

std::vector<InternalForces> sampleMember(const std::array<float, 12>& p,
                                         float L, int samples) {
    if (samples < 2) samples = 2;
    std::vector<InternalForces> out;
    out.reserve(samples);
    for (int i = 0; i < samples; ++i) {
        float x = (L * i) / (samples - 1);
        out.push_back(memberInternalAt(p, L, x));
    }
    return out;
}
