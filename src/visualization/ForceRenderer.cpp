#include "visualization/ForceRenderer.hpp"
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <cmath>

ForceRenderer::ForceRenderer() : cylinderVAO(0), cylinderVBO(0), coneVAO(0), coneVBO(0) {
}

ForceRenderer::~ForceRenderer() {
    if (cylinderVAO) {
        glDeleteVertexArrays(1, &cylinderVAO);
        glDeleteBuffers(1, &cylinderVBO);
    }
    
    if (coneVAO) {
        glDeleteVertexArrays(1, &coneVAO);
        glDeleteBuffers(1, &coneVBO);
    }
}

void ForceRenderer::initialize() {
    // Load shaders
    forceShader.load("shaders/force_vertex.glsl", "shaders/force_fragment.glsl");
    
    // Generate meshes for arrow rendering
    generateCylinderMesh();
    generateConeMesh();
}

glm::vec3 ForceRenderer::getBeamColor(float force, float maxStress) {
    // Normalize force to [-1, 1] range
    float normalizedForce = std::clamp(force / maxStress, -1.0f, 1.0f);
    
    if (std::abs(normalizedForce) < 0.05f) {
        // Near-zero forces are neutral color
        return NEUTRAL_COLOR;
    } else if (normalizedForce > 0) {
        // Tension (positive force) - blue
        return NEUTRAL_COLOR + (TENSION_COLOR - NEUTRAL_COLOR) * normalizedForce;
    } else {
        // Compression (negative force) - red
        return NEUTRAL_COLOR + (COMPRESSION_COLOR - NEUTRAL_COLOR) * -normalizedForce;
    }
}

void ForceRenderer::renderForceVectors(const std::vector<Node>& nodes, 
                                      const glm::mat4& view, 
                                      const glm::mat4& projection) {
    for (const auto& node : nodes) {
        glm::vec3 force = node.getAppliedForce();
        float magnitude = glm::length(force);
        
        // Only draw arrows for significant forces
        if (magnitude > 0.001f) {
            glm::vec3 start = node.getPosition();
            glm::vec3 end = start + glm::normalize(force) * magnitude * FORCE_SCALE;
            
            // Use red for applied forces
            drawArrow(start, end, COMPRESSION_COLOR, view, projection);
        }
    }
}

void ForceRenderer::drawArrow(const glm::vec3& start, const glm::vec3& end, 
                             const glm::vec3& color,
                             const glm::mat4& view, 
                             const glm::mat4& projection) {
    glm::vec3 direction = end - start;
    float length = glm::length(direction);
    
    if (length < 0.0001f) return; // Avoid zero-length arrows
    
    direction = glm::normalize(direction);
    
    // Calculate rotation to align with direction
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 right = glm::cross(direction, up);
    
    // Handle case where direction is parallel to up
    if (glm::length(right) < 0.001f) {
        up = glm::vec3(1.0f, 0.0f, 0.0f);
        right = glm::cross(direction, up);
    }
    
    right = glm::normalize(right);
    up = glm::normalize(glm::cross(right, direction));
    
    // Create rotation matrix
    glm::mat4 rotationMatrix(1.0f);
    rotationMatrix[0] = glm::vec4(right, 0.0f);
    rotationMatrix[1] = glm::vec4(up, 0.0f);
    rotationMatrix[2] = glm::vec4(direction, 0.0f);
    
    // Draw cylinder (shaft)
    float shaftLength = length * 0.8f; // Shaft is 80% of total length
    float shaftRadius = length * 0.02f; // Radius proportional to length
    
    glm::mat4 shaftModel = glm::mat4(1.0f);
    shaftModel = glm::translate(shaftModel, start);
    shaftModel = shaftModel * rotationMatrix;
    shaftModel = glm::scale(shaftModel, glm::vec3(shaftRadius, shaftLength, shaftRadius));
    
    forceShader.use();
    forceShader.setMat4("model", shaftModel);
    forceShader.setMat4("view", view);
    forceShader.setMat4("projection", projection);
    forceShader.setVec3("forceColor", color);
    
    glBindVertexArray(cylinderVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36 * 6); // 36 triangles per cylinder
    
    // Draw cone (arrowhead)
    float coneLength = length * 0.2f; // Cone is 20% of total length
    float coneRadius = shaftRadius * 2.5f; // Wider than shaft
    
    glm::mat4 coneModel = glm::mat4(1.0f);
    coneModel = glm::translate(coneModel, start + direction * shaftLength);
    coneModel = coneModel * rotationMatrix;
    coneModel = glm::scale(coneModel, glm::vec3(coneRadius, coneLength, coneRadius));
    
    forceShader.setMat4("model", coneModel);
    
    glBindVertexArray(coneVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36 * 3); // 36 triangles per cone
    
    glBindVertexArray(0);
}

