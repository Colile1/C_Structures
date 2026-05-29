#include "ui/UIHandler.hpp"
#include <imgui.h>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>
#include <string>

void UIHandler::initialize(int w, int h) {
    screenWidth  = w;
    screenHeight = h;
}

Node* UIHandler::findNodeUnderCursor(const glm::vec3& worldPos, std::vector<Node>& nodes) {
    Node* best = nullptr;
    float bestDist = 0.35f;
    for (auto& n : nodes) {
        float d = glm::distance(n.getPosition(), worldPos);
        if (d < bestDist) { bestDist = d; best = &n; }
    }
    return best;
}

glm::vec3 UIHandler::screenToWorld(int mx, int my,
                                    const glm::mat4& view,
                                    const glm::mat4& proj) const {
    float ndcX =  (2.0f * mx) / screenWidth  - 1.0f;
    float ndcY = -(2.0f * my) / screenHeight + 1.0f;
    glm::vec4 rayClip(ndcX, ndcY, -1.0f, 1.0f);
    glm::vec4 rayEye  = glm::inverse(proj) * rayClip;
    rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);
    glm::vec3 rayDir  = glm::normalize(glm::vec3(glm::inverse(view) * rayEye));
    glm::vec3 camPos  = glm::vec3(glm::inverse(view)[3]);
    float t = (std::abs(rayDir.y) > 1e-6f) ? (-camPos.y / rayDir.y) : 0.0f;
    return camPos + t * rayDir;
}

void UIHandler::handleEvent(SDL_Event& e,
                             std::vector<Node>& nodes,
                             std::vector<Beam>& beams,
                             const glm::mat4& view,
                             const glm::mat4& proj) {
    ImGuiIO& io = ImGui::GetIO();

    if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_RESIZED) {
        screenWidth  = e.window.data1;
        screenHeight = e.window.data2;
    }

    // Let ImGui consume its events first
    if (io.WantCaptureMouse) { draggingNode = false; return; }

    if (e.type == SDL_MOUSEMOTION) {
        currentMouseWorldPos = screenToWorld(e.motion.x, e.motion.y, view, proj);
        if (draggingNode && selectedNode) {
            selectedNode->setPosition(
                {currentMouseWorldPos.x, selectedNode->getPosition().y, currentMouseWorldPos.z});
            needsSolveFlag = true;
        }
    }

    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
        glm::vec3 wp = screenToWorld(e.button.x, e.button.y, view, proj);

        switch (currentTool) {
            case ToolMode::SELECT: {
                Node* hit = findNodeUnderCursor(wp, nodes);
                if (hit) { selectedNode = hit; draggingNode = true; }
                else     { selectedNode = nullptr; }
                break;
            }
            case ToolMode::NODE_PLACEMENT:
                nodes.emplace_back(wp.x, 0.0f, wp.z);
                needsSolveFlag = true;
                break;

            case ToolMode::BEAM_CREATION:
                if (Node* hit = findNodeUnderCursor(wp, nodes)) {
                    if (!beamStart) {
                        beamStart = hit;
                    } else if (hit != beamStart) {
                        beams.emplace_back(beamStart, hit, 2e11f, 0.01f);
                        beamStart = nullptr;
                        needsSolveFlag = true;
                    }
                }
                break;

            case ToolMode::FORCE_APPLICATION:
                if (Node* hit = findNodeUnderCursor(wp, nodes)) {
                    hit->applyForce(forceVector);
                    needsSolveFlag = true;
                }
                break;
        }
    }

    if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT)
        draggingNode = false;

    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_RIGHT) {
        beamStart    = nullptr;
        draggingNode = false;
    }

    if (!io.WantCaptureKeyboard && e.type == SDL_KEYDOWN) {
        switch (e.key.keysym.sym) {
            case SDLK_1: currentTool = ToolMode::SELECT;            break;
            case SDLK_n: currentTool = ToolMode::NODE_PLACEMENT;    break;
            case SDLK_b: currentTool = ToolMode::BEAM_CREATION; beamStart = nullptr; break;
            case SDLK_f: currentTool = ToolMode::FORCE_APPLICATION; break;
            case SDLK_ESCAPE: beamStart = nullptr; selectedNode = nullptr; draggingNode = false; break;
            case SDLK_DELETE:
                if (selectedNode) {
                    beams.erase(
                        std::remove_if(beams.begin(), beams.end(),
                            [&](const Beam& b){
                                return b.getStart() == selectedNode || b.getEnd() == selectedNode;
                            }),
                        beams.end()
                    );
                    nodes.erase(
                        std::find_if(nodes.begin(), nodes.end(),
                            [&](const Node& n){ return &n == selectedNode; }));
                    selectedNode   = nullptr;
                    needsSolveFlag = true;
                }
                break;
            default: break;
        }
    }
}

