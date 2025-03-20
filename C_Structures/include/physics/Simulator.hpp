#pragma once
#include <Eigen/Sparse>
#include <vector>
#include "../model/Node.hpp"
#include "../model/Beam.hpp"

class Simulator {
public:
    Simulator(std::vector<Node>& nodes, std::vector<Beam>& beams);
    
    void solveStaticForces();
    std::vector<float> getNodeDisplacements() const;

private:
    void assembleGlobalStiffnessMatrix();
    void applySupportConstraints();

    std::vector<Node>& m_nodes;
    std::vector<Beam>& m_beams;
    Eigen::SparseMatrix<double> m_globalK;
    Eigen::VectorXd m_forces;
    Eigen::VectorXd m_displacements;
};
