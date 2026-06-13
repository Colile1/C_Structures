#include <gtest/gtest.h>
#include <glm/glm.hpp>
#include <cmath>
#include <vector>
#include "physics/FrameSimulator.hpp"
#include "model/Node.hpp"
#include "model/Beam.hpp"

// FrameTests.cpp : verifies the 6-DOF frame solver against closed-form
// Euler-Bernoulli results (independently confirmed in NumPy).

// Helper: a steel beam with an explicit second moment of area.
static Beam makeBeam(int a, int b, float E, float A, float I) {
    Beam beam(a, b, E, A);
    beam.setMomentOfInertia(I);
    return beam;
}

// Cantilever along +X, transverse tip load in −Y, fixed base.
// Tip deflection = −PL³/(3EI); tip slope = −PL²/(2EI); base reactions balance.
TEST(Frame, CantileverTipDeflection) {
    const float E = 200e9f, A = 1e-2f, I = 1e-5f, L = 2.0f, P = 1000.0f;
    Node n0(0.0f, 0.0f, 0.0f); n0.setJointType(JointType::FIXED);
    Node n1(L, 0.0f, 0.0f);
    std::vector<Node> nodes { n0, n1 };
    std::vector<Beam> beams { makeBeam(0, 1, E, A, I) };
    nodes[1].applyForce(glm::vec3(0.0f, -P, 0.0f));

    FrameSimulator sim(nodes, beams);
    sim.solve();

    auto u = sim.getNodeTranslations();
    auto r = sim.getNodeRotations();
    const float vy   = -P * L*L*L / (3.0f * E * I);
    const float slope = -P * L*L  / (2.0f * E * I);
    EXPECT_NEAR(u[1].y, vy,    std::abs(vy)    * 1e-3f);
    EXPECT_NEAR(r[1].z, slope, std::abs(slope) * 1e-3f);

    glm::vec3 rf = sim.getNodeReactionForce(0);
    glm::vec3 rm = sim.getNodeReactionMoment(0);
    EXPECT_NEAR(rf.y, P, 1e-1f);                 // vertical reaction balances load
    EXPECT_NEAR(std::abs(rm.z), P * L, 1.0f);    // fixed-end moment magnitude

    glm::vec3 net;
    EXPECT_TRUE(sim.checkForceEquilibrium(net));
}

// Same cantilever modelled with two elements (L/2 each): assembly across an
// interior node must reproduce the identical exact tip deflection.
TEST(Frame, TwoElementCantileverMatchesOneElement) {
    const float E = 200e9f, A = 1e-2f, I = 1e-5f, L = 2.0f, P = 1000.0f;
    Node n0(0.0f, 0.0f, 0.0f); n0.setJointType(JointType::FIXED);
    Node n1(L*0.5f, 0.0f, 0.0f);
    Node n2(L,      0.0f, 0.0f);
    std::vector<Node> nodes { n0, n1, n2 };
    std::vector<Beam> beams { makeBeam(0,1,E,A,I), makeBeam(1,2,E,A,I) };
    nodes[2].applyForce(glm::vec3(0.0f, -P, 0.0f));

    FrameSimulator sim(nodes, beams);
    sim.solve();

    auto u = sim.getNodeTranslations();
    const float vy = -P * L*L*L / (3.0f * E * I);
    EXPECT_NEAR(u[2].y, vy, std::abs(vy) * 1e-3f);
}

// Cantilever along +X, fixed base, with a concentrated moment Mz applied at the
// free tip. Closed form: tip slope θz = ML/EI, tip deflection vy = ML²/(2EI),
// and the base carries an equal and opposite reaction moment −M.
TEST(Frame, CantileverTipMoment) {
    const float E = 200e9f, A = 1e-2f, I = 1e-5f, L = 2.0f, M = 5000.0f;
    Node n0(0.0f, 0.0f, 0.0f); n0.setJointType(JointType::FIXED);
    Node n1(L, 0.0f, 0.0f);
    std::vector<Node> nodes { n0, n1 };
    std::vector<Beam> beams { makeBeam(0, 1, E, A, I) };
    nodes[1].applyMoment(glm::vec3(0.0f, 0.0f, M));   // about global Z

    FrameSimulator sim(nodes, beams);
    sim.solve();

    auto u = sim.getNodeTranslations();
    auto r = sim.getNodeRotations();
    const float theta = M * L / (E * I);
    const float vy    = M * L * L / (2.0f * E * I);
    EXPECT_NEAR(r[1].z, theta, std::abs(theta) * 1e-3f);
    EXPECT_NEAR(u[1].y, vy,    std::abs(vy)    * 1e-3f);

    glm::vec3 rm = sim.getNodeReactionMoment(0);
    EXPECT_NEAR(rm.z, -M, std::abs(M) * 1e-3f);        // base balances the applied moment
}

// Axial load on a frame element must still behave like a bar: δ = PL/(AE).
TEST(Frame, AxialBehavesLikeBar) {
    const float E = 200e9f, A = 1e-4f, I = 1e-6f, L = 2.0f, P = 1000.0f;
    Node n0(0.0f, 0.0f, 0.0f); n0.setJointType(JointType::FIXED);
    Node n1(L, 0.0f, 0.0f);
    std::vector<Node> nodes { n0, n1 };
    std::vector<Beam> beams { makeBeam(0, 1, E, A, I) };
    nodes[1].applyForce(glm::vec3(P, 0.0f, 0.0f));

    FrameSimulator sim(nodes, beams);
    sim.solve();

    auto u = sim.getNodeTranslations();
    const float dx = P * L / (A * E);
    EXPECT_NEAR(u[1].x, dx, std::abs(dx) * 1e-3f);
    EXPECT_NEAR(sim.getNodeReactionForce(0).x, -P, 1e-1f);
}
