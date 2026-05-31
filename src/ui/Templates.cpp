// Copyright (c) 2026 Colile Sibanda. All rights reserved.
// Proprietary — see LICENSE for terms. Unauthorised use prohibited.
#include "ui/Templates.hpp"

void loadTemplate(int idx, std::vector<Node>& nodes, std::vector<Beam>& beams) {
    nodes.clear();
    beams.clear();

    switch (idx) {

    // ── 0: Simple Beam ────────────────────────────────────────────────────────
    // 3 nodes along X: pin at left, free midpoint, roller at right.
    // 10 kN downward at midpoint — classic simply-supported beam.
    case 0: {
        nodes.emplace_back(0.0f, 0.0f, 0.0f); nodes.back().setJointType(JointType::FIXED);
        nodes.emplace_back(2.0f, 0.0f, 0.0f);                         // free midpoint
        nodes.emplace_back(4.0f, 0.0f, 0.0f); nodes.back().setJointType(JointType::ROLLER_Y);
        beams.emplace_back(0, 1, BeamMaterial::STEEL, 1e-4f);
        beams.emplace_back(1, 2, BeamMaterial::STEEL, 1e-4f);
        nodes[1].applyForce(glm::vec3(0.0f, -10000.0f, 0.0f));
        break;
    }

    // ── 1: Triangle Truss ─────────────────────────────────────────────────────
    // Classic symmetric pin-jointed truss: two fixed supports, free apex.
    // 50 kN downward at apex — shows tension/compression in inclined members.
    case 1: {
        nodes.emplace_back(-2.0f, 0.0f, 0.0f); nodes.back().setJointType(JointType::FIXED);
        nodes.emplace_back( 2.0f, 0.0f, 0.0f); nodes.back().setJointType(JointType::FIXED);
        nodes.emplace_back( 0.0f, 3.0f, 0.0f); // free apex
        beams.emplace_back(0, 2, BeamMaterial::STEEL, 1e-4f);
        beams.emplace_back(1, 2, BeamMaterial::STEEL, 1e-4f);
        nodes[2].applyForce(glm::vec3(0.0f, -50000.0f, 0.0f));
        break;
    }

    // ── 2: Portal Frame ───────────────────────────────────────────────────────
    // Two fixed-base columns (3 m tall), connected by a horizontal beam (4 m wide).
    // 20 kN horizontal load at top-left — classic frame with bending moments.
    case 2: {
        nodes.emplace_back(0.0f, 0.0f, 0.0f); nodes.back().setJointType(JointType::FIXED);
        nodes.emplace_back(0.0f, 3.0f, 0.0f);                         // top-left
        nodes.emplace_back(4.0f, 3.0f, 0.0f);                         // top-right
        nodes.emplace_back(4.0f, 0.0f, 0.0f); nodes.back().setJointType(JointType::FIXED);
        beams.emplace_back(0, 1, BeamMaterial::STEEL, 2e-4f);          // left column
        beams.emplace_back(1, 2, BeamMaterial::STEEL, 1e-4f);          // beam
        beams.emplace_back(3, 2, BeamMaterial::STEEL, 2e-4f);          // right column
        nodes[1].applyForce(glm::vec3(20000.0f, 0.0f, 0.0f));          // sway load
        break;
    }

    // ── 3: Cantilever ─────────────────────────────────────────────────────────
    // Fixed wall at left, free tip at right — 3 elements for bending accuracy.
    // 5 kN tip load downward — shows the classic moment diagram shape.
    case 3: {
        nodes.emplace_back(0.0f, 0.0f, 0.0f); nodes.back().setJointType(JointType::FIXED);
        nodes.emplace_back(1.0f, 0.0f, 0.0f);
        nodes.emplace_back(2.0f, 0.0f, 0.0f);
        nodes.emplace_back(3.0f, 0.0f, 0.0f);
        beams.emplace_back(0, 1, BeamMaterial::STEEL, 1e-4f);
        beams.emplace_back(1, 2, BeamMaterial::STEEL, 1e-4f);
        beams.emplace_back(2, 3, BeamMaterial::STEEL, 1e-4f);
        nodes[3].applyForce(glm::vec3(0.0f, -5000.0f, 0.0f));
        break;
    }

    default: break;
    }
}
