#include <gtest/gtest.h>
#include <glm/glm.hpp>
#include "physics/Simulator.hpp"
#include "model/Node.hpp"
#include "model/Beam.hpp"

TEST(PhysicsEngine, BasicCantilever) {
    Node n0(0.0f, 0.0f, 0.0f);
    n0.setFixed(true);
    Node n1(2.0f, 0.0f, 0.0f);
    std::vector<Node> nodes { n0, n1 };
    std::vector<Beam> beams { Beam(&nodes[0], &nodes[1], 2e11f, 0.01f) };

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
    std::vector<Beam> beams { Beam(&nodes[0], &nodes[1], 2e11f, 0.01f) };

    nodes[1].applyForce(glm::vec3(5000.0f, 0.0f, 0.0f)); // Tension

    Simulator sim(nodes, beams);
    sim.solveStaticForces();

    float force = sim.getBeamForce(beams[0]);
    ASSERT_GT(force, 0.0f); // Positive = tension
}
