// Copyright (c) 2026 Colile Sibanda. All rights reserved.
// Proprietary — see LICENSE for terms. Unauthorised use prohibited.
// data/JSONHandler.cpp : JSON project save/load (nodes, beams, distributed loads, view prefs).
#include "data/JSONHandler.hpp"
#include <nlohmann/json.hpp>
#include <fstream>

using json = nlohmann::json;

// ── Helpers ────────────────────────────────────────────────────────────────────

static json vec3ToJson(const glm::vec3& v) {
    return json::array({ v.x, v.y, v.z });
}

static glm::vec3 jsonToVec3(const json& j) {
    return { j[0].get<float>(), j[1].get<float>(), j[2].get<float>() };
}

// ── Save ───────────────────────────────────────────────────────────────────────

bool JSONHandler::saveProject(const std::string& path,
                              const std::vector<Node>& nodes,
                              const std::vector<Beam>& beams,
                              const std::vector<DistributedLoad>& distLoads,
                              const ViewPrefs& prefs) {
    json root;
    root["version"] = 1;

    // View preferences
    root["view"] = {
        { "useFrameMode",    prefs.useFrameMode    },
        { "showDiagram",     prefs.showDiagram     },
        { "diagramType",     prefs.diagramType     },
        { "beginnerMode",    prefs.beginnerMode    },
        { "showForceLabels", prefs.showForceLabels },
        { "showGlassBox",    prefs.showGlassBox    },
        { "selfWeight",      prefs.selfWeight      },
        { "showPalette",     prefs.showPalette     },
    };

    // Nodes: position, joint type, applied force, applied moment
    json nodesArr = json::array();
    for (const Node& n : nodes) {
        const glm::vec3 pos = n.getPosition();
        nodesArr.push_back({
            { "x",      pos.x                                       },
            { "y",      pos.y                                       },
            { "z",      pos.z                                       },
            { "joint",  static_cast<int>(n.getJointType())         },
            { "force",  vec3ToJson(n.getAppliedForce())            },
            { "moment", vec3ToJson(n.getAppliedMoment())           },
        });
    }
    root["nodes"] = std::move(nodesArr);

    // Beams: all fields CSV omits (I, density, material, releases)
    json beamsArr = json::array();
    for (const Beam& b : beams) {
        beamsArr.push_back({
            { "start",        b.getStartIdx()                           },
            { "end",          b.getEndIdx()                             },
            { "E",            b.getYoungsModulus()                      },
            { "A",            b.getCrossSection()                       },
            { "I",            b.getMomentOfInertia()                    },
            { "density",      b.getDensity()                            },
            { "material",     static_cast<int>(b.getMaterial())         },
            { "startRelease", b.getStartMomentRelease()                 },
            { "endRelease",   b.getEndMomentRelease()                   },
        });
    }
    root["beams"] = std::move(beamsArr);

    // Distributed loads
    json dlArr = json::array();
    for (const DistributedLoad& dl : distLoads) {
        dlArr.push_back({
            { "beamIdx",   dl.beamIdx                          },
            { "type",      static_cast<int>(dl.type)           },
            { "direction", vec3ToJson(dl.direction)            },
            { "w",         dl.w                                },
            { "w2",        dl.w2                               },
            { "pos",       dl.pos                              },
        });
    }
    root["distributedLoads"] = std::move(dlArr);

    std::ofstream file(path);
    if (!file.is_open()) return false;
    file << root.dump(2);
    return file.good();
}

// ── Load ───────────────────────────────────────────────────────────────────────

