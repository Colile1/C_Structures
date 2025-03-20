#include "include/model/Beam.hpp"
#include <glm/glm.hpp>

Beam::Beam(Node* start, Node* end, float E, float A)
    : startNode(start), endNode(end), 
      youngsModulus(E), crossSection(A) {}

Node* Beam::getStart() const { 
    return startNode; 
}

Node* Beam::getEnd() const { 
    return endNode; 
}

float Beam::getLength() const {
    glm::vec3 delta = endNode->getPosition() - startNode->getPosition();
    return glm::length(delta);
}

float Beam::getStiffness() const {
    return (youngsModulus * crossSection) / getLength();
}
