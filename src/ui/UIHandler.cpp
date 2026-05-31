// Copyright (c) 2026 Colile Sibanda. All rights reserved.
// Proprietary — see LICENSE for terms. Unauthorised use prohibited.
#include "ui/UIHandler.hpp"
#include "visualization/ForceRenderer.hpp"
#include <imgui.h>
#include "IconsFontAwesome6.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>
#include <string>
#include <SDL2/SDL.h>

// ── Helpers ───────────────────────────────────────────────────────────────────

// eraseNodeAt
// Purpose: remove a node and keep beam connectivity valid.
// Inputs:  idx — node index to remove; nodes/beams — the model vectors.
// Output:  none (mutates nodes and beams in place). Beams touching the node
//          are dropped; indices above idx are shifted down to match the
//          compacted node vector.
static void eraseNodeAt(int idx, std::vector<Node>& nodes, std::vector<Beam>& beams) {
    if (idx < 0 || idx >= static_cast<int>(nodes.size())) return;
    beams.erase(std::remove_if(beams.begin(), beams.end(),
        [idx](const Beam& b){
            return b.getStartIdx() == idx || b.getEndIdx() == idx;
        }), beams.end());
    nodes.erase(nodes.begin() + idx);
    for (Beam& b : beams) {
        if (b.getStartIdx() > idx) b.setStartIdx(b.getStartIdx() - 1);
        if (b.getEndIdx()   > idx) b.setEndIdx(b.getEndIdx()   - 1);
    }
}

