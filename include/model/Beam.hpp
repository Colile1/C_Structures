#pragma once
#include "Node.hpp"

/**
 * @class Beam
 * @brief Represents a beam defined by two nodes.
 */
class Beam {
public:
    /**
     * @brief Construct a new Beam object.
     * 
     * @param start Pointer to the starting node.
     * @param end Pointer to the ending node.
     * @param youngsModulus Young's modulus of the beam material.
     * @param crossSection Cross-sectional area of the beam.
     */
    Beam(Node* start, Node* end, float youngsModulus, float crossSection);
    
    /**
     * @brief Get the starting node of the beam.
     * 
     * @return Node* Pointer to the starting node.
     */
    Node* getStart() const;

    /**
     * @brief Get the ending node of the beam.
     * 
     * @return Node* Pointer to the ending node.
     */
    Node* getEnd() const;

    /**
     * @brief Calculate the length of the beam.
     * 
     * @return float The length of the beam.
     */
    float getLength() const;

    /**
     * @brief Calculate the stiffness of the beam.
     * 
     * @return float The stiffness of the beam (AE/L).
     */
    float getStiffness() const; // AE/L
    
private:
    Node* startNode; ///< Pointer to the starting node
    Node* endNode; ///< Pointer to the ending node
    float youngsModulus; ///< Material property (Pa)
    float crossSection; ///< Area (m²)
};
