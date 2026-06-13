// Copyright (c) 2026 Colile Sibanda. All rights reserved.
// Proprietary — see LICENSE for terms. Unauthorised use prohibited.
#include "ui/UIHandler.hpp"
#include "visualization/ForceRenderer.hpp"
#include <imgui.h>
#include "IconsFontAwesome6.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>
#include <cstdio>
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
        glm::vec3 m = n.getAppliedMoment();
        glm::vec3 p = n.getPosition();
        snap.nodes.push_back({p.x, p.y, p.z, n.getJointType(),
                              f.x, f.y, f.z, m.x, m.y, m.z});
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
        nodes.back().applyMoment({ns.mx, ns.my, ns.mz});
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
        glm::vec3 m = n.getAppliedMoment();
        glm::vec3 p = n.getPosition();
        current.nodes.push_back({p.x, p.y, p.z, n.getJointType(),
                                 f.x, f.y, f.z, m.x, m.y, m.z});
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
        glm::vec3 m = n.getAppliedMoment();
        glm::vec3 p = n.getPosition();
        current.nodes.push_back({p.x, p.y, p.z, n.getJointType(),
                                 f.x, f.y, f.z, m.x, m.y, m.z});
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
            case SDLK_i: if (ctrl) m_wantPNG = true; break;
            case SDLK_e: if (ctrl) m_wantPDF = true; break;

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

// ── Component palette ───────────────────────────────────────────────────────────

// imageOrTextButton
// Purpose: draw an icon image-button, falling back to a text button when the SVG
//          could not be rasterised (e.g. resources/ missing). A light backdrop
//          keeps the dark schematic symbols legible on the dark UI theme.
// Output:  true when clicked.
static bool imageOrTextButton(IconLibrary& icons, const char* cat, const char* name,
                              float side, bool active) {
    ImTextureID tex = icons.texture(cat, name);
    const ImVec4 bg = active ? ImVec4(0.80f, 0.90f, 1.00f, 1.0f)   // selected: bluish
                             : ImVec4(0.93f, 0.95f, 0.97f, 1.0f);  // card white
    if (tex) {
        char id[64]; std::snprintf(id, sizeof id, "##ic_%s_%s", cat, name);
        return ImGui::ImageButton(id, tex, ImVec2(side, side),
                                  ImVec2(0, 0), ImVec2(1, 1), bg, ImVec4(1, 1, 1, 1));
    }
    char lbl[48]; std::snprintf(lbl, sizeof lbl, "%.6s##t_%s", name, name);
    if (active) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.22f, 0.52f, 0.88f, 1.0f));
    bool c = ImGui::Button(lbl, ImVec2(side + 8, side + 8));
    if (active) ImGui::PopStyleColor();
    return c;
}

