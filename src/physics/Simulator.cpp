#include "physics/Simulator.hpp"
#include <iostream>

Simulator::Simulator(std::vector<Node>& nodes, std::vector<Beam>& beams)
    : m_nodes(nodes), m_beams(beams) {
    const int numDofs = 3 * nodes.size();
    m_globalK.resize(numDofs, numDofs);
    m_forces.resize(numDofs);
    m_displacements.resize(numDofs);
}

void Simulator::assembleGlobalStiffnessMatrix() {
    // Assemble stiffness matrix for each beam
    for (const Beam& beam : m_beams) {
        const int i = &beam.getStart() - &m_nodes[0]; // Get node index
        const int j = &beam.getEnd() - &m_nodes[0];
        
        const double AE_L = beam.getStiffness();
        Eigen::Matrix3d kBlock = AE_L * Eigen::Matrix3d::Identity();
        
        // Add to global matrix
        m_globalK.block<3,3>(3*i, 3*i) += kBlock;
        m_globalK.block<3,3>(3*j, 3*j) += kBlock;
        m_globalK.block<3,3>(3*i, 3*j) -= kBlock;
        m_globalK.block<3,3>(3*j, 3*i) -= kBlock;
    }
}

void Simulator::applySupportConstraints() {
    for (size_t i = 0; i < m_nodes.size(); ++i) {
        if (m_nodes[i].isFixed()) {
            m_globalK.block<3,3>(3*i, 3*i) = Eigen::Matrix3d::Identity();
            m_forces.segment<3>(3*i).setZero();
        }
    }
}

void Simulator::solveStaticForces() {
    assembleGlobalStiffnessMatrix();
    applySupportConstraints();
    
    Eigen::ConjugateGradient<Eigen::SparseMatrix<double>> solver;
    solver.compute(m_globalK);
    m_displacements = solver.solve(m_forces);
}