static const char* toolLabel(ToolMode m) {
    switch (m) {
        case ToolMode::SELECT:            return "Select";
        case ToolMode::NODE_PLACEMENT:    return "Node";
        case ToolMode::BEAM_CREATION:     return "Beam";
        case ToolMode::FORCE_APPLICATION: return "Force";
    }
    return "";
}

void UIHandler::renderUI(SDL_Window* window,
                          std::vector<Node>& nodes,
                          std::vector<Beam>& beams,
                          float& dispScale) {
    int w = 0, h = 0;
    SDL_GetWindowSize(window, &w, &h);
    const float menuH = ImGui::GetFrameHeight();

    // ── Top menu bar ──────────────────────────────────────────────────────────
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New Structure", "Ctrl+N")) {
                nodes.clear();
                beams.clear();
                selectedNode   = nullptr;
                beamStart      = nullptr;
                needsSolveFlag = true;
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit", "Alt+F4")) {
                SDL_Event q; q.type = SDL_QUIT;
                SDL_PushEvent(&q);
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Tools")) {
            if (ImGui::MenuItem("Select",  "1", currentTool == ToolMode::SELECT))
                currentTool = ToolMode::SELECT;
            if (ImGui::MenuItem("Node",    "N", currentTool == ToolMode::NODE_PLACEMENT))
                currentTool = ToolMode::NODE_PLACEMENT;
            if (ImGui::MenuItem("Beam",    "B", currentTool == ToolMode::BEAM_CREATION))
                { currentTool = ToolMode::BEAM_CREATION; beamStart = nullptr; }
            if (ImGui::MenuItem("Force",   "F", currentTool == ToolMode::FORCE_APPLICATION))
                currentTool = ToolMode::FORCE_APPLICATION;
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Simulation")) {
            if (ImGui::MenuItem("Run Solver", "Enter"))
                needsSolveFlag = true;
            ImGui::EndMenu();
        }

        // Mode indicator in menu bar right side
        float rightEdge = ImGui::GetContentRegionAvail().x;
        std::string modeStr = "Mode: ";
        modeStr += toolLabel(currentTool);
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + rightEdge
                             - ImGui::CalcTextSize(modeStr.c_str()).x - 8.0f);
        ImGui::TextDisabled("%s", modeStr.c_str());

        ImGui::EndMainMenuBar();
    }

    // ── Left toolbar ──────────────────────────────────────────────────────────
    const float tbW = 130.0f;
    ImGui::SetNextWindowPos(ImVec2(0, menuH), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(tbW, (float)h - menuH - 24.0f), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.90f);
    ImGui::Begin("##toolbar", nullptr,
                 ImGuiWindowFlags_NoTitleBar   | ImGuiWindowFlags_NoResize |
                 ImGuiWindowFlags_NoMove       | ImGuiWindowFlags_NoScrollbar |
                 ImGuiWindowFlags_NoBringToFrontOnFocus);

    ImGui::TextColored({0.55f, 0.85f, 1.0f, 1.0f}, "TOOLS");
    ImGui::Separator();
    ImGui::Spacing();

    auto toolBtn = [&](ToolMode mode, const char* icon, const char* tip, const char* key) {
        bool active = (currentTool == mode);
        if (active)
            ImGui::PushStyleColor(ImGuiCol_Button,
                                  ImVec4(0.22f, 0.52f, 0.88f, 1.0f));
        char label[32];
        snprintf(label, sizeof(label), "%s  %s", icon, toolLabel(mode));
        if (ImGui::Button(label, ImVec2(112, 34)))
            currentTool = mode;
        if (active) ImGui::PopStyleColor();
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s  [%s]", tip, key);
        ImGui::Spacing();
    };

    toolBtn(ToolMode::SELECT,            "[S]", "Select / Drag nodes", "1");
    toolBtn(ToolMode::NODE_PLACEMENT,    "[+]", "Place a node",        "N");
    toolBtn(ToolMode::BEAM_CREATION,     "[=]", "Connect two nodes",   "B");
    toolBtn(ToolMode::FORCE_APPLICATION, "[v]", "Apply force to node", "F");

    // Beam-creation hint
    if (currentTool == ToolMode::BEAM_CREATION) {
        ImGui::Separator();
        if (beamStart)
            ImGui::TextColored({1.0f, 0.85f, 0.2f, 1.0f}, "Click end node");
        else
            ImGui::TextWrapped("Click start node");
    }

    // Force options
    if (currentTool == ToolMode::FORCE_APPLICATION) {
        ImGui::Separator();
        ImGui::Text("Force (N):");
        bool ch = false;
        ch |= ImGui::DragFloat("Fx", &forceMagX, 50.f, -1e5f, 1e5f, "%.0f");
        ch |= ImGui::DragFloat("Fy", &forceMagY, 50.f, -1e5f, 1e5f, "%.0f");
        ch |= ImGui::DragFloat("Fz", &forceMagZ, 50.f, -1e5f, 1e5f, "%.0f");
        if (ch) forceVector = {forceMagX, forceMagY, forceMagZ};
    }

    ImGui::Separator();
    ImGui::Spacing();
    ImGui::TextColored({0.6f, 0.6f, 0.6f, 1.0f}, "Camera:");
    ImGui::TextWrapped("Right-drag: orbit\nScroll: zoom");
    ImGui::Spacing();
    ImGui::TextColored({0.6f, 0.6f, 0.6f, 1.0f}, "Editing:");
    ImGui::TextWrapped("Enter: re-solve\nDel: delete node");

    ImGui::End();

    // ── Right properties panel ─────────────────────────────────────────────────
    const float propW = 185.0f;
    ImGui::SetNextWindowPos(ImVec2((float)w - propW, menuH), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(propW, (float)h - menuH - 24.0f), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.90f);
    ImGui::Begin("Properties", nullptr,
                 ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                 ImGuiWindowFlags_NoBringToFrontOnFocus);

    ImGui::TextColored({0.55f, 0.85f, 1.0f, 1.0f}, "SCENE");
    ImGui::Separator();
    ImGui::Text("Nodes : %d", (int)nodes.size());
    ImGui::Text("Beams : %d", (int)beams.size());
    ImGui::Spacing();
    ImGui::TextColored({0.55f, 0.85f, 1.0f, 1.0f}, "DISPLAY");
    ImGui::Separator();
    ImGui::SliderFloat("Disp.Scale", &dispScale, 1.0f, 5000.0f, "%.0fx", ImGuiSliderFlags_Logarithmic);

    if (selectedNode) {
        ImGui::Spacing();
        ImGui::TextColored({0.55f, 0.85f, 1.0f, 1.0f}, "SELECTED NODE");
        ImGui::Separator();

        glm::vec3 pos = selectedNode->getPosition();
        float px = pos.x, py = pos.y, pz = pos.z;
        bool posChg = false;
        posChg |= ImGui::DragFloat("X##p", &px, 0.05f, -100.f, 100.f, "%.2f");
        posChg |= ImGui::DragFloat("Y##p", &py, 0.05f, -100.f, 100.f, "%.2f");
        posChg |= ImGui::DragFloat("Z##p", &pz, 0.05f, -100.f, 100.f, "%.2f");
        if (posChg) {
            selectedNode->setPosition({px, py, pz});
            needsSolveFlag = true;
        }

        ImGui::Spacing();
        static const char* jointNames[] = {
            "Free", "Fixed", "Pin XY", "Roller X", "Roller Y", "Roller Z"
        };
        int jt = static_cast<int>(selectedNode->getJointType());
        if (ImGui::Combo("Joint Type", &jt, jointNames, 6)) {
            selectedNode->setJointType(static_cast<JointType>(jt));
            needsSolveFlag = true;
        }

        glm::vec3 f = selectedNode->getAppliedForce();
        ImGui::Text("Force: (%.0f,%.0f,%.0f)", f.x, f.y, f.z);

        if (ImGui::Button("Clear Force", ImVec2(-1, 0))) {
            selectedNode->clearForce();
            needsSolveFlag = true;
        }
        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.65f, 0.18f, 0.18f, 1.0f));
        if (ImGui::Button("Delete Node", ImVec2(-1, 0))) {
            Node* target = selectedNode;
            beams.erase(
                std::remove_if(beams.begin(), beams.end(),
                    [target](const Beam& b){
                        return b.getStart() == target || b.getEnd() == target;
                    }),
                beams.end());
            nodes.erase(
                std::find_if(nodes.begin(), nodes.end(),
                    [target](const Node& n){ return &n == target; }));
            selectedNode   = nullptr;
            needsSolveFlag = true;
        }
        ImGui::PopStyleColor();
    } else {
        ImGui::Spacing();
        ImGui::TextDisabled("No node selected.\nUse Select tool\nand click a node.");
    }

    ImGui::End();

    // ── Bottom status bar ──────────────────────────────────────────────────────
    ImGui::SetNextWindowPos(ImVec2(0, (float)h - 24.0f), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2((float)w, 24.0f), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.90f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6, 3));
    ImGui::Begin("##status", nullptr,
                 ImGuiWindowFlags_NoTitleBar    | ImGuiWindowFlags_NoResize |
                 ImGuiWindowFlags_NoMove        | ImGuiWindowFlags_NoScrollbar |
                 ImGuiWindowFlags_NoBringToFrontOnFocus);
    ImGui::Text("Mode: %-8s  |  World (%.2f, %.2f, %.2f)  |  Nodes: %d  Beams: %d",
                toolLabel(currentTool),
                currentMouseWorldPos.x, currentMouseWorldPos.y, currentMouseWorldPos.z,
                (int)nodes.size(), (int)beams.size());
    ImGui::End();
    ImGui::PopStyleVar();
}