void UIHandler::renderPalette(float originY, float availH,
                              std::vector<Node>& nodes,
                              std::vector<Beam>& beams) {
    const float palW = 250.0f;
    ImGui::SetNextWindowPos(ImVec2(146.0f, originY), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(palW, availH), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowBgAlpha(0.94f);
    if (!ImGui::Begin("Component Palette", &showPalette)) { ImGui::End(); return; }

    // ── View-mode toggle (symbol / realistic-2D / realistic-3D) ────────────────
    ImGui::TextDisabled("Icon view");
    auto modeBtn = [&](const char* label, IconView v) {
        bool on = (m_icons.view() == v);
        if (on) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.22f, 0.52f, 0.88f, 1.0f));
        if (ImGui::Button(label, ImVec2(74, 0))) m_icons.setView(v);
        if (on) ImGui::PopStyleColor();
    };
    modeBtn("Symbol",  IconView::Symbol);      ImGui::SameLine(0, 3);
    modeBtn("2D",      IconView::Realistic2D); ImGui::SameLine(0, 3);
    modeBtn("3D",      IconView::Realistic3D);
    ImGui::Separator();

    const float side  = 40.0f;
    const float cellW = side + 14.0f;          // button + ImGui frame padding/spacing
    auto perRow = [&]() {
        float avail = ImGui::GetContentRegionAvail().x;
        return std::max(1, static_cast<int>(avail / cellW));
    };

    const bool hasSel = (selectedNode >= 0 && selectedNode < (int)nodes.size());
    JointType  curJT  = hasSel ? nodes[selectedNode].getJointType() : JointType::FREE;

    // ── Joints & supports ──────────────────────────────────────────────────────
    ImGui::TextColored({0.55f, 0.85f, 1.0f, 1.0f}, "JOINTS & SUPPORTS");
    if (hasSel) ImGui::TextDisabled("Click to set selected node's support.");
    else        ImGui::TextDisabled("Select a node to assign a support.");
    struct JIcon { const char* name; const char* title; int jt; };
    static const JIcon joints[] = {
        {"free",          "Free node (internal joint)",   (int)JointType::FREE},
        {"fixed",         "Fixed support (encastré)",     (int)JointType::FIXED},
        {"pin_xy",        "Pinned support (pin / hinge)", (int)JointType::PIN_XY},
        {"roller_x",      "Roller support (X-constrained)", (int)JointType::ROLLER_X},
        {"roller_y",      "Roller support (Y-constrained)", (int)JointType::ROLLER_Y},
        {"roller_z",      "Roller support (Z-constrained)", (int)JointType::ROLLER_Z},
        {"internal_hinge","Internal hinge (set per member-end on a beam)", -1},
        {"rigid",         "Rigid moment connection (default joint)",       -1},
    };
    int col = 0, pr = perRow();
    for (const auto& j : joints) {
        bool active = hasSel && j.jt >= 0 && curJT == static_cast<JointType>(j.jt);
        if (imageOrTextButton(m_icons, "joints", j.name, side, active) && j.jt >= 0 && hasSel) {
            pushSnapshot(nodes, beams);
            nodes[selectedNode].setJointType(static_cast<JointType>(j.jt));
            needsSolveFlag = true;
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", j.title);
        if (++col % pr != 0) ImGui::SameLine();
    }
    if (col % pr != 0) ImGui::NewLine();
    ImGui::Spacing();

    // ── Beam sections & types (illustrative reference) ─────────────────────────
    ImGui::TextColored({0.55f, 0.85f, 1.0f, 1.0f}, "SECTIONS & TYPES");
    struct NIcon { const char* name; const char* title; };
    static const NIcon sections[] = {
        {"i_beam","I-section (universal beam)"}, {"h_column","H-section (universal column)"},
        {"channel_c","Channel (C / PFC)"},       {"angle_l","Angle (L)"},
        {"t_section","Tee (T)"},                 {"box_rhs","Box / RHS (hollow rect)"},
        {"pipe_chs","Pipe / CHS (hollow round)"},{"solid_rect","Solid rectangular bar"},
        {"solid_round","Solid round bar"},       {"truss","Truss member"},
        {"simply_supported","Simply supported beam"}, {"cantilever","Cantilever beam"},
        {"continuous","Continuous beam"},        {"fixed_both","Fixed-fixed beam"},
        {"overhanging","Overhanging beam"},
    };
    col = 0; pr = perRow();
    for (const auto& s : sections) {
        imageOrTextButton(m_icons, "beams", s.name, side, false);
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", s.title);
        if (++col % pr != 0) ImGui::SameLine();
    }
    if (col % pr != 0) ImGui::NewLine();
    ImGui::Spacing();

    // ── Forces & loads ─────────────────────────────────────────────────────────
    ImGui::TextColored({0.55f, 0.85f, 1.0f, 1.0f}, "FORCES & LOADS");
    static const NIcon forces[] = {
        {"point_load","Point load — click to pick the Force tool"},
        {"moment","Moment (couple) — set Mx/My/Mz in node properties"},
        {"udl","Uniformly distributed load — add in the Loads panel (frame mode)"},
        {"triangular_load","Triangular load — add in the Loads panel (frame mode)"},
        {"self_weight","Self-weight — toggle in the Loads panel"},
        {"tension","Axial tension (result colour)"},
        {"compression","Axial compression (result colour)"},
        {"shear","Shear force (diagram)"},
        {"reaction","Support reaction (Reactions panel)"},
    };
    col = 0; pr = perRow();
    for (const auto& f : forces) {
        bool isPoint = std::string(f.name) == "point_load";
        bool active  = isPoint && currentTool == ToolMode::FORCE_APPLICATION;
        if (imageOrTextButton(m_icons, "forces", f.name, side, active) && isPoint)
            currentTool = ToolMode::FORCE_APPLICATION;
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", f.title);
        if (++col % pr != 0) ImGui::SameLine();
    }
    if (col % pr != 0) ImGui::NewLine();

    ImGui::End();
}

