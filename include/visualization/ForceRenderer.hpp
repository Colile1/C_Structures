#pragma once
#include <glm/glm.hpp>
#include <vector>
#include "../model/Beam.hpp"

class ForceRenderer {
public:
    static glm::vec3 getBeamColor(float force, float maxStress);
    static void renderForceVectors(const std::vector<Node>& nodes);
    
    // Color scheme constants
    static constexpr glm::vec3 TENSION_COLOR = {0.0f, 0.0f, 1.0f}; // Blue
    static constexpr glm::vec3 COMPRESSION_COLOR = {1.0f, 0.0f, 0.0f}; // Red
    static constexpr float MAX_STRESS = 1e6f; // 1 MPa (adjust based on material)
};
