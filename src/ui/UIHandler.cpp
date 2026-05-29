#include "ui/UIHandler.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <cmath>

// ui/UIHandler.cpp : 2D toolbar overlay and user interaction handling.

// Orthographic vertex shader — maps screen pixels to NDC.
static const char* UI_VERT = R"glsl(
#version 330 core
layout(location = 0) in vec2 aPos;
uniform mat4 uOrtho;
void main() { gl_Position = uOrtho * vec4(aPos, 0.0, 1.0); }
)glsl";

// Simple flat-color fragment shader with alpha.
static const char* UI_FRAG = R"glsl(
#version 330 core
out vec4 FragColor;
uniform vec3 uColor;
uniform float uAlpha;
void main() { FragColor = vec4(uColor, uAlpha); }
)glsl";

static GLuint compileUIShader(GLenum type, const char* src) {
    GLuint id = glCreateShader(type);
    glShaderSource(id, 1, &src, nullptr);
    glCompileShader(id);
    return id;
}

UIHandler::~UIHandler() {
    if (rectVAO) { glDeleteVertexArrays(1, &rectVAO); glDeleteBuffers(1, &rectVBO); }
    if (lineVAO) { glDeleteVertexArrays(1, &lineVAO); glDeleteBuffers(1, &lineVBO); }
    if (uiShader) glDeleteProgram(uiShader);
}

// Initializes the 2D shader and geometry buffers.
void UIHandler::initialize(int w, int h) {
    screenWidth  = w;
    screenHeight = h;

    GLuint vert = compileUIShader(GL_VERTEX_SHADER, UI_VERT);
    GLuint frag = compileUIShader(GL_FRAGMENT_SHADER, UI_FRAG);
    uiShader = glCreateProgram();
    glAttachShader(uiShader, vert);
    glAttachShader(uiShader, frag);
    glLinkProgram(uiShader);
    glDeleteShader(vert);
    glDeleteShader(frag);

    // Rect VAO: two triangles forming a quad, positions updated each draw.
    glGenVertexArrays(1, &rectVAO);
    glGenBuffers(1, &rectVBO);
    glBindVertexArray(rectVAO);
    glBindBuffer(GL_ARRAY_BUFFER, rectVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 12, nullptr, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    // Line VAO: two vertices for a line segment.
    glGenVertexArrays(1, &lineVAO);
    glGenBuffers(1, &lineVBO);
    glBindVertexArray(lineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 4, nullptr, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
}

// Pure: returns node within snap threshold or nullptr.
Node* UIHandler::findNodeUnderCursor(const glm::vec3& worldPos, std::vector<Node>& nodes) {
    for (auto& node : nodes) {
        if (glm::distance(node.getPosition(), worldPos) < 0.25f)
            return &node;
    }
    return nullptr;
}

// Pure: unprojects mouse pixel → ray → intersects XZ plane (y=0).
glm::vec3 UIHandler::screenToWorld(int mx, int my,
                                   const glm::mat4& view,
                                   const glm::mat4& projection) const {
    float ndcX =  (2.0f * mx) / screenWidth  - 1.0f;
    float ndcY = -(2.0f * my) / screenHeight + 1.0f;

    glm::vec4 rayClip(ndcX, ndcY, -1.0f, 1.0f);
    glm::vec4 rayEye = glm::inverse(projection) * rayClip;
    rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);
    glm::vec3 rayWorld = glm::normalize(glm::vec3(glm::inverse(view) * rayEye));

    glm::vec3 camPos = glm::vec3(glm::inverse(view)[3]);
    // Intersect with y=0 plane.
    float t = (camPos.y != 0.0f) ? (-camPos.y / rayWorld.y) : 0.0f;
    return camPos + t * rayWorld;
}

void UIHandler::handleEvent(SDL_Event& e, std::vector<Node>& nodes,
                            std::vector<Beam>& beams,
                            const glm::mat4& view, const glm::mat4& projection) {
    if (e.type == SDL_MOUSEMOTION) {
        // Skip events over the toolbar region.
        if (e.motion.x > 110)
            currentMouseWorldPos = screenToWorld(e.motion.x, e.motion.y, view, projection);
    }

    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
        int mx = e.button.x;
        int my = e.button.y;

        // Toolbar button hits.
        if (mx < 110) {
            if (my >= 10 && my <= 50)  { currentTool = ToolMode::NODE_PLACEMENT;  selectedNode = nullptr; return; }
            if (my >= 60 && my <= 100) { currentTool = ToolMode::BEAM_CREATION;   selectedNode = nullptr; return; }
            if (my >= 110 && my <= 150){ currentTool = ToolMode::FORCE_APPLICATION; return; }
            return;
        }

        glm::vec3 worldPos = screenToWorld(mx, my, view, projection);

        switch (currentTool) {
            case ToolMode::NODE_PLACEMENT:
                nodes.emplace_back(worldPos.x, worldPos.y, worldPos.z);
                break;

            case ToolMode::BEAM_CREATION:
                if (Node* hit = findNodeUnderCursor(worldPos, nodes)) {
                    if (!selectedNode) {
                        selectedNode = hit;
                    } else if (hit != selectedNode) {
                        beams.emplace_back(selectedNode, hit, 2e11f, 0.01f);
                        selectedNode = nullptr;
                    }
                }
                break;

            case ToolMode::FORCE_APPLICATION:
                if (Node* hit = findNodeUnderCursor(worldPos, nodes))
                    hit->applyForce(forceVector);
                break;
        }
    }

    // Right-click cancels beam selection.
    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_RIGHT)
        selectedNode = nullptr;

    // Key shortcuts: N = nodes, B = beams, F = forces, Esc = cancel.
    if (e.type == SDL_KEYDOWN) {
        switch (e.key.keysym.sym) {
            case SDLK_n: currentTool = ToolMode::NODE_PLACEMENT;  selectedNode = nullptr; break;
            case SDLK_b: currentTool = ToolMode::BEAM_CREATION;   selectedNode = nullptr; break;
            case SDLK_f: currentTool = ToolMode::FORCE_APPLICATION; break;
            case SDLK_ESCAPE: selectedNode = nullptr; break;
            default: break;
        }
    }
}

