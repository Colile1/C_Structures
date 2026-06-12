// Copyright (c) 2026 Colile Sibanda. All rights reserved.
// Proprietary — see LICENSE for terms. Unauthorised use prohibited.
#include <gtest/gtest.h>
#include <glm/glm.hpp>
#include <cmath>
#include "physics/DistributedLoad.hpp"
#include "physics/FrameSimulator.hpp"
#include "model/Node.hpp"
#include "model/Beam.hpp"

// Sign convention: CENL forces are work-equivalent loads added to the force vector,
// NOT fixed-end reactions. A downward UDL (direction=-Y, w>0) produces:
//   Vy_i = Vy_j = -wL/2  (downward, negative in global Y)
//   Mz_i = -wL²/12       (hogging when CCW=+, negative)
//   Mz_j = +wL²/12       (sagging, positive)
// These are the forces that, when added to nodal loads, reproduce the same
// structural response as the original distributed load.

TEST(DistributedLoad, HorizontalBeamUDL_CENL) {
    // Beam along +X, length 4 m; UDL 1000 N/m downward (-Y).
    glm::vec3 p1(0,0,0), p2(4,0,0);
    const float L = 4.0f, w = 1000.0f;

    DistributedLoad dl;
    dl.beamIdx   = 0;
    dl.type      = LoadType::UDL;
    dl.direction = glm::vec3(0,-1,0); // downward
    dl.w         = w;
    dl.w2 = dl.pos = 0.0f;

    auto f = consistentNodalLoads(dl, p1, p2, L);

    // Work-equivalent shear loads are directed WITH the applied load (downward = negative Y).
    EXPECT_NEAR(f[1],  -(double)(w*L/2),          1.0) << "Vy at i = -wL/2";
    EXPECT_NEAR(f[5],  -(double)(w*L*L/12.0f),    1.0) << "Mz at i = -wL²/12";
    EXPECT_NEAR(f[7],  -(double)(w*L/2),          1.0) << "Vy at j = -wL/2";
    EXPECT_NEAR(f[11],  (double)(w*L*L/12.0f),    1.0) << "Mz at j = +wL²/12";
    EXPECT_NEAR(f[0],  0.0, 1.0) << "No axial force at i from transverse UDL";
    EXPECT_NEAR(f[6],  0.0, 1.0) << "No axial force at j from transverse UDL";
}

TEST(DistributedLoad, UDL_ForceEquilibrium) {
    // Total nodal Vy must equal -w*L (total downward load).
    glm::vec3 p1(0,0,0), p2(3,0,0);
    const float L = 3.0f, w = 500.0f;
    DistributedLoad dl{ 0, LoadType::UDL, glm::vec3(0,-1,0), w, 0, 0 };
    auto f = consistentNodalLoads(dl, p1, p2, L);

    double totalFy = f[1] + f[7];
    EXPECT_NEAR(totalFy, -(double)(w * L), 1.0) << "Sum of shear CENL must equal -w*L";

    // Net moment about midspan must be zero (symmetry of UDL).
    // Moment of F_y about midspan = F_y * (x_force - L/2).
    double half = L / 2.0;
    double momentAboutMid = f[1]*(-half) + f[5]   // Vy_i and Mz_i at x=0
                           + f[7]*(+half) + f[11]; // Vy_j and Mz_j at x=L
    EXPECT_NEAR(momentAboutMid, 0.0, 10.0) << "Moment balance about midspan";
}

TEST(DistributedLoad, TriangularLoad_NodeI_Only) {
    // Triangular load: full intensity ws at i, zero at j.
    glm::vec3 p1(0,0,0), p2(6,0,0);
    const float L = 6.0f, ws = 1000.0f, we = 0.0f;
    DistributedLoad dl{ 0, LoadType::TRIANGULAR, glm::vec3(0,-1,0), ws, we, 0 };
    auto f = consistentNodalLoads(dl, p1, p2, L);

    // Total load = area under triangle = ws*L/2, all downward.
    double totalFy = f[1] + f[7];
    EXPECT_NEAR(totalFy, -(double)(ws * L / 2.0f), 1.0) << "Triangular CENL sum = -ws*L/2";
    // Node i carries more load (7/10 vs 3/10 split).
    EXPECT_LT(f[1], f[7]) << "More shear (more negative) at the heavier-loaded end i";
}

TEST(DistributedLoad, CantileverUDL_TipDeflection) {
    // Cantilever (FIXED at left, free at right), 4 elements of 1 m each.
    // UDL 1000 N/m downward; E=200 GPa, I=8.33e-6 m^4.
    // Expected tip deflection: -wL^4/(8EI) (downward).
    const float E = 200e9f, A = 0.01f, I = 8.33e-6f;
    const float Lm = 4.0f; // total length
    const int   NE = 4;     // number of elements
    const float Le = Lm / NE;
    const float w  = 1000.0f;

    std::vector<Node> nodes;
    nodes.emplace_back(0.0f, 0.0f, 0.0f); nodes.back().setJointType(JointType::FIXED);
    for (int k = 1; k <= NE; ++k)
        nodes.emplace_back(k * Le, 0.0f, 0.0f);

    std::vector<Beam> beams;
    for (int k = 0; k < NE; ++k) {
        beams.emplace_back(k, k+1, E, A);
        beams.back().setMomentOfInertia(I);
    }

    std::vector<DistributedLoad> loads;
    for (int k = 0; k < NE; ++k)
        loads.push_back({ k, LoadType::UDL, glm::vec3(0,-1,0), w, 0, 0 });

    FrameSimulator fs(nodes, beams);
    fs.setDistributedLoads(loads);
    fs.solve();

    auto trans = fs.getNodeTranslations();
    float tipDy = trans[NE].y;
    float expected = -(w * Lm*Lm*Lm*Lm) / (8.0f * E * I);
    // 4-element Hermite cantilever under UDL: within 0.5% of exact.
    EXPECT_NEAR(tipDy, expected, std::abs(expected) * 0.005f)
        << "Cantilever tip deflection under UDL should match -wL^4/(8EI)";
    EXPECT_LT(tipDy, 0.0f) << "Tip must deflect downward";
}
