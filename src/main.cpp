#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <cmath>

#include "../include/model/Node.hpp"
#include "../include/model/Beam.hpp"
#include "../include/physics/Simulator.hpp"
#include "../include/data/CSVHandler.hpp"
#include "../include/visualization/ForceRenderer.hpp"
#include "../include/ui/UIHandler.hpp"
#include "../include/graphics/Shader.hpp"
#include "../include/graphics/Camera.hpp"

// main.cpp : application entry point; orchestrates init, render loop, and shutdown.

static const int WIN_W = 1024;
static const int WIN_H = 768;

// Geometry shaders for nodes and beams.
static const char* GEO_VERT = R"glsl(
#version 330 core
layout(location = 0) in vec3 aPos;
uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
void main() {
    gl_Position = uProjection * uView * uModel * vec4(aPos, 1.0);
}
)glsl";

static const char* GEO_FRAG = R"glsl(
#version 330 core
out vec4 FragColor;
uniform vec3 uColor;
void main() { FragColor = vec4(uColor, 1.0); }
)glsl";

// Pure: builds a sphere VAO using a UV-sphere tessellation.
static void buildSphereVAO(GLuint& vao, GLuint& vbo, int& vertexCount) {
    const int stacks = 8, slices = 12;
    std::vector<float> verts;

    for (int i = 0; i < stacks; ++i) {
        float phi1 = static_cast<float>(M_PI) * (-0.5f + static_cast<float>(i)   / stacks);
        float phi2 = static_cast<float>(M_PI) * (-0.5f + static_cast<float>(i+1) / stacks);

        for (int j = 0; j < slices; ++j) {
            float th1 = 2.0f * static_cast<float>(M_PI) * static_cast<float>(j)   / slices;
            float th2 = 2.0f * static_cast<float>(M_PI) * static_cast<float>(j+1) / slices;

            auto push = [&](float th, float ph) {
                verts.push_back(std::cos(ph) * std::sin(th));
                verts.push_back(std::sin(ph));
                verts.push_back(std::cos(ph) * std::cos(th));
            };
            push(th1, phi1); push(th2, phi1); push(th2, phi2);
            push(th1, phi1); push(th2, phi2); push(th1, phi2);
        }
    }

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
    vertexCount = static_cast<int>(verts.size()) / 3;
}

// Pure: builds a 12-segment cylinder VAO (unit radius, 0→1 along Y).
static void buildCylinderVAO(GLuint& vao, GLuint& vbo, int& vertexCount) {
    const int segs = 12;
    std::vector<float> verts;

    for (int i = 0; i < segs; ++i) {
        float a1 = 2.0f * static_cast<float>(M_PI) * i / segs;
        float a2 = 2.0f * static_cast<float>(M_PI) * (i+1) / segs;
        float x1 = std::cos(a1), z1 = std::sin(a1);
        float x2 = std::cos(a2), z2 = std::sin(a2);

        // Side faces.
        auto push6 = [&](float ax, float az, float bx, float bz, float y0, float y1) {
            verts.insert(verts.end(), {ax,y0,az, bx,y0,bz, bx,y1,bz,
                                       ax,y0,az, bx,y1,bz, ax,y1,az});
        };
        push6(x1,z1,x2,z2, 0.0f, 1.0f);
        // Caps.
        verts.insert(verts.end(), {0.0f,0.0f,0.0f, x1,0.0f,z1, x2,0.0f,z2});
        verts.insert(verts.end(), {0.0f,1.0f,0.0f, x2,1.0f,z2, x1,1.0f,z1});
    }

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
    vertexCount = static_cast<int>(verts.size()) / 3;
}

// Output: draws a sphere at world position with given radius and color.
static void drawNode(const Shader& shader,
                     GLuint vao, int vCount,
                     const glm::vec3& pos, float radius,
                     const glm::vec3& color,
                     const glm::mat4& view, const glm::mat4& proj) {
    glm::mat4 model = glm::translate(glm::mat4(1.0f), pos);
    model = glm::scale(model, glm::vec3(radius));
    shader.setMat4("uModel", model);
    shader.setMat4("uView",  view);
    shader.setMat4("uProjection", proj);
    shader.setVec3("uColor", color);
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, vCount);
    glBindVertexArray(0);
}

