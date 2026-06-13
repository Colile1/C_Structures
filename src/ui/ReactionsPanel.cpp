// Copyright (c) 2026 Colile Sibanda. All rights reserved.
// Proprietary — see LICENSE for terms. Unauthorised use prohibited.
// ui/ReactionsPanel.cpp : renders the reactions table + equilibrium tick.
#include "ui/ReactionsPanel.hpp"
#include "physics/Simulator.hpp"
#include "physics/FrameSimulator.hpp"
#include <imgui.h>
#include <cmath>

// renderReactionsPanel
// Purpose: draw a "Reactions" window listing each support's reaction force and
//          a green/red equilibrium indicator (Σloads + Σreactions ≈ 0).
// Inputs:  nodes — scene nodes; sim — solver; beginnerMode — show plain-English summary.
// Output:  none (draws into the current ImGui frame).
void renderReactionsPanel(const std::vector<Node>& nodes, const Simulator& sim,
                          bool beginnerMode) {
    ImGui::Begin("Reactions");

    if (ImGui::BeginTable("reactions", 4,
            ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("Node");
        ImGui::TableSetupColumn("Rx (N)");
        ImGui::TableSetupColumn("Ry (N)");
        ImGui::TableSetupColumn("Rz (N)");
        ImGui::TableHeadersRow();

        bool anySupport = false;
        for (int i = 0; i < static_cast<int>(nodes.size()); ++i) {
            if (nodes[i].getJointType() == JointType::FREE) continue;
            anySupport = true;
            glm::vec3 r = sim.getNodeReaction(i);
            ImGui::TableNextRow();
            ImGui::TableNextColumn(); ImGui::Text("%d", i);
            ImGui::TableNextColumn(); ImGui::Text("%.1f", r.x);
            ImGui::TableNextColumn(); ImGui::Text("%.1f", r.y);
            ImGui::TableNextColumn(); ImGui::Text("%.1f", r.z);
        }
        if (!anySupport) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextDisabled("no supports");
        }
        ImGui::EndTable();
    }

    glm::vec3 net;
    const bool balanced = sim.checkEquilibrium(net);
    const float mag = std::sqrt(net.x*net.x + net.y*net.y + net.z*net.z);
    if (balanced)
        ImGui::TextColored(ImVec4(0.25f, 0.85f, 0.35f, 1.0f),
                           "Equilibrium OK  (residual %.2g N)", static_cast<double>(mag));
    else
        ImGui::TextColored(ImVec4(0.95f, 0.45f, 0.25f, 1.0f),
                           "Unbalanced: %.1f, %.1f, %.1f N", net.x, net.y, net.z);

    // Plain-English summary (beginner mode only)
    if (beginnerMode) {
        ImGui::Spacing();
        ImGui::TextColored({0.55f,0.9f,0.55f,1.f}, "What the supports are doing:");
        ImGui::Separator();
        for (int i = 0; i < static_cast<int>(nodes.size()); ++i) {
            if (nodes[i].getJointType() == JointType::FREE) continue;
            glm::vec3 r = sim.getNodeReaction(i);
            float rkN = glm::length(r) * 1e-3f;
            char buf[192];
            if (rkN < 0.05f) {
                std::snprintf(buf, sizeof buf,
                    "Node %d: support barely loaded (< 0.05 kN total).", i);
            } else {
                std::snprintf(buf, sizeof buf,
                    "Node %d: pushing back with %.2f kN"
                    " (%.1f right, %.1f up, %.1f out).",
                    i, rkN, r.x*1e-3f, r.y*1e-3f, r.z*1e-3f);
            }
            ImGui::TextWrapped("%s", buf);
        }
        if (balanced)
            ImGui::TextWrapped("The structure is in balance — all applied loads are "
                               "carried to the supports with no leftover force.");
        else
            ImGui::TextColored({1.f,0.55f,0.25f,1.f},
                               "Forces are not balanced — check your supports.");
    }

    ImGui::End();
}