bool JSONHandler::loadProject(const std::string& path,
                              std::vector<Node>& nodes,
                              std::vector<Beam>& beams,
                              std::vector<DistributedLoad>& distLoads,
                              ViewPrefs& prefs) {
    std::ifstream file(path);
    if (!file.is_open()) return false;

    json root;
    try {
        file >> root;
    } catch (const json::exception&) {
        return false;
    }

    nodes.clear();
    beams.clear();
    distLoads.clear();
    prefs = ViewPrefs{};   // reset to defaults before applying saved values

    // View preferences (missing keys keep defaults)
    if (root.contains("view")) {
        const json& v = root["view"];
        if (v.contains("useFrameMode"))    prefs.useFrameMode    = v["useFrameMode"].get<bool>();
        if (v.contains("showDiagram"))     prefs.showDiagram     = v["showDiagram"].get<bool>();
        if (v.contains("diagramType"))     prefs.diagramType     = v["diagramType"].get<int>();
        if (v.contains("beginnerMode"))    prefs.beginnerMode    = v["beginnerMode"].get<bool>();
        if (v.contains("showForceLabels")) prefs.showForceLabels = v["showForceLabels"].get<bool>();
        if (v.contains("showGlassBox"))    prefs.showGlassBox    = v["showGlassBox"].get<bool>();
        if (v.contains("selfWeight"))      prefs.selfWeight      = v["selfWeight"].get<bool>();
        if (v.contains("showPalette"))     prefs.showPalette     = v["showPalette"].get<bool>();
    }

    // Nodes
    if (root.contains("nodes")) {
        for (const json& nj : root["nodes"]) {
            float x = nj.value("x", 0.0f);
            float y = nj.value("y", 0.0f);
            float z = nj.value("z", 0.0f);
            nodes.emplace_back(x, y, z);

            int jt = nj.value("joint", 0);
            if (jt >= 0 && jt <= 5)
                nodes.back().setJointType(static_cast<JointType>(jt));

            if (nj.contains("force") && nj["force"].is_array() && nj["force"].size() == 3)
                nodes.back().applyForce(jsonToVec3(nj["force"]));
            if (nj.contains("moment") && nj["moment"].is_array() && nj["moment"].size() == 3)
                nodes.back().applyMoment(jsonToVec3(nj["moment"]));
        }
    }

    // Beams
    if (root.contains("beams")) {
        const int n = static_cast<int>(nodes.size());
        for (const json& bj : root["beams"]) {
            int si = bj.value("start", -1);
            int ei = bj.value("end",   -1);
            if (si < 0 || ei < 0 || si >= n || ei >= n) continue;

            float E   = bj.value("E",       200e9f);
            float A   = bj.value("A",       1e-4f);
            float I   = bj.value("I",       8.33e-9f);
            float rho = bj.value("density", 7850.0f);
            int   mat = bj.value("material", static_cast<int>(BeamMaterial::STEEL));
            bool  sr  = bj.value("startRelease", false);
            bool  er  = bj.value("endRelease",   false);

            Beam beam(si, ei, E, A);
            beam.setMomentOfInertia(I);
            beam.setDensity(rho);
            // material enum sets E/density to defaults — apply after explicit values
            // so CUSTOM keeps whatever E/rho the user saved.
            if (mat >= 0 && mat <= 4) {
                BeamMaterial m = static_cast<BeamMaterial>(mat);
                if (m != BeamMaterial::CUSTOM) {
                    // preset: let setMaterial overwrite E+density from defaults
                    beam.setMaterial(m);
                    // then restore the exact saved values (user may have fine-tuned)
                    beam.setCrossSection(A);
                    beam.setMomentOfInertia(I);
                } else {
                    // CUSTOM: keep the saved E, A, I, density as-is
                    beam.setDensity(rho);
                }
            }
            beam.setStartMomentRelease(sr);
            beam.setEndMomentRelease(er);
            beams.push_back(std::move(beam));
        }
    }

    // Distributed loads
    if (root.contains("distributedLoads")) {
        for (const json& dlj : root["distributedLoads"]) {
            DistributedLoad dl;
            dl.beamIdx  = dlj.value("beamIdx", 0);
            int t       = dlj.value("type", 0);
            dl.type     = (t >= 0 && t <= 2) ? static_cast<LoadType>(t) : LoadType::UDL;
            if (dlj.contains("direction") && dlj["direction"].is_array() && dlj["direction"].size() == 3)
                dl.direction = jsonToVec3(dlj["direction"]);
            else
                dl.direction = { 0.0f, -1.0f, 0.0f };
            dl.w   = dlj.value("w",   0.0f);
            dl.w2  = dlj.value("w2",  0.0f);
            dl.pos = dlj.value("pos", 0.0f);
            distLoads.push_back(dl);
        }
    }

    return true;
}
