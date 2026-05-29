#include "physics/Simulator.hpp"
#include <iostream>
#include <cmath>

Simulator::Simulator(std::vector<Node>& nodes, std::vector<Beam>& beams)
    : m_nodes(&nodes), m_beams(&beams) {
    const int n = 3 * static_cast<int>(nodes.size());
    m_globalK.resize(n, n);
    m_forces.resize(n);
    m_displacements.resize(n);
    m_forces.setZero();
    m_displacements.setZero();
}

void Simulator::populateForceVector() {
    m_forces.setZero();
    for (int i = 0; i < static_cast<int>(m_nodes->size()); ++i) {
        glm::vec3 f = (*m_nodes)[i].getAppliedForce();
        m_forces[3*i+0] = static_cast<double>(f.x);
        m_forces[3*i+1] = static_cast<double>(f.y);
        m_forces[3*i+2] = static_cast<double>(f.z);
    }
}

void Simulator::assembleGlobalStiffnessMatrix() {
    std::vector<Eigen::Triplet<double>> triplets;
    triplets.reserve(m_beams->size() * 36);

    for (const Beam& beam : *m_beams) {
        const int i = static_cast<int>(beam.getStart() - &(*m_nodes)[0]);
        const int j = static_cast<int>(beam.getEnd()   - &(*m_nodes)[0]);

        glm::vec3 axis = beam.getEnd()->getPosition() - beam.getStart()->getPosition();
        float L = glm::length(axis);
        if (L < 1e-8f) continue;

        // Direction cosines
        double lx = axis.x / L, ly = axis.y / L, lz = axis.z / L;
        double AE_L = static_cast<double>(beam.getStiffness());

        // 3×3 local stiffness (axial only, oriented along beam axis)
        double k[3][3] = {
            {lx*lx, lx*ly, lx*lz},
            {ly*lx, ly*ly, ly*lz},
            {lz*lx, lz*ly, lz*lz}
        };

        for (int r = 0; r < 3; ++r) {
            for (int c = 0; c < 3; ++c) {
                double v = AE_L * k[r][c];
                triplets.emplace_back(3*i+r, 3*i+c,  v);
                triplets.emplace_back(3*j+r, 3*j+c,  v);
                triplets.emplace_back(3*i+r, 3*j+c, -v);
                triplets.emplace_back(3*j+r, 3*i+c, -v);
            }
        }
    }
    m_globalK.setFromTriplets(triplets.begin(), triplets.end());
}

// Zeros BOTH the row AND column for each fixed-support DOF so the system
// stays symmetric (required for ConjugateGradient). Also fixes any DOF whose
// diagonal is zero (no stiffness in that direction) to prevent singularity.
void Simulator::applySupportConstraints() {
    const int n = static_cast<int>(m_globalK.rows());
    std::vector<bool> fixed(n, false);

    for (int i = 0; i < static_cast<int>(m_nodes->size()); ++i) {
        if ((*m_nodes)[i].isFixed()) {
            fixed[3*i]   = true;
            fixed[3*i+1] = true;
            fixed[3*i+2] = true;
        }
    }

    // Also mark DOFs with zero diagonal (no stiffness) as fixed to zero
    // so the system is non-singular.
    for (int i = 0; i < n; ++i) {
        if (!fixed[i] && std::abs(m_globalK.coeff(i, i)) < 1e-12) {
            fixed[i] = true;
        }
    }

    // Walk every stored non-zero: if either row OR col is fixed, apply BC.
    // Column-major outer loop hits every non-zero exactly once.
    for (int col = 0; col < m_globalK.outerSize(); ++col) {
        for (Eigen::SparseMatrix<double>::InnerIterator it(m_globalK, col); it; ++it) {
            if (fixed[it.row()] || fixed[it.col()])
                it.valueRef() = (it.row() == it.col()) ? 1.0 : 0.0;
        }
    }

    for (int i = 0; i < n; ++i) {
        if (fixed[i]) m_forces[i] = 0.0;
    }
}

void Simulator::solveStaticForces() {
    if (m_nodes->empty()) return;

    populateForceVector();
    assembleGlobalStiffnessMatrix();
    applySupportConstraints();

    Eigen::ConjugateGradient<Eigen::SparseMatrix<double>,
                             Eigen::Lower | Eigen::Upper> solver;
    solver.setMaxIterations(1000);
    solver.setTolerance(1e-8);
    solver.compute(m_globalK);
    if (solver.info() != Eigen::Success) {
        std::cerr << "Simulator: decomposition failed\n"; return;
    }
    m_displacements = solver.solve(m_forces);
    if (solver.info() != Eigen::Success)
        std::cerr << "Simulator: solve did not converge (iter="
                  << solver.iterations() << ", err=" << solver.error() << ")\n";
}

std::vector<glm::vec3> Simulator::getNodeDisplacements() const {
    std::vector<glm::vec3> result;
    result.reserve(m_nodes->size());
    for (int i = 0; i < static_cast<int>(m_nodes->size()); ++i) {
        result.emplace_back(
            static_cast<float>(m_displacements[3*i+0]),
            static_cast<float>(m_displacements[3*i+1]),
            static_cast<float>(m_displacements[3*i+2]));
    }
    return result;
}

float Simulator::getBeamForce(const Beam& beam) const {
    const int i = static_cast<int>(beam.getStart() - &(*m_nodes)[0]);
    const int j = static_cast<int>(beam.getEnd()   - &(*m_nodes)[0]);

    glm::vec3 dI(m_displacements[3*i], m_displacements[3*i+1], m_displacements[3*i+2]);
    glm::vec3 dJ(m_displacements[3*j], m_displacements[3*j+1], m_displacements[3*j+2]);

    glm::vec3 axis   = beam.getEnd()->getPosition() - beam.getStart()->getPosition();
    float     length = glm::length(axis);
    if (length < 1e-6f) return 0.0f;

    return beam.getStiffness() * glm::dot(dJ - dI, axis / length);
}
