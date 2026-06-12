// Copyright (c) 2026 Colile Sibanda. All rights reserved.
// Proprietary — see LICENSE for terms. Unauthorised use prohibited.
// ui/ModelCheckPanel.cpp : renders the determinacy/stability verdict.
#include "ui/ModelCheckPanel.hpp"
#include "physics/Determinacy.hpp"
#include <imgui.h>

// renderModelCheckPanel
// Purpose: draw a "Model Check" window with the determinacy counts and a
//          colour-coded plain-language verdict before/while solving.
// Inputs:  nodes, beams — the scene; frame — true when the 6-DOF solver is active.
// Output:  none (draws into the current ImGui frame).
void renderModelCheckPanel(const std::vector<Node>& nodes,
                           const std::vector<Beam>& beams,
                           bool frame) {
    const DeterminacyResult d = analyzeDeterminacy(nodes, beams, frame);

    ImGui::Begin("Model Check");
    ImGui::Text("Members m = %d   Reactions r = %d   Nodes n = %d",
                d.members, d.reactions, d.nodes);
    if (d.frame)
        ImGui::Text("6m + r = %d   vs   6n = %d", d.memberUnknowns + d.reactions, d.dof);
    else
        ImGui::Text("m + r = %d   vs   3n = %d", d.memberUnknowns + d.reactions, d.dof);
    ImGui::Separator();

    ImVec4 colour;
    switch (d.stability) {
        case Stability::DETERMINATE:   colour = ImVec4(0.25f, 0.85f, 0.35f, 1.0f); break;
        case Stability::INDETERMINATE: colour = ImVec4(0.95f, 0.80f, 0.20f, 1.0f); break;
        default:                       colour = ImVec4(0.95f, 0.35f, 0.25f, 1.0f); break;
    }
    ImGui::PushTextWrapPos(0.0f);
    ImGui::TextColored(colour, "%s", d.message.c_str());
    ImGui::PopTextWrapPos();
    ImGui::End();
}
