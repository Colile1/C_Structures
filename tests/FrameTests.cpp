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

// Member-end releases (internal hinges): a released end transmits force but no
// bending moment. Releasing the two outer ends of a beam built from two members
// between FIXED supports turns a fixed-fixed beam into a simply supported one.
// A central point load P over span L then gives support reactions P/2, no
// support moment (a fixed end would carry PL/8), and mid-span deflection
// PL³/(48EI). The FIXED end nodes also keep the model stable out of plane.
TEST(Frame, EndReleasesGiveSimplySupportedBeam) {
    const float E = 200e9f, A = 1e-2f, I = 1e-5f, L = 4.0f, P = 2000.0f;
    Node a(0.0f,    0.0f, 0.0f);  a.setJointType(JointType::FIXED);
    Node c(L*0.5f,  0.0f, 0.0f);
    Node b(L,       0.0f, 0.0f);  b.setJointType(JointType::FIXED);
    std::vector<Node> nodes { a, c, b };

    Beam ac = makeBeam(0, 1, E, A, I); ac.setStartMomentRelease(true); // pin at A
    Beam cb = makeBeam(1, 2, E, A, I); cb.setEndMomentRelease(true);   // pin at B
    std::vector<Beam> beams { ac, cb };
    nodes[1].applyForce(glm::vec3(0.0f, -P, 0.0f));

    FrameSimulator sim(nodes, beams);
    sim.solve();

    auto u = sim.getNodeTranslations();
    const float vy = -P * L*L*L / (48.0f * E * I);
    EXPECT_NEAR(u[1].y, vy, std::abs(vy) * 1e-3f);

    EXPECT_NEAR(sim.getNodeReactionForce(0).y, P*0.5f, 1.0f);
    EXPECT_NEAR(sim.getNodeReactionForce(2).y, P*0.5f, 1.0f);
    // Hinged supports carry essentially no moment (a fixed end would show PL/8).
    EXPECT_NEAR(sim.getNodeReactionMoment(0).z, 0.0f, std::abs(P*L/8.0f) * 1e-3f);

    glm::vec3 net;
    EXPECT_TRUE(sim.checkForceEquilibrium(net));
}

// Three-hinged portal frame (the textbook member-release case): two pinned bases
// — modelled as a FIXED node with the column base moment released — and an
// internal hinge at the crown make the frame statically determinate. A downward
// load P at the crown gives vertical reactions P/2 and a horizontal thrust
// PL/(4h) at each base, with zero moment at the hinged bases, independent of EI.
TEST(Frame, ThreeHingedPortalReactions) {
    const float E = 200e9f, A = 1e-2f, I = 1e-5f;
    const float L = 4.0f, h = 3.0f, P = 1200.0f;
    Node A0(0.0f,   0.0f, 0.0f);  A0.setJointType(JointType::FIXED); // base left
    Node B (0.0f,   h,    0.0f);
    Node C (L*0.5f, h,    0.0f);                                      // crown
    Node D (L,      h,    0.0f);
    Node Eb(L,      0.0f, 0.0f);  Eb.setJointType(JointType::FIXED); // base right
    std::vector<Node> nodes { A0, B, C, D, Eb };

    Beam ab = makeBeam(0, 1, E, A, I); ab.setStartMomentRelease(true); // hinge, base left
    Beam bc = makeBeam(1, 2, E, A, I); bc.setEndMomentRelease(true);   // hinge at crown
    Beam cd = makeBeam(2, 3, E, A, I);
    Beam ed = makeBeam(4, 3, E, A, I); ed.setStartMomentRelease(true); // hinge, base right
    std::vector<Beam> beams { ab, bc, cd, ed };
    nodes[2].applyForce(glm::vec3(0.0f, -P, 0.0f));

    FrameSimulator sim(nodes, beams);
    sim.solve();

    const float V = P * 0.5f;
    const float H = P * L / (4.0f * h);
    glm::vec3 rA = sim.getNodeReactionForce(0);
    glm::vec3 rE = sim.getNodeReactionForce(4);
    EXPECT_NEAR(rA.y,  V, 1.0f);
    EXPECT_NEAR(rE.y,  V, 1.0f);
    EXPECT_NEAR(rA.x,  H, std::abs(H) * 2e-3f);   // inward thrust at left base
    EXPECT_NEAR(rE.x, -H, std::abs(H) * 2e-3f);   // inward thrust at right base
    EXPECT_NEAR(sim.getNodeReactionMoment(0).z, 0.0f, std::abs(P*L) * 1e-3f);

    glm::vec3 net;
    EXPECT_TRUE(sim.checkForceEquilibrium(net));
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
