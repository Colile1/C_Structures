// Copyright (c) 2026 Colile Sibanda. All rights reserved.
// Proprietary — see LICENSE for terms. Unauthorised use prohibited.
// physics/FrameSimulator.cpp : assembly and solve for the 6-DOF frame model.
#include "physics/FrameSimulator.hpp"
#include "physics/FrameElement.hpp"
#include <Eigen/SparseLU>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <functional>

// Fold a value into a running hash (FNV-ish mix). Doubles are hashed by their
// exact bit pattern so any geometry/section change flips the signature.
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

FrameSimulator::FrameSimulator(std::vector<Node>& nodes, std::vector<Beam>& beams)
    : m_nodes(&nodes), m_beams(&beams) {
    const int n = DPN * static_cast<int>(nodes.size());
    m_K.resize(n, n);
    m_F.resize(n);     m_F.setZero();
    m_u.resize(n);     m_u.setZero();
    m_reactions.resize(n); m_reactions.setZero();
}

bool FrameSimulator::isDofConstrained(const Node& nd, int dof) const {
    if (dof < 3) return nd.isDOFConstrained(dof); // translations via existing per-DOF check
    // Rotations (dof 3=Rx, 4=Ry, 5=Rz):
    // FIXED restrains all rotations. All other support types (pin, rollers) leave
    // rotations free — they only restrain specific translations. An internal hinge
    // (FREE joint) is also rotation-free. This matches standard frame analysis
    // where roller and pin supports are moment-releases at the support node.
    return nd.getJointType() == JointType::FIXED;
}

// Map a beam's internal-hinge flags to the element's local release mask. A
// moment release frees the two transverse bending rotations (ry, rz) at that
// end so the member transmits force but no bending moment to its node.
static FrameElement::Releases beamReleases(const Beam& beam) {
    FrameElement::Releases r{};
    if (beam.getStartMomentRelease()) { r[4]  = true; r[5]  = true; }
    if (beam.getEndMomentRelease())   { r[10] = true; r[11] = true; }
    return r;
}

std::vector<DistributedLoad> FrameSimulator::effectiveLoads() const {
    std::vector<DistributedLoad> loads = m_distLoads;
    if (m_selfWeight) {
        std::vector<DistributedLoad> sw = selfWeightLoads(*m_beams);
        loads.insert(loads.end(), sw.begin(), sw.end());
    }
    return loads;
}

void FrameSimulator::populateForces() {
    m_F.setZero();
    const int nNodes = static_cast<int>(m_nodes->size());
    for (int i = 0; i < nNodes; ++i) {
        glm::vec3 f = (*m_nodes)[i].getAppliedForce();
        m_F[DPN*i + 0] = static_cast<double>(f.x);
        m_F[DPN*i + 1] = static_cast<double>(f.y);
        m_F[DPN*i + 2] = static_cast<double>(f.z);
        // Concentrated nodal moments drive the three rotational DOFs (Mx, My, Mz).
        glm::vec3 m = (*m_nodes)[i].getAppliedMoment();
        m_F[DPN*i + 3] = static_cast<double>(m.x);
        m_F[DPN*i + 4] = static_cast<double>(m.y);
        m_F[DPN*i + 5] = static_cast<double>(m.z);
    }

    // Consistent equivalent nodal loads from distributed/moment/self-weight loads.
    std::vector<DistributedLoad> loads = effectiveLoads();
    if (!loads.empty()) {
        const int nBeams = static_cast<int>(m_beams->size());
        std::vector<double> Fvec(DPN * nNodes, 0.0);
        std::vector<glm::vec3> pos(nNodes);
        for (int i = 0; i < nNodes; ++i) pos[i] = (*m_nodes)[i].getPosition();
        std::vector<std::pair<int,int>> conn(nBeams);
        for (int i = 0; i < nBeams; ++i)
            conn[i] = { (*m_beams)[i].getStartIdx(), (*m_beams)[i].getEndIdx() };
        applyDistributedLoads(loads, pos, conn, Fvec);
        for (int i = 0; i < DPN * nNodes; ++i) m_F[i] += Fvec[i];
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
            E, A, G, J, Iy, Iz, beamReleases(beam));

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

// stiffnessSignature
// Purpose: hash the data that determines KFF and the free-DOF set so an
//          unchanged structure (only the loads differ) can reuse the cached
//          factorisation. Loads (nodal moments, distributed, self-weight) are
//          deliberately excluded — changing them must not force a re-factorise.
std::size_t FrameSimulator::stiffnessSignature() const {
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
        hashMix(h, static_cast<double>(b.getMomentOfInertia()));
        hashMix(h, b.getStartMomentRelease() ? 1 : 0);
        hashMix(h, b.getEndMomentRelease()   ? 1 : 0);
    }
    return h;
}