// Output: draws a cylinder stretching between two world positions.
static void drawBeam(const Shader& shader,
                     GLuint vao, int vCount,
                     const glm::vec3& start, const glm::vec3& end,
                     float radius, const glm::vec3& color,
                     const glm::mat4& view, const glm::mat4& proj) {
    glm::vec3 dir = end - start;
    float len = glm::length(dir);
    if (len < 1e-4f) return;

    glm::vec3 up = (std::abs(dir.y / len) < 0.99f)
                   ? glm::vec3(0,1,0) : glm::vec3(1,0,0);
    glm::vec3 zAxis = glm::normalize(dir);
    glm::vec3 xAxis = glm::normalize(glm::cross(up, zAxis));
    glm::vec3 yAxis = glm::cross(zAxis, xAxis);

    // Build a model matrix: align Y-axis of unit cylinder to the beam direction.
    glm::mat4 rot(1.0f);
    rot[0] = glm::vec4(xAxis * radius, 0.0f);
    rot[1] = glm::vec4(zAxis * len,    0.0f);
    rot[2] = glm::vec4(yAxis * radius, 0.0f);
    glm::mat4 model = glm::translate(glm::mat4(1.0f), start) * rot;

    shader.setMat4("uModel", model);
    shader.setMat4("uView",  view);
    shader.setMat4("uProjection", proj);
    shader.setVec3("uColor", color);
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, vCount);
    glBindVertexArray(0);
}

// Output: renders a grid on the XZ plane for spatial reference.
static void drawGrid(const Shader& shader, GLuint vao, GLuint vbo,
                     const glm::mat4& view, const glm::mat4& proj) {
    const int half = 10;
    std::vector<float> lines;
    for (int i = -half; i <= half; ++i) {
        float f = static_cast<float>(i);
        lines.insert(lines.end(), {f, 0.0f, -half,  f, 0.0f,  half});
        lines.insert(lines.end(), {-half, 0.0f, f,   half, 0.0f, f});
    }
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, lines.size() * sizeof(float), lines.data(), GL_DYNAMIC_DRAW);

    shader.setMat4("uModel", glm::mat4(1.0f));
    shader.setMat4("uView",  view);
    shader.setMat4("uProjection", proj);
    shader.setVec3("uColor", {0.30f, 0.30f, 0.30f});

    glBindVertexArray(vao);
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(lines.size() / 3));
    glBindVertexArray(0);
}

// Pure: sets up a test structure (cantilever with axial force).
static void loadTestStructure(std::vector<Node>& nodes, std::vector<Beam>& beams) {
    nodes.emplace_back(0.0f, 0.0f, 0.0f);
    nodes.back().setFixed(true);
    nodes.emplace_back(2.0f, 0.0f, 0.0f);
    nodes.emplace_back(4.0f, 0.0f, 0.0f);
    beams.emplace_back(&nodes[0], &nodes[1], 2e11f, 0.01f);
    beams.emplace_back(&nodes[1], &nodes[2], 2e11f, 0.01f);
    nodes[2].applyForce(glm::vec3(0.0f, -5000.0f, 0.0f));
}

