#pragma once
#include <vector>
#include <string>
#include "../model/Node.hpp"
#include "../model/Beam.hpp"

class CSVHandler {
public:
    static void loadStructure(const std::string& path, 
                            std::vector<Node>& nodes,
                            std::vector<Beam>& beams);
    
    static void saveStructure(const std::string& path, 
                            const std::vector<Node>& nodes,
                            const std::vector<Beam>& beams);
};
