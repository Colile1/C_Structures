#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_opengl3.h>
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

static const int WIN_W = 1280;
static const int WIN_H = 800;

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

static void buildCylinderVAO(GLuint& vao, GLuint& vbo, int& vertexCount) {
    const int segs = 12;
    std::vector<float> verts;
    for (int i = 0; i < segs; ++i) {
        float a1 = 2.0f * static_cast<float>(M_PI) * i / segs;
        float a2 = 2.0f * static_cast<float>(M_PI) * (i+1) / segs;
        float x1 = std::cos(a1), z1 = std::sin(a1);
        float x2 = std::cos(a2), z2 = std::sin(a2);
        verts.insert(verts.end(), {x1,0,z1, x2,0,z2, x2,1,z2,
                                   x1,0,z1, x2,1,z2, x1,1,z1});
        verts.insert(verts.end(), {0,0,0, x1,0,z1, x2,0,z2});
        verts.insert(verts.end(), {0,1,0, x2,1,z2, x1,1,z1});
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

static void drawNode(const Shader& sh, GLuint vao, int vc,
                     const glm::vec3& pos, float r, const glm::vec3& col,
                     const glm::mat4& view, const glm::mat4& proj) {
    glm::mat4 m = glm::scale(glm::translate(glm::mat4(1.0f), pos), glm::vec3(r));
    sh.setMat4("uModel", m);
    sh.setMat4("uView",  view);
    sh.setMat4("uProjection", proj);
    sh.setVec3("uColor", col);
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, vc);
    glBindVertexArray(0);
}

static void drawBeam(const Shader& sh, GLuint vao, int vc,
                     const glm::vec3& start, const glm::vec3& end,
                     float r, const glm::vec3& col,
                     const glm::mat4& view, const glm::mat4& proj) {
    glm::vec3 dir = end - start;
    float len = glm::length(dir);
    if (len < 1e-4f) return;

    glm::vec3 up = (std::abs(dir.y / len) < 0.99f) ? glm::vec3(0,1,0) : glm::vec3(1,0,0);
    glm::vec3 z  = glm::normalize(dir);
    glm::vec3 x  = glm::normalize(glm::cross(up, z));
    glm::vec3 y  = glm::cross(z, x);

    glm::mat4 rot(1.0f);
    rot[0] = glm::vec4(x * r,   0.0f);
    rot[1] = glm::vec4(z * len, 0.0f);
    rot[2] = glm::vec4(y * r,   0.0f);
    glm::mat4 m  = glm::translate(glm::mat4(1.0f), start) * rot;

    sh.setMat4("uModel", m);
    sh.setMat4("uView",  view);
    sh.setMat4("uProjection", proj);
    sh.setVec3("uColor", col);
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, vc);
    glBindVertexArray(0);
}

static void drawGrid(const Shader& sh, GLuint vao, GLuint vbo,
                     const glm::mat4& view, const glm::mat4& proj) {
    const int half = 10;
    std::vector<float> lines;
    for (int i = -half; i <= half; ++i) {
        float f = static_cast<float>(i);
        lines.insert(lines.end(), {f,0,-half, f,0,half});
        lines.insert(lines.end(), {-half,0,f, half,0,f});
    }
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, lines.size() * sizeof(float), lines.data(), GL_DYNAMIC_DRAW);
    sh.setMat4("uModel", glm::mat4(1.0f));
    sh.setMat4("uView",  view);
    sh.setMat4("uProjection", proj);
    sh.setVec3("uColor", {0.28f, 0.28f, 0.28f});
    glBindVertexArray(vao);
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(lines.size() / 3));
    glBindVertexArray(0);
}

