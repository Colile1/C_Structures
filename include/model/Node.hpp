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
        : position(x, y, z), appliedForce(0.0f, 0.0f, 0.0f),
          appliedMoment(0.0f, 0.0f, 0.0f), jointType(JointType::FREE) {}

    void applyForce(const glm::vec3& force) { appliedForce += force; }
    glm::vec3 getAppliedForce() const { return appliedForce; }
    glm::vec3 getPosition()     const { return position; }

    // Concentrated nodal moment (Mx, My, Mz) about the global axes. Only the
    // frame solver (6 DOF/node) acts on it; the truss solver ignores rotations.
    void applyMoment(const glm::vec3& moment) { appliedMoment += moment; }
    glm::vec3 getAppliedMoment() const { return appliedMoment; }
    void clearMoment() { appliedMoment = {0.0f, 0.0f, 0.0f}; }

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
    glm::vec3 appliedMoment;
    JointType jointType;
};
