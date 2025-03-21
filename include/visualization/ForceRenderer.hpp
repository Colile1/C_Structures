#pragma once
#include <glm/glm.hpp>
#include <vector>
#include "../model/Beam.hpp"
#include "../model/Node.hpp"
#include "../graphics/Shader.hpp"

class ForceRenderer {
public:
    ForceRenderer();
    ~ForceRenderer();
    
    // Initialize the renderer with shaders
    void initialize();
    
    // Get color based on force magnitude
    static glm::vec3 getBeamColor(float force, float maxStress);
    
    // Render force vectors for all nodes
    void renderForceVectors(const std::vector<Node>& nodes, const glm::mat4& view, const glm::mat4& projection);
    
    // Draw an arrow representing a force
    void drawArrow(const glm::vec3& start, const glm::vec3& end, const glm::vec3& color, 
                  const glm::mat4& view, const glm::mat4& projection);
    
    // Color scheme constants
    static constexpr glm::vec3 TENSION_COLOR = {0.0f, 0.0f, 1.0f}; // Blue
    static constexpr glm::vec3 COMPRESSION_COLOR = {1.0f, 0.0f, 0.0f}; // Red
    static constexpr glm::vec3 NEUTRAL_COLOR = {0.7f, 0.7f, 0.7f}; // Gray
    static constexpr float MAX_STRESS = 1e6f; // 1 MPa (adjust based on material)
    static constexpr float FORCE_SCALE = 0.001f; // Scale factor for force vectors

private:
    Shader forceShader;
    
    // VAOs and VBOs for arrow rendering
    unsigned int cylinderVAO, cylinderVBO;
    unsigned int coneVAO, coneVBO;
    
    // Generate cylinder and cone meshes for arrows
    void generateCylinderMesh();
    void generateConeMesh();
};
