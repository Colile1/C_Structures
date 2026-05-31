// Copyright (c) 2026 Colile Sibanda. All rights reserved.
// Proprietary — see LICENSE for terms. Unauthorised use prohibited.
// ui/GlassBoxPanel.cpp : show the global stiffness matrix and solve summary.
#include "ui/GlassBoxPanel.hpp"
#include <imgui.h>
#include <Eigen/Dense>
#include <cstdio>
#include <cmath>

void renderGlassBoxPanel(const std::vector<Node>& nodes,
                          const std::vector<Beam>& beams,
                          const Simulator& physics,
                          bool* pOpen)
{
    if (!pOpen || !*pOpen) return;
    ImGui::SetNextWindowSize(ImVec2(560, 400), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Stiffness Matrix (K)", pOpen)) { ImGui::End(); return; }

    const int nNodes = static_cast<int>(nodes.size());

    if (nNodes == 0) {
        ImGui::TextDisabled("No nodes. Add nodes and beams first.");
        ImGui::End(); return;
    }
    if (nNodes > 6) {
        ImGui::TextColored({1.f,0.8f,0.3f,1.f},
            "Model has %d nodes (%d DOFs). Showing is practical for ≤6 nodes (18 DOFs).\n"
            "Here is a summary instead:", nNodes, 3*nNodes);
        ImGui::Spacing();
        auto disp = physics.getNodeDisplacements();
        ImGui::Text("%-8s  %-12s  %-12s  %-12s", "Node","dx(m)","dy(m)","dz(m)");
        ImGui::Separator();
        for (int i = 0; i < nNodes; ++i) {
            if (i >= (int)disp.size()) break;
            ImGui::Text("  %3d    %12.5e  %12.5e  %12.5e",
                i+1, disp[i].x, disp[i].y, disp[i].z);
        }
        ImGui::End(); return;
    }

    ImGui::TextColored({0.55f,0.85f,1.f,1.f}, "Global Stiffness Matrix K  (%d×%d)",
                       3*nNodes, 3*nNodes);
    ImGui::TextWrapped("Each row/column corresponds to a DOF: "
                       "columns labelled 'N.x N.y N.z' for each node N.");
    ImGui::Spacing();

    // Rebuild K locally for display (matches Simulator assembly).
    const int ndof = 3 * nNodes;
    Eigen::MatrixXd K = Eigen::MatrixXd::Zero(ndof, ndof);
    for (const auto& beam : beams) {
        const int si = beam.getStartIdx(), ei = beam.getEndIdx();
        if (si < 0 || ei < 0 || si >= nNodes || ei >= nNodes) continue;
        glm::vec3 axis = nodes[ei].getPosition() - nodes[si].getPosition();
        double L = glm::length(axis);
        if (L < 1e-8) continue;
        double lx = axis.x/L, ly = axis.y/L, lz = axis.z/L;
        double k = beam.getYoungsModulus() * beam.getCrossSection() / L;
        double n[3] = {lx, ly, lz};
        for (int r = 0; r < 3; ++r) for (int c = 0; c < 3; ++c) {
            double v = k * n[r] * n[c];
            K(3*si+r, 3*si+c) += v; K(3*ei+r, 3*ei+c) += v;
            K(3*si+r, 3*ei+c) -= v; K(3*ei+r, 3*si+c) -= v;
        }
    }

    // Column headers
    if (ImGui::BeginTable("##Kmat", ndof + 1,
                          ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollX |
                          ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg,
                          ImVec2(0, 200))) {
        ImGui::TableSetupScrollFreeze(1, 1);
        ImGui::TableSetupColumn("DOF", ImGuiTableColumnFlags_WidthFixed, 40.f);
        for (int j = 0; j < ndof; ++j) {
            char label[16];
            static const char* axes[] = {"x","y","z"};
            std::snprintf(label, sizeof label, "N%d.%s", j/3+1, axes[j%3]);
            ImGui::TableSetupColumn(label, ImGuiTableColumnFlags_WidthFixed, 68.f);
        }
        ImGui::TableHeadersRow();

        for (int r = 0; r < ndof; ++r) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            static const char* axes[] = {"x","y","z"};
            bool isFixed = nodes[r/3].isDOFConstrained(r%3);
            if (isFixed)
                ImGui::TextColored({1.f,0.5f,0.5f,1.f}, "N%d.%s*", r/3+1, axes[r%3]);
            else
                ImGui::Text("N%d.%s", r/3+1, axes[r%3]);

            for (int c = 0; c < ndof; ++c) {
                ImGui::TableSetColumnIndex(c + 1);
                double v = K(r, c);
                if (std::abs(v) < 1.0) {
                    ImGui::TextDisabled("   0");
                } else {
                    char buf[20]; std::snprintf(buf, sizeof buf, "%.2e", v);
                    bool diag = (r == c);
                    ImVec4 col = diag ? ImVec4(1.f,1.f,0.5f,1.f) : ImVec4(0.85f,0.85f,0.85f,1.f);
                    ImGui::TextColored(col, "%s", buf);
                }
            }
        }
        ImGui::EndTable();
    }
    ImGui::TextDisabled("* = constrained DOF (highlighted in red)  |  diagonal = yellow");

    ImGui::Spacing();
    ImGui::TextColored({0.55f,0.85f,1.f,1.f}, "Displacements u  (K·u = F)");
    ImGui::Separator();
    auto disp = physics.getNodeDisplacements();
    if (ImGui::BeginTable("##uvec", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("Node"); ImGui::TableSetupColumn("ux (m)");
        ImGui::TableSetupColumn("uy (m)"); ImGui::TableSetupColumn("uz (m)");
        ImGui::TableHeadersRow();
        for (int i = 0; i < nNodes; ++i) {
            if (i >= (int)disp.size()) break;
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::Text("%d", i+1);
            ImGui::TableSetColumnIndex(1); ImGui::Text("%.5e", disp[i].x);
            ImGui::TableSetColumnIndex(2); ImGui::Text("%.5e", disp[i].y);
            ImGui::TableSetColumnIndex(3); ImGui::Text("%.5e", disp[i].z);
        }
        ImGui::EndTable();
    }
    ImGui::End();
}
