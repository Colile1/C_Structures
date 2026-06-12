// Copyright (c) 2026 Colile Sibanda. All rights reserved.
// Proprietary — see LICENSE for terms. Unauthorised use prohibited.
#include <gtest/gtest.h>
#include <glm/glm.hpp>
#include <vector>
#include "physics/Simulator.hpp"
#include "physics/FrameSimulator.hpp"
#include "model/Node.hpp"
#include "model/Beam.hpp"

// Two free nodes, no supports — truss should report MECHANISM.
TEST(SolverStatus, TrussUnconstrained_ReturnsMechanism) {
    std::vector<Node> nodes;
    nodes.emplace_back(0.0f, 0.0f, 0.0f);
    nodes.emplace_back(1.0f, 0.0f, 0.0f);
    nodes[1].applyForce(glm::vec3(1000.0f, 0.0f, 0.0f));
    std::vector<Beam> beams { Beam(0, 1, 2e11f, 1e-4f) };

    Simulator sim(nodes, beams);
    SolveResult r = sim.solveStaticForces();
    EXPECT_EQ(r.status, SolveStatus::MECHANISM);
    EXPECT_FALSE(r.message.empty());
}

// Fixed base — truss should succeed.
TEST(SolverStatus, TrussConstrained_ReturnsOK) {
    std::vector<Node> nodes;
    nodes.emplace_back(0.0f, 0.0f, 0.0f);
    nodes[0].setFixed(true);
    nodes.emplace_back(1.0f, 0.0f, 0.0f);
    nodes[1].applyForce(glm::vec3(1000.0f, 0.0f, 0.0f));
    std::vector<Beam> beams { Beam(0, 1, 2e11f, 1e-4f) };

    Simulator sim(nodes, beams);
    SolveResult r = sim.solveStaticForces();
    EXPECT_EQ(r.status, SolveStatus::OK);
    EXPECT_TRUE(r.message.empty());
}

// Two free nodes, no supports — frame should report MECHANISM.
TEST(SolverStatus, FrameUnconstrained_ReturnsMechanism) {
    std::vector<Node> nodes;
    nodes.emplace_back(0.0f, 0.0f, 0.0f);
    nodes.emplace_back(0.0f, 3.0f, 0.0f);
    nodes[1].applyForce(glm::vec3(1000.0f, 0.0f, 0.0f));
    std::vector<Beam> beams { Beam(0, 1, BeamMaterial::STEEL, 1e-4f) };

    FrameSimulator fs(nodes, beams);
    SolveResult r = fs.solve();
    EXPECT_EQ(r.status, SolveStatus::MECHANISM);
    EXPECT_FALSE(r.message.empty());
}

// Fixed base — frame should succeed.
TEST(SolverStatus, FrameConstrained_ReturnsOK) {
    std::vector<Node> nodes;
    nodes.emplace_back(0.0f, 0.0f, 0.0f);
    nodes[0].setJointType(JointType::FIXED);
    nodes.emplace_back(0.0f, 3.0f, 0.0f);
    nodes[1].applyForce(glm::vec3(1000.0f, 0.0f, 0.0f));
    std::vector<Beam> beams { Beam(0, 1, BeamMaterial::STEEL, 1e-4f) };

    FrameSimulator fs(nodes, beams);
    SolveResult r = fs.solve();
    EXPECT_EQ(r.status, SolveStatus::OK);
    EXPECT_TRUE(r.message.empty());
}
