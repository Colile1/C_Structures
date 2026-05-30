// Copyright (c) 2026 Colile Sibanda. All rights reserved.
// Proprietary — see LICENSE for terms. Unauthorised use prohibited.
#pragma once
#include <Eigen/Sparse>
#include <glm/glm.hpp>
#include <array>
#include <vector>
#include "../model/Node.hpp"
#include "../model/Beam.hpp"

// physics/FrameSimulator.hpp : 3D frame solver with 6 DOF per node
// (3 translations + 3 rotations). Builds 12x12 frame elements, assembles the
// 6n global system, applies boundary conditions, and solves by static
// condensation + SparseLU — the "rigid-jointed" counterpart to Simulator's
// pin-jointed truss. Poisson ratio and torsion constant use documented
// defaults ( nu = 0.3; J = Iy + Iz) until per-section data is added.
class FrameSimulator {
public:
    FrameSimulator(std::vector<Node>& nodes, std::vector<Beam>& beams);

    void solve();

    // Per-node translation (m) and rotation (rad) after the last solve.
    std::vector<glm::vec3> getNodeTranslations() const;
    std::vector<glm::vec3> getNodeRotations() const;

    // Support reactions: force (N) and moment (N·m), non-zero on constrained DOFs.
    glm::vec3 getNodeReactionForce(int nodeIdx) const;
    glm::vec3 getNodeReactionMoment(int nodeIdx) const;

    // Global equilibrium self-check on forces (Σloads + Σreaction forces ≈ 0).
    bool checkForceEquilibrium(glm::vec3& netResidual, float tol = 1e-2f) const;

    // Member-end forces in local coordinates after the last solve:
    // [N1,Vy1,Vz1,T1,My1,Mz1, N2,Vy2,Vz2,T2,My2,Mz2]. Basis for force diagrams.
    std::array<float, 12> getMemberEndForces(const Beam& beam) const;

private:
    static constexpr int DPN = 6; // DOF per node

    void assemble();
    void populateForces();
    bool isDofConstrained(const Node& nd, int dof) const; // dof 0..5

    std::vector<Node>* m_nodes;
    std::vector<Beam>* m_beams;
    Eigen::SparseMatrix<double> m_K;
    Eigen::VectorXd m_F;
    Eigen::VectorXd m_u;
    Eigen::VectorXd m_reactions; // r = K*u - F
};
