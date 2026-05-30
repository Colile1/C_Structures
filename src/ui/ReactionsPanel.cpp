// Copyright (c) 2026 Colile Sibanda. All rights reserved.
// Proprietary — see LICENSE for terms. Unauthorised use prohibited.
// ui/ReactionsPanel.cpp : renders the reactions table + equilibrium tick.
#include "ui/ReactionsPanel.hpp"
#include "physics/Simulator.hpp"
#include <imgui.h>
#include <cmath>

// renderReactionsPanel
// Purpose: draw a "Reactions" window listing each support's reaction force and
//          a green/red equilibrium indicator (Σloads + Σreactions ≈ 0).
// Inputs:  nodes — scene nodes; sim — solver holding the latest solution.
// Output:  none (draws into the current ImGui frame).
void renderReactionsPanel(const std::vector<Node>& nodes, const Simulator& sim) {
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

    ImGui::End();
}