void ForceRenderer::generateCylinderMesh() {
    // Create a simple cylinder mesh
    std::vector<float> vertices;
    const int segments = 12;
    
    for (int i = 0; i < segments; i++) {
        float angle1 = 2.0f * M_PI * i / segments;
        float angle2 = 2.0f * M_PI * ((i + 1) % segments) / segments;
        
        float x1 = std::cos(angle1);
        float z1 = std::sin(angle1);
        float x2 = std::cos(angle2);
        float z2 = std::sin(angle2);
        
        // Bottom face
        vertices.push_back(0.0f); vertices.push_back(0.0f); vertices.push_back(0.0f);
        vertices.push_back(x1); vertices.push_back(0.0f); vertices.push_back(z1);
        vertices.push_back(x2); vertices.push_back(0.0f); vertices.push_back(z2);
        
        // Top face
        vertices.push_back(0.0f); vertices.push_back(1.0f); vertices.push_back(0.0f);
        vertices.push_back(x1); vertices.push_back(1.0f); vertices.push_back(z1);
        vertices.push_back(x2); vertices.push_back(1.0f); vertices.push_back(z2);
        
        // Side faces (2 triangles per side)
        vertices.push_back(x1); vertices.push_back(0.0f); vertices.push_back(z1);
        vertices.push_back(x1); vertices.push_back(1.0f); vertices.push_back(z1);
        vertices.push_back(x2); vertices.push_back(1.0f); vertices.push_back(z2);
        
        vertices.push_back(x1); vertices.push_back(0.0f); vertices.push_back(z1);
        vertices.push_back(x2); vertices.push_back(1.0f); vertices.push_back(z2);
        vertices.push_back(x2); vertices.push_back(0.0f); vertices.push_back(z2);
    }
    
    glGenVertexArrays(1, &cylinderVAO);
    glGenBuffers(1, &cylinderVBO);
    
    glBindVertexArray(cylinderVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cylinderVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void ForceRenderer::generateConeMesh() {
    // Create a simple cone mesh
    std::vector<float> vertices;
    const int segments = 12;
    
    for (int i = 0; i < segments; i++) {
        float angle1 = 2.0f * M_PI * i / segments;
        float angle2 = 2.0f * M_PI * ((i + 1) % segments) / segments;
        
        float x1 = std::cos(angle1);
        float z1 = std::sin(angle1);
        float x2 = std::cos(angle2);
        float z2 = std::sin(angle2);
        
        // Bottom face
        vertices.push_back(0.0f); vertices.push_back(0.0f); vertices.push_back(0.0f);
        vertices.push_back(x1); vertices.push_back(0.0f); vertices.push_back(z1);
        vertices.push_back(x2); vertices.push_back(0.0f); vertices.push_back(z2);
        
        // Side faces
        vertices.push_back(0.0f); vertices.push_back(1.0f); vertices.push_back(0.0f); // Tip
        vertices.push_back(x1); vertices.push_back(0.0f); vertices.push_back(z1);
        vertices.push_back(x2); vertices.push_back(0.0f); vertices.push_back(z2);
    }
    
    glGenVertexArrays(1, &coneVAO);
    glGenBuffers(1, &coneVBO);
    
    glBindVertexArray(coneVAO);
    glBindBuffer(GL_ARRAY_BUFFER, coneVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}
