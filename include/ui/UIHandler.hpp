#pragma once
#include <SDL2/SDL.h>
#include <vector>
#include "../model/Node.hpp"
#include "../model/Beam.hpp"

enum class ToolMode { NODE_PLACEMENT, BEAM_CREATION, FORCE_APPLICATION };

class UIHandler {
public:
    void handleEvent(SDL_Event& e, std::vector<Node>& nodes, 
                    std::vector<Beam>& beams, glm::vec3 worldPos);
    
    ToolMode getCurrentTool() const;
    void renderUI(SDL_Window* window);

private:
    ToolMode currentTool = ToolMode::NODE_PLACEMENT;
    Node* selectedNode = nullptr;
    glm::vec3 forceVector = {0, 0, 0};
    
    void renderToolbar(SDL_Window* window);
    glm::vec2 screenToWorld(int x, int y, SDL_Window* window);
};
