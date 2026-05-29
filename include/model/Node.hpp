#pragma once
#include <glm/glm.hpp> // For vec3

class Node {
public:
    Node(float x, float y, float z) : position(x, y, z), fixed(false), appliedForce(0.0f, 0.0f, 0.0f) {}
    
    void applyForce(const glm::vec3& force) {
        appliedForce += force;
    }

    glm::vec3 getAppliedForce() const {
        return appliedForce;
    }

    glm::vec3 getPosition() const {
        return position; // Return the position
    }

    bool isFixed() const {
        return fixed; // Return if the node is fixed
    }

    void setFixed(bool fixed) {
        this->fixed = fixed;
    }

    void setPosition(const glm::vec3& pos) { position = pos; }
    void clearForce() { appliedForce = {0.0f, 0.0f, 0.0f}; }

private:
    glm::vec3 position; // 3D coordinates (meters)
    glm::vec3 appliedForce; // Force vector (Newtons)
    bool fixed = false; // Fixed support constraint
};
