#include <gtest/gtest.h>
#include <vector>
#include "physics/Determinacy.hpp"
#include "model/Node.hpp"
#include "model/Beam.hpp"

// DeterminacyTests.cpp : verifies the count-based static-determinacy classifier
// against classic textbook truss configurations.

// Single bar, one fixed end: m=1, r=3, n=2 → m+r=4 < 3n=6 → unstable count,
// and the free node is held by only one member (mechanism).
TEST(Determinacy, SingleBarIsUnstable) {
    std::vector<Node> nodes;
    nodes.emplace_back(0.0f, 0.0f, 0.0f); nodes.back().setFixed(true);
    nodes.emplace_back(2.0f, 0.0f, 0.0f);
    std::vector<Beam> beams { Beam(0, 1, 2e11f, 0.01f) };

    auto d = analyzeDeterminacy(nodes, beams);
    EXPECT_EQ(d.members, 1);
    EXPECT_EQ(d.reactions, 3);
    EXPECT_EQ(d.dof, 6);
    EXPECT_EQ(d.stability, Stability::UNSTABLE);
    EXPECT_TRUE(d.hasMechanismHint);
}

// Symmetric two-bar truss, both bases fixed: m=2, r=6, n=3 → m+r=8 > 3n=9? No:
// 8 < 9 in pure 3D (the apex is free out-of-plane). Confirms the honest 3D count.
TEST(Determinacy, SymmetricTwoBarCount) {
    std::vector<Node> nodes;
    nodes.emplace_back(-2.0f, 0.0f, 0.0f); nodes.back().setFixed(true);
    nodes.emplace_back( 2.0f, 0.0f, 0.0f); nodes.back().setFixed(true);
    nodes.emplace_back( 0.0f, 3.0f, 0.0f);
    std::vector<Beam> beams { Beam(0, 2, 2e11f, 1e-4f), Beam(1, 2, 2e11f, 1e-4f) };

    auto d = analyzeDeterminacy(nodes, beams);
    EXPECT_EQ(d.members, 2);
    EXPECT_EQ(d.reactions, 6);
    EXPECT_EQ(d.dof, 9);
    // 2 free members hold the apex (degree 2) so no obvious mechanism flag,
    // but the count is deficient out-of-plane → unstable in full 3D.
    EXPECT_FALSE(d.hasMechanismHint);
    EXPECT_EQ(d.stability, Stability::UNSTABLE);
}

// A statically determinate space truss: a tetrahedron with 3 supported nodes.
// 4 nodes, 6 members, supports giving r=6 → m+r = 12 = 3n. Determinate.
TEST(Determinacy, TetrahedronIsDeterminate) {
    std::vector<Node> nodes;
    nodes.emplace_back(0.0f, 0.0f, 0.0f); nodes.back().setJointType(JointType::FIXED);   // r=3
    nodes.emplace_back(1.0f, 0.0f, 0.0f); nodes.back().setJointType(JointType::PIN_XY);   // r=2
    nodes.emplace_back(0.0f, 1.0f, 0.0f); nodes.back().setJointType(JointType::ROLLER_Z); // r=1
    nodes.emplace_back(0.0f, 0.0f, 1.0f);                                                 // free
    std::vector<Beam> beams {
        Beam(0,1,2e11f,1e-4f), Beam(0,2,2e11f,1e-4f), Beam(0,3,2e11f,1e-4f),
        Beam(1,2,2e11f,1e-4f), Beam(1,3,2e11f,1e-4f), Beam(2,3,2e11f,1e-4f)
    };
    auto d = analyzeDeterminacy(nodes, beams);
    EXPECT_EQ(d.members, 6);
    EXPECT_EQ(d.reactions, 6);
    EXPECT_EQ(d.dof, 12);
    EXPECT_EQ(d.degree, 0);
    EXPECT_EQ(d.stability, Stability::DETERMINATE);
}

// Add one redundant member to the tetrahedron → indeterminate to degree 1.
TEST(Determinacy, ExtraMemberIsIndeterminate) {
    std::vector<Node> nodes;
    nodes.emplace_back(0.0f, 0.0f, 0.0f); nodes.back().setJointType(JointType::FIXED);
    nodes.emplace_back(1.0f, 0.0f, 0.0f); nodes.back().setJointType(JointType::PIN_XY);
    nodes.emplace_back(0.0f, 1.0f, 0.0f); nodes.back().setJointType(JointType::ROLLER_Z);
    nodes.emplace_back(0.0f, 0.0f, 1.0f);
    nodes.emplace_back(1.0f, 1.0f, 1.0f); nodes.back().setJointType(JointType::FREE);
    std::vector<Beam> beams {
        Beam(0,1,2e11f,1e-4f), Beam(0,2,2e11f,1e-4f), Beam(0,3,2e11f,1e-4f),
        Beam(1,2,2e11f,1e-4f), Beam(1,3,2e11f,1e-4f), Beam(2,3,2e11f,1e-4f),
        // node 4 tied by 3 members (stable) plus one extra to make it redundant
        Beam(4,1,2e11f,1e-4f), Beam(4,2,2e11f,1e-4f), Beam(4,3,2e11f,1e-4f),
        Beam(4,0,2e11f,1e-4f)
    };
    auto d = analyzeDeterminacy(nodes, beams);
    EXPECT_EQ(d.members, 10);
    EXPECT_EQ(d.reactions, 6);
    EXPECT_EQ(d.dof, 15);
    EXPECT_EQ(d.degree, 1);
    EXPECT_EQ(d.stability, Stability::INDETERMINATE);
}