// The factorisation of KFF and the free-DOF map are cached and reused while the
// stiffness signature is unchanged, so editing a load re-solves with only a
// fresh back-substitution. Both paths produce identical displacements/reactions.
SolveResult FrameSimulator::solve() {
    m_reactions.setZero();
    if (m_nodes->empty()) { m_factorized = false; return {}; }

    const int nNodes = static_cast<int>(m_nodes->size());
    const int n = DPN * nNodes;
    // Re-size working storage if the node count changed since construction; this
    // lets the simulator outlive model edits without being reconstructed.
    if (static_cast<int>(m_F.size()) != n) {
        m_K.resize(n, n);
        m_F.resize(n);
        m_u.resize(n);
        m_reactions.resize(n);
        m_reactions.setZero();
        m_factorized = false;
    }
    if (m_beams->empty()) { m_u.setZero(); m_factorized = false; return {}; }

    populateForces();

    const std::size_t sig = stiffnessSignature();
    const bool reuse = m_factorized && sig == m_stiffnessSig;

    if (!reuse) {
        assemble();

        // Fixed DOFs from joint types, plus any DOF with no stiffness (auto-pin).
        std::vector<bool> fixed(n, false);
        for (int i = 0; i < nNodes; ++i)
            for (int d = 0; d < DPN; ++d)
                if (isDofConstrained((*m_nodes)[i], d))
                    fixed[DPN*i + d] = true;
        for (int i = 0; i < n; ++i)
            if (!fixed[i] && std::abs(m_K.coeff(i,i)) < 1e-14)
                fixed[i] = true;

        // Free-DOF index map (cached for reuse).
        m_freeList.clear();
        m_toFree.assign(n, -1);
        for (int i = 0; i < n; ++i)
            if (!fixed[i]) { m_toFree[i] = static_cast<int>(m_freeList.size()); m_freeList.push_back(i); }
        const int nf = static_cast<int>(m_freeList.size());

        if (nf > 0) {
            std::vector<Eigen::Triplet<double>> sub;
            sub.reserve(nf * 12);
            for (int gCol : m_freeList) {
                int lCol = m_toFree[gCol];
                for (Eigen::SparseMatrix<double>::InnerIterator it(m_K, gCol); it; ++it) {
                    int gRow = static_cast<int>(it.row());
                    if (fixed[gRow]) continue;
                    sub.emplace_back(m_toFree[gRow], lCol, it.value());
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
    m_u.setZero();
    if (nf == 0) { m_reactions = m_K * m_u - m_F; return {}; }

    Eigen::VectorXd fF(nf);
    for (int li = 0; li < nf; ++li) fF[li] = m_F[m_freeList[li]];

    Eigen::VectorXd uF = m_solver.solve(fF);
    if (m_solver.info() != Eigen::Success) {
        m_factorized = false;
        return {SolveStatus::FAILED,
                "Solver failed \xe2\x80\x94 check model for singularities."};
    }
    for (int li = 0; li < nf; ++li) m_u[m_freeList[li]] = uF[li];

    m_reactions = m_K * m_u - m_F; // residual: reactions at constrained DOFs
    return {};
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

std::array<float, 12> FrameSimulator::getMemberEndForces(const Beam& beam) const {
    std::array<float, 12> out{};
    const int nNodes = static_cast<int>(m_nodes->size());
    const int i = beam.getStartIdx(), j = beam.getEndIdx();
    if (i < 0 || j < 0 || i >= nNodes || j >= nNodes || i == j) return out;

    Eigen::Matrix<double, 12, 1> ue;
    for (int d = 0; d < DPN; ++d) {
        ue[d]       = m_u[DPN*i + d];
        ue[DPN + d] = m_u[DPN*j + d];
    }
    const double E  = beam.getYoungsModulus();
    const double A  = beam.getCrossSection();
    const double I  = beam.getMomentOfInertia();
    const double G  = E / (2.0 * (1.0 + 0.3));
    const double J  = 2.0 * I;

    FrameElement::Vec12 p = FrameElement::localEndForces(
        (*m_nodes)[i].getPosition(), (*m_nodes)[j].getPosition(),
        E, A, G, J, I, I, ue, beamReleases(beam));

    // localEndForces gives only the k·u part. The true member-end forces add the
    // fixed-end forces f^F = -(local CENL): p_true = k·u - Σ CENL_local. Without
    // this, a loaded member's end shear/moment are wrong (e.g. a UDL on a pinned
    // member would report ±wL²/12 instead of zero end moment).
    const int bi = beamIndex(beam);
    if (bi >= 0) {
        const glm::vec3 pi = (*m_nodes)[i].getPosition();
        const glm::vec3 pj = (*m_nodes)[j].getPosition();
        const float L = glm::length(pj - pi);
        for (const auto& dl : effectiveLoads()) {
            if (dl.beamIdx != bi) continue;
            auto cenl = consistentNodalLoadsLocal(dl, pi, pj, L);
            for (int k = 0; k < 12; ++k) p[k] -= cenl[k];
        }
    }

    for (int k = 0; k < 12; ++k) out[k] = static_cast<float>(p[k]);
    return out;
}

int FrameSimulator::beamIndex(const Beam& beam) const {
    for (int b = 0; b < static_cast<int>(m_beams->size()); ++b)
        if (&(*m_beams)[b] == &beam) return b;
    return -1;
}

SpanLoad FrameSimulator::getMemberSpanLoad(const Beam& beam) const {
    SpanLoad out;
    const int bi = beamIndex(beam);
    if (bi < 0) return out;
    const int nNodes = static_cast<int>(m_nodes->size());
    const int i = beam.getStartIdx(), j = beam.getEndIdx();
    if (i < 0 || j < 0 || i >= nNodes || j >= nNodes || i == j) return out;

    const glm::vec3 pi = (*m_nodes)[i].getPosition();
    const glm::vec3 pj = (*m_nodes)[j].getPosition();
    const float L = glm::length(pj - pi);
    for (const auto& dl : effectiveLoads()) {
        if (dl.beamIdx != bi) continue;
        SpanLoad s = spanLoadLocal(dl, pi, pj, L);
        out.qy0 += s.qy0; out.qyL += s.qyL;
        out.qz0 += s.qz0; out.qzL += s.qzL;
        for (const auto& m : s.moments) out.moments.push_back(m);
    }
    return out;
}

bool FrameSimulator::checkForceEquilibrium(glm::vec3& netResidual, float tol) const {
    netResidual = glm::vec3(0.0f);
    for (int i = 0; i < static_cast<int>(m_nodes->size()); ++i) {
        netResidual += (*m_nodes)[i].getAppliedForce();
        netResidual += getNodeReactionForce(i);
    }
    // Distributed and self-weight loads also act on the structure: add each
    // load's global force resultant (the translational part of its CENL) so the
    // check balances them against the reactions, not just the nodal point loads.
    const int nNodes = static_cast<int>(m_nodes->size());
    for (const auto& dl : effectiveLoads()) {
        if (dl.beamIdx < 0 || dl.beamIdx >= static_cast<int>(m_beams->size())) continue;
        const Beam& bm = (*m_beams)[dl.beamIdx];
        const int si = bm.getStartIdx(), ei = bm.getEndIdx();
        if (si < 0 || ei < 0 || si >= nNodes || ei >= nNodes) continue;
        const glm::vec3 pi = (*m_nodes)[si].getPosition();
        const glm::vec3 pj = (*m_nodes)[ei].getPosition();
        const float L = glm::length(pj - pi);
        auto cenl = consistentNodalLoads(dl, pi, pj, L);
        netResidual.x += static_cast<float>(cenl[0] + cenl[6]);
        netResidual.y += static_cast<float>(cenl[1] + cenl[7]);
        netResidual.z += static_cast<float>(cenl[2] + cenl[8]);
    }
    return std::abs(netResidual.x) <= tol
        && std::abs(netResidual.y) <= tol
        && std::abs(netResidual.z) <= tol;
}
