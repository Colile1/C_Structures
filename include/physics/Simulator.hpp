// Copyright (c) 2026 Colile Sibanda. All rights reserved.
// Proprietary — see LICENSE for terms. Unauthorised use prohibited.
#pragma once
#include <Eigen/Sparse>
#include <Eigen/SparseLU>
#include <cstddef>
#include <glm/glm.hpp>
#include <vector>
#include "../model/Node.hpp"
#include "../model/Beam.hpp"
#include "SolveResult.hpp"

// physics/Simulator.hpp : static force solver using Eigen sparse matrices.
class Simulator {
public:
    Simulator(std::vector<Node>& nodes, std::vector<Beam>& beams);

    SolveResult solveStaticForces();

    // Returns per-node displacement as 3D vectors (x,y,z) in metres.
    std::vector<glm::vec3> getNodeDisplacements() const;

    // Returns signed axial force in the beam (positive = tension, negative = compression).
    float getBeamForce(const Beam& beam) const;

    // Returns the support reaction at a node (non-zero only on constrained DOFs).
    // Computed from the residual r = K*u - F after the last solve.
    glm::vec3 getNodeReaction(int nodeIdx) const;

    // Global equilibrium self-check: Σ(applied loads) + Σ(reactions) should be ≈ 0.
    // Writes the net residual force to netResidual; returns true if within tol (N).
    bool checkEquilibrium(glm::vec3& netResidual, float tol = 1e-2f) const;

private:
    void assembleGlobalStiffnessMatrix();
    void applySupportConstraints();
    void populateForceVector();

    // Hash of everything that determines the stiffness matrix (node positions,
    // joint types/BCs, beam connectivity and AE). When this is unchanged between
    // solves only the load vector differs, so the cached factorisation is reused.
    std::size_t stiffnessSignature() const;

    std::vector<Node>* m_nodes;
    std::vector<Beam>* m_beams;
    Eigen::SparseMatrix<double> m_globalK;
    Eigen::VectorXd m_forces;
    Eigen::VectorXd m_displacements;
    Eigen::VectorXd m_reactions; // r = K*u - F, cached after solve

    // ── Cached factorisation (reused when the stiffness matrix is unchanged) ──
    // Editing a load re-solves with a fresh RHS but no re-factorisation, keeping
    // large models interactive. Invalidated whenever the signature changes.
    Eigen::SparseLU<Eigen::SparseMatrix<double>> m_solver;
    bool             m_factorized   = false;
    std::size_t      m_stiffnessSig = 0;
    std::vector<int> m_freeList;      // free-DOF local index -> global DOF
    std::vector<int> m_globalToFree;  // global DOF -> local free index, or -1
};
