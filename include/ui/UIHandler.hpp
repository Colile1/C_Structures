#pragma once
#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include "../model/Node.hpp"
#include "../model/Beam.hpp"

// ui/UIHandler.hpp : toolbar, tool mode switching, and 2D overlay rendering.
enum class ToolMode { NODE_PLACEMENT, BEAM_CREATION, FORCE_APPLICATION };

class UIHandler {
public:
    UIHandler() = default;
    ~UIHandler();

    // Must be called after OpenGL context creation.
    void initialize(int screenW, int screenH);

    void handleEvent(SDL_Event& e, std::vector<Node>& nodes,
                     std::vector<Beam>& beams,
                     const glm::mat4& view, const glm::mat4& projection);

    ToolMode getCurrentTool() const { return currentTool; }
    void renderUI(SDL_Window* window);

    // Expose force vector so main can read it.
    glm::vec3 getForceVector() const { return forceVector; }

private:
    ToolMode currentTool = ToolMode::NODE_PLACEMENT;
    Node* selectedNode   = nullptr;
    glm::vec3 forceVector        = {1000.0f, 0.0f, 0.0f};
    glm::vec3 currentMouseWorldPos = {0.0f, 0.0f, 0.0f};
    int screenWidth  = 800;
    int screenHeight = 600;

    // 2D UI shader + geometry
    GLuint uiShader = 0;
    GLuint rectVAO  = 0;
    GLuint rectVBO  = 0;
    GLuint lineVAO  = 0;
    GLuint lineVBO  = 0;

    // Input wrapper: returns pointer to the first node within snap distance.
    Node* findNodeUnderCursor(const glm::vec3& worldPos, std::vector<Node>& nodes);

    // Pure: converts screen pixel coordinates to world XZ-plane position.
    glm::vec3 screenToWorld(int x, int y,
                            const glm::mat4& view, const glm::mat4& projection) const;

    void renderToolbar(SDL_Window* window);

    // Output: draws a filled 2D rectangle in screen pixels.
    void drawRect(float x, float y, float w, float h, const glm::vec3& color, float alpha = 1.0f);

    // Output: draws a labelled button; highlights when active.
    void drawButton(float x, float y, float w, float h, bool active);

    // Output: draws a filled slider track + knob.
    void drawSlider(float x, float y, float w, float h, float val, float minVal, float maxVal);

    // Output: draws a dashed preview line between two world positions projected to screen.
    void drawPreviewBeam(const glm::vec3& start, const glm::vec3& end,
                         const glm::mat4& view, const glm::mat4& projection);

    // Helper: sets the 2D shader's color uniform and draws the rect VAO.
    void submitRect(float x, float y, float w, float h,
                    const glm::vec3& color, float alpha);
};
