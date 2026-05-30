// Copyright (c) 2026 Colile Sibanda. All rights reserved.
// Proprietary — see LICENSE for terms. Unauthorised use prohibited.
#pragma once
#include <Eigen/Dense>
#include <glm/glm.hpp>

// physics/FrameElement.hpp : 3D Euler-Bernoulli frame element (12 DOF).
// Pure functions (no I/O, no solving) so the stiffness math is unit-testable.
// DOF order per node: [ux, uy, uz, rx, ry, rz]; element DOF vector is
// [node1(6), node2(6)] = 12. Local x-axis runs along the member.

namespace FrameElement {

using Mat12 = Eigen::Matrix<double, 12, 12>;
using Mat3  = Eigen::Matrix<double, 3, 3>;
using Vec12 = Eigen::Matrix<double, 12, 1>;

// localStiffness
// Purpose: build the 12x12 stiffness matrix in the element's local frame.
// Inputs:  L length (m); E, A, G, J, Iy, Iz section/material properties (SI).
// Output:  symmetric 12x12 local stiffness matrix.
Mat12 localStiffness(double L, double E, double A,
                     double G, double J, double Iy, double Iz);

// rotation
// Purpose: 3x3 rotation whose rows are the local x',y',z' axes in global coords.
//          x' is along (p2 - p1); a global reference picks the bending plane.
// Inputs:  p1, p2 element end positions; outLength receives the member length.
// Output:  3x3 rotation matrix (returns identity-ish if degenerate).
Mat3 rotation(const glm::vec3& p1, const glm::vec3& p2, double& outLength);

// globalStiffness
// Purpose: element stiffness transformed into global coordinates, T^T k T.
// Inputs:  p1, p2 positions; E, A, G, J, Iy, Iz properties.
// Output:  12x12 global element stiffness (zero matrix if member length ~ 0).
Mat12 globalStiffness(const glm::vec3& p1, const glm::vec3& p2,
                      double E, double A, double G, double J,
                      double Iy, double Iz);

// localEndForces
// Purpose: member-end forces in local coordinates, p = k_local · T · u_global.
//          Used to derive axial/shear/moment/torsion diagrams.
// Inputs:  p1, p2 positions; section/material props; uGlobalElem the element's
//          12 global DOFs ([node1 6][node2 6]).
// Output:  12-vector [N1,Vy1,Vz1,T1,My1,Mz1, N2,Vy2,Vz2,T2,My2,Mz2] (local).
Vec12 localEndForces(const glm::vec3& p1, const glm::vec3& p2,
                     double E, double A, double G, double J,
                     double Iy, double Iz, const Vec12& uGlobalElem);

} // namespace FrameElement
