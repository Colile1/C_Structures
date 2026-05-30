// Copyright (c) 2026 Colile Sibanda. All rights reserved.
// Proprietary — see LICENSE for terms. Unauthorised use prohibited.
// File: Beam.cpp
// Purpose: Beam connectivity (by node index) and derived axial properties.
#include "model/Beam.hpp"
#include <glm/glm.hpp>

// Material-preset constructor.
Beam::Beam(int startIdx, int endIdx, BeamMaterial mat, float A, float I)
    : startNode(startIdx), endNode(endIdx),
      youngsModulus(defaultE(mat)), crossSection(A),
      momentOfInertia(I), material(mat) {}

// Legacy constructor (used by tests and old code).
Beam::Beam(int startIdx, int endIdx, float E, float A)
    : startNode(startIdx), endNode(endIdx),
      youngsModulus(E), crossSection(A),
      momentOfInertia(8.33e-9f), material(BeamMaterial::CUSTOM) {}

// getLength
// Purpose: Euclidean distance between the beam's two endpoint nodes.
// Inputs:  nodes — the model node vector this beam indexes into.
// Output:  length in metres (0 if either index is out of range).
float Beam::getLength(const std::vector<Node>& nodes) const {
    if (startNode < 0 || endNode < 0 ||
        startNode >= static_cast<int>(nodes.size()) ||
        endNode   >= static_cast<int>(nodes.size()))
        return 0.0f;
    return glm::length(nodes[endNode].getPosition() -
                       nodes[startNode].getPosition());
}

// getStiffness
// Purpose: axial stiffness AE/L of the member.
// Inputs:  nodes — the model node vector this beam indexes into.
// Output:  stiffness in N/m (0 for a degenerate zero-length member).
float Beam::getStiffness(const std::vector<Node>& nodes) const {
    float L = getLength(nodes);
    return (L > 1e-8f) ? (youngsModulus * crossSection) / L : 0.0f;
}
