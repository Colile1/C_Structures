#pragma once
#include <glm/vec3.hpp>

class Node {
public:
    Node(float x, float y, float z) : position(x, y, z), fixed(false) {}

    void applyForce(const glm::vec3& force) {
        // Apply force to the node
    }

    bool isFixed() const { return fixed; }
    void setFixed(bool value) { fixed = value; }
    glm::vec3 getPosition() const { return position; }

private:
    glm::vec3 position;
    bool fixed;
};
