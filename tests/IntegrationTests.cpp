#include <gtest/gtest.h>
#include <glm/glm.hpp>
#include <cstdio>
#include "model/Node.hpp"
#include "model/Beam.hpp"
#include "physics/Simulator.hpp"
#include "data/CSVHandler.hpp"

// IntegrationTests.cpp : tests that exercise multiple connected components together.

// ── Node + Beam + Simulator ───────────────────────────────────────────────────

TEST(Integration, SingleBeamAxialDisplacement) {
    // Steel bar, 1m long, 10kN axial load.
    // Expected displacement = F*L / (A*E) = 10000 * 1 / (0.01 * 2e11) = 5e-6 m
    Node n0(0.0f, 0.0f, 0.0f); n0.setFixed(true);
    Node n1(1.0f, 0.0f, 0.0f);
    std::vector<Node> nodes { n0, n1 };
    std::vector<Beam> beams { Beam(0, 1, 2e11f, 0.01f) };

    nodes[1].applyForce(glm::vec3(10000.0f, 0.0f, 0.0f));
    Simulator sim(nodes, beams);
    sim.solveStaticForces();

    auto disp = sim.getNodeDisplacements();
    float expected = 10000.0f * 1.0f / (0.01f * 2e11f);
    EXPECT_NEAR(disp[1].x, expected, expected * 0.01f) // within 1%
        << "Axial displacement should match F*L/(A*E)";
    EXPECT_NEAR(disp[0].x, 0.0f, 1e-8f) << "Fixed node must not move";
}

TEST(Integration, TwoBeamForceTransmission) {
    // Two equal beams in series. Force applied at free end.
    // Both beams carry the same axial force.
    Node n0(0.0f, 0.0f, 0.0f); n0.setFixed(true);
    Node n1(1.0f, 0.0f, 0.0f);
    Node n2(2.0f, 0.0f, 0.0f);
    std::vector<Node> nodes { n0, n1, n2 };
    std::vector<Beam> beams {
        Beam(0, 1, 2e11f, 0.01f),
        Beam(1, 2, 2e11f, 0.01f)
    };
    nodes[2].applyForce(glm::vec3(5000.0f, 0.0f, 0.0f));

    Simulator sim(nodes, beams);
    sim.solveStaticForces();

    float f0 = sim.getBeamForce(beams[0]);
    float f1 = sim.getBeamForce(beams[1]);

    EXPECT_GT(f0, 0.0f) << "Both beams should be in tension";
    EXPECT_GT(f1, 0.0f);
    EXPECT_NEAR(f0, f1, std::abs(f0) * 0.05f)
        << "Beams in series carry the same force";
}

TEST(Integration, ZeroForceProducesZeroDisplacement) {
    Node n0(0.0f, 0.0f, 0.0f); n0.setFixed(true);
    Node n1(1.0f, 0.0f, 0.0f);
    std::vector<Node> nodes { n0, n1 };
    std::vector<Beam> beams { Beam(0, 1, 2e11f, 0.01f) };

    // No force applied.
    Simulator sim(nodes, beams);
    sim.solveStaticForces();

    auto disp = sim.getNodeDisplacements();
    EXPECT_NEAR(disp[0].x, 0.0f, 1e-8f);
    EXPECT_NEAR(disp[1].x, 0.0f, 1e-6f)
        << "No applied force should produce no displacement";
}

TEST(Integration, CompressionGivesNegativeBeamForce) {
    Node n0(0.0f, 0.0f, 0.0f); n0.setFixed(true);
    Node n1(1.0f, 0.0f, 0.0f);
    std::vector<Node> nodes { n0, n1 };
    std::vector<Beam> beams { Beam(0, 1, 2e11f, 0.01f) };

    nodes[1].applyForce(glm::vec3(-5000.0f, 0.0f, 0.0f)); // compressive
    Simulator sim(nodes, beams);
    sim.solveStaticForces();

    float force = sim.getBeamForce(beams[0]);
    EXPECT_LT(force, 0.0f) << "Compressive load should yield negative beam force";
}

TEST(Integration, ForceAccumulatesAcrossMultipleApplyCalls) {
    Node n(0.0f, 0.0f, 0.0f);
    n.applyForce(glm::vec3(100.0f, 200.0f, 300.0f));
    n.applyForce(glm::vec3( 50.0f,  50.0f,  50.0f));

    EXPECT_FLOAT_EQ(n.getAppliedForce().x, 150.0f);
    EXPECT_FLOAT_EQ(n.getAppliedForce().y, 250.0f);
    EXPECT_FLOAT_EQ(n.getAppliedForce().z, 350.0f);
}

