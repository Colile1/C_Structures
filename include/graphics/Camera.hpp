#pragma once
#include <glm/glm.hpp>

// graphics/Camera.hpp : orbit camera controlled by mouse drag and scroll.
class Camera {
public:
    Camera();

    // Returns the view matrix for the current orbit position.
    glm::mat4 getViewMatrix() const;

    // Returns a perspective projection matrix.
    glm::mat4 getProjectionMatrix(float aspectRatio) const;

    // Called with relative mouse delta when left-button is held.
    void handleMouseDrag(int dx, int dy);

    // Called with scroll wheel delta (positive = zoom in).
    void handleScroll(int delta);

    // Exposes camera position for ray unprojection.
    glm::vec3 getPosition() const;

private:
    glm::vec3 target;
    float radius;
    float yaw;
    float pitch;
};
