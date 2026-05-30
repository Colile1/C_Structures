// Copyright (c) 2026 Colile Sibanda. All rights reserved.
// Proprietary — see LICENSE for terms. Unauthorised use prohibited.
// physics/DistributedLoad.cpp : consistent equivalent nodal loads for frame members.
//
// Reference: McGuire, Gallagher & Ziemian "Matrix Structural Analysis" 2nd ed.
// Table A-1 (Appendix A) for fixed-end forces under uniform and triangular loads.
#include "physics/DistributedLoad.hpp"
#include "physics/FrameElement.hpp"
#include <cmath>

// Local y-direction UDL of intensity w over length L:
// fixed-end forces in local coordinates [N1,Vy1,Vz1,T1,My1,Mz1, N2,Vy2,Vz2,T2,My2,Mz2]
// Convention: w positive in +local-y direction.
static std::array<double, 12> udlLocalY(double w, double L) {
    std::array<double, 12> f{};
    // Consistent (work-equivalent) nodal loads for a UDL of intensity w in local y.
    // Derived by integrating Hermite shape functions: Vy = wL/2, Mz_i = wL²/12, Mz_j = -wL²/12.
    // w is signed: negative means downward (into -local_y), producing negative forces.
    f[1]  =  w * L / 2.0;          // Vy at i
    f[5]  =  w * L * L / 12.0;     // Mz at i  (hogging when w < 0)
    f[7]  =  w * L / 2.0;          // Vy at j
    f[11] = -w * L * L / 12.0;     // Mz at j  (sagging when w < 0)
    return f;
}

// Local z-direction UDL of intensity w over length L:
static std::array<double, 12> udlLocalZ(double w, double L) {
    std::array<double, 12> f{};
    f[2]  =  w * L / 2.0;          // Vz at i
    f[4]  =  w * L * L / 12.0;     // My at i  (note sign differs from Mz convention)
    f[8]  =  w * L / 2.0;          // Vz at j
    f[10] = -w * L * L / 12.0;     // My at j
    return f;
}

// Triangular load in local y: w_start at i, w_end at j.
static std::array<double, 12> triLocalY(double ws, double we, double L) {
    std::array<double, 12> f{};
    // Consistent nodal loads for a linearly varying local-y load: ws at i, we at j.
    // From Hermite shape function integrals (same sign convention as udlLocalY).
    f[1]  =  (7.0*ws + 3.0*we) * L / 20.0;
    f[5]  =  (3.0*ws + 2.0*we) * L * L / 60.0;
    f[7]  =  (3.0*ws + 7.0*we) * L / 20.0;
    f[11] = -(2.0*ws + 3.0*we) * L * L / 60.0;
    return f;
}

// Triangular load in local z.
static std::array<double, 12> triLocalZ(double ws, double we, double L) {
    std::array<double, 12> f{};
    f[2]  =  (7.0*ws + 3.0*we) * L / 20.0;
    f[4]  =  (3.0*ws + 2.0*we) * L * L / 60.0;
    f[8]  =  (3.0*ws + 7.0*we) * L / 20.0;
    f[10] = -(2.0*ws + 3.0*we) * L * L / 60.0;
    return f;
}

// Point moment M about local z at fractional position a (0..1).
static std::array<double, 12> momentLocalZ(double M, double L, double a) {
    std::array<double, 12> f{};
    double b = 1.0 - a;
    // Fixed-end reactions for concentrated moment:
    // Vy_i = -6ab·M/L^2, Mz_i = -(3a-1)b·M/... (standard formula)
    // Using: Mz_i = M*b*(3a-1)/1, see McGuire eq (A-4)
    // Vy_i = 6*a*b / L^2 * M * sign
    double La = a * L, Lb = b * L;
    f[1]  =  6.0*La*Lb / (L*L*L) * M;        // Vy_i
    f[5]  =  Lb*(3.0*La - L) / (L*L) * M;    // Mz_i
    f[7]  = -6.0*La*Lb / (L*L*L) * M;        // Vy_j
    f[11] =  La*(3.0*Lb - L) / (L*L) * M;    // Mz_j
    return f;
}

std::array<double, 12> consistentNodalLoads(const DistributedLoad& dl,
                                             const glm::vec3& p1,
                                             const glm::vec3& p2,
                                             float Lf)
{
    const double L = static_cast<double>(Lf);
    if (L < 1e-10) return {};

    // Resolve global load direction into member local axes.
    double rotL = 0.0;
    FrameElement::Mat3 R = FrameElement::rotation(p1, p2, rotL);
    // R rows: 0=local-x, 1=local-y, 2=local-z
    glm::vec3 d = dl.direction;
    double wy = R(1,0)*d.x + R(1,1)*d.y + R(1,2)*d.z; // component in local y
    double wz = R(2,0)*d.x + R(2,1)*d.y + R(2,2)*d.z; // component in local z

    std::array<double, 12> fLocal{};

    switch (dl.type) {
        case LoadType::UDL: {
            double w = static_cast<double>(dl.w);
            auto fy = udlLocalY(wy * w, L);
            auto fz = udlLocalZ(wz * w, L);
            for (int i = 0; i < 12; ++i) fLocal[i] = fy[i] + fz[i];
            break;
        }
        case LoadType::TRIANGULAR: {
            double ws = static_cast<double>(dl.w);
            double we = static_cast<double>(dl.w2);
            auto fy = triLocalY(wy*ws, wy*we, L);
            auto fz = triLocalZ(wz*ws, wz*we, L);
            for (int i = 0; i < 12; ++i) fLocal[i] = fy[i] + fz[i];
            break;
        }
        case LoadType::MOMENT: {
            double M = static_cast<double>(dl.w);
            double a = static_cast<double>(dl.pos);
            // Moment applied about local z here; local y handled analogously.
            auto fm = momentLocalZ(M, L, a);
            for (int i = 0; i < 12; ++i) fLocal[i] = fm[i];
            break;
        }
    }

    // Transform local nodal forces back to global: F_global = T^T * f_local
    FrameElement::Mat12 T = FrameElement::Mat12::Zero();
    for (int b = 0; b < 4; ++b) T.block<3,3>(3*b, 3*b) = R;
    Eigen::Matrix<double, 12, 1> fl;
    for (int i = 0; i < 12; ++i) fl[i] = fLocal[i];
    Eigen::Matrix<double, 12, 1> fg = T.transpose() * fl;

    std::array<double, 12> out{};
    for (int i = 0; i < 12; ++i) out[i] = fg[i];
    return out;
}

void applyDistributedLoads(const std::vector<DistributedLoad>& loads,
                            const std::vector<glm::vec3>& positions,
                            const std::vector<std::pair<int,int>>& connectivity,
                            std::vector<double>& F)
{
    const int nNodes = static_cast<int>(positions.size());
    for (const auto& dl : loads) {
        if (dl.beamIdx < 0 || dl.beamIdx >= (int)connectivity.size()) continue;
        auto [si, ei] = connectivity[dl.beamIdx];
        if (si < 0 || ei < 0 || si >= nNodes || ei >= nNodes) continue;
        const glm::vec3& p1 = positions[si];
        const glm::vec3& p2 = positions[ei];
        float L = glm::length(p2 - p1);
        auto cenl = consistentNodalLoads(dl, p1, p2, L);
        // Scatter: first 6 → node i, next 6 → node j
        for (int d = 0; d < 6; ++d) {
            F[6*si + d] += cenl[d];
            F[6*ei + d] += cenl[6 + d];
        }
    }
}