// Triangle truss: two fixed base nodes, free apex under vertical load.
// Diagonal beams provide Y-stiffness so the -Y force produces visible deflection.
// Triangle truss: two fixed base nodes, free apex under vertical load.
static void loadTestStructure(std::vector<Node>& nodes, std::vector<Beam>& beams) {
    nodes.emplace_back(-2.0f, 0.0f, 0.0f);
    nodes.back().setJointType(JointType::FIXED);
    nodes.emplace_back( 2.0f, 0.0f, 0.0f);
    nodes.back().setJointType(JointType::FIXED);
    nodes.emplace_back( 0.0f, 3.0f, 0.0f); // free apex
    beams.emplace_back(&nodes[0], &nodes[2], 2e11f, 1e-4f);
    beams.emplace_back(&nodes[1], &nodes[2], 2e11f, 1e-4f);
    nodes[2].applyForce(glm::vec3(0.0f, -50000.0f, 0.0f));
}

int main() {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL_Init: " << SDL_GetError() << "\n"; return 1;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    SDL_Window* window = SDL_CreateWindow(
        "C_Structures — Structural Analysis",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WIN_W, WIN_H,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!window) {
        std::cerr << "Window: " << SDL_GetError() << "\n"; return 1;
    }

    SDL_GLContext glCtx = SDL_GL_CreateContext(window);
    if (!glCtx) {
        std::cerr << "GL context: " << SDL_GetError() << "\n"
                  << "WSL users: ensure a display server (WSLg or VcXsrv) is running.\n";
        SDL_DestroyWindow(window); SDL_Quit(); return 1;
    }
    SDL_GL_SetSwapInterval(1);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "GLEW init failed\n"; return 1;
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LINE_SMOOTH);

    // ── ImGui ──────────────────────────────────────────────────────────────────
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = nullptr; // no imgui.ini file

    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding    = 4.0f;
    style.FrameRounding     = 3.0f;
    style.GrabRounding      = 3.0f;
    style.WindowBorderSize  = 0.0f;
    style.Colors[ImGuiCol_WindowBg]      = ImVec4(0.10f, 0.10f, 0.13f, 0.90f);
    style.Colors[ImGuiCol_Header]        = ImVec4(0.22f, 0.52f, 0.88f, 0.80f);
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.28f, 0.62f, 1.00f, 0.80f);

    ImGui_ImplSDL2_InitForOpenGL(window, glCtx);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    // ── Geometry ───────────────────────────────────────────────────────────────
    Shader geoShader;
    geoShader.loadFromSource(GEO_VERT, GEO_FRAG);

    GLuint sphereVAO, sphereVBO; int sphereVC = 0;
    buildSphereVAO(sphereVAO, sphereVBO, sphereVC);

    GLuint cylVAO, cylVBO; int cylVC = 0;
    buildCylinderVAO(cylVAO, cylVBO, cylVC);

    GLuint gridVAO, gridVBO;
    glGenVertexArrays(1, &gridVAO); glGenBuffers(1, &gridVBO);
    glBindVertexArray(gridVAO);
    glBindBuffer(GL_ARRAY_BUFFER, gridVBO);
    glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    // ── Scene ──────────────────────────────────────────────────────────────────
    std::vector<Node> nodes;
    std::vector<Beam> beams;
    nodes.reserve(256); // prevent pointer invalidation from reallocation
    loadTestStructure(nodes, beams);

    Simulator physics(nodes, beams);
    physics.solveStaticForces();

    ForceRenderer forceRenderer;
    forceRenderer.initialize();

    Camera   camera;
    UIHandler ui;
    ui.initialize(WIN_W, WIN_H);

    float dispScale = 50.0f; // multiply displacements for visibility
    bool running   = true;
    bool rmb       = false;

    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            ImGui_ImplSDL2_ProcessEvent(&e);

            if (e.type == SDL_QUIT) { running = false; break; }

            int w, h;
            SDL_GetWindowSize(window, &w, &h);
            float aspect = (h > 0) ? static_cast<float>(w) / h : 1.0f;
            glm::mat4 view = camera.getViewMatrix();
            glm::mat4 proj = camera.getProjectionMatrix(aspect);

            ui.handleEvent(e, nodes, beams, view, proj);

            // Camera orbit (right-mouse drag) — only when ImGui doesn't want the mouse
            if (!io.WantCaptureMouse) {
                if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_RIGHT)
                    rmb = true;
                if (e.type == SDL_MOUSEBUTTONUP   && e.button.button == SDL_BUTTON_RIGHT)
                    rmb = false;
                if (e.type == SDL_MOUSEMOTION && rmb)
                    camera.handleMouseDrag(e.motion.xrel, e.motion.yrel);
                if (e.type == SDL_MOUSEWHEEL)
                    camera.handleScroll(e.wheel.y);
            }

            if (!io.WantCaptureKeyboard) {
                if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_RETURN) {
                    physics = Simulator(nodes, beams);
                    physics.solveStaticForces();
                }
            }
        }

        if (ui.consumeNeedsSolve()) {
            physics = Simulator(nodes, beams);
            physics.solveStaticForces();
        }

        int w, h;
        SDL_GetWindowSize(window, &w, &h);
        glViewport(0, 0, w, h);
        glClearColor(0.08f, 0.08f, 0.10f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        float aspect = (h > 0) ? static_cast<float>(w) / h : 1.0f;
        glm::mat4 view = camera.getViewMatrix();
        glm::mat4 proj = camera.getProjectionMatrix(aspect);

        geoShader.use();
        drawGrid(geoShader, gridVAO, gridVBO, view, proj);

        auto displacements = physics.getNodeDisplacements();

        for (const auto& beam : beams) {
            int si = static_cast<int>(beam.getStart() - &nodes[0]);
            int ei = static_cast<int>(beam.getEnd()   - &nodes[0]);
            if (si < 0 || si >= (int)displacements.size()) continue;
            if (ei < 0 || ei >= (int)displacements.size()) continue;
            glm::vec3 s = beam.getStart()->getPosition() + displacements[si] * dispScale;
            glm::vec3 ep = beam.getEnd()->getPosition()  + displacements[ei] * dispScale;
            float force = physics.getBeamForce(beam);
            glm::vec3 col = ForceRenderer::getBeamColor(force, ForceRenderer::MAX_STRESS);
            drawBeam(geoShader, cylVAO, cylVC, s, ep, 0.04f, col, view, proj);
        }

        for (int i = 0; i < (int)nodes.size(); ++i) {
            if (i >= (int)displacements.size()) break;
            glm::vec3 pos = nodes[i].getPosition() + displacements[i] * dispScale;
            glm::vec3 col;
            switch (nodes[i].getJointType()) {
                case JointType::FIXED:    col = {0.90f, 0.25f, 0.25f}; break;
                case JointType::PIN_XY:   col = {0.95f, 0.65f, 0.15f}; break;
                case JointType::ROLLER_X: col = {0.25f, 0.75f, 0.95f}; break;
                case JointType::ROLLER_Y: col = {0.25f, 0.90f, 0.55f}; break;
                case JointType::ROLLER_Z: col = {0.75f, 0.40f, 0.95f}; break;
                default:                  col = {0.90f, 0.90f, 0.90f}; break;
            }
            float radius = 0.09f;
            drawNode(geoShader, sphereVAO, sphereVC, pos, radius, col, view, proj);
        }

        forceRenderer.renderForceVectors(nodes, view, proj);

        // ── ImGui frame ────────────────────────────────────────────────────────
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        ui.renderUI(window, nodes, beams, dispScale);

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        SDL_GL_SwapWindow(window);
    }

    // ── Cleanup ────────────────────────────────────────────────────────────────
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    glDeleteVertexArrays(1, &sphereVAO); glDeleteBuffers(1, &sphereVBO);
    glDeleteVertexArrays(1, &cylVAO);    glDeleteBuffers(1, &cylVBO);
    glDeleteVertexArrays(1, &gridVAO);   glDeleteBuffers(1, &gridVBO);

    SDL_GL_DeleteContext(glCtx);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
