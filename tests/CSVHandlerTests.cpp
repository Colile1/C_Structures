#include <gtest/gtest.h>
#include <glm/glm.hpp>
#include <fstream>
#include <cstdio>
#include "data/CSVHandler.hpp"
#include "model/Node.hpp"
#include "model/Beam.hpp"

// CSVHandlerTests.cpp : verifies save/load round-trips preserve all structural data.

// Pure: writes a known structure, reads it back, returns true if data matches.
static bool roundTripMatches(const std::string& path) {
    std::vector<Node> original;
    original.emplace_back(0.0f, 0.0f, 0.0f); original.back().setFixed(true);
    original.emplace_back(2.0f, 0.0f, 0.0f);
    original.emplace_back(4.0f, 1.0f, 0.0f);

    std::vector<Beam> origBeams;
    origBeams.emplace_back(&original[0], &original[1], 2e11f, 0.01f);
    origBeams.emplace_back(&original[1], &original[2], 1.5e11f, 0.005f);

    CSVHandler::saveStructure(path, original, origBeams);

    std::vector<Node> loaded;
    std::vector<Beam> loadedBeams;
    CSVHandler::loadStructure(path, loaded, loadedBeams);

    if (loaded.size() != original.size())   return false;
    if (loadedBeams.size() != origBeams.size()) return false;

    // Verify node positions and fixed flags.
    for (size_t i = 0; i < original.size(); ++i) {
        auto op = original[i].getPosition();
        auto lp = loaded[i].getPosition();
        if (std::abs(op.x - lp.x) > 1e-4f) return false;
        if (std::abs(op.y - lp.y) > 1e-4f) return false;
        if (std::abs(op.z - lp.z) > 1e-4f) return false;
        if (original[i].isFixed() != loaded[i].isFixed()) return false;
    }

    // Verify beam material properties.
    for (size_t i = 0; i < origBeams.size(); ++i) {
        if (std::abs(origBeams[i].getYoungsModulus() - loadedBeams[i].getYoungsModulus()) > 1.0f)
            return false;
        if (std::abs(origBeams[i].getCrossSection() - loadedBeams[i].getCrossSection()) > 1e-8f)
            return false;
    }
    return true;
}

TEST(CSVHandler, RoundTripPreservesNodes) {
    const std::string path = "test_roundtrip_tmp.csv";
    ASSERT_TRUE(roundTripMatches(path));
    std::remove(path.c_str());
}

TEST(CSVHandler, SavedFileIsReadable) {
    const std::string path = "test_readable_tmp.csv";
    std::vector<Node> nodes;
    nodes.emplace_back(1.0f, 2.0f, 3.0f);
    nodes.back().setFixed(true);
    std::vector<Beam> beams;
    CSVHandler::saveStructure(path, nodes, beams);

    std::ifstream f(path);
    ASSERT_TRUE(f.good()) << "File should exist after save";
    std::string line;
    ASSERT_TRUE(std::getline(f, line));
    EXPECT_NE(line.find("NODE"), std::string::npos) << "File should start with NODE";
    std::remove(path.c_str());
}

TEST(CSVHandler, FixedFlagPreserved) {
    const std::string path = "test_fixed_tmp.csv";
    std::vector<Node> nodes;
    nodes.emplace_back(0.0f, 0.0f, 0.0f); nodes.back().setFixed(true);
    nodes.emplace_back(1.0f, 0.0f, 0.0f); // free
    std::vector<Beam> beams;
    CSVHandler::saveStructure(path, nodes, beams);

    std::vector<Node> loaded;
    std::vector<Beam> loadedBeams;
    CSVHandler::loadStructure(path, loaded, loadedBeams);

    ASSERT_EQ(loaded.size(), 2u);
    EXPECT_TRUE(loaded[0].isFixed());
    EXPECT_FALSE(loaded[1].isFixed());
    std::remove(path.c_str());
}

TEST(CSVHandler, BeamMaterialPropertiesPreserved) {
    const std::string path = "test_material_tmp.csv";
    std::vector<Node> nodes;
    nodes.emplace_back(0.0f, 0.0f, 0.0f);
    nodes.emplace_back(3.0f, 0.0f, 0.0f);
    std::vector<Beam> beams;
    beams.emplace_back(0, 1, 2e11f, 0.008f);

    CSVHandler::saveStructure(path, nodes, beams);

    std::vector<Node> loadedNodes;
    std::vector<Beam> loadedBeams;
    CSVHandler::loadStructure(path, loadedNodes, loadedBeams);

    ASSERT_EQ(loadedBeams.size(), 1u);
    EXPECT_NEAR(loadedBeams[0].getYoungsModulus(), 2e11f,  1e6f);
    EXPECT_NEAR(loadedBeams[0].getCrossSection(),  0.008f, 1e-6f);
    std::remove(path.c_str());
}

TEST(CSVHandler, BeamStiffnessConsistentAfterLoad) {
    const std::string path = "test_stiffness_tmp.csv";
    Node a(0.0f, 0.0f, 0.0f), b(2.0f, 0.0f, 0.0f);
    std::vector<Node> nodes { a, b };
    std::vector<Beam> origBeams { Beam(0, 1, 2e11f, 0.01f) };
    float origStiffness = origBeams[0].getStiffness(nodes);

    CSVHandler::saveStructure(path, nodes, origBeams);

    std::vector<Node> loadedNodes;
    std::vector<Beam> loadedBeams;
    CSVHandler::loadStructure(path, loadedNodes, loadedBeams);

    ASSERT_EQ(loadedBeams.size(), 1u);
    EXPECT_NEAR(loadedBeams[0].getStiffness(loadedNodes), origStiffness, 1.0f);
    std::remove(path.c_str());
}
