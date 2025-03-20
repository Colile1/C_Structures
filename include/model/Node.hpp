#pragma once
#include <glm/glm.hpp> // For vec3

/**
 * @brief Represents a node in 3D space.
 */
class Node {
public:
    /**
     * @brief Construct a new Node object.
     * 
     * @param x X coordinate.
     * @param y Y coordinate.
     * @param z Z coordinate.
     */
    Node(float x, float y, float z);
    
    /**
     * @brief Get the position of the node.
     * 
     * @return glm::vec3 The position of the node.
     */
    glm::vec3 getPosition() const;

    /**
     * @brief Check if the node is fixed.
     * 
     * @return true if the node is fixed, false otherwise.
     */
    bool isFixed() const;
    
    /**
     * @brief Set the fixed state of the node.
     * 
     * @param fixed The fixed state to set.
     */
    void setFixed(bool fixed);

    /**
     * @brief Apply a force to the node.
     * 
     * @param force The force vector to apply.
     */
    void applyForce(const glm::vec3& force);
    
private:
    glm::vec3 position; // 3D coordinates (meters)
    glm::vec3 appliedForce; // Force vector (Newtons)
    bool fixed = false; // Fixed support constraint
};
