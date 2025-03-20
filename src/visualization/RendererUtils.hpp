#pragma once
#include <glm/glm.hpp>
#include <GL/gl.h> // Include OpenGL header for drawing functions

void drawArrow(glm::vec3 start, glm::vec3 end, glm::vec3 color);
void drawCylinder(const glm::vec3& start, const glm::vec3& end, float radius, glm::vec3 color);
void drawCone(const glm::vec3& base, const glm::vec3& tip, float radius, glm::vec3 color);
