// Copyright (c) 2026 Colile Sibanda. All rights reserved.
// Proprietary — see LICENSE for terms. Unauthorised use prohibited.
#include "graphics/Camera.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>

// graphics/Camera.cpp : orbit camera centred on a target point.

Camera::Camera()
    : target(0.0f, 0.0f, 0.0f), radius(8.0f),
      yaw(45.0f), pitch(30.0f) {}

// Pure: computes spherical camera position from orbit angles.
glm::vec3 Camera::getPosition() const {
    float yawRad   = glm::radians(yaw);
    float pitchRad = glm::radians(pitch);
    return target + glm::vec3(
        radius * std::cos(pitchRad) * std::sin(yawRad),
        radius * std::sin(pitchRad),
        radius * std::cos(pitchRad) * std::cos(yawRad)
    );
}

glm::mat4 Camera::getViewMatrix() const {
    return glm::lookAt(getPosition(), target, glm::vec3(0.0f, 1.0f, 0.0f));
}

glm::mat4 Camera::getProjectionMatrix(float aspect) const {
    return glm::perspective(glm::radians(45.0f), aspect, 0.1f, 1000.0f);
}

// Input wrapper: adjusts yaw and pitch from mouse drag deltas.
void Camera::handleMouseDrag(int dx, int dy) {
    yaw   += static_cast<float>(dx) * 0.4f;
    pitch -= static_cast<float>(dy) * 0.4f;
    pitch  = std::clamp(pitch, -89.0f, 89.0f);
}

// Input wrapper: adjusts zoom radius from scroll wheel delta.
void Camera::handleScroll(int delta) {
    radius -= static_cast<float>(delta) * 0.5f;
    radius  = std::clamp(radius, 1.0f, 200.0f);
}
