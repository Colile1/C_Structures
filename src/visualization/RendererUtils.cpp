#include "RendererUtils.hpp"
#include <GL/gl.h> // Include OpenGL header for drawing functions

void drawArrow(glm::vec3 start, glm::vec3 end, glm::vec3 color) {
    // Shaft
    drawCylinder(start, end, 0.02f, color);
    
    // Arrowhead
    glm::vec3 direction = glm::normalize(end - start);
    glm::vec3 tip = end + direction * 0.1f; // Adjust the length of the arrowhead
    drawCone(end, tip, 0.05f, color); // Assuming drawCone is defined elsewhere
}

void drawCylinder(const glm::vec3& start, const glm::vec3& end, float radius, glm::vec3 color) {
    // Implement cylinder drawing logic here
    glPushMatrix();
    // Calculate the cylinder's position and orientation
    // Draw cylinder using OpenGL functions
    glPopMatrix();
}

void drawCone(const glm::vec3& base, const glm::vec3& tip, float radius, glm::vec3 color) {
    // Implement cone drawing logic here
    glPushMatrix();
    // Calculate the cone's position and orientation
    // Draw cone using OpenGL functions
    glPopMatrix();
}
