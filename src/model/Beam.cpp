// Copyright (c) 2026 Colile Sibanda. All rights reserved.
// Proprietary — see LICENSE for terms. Unauthorised use prohibited.
#include "model/Beam.hpp"
#include <glm/glm.hpp>

// Material-preset constructor.
Beam::Beam(Node* start, Node* end, BeamMaterial mat, float A, float I)
    : startNode(start), endNode(end),
      youngsModulus(defaultE(mat)), crossSection(A),
      momentOfInertia(I), material(mat) {}

// Legacy constructor (used by tests and old code).
Beam::Beam(Node* start, Node* end, float E, float A)
    : startNode(start), endNode(end),
      youngsModulus(E), crossSection(A),
      momentOfInertia(8.33e-9f), material(BeamMaterial::CUSTOM) {}

Node* Beam::getStart() const { return startNode; }
Node* Beam::getEnd()   const { return endNode;   }

float Beam::getLength() const {
    return glm::length(endNode->getPosition() - startNode->getPosition());
}

float Beam::getStiffness() const {
    float L = getLength();
    return (L > 1e-8f) ? (youngsModulus * crossSection) / L : 0.0f;
}
