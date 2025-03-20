#include <gtest/gtest.h>
#include "physics/Simulator.hpp"
#include "model/Node.hpp"
#include "model/Beam.hpp"

TEST(PhysicsEngine, BasicCantilever) {
    std::vector<Node> nodes {
        Node(0,0,0).setFixed(true),
        Node(2,0,0)
    };
    std::vector<Beam> beams {Beam(&nodes[0], &nodes[1], 2e11, 0.01)};
    
    Simulator simulator(nodes, beams);
    nodes[1].applyForce(glm::vec3(1000, 0, 0));
    simulator.solveStaticForces();
    
    auto displacements = simulator.getNodeDisplacements();
    ASSERT_GT(displacements[1].x, 0.001); // Expect >1mm displacement
}
