#include <gtest/gtest.h>
#include <glm/glm.hpp>
#include <vector>
#include "model/Node.hpp"
#include "model/Beam.hpp"

TEST(NodeTest, PositionAndForces) {
    Node node(1.0f, 2.0f, 3.0f);
    ASSERT_FLOAT_EQ(node.getPosition().x, 1.0f);
    node.applyForce(glm::vec3(100.0f, 0.0f, 0.0f));
    ASSERT_FLOAT_EQ(node.getAppliedForce().x, 100.0f);
    node.applyForce(glm::vec3(50.0f, 0.0f, 0.0f));
    ASSERT_FLOAT_EQ(node.getAppliedForce().x, 150.0f);
}

TEST(NodeTest, Creation) {
    Node n(1.0f, 2.0f, 3.0f);
    ASSERT_FLOAT_EQ(n.getPosition().y, 2.0f);
    ASSERT_FLOAT_EQ(n.getPosition().z, 3.0f);
    ASSERT_FALSE(n.isFixed());
}

TEST(NodeTest, FixedConstraint) {
    Node n(0.0f, 0.0f, 0.0f);
    n.setFixed(true);
    ASSERT_TRUE(n.isFixed());
    n.setFixed(false);
    ASSERT_FALSE(n.isFixed());
}

TEST(BeamTest, LengthCalculation) {
    std::vector<Node> nodes { Node(0.0f, 0.0f, 0.0f), Node(3.0f, 4.0f, 0.0f) }; // 5m beam
    Beam beam(0, 1, 2e11f, 0.01f);
    ASSERT_FLOAT_EQ(beam.getLength(nodes), 5.0f);
}

TEST(BeamTest, StiffnessCalculation) {
    std::vector<Node> nodes { Node(0.0f, 0.0f, 0.0f), Node(3.0f, 0.0f, 0.0f) };
    Beam beam(0, 1, 2e11f, 0.01f); // E=200GPa, A=0.01m^2, L=3m
    float expected = (2e11f * 0.01f) / 3.0f;
    ASSERT_NEAR(beam.getStiffness(nodes), expected, 1.0f);
}

TEST(BeamTest, MaterialProperties) {
    Beam beam(0, 1, 2e11f, 0.005f);
    ASSERT_FLOAT_EQ(beam.getYoungsModulus(), 2e11f);
    ASSERT_FLOAT_EQ(beam.getCrossSection(),  0.005f);
}
