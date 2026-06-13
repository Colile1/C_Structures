// Copyright (c) 2026 Colile Sibanda. All rights reserved.
// Proprietary — see LICENSE for terms. Unauthorised use prohibited.
#include <gtest/gtest.h>
#include <glm/glm.hpp>
#include <cmath>
#include <vector>
#include "physics/Simulator.hpp"
#include "physics/FrameSimulator.hpp"
#include "model/Node.hpp"
#include "model/Beam.hpp"

// ResolveCacheTests.cpp : guards the cached-factorisation re-solve path added in
// build step A15. A simulator is kept alive across model edits and reuses its LU
// factorisation when only the loads change. These tests assert that the cached
// re-solve produces results identical to a fresh solve of the same model — i.e.
// the optimisation is invisible to the answer (the "results identical to the
// Enter-triggered path" acceptance criterion).

namespace {

// A symmetric two-bar truss: two fixed bases and a free apex.
void makeTruss(std::vector<Node>& nodes, std::vector<Beam>& beams) {
    Node base0(-2.0f, 0.0f, 0.0f); base0.setJointType(JointType::FIXED);
    Node base1( 2.0f, 0.0f, 0.0f); base1.setJointType(JointType::FIXED);
    Node apex ( 0.0f, 3.0f, 0.0f);
    nodes = { base0, base1, apex };
    beams = { Beam(0, 2, 2e11f, 1e-4f), Beam(1, 2, 2e11f, 1e-4f) };
}

float maxDispDiff(const std::vector<glm::vec3>& a, const std::vector<glm::vec3>& b) {
    float d = 0.0f;
    for (std::size_t i = 0; i < a.size() && i < b.size(); ++i)
        d = std::max(d, glm::length(a[i] - b[i]));
    return d;
}

} // namespace

// Re-solving with no change between solves must be a no-op on the results
// (the cache-hit path returns the exact same displacements and beam force).
TEST(ResolveCache, NoChangeReSolveIsIdentical) {
    std::vector<Node> nodes; std::vector<Beam> beams;
    makeTruss(nodes, beams);
    nodes[2].applyForce(glm::vec3(0.0f, -50000.0f, 0.0f));

    Simulator sim(nodes, beams);
    sim.solveStaticForces();
    auto disp1  = sim.getNodeDisplacements();
    float force1 = sim.getBeamForce(beams[0]);

    sim.solveStaticForces(); // cache hit — nothing changed
    auto disp2  = sim.getNodeDisplacements();
    float force2 = sim.getBeamForce(beams[0]);

    EXPECT_FLOAT_EQ(maxDispDiff(disp1, disp2), 0.0f);
    EXPECT_FLOAT_EQ(force1, force2);
}

// Changing only the applied load and re-solving on the cached factorisation must
// match a fresh simulator solving the same edited model bit-for-bit.
TEST(ResolveCache, LoadChangeMatchesFreshSolve) {
    std::vector<Node> nodes; std::vector<Beam> beams;
    makeTruss(nodes, beams);
    nodes[2].applyForce(glm::vec3(0.0f, -50000.0f, 0.0f));

    Simulator cached(nodes, beams);
    cached.solveStaticForces(); // primes the factorisation

    // Edit a load in place — the stiffness matrix is untouched.
    nodes[2].clearForce();
    nodes[2].applyForce(glm::vec3(8000.0f, -30000.0f, 0.0f));
    cached.solveStaticForces(); // re-solves on the cached factorisation

    // A brand-new simulator on the identical edited model is the reference path.
    Simulator fresh(nodes, beams);
    fresh.solveStaticForces();

    EXPECT_NEAR(maxDispDiff(cached.getNodeDisplacements(),
                            fresh.getNodeDisplacements()), 0.0f, 1e-12f);
    EXPECT_NEAR(cached.getBeamForce(beams[0]), fresh.getBeamForce(beams[0]), 1e-6f);
    glm::vec3 rc = cached.getNodeReaction(0), rf = fresh.getNodeReaction(0);
    EXPECT_NEAR(glm::length(rc - rf), 0.0f, 1e-6f);
}

// A geometry change must invalidate the cache and re-factorise, again matching a
// fresh solve. This exercises the signature-driven refactor path.
TEST(ResolveCache, GeometryChangeRefactorsCorrectly) {
    std::vector<Node> nodes; std::vector<Beam> beams;
    makeTruss(nodes, beams);
    nodes[2].applyForce(glm::vec3(0.0f, -50000.0f, 0.0f));

    Simulator cached(nodes, beams);
    cached.solveStaticForces();

    // Move the apex: KFF changes, so the cached factorisation must be discarded.
    nodes[2].setPosition(glm::vec3(0.5f, 3.5f, 0.0f));
    cached.solveStaticForces();

    Simulator fresh(nodes, beams);
    fresh.solveStaticForces();

    EXPECT_NEAR(maxDispDiff(cached.getNodeDisplacements(),
                            fresh.getNodeDisplacements()), 0.0f, 1e-12f);
}

// Adding a node between solves grows the system; the kept-alive simulator must
// self-resize and still match a fresh solve of the enlarged model.
TEST(ResolveCache, NodeCountChangeSelfResizes) {
    std::vector<Node> nodes; std::vector<Beam> beams;
    makeTruss(nodes, beams);
    nodes.reserve(16); // avoid reallocation so references stay valid
    nodes[2].applyForce(glm::vec3(0.0f, -50000.0f, 0.0f));

    Simulator cached(nodes, beams);
    cached.solveStaticForces();

    // Add a third fixed base and a bar to the apex — node count grows from 3 to 4.
    nodes.emplace_back(0.0f, 0.0f, 0.0f);
    nodes.back().setJointType(JointType::FIXED);
    beams.emplace_back(3, 2, 2e11f, 1e-4f);
    cached.solveStaticForces();

    Simulator fresh(nodes, beams);
    fresh.solveStaticForces();

    EXPECT_NEAR(maxDispDiff(cached.getNodeDisplacements(),
                            fresh.getNodeDisplacements()), 0.0f, 1e-12f);
}

// Frame solver: a tip-moment change re-solves on the cached factorisation and
// must match a fresh frame solve (the 6-DOF counterpart of the truss test).
TEST(ResolveCache, FrameLoadChangeMatchesFreshSolve) {
    const float E = 200e9f, A = 1e-2f, I = 1e-5f, L = 2.0f;
    Node n0(0.0f, 0.0f, 0.0f); n0.setJointType(JointType::FIXED);
    Node n1(L, 0.0f, 0.0f);
    std::vector<Node> nodes { n0, n1 };
    Beam b(0, 1, E, A); b.setMomentOfInertia(I);
    std::vector<Beam> beams { b };
    nodes[1].applyMoment(glm::vec3(0.0f, 0.0f, 5000.0f));

    FrameSimulator cached(nodes, beams);
    cached.solve();

    // Change only the applied moment.
    nodes[1].clearMoment();
    nodes[1].applyMoment(glm::vec3(0.0f, 0.0f, -12000.0f));
    cached.solve();

    FrameSimulator fresh(nodes, beams);
    fresh.solve();

    auto uc = cached.getNodeTranslations();
    auto uf = fresh.getNodeTranslations();
    auto rc = cached.getNodeRotations();
    auto rf = fresh.getNodeRotations();
    EXPECT_NEAR(maxDispDiff(uc, uf), 0.0f, 1e-12f);
    EXPECT_NEAR(maxDispDiff(rc, rf), 0.0f, 1e-12f);
}
