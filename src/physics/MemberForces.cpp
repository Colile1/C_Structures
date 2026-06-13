// Copyright (c) 2026 Colile Sibanda. All rights reserved.
// Proprietary — see LICENSE for terms. Unauthorised use prohibited.
// physics/MemberForces.cpp : internal-force diagram sampling for frame members.
#include "physics/MemberForces.hpp"
#include <cmath>

InternalForces memberInternalAt(const std::array<float, 12>& p, float L, float x,
                                const SpanLoad& load) {
    InternalForces f;
    if (x < 0.0f) x = 0.0f;
    if (L > 0.0f && x > L) x = L;
    const double xd = static_cast<double>(x);

    // Left-segment free body of the true local end forces p. With no span load
    // axial/shear/torsion are constant and moments vary linearly. The sign
    // conventions below were verified against closed-form cantilever results
    // (Vy = -p[1], dMz/dx = -Vy; Vz = -p[2], dMy/dx = +Vz).
    f.N  = -p[0];
    f.T  = -p[3];

    // Span load: linear local intensity q(s) = a + b*s. Integrating along the
    // left segment adds Iq = ∫q (shear) and Im = ∫q·(x-s) (moment); for a UDL
    // this gives linearly-varying shear and a parabolic moment, for a triangular
    // load a cubic moment. Derived to satisfy dM/dx = ∓V with the sign rules above.
    const double Linv = (L > 0.0f) ? 1.0 / static_cast<double>(L) : 0.0;
    const double by = (load.qyL - load.qy0) * Linv;   // local-y slope
    const double bz = (load.qzL - load.qz0) * Linv;    // local-z slope
    const double IqY = load.qy0 * xd + by * xd * xd * 0.5;
    const double IqZ = load.qz0 * xd + bz * xd * xd * 0.5;
    const double ImY = load.qy0 * xd * xd * 0.5 + by * xd * xd * xd / 6.0;
    const double ImZ = load.qz0 * xd * xd * 0.5 + bz * xd * xd * xd / 6.0;

    f.Vy = static_cast<float>(-static_cast<double>(p[1]) - IqY);
    f.Vz = static_cast<float>(-static_cast<double>(p[2]) - IqZ);
    double My = -xd * static_cast<double>(p[2]) - static_cast<double>(p[4]) - ImZ;
    double Mz =  xd * static_cast<double>(p[1]) - static_cast<double>(p[5]) + ImY;

    // Concentrated internal moments add a step to the Mz diagram (and never to
    // the shear). A section past the moment's location carries the extra moment.
    for (const auto& m : load.moments)
        if (xd >= m.a * static_cast<double>(L)) Mz += m.mz;

    f.My = static_cast<float>(My);
    f.Mz = static_cast<float>(Mz);
    return f;
}

std::vector<InternalForces> sampleMember(const std::array<float, 12>& p,
                                         float L, int samples,
                                         const SpanLoad& load) {
    if (samples < 2) samples = 2;
    std::vector<InternalForces> out;
    out.reserve(samples);
    for (int i = 0; i < samples; ++i) {
        float x = (L * i) / (samples - 1);
        out.push_back(memberInternalAt(p, L, x, load));
    }
    return out;
}

// Read one component out of an InternalForces sample.
static float component(const InternalForces& f, DiagramComponent c) {
    switch (c) {
        case DiagramComponent::N:  return f.N;
        case DiagramComponent::Vy: return f.Vy;
        case DiagramComponent::Vz: return f.Vz;
        case DiagramComponent::T:  return f.T;
        case DiagramComponent::My: return f.My;
        case DiagramComponent::Mz: return f.Mz;
    }
    return 0.0f;
}

DiagramStats diagramStats(const std::vector<InternalForces>& samples,
                          DiagramComponent comp, float L) {
    DiagramStats s;
    if (samples.empty()) return s;
    s.startVal = component(samples.front(), comp);
    s.endVal   = component(samples.back(), comp);
    s.peakVal  = s.startVal;
    s.peakX    = 0.0f;
    const int n = static_cast<int>(samples.size());
    for (int i = 0; i < n; ++i) {
        float v = component(samples[i], comp);
        if (std::abs(v) > std::abs(s.peakVal)) {
            s.peakVal = v;
            s.peakX   = (n > 1) ? (L * i) / (n - 1) : 0.0f;
        }
    }
    return s;
}