// renderReactionsPanel (frame overload)
// Purpose: draw the "Reactions" window for the 6-DOF frame solver, listing each
//          support's reaction force (Rx,Ry,Rz) and moment (Mx,My,Mz) plus a
//          green/red ΣF equilibrium indicator.
// Inputs:  nodes — scene nodes; sim — frame solver; beginnerMode — plain-English summary.
// Output:  none (draws into the current ImGui frame).
void renderReactionsPanel(const std::vector<Node>& nodes, const FrameSimulator& sim,
                          bool beginnerMode) {
    ImGui::Begin("Reactions");

    if (ImGui::BeginTable("reactions", 7,
            ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("Node");
        ImGui::TableSetupColumn("Rx (N)");
        ImGui::TableSetupColumn("Ry (N)");
        ImGui::TableSetupColumn("Rz (N)");
        ImGui::TableSetupColumn("Mx (N\xc2\xb7m)");
        ImGui::TableSetupColumn("My (N\xc2\xb7m)");
        ImGui::TableSetupColumn("Mz (N\xc2\xb7m)");
        ImGui::TableHeadersRow();

        bool anySupport = false;
        for (int i = 0; i < static_cast<int>(nodes.size()); ++i) {
            if (nodes[i].getJointType() == JointType::FREE) continue;
            anySupport = true;
            glm::vec3 rf = sim.getNodeReactionForce(i);
            glm::vec3 rm = sim.getNodeReactionMoment(i);
            ImGui::TableNextRow();
            ImGui::TableNextColumn(); ImGui::Text("%d", i);
            ImGui::TableNextColumn(); ImGui::Text("%.1f", rf.x);
            ImGui::TableNextColumn(); ImGui::Text("%.1f", rf.y);
            ImGui::TableNextColumn(); ImGui::Text("%.1f", rf.z);
            ImGui::TableNextColumn(); ImGui::Text("%.1f", rm.x);
            ImGui::TableNextColumn(); ImGui::Text("%.1f", rm.y);
            ImGui::TableNextColumn(); ImGui::Text("%.1f", rm.z);
        }
        if (!anySupport) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextDisabled("no supports");
        }
        ImGui::EndTable();
    }

    glm::vec3 net;
    const bool balanced = sim.checkForceEquilibrium(net);
    const float mag = std::sqrt(net.x*net.x + net.y*net.y + net.z*net.z);
    if (balanced)
        ImGui::TextColored(ImVec4(0.25f, 0.85f, 0.35f, 1.0f),
                           "Equilibrium OK  (residual %.2g N)", static_cast<double>(mag));
    else
        ImGui::TextColored(ImVec4(0.95f, 0.45f, 0.25f, 1.0f),
                           "Unbalanced: %.1f, %.1f, %.1f N", net.x, net.y, net.z);

    // Plain-English summary (beginner mode only)
    if (beginnerMode) {
        ImGui::Spacing();
        ImGui::TextColored({0.55f,0.9f,0.55f,1.f}, "What the supports are doing:");
        ImGui::Separator();
        for (int i = 0; i < static_cast<int>(nodes.size()); ++i) {
            if (nodes[i].getJointType() == JointType::FREE) continue;
            glm::vec3 rf = sim.getNodeReactionForce(i);
            glm::vec3 rm = sim.getNodeReactionMoment(i);
            float fkN   = glm::length(rf) * 1e-3f;
            float mkNm  = glm::length(rm) * 1e-3f;
            char buf[256];
            if (fkN < 0.05f && mkNm < 0.05f) {
                std::snprintf(buf, sizeof buf,
                    "Node %d: support barely loaded.", i);
            } else if (mkNm < 0.05f) {
                std::snprintf(buf, sizeof buf,
                    "Node %d: pushing back with %.2f kN force"
                    " (%.1f right, %.1f up).",
                    i, fkN, rf.x*1e-3f, rf.y*1e-3f);
            } else {
                std::snprintf(buf, sizeof buf,
                    "Node %d: pushing back with %.2f kN force and"
                    " %.2f kN\xc2\xb7m resisting moment.",
                    i, fkN, mkNm);
            }
            ImGui::TextWrapped("%s", buf);
        }
        if (balanced)
            ImGui::TextWrapped("The structure is in balance — all applied loads are "
                               "carried to the supports with no leftover force.");
        else
            ImGui::TextColored({1.f,0.55f,0.25f,1.f},
                               "Forces are not balanced — check your supports.");
    }

    ImGui::End();
}
