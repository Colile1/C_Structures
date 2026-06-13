// Copyright (c) 2026 Colile Sibanda. All rights reserved.
// Proprietary — see LICENSE for terms. Unauthorised use prohibited.
// export/Exporter.hpp : PNG image capture and one-page PDF report generation.
#pragma once
#include <string>
#include <vector>

namespace Export {

struct ReactionRow {
    int   nodeIdx;
    float rx, ry, rz;
    float mx, my, mz;   // moment components — non-zero in frame mode
    bool  hasFrame;      // true → show moment columns in the report
};

struct MemberRow {
    int   memberIdx;
    float N;    // axial force (N);  positive = tension
    float Vy;   // shear force (N)   — populated in frame mode
    float Mz;   // bending moment (N·m) — populated in frame mode
};

struct ReportData {
    std::string mode;       // "Truss" or "Frame"
    int         nodeCount;
    int         beamCount;
    bool        equilibriumOK;
    float       residualMag; // magnitude of force residual (N)
    std::vector<ReactionRow> reactions;
    std::vector<MemberRow>   members;
};

// Capture the current GL framebuffer and write a PNG file.
// Returns the auto-generated filename on success, empty string on failure.
std::string savePNG(int w, int h);

// Write a one-page PDF report that embeds a viewport screenshot plus
// reactions and member-force tables.
// w, h — current GL viewport pixel dimensions.
// Returns the auto-generated filename on success, empty string on failure.
std::string savePDF(const ReportData& data, int w, int h);

} // namespace Export
