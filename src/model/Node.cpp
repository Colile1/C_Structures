#include "include/model/Node.hpp"

Node::Node(float x, float y, float z) 
    : position(x, y, z) {}

glm::vec3 Node::getPosition() const { 
    return position; 
}

bool Node::isFixed() const { 
    return fixed; 
}

void Node::setFixed(bool fixed) { 
    this->fixed = fixed; 
}

void Node::applyForce(const glm::vec3& force) {
    appliedForce += force;
}