// Output: submits a dynamic quad to the GPU and draws it.
void UIHandler::submitRect(float x, float y, float w, float h,
                           const glm::vec3& color, float alpha) {
    float verts[12] = {
        x,     y,
        x + w, y,
        x + w, y + h,
        x,     y,
        x + w, y + h,
        x,     y + h
    };
    glBindBuffer(GL_ARRAY_BUFFER, rectVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);

    glUniform3fv(glGetUniformLocation(uiShader, "uColor"), 1, glm::value_ptr(color));
    glUniform1f(glGetUniformLocation(uiShader, "uAlpha"), alpha);

    glBindVertexArray(rectVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

void UIHandler::drawRect(float x, float y, float w, float h,
                         const glm::vec3& color, float alpha) {
    submitRect(x, y, w, h, color, alpha);
}

// Output: button with highlight when active.
void UIHandler::drawButton(float x, float y, float w, float h, bool active) {
    glm::vec3 bg  = active ? glm::vec3(0.25f, 0.55f, 0.90f) : glm::vec3(0.22f, 0.22f, 0.22f);
    glm::vec3 brd = active ? glm::vec3(0.40f, 0.70f, 1.00f) : glm::vec3(0.40f, 0.40f, 0.40f);
    submitRect(x, y, w, h, bg, 1.0f);
    // 1px border via four thin rects.
    submitRect(x, y,         w, 1.0f, brd, 1.0f);
    submitRect(x, y + h - 1, w, 1.0f, brd, 1.0f);
    submitRect(x, y,         1.0f, h, brd, 1.0f);
    submitRect(x + w - 1, y, 1.0f, h, brd, 1.0f);
}

// Output: track + filled knob for a float value in [minVal, maxVal].
void UIHandler::drawSlider(float x, float y, float w, float h,
                           float val, float minVal, float maxVal) {
    submitRect(x, y, w, h, {0.15f, 0.15f, 0.15f}, 1.0f);
    float t = (val - minVal) / (maxVal - minVal);
    t = std::clamp(t, 0.0f, 1.0f);
    submitRect(x, y, w * t, h, {0.25f, 0.55f, 0.90f}, 1.0f);
}

// Output: screen-space line for beam preview.
void UIHandler::drawPreviewBeam(const glm::vec3& startWorld, const glm::vec3& endWorld,
                                const glm::mat4& view, const glm::mat4& projection) {
    auto project = [&](glm::vec3 p) -> glm::vec2 {
        glm::vec4 clip = projection * view * glm::vec4(p, 1.0f);
        glm::vec3 ndc  = glm::vec3(clip) / clip.w;
        return { (ndc.x + 1.0f) * 0.5f * screenWidth,
                 (1.0f - ndc.y) * 0.5f * screenHeight };
    };
    glm::vec2 s = project(startWorld);
    glm::vec2 e = project(endWorld);
    float verts[4] = { s.x, s.y, e.x, e.y };

    glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);

    glUniform3f(glGetUniformLocation(uiShader, "uColor"), 1.0f, 0.8f, 0.0f);
    glUniform1f(glGetUniformLocation(uiShader, "uAlpha"), 0.8f);

    glBindVertexArray(lineVAO);
    glDrawArrays(GL_LINES, 0, 2);
    glBindVertexArray(0);
}

void UIHandler::renderToolbar(SDL_Window* window) {
    int w, h;
    SDL_GetWindowSize(window, &w, &h);

    // Semi-transparent dark background panel.
    drawRect(0.0f, 0.0f, 110.0f, static_cast<float>(h), {0.12f, 0.12f, 0.12f}, 0.85f);

    drawButton(10.0f,  10.0f, 90.0f, 40.0f, currentTool == ToolMode::NODE_PLACEMENT);
    drawButton(10.0f,  60.0f, 90.0f, 40.0f, currentTool == ToolMode::BEAM_CREATION);
    drawButton(10.0f, 110.0f, 90.0f, 40.0f, currentTool == ToolMode::FORCE_APPLICATION);

    if (currentTool == ToolMode::FORCE_APPLICATION)
        drawSlider(10.0f, 160.0f, 90.0f, 18.0f, forceVector.x, -5000.0f, 5000.0f);
}

void UIHandler::renderUI(SDL_Window* window) {
    if (!uiShader) return;

    int w, h;
    SDL_GetWindowSize(window, &w, &h);

    glm::mat4 ortho = glm::ortho(0.0f, static_cast<float>(w),
                                 static_cast<float>(h), 0.0f,
                                 -1.0f, 1.0f);
    glUseProgram(uiShader);
    glUniformMatrix4fv(glGetUniformLocation(uiShader, "uOrtho"), 1, GL_FALSE,
                       glm::value_ptr(ortho));

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    renderToolbar(window);

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}
