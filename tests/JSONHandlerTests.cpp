// Copyright (c) 2026 Colile Sibanda. All rights reserved.
// Proprietary — see LICENSE for terms. Unauthorised use prohibited.
// JSONHandlerTests.cpp : round-trip tests for the JSON project format.
#include <gtest/gtest.h>
#include <cstdio>
#include <fstream>
#include "data/JSONHandler.hpp"
#include "model/Node.hpp"
#include "model/Beam.hpp"
#include "physics/DistributedLoad.hpp"

static const std::string kPath = "test_json_roundtrip_tmp.json";

struct JSONHandlerFixture : ::testing::Test {
    void TearDown() override { std::remove(kPath.c_str()); }
};

// ── Node round-trip ────────────────────────────────────────────────────────────

TEST_F(JSONHandlerFixture, NodePositionAndJointType) {
    std::vector<Node> orig;
    orig.emplace_back(1.0f, 2.0f, 3.0f); orig.back().setJointType(JointType::FIXED);
    orig.emplace_back(4.0f, 0.0f, 0.0f); orig.back().setJointType(JointType::PIN_XY);
    orig.emplace_back(0.0f, 5.0f, 0.0f); orig.back().setJointType(JointType::ROLLER_Y);
    std::vector<Beam> beams;
    std::vector<DistributedLoad> dl;
    ViewPrefs vp;

    ASSERT_TRUE(JSONHandler::saveProject(kPath, orig, beams, dl, vp));

    std::vector<Node> loaded; std::vector<Beam> lb; std::vector<DistributedLoad> ldl;
    ViewPrefs lvp;
    ASSERT_TRUE(JSONHandler::loadProject(kPath, loaded, lb, ldl, lvp));

    ASSERT_EQ(loaded.size(), 3u);
    EXPECT_NEAR(loaded[0].getPosition().x, 1.0f, 1e-4f);
    EXPECT_NEAR(loaded[0].getPosition().y, 2.0f, 1e-4f);
    EXPECT_NEAR(loaded[0].getPosition().z, 3.0f, 1e-4f);
    EXPECT_EQ(loaded[0].getJointType(), JointType::FIXED);
    EXPECT_EQ(loaded[1].getJointType(), JointType::PIN_XY);
    EXPECT_EQ(loaded[2].getJointType(), JointType::ROLLER_Y);
}

TEST_F(JSONHandlerFixture, NodeAppliedForceAndMoment) {
    std::vector<Node> orig;
    orig.emplace_back(0.0f, 0.0f, 0.0f);
    orig.back().applyForce({100.0f, -500.0f, 0.0f});
    orig.back().applyMoment({0.0f, 0.0f, 1500.0f});
    std::vector<Beam> b; std::vector<DistributedLoad> dl; ViewPrefs vp;

    ASSERT_TRUE(JSONHandler::saveProject(kPath, orig, b, dl, vp));

    std::vector<Node> loaded; std::vector<Beam> lb;
    std::vector<DistributedLoad> ldl; ViewPrefs lvp;
    ASSERT_TRUE(JSONHandler::loadProject(kPath, loaded, lb, ldl, lvp));

    ASSERT_EQ(loaded.size(), 1u);
    EXPECT_NEAR(loaded[0].getAppliedForce().x,   100.0f, 1e-3f);
    EXPECT_NEAR(loaded[0].getAppliedForce().y,  -500.0f, 1e-3f);
    EXPECT_NEAR(loaded[0].getAppliedMoment().z, 1500.0f, 1e-3f);
}

// ── Beam round-trip ────────────────────────────────────────────────────────────

TEST_F(JSONHandlerFixture, BeamAllFields) {
    std::vector<Node> nodes;
    nodes.emplace_back(0.0f, 0.0f, 0.0f);
    nodes.emplace_back(3.0f, 0.0f, 0.0f);

    std::vector<Beam> orig;
    orig.emplace_back(0, 1, 70e9f, 2e-4f);
    orig.back().setMomentOfInertia(1.5e-8f);
    orig.back().setDensity(2700.0f);
    orig.back().setStartMomentRelease(true);
    orig.back().setEndMomentRelease(false);
    std::vector<DistributedLoad> dl; ViewPrefs vp;

    ASSERT_TRUE(JSONHandler::saveProject(kPath, nodes, orig, dl, vp));

    std::vector<Node> ln; std::vector<Beam> lb;
    std::vector<DistributedLoad> ldl; ViewPrefs lvp;
    ASSERT_TRUE(JSONHandler::loadProject(kPath, ln, lb, ldl, lvp));

    ASSERT_EQ(lb.size(), 1u);
    EXPECT_EQ(lb[0].getStartIdx(), 0);
    EXPECT_EQ(lb[0].getEndIdx(),   1);
    EXPECT_NEAR(lb[0].getYoungsModulus(),   70e9f,  1e3f);
    EXPECT_NEAR(lb[0].getCrossSection(),    2e-4f,  1e-8f);
    EXPECT_NEAR(lb[0].getMomentOfInertia(), 1.5e-8f, 1e-12f);
    EXPECT_NEAR(lb[0].getDensity(),         2700.0f, 0.1f);
    EXPECT_TRUE(lb[0].getStartMomentRelease());
    EXPECT_FALSE(lb[0].getEndMomentRelease());
}