// Regression for the former dangling-pointer hazard: a Beam referenced its
// endpoints by Node*, so growing the node vector (reallocation) corrupted
// connectivity. With integer indices, adding many nodes after a beam exists
// must leave that beam pointing at the same physical endpoints.
TEST(Integration, NodeVectorReallocationKeepsConnectivity) {
    std::vector<Node> nodes;
    nodes.emplace_back(0.0f, 0.0f, 0.0f);
    nodes.emplace_back(3.0f, 4.0f, 0.0f); // 5 m from node 0
    std::vector<Beam> beams { Beam(0, 1, 2e11f, 0.01f) };

    const float before = beams[0].getLength(nodes);
    for (int k = 0; k < 300; ++k)          // force at least one reallocation
        nodes.emplace_back(static_cast<float>(k), 0.0f, 0.0f);

    EXPECT_EQ(beams[0].getStartIdx(), 0);
    EXPECT_EQ(beams[0].getEndIdx(),   1);
    EXPECT_FLOAT_EQ(beams[0].getLength(nodes), before); // still 5 m, no corruption
    EXPECT_FLOAT_EQ(before, 5.0f);
}

// ── CSV + Simulator pipeline ──────────────────────────────────────────────────

TEST(Integration, CSVThenSimulate) {
    // Save a structure, load it, solve, and verify physics result is sensible.
    const std::string path = "integration_sim_tmp.csv";

    std::vector<Node> orig;
    orig.emplace_back(0.0f, 0.0f, 0.0f); orig.back().setFixed(true);
    orig.emplace_back(2.0f, 0.0f, 0.0f);
    std::vector<Beam> origBeams { Beam(0, 1, 2e11f, 0.01f) };
    CSVHandler::saveStructure(path, orig, origBeams);

    std::vector<Node> loaded;
    std::vector<Beam> loadedBeams;
    CSVHandler::loadStructure(path, loaded, loadedBeams);

    loaded[1].applyForce(glm::vec3(8000.0f, 0.0f, 0.0f));
    Simulator sim(loaded, loadedBeams);
    sim.solveStaticForces();

    auto disp = sim.getNodeDisplacements();
    EXPECT_NEAR(disp[0].x, 0.0f, 1e-8f) << "Fixed node must not move after CSV load";
    EXPECT_GT(disp[1].x, 0.0f)          << "Free node should displace under load";

    float force = sim.getBeamForce(loadedBeams[0]);
    EXPECT_GT(force, 0.0f) << "Beam should be in tension";

    std::remove(path.c_str());
}

TEST(Integration, MultiBeamCSVRoundTrip) {
    // Three nodes, two beams — save to CSV, reload, verify beam count and connectivity.
    const std::string path = "integration_multi_tmp.csv";

    std::vector<Node> nodes;
    nodes.emplace_back(0.0f, 0.0f, 0.0f); nodes.back().setFixed(true);
    nodes.emplace_back(1.0f, 0.0f, 0.0f);
    nodes.emplace_back(2.0f, 0.0f, 0.0f);
    std::vector<Beam> beams {
        Beam(0, 1, 2e11f, 0.01f),
        Beam(1, 2, 2e11f, 0.005f)
    };
    CSVHandler::saveStructure(path, nodes, beams);

    std::vector<Node> ln;
    std::vector<Beam> lb;
    CSVHandler::loadStructure(path, ln, lb);

    ASSERT_EQ(ln.size(), 3u) << "Should load 3 nodes";
    ASSERT_EQ(lb.size(), 2u) << "Should load 2 beams";
    EXPECT_TRUE(ln[0].isFixed());
    EXPECT_NEAR(lb[1].getCrossSection(), 0.005f, 1e-7f);
    // Connectivity is stored as indices, so it survives the round-trip exactly.
    EXPECT_EQ(lb[0].getStartIdx(), 0); EXPECT_EQ(lb[0].getEndIdx(), 1);
    EXPECT_EQ(lb[1].getStartIdx(), 1); EXPECT_EQ(lb[1].getEndIdx(), 2);

    std::remove(path.c_str());
}
