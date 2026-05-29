#include "RendererUtils.hpp"

// RendererUtils.cpp : legacy fixed-function stubs, superseded by ForceRenderer's VAO approach.
// These are kept as no-ops so existing call sites compile; do not use for new code.

void drawArrow(glm::vec3 start, glm::vec3 end, glm::vec3 color) {
    (void)start; (void)end; (void)color;
}

void drawCylinder(const glm::vec3& start, const glm::vec3& end, float radius, glm::vec3 color) {
    (void)start; (void)end; (void)radius; (void)color;
}

void drawCone(const glm::vec3& base, const glm::vec3& tip, float radius, glm::vec3 color) {
    (void)base; (void)tip; (void)radius; (void)color;
}
