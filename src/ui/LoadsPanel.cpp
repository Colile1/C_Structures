// Copyright (c) 2026 Colile Sibanda. All rights reserved.
// Proprietary — see LICENSE for terms. Unauthorised use prohibited.
#include "ui/LoadsPanel.hpp"
#include <imgui.h>
#include <cstdio>

bool renderLoadsPanel(std::vector<DistributedLoad>& loads,
                      FrameSimulator& frameSim,
                      const std::vector<Beam>& beams)
{
    bool changed = false;
    ImGui::SetNextWindowSize(ImVec2(260, 0), ImGuiCond_FirstUseEver);
    ImGui::Begin("Distributed Loads");

    ImGui::TextColored({0.55f,0.85f,1.0f,1.0f}, "DISTRIBUTED LOADS");
    ImGui::Separator();
    ImGui::TextWrapped("Applied in frame mode. Each load is resolved into "
                       "consistent equivalent nodal loads.");
    ImGui::Spacing();

    // ── Add new load ─────────────────────────────────────────────────────────
    static int    s_beamIdx   = 0;
    static int    s_typeIdx   = 0;      // 0=UDL  1=Triangular  2=Moment
    static float  s_w         = 1000.0f;
    static float  s_w2        = 0.0f;
    static float  s_pos       = 0.5f;
    static float  s_dir[3]    = {0.0f, -1.0f, 0.0f};

    const char* typeNames[] = { "UDL (uniform)", "Triangular", "Point moment" };
    ImGui::Text("Add load:");
    ImGui::InputInt("Beam index", &s_beamIdx);
    s_beamIdx = std::max(0, std::min(s_beamIdx, (int)beams.size()-1));
    ImGui::Combo("Type##dlt", &s_typeIdx, typeNames, 3);
    ImGui::DragFloat3("Direction", s_dir, 0.01f, -1.0f, 1.0f, "%.2f");
    ImGui::DragFloat(s_typeIdx == 2 ? "Moment (N·m)##w" : "w (N/m)##w",
                     &s_w, 10.0f, -1e7f, 1e7f, "%.0f");
    if (s_typeIdx == 1)
        ImGui::DragFloat("w end (N/m)", &s_w2, 10.0f, -1e7f, 1e7f, "%.0f");
    if (s_typeIdx == 2)
        ImGui::SliderFloat("Position", &s_pos, 0.0f, 1.0f, "%.2f");

    if (ImGui::Button("Add Load", ImVec2(-1, 0))) {
        DistributedLoad dl;
        dl.beamIdx   = s_beamIdx;
        dl.type      = static_cast<LoadType>(s_typeIdx);
        dl.direction = glm::vec3(s_dir[0], s_dir[1], s_dir[2]);
        dl.w         = s_w;
        dl.w2        = s_w2;
        dl.pos       = s_pos;
        loads.push_back(dl);
        frameSim.setDistributedLoads(loads);
        changed = true;
    }

    // ── Load list ─────────────────────────────────────────────────────────────
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Text("Active loads (%d):", (int)loads.size());

    const char* ltNames[] = { "UDL", "Tri", "Mom" };
    int removeIdx = -1;
    for (int i = 0; i < (int)loads.size(); ++i) {
        const auto& dl = loads[i];
        char label[64];
        std::snprintf(label, sizeof label, "[%d] Beam %d  %s  %.0f N/m",
                      i, dl.beamIdx, ltNames[(int)dl.type], dl.w);
        ImGui::TextUnformatted(label);
        ImGui::SameLine();
        char btn[16]; std::snprintf(btn, sizeof btn, "X##dl%d", i);
        if (ImGui::SmallButton(btn)) removeIdx = i;
    }
    if (removeIdx >= 0) {
        loads.erase(loads.begin() + removeIdx);
        frameSim.setDistributedLoads(loads);
        changed = true;
    }

    ImGui::End();
    return changed;
}
