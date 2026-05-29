#include <gtest/gtest.h>
#include <glm/glm.hpp>
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
    Node a(0.0f, 0.0f, 0.0f), b(3.0f, 4.0f, 0.0f); // 5m beam
    Beam beam(&a, &b, 2e11f, 0.01f);
    ASSERT_FLOAT_EQ(beam.getLength(), 5.0f);
}

TEST(BeamTest, StiffnessCalculation) {
    Node n1(0.0f, 0.0f, 0.0f), n2(3.0f, 0.0f, 0.0f);
    Beam beam(&n1, &n2, 2e11f, 0.01f); // E=200GPa, A=0.01m^2, L=3m
    float expected = (2e11f * 0.01f) / 3.0f;
    ASSERT_NEAR(beam.getStiffness(), expected, 1.0f);
}

TEST(BeamTest, MaterialProperties) {
    Node n1(0.0f, 0.0f, 0.0f), n2(1.0f, 0.0f, 0.0f);
    Beam beam(&n1, &n2, 2e11f, 0.005f);
    ASSERT_FLOAT_EQ(beam.getYoungsModulus(), 2e11f);
    ASSERT_FLOAT_EQ(beam.getCrossSection(),  0.005f);
}
