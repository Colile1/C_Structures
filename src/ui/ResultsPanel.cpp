// Copyright (c) 2026 Colile Sibanda. All rights reserved.
// Proprietary — see LICENSE for terms. Unauthorised use prohibited.
#include "ui/ResultsPanel.hpp"
#include <imgui.h>
#include <cmath>
#include <cstdio>

void renderResultsPanel(const std::vector<Node>& nodes,
                        const std::vector<Beam>& beams,
                        const Simulator& physics,
                        bool beginnerMode,
                        float dispScale)
{
    if (nodes.empty() || beams.empty()) return;

    ImGui::SetNextWindowSize(ImVec2(300, 0), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowCollapsed(true, ImGuiCond_FirstUseEver);
    ImGui::Begin("Results");

    auto displacements = physics.getNodeDisplacements();

    if (beginnerMode) {
        // ── Plain-English summary ─────────────────────────────────────────────
        ImGui::TextColored({0.55f,0.9f,0.55f,1.f}, "What happened to each member:");
        ImGui::Separator();
        for (int i = 0; i < (int)beams.size(); ++i) {
            float force = physics.getBeamForce(beams[i]);
            float fkN   = force * 1e-3f;
            char  desc[128];
            if (std::abs(force) < 50.0f) {
                std::snprintf(desc, sizeof desc, "Member %d: no significant force", i+1);
                ImGui::TextDisabled("%s", desc);
            } else if (force > 0.0f) {
                std::snprintf(desc, sizeof desc,
                    "Member %d: being STRETCHED — %.1f kN of pull (tension)", i+1, fkN);
                ImGui::TextColored({0.4f,0.6f,1.f,1.f}, "%s", desc);
            } else {
                std::snprintf(desc, sizeof desc,
                    "Member %d: being SQUASHED — %.1f kN of push (compression)", i+1, std::abs(fkN));
                ImGui::TextColored({1.f,0.45f,0.45f,1.f}, "%s", desc);
            }
        }
        ImGui::Spacing();
        ImGui::TextColored({0.55f,0.9f,0.55f,1.f}, "How each node moved:");
        ImGui::Separator();
        for (int i = 0; i < (int)nodes.size(); ++i) {
            if (i >= (int)displacements.size()) break;
            glm::vec3 d = displacements[i] * 1000.0f; // to mm
            if (nodes[i].getJointType() == JointType::FIXED) {
                ImGui::TextDisabled("Node %d: fixed — did not move", i+1);
            } else if (glm::length(displacements[i]) < 1e-9f) {
                ImGui::TextDisabled("Node %d: no movement", i+1);
            } else {
                char buf[192];
                std::snprintf(buf, sizeof buf,
                    "Node %d: moved %.3f mm right, %.3f mm up, %.3f mm out",
                    i+1, d.x, d.y, d.z);
                ImGui::TextWrapped("%s", buf);
            }
        }
    } else {
        // ── Engineer mode: compact tables ────────────────────────────────────
        if (ImGui::BeginTable("##mforces", 3,
                              ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
            ImGui::TableSetupColumn("Member"); ImGui::TableSetupColumn("Force (kN)");
            ImGui::TableSetupColumn("State");
            ImGui::TableHeadersRow();
            for (int i = 0; i < (int)beams.size(); ++i) {
                float f = physics.getBeamForce(beams[i]);
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0); ImGui::Text("%d", i+1);
                ImGui::TableSetColumnIndex(1);
                if (f > 50.0f)
                    ImGui::TextColored({0.4f,0.6f,1.f,1.f}, "+%.2f", f*1e-3f);
                else if (f < -50.0f)
                    ImGui::TextColored({1.f,0.45f,0.45f,1.f}, "%.2f", f*1e-3f);
                else
                    ImGui::TextDisabled("~0");
                ImGui::TableSetColumnIndex(2);
                ImGui::TextDisabled(f > 50 ? "Tension" : (f < -50 ? "Compression" : "Zero"));
            }
            ImGui::EndTable();
        }
        ImGui::Spacing();
        if (ImGui::BeginTable("##ndisp", 4,
                              ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
            ImGui::TableSetupColumn("Node"); ImGui::TableSetupColumn("dx (mm)");
            ImGui::TableSetupColumn("dy (mm)"); ImGui::TableSetupColumn("dz (mm)");
            ImGui::TableHeadersRow();
            for (int i = 0; i < (int)nodes.size(); ++i) {
                if (i >= (int)displacements.size()) break;
                glm::vec3 d = displacements[i] * 1000.0f;
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0); ImGui::Text("%d", i+1);
                ImGui::TableSetColumnIndex(1); ImGui::Text("%.4f", d.x);
                ImGui::TableSetColumnIndex(2); ImGui::Text("%.4f", d.y);
                ImGui::TableSetColumnIndex(3); ImGui::Text("%.4f", d.z);
            }
            ImGui::EndTable();
        }
    }

    ImGui::End();
}
