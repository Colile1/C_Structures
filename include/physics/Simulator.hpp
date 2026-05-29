#pragma once
#include <Eigen/Sparse>
#include <glm/glm.hpp>
#include <vector>
#include "../model/Node.hpp"
#include "../model/Beam.hpp"

// physics/Simulator.hpp : static force solver using Eigen sparse matrices.
class Simulator {
public:
    Simulator(std::vector<Node>& nodes, std::vector<Beam>& beams);

    void solveStaticForces();

    // Returns per-node displacement as 3D vectors (x,y,z) in metres.
    std::vector<glm::vec3> getNodeDisplacements() const;

    // Returns signed axial force in the beam (positive = tension, negative = compression).
    float getBeamForce(const Beam& beam) const;

private:
    void assembleGlobalStiffnessMatrix();
    void applySupportConstraints();
    void populateForceVector();

    std::vector<Node>* m_nodes;
    std::vector<Beam>* m_beams;
    Eigen::SparseMatrix<double> m_globalK;
    Eigen::VectorXd m_forces;
    Eigen::VectorXd m_displacements;
};
