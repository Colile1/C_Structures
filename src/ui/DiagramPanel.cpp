// Copyright (c) 2026 Colile Sibanda. All rights reserved.
// Proprietary — see LICENSE for terms. Unauthorised use prohibited.
// ui/DiagramPanel.cpp : annotate the overlaid internal-force diagram with each
// member's end values and signed peak+location, plus the sign convention.
#include "ui/DiagramPanel.hpp"
#include "physics/MemberForces.hpp"
#include <imgui.h>
#include <cmath>

namespace {
// Per-quantity display metadata, indexed by diagramType 0..5 (the UI switch order).
struct QuantityInfo {
    const char* name;        // full label
    const char* units;       // display units after scaling SI by toDisplay
    const char* convention;  // one-line sign convention
    float       toDisplay;   // multiply the SI value by this for display
};

const QuantityInfo kQuantities[6] = {
    {"Axial N",   "kN",   "tension +, compression -",        1e-3f},
    {"Shear Vy",  "kN",   "local-y shear (left free body)",  1e-3f},
    {"Shear Vz",  "kN",   "local-z shear (left free body)",  1e-3f},
    {"Torsion T", "kN.m", "torque about the member axis",    1e-3f},
    {"Moment My", "kN.m", "bending about local y",           1e-3f},
    {"Moment Mz", "kN.m", "sagging +, hogging -",            1e-3f},
};
} // namespace

void renderDiagramPanel(const std::vector<Node>& nodes,
                        const std::vector<Beam>& beams,
                        const FrameSimulator& frameSim,
                        int diagramType)
{
    if (beams.empty() || diagramType < 0 || diagramType > 5) return;
    const QuantityInfo&   q    = kQuantities[diagramType];
    const DiagramComponent comp = static_cast<DiagramComponent>(diagramType);

    ImGui::SetNextWindowSize(ImVec2(340, 0), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowCollapsed(true, ImGuiCond_FirstUseEver);
    ImGui::Begin("Diagram");

    ImGui::TextColored({0.55f,0.85f,1.0f,1.0f}, "%s  [%s]", q.name, q.units);
    ImGui::TextDisabled("Convention: %s", q.convention);
    ImGui::Separator();

    // Headline peak across every member, found while filling the table.
    float globalPeak = 0.0f; int globalMember = -1; float globalX = 0.0f;

    if (ImGui::BeginTable("##diag", 4,
                          ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("Mbr");
        ImGui::TableSetupColumn("Start");
        ImGui::TableSetupColumn("Peak @ x(m)");
        ImGui::TableSetupColumn("End");
        ImGui::TableHeadersRow();

        for (int i = 0; i < (int)beams.size(); ++i) {
            float L  = beams[i].getLength(nodes);
            auto  ef = frameSim.getMemberEndForces(beams[i]);
            auto  sl = frameSim.getMemberSpanLoad(beams[i]);
            auto  pts = sampleMember(ef, L, 33, sl);
            DiagramStats s = diagramStats(pts, comp, L);

            if (std::abs(s.peakVal) > std::abs(globalPeak)) {
                globalPeak = s.peakVal; globalMember = i; globalX = s.peakX;
            }

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::Text("%d", i + 1);
            ImGui::TableSetColumnIndex(1); ImGui::Text("%.2f", s.startVal * q.toDisplay);
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%.2f @ %.2f", s.peakVal * q.toDisplay, s.peakX);
            ImGui::TableSetColumnIndex(3); ImGui::Text("%.2f", s.endVal * q.toDisplay);
        }
        ImGui::EndTable();
    }

    ImGui::Spacing();
    if (globalMember >= 0 && std::abs(globalPeak) > 1e-6f) {
        ImGui::TextColored({1.0f,0.95f,0.4f,1.0f},
            "Max |%s| = %.2f %s  (member %d @ %.2f m)",
            q.name, std::abs(globalPeak) * q.toDisplay, q.units,
            globalMember + 1, globalX);
    } else {
        ImGui::TextDisabled("No significant %s in this model.", q.name);
    }

    ImGui::End();
}
