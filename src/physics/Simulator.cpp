// Copyright (c) 2026 Colile Sibanda. All rights reserved.
// Proprietary — see LICENSE for terms. Unauthorised use prohibited.
#include "physics/Simulator.hpp"
#include <Eigen/SparseLU>
#include <iostream>
#include <cmath>
#include <algorithm>

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

        double lx = axis.x / L, ly = axis.y / L, lz = axis.z / L;
        double AE_L = static_cast<double>(beam.getStiffness());

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

// Proper static condensation: extract the free-DOF sub-system and solve it
// directly.  This avoids the ill-conditioning of the penalty-BC approach and
// works regardless of whether SimplicialLDLT recognises the matrix as SPD.
void Simulator::solveStaticForces() {
    if (m_nodes->empty()) return;
    if (m_beams->empty()) { m_displacements.setZero(); return; }

    populateForceVector();
    assembleGlobalStiffnessMatrix();

    const int n = static_cast<int>(m_globalK.rows());

    // --- Step 1: determine fixed DOFs ----------------------------------------
    std::vector<bool> isFixed(n, false);
    for (int i = 0; i < static_cast<int>(m_nodes->size()); ++i) {
        const Node& nd = (*m_nodes)[i];
        for (int d = 0; d < 3; ++d)
            if (nd.isDOFConstrained(d))
                isFixed[3*i + d] = true;
    }
    // Pin any DOF whose diagonal is zero (no stiffness — unstable direction).
    for (int i = 0; i < n; ++i)
        if (!isFixed[i] && std::abs(m_globalK.coeff(i, i)) < 1e-14)
            isFixed[i] = true;

    // --- Step 2: build free-DOF index map ------------------------------------
    std::vector<int> freeList;
    freeList.reserve(n);
    std::vector<int> globalToFree(n, -1);
    for (int i = 0; i < n; ++i) {
        if (!isFixed[i]) {
            globalToFree[i] = static_cast<int>(freeList.size());
            freeList.push_back(i);
        }
    }
    const int nf = static_cast<int>(freeList.size());
    m_displacements.setZero();
    if (nf == 0) return; // fully constrained

    // --- Step 3: extract KFF and fF ------------------------------------------
    std::vector<Eigen::Triplet<double>> sub;
    sub.reserve(nf * 6);
    for (int gCol : freeList) {
        int lCol = globalToFree[gCol];
        for (Eigen::SparseMatrix<double>::InnerIterator it(m_globalK, gCol); it; ++it) {
            int gRow = static_cast<int>(it.row());
            if (isFixed[gRow]) continue;
            sub.emplace_back(globalToFree[gRow], lCol, it.value());
        }
    }
    Eigen::SparseMatrix<double> Kff(nf, nf);
    Kff.setFromTriplets(sub.begin(), sub.end());

    Eigen::VectorXd ff(nf);
    for (int li = 0; li < nf; ++li)
        ff[li] = m_forces[freeList[li]];

    // --- Step 4: solve with SparseLU -----------------------------------------
    Kff.makeCompressed();
    Eigen::SparseLU<Eigen::SparseMatrix<double>> solver;
    solver.compute(Kff);
    if (solver.info() != Eigen::Success) {
        std::cerr << "Simulator: factorization failed "
                     "(structure may be a mechanism or under-constrained)\n";
        return;
    }
    Eigen::VectorXd uf = solver.solve(ff);
    if (solver.info() != Eigen::Success) {
        std::cerr << "Simulator: solve failed\n";
        return;
    }

    // --- Step 5: scatter back ------------------------------------------------
    for (int li = 0; li < nf; ++li)
        m_displacements[freeList[li]] = uf[li];
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