void UIHandler::initialize(int w, int h) {
    screenWidth  = w;
    screenHeight = h;
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

glm::vec3 UIHandler::worldToScreen(const glm::vec3& w,
                                    const glm::mat4& view,
                                    const glm::mat4& proj) const {
    glm::vec4 clip = proj * view * glm::vec4(w, 1.0f);
    if (std::abs(clip.w) < 1e-6f) return {-9999, -9999, 0};
    glm::vec3 ndc = glm::vec3(clip) / clip.w;
    float sx = (ndc.x + 1.0f) * 0.5f * screenWidth;
    float sy = (1.0f - ndc.y) * 0.5f * screenHeight;
    return {sx, sy, 0.0f};
}

// Returns the index of the nearest node within a world-space pick radius, or -1.
int UIHandler::findNodeUnderCursor(const glm::vec3& worldPos,
                                    const std::vector<Node>& nodes) const {
    int   best     = -1;
    float bestDist = 0.35f;
    for (int i = 0; i < static_cast<int>(nodes.size()); ++i) {
        float d = glm::distance(nodes[i].getPosition(), worldPos);
        if (d < bestDist) { bestDist = d; best = i; }
    }
    return best;
}

// 2-D screen-space proximity test for beam picking. Returns a beam index or -1.
int UIHandler::findBeamUnderCursor(int mx, int my,
                                    const std::vector<Node>& nodes,
                                    const std::vector<Beam>& beams,
                                    const glm::mat4& view,
                                    const glm::mat4& proj) const {
    const float THRESH = 8.0f; // pixels
    int   best  = -1;
    float bestD = THRESH;
    glm::vec2 mp(static_cast<float>(mx), static_cast<float>(my));

    for (int i = 0; i < static_cast<int>(beams.size()); ++i) {
        const Beam& b = beams[i];
        glm::vec3 sa = worldToScreen(nodes[b.getStartIdx()].getPosition(), view, proj);
        glm::vec3 ea = worldToScreen(nodes[b.getEndIdx()].getPosition(),   view, proj);
        glm::vec2 s(sa.x, sa.y), e(ea.x, ea.y);

        glm::vec2 se  = e - s;
        float     len2 = glm::dot(se, se);
        if (len2 < 1.0f) continue;
        float t = glm::clamp(glm::dot(mp - s, se) / len2, 0.0f, 1.0f);
        float d = glm::length(mp - (s + t * se));
        if (d < bestD) { bestD = d; best = i; }
    }
    return best;
}

// ── Undo / redo ───────────────────────────────────────────────────────────────

void UIHandler::pushSnapshot(const std::vector<Node>& nodes,
                              const std::vector<Beam>& beams) {
    SceneSnapshot snap;
    snap.nodes.reserve(nodes.size());
    for (const auto& n : nodes) {
        glm::vec3 f = n.getAppliedForce();
        glm::vec3 p = n.getPosition();
        snap.nodes.push_back({p.x, p.y, p.z, n.getJointType(), f.x, f.y, f.z});
    }
    snap.beams.reserve(beams.size());
    for (const auto& b : beams) {
        snap.beams.push_back({b.getStartIdx(), b.getEndIdx(),
                              b.getYoungsModulus(), b.getCrossSection(),
                              b.getMomentOfInertia(), b.getMaterial()});
    }
    m_undoStack.push_back(std::move(snap));
    if (static_cast<int>(m_undoStack.size()) > MAX_UNDO)
        m_undoStack.pop_front();
    m_redoStack.clear();
}

void UIHandler::applySnapshot(const SceneSnapshot& s,
                               std::vector<Node>& nodes,
                               std::vector<Beam>& beams) {
    // Rebuild nodes then beams. Beams reference nodes by index, so a rebuilt
    // node vector stays consistent without any pointer fix-up.
    selectedNode = -1;
    selectedBeam = -1;
    beamStart    = -1;

    nodes.clear();
    nodes.reserve(s.nodes.size() + 32);
    for (const auto& ns : s.nodes) {
        nodes.emplace_back(ns.x, ns.y, ns.z);
        nodes.back().setJointType(ns.joint);
        nodes.back().applyForce({ns.fx, ns.fy, ns.fz});
    }

    beams.clear();
    for (const auto& bs : s.beams) {
        beams.emplace_back(bs.iStart, bs.iEnd, bs.E, bs.A);
        beams.back().setMomentOfInertia(bs.I);
        beams.back().setMaterial(bs.material);
        // setMaterial overwrites E for non-CUSTOM; restore explicit E.
        if (bs.material == BeamMaterial::CUSTOM)
            beams.back().setYoungsModulus(bs.E);
    }
    needsSolveFlag = true;
}

void UIHandler::undo(std::vector<Node>& nodes, std::vector<Beam>& beams) {
    if (m_undoStack.empty()) return;

    // Push current state onto redo stack.
    SceneSnapshot current;
    current.nodes.reserve(nodes.size());
    for (const auto& n : nodes) {
        glm::vec3 f = n.getAppliedForce();
        glm::vec3 p = n.getPosition();
        current.nodes.push_back({p.x, p.y, p.z, n.getJointType(), f.x, f.y, f.z});
    }
    for (const auto& b : beams) {
        current.beams.push_back({b.getStartIdx(), b.getEndIdx(),
                                 b.getYoungsModulus(), b.getCrossSection(),
                                 b.getMomentOfInertia(), b.getMaterial()});
    }
    m_redoStack.push_back(std::move(current));
    if (static_cast<int>(m_redoStack.size()) > MAX_UNDO)
        m_redoStack.pop_front();

    applySnapshot(m_undoStack.back(), nodes, beams);
    m_undoStack.pop_back();
}

void UIHandler::redo(std::vector<Node>& nodes, std::vector<Beam>& beams) {
    if (m_redoStack.empty()) return;

    SceneSnapshot current;
    current.nodes.reserve(nodes.size());
    for (const auto& n : nodes) {
        glm::vec3 f = n.getAppliedForce();
        glm::vec3 p = n.getPosition();
        current.nodes.push_back({p.x, p.y, p.z, n.getJointType(), f.x, f.y, f.z});
    }
    for (const auto& b : beams) {
        current.beams.push_back({b.getStartIdx(), b.getEndIdx(),
                                 b.getYoungsModulus(), b.getCrossSection(),
                                 b.getMomentOfInertia(), b.getMaterial()});
    }
    m_undoStack.push_back(std::move(current));

    applySnapshot(m_redoStack.back(), nodes, beams);
    m_redoStack.pop_back();
}

// ── Event handling ────────────────────────────────────────────────────────────

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

    if (io.WantCaptureMouse) { draggingNode = false; return; }

    if (e.type == SDL_MOUSEMOTION) {
        currentMouseWorldPos = screenToWorld(e.motion.x, e.motion.y, view, proj);
        if (draggingNode && selectedNode >= 0 &&
            selectedNode < static_cast<int>(nodes.size())) {
            Node& sn = nodes[selectedNode];
            sn.setPosition(
                {currentMouseWorldPos.x, sn.getPosition().y, currentMouseWorldPos.z});
            needsSolveFlag = true;
        }
    }

    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
        glm::vec3 wp = screenToWorld(e.button.x, e.button.y, view, proj);

        switch (currentTool) {
            case ToolMode::SELECT: {
                int hitNode = findNodeUnderCursor(wp, nodes);
                if (hitNode >= 0) {
                    selectedNode = hitNode;
                    selectedBeam = -1;
                    draggingNode = true;
                } else {
                    selectedBeam = findBeamUnderCursor(e.button.x, e.button.y,
                                                       nodes, beams, view, proj);
                    selectedNode = -1;
                }
                break;
            }
            case ToolMode::NODE_PLACEMENT:
                pushSnapshot(nodes, beams);
                nodes.emplace_back(wp.x, 0.0f, wp.z);
                needsSolveFlag = true;
                break;

            case ToolMode::BEAM_CREATION: {
                int hit = findNodeUnderCursor(wp, nodes);
                if (hit >= 0) {
                    if (beamStart < 0) {
                        beamStart = hit;
                    } else if (hit != beamStart) {
                        pushSnapshot(nodes, beams);
                        beams.emplace_back(beamStart, hit,
                                           BeamMaterial::STEEL, 1e-4f);
                        beamStart      = -1;
                        needsSolveFlag = true;
                    }
                }
                break;
            }
            case ToolMode::FORCE_APPLICATION: {
                int hit = findNodeUnderCursor(wp, nodes);
                if (hit >= 0) {
                    pushSnapshot(nodes, beams);
                    nodes[hit].applyForce(forceVector);
                    needsSolveFlag = true;
                }
                break;
            }
        }
    }

    if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT) {
        if (draggingNode) needsSolveFlag = true;
        draggingNode = false;
    }

    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_RIGHT) {
        beamStart    = -1;
        draggingNode = false;
    }

    if (!io.WantCaptureKeyboard && e.type == SDL_KEYDOWN) {
        const bool ctrl  = (e.key.keysym.mod & KMOD_CTRL)  != 0;
        const bool shift = (e.key.keysym.mod & KMOD_SHIFT) != 0;

        switch (e.key.keysym.sym) {
            // ── Undo / Redo ──────────────────────────────────────────────────
            case SDLK_z:
                if (ctrl && !shift) undo(nodes, beams);
                if (ctrl &&  shift) redo(nodes, beams);
                break;
            case SDLK_y:
                if (ctrl) redo(nodes, beams);
                break;

            // ── Tool selection ───────────────────────────────────────────────
            case SDLK_1: currentTool = ToolMode::SELECT;            break;
            case SDLK_n:
                if (ctrl) {
                    pushSnapshot(nodes, beams);
                    nodes.clear(); beams.clear();
                    selectedNode = -1; selectedBeam = -1;
                    beamStart    = -1;
                    needsSolveFlag = true;
                } else {
                    currentTool = ToolMode::NODE_PLACEMENT;
                }
                break;
            case SDLK_b: currentTool = ToolMode::BEAM_CREATION; beamStart = -1; break;
            case SDLK_f:
                if (ctrl) m_wantScreenshot = true;          // Ctrl+F → screenshot (F12 also works)
                else      currentTool = ToolMode::FORCE_APPLICATION;
                break;
            case SDLK_F12: m_wantScreenshot = true; break;
            case SDLK_o: if (ctrl) m_showOpenDlg = true; break;
            case SDLK_s: if (ctrl) m_showSaveDlg = true; break;

            // ── Delete selected ──────────────────────────────────────────────
            case SDLK_DELETE:
                if (selectedNode >= 0 && selectedNode < static_cast<int>(nodes.size())) {
                    pushSnapshot(nodes, beams);
                    eraseNodeAt(selectedNode, nodes, beams);
                    selectedNode   = -1;
                    needsSolveFlag = true;
                } else if (selectedBeam >= 0 &&
                           selectedBeam < static_cast<int>(beams.size())) {
                    pushSnapshot(nodes, beams);
                    beams.erase(beams.begin() + selectedBeam);
                    selectedBeam   = -1;
                    needsSolveFlag = true;
                }
                break;

            case SDLK_ESCAPE:
                beamStart = -1; selectedNode = -1;
                selectedBeam = -1; draggingNode = false;
                break;

            default: break;
        }
    }
}

