#pragma once
#include <SDL2/SDL.h>
#include <glm/glm.hpp>
#include <vector>
#include "../model/Node.hpp"
#include "../model/Beam.hpp"

enum class ToolMode { SELECT, NODE_PLACEMENT, BEAM_CREATION, FORCE_APPLICATION };

class UIHandler {
public:
    UIHandler()  = default;
    ~UIHandler() = default;

    void initialize(int screenW, int screenH);

    // Pass every SDL event here before your own handling.
    void handleEvent(SDL_Event& e,
                     std::vector<Node>& nodes,
                     std::vector<Beam>& beams,
                     const glm::mat4& view,
                     const glm::mat4& proj);

    // Call once per frame, after 3-D rendering, inside the ImGui frame.
    void renderUI(SDL_Window* window,
                  std::vector<Node>& nodes,
                  std::vector<Beam>& beams);

    ToolMode getCurrentTool()  const { return currentTool; }
    glm::vec3 getForceVector() const { return forceVector; }

    // Returns true once (resets the flag) when the solver should re-run.
    bool consumeNeedsSolve() { bool v = needsSolveFlag; needsSolveFlag = false; return v; }

    glm::vec3 currentMouseWorldPos = {};

private:
    ToolMode  currentTool  = ToolMode::NODE_PLACEMENT;
    Node*     selectedNode = nullptr;
    Node*     beamStart    = nullptr;
    bool      draggingNode = false;
    bool      needsSolveFlag = false;

    glm::vec3 forceVector       = {0.0f, -1000.0f, 0.0f};
    float     forceMagX         =  0.0f;
    float     forceMagY         = -1000.0f;
    float     forceMagZ         =  0.0f;

    int screenWidth  = 800;
    int screenHeight = 600;

    Node*     findNodeUnderCursor(const glm::vec3& w, std::vector<Node>& nodes);
    glm::vec3 screenToWorld(int mx, int my,
                            const glm::mat4& view,
                            const glm::mat4& proj) const;
};
