#include "visualization/ForceRenderer.hpp"
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <cmath>

static const char* FORCE_VERT = R"glsl(
#version 330 core
layout(location = 0) in vec3 aPos;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
)glsl";

static const char* FORCE_FRAG = R"glsl(
#version 330 core
out vec4 FragColor;
uniform vec3 forceColor;
void main() { FragColor = vec4(forceColor, 1.0); }
)glsl";

const glm::vec3 ForceRenderer::TENSION_COLOR     = {0.0f, 0.0f, 1.0f};
const glm::vec3 ForceRenderer::COMPRESSION_COLOR = {1.0f, 0.0f, 0.0f};
const glm::vec3 ForceRenderer::NEUTRAL_COLOR     = {0.7f, 0.7f, 0.7f};

ForceRenderer::ForceRenderer() : cylinderVAO(0), cylinderVBO(0), coneVAO(0), coneVBO(0),
                                  cylinderVCount(0), coneVCount(0) {}

ForceRenderer::~ForceRenderer() {
    if (cylinderVAO) { glDeleteVertexArrays(1, &cylinderVAO); glDeleteBuffers(1, &cylinderVBO); }
    if (coneVAO)     { glDeleteVertexArrays(1, &coneVAO);     glDeleteBuffers(1, &coneVBO); }
}

void ForceRenderer::initialize() {
    forceShader.loadFromSource(FORCE_VERT, FORCE_FRAG);
    generateCylinderMesh();
    generateConeMesh();
}

glm::vec3 ForceRenderer::getBeamColor(float force, float maxStress) {
    float n = std::clamp(force / maxStress, -1.0f, 1.0f);
    if (std::abs(n) < 0.05f)  return NEUTRAL_COLOR;
    if (n > 0)                 return NEUTRAL_COLOR + (TENSION_COLOR     - NEUTRAL_COLOR) *  n;
    return                            NEUTRAL_COLOR + (COMPRESSION_COLOR - NEUTRAL_COLOR) * -n;
}

void ForceRenderer::renderForceVectors(const std::vector<Node>& nodes,
                                       const glm::mat4& view,
                                       const glm::mat4& projection) {
    for (const auto& node : nodes) {
        glm::vec3 force = node.getAppliedForce();
        float mag = glm::length(force);
        if (mag > 0.001f) {
            glm::vec3 start = node.getPosition();
            glm::vec3 end   = start + glm::normalize(force) * mag * FORCE_SCALE;
            drawArrow(start, end, COMPRESSION_COLOR, view, projection);
        }
    }
}

void ForceRenderer::drawArrow(const glm::vec3& start, const glm::vec3& end,
                               const glm::vec3& color,
                               const glm::mat4& view,
                               const glm::mat4& projection) {
    glm::vec3 dir = end - start;
    float len = glm::length(dir);
    if (len < 0.0001f) return;

    dir = glm::normalize(dir);

    glm::vec3 up    = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 right = glm::cross(dir, up);
    if (glm::length(right) < 0.001f) {
        up    = glm::vec3(1.0f, 0.0f, 0.0f);
        right = glm::cross(dir, up);
    }
    right = glm::normalize(right);
    up    = glm::normalize(glm::cross(right, dir));

    glm::mat4 rot(1.0f);
    rot[0] = glm::vec4(right, 0.0f);
    rot[1] = glm::vec4(up,    0.0f);
    rot[2] = glm::vec4(dir,   0.0f);

    float shaftLen    = len * 0.8f;
    float shaftRadius = len * 0.02f;
    float coneLen     = len * 0.2f;
    float coneRadius  = shaftRadius * 2.5f;

    forceShader.use();
    forceShader.setMat4("view",       view);
    forceShader.setMat4("projection", projection);
    forceShader.setVec3("forceColor", color);

    // Shaft
    glm::mat4 shaftModel = glm::translate(glm::mat4(1.0f), start) * rot;
    shaftModel = glm::scale(shaftModel, glm::vec3(shaftRadius, shaftLen, shaftRadius));
    forceShader.setMat4("model", shaftModel);
    glBindVertexArray(cylinderVAO);
    glDrawArrays(GL_TRIANGLES, 0, cylinderVCount);

    // Cone (arrowhead)
    glm::mat4 coneModel = glm::translate(glm::mat4(1.0f), start + dir * shaftLen) * rot;
    coneModel = glm::scale(coneModel, glm::vec3(coneRadius, coneLen, coneRadius));
    forceShader.setMat4("model", coneModel);
    glBindVertexArray(coneVAO);
    glDrawArrays(GL_TRIANGLES, 0, coneVCount);

    glBindVertexArray(0);
}

void ForceRenderer::generateCylinderMesh() {
    std::vector<float> verts;
    const int segs = 12;

    for (int i = 0; i < segs; ++i) {
        float a1 = 2.0f * static_cast<float>(M_PI) * i / segs;
        float a2 = 2.0f * static_cast<float>(M_PI) * (i + 1) / segs;
        float x1 = std::cos(a1), z1 = std::sin(a1);
        float x2 = std::cos(a2), z2 = std::sin(a2);

        // Bottom cap
        verts.insert(verts.end(), {0,0,0, x1,0,z1, x2,0,z2});
        // Top cap
        verts.insert(verts.end(), {0,1,0, x2,1,z2, x1,1,z1});
        // Side (2 triangles)
        verts.insert(verts.end(), {x1,0,z1, x2,0,z2, x2,1,z2});
        verts.insert(verts.end(), {x1,0,z1, x2,1,z2, x1,1,z1});
    }

    cylinderVCount = static_cast<int>(verts.size()) / 3;  // 144

    glGenVertexArrays(1, &cylinderVAO);
    glGenBuffers(1, &cylinderVBO);
    glBindVertexArray(cylinderVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cylinderVBO);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
}

void ForceRenderer::generateConeMesh() {
    std::vector<float> verts;
    const int segs = 12;

    for (int i = 0; i < segs; ++i) {
        float a1 = 2.0f * static_cast<float>(M_PI) * i / segs;
        float a2 = 2.0f * static_cast<float>(M_PI) * (i + 1) / segs;
        float x1 = std::cos(a1), z1 = std::sin(a1);
        float x2 = std::cos(a2), z2 = std::sin(a2);

        // Base cap
        verts.insert(verts.end(), {0,0,0, x1,0,z1, x2,0,z2});
        // Side to tip
        verts.insert(verts.end(), {0,1,0, x2,0,z2, x1,0,z1});
    }

    coneVCount = static_cast<int>(verts.size()) / 3;  // 72

    glGenVertexArrays(1, &coneVAO);
    glGenBuffers(1, &coneVBO);
    glBindVertexArray(coneVAO);
    glBindBuffer(GL_ARRAY_BUFFER, coneVBO);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
}