TEST_F(JSONHandlerFixture, BeamMaterialPreset) {
    std::vector<Node> nodes;
    nodes.emplace_back(0.0f, 0.0f, 0.0f);
    nodes.emplace_back(1.0f, 0.0f, 0.0f);
    std::vector<Beam> orig;
    orig.emplace_back(0, 1, BeamMaterial::TIMBER);
    std::vector<DistributedLoad> dl; ViewPrefs vp;

    ASSERT_TRUE(JSONHandler::saveProject(kPath, nodes, orig, dl, vp));

    std::vector<Node> ln; std::vector<Beam> lb;
    std::vector<DistributedLoad> ldl; ViewPrefs lvp;
    ASSERT_TRUE(JSONHandler::loadProject(kPath, ln, lb, ldl, lvp));

    ASSERT_EQ(lb.size(), 1u);
    EXPECT_EQ(lb[0].getMaterial(), BeamMaterial::TIMBER);
    EXPECT_NEAR(lb[0].getYoungsModulus(), 12e9f, 1e3f);
}

// ── Distributed load round-trip ────────────────────────────────────────────────

TEST_F(JSONHandlerFixture, DistributedLoadRoundTrip) {
    std::vector<Node> nodes;
    nodes.emplace_back(0.0f, 0.0f, 0.0f);
    nodes.emplace_back(4.0f, 0.0f, 0.0f);
    std::vector<Beam> beams;
    beams.emplace_back(0, 1, 200e9f, 1e-4f);

    std::vector<DistributedLoad> dl;
    dl.push_back({ 0, LoadType::UDL, {0.0f, -1.0f, 0.0f}, 5000.0f, 0.0f, 0.0f });
    dl.push_back({ 0, LoadType::TRIANGULAR, {0.0f, -1.0f, 0.0f}, 2000.0f, 4000.0f, 0.0f });
    ViewPrefs vp;

    ASSERT_TRUE(JSONHandler::saveProject(kPath, nodes, beams, dl, vp));

    std::vector<Node> ln; std::vector<Beam> lb;
    std::vector<DistributedLoad> ldl; ViewPrefs lvp;
    ASSERT_TRUE(JSONHandler::loadProject(kPath, ln, lb, ldl, lvp));

    ASSERT_EQ(ldl.size(), 2u);
    EXPECT_EQ(ldl[0].type,    LoadType::UDL);
    EXPECT_EQ(ldl[0].beamIdx, 0);
    EXPECT_NEAR(ldl[0].w,     5000.0f, 0.1f);
    EXPECT_NEAR(ldl[0].direction.y, -1.0f, 1e-4f);
    EXPECT_EQ(ldl[1].type,    LoadType::TRIANGULAR);
    EXPECT_NEAR(ldl[1].w,     2000.0f, 0.1f);
    EXPECT_NEAR(ldl[1].w2,    4000.0f, 0.1f);
}

// ── View preferences round-trip ────────────────────────────────────────────────

TEST_F(JSONHandlerFixture, ViewPrefsRoundTrip) {
    std::vector<Node> n; std::vector<Beam> b; std::vector<DistributedLoad> dl;
    ViewPrefs orig;
    orig.useFrameMode    = true;
    orig.showDiagram     = false;
    orig.diagramType     = 2;
    orig.beginnerMode    = false;
    orig.showForceLabels = false;
    orig.showGlassBox    = true;
    orig.selfWeight      = true;
    orig.showPalette     = false;

    ASSERT_TRUE(JSONHandler::saveProject(kPath, n, b, dl, orig));

    std::vector<Node> ln; std::vector<Beam> lb; std::vector<DistributedLoad> ldl;
    ViewPrefs loaded;
    ASSERT_TRUE(JSONHandler::loadProject(kPath, ln, lb, ldl, loaded));

    EXPECT_EQ(loaded.useFrameMode,    orig.useFrameMode);
    EXPECT_EQ(loaded.showDiagram,     orig.showDiagram);
    EXPECT_EQ(loaded.diagramType,     orig.diagramType);
    EXPECT_EQ(loaded.beginnerMode,    orig.beginnerMode);
    EXPECT_EQ(loaded.showForceLabels, orig.showForceLabels);
    EXPECT_EQ(loaded.showGlassBox,    orig.showGlassBox);
    EXPECT_EQ(loaded.selfWeight,      orig.selfWeight);
    EXPECT_EQ(loaded.showPalette,     orig.showPalette);
}

// ── Error paths ────────────────────────────────────────────────────────────────

TEST(JSONHandler, LoadMissingFileReturnsFalse) {
    std::vector<Node> n; std::vector<Beam> b; std::vector<DistributedLoad> dl;
    ViewPrefs vp;
    EXPECT_FALSE(JSONHandler::loadProject("nonexistent_file.json", n, b, dl, vp));
}

TEST(JSONHandler, LoadBadJsonReturnsFalse) {
    const std::string p = "test_bad_json_tmp.json";
    { std::ofstream f(p); f << "{ this is not valid json !!!"; }
    std::vector<Node> n; std::vector<Beam> b; std::vector<DistributedLoad> dl;
    ViewPrefs vp;
    EXPECT_FALSE(JSONHandler::loadProject(p, n, b, dl, vp));
    std::remove(p.c_str());
}
