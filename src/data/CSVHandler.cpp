// Copyright (c) 2026 Colile Sibanda. All rights reserved.
// Proprietary — see LICENSE for terms. Unauthorised use prohibited.
#include "data/CSVHandler.hpp"
#include <fstream>
#include <sstream>

void CSVHandler::loadStructure(const std::string& path, 
                             std::vector<Node>& nodes,
                             std::vector<Beam>& beams) {
    std::ifstream file(path);
    std::string line;
    
    // Node format: x,y,z,fixed (0/1)
    // Beam format: start_index,end_index,youngs_modulus,cross_section
    while (getline(file, line)) {
        std::stringstream ss(line);
        std::string type;
        ss >> type;
        
        if (type == "NODE") {
            float x, y, z;
            int fixed;
            ss >> x >> y >> z >> fixed;
            
            nodes.emplace_back(x, y, z);
            nodes.back().setFixed(fixed);
        } 
        else if (type == "BEAM") {
            int startIdx, endIdx;
            float E, A;
            ss >> startIdx >> endIdx >> E >> A;

            const int n = static_cast<int>(nodes.size());
            if (startIdx < 0 || endIdx < 0 || startIdx >= n || endIdx >= n)
                continue; // skip beam referencing a non-existent node
            beams.emplace_back(startIdx, endIdx, E, A);
        }
    }
}

void CSVHandler::saveStructure(const std::string& path, 
                             const std::vector<Node>& nodes,
                             const std::vector<Beam>& beams) {
    std::ofstream file(path);
    
    // Save nodes
    for (size_t i = 0; i < nodes.size(); ++i) {
        const Node& node = nodes[i];
        file << "NODE " << node.getPosition().x << " " 
             << node.getPosition().y << " " 
             << node.getPosition().z << " " 
             << (node.isFixed() ? 1 : 0) << "\n";
    }
    
    // Save beams. Connectivity is stored directly as node indices, so the
    // mapping is exact and survives position edits (no float-equality scan).
    for (const Beam& beam : beams) {
        file << "BEAM " << beam.getStartIdx() << " " << beam.getEndIdx() << " "
             << beam.getYoungsModulus() << " "
             << beam.getCrossSection() << "\n";
    }
}
