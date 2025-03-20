#include "visualization/ForceRenderer.hpp"
#include <algorithm>
#include <glm/glm.hpp> // Include for glm types

glm::vec3 ForceRenderer::getBeamColor(float force, float maxStress) {
    float t = std::clamp(force / maxStress, -1.0f, 1.0f);
    return t > 0 ? TENSION_COLOR * t : COMPRESSION_COLOR * -t;
}

void ForceRenderer::renderForceVectors(const std::vector<Node>& nodes) {
    for (const auto& node : nodes) {
        if (glm::length(node.getAppliedForce()) > 0.001f) {
            drawArrow(node.getPosition(), 
                      node.getPosition() + node.getAppliedForce() * 0.001f,
                      COMPRESSION_COLOR);
        }
    }
}
    for (const auto& node : nodes) {
        if (glm::length(node.getAppliedForce()) > 0.001f) {
            drawArrow(node.getPosition(), 
                      node.getPosition() + node.getAppliedForce() * 0.001f,
                      COMPRESSION_COLOR);
        void drawArrow(glm::vec3 start, glm::vec3 end, glm::vec3 color) {
    // Shaft
    drawCylinder(start, end, 0.02f, color);
    
    // Arrowhead
    glm::vec3 direction = glm::normalize(end - start);
    glm::vec3 tip = end + direction * 0.1f; // Adjust the length of the arrowhead
    drawCone(end, tip, 0.05f, color); // Assuming drawCone is defined elsewhere
}
    }
}
    for (const auto& node : nodes) {
        if (glm::length(node.getAppliedForce()) > 0.001f) {
            drawArrow(node.getPosition(), 
                      node.getPosition() + node.getAppliedForce() * 0.001f,
                      COMPRESSION_COLOR);
        void drawArrow(glm::vec3 start, glm::vec3 end, glm::vec3 color) {
    // Shaft
    drawCylinder(start, end, 0.02f, color);
    
    // Arrowhead
    glm::vec3 direction = glm::normalize(end - start);
    glm::vec3 tip = end + direction * 0.1f; // Adjust the length of the arrowhead
    drawCone(end, tip, 0.05f, color); // Assuming drawCone is defined elsewhere
}
    }
}
