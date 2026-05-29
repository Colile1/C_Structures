#include "physics/Simulator.hpp"
#include <iostream>

// physics/Simulator.cpp : assembles global stiffness and solves K*u = F.

Simulator::Simulator(std::vector<Node>& nodes, std::vector<Beam>& beams)
    : m_nodes(&nodes), m_beams(&beams) {
    const int numDofs = 3 * static_cast<int>(nodes.size());
    m_globalK.resize(numDofs, numDofs);
    m_forces.resize(numDofs);
    m_displacements.resize(numDofs);
    m_forces.setZero();
    m_displacements.setZero();
}

// Pure: writes applied node forces into the global force vector.
void Simulator::populateForceVector() {
    m_forces.setZero();
    for (int i = 0; i < static_cast<int>(m_nodes->size()); ++i) {
        glm::vec3 f = (*m_nodes)[i].getAppliedForce();
        m_forces[3 * i + 0] = static_cast<double>(f.x);
        m_forces[3 * i + 1] = static_cast<double>(f.y);
        m_forces[3 * i + 2] = static_cast<double>(f.z);
    }
}

// Pure: adds each beam's 6×6 stiffness contribution to the global sparse matrix.
void Simulator::assembleGlobalStiffnessMatrix() {
    m_globalK.setZero();
    std::vector<Eigen::Triplet<double>> triplets;

    for (const Beam& beam : *m_beams) {
        // Derive node indices via pointer offset from the vector base.
        const int i = static_cast<int>(beam.getStart() - &(*m_nodes)[0]);
        const int j = static_cast<int>(beam.getEnd()   - &(*m_nodes)[0]);

        const double AE_L = static_cast<double>(beam.getStiffness());
        Eigen::Matrix3d kBlock = AE_L * Eigen::Matrix3d::Identity();

        for (int r = 0; r < 3; ++r) {
            for (int c = 0; c < 3; ++c) {
                triplets.emplace_back(3*i+r, 3*i+c,  kBlock(r, c));
                triplets.emplace_back(3*j+r, 3*j+c,  kBlock(r, c));
                triplets.emplace_back(3*i+r, 3*j+c, -kBlock(r, c));
                triplets.emplace_back(3*j+r, 3*i+c, -kBlock(r, c));
            }
        }
    }
    m_globalK.setFromTriplets(triplets.begin(), triplets.end());
}

// Pure: zeros out rows/columns for fixed-support DOFs to enforce boundary conditions.
void Simulator::applySupportConstraints() {
    for (int i = 0; i < static_cast<int>(m_nodes->size()); ++i) {
        if ((*m_nodes)[i].isFixed()) {
            for (int dof = 0; dof < 3; ++dof) {
                const int idx = 3 * i + dof;
                // Zero the row and column, set diagonal to 1.
                for (Eigen::SparseMatrix<double>::InnerIterator it(m_globalK, idx); it; ++it)
                    it.valueRef() = (it.row() == it.col()) ? 1.0 : 0.0;
                m_forces[idx] = 0.0;
            }
        }
    }
}

void Simulator::solveStaticForces() {
    populateForceVector();
    assembleGlobalStiffnessMatrix();
    applySupportConstraints();

    Eigen::ConjugateGradient<Eigen::SparseMatrix<double>> solver;
    solver.compute(m_globalK);
    if (solver.info() != Eigen::Success) {
        std::cerr << "Simulator: matrix decomposition failed" << std::endl;
        return;
    }
    m_displacements = solver.solve(m_forces);
    if (solver.info() != Eigen::Success)
        std::cerr << "Simulator: solve failed" << std::endl;
}

// Output: converts flat Eigen vector to per-node glm::vec3 displacements.
std::vector<glm::vec3> Simulator::getNodeDisplacements() const {
    std::vector<glm::vec3> result;
    result.reserve(m_nodes->size());
    for (int i = 0; i < static_cast<int>(m_nodes->size()); ++i) {
        result.emplace_back(
            static_cast<float>(m_displacements[3*i + 0]),
            static_cast<float>(m_displacements[3*i + 1]),
            static_cast<float>(m_displacements[3*i + 2])
        );
    }
    return result;
}

// Pure: computes signed axial force for a beam from the solved displacements.
float Simulator::getBeamForce(const Beam& beam) const {
    const int i = static_cast<int>(beam.getStart() - &(*m_nodes)[0]);
    const int j = static_cast<int>(beam.getEnd()   - &(*m_nodes)[0]);

    glm::vec3 dispStart(
        static_cast<float>(m_displacements[3*i]),
        static_cast<float>(m_displacements[3*i+1]),
        static_cast<float>(m_displacements[3*i+2])
    );
    glm::vec3 dispEnd(
        static_cast<float>(m_displacements[3*j]),
        static_cast<float>(m_displacements[3*j+1]),
        static_cast<float>(m_displacements[3*j+2])
    );

    glm::vec3 axis = beam.getEnd()->getPosition() - beam.getStart()->getPosition();
    float length = glm::length(axis);
    if (length < 1e-6f) return 0.0f;
    glm::vec3 unitAxis = axis / length;

    float extension = glm::dot(dispEnd - dispStart, unitAxis);
    return beam.getStiffness() * extension;
}
