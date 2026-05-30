// Copyright (c) 2026 Colile Sibanda. All rights reserved.
// Proprietary — see LICENSE for terms. Unauthorised use prohibited.
// physics/FrameSimulator.cpp : assembly and solve for the 6-DOF frame model.
#include "physics/FrameSimulator.hpp"
#include "physics/FrameElement.hpp"
#include <Eigen/SparseLU>
#include <iostream>
#include <cmath>

FrameSimulator::FrameSimulator(std::vector<Node>& nodes, std::vector<Beam>& beams)
    : m_nodes(&nodes), m_beams(&beams) {
    const int n = DPN * static_cast<int>(nodes.size());
    m_K.resize(n, n);
    m_F.resize(n);     m_F.setZero();
    m_u.resize(n);     m_u.setZero();
    m_reactions.resize(n); m_reactions.setZero();
}

bool FrameSimulator::isDofConstrained(const Node& nd, int dof) const {
    if (dof < 3) return nd.isDOFConstrained(dof);      // translations
    return nd.getJointType() == JointType::FIXED;       // rotations: only FIXED
}

void FrameSimulator::populateForces() {
    m_F.setZero();
    for (int i = 0; i < static_cast<int>(m_nodes->size()); ++i) {
        glm::vec3 f = (*m_nodes)[i].getAppliedForce();
        m_F[DPN*i + 0] = static_cast<double>(f.x);
        m_F[DPN*i + 1] = static_cast<double>(f.y);
        m_F[DPN*i + 2] = static_cast<double>(f.z);
        // Rotational DOFs (3,4,5): applied moments are added in Phase 2.3.
    }
}

void FrameSimulator::assemble() {
    std::vector<Eigen::Triplet<double>> triplets;
    triplets.reserve(m_beams->size() * 144);
    const int nNodes = static_cast<int>(m_nodes->size());
    const double nu = 0.3; // Poisson ratio (default until per-material data)

    for (const Beam& beam : *m_beams) {
        const int i = beam.getStartIdx();
        const int j = beam.getEndIdx();
        if (i < 0 || j < 0 || i >= nNodes || j >= nNodes || i == j) continue;

        const double E  = beam.getYoungsModulus();
        const double A  = beam.getCrossSection();
        const double I  = beam.getMomentOfInertia();
        const double G  = E / (2.0 * (1.0 + nu));
        const double Iy = I, Iz = I;     // symmetric section default
        const double J  = Iy + Iz;       // polar second moment approximation

        FrameElement::Mat12 ke = FrameElement::globalStiffness(
            (*m_nodes)[i].getPosition(), (*m_nodes)[j].getPosition(),
            E, A, G, J, Iy, Iz);

        const int map[12] = {
            DPN*i+0, DPN*i+1, DPN*i+2, DPN*i+3, DPN*i+4, DPN*i+5,
            DPN*j+0, DPN*j+1, DPN*j+2, DPN*j+3, DPN*j+4, DPN*j+5
        };
        for (int r = 0; r < 12; ++r)
            for (int c = 0; c < 12; ++c)
                if (ke(r,c) != 0.0)
                    triplets.emplace_back(map[r], map[c], ke(r,c));
    }
    m_K.setFromTriplets(triplets.begin(), triplets.end());
}

