#include <gtest/gtest.h>
#include <cmath>
#include <vector>
#include <array>
#include "physics/FrameSimulator.hpp"
#include "physics/MemberForces.hpp"
#include "physics/DistributedLoad.hpp"
#include "model/Node.hpp"
#include "model/Beam.hpp"

// MemberForcesTests.cpp : internal-force diagrams must reproduce the textbook
// cantilever shear/moment distribution (constant shear, linear moment).

static Beam makeBeam(int a, int b, float E, float A, float I) {
    Beam beam(a, b, E, A);
    beam.setMomentOfInertia(I);
    return beam;
}

// Cantilever, −Y tip load: |M| = P·L at the fixed end, 0 at the tip; shear
// constant at magnitude P; no axial force.
TEST(MemberForces, CantileverMomentDiagram) {
    const float E = 200e9f, A = 1e-2f, I = 1e-5f, L = 2.0f, P = 1000.0f;
    Node n0(0.0f, 0.0f, 0.0f); n0.setJointType(JointType::FIXED);
    Node n1(L, 0.0f, 0.0f);
    std::vector<Node> nodes { n0, n1 };
    std::vector<Beam> beams { makeBeam(0, 1, E, A, I) };
    nodes[1].applyForce(glm::vec3(0.0f, -P, 0.0f));

    FrameSimulator sim(nodes, beams);
    sim.solve();

    auto p = sim.getMemberEndForces(beams[0]);

    InternalForces base = memberInternalAt(p, L, 0.0f);
    InternalForces tip  = memberInternalAt(p, L, L);
    InternalForces mid  = memberInternalAt(p, L, L * 0.5f);

    EXPECT_NEAR(std::abs(base.Mz), P * L,      1.0f);   // PL at the support
    EXPECT_NEAR(tip.Mz,            0.0f,       1.0f);   // zero at the free tip
    EXPECT_NEAR(std::abs(mid.Mz),  P * L*0.5f, 1.0f);   // half-way → PL/2
    EXPECT_NEAR(std::abs(base.Vy), P,          1e-1f);  // shear magnitude = P
    EXPECT_NEAR(std::abs(tip.Vy),  P,          1e-1f);  // constant along member
    EXPECT_NEAR(base.N,            0.0f,       1e-1f);  // no axial force
}

// Axial-only member: N constant and equal to the applied load (tension); no
// shear or moment anywhere along the member.
TEST(MemberForces, AxialMemberConstantN) {
    const float E = 200e9f, A = 1e-4f, I = 1e-6f, L = 2.0f, P = 1000.0f;
    Node n0(0.0f, 0.0f, 0.0f); n0.setJointType(JointType::FIXED);
    Node n1(L, 0.0f, 0.0f);
    std::vector<Node> nodes { n0, n1 };
    std::vector<Beam> beams { makeBeam(0, 1, E, A, I) };
    nodes[1].applyForce(glm::vec3(P, 0.0f, 0.0f));

    FrameSimulator sim(nodes, beams);
    sim.solve();

    auto p = sim.getMemberEndForces(beams[0]);
    auto samples = sampleMember(p, L, 5);
    for (const auto& s : samples) {
        EXPECT_NEAR(s.N,  P,    1.0f);
        EXPECT_NEAR(s.Vy, 0.0f, 1e-1f);
        EXPECT_NEAR(s.Mz, 0.0f, 1e-1f);
    }
}

// Simply-supported beam under a UDL: the reconstructed diagram must be the
// textbook parabola — zero moment at the pinned ends, M = wL²/8 at mid-span, and
// zero shear at mid-span. Built from the true SS end forces plus a UDL SpanLoad,
// which is exactly what the G2 fix has to reproduce.
TEST(MemberForces, SimplySupportedUDL) {
    const float  L = 4.0f;
    const double w = -1000.0;                         // local-y intensity (downward, signed)
    std::array<float, 12> p{};
    p[1] = static_cast<float>(-w * L / 2.0);          // true SS end shear; ends carry no moment

    SpanLoad sl; sl.qy0 = sl.qyL = w;

    auto pts = sampleMember(p, L, 5, sl);             // stations at 0, L/4, L/2, 3L/4, L
    const double wmag = std::abs(w);
    EXPECT_NEAR(std::abs(pts.front().Mz), 0.0,                  1.0f); // pinned end → 0
    EXPECT_NEAR(std::abs(pts.back().Mz),  0.0,                  1.0f); // pinned end → 0
    EXPECT_NEAR(std::abs(pts[2].Mz),      wmag * L * L / 8.0,   1.0f); // wL²/8 at mid-span
    EXPECT_NEAR(pts[2].Vy,                0.0f,                 1e-1f); // zero shear at mid-span
    EXPECT_NEAR(std::abs(pts.front().Vy), wmag * L / 2.0,       1.0f); // wL/2 at the support
}

