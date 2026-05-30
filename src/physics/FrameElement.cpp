// Copyright (c) 2026 Colile Sibanda. All rights reserved.
// Proprietary — see LICENSE for terms. Unauthorised use prohibited.
// physics/FrameElement.cpp : 3D frame element stiffness and transformation.
#include "physics/FrameElement.hpp"
#include <cmath>

namespace FrameElement {

Mat12 localStiffness(double L, double E, double A,
                     double G, double J, double Iy, double Iz) {
    Mat12 k = Mat12::Zero();
    if (L <= 0.0) return k;

    const double a  = E * A / L;          // axial
    const double t  = G * J / L;          // torsion
    const double z12 = 12.0*E*Iz/(L*L*L), z6 = 6.0*E*Iz/(L*L), z4 = 4.0*E*Iz/L, z2 = 2.0*E*Iz/L;
    const double y12 = 12.0*E*Iy/(L*L*L), y6 = 6.0*E*Iy/(L*L), y4 = 4.0*E*Iy/L, y2 = 2.0*E*Iy/L;

    // Axial (dof 0,6)
    k(0,0) =  a; k(0,6) = -a; k(6,6) = a;
    // Torsion (dof 3,9)
    k(3,3) =  t; k(3,9) = -t; k(9,9) = t;
    // Bending about local z: local-y translation (1,7) + rotation z (5,11)
    k(1,1) =  z12; k(1,5) =  z6;  k(1,7)  = -z12; k(1,11) =  z6;
    k(5,5) =  z4;  k(5,7) = -z6;  k(5,11) =  z2;
    k(7,7) =  z12; k(7,11) = -z6;
    k(11,11) = z4;
    // Bending about local y: local-z translation (2,8) + rotation y (4,10)
    k(2,2) =  y12; k(2,4) = -y6;  k(2,8)  = -y12; k(2,10) = -y6;
    k(4,4) =  y4;  k(4,8) =  y6;  k(4,10) =  y2;
    k(8,8) =  y12; k(8,10) =  y6;
    k(10,10) = y4;

    // Mirror upper triangle into the lower triangle.
    for (int i = 0; i < 12; ++i)
        for (int j = i + 1; j < 12; ++j)
            k(j,i) = k(i,j);
    return k;
}

Mat3 rotation(const glm::vec3& p1, const glm::vec3& p2, double& outLength) {
    glm::vec3 d = p2 - p1;
    double L = glm::length(d);
    outLength = L;
    Mat3 R = Mat3::Identity();
    if (L < 1e-12) return R;

    glm::vec3 x = d / static_cast<float>(L);
    // Reference vector: global Y unless the member is ~parallel to Y, then global Z.
    glm::vec3 up = (std::abs(glm::dot(x, glm::vec3(0,1,0))) < 0.99f)
                       ? glm::vec3(0,1,0) : glm::vec3(0,0,1);
    glm::vec3 z = glm::normalize(glm::cross(x, up));
    glm::vec3 y = glm::cross(z, x); // already unit length

    R << x.x, x.y, x.z,
         y.x, y.y, y.z,
         z.x, z.y, z.z;
    return R;
}

Mat12 globalStiffness(const glm::vec3& p1, const glm::vec3& p2,
                      double E, double A, double G, double J,
                      double Iy, double Iz) {
    double L = 0.0;
    Mat3 R = rotation(p1, p2, L);
    if (L < 1e-12) return Mat12::Zero();

    Mat12 k = localStiffness(L, E, A, G, J, Iy, Iz);
    Mat12 T = Mat12::Zero();
    for (int b = 0; b < 4; ++b) T.block<3,3>(3*b, 3*b) = R;
    return T.transpose() * k * T;
}

} // namespace FrameElement