// ── Rendering helpers ─────────────────────────────────────────────────────────

static const char* toolLabel(ToolMode m) {
    switch (m) {
        case ToolMode::SELECT:            return "Select";
        case ToolMode::NODE_PLACEMENT:    return "Node";
        case ToolMode::BEAM_CREATION:     return "Beam";
        case ToolMode::FORCE_APPLICATION: return "Force";
    }
    return "";
}

// ── UI ────────────────────────────────────────────────────────────────────────

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
                pushSnapshot(nodes, beams);
                nodes.clear(); beams.clear();
                selectedNode = -1; selectedBeam = -1;
                beamStart    = -1;
                needsSolveFlag = true;
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Open...", "Ctrl+O")) m_showOpenDlg = true;
            if (ImGui::MenuItem("Save...", "Ctrl+S")) m_showSaveDlg = true;
            ImGui::Separator();
            if (ImGui::MenuItem("Export Screenshot", "F12")) m_wantScreenshot = true;
            ImGui::Separator();
            if (ImGui::BeginMenu("Load Template")) {
                if (ImGui::MenuItem("Simple Beam"))   m_templateIdx = 0;
                if (ImGui::MenuItem("Triangle Truss")) m_templateIdx = 1;
                if (ImGui::MenuItem("Portal Frame"))  m_templateIdx = 2;
                if (ImGui::MenuItem("Cantilever"))    m_templateIdx = 3;
                ImGui::EndMenu();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit", "Alt+F4")) {
                SDL_Event q; q.type = SDL_QUIT; SDL_PushEvent(&q);
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit")) {
            if (ImGui::MenuItem("Undo", "Ctrl+Z", false, canUndo())) undo(nodes, beams);
            if (ImGui::MenuItem("Redo", "Ctrl+Y", false, canRedo())) redo(nodes, beams);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Tools")) {
            if (ImGui::MenuItem("Select",  "1",   currentTool == ToolMode::SELECT))
                currentTool = ToolMode::SELECT;
            if (ImGui::MenuItem("Node",    "N",   currentTool == ToolMode::NODE_PLACEMENT))
                currentTool = ToolMode::NODE_PLACEMENT;
            if (ImGui::MenuItem("Beam",    "B",   currentTool == ToolMode::BEAM_CREATION))
                { currentTool = ToolMode::BEAM_CREATION; beamStart = -1; }
            if (ImGui::MenuItem("Force",   "F",   currentTool == ToolMode::FORCE_APPLICATION))
                currentTool = ToolMode::FORCE_APPLICATION;
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Simulation")) {
            if (ImGui::MenuItem("Run Solver", "Enter")) needsSolveFlag = true;
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Stiffness Matrix", nullptr, &showGlassBox);
            ImGui::Separator();
            if (ImGui::MenuItem(beginnerMode ? "Switch to Engineer Mode" : "Switch to Beginner Mode"))
                beginnerMode = !beginnerMode;
            ImGui::EndMenu();
        }

        // Right-aligned: mode indicator + Beginner/Engineer badge
        float rightEdge = ImGui::GetContentRegionAvail().x;
        std::string modeStr = std::string("Mode: ") + toolLabel(currentTool);
        const char* badge = beginnerMode ? "  [Beginner]" : "  [Engineer]";
        ImVec4 badgeCol = beginnerMode ? ImVec4(0.3f,0.9f,0.4f,1.f) : ImVec4(0.4f,0.7f,1.f,1.f);
        float totalW = ImGui::CalcTextSize((modeStr + badge).c_str()).x + 8.0f;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + rightEdge - totalW);
        ImGui::TextDisabled("%s", modeStr.c_str());
        ImGui::SameLine(0, 0);
        ImGui::TextColored(badgeCol, "%s", badge);
        ImGui::EndMainMenuBar();
    }

    // ── Left toolbar ──────────────────────────────────────────────────────────
    const float tbW = 140.0f;
    ImGui::SetNextWindowPos(ImVec2(0, menuH), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(tbW, (float)h - menuH - 24.0f), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.90f);
    ImGui::Begin("##toolbar", nullptr,
                 ImGuiWindowFlags_NoTitleBar   | ImGuiWindowFlags_NoResize   |
                 ImGuiWindowFlags_NoMove       | ImGuiWindowFlags_NoScrollbar |
                 ImGuiWindowFlags_NoBringToFrontOnFocus);

    // Beginner / Engineer mode toggle at top of toolbar
    {
        ImVec4 bc = beginnerMode ? ImVec4(0.15f,0.6f,0.25f,1.f) : ImVec4(0.15f,0.4f,0.75f,1.f);
        ImGui::PushStyleColor(ImGuiCol_Button, bc);
        if (ImGui::Button(beginnerMode ? ICON_FA_GRADUATION_CAP "  Beginner" :
                                         ICON_FA_GEAR "  Engineer", ImVec2(122,26)))
            beginnerMode = !beginnerMode;
        ImGui::PopStyleColor();
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip(beginnerMode
                ? "Beginner mode: friendly labels, plain-English results.\nClick to switch to Engineer mode."
                : "Engineer mode: full E/I/stiffness values, K-matrix.\nClick to switch to Beginner mode.");
    }
    ImGui::Spacing();
    ImGui::TextColored({0.55f, 0.85f, 1.0f, 1.0f}, "TOOLS");
    ImGui::Separator();
    ImGui::Spacing();

    auto toolBtn = [&](ToolMode mode, const char* icon, const char* tip, const char* key) {
        bool active = (currentTool == mode);
        if (active) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.22f,0.52f,0.88f,1.0f));
        char label[48];
        snprintf(label, sizeof(label), "%s  %s", icon, toolLabel(mode));
        if (ImGui::Button(label, ImVec2(122, 34))) currentTool = mode;
        if (active) ImGui::PopStyleColor();
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s  [%s]", tip, key);
        ImGui::Spacing();
    };

    toolBtn(ToolMode::SELECT,            ICON_FA_ARROW_POINTER, "Select / Drag nodes",  "1");
    toolBtn(ToolMode::NODE_PLACEMENT,    ICON_FA_CIRCLE_PLUS,   "Place a node",         "N");
    toolBtn(ToolMode::BEAM_CREATION,     ICON_FA_RULER,         "Connect two nodes",    "B");
    toolBtn(ToolMode::FORCE_APPLICATION, ICON_FA_BOLT,          "Apply force to node",  "F");

    if (currentTool == ToolMode::BEAM_CREATION) {
        ImGui::Separator();
        ImGui::TextColored(beamStart >= 0 ? ImVec4(1.f,.85f,.2f,1.f) : ImVec4(.6f,.6f,.6f,1.f),
                           beamStart >= 0 ? "Click end node" : "Click start node");
    }
    if (currentTool == ToolMode::FORCE_APPLICATION) {
        ImGui::Separator();
        ImGui::Text("Force (N):");
        bool ch = false;
        ch |= ImGui::DragFloat("Fx", &forceMagX, 50.f, -1e6f, 1e6f, "%.0f");
        ch |= ImGui::DragFloat("Fy", &forceMagY, 50.f, -1e6f, 1e6f, "%.0f");
        ch |= ImGui::DragFloat("Fz", &forceMagZ, 50.f, -1e6f, 1e6f, "%.0f");
        if (ch) forceVector = {forceMagX, forceMagY, forceMagZ};
    }

    ImGui::Separator();
    ImGui::Spacing();
    // Undo/redo buttons
    {
        bool noUndo = !canUndo();
        bool noRedo = !canRedo();
        if (noUndo) ImGui::BeginDisabled();
        if (ImGui::Button(ICON_FA_ROTATE_LEFT "  Undo", ImVec2(57,0))) undo(nodes, beams);
        if (noUndo) ImGui::EndDisabled();
        ImGui::SameLine();
        if (noRedo) ImGui::BeginDisabled();
        if (ImGui::Button(ICON_FA_ROTATE_RIGHT "  Redo", ImVec2(57,0))) redo(nodes, beams);
        if (noRedo) ImGui::EndDisabled();
    }
    ImGui::Spacing();
    ImGui::TextColored({0.6f,0.6f,0.6f,1.0f}, "Camera:");
    ImGui::TextWrapped("R-drag: orbit\nScroll: zoom");
    ImGui::Spacing();
    ImGui::TextColored({0.6f,0.6f,0.6f,1.0f}, "Keys:");
    ImGui::TextWrapped("Ctrl+Z: Undo\nCtrl+Y: Redo\nEnter: Solve\nDel: Delete");
    ImGui::End();

    // ── Right properties panel ─────────────────────────────────────────────────
    const float propW = 210.0f;
    ImGui::SetNextWindowPos(ImVec2((float)w - propW, menuH), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(propW, (float)h - menuH - 24.0f), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.90f);
    ImGui::Begin("Properties", nullptr,
                 ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                 ImGuiWindowFlags_NoBringToFrontOnFocus);

    ImGui::TextColored({0.55f,0.85f,1.0f,1.0f}, "SCENE");
    ImGui::Separator();
    ImGui::Text("Nodes : %d", (int)nodes.size());
    ImGui::Text("Beams : %d", (int)beams.size());
    ImGui::Spacing();
    ImGui::TextColored({0.55f,0.85f,1.0f,1.0f}, "DISPLAY");
    ImGui::Separator();
    ImGui::SliderFloat("Disp.Scale", &dispScale, 1.0f, 5000.0f, "%.0fx",
                       ImGuiSliderFlags_Logarithmic);
    ImGui::Checkbox("Show member forces", &showForceLabels);

    // ── Analysis mode ─────────────────────────────────────────────────────────
    ImGui::Spacing();
    ImGui::TextColored({0.55f,0.85f,1.0f,1.0f}, "ANALYSIS MODE");
    ImGui::Separator();
    if (ImGui::Checkbox("Frame mode (6-DOF)", &useFrameMode))
        needsSolveFlag = true;
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Off = pin-jointed truss\nOn  = rigid-jointed frame (bending/shear/moment)");
    if (useFrameMode) {
        ImGui::Checkbox("Show diagram", &showDiagram);
        if (showDiagram) {
            static const char* dnames[] = {
                "Axial  N", "Shear Vy", "Shear Vz",
                "Torsion T", "Moment My", "Moment Mz"
            };
            ImGui::Combo("Diagram", &diagramType, dnames, 6);
        }
    }

    // Stress legend: diverging ramp with numeric end-stops (±MAX_STRESS).
    const float maxKN = ForceRenderer::MAX_STRESS * 1e-3f;
    ImGui::ColorButton("##cmp", ImVec4(1,0,0,1),
                       ImGuiColorEditFlags_NoTooltip, ImVec2(16,16));
    ImGui::SameLine(); ImGui::Text("Compression  -%.0f kN", maxKN);
    ImGui::ColorButton("##neu", ImVec4(1,1,1,1),
                       ImGuiColorEditFlags_NoTooltip, ImVec2(16,16));
    ImGui::SameLine(); ImGui::Text("Neutral       0 kN");
    ImGui::ColorButton("##ten", ImVec4(0,0,1,1),
                       ImGuiColorEditFlags_NoTooltip, ImVec2(16,16));
    ImGui::SameLine(); ImGui::Text("Tension     +%.0f kN", maxKN);

    // Selection indices can go stale after a rebuild; treat out-of-range as none.
    const bool hasNode = selectedNode >= 0 && selectedNode < static_cast<int>(nodes.size());
    const bool hasBeam = selectedBeam >= 0 && selectedBeam < static_cast<int>(beams.size());

    // ── Selected Node ──────────────────────────────────────────────────────────
    if (hasNode) {
        Node& node = nodes[selectedNode];
        ImGui::Spacing();
        ImGui::TextColored({0.55f,0.85f,1.0f,1.0f}, "SELECTED NODE");
        ImGui::Separator();

        glm::vec3 pos = node.getPosition();
        float px = pos.x, py = pos.y, pz = pos.z;
        bool posChg = false;
        posChg |= ImGui::DragFloat("X (m)##p", &px, 0.05f, -100.f, 100.f, "%.3f");
        posChg |= ImGui::DragFloat("Y (m)##p", &py, 0.05f, -100.f, 100.f, "%.3f");
        posChg |= ImGui::DragFloat("Z (m)##p", &pz, 0.05f, -100.f, 100.f, "%.3f");
        if (posChg) {
            pushSnapshot(nodes, beams);
            node.setPosition({px, py, pz});
            needsSolveFlag = true;
        }

        ImGui::Spacing();
        // Joint type: beginner-friendly names vs engineering names
        static const char* jointNamesBeg[] = {
            "No support (free)", "Fixed wall (all locked)",
            "Pin support (pivot)", "Slide left-right",
            "Slide up-down", "Slide in-out"
        };
        static const char* jointNamesEng[] = {
            "FREE", "FIXED (all DOF)", "PIN_XY (Ux=Uy=0)",
            "ROLLER_X (Ux=0)", "ROLLER_Y (Uy=0)", "ROLLER_Z (Uz=0)"
        };
        int jt = static_cast<int>(node.getJointType());
        if (ImGui::Combo("Support", &jt,
                         beginnerMode ? jointNamesBeg : jointNamesEng, 6)) {
            pushSnapshot(nodes, beams);
            node.setJointType(static_cast<JointType>(jt));
            needsSolveFlag = true;
        }
        if (ImGui::IsItemHovered() && beginnerMode)
            ImGui::SetTooltip("Choose how this node is held in place.\n"
                              "Fixed wall: completely locked.\n"
                              "Pin: can rotate but cannot move.\n"
                              "Roller: can slide in one direction.");

        ImGui::Spacing();
        ImGui::TextDisabled(beginnerMode ? "Applied force (N)" : "Applied load (N)");
        glm::vec3 f = node.getAppliedForce();
        float fx = f.x, fy = f.y, fz = f.z;
        bool fChg = false;
        fChg |= ImGui::DragFloat("Fx (N)", &fx, 10.0f, -1e6f, 1e6f, "%.0f");
        fChg |= ImGui::DragFloat("Fy (N)", &fy, 10.0f, -1e6f, 1e6f, "%.0f");
        fChg |= ImGui::DragFloat("Fz (N)", &fz, 10.0f, -1e6f, 1e6f, "%.0f");
        if (fChg) {
            pushSnapshot(nodes, beams);
            node.clearForce();
            node.applyForce({fx, fy, fz});
            needsSolveFlag = true;
        }
        if (ImGui::Button("Clear Force", ImVec2(-1, 0))) {
            pushSnapshot(nodes, beams);
            node.clearForce();
            needsSolveFlag = true;
        }
        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.65f,0.18f,0.18f,1.0f));
        if (ImGui::Button("Delete Node", ImVec2(-1, 0))) {
            pushSnapshot(nodes, beams);
            eraseNodeAt(selectedNode, nodes, beams);
            selectedNode   = -1;
            needsSolveFlag = true;
        }
        ImGui::PopStyleColor();

    // ── Selected Beam ──────────────────────────────────────────────────────────
    } else if (hasBeam) {
        Beam& beam = beams[selectedBeam];
        ImGui::Spacing();
        ImGui::TextColored({0.55f,0.85f,1.0f,1.0f}, "SELECTED BEAM");
        ImGui::Separator();

        float beamL = beam.getLength(nodes);
        ImGui::Text("Length: %.3f m  (%.1f cm)", beamL, beamL * 100.0f);
        ImGui::Spacing();

        // Material preset — always visible
        static const char* matNamesBeg[] = {
            "Steel", "Aluminum", "Concrete", "Timber", "Custom"
        };
        static const char* matNamesEng[] = {
            "Steel (200 GPa)", "Aluminum (70 GPa)",
            "Concrete (30 GPa)", "Timber (12 GPa)", "Custom"
        };
        int matIdx = static_cast<int>(beam.getMaterial());
        if (ImGui::Combo("Material", &matIdx,
                         beginnerMode ? matNamesBeg : matNamesEng, 5)) {
            pushSnapshot(nodes, beams);
            beam.setMaterial(static_cast<BeamMaterial>(matIdx));
            needsSolveFlag = true;
        }
        if (ImGui::IsItemHovered() && beginnerMode)
            ImGui::SetTooltip("Steel: very stiff (bridges, buildings)\n"
                              "Aluminum: lighter, somewhat flexible\n"
                              "Concrete: heavy, medium stiffness\n"
                              "Timber: flexible, natural material");

        // Technical properties — Engineer mode only
        if (!beginnerMode) {
            ImGui::Spacing();
            float E = beam.getYoungsModulus();
            float Egpa = E * 1e-9f;
            if (ImGui::DragFloat("E (GPa)", &Egpa, 0.5f, 0.1f, 1000.0f, "%.1f")) {
                pushSnapshot(nodes, beams);
                beam.setYoungsModulus(Egpa * 1e9f);
                needsSolveFlag = true;
            }
            float Acm2 = beam.getCrossSection() * 1e4f;
            if (ImGui::DragFloat("A (cm\xc2\xb2)", &Acm2, 0.1f, 0.001f, 1000.0f, "%.4f")) {
                pushSnapshot(nodes, beams);
                beam.setCrossSection(Acm2 * 1e-4f);
                needsSolveFlag = true;
            }
            float Icm4 = beam.getMomentOfInertia() * 1e8f;
            if (ImGui::DragFloat("I (cm\xe2\x81\xb4)", &Icm4, 0.001f, 1e-6f, 1e6f, "%.6f")) {
                pushSnapshot(nodes, beams);
                beam.setMomentOfInertia(Icm4 * 1e-8f);
            }
        }
        ImGui::Spacing();

        // Derived stiffness — always show but label differs
        if (beginnerMode)
            ImGui::TextDisabled("Stiffness: %.2e N/m", static_cast<double>(beam.getStiffness(nodes)));
        else
            ImGui::Text("AE/L: %.3e N/m", static_cast<double>(beam.getStiffness(nodes)));
        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.65f,0.18f,0.18f,1.0f));
        if (ImGui::Button("Delete Beam", ImVec2(-1, 0))) {
            pushSnapshot(nodes, beams);
            beams.erase(beams.begin() + selectedBeam);
            selectedBeam   = -1;
            needsSolveFlag = true;
        }
        ImGui::PopStyleColor();

    } else {
        ImGui::Spacing();
        ImGui::TextDisabled("No selection.\nSelect tool + click\na node or beam.");
    }

    ImGui::End();

    // ── Bottom status bar ──────────────────────────────────────────────────────
    ImGui::SetNextWindowPos(ImVec2(0, (float)h - 24.0f), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2((float)w, 24.0f), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.90f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6, 3));
    ImGui::Begin("##status", nullptr,
                 ImGuiWindowFlags_NoTitleBar    | ImGuiWindowFlags_NoResize  |
                 ImGuiWindowFlags_NoMove        | ImGuiWindowFlags_NoScrollbar |
                 ImGuiWindowFlags_NoBringToFrontOnFocus);
    ImGui::Text("Mode: %-8s  |  (%.2f, %.2f, %.2f) m  |  Nodes: %d  Beams: %d  "
                "| Undo: %d  Redo: %d",
                toolLabel(currentTool),
                currentMouseWorldPos.x, currentMouseWorldPos.y, currentMouseWorldPos.z,
                (int)nodes.size(), (int)beams.size(),
                (int)m_undoStack.size(), (int)m_redoStack.size());
    ImGui::End();
    ImGui::PopStyleVar();

    // ── File open/save popups (modal text-input dialogs) ───────────────────────
    if (m_showOpenDlg) { ImGui::OpenPopup("Open File##dlg"); m_showOpenDlg = false; }
    if (m_showSaveDlg) { ImGui::OpenPopup("Save File##dlg"); m_showSaveDlg = false; }

    auto filePopup = [&](const char* title, bool isSave) {
        ImGui::SetNextWindowSize(ImVec2(420, 0), ImGuiCond_Always);
        if (ImGui::BeginPopupModal(title, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("%s", isSave ? "Save structure to CSV file:" : "Open CSV structure file:");
            ImGui::Spacing();
            ImGui::InputText("Path", m_pathBuf, sizeof(m_pathBuf));
            ImGui::Spacing();
            if (ImGui::Button(isSave ? "Save" : "Open", ImVec2(80, 0))) {
                std::string p(m_pathBuf);
                if (!p.empty()) {
                    if (isSave) m_pendingSave = p; else m_pendingLoad = p;
                }
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(80, 0))) ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }
    };
    filePopup("Open File##dlg", false);
    filePopup("Save File##dlg", true);
}