// ── UI ────────────────────────────────────────────────────────────────────────

void UIHandler::renderUI(SDL_Window* window,
                          std::vector<Node>& nodes,
                          std::vector<Beam>& beams,
                          float& dispMult,
                          float autoDispScale) {
    int w = 0, h = 0;
    SDL_GetWindowSize(window, &w, &h);
    const float menuH = ImGui::GetFrameHeight();

    // ── Top menu bar ──────────────────────────────────────────────────────────
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem(ICON_FA_FILE "  New Structure", "Ctrl+N")) {
                pushSnapshot(nodes, beams);
                nodes.clear(); beams.clear();
                selectedNode = -1; selectedBeam = -1;
                beamStart    = -1;
                needsSolveFlag = true;
            }
            ImGui::Separator();
            if (ImGui::MenuItem(ICON_FA_FOLDER "  Open...",        "Ctrl+O")) m_showOpenDlg = true;
            if (ImGui::MenuItem(ICON_FA_FLOPPY_DISK "  Save...",   "Ctrl+S")) m_showSaveDlg = true;
            ImGui::Separator();
            if (ImGui::MenuItem(ICON_FA_CAMERA "  Export Screenshot", "F12")) m_wantScreenshot = true;
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Save the current view as a BMP file.");
            if (ImGui::MenuItem(ICON_FA_FILE_IMAGE "  Export PNG Image", "Ctrl+I")) m_wantPNG = true;
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Save the current view as a PNG file.");
            if (ImGui::MenuItem(ICON_FA_FILE_PDF "  Export PDF Report", "Ctrl+E")) m_wantPDF = true;
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Save a one-page PDF with model summary, reactions,\n"
                                  "member forces, and a viewport screenshot.");
            ImGui::Separator();
            if (ImGui::MenuItem(ICON_FA_TABLE "  Example Structures..."))
                m_showTemplatesDlg = true;
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Browse pre-built example structures as one-click cards.");
            if (ImGui::BeginMenu(ICON_FA_TABLE "  Load Template")) {
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Pre-built example structures — great starting points.");
                if (ImGui::MenuItem(ICON_FA_RULER "  Simple Beam",
                                    nullptr, false, true))   m_templateIdx = 0;
                if (ImGui::IsItemHovered()) ImGui::SetTooltip(
                    "Horizontal beam on two supports with a midpoint load.\n"
                    "Classic simply-supported beam — shows reactions and midspan deflection.");
                if (ImGui::MenuItem(ICON_FA_DRAW_POLYGON "  Triangle Truss",
                                    nullptr, false, true))   m_templateIdx = 1;
                if (ImGui::IsItemHovered()) ImGui::SetTooltip(
                    "Two fixed bases, free apex under downward load.\n"
                    "Shows tension and compression in inclined members.");
                if (ImGui::MenuItem(ICON_FA_HOUSE "  Portal Frame",
                                    nullptr, false, true))   m_templateIdx = 2;
                if (ImGui::IsItemHovered()) ImGui::SetTooltip(
                    "Two fixed-base columns and a horizontal beam.\n"
                    "Side load produces bending moments — ideal for frame mode.");
                if (ImGui::MenuItem(ICON_FA_WRENCH "  Cantilever",
                                    nullptr, false, true))   m_templateIdx = 3;
                if (ImGui::IsItemHovered()) ImGui::SetTooltip(
                    "Fixed wall on the left, free tip on the right.\n"
                    "Tip load produces a classic triangular moment diagram.");
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
            ImGui::MenuItem("Component Palette", nullptr, &showPalette);
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

    // ── Quick support assignment (shown when a node is selected) ─────────────
    const bool hasSelNode = (selectedNode >= 0 && selectedNode < (int)nodes.size());
    if (hasSelNode) {
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::TextColored({0.55f,0.85f,1.f,1.f}, "SUPPORTS");

        struct SupportBtn { JointType jt; const char* icon; const char* tip; };
        static const SupportBtn sbts[] = {
            { JointType::FREE,     ICON_FA_CIRCLE,        beginnerMode ? "No support (free)" : "FREE" },
            { JointType::FIXED,    ICON_FA_LOCK,          beginnerMode ? "Fixed wall"        : "FIXED (all DOF)" },
            { JointType::PIN_XY,   ICON_FA_THUMBTACK,     beginnerMode ? "Pin support"       : "PIN_XY (Ux=Uy=0)" },
            { JointType::ROLLER_X, ICON_FA_ARROW_RIGHT,   beginnerMode ? "Slide left/right"  : "ROLLER_X (Ux=0)" },
            { JointType::ROLLER_Y, ICON_FA_ARROW_UP,      beginnerMode ? "Slide up/down"     : "ROLLER_Y (Uy=0)" },
            { JointType::ROLLER_Z, ICON_FA_ARROW_DOWN_UP_ACROSS_LINE,
                                                           beginnerMode ? "Slide in/out"      : "ROLLER_Z (Uz=0)" },
        };
        JointType curJT = nodes[selectedNode].getJointType();
        for (const auto& sb : sbts) {
            bool active = (curJT == sb.jt);
            if (active) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.22f,0.52f,0.88f,1.f));
            char lbl[32]; snprintf(lbl, sizeof lbl, "%s##sup%d", sb.icon, (int)sb.jt);
            if (ImGui::Button(lbl, ImVec2(40, 28))) {
                pushSnapshot(nodes, beams);
                nodes[selectedNode].setJointType(sb.jt);
                needsSolveFlag = true;
            }
            if (active) ImGui::PopStyleColor();
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", sb.tip);
            ImGui::SameLine(0, 2);
        }
        ImGui::NewLine();
        ImGui::Spacing();
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
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
            ImGui::SetTooltip("Undo the last change  [Ctrl+Z]");
        ImGui::SameLine();
        if (noRedo) ImGui::BeginDisabled();
        if (ImGui::Button(ICON_FA_ROTATE_RIGHT "  Redo", ImVec2(57,0))) redo(nodes, beams);
        if (noRedo) ImGui::EndDisabled();
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
            ImGui::SetTooltip("Redo the last undone change  [Ctrl+Y]");
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
    ImGui::SliderFloat("Disp. mult.", &dispMult, 0.1f, 20.0f, "%.2f\xc3\x97",
                       ImGuiSliderFlags_Logarithmic);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Scale the deformed shape overlay.\n"
                          "Auto-scale already fits the shape to the view;\n"
                          "this slider multiplies that further.");
    char scaleLabel[48];
    std::snprintf(scaleLabel, sizeof scaleLabel, "\xc3\x97%.0f (auto)", autoDispScale * dispMult);
    ImGui::TextDisabled("%s", scaleLabel);
    ImGui::Checkbox("Show member forces", &showForceLabels);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Draw the axial force value next to each member in the 3D view.");

    // ── Analysis mode ─────────────────────────────────────────────────────────
    ImGui::Spacing();
    ImGui::TextColored({0.55f,0.85f,1.0f,1.0f}, "ANALYSIS MODE");
    ImGui::Separator();
    if (ImGui::Checkbox("Frame mode (6-DOF)", &useFrameMode))
        needsSolveFlag = true;
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Off = pin-jointed truss (axial forces only)\n"
                          "On  = rigid-jointed frame (axial + shear + bending + torsion)");
    if (useFrameMode) {
        ImGui::Checkbox("Show diagram", &showDiagram);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Overlay the selected internal-force diagram on each member.");
        if (showDiagram) {
            static const char* dnames[] = {
                "Axial  N", "Shear Vy", "Shear Vz",
                "Torsion T", "Moment My", "Moment Mz"
            };
            ImGui::Combo("Diagram", &diagramType, dnames, 6);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Choose which internal force to display:\n"
                                  "N = axial  Vy/Vz = shear  T = torsion  My/Mz = bending moment");
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

        // Picture of the node's current support type (current icon view mode).
        {
            static const char* jIcon[] = {
                "free", "fixed", "pin_xy", "roller_x", "roller_y", "roller_z"
            };
            int jti = static_cast<int>(node.getJointType());
            if (jti >= 0 && jti < 6) {
                if (ImTextureID t = m_icons.texture("joints", jIcon[jti])) {
                    const float s = 52.0f;
                    ImVec2 p = ImGui::GetCursorScreenPos();
                    ImGui::GetWindowDrawList()->AddRectFilled(
                        p, ImVec2(p.x + s, p.y + s), IM_COL32(238, 242, 247, 255), 3.0f);
                    ImGui::Image(t, ImVec2(s, s));
                }
            }
        }

        glm::vec3 pos = node.getPosition();
        float px = pos.x, py = pos.y, pz = pos.z;
        bool posChg = false;
        posChg |= ImGui::DragFloat("X (m)##p", &px, 0.05f, -100.f, 100.f, "%.3f");
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Horizontal position (drag or double-click to type).");
        posChg |= ImGui::DragFloat("Y (m)##p", &py, 0.05f, -100.f, 100.f, "%.3f");
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Vertical position (drag or double-click to type).");
        posChg |= ImGui::DragFloat("Z (m)##p", &pz, 0.05f, -100.f, 100.f, "%.3f");
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Depth position (drag or double-click to type).");
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
        if (ImGui::IsItemHovered()) ImGui::SetTooltip(beginnerMode
            ? "Horizontal push/pull on this joint (positive = rightward)."
            : "Nodal force in the global X direction (N).");
        fChg |= ImGui::DragFloat("Fy (N)", &fy, 10.0f, -1e6f, 1e6f, "%.0f");
        if (ImGui::IsItemHovered()) ImGui::SetTooltip(beginnerMode
            ? "Vertical push/pull on this joint (positive = upward).\nTip: gravity loads are negative."
            : "Nodal force in the global Y direction (N).");
        fChg |= ImGui::DragFloat("Fz (N)", &fz, 10.0f, -1e6f, 1e6f, "%.0f");
        if (ImGui::IsItemHovered()) ImGui::SetTooltip(beginnerMode
            ? "In/out-of-plane push on this joint (positive = toward you)."
            : "Nodal force in the global Z direction (N).");
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
        ImGui::TextDisabled(beginnerMode ? "Applied turning effect (N·m)"
                                         : "Applied moment (N·m)");
        glm::vec3 mo = node.getAppliedMoment();
        float mx = mo.x, my = mo.y, mz = mo.z;
        bool mChg = false;
        mChg |= ImGui::DragFloat("Mx (N\xc2\xb7m)", &mx, 10.0f, -1e6f, 1e6f, "%.0f");
        if (ImGui::IsItemHovered()) ImGui::SetTooltip(beginnerMode
            ? "Concentrated moment twisting about the horizontal axis.\nOnly active in frame mode."
            : "Concentrated nodal moment about the global X axis (N\xc2\xb7m). Frame mode only.");
        mChg |= ImGui::DragFloat("My (N\xc2\xb7m)", &my, 10.0f, -1e6f, 1e6f, "%.0f");
        if (ImGui::IsItemHovered()) ImGui::SetTooltip(beginnerMode
            ? "Concentrated moment twisting about the vertical axis.\nOnly active in frame mode."
            : "Concentrated nodal moment about the global Y axis (N\xc2\xb7m). Frame mode only.");
        mChg |= ImGui::DragFloat("Mz (N\xc2\xb7m)", &mz, 10.0f, -1e6f, 1e6f, "%.0f");
        if (ImGui::IsItemHovered()) ImGui::SetTooltip(beginnerMode
            ? "Concentrated moment twisting in the XY-plane (most common).\nOnly active in frame mode."
            : "Concentrated nodal moment about the global Z axis (N\xc2\xb7m). Frame mode only.");
        if (mChg) {
            pushSnapshot(nodes, beams);
            node.clearMoment();
            node.applyMoment({mx, my, mz});
            needsSolveFlag = true;
        }
        if (ImGui::IsItemHovered() && beginnerMode)
            ImGui::SetTooltip("A concentrated moment twists the joint.\n"
                              "Only acts in frame mode (rigid joints).");
        if (ImGui::Button("Clear Moment", ImVec2(-1, 0))) {
            pushSnapshot(nodes, beams);
            node.clearMoment();
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
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Young's modulus — material stiffness (GPa).\n"
                                  "Steel \xe2\x89\x88 200, Aluminium \xe2\x89\x88 70, Concrete \xe2\x89\x88 30, Timber \xe2\x89\x88 12.");
            float Acm2 = beam.getCrossSection() * 1e4f;
            if (ImGui::DragFloat("A (cm\xc2\xb2)", &Acm2, 0.1f, 0.001f, 1000.0f, "%.4f")) {
                pushSnapshot(nodes, beams);
                beam.setCrossSection(Acm2 * 1e-4f);
                needsSolveFlag = true;
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Cross-sectional area (cm\xc2\xb2).\n"
                                  "Governs axial stiffness EA/L and self-weight.");
            float Icm4 = beam.getMomentOfInertia() * 1e8f;
            if (ImGui::DragFloat("I (cm\xe2\x81\xb4)", &Icm4, 0.001f, 1e-6f, 1e6f, "%.6f")) {
                pushSnapshot(nodes, beams);
                beam.setMomentOfInertia(Icm4 * 1e-8f);
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Second moment of area (cm\xe2\x81\xb4).\n"
                                  "Governs bending stiffness EI — only used in frame mode.");
        }
        ImGui::Spacing();

        // Derived stiffness — always show but label differs
        if (beginnerMode)
            ImGui::TextDisabled("Stiffness: %.2e N/m", static_cast<double>(beam.getStiffness(nodes)));
        else
            ImGui::Text("AE/L: %.3e N/m", static_cast<double>(beam.getStiffness(nodes)));
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip(beginnerMode
                ? "How stiff this member is — higher means less stretch under the same load."
                : "Axial stiffness AE/L (N/m). Determines elongation under axial load.");

        // Member-end releases (internal hinges) — frame mode only. A released
        // end transmits force but no bending moment, turning a rigid connection
        // into a pin (the internal_hinge vs rigid joint distinction).
        if (useFrameMode) {
            ImGui::Spacing();
            ImGui::TextColored({0.55f,0.85f,1.0f,1.0f},
                               beginnerMode ? "JOINT CONNECTIONS" : "MEMBER-END RELEASES");
            ImGui::Separator();
            bool relStart = beam.getStartMomentRelease();
            bool relEnd   = beam.getEndMomentRelease();
            if (ImGui::Checkbox(beginnerMode ? "Hinge at start (pin)" : "Release moment at start",
                                &relStart)) {
                pushSnapshot(nodes, beams);
                beam.setStartMomentRelease(relStart);
                needsSolveFlag = true;
            }
            if (ImGui::Checkbox(beginnerMode ? "Hinge at end (pin)" : "Release moment at end",
                                &relEnd)) {
                pushSnapshot(nodes, beams);
                beam.setEndMomentRelease(relEnd);
                needsSolveFlag = true;
            }
            if (ImGui::IsItemHovered() && beginnerMode)
                ImGui::SetTooltip("Rigid = the member carries bending moment into the joint.\n"
                                  "Hinge = a pin that lets the member rotate freely (no moment).");
        }

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

    // ── Component palette (left, icon-based; toggled from the View menu) ────────
    if (showPalette)
        renderPalette(menuH, (float)h - menuH - 24.0f, nodes, beams);

    // ── Example Structures card popup ─────────────────────────────────────────
    if (m_showTemplatesDlg) {
        ImGui::OpenPopup("Example Structures##cards");
        m_showTemplatesDlg = false;
    }
    ImGui::SetNextWindowSize(ImVec2(480, 0), ImGuiCond_Always);
    if (ImGui::BeginPopupModal("Example Structures##cards", nullptr,
                               ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextColored({0.55f,0.85f,1.0f,1.0f},
                           "Click a card to load the example — your current model will be replaced.");
        ImGui::Spacing();

        struct TplCard {
            const char* icon;
            const char* title;
            const char* line1;
            const char* line2;
            int         idx;
        };
        static const TplCard cards[] = {
            { ICON_FA_RULER,
              "Simple Beam",
              "Horizontal beam on two supports with a 10 kN midpoint load.",
              "Shows reactions and midspan deflection — the classic first example.",
              0 },
            { ICON_FA_DRAW_POLYGON,
              "Triangle Truss",
              "Two fixed bases with a 50 kN downward load at the free apex.",
              "Demonstrates tension in one member and compression in the other.",
              1 },
            { ICON_FA_HOUSE,
              "Portal Frame",
              "Two fixed-base columns joined by a beam, with a 20 kN side load.",
              "Switch to Frame mode to see the bending moment diagram.",
              2 },
            { ICON_FA_WRENCH,
              "Cantilever",
              "Fixed wall at left, free tip at right, with a 5 kN tip load.",
              "Produces a triangular moment diagram — great for checking EI.",
              3 },
        };

        for (const auto& c : cards) {
            // Card background
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.13f, 0.15f, 0.20f, 1.0f));
            ImGui::BeginChild(c.title, ImVec2(-1, 72), true,
                              ImGuiWindowFlags_NoScrollbar);

            // Icon column
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 6.0f);
            ImGui::TextColored({0.55f,0.85f,1.0f,1.0f}, "%s", c.icon);
            ImGui::SameLine(50.0f);

            // Text column
            float textX = ImGui::GetCursorPosX();
            ImGui::BeginGroup();
            ImGui::TextColored({0.95f,0.95f,0.95f,1.0f}, "%s", c.title);
            ImGui::SetCursorPosX(textX);
            ImGui::TextDisabled("%s", c.line1);
            ImGui::SetCursorPosX(textX);
            ImGui::TextDisabled("%s", c.line2);
            ImGui::EndGroup();

            // Load button (right-aligned)
            float btnW = 68.0f;
            ImGui::SameLine(ImGui::GetWindowWidth() - btnW - 8.0f);
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 36.0f);
            char btnId[32]; std::snprintf(btnId, sizeof btnId, "Load##%d", c.idx);
            ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.15f,0.50f,0.90f,1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f,0.60f,1.00f,1.0f));
            if (ImGui::Button(btnId, ImVec2(btnW, 28))) {
                m_templateIdx = c.idx;
                ImGui::CloseCurrentPopup();
            }
            ImGui::PopStyleColor(2);

            ImGui::EndChild();
            ImGui::PopStyleColor();
            ImGui::Spacing();
        }

        ImGui::Spacing();
        if (ImGui::Button("Close", ImVec2(-1, 0))) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

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