// Fixed-fixed beam under a UDL through the full solver: exercises the end-force
// CENL correction + local projection end-to-end. Known result: end moment wL²/12,
// mid-span moment wL²/24, zero shear at mid-span, end shear wL/2. Both ends FIXED
// keep the single-element model free of the torsion rigid-body mode (a literal
// simply-supported frame member is a mechanism in the 6-DOF solver).
TEST(MemberForces, FixedFixedUDL) {
    const float  E = 200e9f, A = 1e-2f, I = 1e-5f, L = 4.0f;
    const double w = 1000.0;                          // magnitude; direction carries the sign
    Node n0(0.0f, 0.0f, 0.0f); n0.setJointType(JointType::FIXED);
    Node n1(L,    0.0f, 0.0f); n1.setJointType(JointType::FIXED);
    std::vector<Node> nodes { n0, n1 };
    std::vector<Beam> beams { makeBeam(0, 1, E, A, I) };
    std::vector<DistributedLoad> loads {
        { 0, LoadType::UDL, glm::vec3(0.0f,-1.0f,0.0f), static_cast<float>(w), 0.0f, 0.0f }
    };

    FrameSimulator sim(nodes, beams);
    sim.setDistributedLoads(loads);
    sim.solve();

    auto ef = sim.getMemberEndForces(beams[0]);
    auto sl = sim.getMemberSpanLoad(beams[0]);
    auto pts = sampleMember(ef, L, 5, sl);            // mid-span is index 2

    auto Mres = [](const InternalForces& f){ return std::sqrt(f.My*f.My + f.Mz*f.Mz); };
    auto Vres = [](const InternalForces& f){ return std::sqrt(f.Vy*f.Vy + f.Vz*f.Vz); };

    const double Mend = w*L*L/12.0, Mmid = w*L*L/24.0, Vend = w*L/2.0;
    EXPECT_NEAR(Mres(pts.front()), Mend, 2.0);
    EXPECT_NEAR(Mres(pts.back()),  Mend, 2.0);
    EXPECT_NEAR(Mres(pts[2]),      Mmid, 2.0);
    EXPECT_NEAR(Vres(pts[2]),      0.0,  1.0);
    EXPECT_NEAR(Vres(pts.front()), Vend, 2.0);
}

// Fixed-fixed beam with a concentrated moment M at mid-span: the moment diagram
// steps by M across the application point (−M/2 just left, +M/2 just right) while
// the shear is unaffected (no jump). Supports read ±M/4.
TEST(MemberForces, FixedFixedCentralMoment) {
    const float  E = 200e9f, A = 1e-2f, I = 1e-5f, L = 4.0f;
    const double M = 5000.0;
    Node n0(0.0f, 0.0f, 0.0f); n0.setJointType(JointType::FIXED);
    Node n1(L,    0.0f, 0.0f); n1.setJointType(JointType::FIXED);
    std::vector<Node> nodes { n0, n1 };
    std::vector<Beam> beams { makeBeam(0, 1, E, A, I) };
    std::vector<DistributedLoad> loads {
        { 0, LoadType::MOMENT, glm::vec3(0.0f,-1.0f,0.0f), static_cast<float>(M), 0.0f, 0.5f }
    };

    FrameSimulator sim(nodes, beams);
    sim.setDistributedLoads(loads);
    sim.solve();

    auto ef = sim.getMemberEndForces(beams[0]);
    auto sl = sim.getMemberSpanLoad(beams[0]);

    InternalForces left  = memberInternalAt(ef, L, L * 0.499f, sl);
    InternalForces right = memberInternalAt(ef, L, L * 0.501f, sl);
    InternalForces start = memberInternalAt(ef, L, 0.0f,       sl);
    InternalForces end   = memberInternalAt(ef, L, L,          sl);

    EXPECT_NEAR(right.Mz - left.Mz,  M,     M * 0.02);  // step of M at the point moment
    EXPECT_NEAR(std::abs(left.Mz),   M/2.0, M * 0.05);  // −M/2 just left of mid
    EXPECT_NEAR(std::abs(right.Mz),  M/2.0, M * 0.05);  // +M/2 just right of mid
    EXPECT_NEAR(std::abs(start.Mz),  M/4.0, M * 0.05);  // support carries M/4
    EXPECT_NEAR(std::abs(end.Mz),    M/4.0, M * 0.05);
    EXPECT_NEAR(left.Vy, right.Vy, std::abs(left.Vy)*0.02 + 1.0); // shear continuous
}
