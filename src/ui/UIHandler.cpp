#include "ui/UIHandler.hpp"
#include <glm/glm.hpp>
#include <string>
#include <cmath>

Node* UIHandler::findNodeUnderCursor(const glm::vec3& worldPos, std::vector<Node>& nodes) {
    for (auto& node : nodes) {
        if (glm::distance(node.getPosition(), worldPos) < 0.1f) { // Assuming a threshold distance
            return &node;
        }

void UIHandler::drawPreviewBeam(const glm::vec3& start, const glm::vec3& end) {
    // Placeholder for drawing a preview beam
}

void UIHandler::drawRect(int x, int y, int width, int height, const glm::vec3& color) {
    // Placeholder for drawing a rectangle
}

void UIHandler::drawButton(int x, int y, int width, int height, const std::string& label, bool active) {
    // Placeholder for drawing a button
}

void UIHandler::drawSlider(int x, int y, int width, int height, float value, float min, float max) {
    // Placeholder for drawing a slider
}

void UIHandler::drawPreviewBeam(const glm::vec3& start, const glm::vec3& end) {
    // Placeholder for drawing a preview beam
}

void UIHandler::drawRect(int x, int y, int width, int height, const glm::vec3& color) {
    // Placeholder for drawing a rectangle
}

void UIHandler::drawButton(int x, int y, int width, int height, const std::string& label, bool active) {
    // Placeholder for drawing a button
}

void UIHandler::drawSlider(int x, int y, int width, int height, float value, float min, float max) {
    // Placeholder for drawing a slider
}

void UIHandler::drawPreviewBeam(const glm::vec3& start, const glm::vec3& end) {
    // Placeholder for drawing a preview beam
}

void UIHandler::drawRect(int x, int y, int width, int height, const glm::vec3& color) {
    // Placeholder for drawing a rectangle
}

void UIHandler::drawButton(int x, int y, int width, int height, const std::string& label, bool active) {
    // Placeholder for drawing a button
}

void UIHandler::drawSlider(int x, int y, int width, int height, float value, float min, float max) {
    // Placeholder for drawing a slider
}

void UIHandler::drawPreviewBeam(const glm::vec3& start, const glm::vec3& end) {
    // Placeholder for drawing a preview beam
}

void UIHandler::drawRect(int x, int y, int width, int height, const glm::vec3& color) {
    // Placeholder for drawing a rectangle
}

void UIHandler::drawButton(int x, int y, int width, int height, const std::string& label, bool active) {
    // Placeholder for drawing a button
}

void UIHandler::drawSlider(int x, int y, int width, int height, float value, float min, float max) {
    // Placeholder for drawing a slider
}

void UIHandler::drawPreviewBeam(const glm::vec3& start, const glm::vec3& end) {
    // Placeholder for drawing a preview beam
}

void UIHandler::drawRect(int x, int y, int width, int height, const glm::vec3& color) {
    // Placeholder for drawing a rectangle
}

void UIHandler::drawButton(int x, int y, int width, int height, const std::string& label, bool active) {
    // Placeholder for drawing a button
}

void UIHandler::drawSlider(int x, int y, int width, int height, float value, float min, float max) {
    // Placeholder for drawing a slider
}

void UIHandler::drawPreviewBeam(const glm::vec3& start, const glm::vec3& end) {
    // Placeholder for drawing a preview beam
}

void UIHandler::drawRect(int x, int y, int width, int height, const glm::vec3& color) {
    // Placeholder for drawing a rectangle
}

void UIHandler::drawButton(int x, int y, int width, int height, const std::string& label, bool active) {
    // Placeholder for drawing a button
}

void UIHandler::drawSlider(int x, int y, int width, int height, float value, float min, float max) {
    // Placeholder for drawing a slider
}
    }
    return nullptr;
}

