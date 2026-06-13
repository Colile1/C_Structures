// Copyright (c) 2026 Colile Sibanda. All rights reserved.
// Proprietary — see LICENSE for terms. Unauthorised use prohibited.
#pragma once
#include <vector>
#include <string>
#include "../model/Node.hpp"
#include "../model/Beam.hpp"
#include "../physics/DistributedLoad.hpp"

// View-mode preferences persisted alongside geometry in a JSON project file.
// Mirrors the toggles in UIHandler; selfWeight is kept here because it lives
// as a local variable in main.cpp rather than inside UIHandler.
struct ViewPrefs {
    bool useFrameMode    = false;
    bool showDiagram     = true;
    int  diagramType     = 5;
    bool beginnerMode    = true;
    bool showForceLabels = true;
    bool showGlassBox    = false;
    bool selfWeight      = false;
    bool showPalette     = true;
};

class JSONHandler {
public:
    // Save full project: nodes, beams, distributed loads, and view preferences.
    // Returns true on success; false if the file could not be opened for writing.
    static bool saveProject(const std::string& path,
                            const std::vector<Node>& nodes,
                            const std::vector<Beam>& beams,
                            const std::vector<DistributedLoad>& distLoads,
                            const ViewPrefs& prefs);

    // Load full project into the supplied vectors (cleared before population).
    // prefs is populated from the "view" object; missing keys take default values.
    // Returns true on success; false if the file cannot be opened or parsed.
    static bool loadProject(const std::string& path,
                            std::vector<Node>& nodes,
                            std::vector<Beam>& beams,
                            std::vector<DistributedLoad>& distLoads,
                            ViewPrefs& prefs);
};
