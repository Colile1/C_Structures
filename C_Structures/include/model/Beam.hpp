#pragma once
#include "Node.hpp"

class Beam {
public:
    Beam(Node* start, Node* end, double youngsModulus, double area)
        : startNode(start), endNode(end), stiffness(youngsModulus * area / length()) {}

    Node& getStart() const { return *startNode; }
    Node& getEnd() const { return *endNode; }
    double getStiffness() const { return stiffness; }

private:
    Node* startNode;
    Node* endNode;
    double stiffness;

    double length() const {
        // Calculate length between startNode and endNode
        return 0.0; // Placeholder
    }
};