void UIHandler::handleEvent(SDL_Event& e, std::vector<Node>& nodes,
                           std::vector<Beam>& beams, glm::vec3 worldPos) {
    // Define currentMouseWorldPos
    currentMouseWorldPos = worldPos; 
    // Define currentMouseWorldPos
    currentMouseWorldPos = worldPos; 
    if (e.type == SDL_MOUSEBUTTONDOWN) {
        switch(currentTool) {
            case ToolMode::NODE_PLACEMENT:
                nodes.emplace_back(worldPos.x, worldPos.y, worldPos.z);
                break;
                
            case ToolMode::BEAM_CREATION:
                if (auto node = findNodeUnderCursor(worldPos, nodes)) {
                    if (!selectedNode) selectedNode = node;
                    else {
                        beams.emplace_back(selectedNode, node, 2e11, 0.01);
                        selectedNode = nullptr;
                    }

void UIHandler::drawPreviewBeam(const glm::vec3& start, const glm::vec3& end) {
    // Placeholder for drawing a preview beam
}

void UIHandler::drawRect(int x, int y, int width, int height, const glm::vec3& color) {
    // Placeholder for drawing a rectangle
}

void UIHandler::drawButton(int x, int y, int width, int height, const std::string& label, bool active) {
    // Placeholder for drawing a button
}

void UIHandler::drawSlider(int x, int y, int width, int height, float value, float min, float max) {
    // Placeholder for drawing a slider
}

void UIHandler::drawPreviewBeam(const glm::vec3& start, const glm::vec3& end) {
    // Placeholder for drawing a preview beam
}

void UIHandler::drawRect(int x, int y, int width, int height, const glm::vec3& color) {
    // Placeholder for drawing a rectangle
}

void UIHandler::drawButton(int x, int y, int width, int height, const std::string& label, bool active) {
    // Placeholder for drawing a button
}

void UIHandler::drawSlider(int x, int y, int width, int height, float value, float min, float max) {
    // Placeholder for drawing a slider
}

void UIHandler::drawPreviewBeam(const glm::vec3& start, const glm::vec3& end) {
    // Placeholder for drawing a preview beam
}

void UIHandler::drawRect(int x, int y, int width, int height, const glm::vec3& color) {
    // Placeholder for drawing a rectangle
}

void UIHandler::drawButton(int x, int y, int width, int height, const std::string& label, bool active) {
    // Placeholder for drawing a button
}

void UIHandler::drawSlider(int x, int y, int width, int height, float value, float min, float max) {
    // Placeholder for drawing a slider
}
                }
                break;
                
            case ToolMode::FORCE_APPLICATION:
                if (auto node = findNodeUnderCursor(worldPos, nodes)) {
                    node->applyForce(forceVector);
                }
                break;
        }
    }
}

void UIHandler::renderUI(SDL_Window* window) {
    // Draw left toolbar
    renderToolbar(window);
    
    // Draw preview elements
    if (currentTool == ToolMode::BEAM_CREATION && selectedNode) {
        drawPreviewBeam(selectedNode->getPosition(), currentMouseWorldPos);
    }
}

void UIHandler::renderToolbar(SDL_Window* window) {
    int w, h;
    SDL_GetWindowSize(window, &w, &h);
    
    // Toolbar background
    drawRect(0, 0, 100, h, {30, 30, 30});
    
    // Tools
    drawButton(10, 10, 80, 40, "Nodes", currentTool == ToolMode::NODE_PLACEMENT);
    drawButton(10, 60, 80, 40, "Beams", currentTool == ToolMode::BEAM_CREATION);
    drawButton(10, 110, 80, 40, "Forces", currentTool == ToolMode::FORCE_APPLICATION);
    
    // Force magnitude slider
    if(currentTool == ToolMode::FORCE_APPLICATION) {
        drawSlider(10, 160, 80, 20, forceVector.x, -1000, 1000);
    }
}

void UIHandler::drawPreviewBeam(const glm::vec3& start, const glm::vec3& end) {
    // Placeholder for drawing a preview beam
}

void UIHandler::drawRect(int x, int y, int width, int height, const glm::vec3& color) {
    // Placeholder for drawing a rectangle
}

void UIHandler::drawButton(int x, int y, int width, int height, const std::string& label, bool active) {
    // Placeholder for drawing a button
}

void UIHandler::drawSlider(int x, int y, int width, int height, float value, float min, float max) {
    // Placeholder for drawing a slider
}
