// Copyright (c) 2026 Colile Sibanda. All rights reserved.
// Proprietary — see LICENSE for terms. Unauthorised use prohibited.
#pragma once
#include <Eigen/Dense>
#include <glm/glm.hpp>
#include <array>

// physics/FrameElement.hpp : 3D Euler-Bernoulli frame element (12 DOF).
// Pure functions (no I/O, no solving) so the stiffness math is unit-testable.
// DOF order per node: [ux, uy, uz, rx, ry, rz]; element DOF vector is
// [node1(6), node2(6)] = 12. Local x-axis runs along the member.

namespace FrameElement {

using Mat12 = Eigen::Matrix<double, 12, 12>;
using Mat3  = Eigen::Matrix<double, 3, 3>;
using Vec12 = Eigen::Matrix<double, 12, 1>;

// A member-end release mask in local DOF order [node1(6), node2(6)]. A released
// DOF transmits no force/moment (an internal hinge releases the bending
// rotations ry, rz). Default-constructed = all false = a fully rigid element.
using Releases = std::array<bool, 12>;

// localStiffness
// Purpose: build the 12x12 stiffness matrix in the element's local frame.
// Inputs:  L length (m); E, A, G, J, Iy, Iz section/material properties (SI).
// Output:  symmetric 12x12 local stiffness matrix.
Mat12 localStiffness(double L, double E, double A,
                     double G, double J, double Iy, double Iz);

// condenseReleases
// Purpose: statically condense the released local DOFs out of a 12x12 local
//   stiffness so a released DOF transmits no force — a member-end release /
//   internal hinge. Each released DOF p is eliminated by the rank-1 update
//   k_ij -= k_ip·k_pj / k_pp, then its row/column are zeroed; applying this
//   sequentially gives the correct multi-release result. Done in the LOCAL
//   frame, before transformation, so the released axis follows the member.
// Inputs:  k local stiffness; released mask over the 12 local DOFs.
// Output:  modified 12x12 local stiffness (released rows/cols zeroed).
Mat12 condenseReleases(Mat12 k, const Releases& released);

// rotation
// Purpose: 3x3 rotation whose rows are the local x',y',z' axes in global coords.
//          x' is along (p2 - p1); a global reference picks the bending plane.
// Inputs:  p1, p2 element end positions; outLength receives the member length.
// Output:  3x3 rotation matrix (returns identity-ish if degenerate).
Mat3 rotation(const glm::vec3& p1, const glm::vec3& p2, double& outLength);

// globalStiffness
// Purpose: element stiffness transformed into global coordinates, T^T k T,
//          with any member-end releases condensed out first (in the local frame).
// Inputs:  p1, p2 positions; E, A, G, J, Iy, Iz properties; optional release mask.
// Output:  12x12 global element stiffness (zero matrix if member length ~ 0).
Mat12 globalStiffness(const glm::vec3& p1, const glm::vec3& p2,
                      double E, double A, double G, double J,
                      double Iy, double Iz, const Releases& released = Releases{});

// localEndForces
// Purpose: member-end forces in local coordinates, p = k_local · T · u_global.
//          Used to derive axial/shear/moment/torsion diagrams.
// Inputs:  p1, p2 positions; section/material props; uGlobalElem the element's
//          12 global DOFs ([node1 6][node2 6]).
//          Released DOFs read ~0 (a hinge transmits no moment), and the
//          retained forces already account for the release via condensation.
// Output:  12-vector [N1,Vy1,Vz1,T1,My1,Mz1, N2,Vy2,Vz2,T2,My2,Mz2] (local).
Vec12 localEndForces(const glm::vec3& p1, const glm::vec3& p2,
                     double E, double A, double G, double J,
                     double Iy, double Iz, const Vec12& uGlobalElem,
                     const Releases& released = Releases{});

} // namespace FrameElement