int main() {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL_Init error: " << SDL_GetError() << std::endl;
        return 1;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    SDL_Window* window = SDL_CreateWindow(
        "C_Structures — Structural Analysis",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WIN_W, WIN_H,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
    );
    if (!window) {
        std::cerr << "Window creation failed: " << SDL_GetError() << std::endl;
        return 1;
    }

    SDL_GLContext glCtx = SDL_GL_CreateContext(window);
    SDL_GL_SetSwapInterval(1); // vsync

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "GLEW init failed" << std::endl;
        return 1;
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LINE_SMOOTH);

    // Geometry shader program.
    Shader geoShader;
    geoShader.loadFromSource(GEO_VERT, GEO_FRAG);

    // Pre-build geometry meshes.
    GLuint sphereVAO, sphereVBO;
    int sphereVCount = 0;
    buildSphereVAO(sphereVAO, sphereVBO, sphereVCount);

    GLuint cylVAO, cylVBO;
    int cylVCount = 0;
    buildCylinderVAO(cylVAO, cylVBO, cylVCount);

    // Grid buffers.
    GLuint gridVAO, gridVBO;
    glGenVertexArrays(1, &gridVAO);
    glGenBuffers(1, &gridVBO);
    glBindVertexArray(gridVAO);
    glBindBuffer(GL_ARRAY_BUFFER, gridVBO);
    glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    // Scene data.
    std::vector<Node> nodes;
    std::vector<Beam> beams;
    loadTestStructure(nodes, beams);

    // Physics and rendering systems.
    Simulator physics(nodes, beams);
    physics.solveStaticForces();

    ForceRenderer forceRenderer;
    forceRenderer.initialize();

    Camera camera;

    UIHandler ui;
    ui.initialize(WIN_W, WIN_H);

    bool running = true;
    bool mouseDown = false;
    int lastMX = 0, lastMY = 0;
    bool needsSolve = false;

    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) { running = false; break; }

            if (e.type == SDL_WINDOWEVENT &&
                e.window.event == SDL_WINDOWEVENT_RESIZED) {
                ui.initialize(e.window.data1, e.window.data2);
            }

            // Camera orbit: right-mouse drag.
            if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_RIGHT) {
                mouseDown = true;
                lastMX = e.button.x;
                lastMY = e.button.y;
            }
            if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_RIGHT)
                mouseDown = false;
            if (e.type == SDL_MOUSEMOTION && mouseDown)
                camera.handleMouseDrag(e.motion.xrel, e.motion.yrel);
            if (e.type == SDL_MOUSEWHEEL)
                camera.handleScroll(e.wheel.y);

            // Keyboard shortcuts.
            if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)
                running = false;
            if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_RETURN) {
                needsSolve = true;
            }

            int w, h;
            SDL_GetWindowSize(window, &w, &h);
            float aspect = (h > 0) ? static_cast<float>(w) / h : 1.0f;
            glm::mat4 view = camera.getViewMatrix();
            glm::mat4 proj = camera.getProjectionMatrix(aspect);

            ui.handleEvent(e, nodes, beams, view, proj);
        }

        if (needsSolve) {
            physics = Simulator(nodes, beams);
            physics.solveStaticForces();
            needsSolve = false;
        }

        int w, h;
        SDL_GetWindowSize(window, &w, &h);
        glViewport(0, 0, w, h);

        glClearColor(0.10f, 0.10f, 0.13f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        float aspect = (h > 0) ? static_cast<float>(w) / h : 1.0f;
        glm::mat4 view = camera.getViewMatrix();
        glm::mat4 proj = camera.getProjectionMatrix(aspect);

        geoShader.use();

        // Draw reference grid.
        drawGrid(geoShader, gridVAO, gridVBO, view, proj);

        // Draw beams coloured by axial force.
        auto displacements = physics.getNodeDisplacements();
        for (const auto& beam : beams) {
            int si = static_cast<int>(beam.getStart() - &nodes[0]);
            int ei = static_cast<int>(beam.getEnd()   - &nodes[0]);
            glm::vec3 startPos = beam.getStart()->getPosition() + displacements[si];
            glm::vec3 endPos   = beam.getEnd()->getPosition()   + displacements[ei];

            float force = physics.getBeamForce(beam);
            glm::vec3 color = ForceRenderer::getBeamColor(force, ForceRenderer::MAX_STRESS);
            drawBeam(geoShader, cylVAO, cylVCount,
                     startPos, endPos, 0.04f, color, view, proj);
        }

        // Draw nodes: red=fixed, white=free.
        for (int i = 0; i < static_cast<int>(nodes.size()); ++i) {
            glm::vec3 pos = nodes[i].getPosition() + displacements[i];
            glm::vec3 color = nodes[i].isFixed()
                              ? glm::vec3(0.9f, 0.25f, 0.25f)
                              : glm::vec3(0.9f, 0.9f, 0.9f);
            drawNode(geoShader, sphereVAO, sphereVCount,
                     pos, 0.08f, color, view, proj);
        }

        // Draw applied force arrows.
        forceRenderer.renderForceVectors(nodes, view, proj);

        // 2D overlay.
        ui.renderUI(window);

        SDL_GL_SwapWindow(window);
    }

    // Cleanup.
    glDeleteVertexArrays(1, &sphereVAO); glDeleteBuffers(1, &sphereVBO);
    glDeleteVertexArrays(1, &cylVAO);    glDeleteBuffers(1, &cylVBO);
    glDeleteVertexArrays(1, &gridVAO);   glDeleteBuffers(1, &gridVBO);

    SDL_GL_DeleteContext(glCtx);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
