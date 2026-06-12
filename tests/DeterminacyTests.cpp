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

// ── Frame mode (6 DOF/node) ────────────────────────────────────────────────
// In a rigid frame a member carries 6 internal forces and a FIXED joint also
// restrains the 3 rotations; the criterion is 6m + r vs 6n.

// Cantilever: one fixed base, one free tip, one member. r=6, 6m=6, 6n=12 →
// degree 0. A cantilever is statically determinate (one rigid member holds the
// tip, so no mechanism hint despite the tip having only one member).
TEST(Determinacy, CantileverFrameIsDeterminate) {
    std::vector<Node> nodes;
    nodes.emplace_back(0.0f, 0.0f, 0.0f); nodes.back().setFixed(true); // r=6 in frame
    nodes.emplace_back(3.0f, 0.0f, 0.0f);                              // free tip
    std::vector<Beam> beams { Beam(0, 1, 2e11f, 0.01f) };

    auto d = analyzeDeterminacy(nodes, beams, /*frame=*/true);
    EXPECT_TRUE(d.frame);
    EXPECT_EQ(d.dofPerNode, 6);
    EXPECT_EQ(d.members, 1);
    EXPECT_EQ(d.memberUnknowns, 6);
    EXPECT_EQ(d.reactions, 6);
    EXPECT_EQ(d.dof, 12);
    EXPECT_EQ(d.degree, 0);
    EXPECT_FALSE(d.hasMechanismHint);
    EXPECT_EQ(d.stability, Stability::DETERMINATE);
}

// Propped cantilever: fixed base + a vertical-roller prop at the tip. r=6+1=7,
// 6m=6, 6n=12 → degree 1. Statically indeterminate to degree 1 (textbook).
TEST(Determinacy, ProppedCantileverFrameIsIndeterminate) {
    std::vector<Node> nodes;
    nodes.emplace_back(0.0f, 0.0f, 0.0f); nodes.back().setJointType(JointType::FIXED);    // r=6
    nodes.emplace_back(3.0f, 0.0f, 0.0f); nodes.back().setJointType(JointType::ROLLER_Y); // r=1
    std::vector<Beam> beams { Beam(0, 1, 2e11f, 0.01f) };

    auto d = analyzeDeterminacy(nodes, beams, /*frame=*/true);
    EXPECT_EQ(d.members, 1);
    EXPECT_EQ(d.reactions, 7);
    EXPECT_EQ(d.dof, 12);
    EXPECT_EQ(d.degree, 1);
    EXPECT_FALSE(d.hasMechanismHint);
    EXPECT_EQ(d.stability, Stability::INDETERMINATE);
}

// An unsupported frame (no supports anywhere) floats as a rigid body: the
// mechanism hint must fire and the verdict must be unstable.
TEST(Determinacy, UnsupportedFrameIsMechanism) {
    std::vector<Node> nodes;
    nodes.emplace_back(0.0f, 0.0f, 0.0f); // both free — no supports
    nodes.emplace_back(3.0f, 0.0f, 0.0f);
    std::vector<Beam> beams { Beam(0, 1, 2e11f, 0.01f) };

    auto d = analyzeDeterminacy(nodes, beams, /*frame=*/true);
    EXPECT_EQ(d.reactions, 0);
    EXPECT_TRUE(d.hasMechanismHint);
    EXPECT_EQ(d.stability, Stability::UNSTABLE);
}

// The same truss criterion must be unchanged when frame mode is off: a
// determinate space tetrahedron still classifies as determinate via 3n.
TEST(Determinacy, TrussModeUnaffectedByFrameFlag) {
    std::vector<Node> nodes;
    nodes.emplace_back(0.0f, 0.0f, 0.0f); nodes.back().setJointType(JointType::FIXED);
    nodes.emplace_back(1.0f, 0.0f, 0.0f); nodes.back().setJointType(JointType::PIN_XY);
    nodes.emplace_back(0.0f, 1.0f, 0.0f); nodes.back().setJointType(JointType::ROLLER_Z);
    nodes.emplace_back(0.0f, 0.0f, 1.0f);
    std::vector<Beam> beams {
        Beam(0,1,2e11f,1e-4f), Beam(0,2,2e11f,1e-4f), Beam(0,3,2e11f,1e-4f),
        Beam(1,2,2e11f,1e-4f), Beam(1,3,2e11f,1e-4f), Beam(2,3,2e11f,1e-4f)
    };
    auto d = analyzeDeterminacy(nodes, beams, /*frame=*/false);
    EXPECT_FALSE(d.frame);
    EXPECT_EQ(d.dofPerNode, 3);
    EXPECT_EQ(d.dof, 12);
    EXPECT_EQ(d.degree, 0);
    EXPECT_EQ(d.stability, Stability::DETERMINATE);
}
