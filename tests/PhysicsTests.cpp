#include <gtest/gtest.h>
#include <glm/glm.hpp>
#include <cmath>
#include <vector>
#include "physics/Simulator.hpp"
#include "model/Node.hpp"
#include "model/Beam.hpp"

TEST(PhysicsEngine, BasicCantilever) {
    Node n0(0.0f, 0.0f, 0.0f);
    n0.setFixed(true);
    Node n1(2.0f, 0.0f, 0.0f);
    std::vector<Node> nodes { n0, n1 };
    std::vector<Beam> beams { Beam(0, 1, 2e11f, 0.01f) };

    nodes[1].applyForce(glm::vec3(10000.0f, 0.0f, 0.0f));

    Simulator sim(nodes, beams);
    sim.solveStaticForces();

    auto displacements = sim.getNodeDisplacements();
    // Fixed node should have zero displacement.
    ASSERT_NEAR(displacements[0].x, 0.0f, 1e-6f);
    // Free node under axial force should displace.
    ASSERT_GT(displacements[1].x, 0.0f);
}

TEST(PhysicsEngine, BeamForceSign) {
    Node n0(0.0f, 0.0f, 0.0f);
    n0.setFixed(true);
    Node n1(1.0f, 0.0f, 0.0f);
    std::vector<Node> nodes { n0, n1 };
    std::vector<Beam> beams { Beam(0, 1, 2e11f, 0.01f) };

    nodes[1].applyForce(glm::vec3(5000.0f, 0.0f, 0.0f)); // Tension

    Simulator sim(nodes, beams);
    sim.solveStaticForces();

    float force = sim.getBeamForce(beams[0]);
    ASSERT_GT(force, 0.0f); // Positive = tension
}

// ── Verification benchmarks (ported from REVIEW.md) ─────────────────────────────
// These guard the axial-truss solver against regressions by checking it against
// closed-form solutions that were independently confirmed in NumPy.

// Single steel bar: elongation must equal the closed-form F·L/(A·E), and the
// member force must equal the applied load (pure tension).
TEST(PhysicsVerification, SingleBarClosedForm) {
    const float E = 200e9f, A = 1e-4f, L = 2.0f, F = 1000.0f;
    Node n0(0.0f, 0.0f, 0.0f);
    n0.setFixed(true);
    Node n1(L, 0.0f, 0.0f);
    std::vector<Node> nodes { n0, n1 };
    std::vector<Beam> beams { Beam(0, 1, E, A) };
    nodes[1].applyForce(glm::vec3(F, 0.0f, 0.0f));

    Simulator sim(nodes, beams);
    sim.solveStaticForces();

    const float expectedElong = F * L / (A * E); // = 1.0e-4 m
    auto disp = sim.getNodeDisplacements();
    ASSERT_NEAR(disp[1].x, expectedElong, expectedElong * 1e-3f);
    ASSERT_NEAR(sim.getBeamForce(beams[0]), F, 1.0f); // +1000 N tension
}

// Symmetric two-bar truss: a vertical load at the apex must split equally, so
// both inclined members carry identical axial force.
TEST(PhysicsVerification, SymmetricTrussEqualForces) {
    Node base0(-2.0f, 0.0f, 0.0f); base0.setFixed(true);
    Node base1( 2.0f, 0.0f, 0.0f); base1.setFixed(true);
    Node apex ( 0.0f, 3.0f, 0.0f);
    std::vector<Node> nodes { base0, base1, apex };
    std::vector<Beam> beams { Beam(0, 2, 2e11f, 1e-4f),
                              Beam(1, 2, 2e11f, 1e-4f) };
    nodes[2].applyForce(glm::vec3(0.0f, -50000.0f, 0.0f));

    Simulator sim(nodes, beams);
    sim.solveStaticForces();

    float f0 = sim.getBeamForce(beams[0]);
    float f1 = sim.getBeamForce(beams[1]);
    ASSERT_NEAR(f0, f1, std::abs(f0) * 1e-3f + 1.0f);
    ASSERT_LT(f0, 0.0f); // both members in compression under downward apex load
}

// ── Support reactions (Phase 1.1) ───────────────────────────────────────────────
// Reaction values independently confirmed in NumPy (K·u − F at constrained DOFs).

// Single bar: the fixed support must react with −F to balance the applied load,
// and global equilibrium (Σloads + Σreactions) must close to zero.
TEST(Reactions, SingleBarBalancesLoad) {
    const float E = 200e9f, A = 1e-4f, L = 2.0f, F = 1000.0f;
    Node n0(0.0f, 0.0f, 0.0f); n0.setFixed(true);
    Node n1(L, 0.0f, 0.0f);
    std::vector<Node> nodes { n0, n1 };
    std::vector<Beam> beams { Beam(0, 1, E, A) };
    nodes[1].applyForce(glm::vec3(F, 0.0f, 0.0f));

    Simulator sim(nodes, beams);
    sim.solveStaticForces();

    glm::vec3 r0 = sim.getNodeReaction(0);
    EXPECT_NEAR(r0.x, -F, 1e-1f);   // resists the +1000 N load
    EXPECT_NEAR(r0.y, 0.0f, 1e-1f);
    EXPECT_NEAR(r0.z, 0.0f, 1e-1f);
    // Free node carries no reaction.
    glm::vec3 r1 = sim.getNodeReaction(1);
    EXPECT_NEAR(r1.x, 0.0f, 1e-1f);

    glm::vec3 net;
    EXPECT_TRUE(sim.checkEquilibrium(net));
    EXPECT_NEAR(glm::length(net), 0.0f, 1e-1f);
}

// Symmetric truss: vertical apex load splits equally to the two fixed bases —
// each carries half the vertical reaction, horizontal reactions are opposite.
TEST(Reactions, SymmetricTrussSplit) {
    Node base0(-2.0f, 0.0f, 0.0f); base0.setFixed(true);
    Node base1( 2.0f, 0.0f, 0.0f); base1.setFixed(true);
    Node apex ( 0.0f, 3.0f, 0.0f);
    std::vector<Node> nodes { base0, base1, apex };
    std::vector<Beam> beams { Beam(0, 2, 2e11f, 1e-4f),
                              Beam(1, 2, 2e11f, 1e-4f) };
    nodes[2].applyForce(glm::vec3(0.0f, -50000.0f, 0.0f));

    Simulator sim(nodes, beams);
    sim.solveStaticForces();

    glm::vec3 rb0 = sim.getNodeReaction(0);
    glm::vec3 rb1 = sim.getNodeReaction(1);
    EXPECT_NEAR(rb0.y, 25000.0f, 1.0f);
    EXPECT_NEAR(rb1.y, 25000.0f, 1.0f);
    EXPECT_NEAR(rb0.x, -rb1.x, 1.0f);          // horizontal reactions cancel
    EXPECT_NEAR(rb0.y + rb1.y, 50000.0f, 1.0f); // balance the 50 kN load

    glm::vec3 net;
    EXPECT_TRUE(sim.checkEquilibrium(net));
}
