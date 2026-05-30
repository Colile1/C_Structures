// Copyright (c) 2026 Colile Sibanda. All rights reserved.
// Proprietary — see LICENSE for terms. Unauthorised use prohibited.
#pragma once
#include <glm/glm.hpp>

enum class JointType {
    FREE,      // no constraints (internal node)
    FIXED,     // all 3 translational DOFs = 0
    PIN_XY,    // Ux=Uy=0, Uz free  (2D pin in XY plane)
    ROLLER_X,  // Ux=0 only
    ROLLER_Y,  // Uy=0 only
    ROLLER_Z,  // Uz=0 only
};

class Node {
public:
    Node(float x, float y, float z)
        : position(x, y, z), jointType(JointType::FREE), appliedForce(0.0f, 0.0f, 0.0f) {}

    void applyForce(const glm::vec3& force) { appliedForce += force; }
    glm::vec3 getAppliedForce() const { return appliedForce; }
    glm::vec3 getPosition()     const { return position; }

    JointType getJointType()          const { return jointType; }
    void      setJointType(JointType t)     { jointType = t; }

    // Convenience: true if this DOF index (0=x,1=y,2=z) is constrained.
    bool isDOFConstrained(int dof) const {
        switch (jointType) {
            case JointType::FREE:     return false;
            case JointType::FIXED:    return true;
            case JointType::PIN_XY:   return (dof == 0 || dof == 1);
            case JointType::ROLLER_X: return (dof == 0);
            case JointType::ROLLER_Y: return (dof == 1);
            case JointType::ROLLER_Z: return (dof == 2);
        }
        return false;
    }

    // Legacy convenience used by rendering.
    bool isFixed() const { return jointType == JointType::FIXED; }
    void setFixed(bool f) { jointType = f ? JointType::FIXED : JointType::FREE; }

    void setPosition(const glm::vec3& pos) { position = pos; }
    void clearForce() { appliedForce = {0.0f, 0.0f, 0.0f}; }

private:
    glm::vec3 position;
    glm::vec3 appliedForce;
    JointType jointType;
};