void FrameSimulator::solve() {
    m_reactions.setZero();
    if (m_nodes->empty()) return;
    if (m_beams->empty()) { m_u.setZero(); return; }

    populateForces();
    assemble();

    const int n = static_cast<int>(m_K.rows());

    // Fixed DOFs from joint types, plus any DOF with no stiffness (auto-pin).
    std::vector<bool> fixed(n, false);
    for (int i = 0; i < static_cast<int>(m_nodes->size()); ++i)
        for (int d = 0; d < DPN; ++d)
            if (isDofConstrained((*m_nodes)[i], d))
                fixed[DPN*i + d] = true;
    for (int i = 0; i < n; ++i)
        if (!fixed[i] && std::abs(m_K.coeff(i,i)) < 1e-14)
            fixed[i] = true;

    // Free-DOF index map.
    std::vector<int> freeList;
    std::vector<int> toFree(n, -1);
    for (int i = 0; i < n; ++i)
        if (!fixed[i]) { toFree[i] = static_cast<int>(freeList.size()); freeList.push_back(i); }
    const int nf = static_cast<int>(freeList.size());
    m_u.setZero();
    if (nf == 0) { m_reactions = m_K * m_u - m_F; return; }

    // Extract KFF and fF.
    std::vector<Eigen::Triplet<double>> sub;
    sub.reserve(nf * 12);
    for (int gCol : freeList) {
        int lCol = toFree[gCol];
        for (Eigen::SparseMatrix<double>::InnerIterator it(m_K, gCol); it; ++it) {
            int gRow = static_cast<int>(it.row());
            if (fixed[gRow]) continue;
            sub.emplace_back(toFree[gRow], lCol, it.value());
        }
    }
    Eigen::SparseMatrix<double> Kff(nf, nf);
    Kff.setFromTriplets(sub.begin(), sub.end());
    Kff.makeCompressed();

    Eigen::VectorXd fF(nf);
    for (int li = 0; li < nf; ++li) fF[li] = m_F[freeList[li]];

    Eigen::SparseLU<Eigen::SparseMatrix<double>> solver;
    solver.compute(Kff);
    if (solver.info() != Eigen::Success) {
        std::cerr << "FrameSimulator: factorization failed "
                     "(mechanism or under-constrained)\n";
        return;
    }
    Eigen::VectorXd uF = solver.solve(fF);
    if (solver.info() != Eigen::Success) {
        std::cerr << "FrameSimulator: solve failed\n";
        return;
    }
    for (int li = 0; li < nf; ++li) m_u[freeList[li]] = uF[li];

    m_reactions = m_K * m_u - m_F; // residual: reactions at constrained DOFs
}

std::vector<glm::vec3> FrameSimulator::getNodeTranslations() const {
    std::vector<glm::vec3> out;
    out.reserve(m_nodes->size());
    for (int i = 0; i < static_cast<int>(m_nodes->size()); ++i)
        out.emplace_back(static_cast<float>(m_u[DPN*i+0]),
                         static_cast<float>(m_u[DPN*i+1]),
                         static_cast<float>(m_u[DPN*i+2]));
    return out;
}

std::vector<glm::vec3> FrameSimulator::getNodeRotations() const {
    std::vector<glm::vec3> out;
    out.reserve(m_nodes->size());
    for (int i = 0; i < static_cast<int>(m_nodes->size()); ++i)
        out.emplace_back(static_cast<float>(m_u[DPN*i+3]),
                         static_cast<float>(m_u[DPN*i+4]),
                         static_cast<float>(m_u[DPN*i+5]));
    return out;
}

glm::vec3 FrameSimulator::getNodeReactionForce(int nodeIdx) const {
    glm::vec3 r(0.0f);
    const int nNodes = static_cast<int>(m_nodes->size());
    if (nodeIdx < 0 || nodeIdx >= nNodes || m_reactions.size() < DPN*nNodes) return r;
    const Node& nd = (*m_nodes)[nodeIdx];
    if (isDofConstrained(nd, 0)) r.x = static_cast<float>(m_reactions[DPN*nodeIdx+0]);
    if (isDofConstrained(nd, 1)) r.y = static_cast<float>(m_reactions[DPN*nodeIdx+1]);
    if (isDofConstrained(nd, 2)) r.z = static_cast<float>(m_reactions[DPN*nodeIdx+2]);
    return r;
}

glm::vec3 FrameSimulator::getNodeReactionMoment(int nodeIdx) const {
    glm::vec3 m(0.0f);
    const int nNodes = static_cast<int>(m_nodes->size());
    if (nodeIdx < 0 || nodeIdx >= nNodes || m_reactions.size() < DPN*nNodes) return m;
    const Node& nd = (*m_nodes)[nodeIdx];
    if (isDofConstrained(nd, 3)) m.x = static_cast<float>(m_reactions[DPN*nodeIdx+3]);
    if (isDofConstrained(nd, 4)) m.y = static_cast<float>(m_reactions[DPN*nodeIdx+4]);
    if (isDofConstrained(nd, 5)) m.z = static_cast<float>(m_reactions[DPN*nodeIdx+5]);
    return m;
}

bool FrameSimulator::checkForceEquilibrium(glm::vec3& netResidual, float tol) const {
    netResidual = glm::vec3(0.0f);
    for (int i = 0; i < static_cast<int>(m_nodes->size()); ++i) {
        netResidual += (*m_nodes)[i].getAppliedForce();
        netResidual += getNodeReactionForce(i);
    }
    return std::abs(netResidual.x) <= tol
        && std::abs(netResidual.y) <= tol
        && std::abs(netResidual.z) <= tol;
}
