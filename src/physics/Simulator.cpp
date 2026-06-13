// Copyright (c) 2026 Colile Sibanda. All rights reserved.
// Proprietary — see LICENSE for terms. Unauthorised use prohibited.
#include "physics/Simulator.hpp"
#include <Eigen/SparseLU>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <functional>
#include <algorithm>

// Fold a value into a running hash (FNV-ish mix). Doubles are hashed by their
// exact bit pattern so any change to geometry/section flips the signature.
namespace {
inline void hashMix(std::size_t& h, std::uint64_t v) {
    h ^= std::hash<std::uint64_t>{}(v) + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
}
inline void hashMix(std::size_t& h, double v) {
    std::uint64_t b; std::memcpy(&b, &v, sizeof b); hashMix(h, b);
}
inline void hashMix(std::size_t& h, int v) {
    hashMix(h, static_cast<std::uint64_t>(static_cast<std::uint32_t>(v)));
}
} // namespace

Simulator::Simulator(std::vector<Node>& nodes, std::vector<Beam>& beams)
    : m_nodes(&nodes), m_beams(&beams) {
    const int n = 3 * static_cast<int>(nodes.size());
    m_globalK.resize(n, n);
    m_forces.resize(n);
    m_displacements.resize(n);
    m_reactions.resize(n);
    m_forces.setZero();
    m_displacements.setZero();
    m_reactions.setZero();
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

    const int nNodes = static_cast<int>(m_nodes->size());
    for (const Beam& beam : *m_beams) {
        const int i = beam.getStartIdx();
        const int j = beam.getEndIdx();
        if (i < 0 || j < 0 || i >= nNodes || j >= nNodes) continue;

        glm::vec3 axis = (*m_nodes)[j].getPosition() - (*m_nodes)[i].getPosition();
        float L = glm::length(axis);
        if (L < 1e-8f) continue;

        double lx = axis.x / L, ly = axis.y / L, lz = axis.z / L;
        double AE_L = static_cast<double>(beam.getStiffness(*m_nodes));

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

// stiffnessSignature
// Purpose: hash the data that determines KFF so an unchanged structure (only the
//          loads differ) can reuse the cached factorisation.
// Inputs:  current node positions/joint types and beam connectivity/AE.
// Output:  a signature; equal signatures ⇒ the same factorisable stiffness.
std::size_t Simulator::stiffnessSignature() const {
    std::size_t h = 1469598103934665603ULL;
    hashMix(h, static_cast<int>(m_nodes->size()));
    for (const Node& nd : *m_nodes) {
        glm::vec3 p = nd.getPosition();
        hashMix(h, static_cast<double>(p.x));
        hashMix(h, static_cast<double>(p.y));
        hashMix(h, static_cast<double>(p.z));
        hashMix(h, static_cast<int>(nd.getJointType()));
    }
    for (const Beam& b : *m_beams) {
        hashMix(h, b.getStartIdx());
        hashMix(h, b.getEndIdx());
        hashMix(h, static_cast<double>(b.getYoungsModulus()));
        hashMix(h, static_cast<double>(b.getCrossSection()));
    }
    return h;
}

// Proper static condensation: extract the free-DOF sub-system and solve it
// directly.  This avoids the ill-conditioning of the penalty-BC approach and
// works regardless of whether SimplicialLDLT recognises the matrix as SPD.
//
// The factorisation of KFF and the free-DOF map are cached and reused whenever
// the stiffness signature is unchanged, so editing a load re-solves with only a
// fresh back-substitution. Both paths produce identical displacements/reactions.
SolveResult Simulator::solveStaticForces() {
    m_reactions.setZero();
    if (m_nodes->empty()) { m_factorized = false; return {}; }

    const int nNodes = static_cast<int>(m_nodes->size());
    const int n = 3 * nNodes;
    // Re-size working storage if the node count changed since construction; this
    // lets the simulator outlive model edits without being reconstructed.
    if (static_cast<int>(m_forces.size()) != n) {
        m_globalK.resize(n, n);
        m_forces.resize(n);
        m_displacements.resize(n);
        m_reactions.resize(n);
        m_reactions.setZero();
        m_factorized = false;
    }
    if (m_beams->empty()) { m_displacements.setZero(); m_factorized = false; return {}; }

    populateForceVector();

    const std::size_t sig = stiffnessSignature();
    const bool reuse = m_factorized && sig == m_stiffnessSig;

    if (!reuse) {
        assembleGlobalStiffnessMatrix();

        // --- Step 1: determine fixed DOFs ------------------------------------
        std::vector<bool> isFixed(n, false);
        for (int i = 0; i < nNodes; ++i) {
            const Node& nd = (*m_nodes)[i];
            for (int d = 0; d < 3; ++d)
                if (nd.isDOFConstrained(d))
                    isFixed[3*i + d] = true;
        }
        // Pin any DOF whose diagonal is zero (no stiffness — unstable direction).
        for (int i = 0; i < n; ++i)
            if (!isFixed[i] && std::abs(m_globalK.coeff(i, i)) < 1e-14)
                isFixed[i] = true;

        // --- Step 2: build (and cache) the free-DOF index map ----------------
        m_freeList.clear();
        m_freeList.reserve(n);
        m_globalToFree.assign(n, -1);
        for (int i = 0; i < n; ++i) {
            if (!isFixed[i]) {
                m_globalToFree[i] = static_cast<int>(m_freeList.size());
                m_freeList.push_back(i);
            }
        }
        const int nf = static_cast<int>(m_freeList.size());

        // --- Step 3: extract and factorise KFF -------------------------------
        if (nf > 0) {
            std::vector<Eigen::Triplet<double>> sub;
            sub.reserve(nf * 6);
            for (int gCol : m_freeList) {
                int lCol = m_globalToFree[gCol];
                for (Eigen::SparseMatrix<double>::InnerIterator it(m_globalK, gCol); it; ++it) {
                    int gRow = static_cast<int>(it.row());
                    if (isFixed[gRow]) continue;
                    sub.emplace_back(m_globalToFree[gRow], lCol, it.value());
                }
            }
            Eigen::SparseMatrix<double> Kff(nf, nf);
            Kff.setFromTriplets(sub.begin(), sub.end());
            Kff.makeCompressed();
            m_solver.compute(Kff);
            if (m_solver.info() != Eigen::Success) {
                m_factorized = false;
                return {SolveStatus::MECHANISM,
                        "Structure is a mechanism or under-constrained \xe2\x80\x94 add supports."};
            }
        }
        m_factorized   = true;
        m_stiffnessSig = sig;
    }

    const int nf = static_cast<int>(m_freeList.size());
    m_displacements.setZero();
    if (nf == 0) { // fully constrained: u = 0, reactions resist all applied load
        m_reactions = m_globalK * m_displacements - m_forces;
        return {};
    }

    // --- Step 4: solve (back-substitution on the cached factorisation) -------
    Eigen::VectorXd ff(nf);
    for (int li = 0; li < nf; ++li)
        ff[li] = m_forces[m_freeList[li]];
    Eigen::VectorXd uf = m_solver.solve(ff);
    if (m_solver.info() != Eigen::Success) {
        m_factorized = false;
        return {SolveStatus::FAILED,
                "Solver failed \xe2\x80\x94 check model for singularities."};
    }

    // --- Step 5: scatter back ------------------------------------------------
    for (int li = 0; li < nf; ++li)
        m_displacements[m_freeList[li]] = uf[li];

    // --- Step 6: cache reactions (residual at every DOF) ---------------------
    // r = K*u - F. At free DOFs r ≈ 0; at constrained DOFs r is the support reaction.
    m_reactions = m_globalK * m_displacements - m_forces;
    return {};
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
    const int i = beam.getStartIdx();
    const int j = beam.getEndIdx();
    const int nNodes = static_cast<int>(m_nodes->size());
    if (i < 0 || j < 0 || i >= nNodes || j >= nNodes) return 0.0f;

    glm::vec3 dI(m_displacements[3*i], m_displacements[3*i+1], m_displacements[3*i+2]);
    glm::vec3 dJ(m_displacements[3*j], m_displacements[3*j+1], m_displacements[3*j+2]);

    glm::vec3 axis   = (*m_nodes)[j].getPosition() - (*m_nodes)[i].getPosition();
    float     length = glm::length(axis);
    if (length < 1e-6f) return 0.0f;

    return beam.getStiffness(*m_nodes) * glm::dot(dJ - dI, axis / length);
}

// getNodeReaction
// Purpose: report the support reaction at a node from the cached residual.
// Inputs:  nodeIdx — node index.
// Output:  reaction force (N); components are non-zero only on constrained DOFs.
glm::vec3 Simulator::getNodeReaction(int nodeIdx) const {
    glm::vec3 r(0.0f);
    const int nNodes = static_cast<int>(m_nodes->size());
    if (nodeIdx < 0 || nodeIdx >= nNodes) return r;
    if (m_reactions.size() < 3 * nNodes) return r;

    const Node& nd = (*m_nodes)[nodeIdx];
    if (nd.isDOFConstrained(0)) r.x = static_cast<float>(m_reactions[3*nodeIdx+0]);
    if (nd.isDOFConstrained(1)) r.y = static_cast<float>(m_reactions[3*nodeIdx+1]);
    if (nd.isDOFConstrained(2)) r.z = static_cast<float>(m_reactions[3*nodeIdx+2]);
    return r;
}

// checkEquilibrium
// Purpose: verify global static equilibrium Σ(applied) + Σ(reactions) ≈ 0.
// Inputs:  netResidual — out param receiving the net unbalanced force (N).
//          tol — magnitude tolerance per axis (N).
// Output:  true if |netResidual| components are all within tol.
bool Simulator::checkEquilibrium(glm::vec3& netResidual, float tol) const {
    netResidual = glm::vec3(0.0f);
    const int nNodes = static_cast<int>(m_nodes->size());
    for (int i = 0; i < nNodes; ++i) {
        netResidual += (*m_nodes)[i].getAppliedForce();
        netResidual += getNodeReaction(i);
    }
    return std::abs(netResidual.x) <= tol
        && std::abs(netResidual.y) <= tol
        && std::abs(netResidual.z) <= tol;
}
