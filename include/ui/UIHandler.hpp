// Copyright (c) 2026 Colile Sibanda. All rights reserved.
// Proprietary — see LICENSE for terms. Unauthorised use prohibited.
#pragma once
#include <SDL2/SDL.h>
#include <glm/glm.hpp>
#include <vector>
#include <deque>
#include "../model/Node.hpp"
#include "../model/Beam.hpp"

enum class ToolMode { SELECT, NODE_PLACEMENT, BEAM_CREATION, FORCE_APPLICATION };

// ── Snapshot-based undo/redo ──────────────────────────────────────────────────
struct NodeState {
    float x, y, z;
    JointType joint;
    float fx, fy, fz;
};
struct BeamState {
    int   iStart, iEnd;          // indices into node list
    float E, A, I;
    BeamMaterial material;
};
struct SceneSnapshot {
    std::vector<NodeState> nodes;
    std::vector<BeamState> beams;
};

class UIHandler {
public:
    UIHandler()  = default;
    ~UIHandler() = default;

    void initialize(int screenW, int screenH);

    void handleEvent(SDL_Event& e,
                     std::vector<Node>& nodes,
                     std::vector<Beam>& beams,
                     const glm::mat4& view,
                     const glm::mat4& proj);

    void renderUI(SDL_Window* window,
                  std::vector<Node>& nodes,
                  std::vector<Beam>& beams,
                  float& dispScale);

    ToolMode  getCurrentTool()  const { return currentTool; }
    glm::vec3 getForceVector()  const { return forceVector; }
    bool      getShowForceLabels() const { return showForceLabels; }
    bool consumeNeedsSolve() { bool v = needsSolveFlag; needsSolveFlag = false; return v; }

    glm::vec3 currentMouseWorldPos = {};

    // ── Undo/redo ─────────────────────────────────────────────────────────────
    void pushSnapshot(const std::vector<Node>& nodes, const std::vector<Beam>& beams);
    bool canUndo() const { return !m_undoStack.empty(); }
    bool canRedo() const { return !m_redoStack.empty(); }
    void undo(std::vector<Node>& nodes, std::vector<Beam>& beams);
    void redo(std::vector<Node>& nodes, std::vector<Beam>& beams);

private:
    // ── Tool / interaction state ──────────────────────────────────────────────
    // Selection is held by index into the node/beam vectors (not pointers) so
    // it can never dangle when those vectors grow or are rebuilt. -1 = none.
    ToolMode  currentTool   = ToolMode::NODE_PLACEMENT;
    int       selectedNode  = -1;
    int       selectedBeam  = -1;
    int       beamStart     = -1;
    bool      draggingNode  = false;
    bool      needsSolveFlag = false;
    bool      showForceLabels = true; // draw per-member force values in 3D view

    glm::vec3 forceVector = {0.0f, -1000.0f, 0.0f};
    float forceMagX =  0.0f;
    float forceMagY = -1000.0f;
    float forceMagZ =  0.0f;

    int screenWidth  = 800;
    int screenHeight = 600;

    // ── Undo stacks ───────────────────────────────────────────────────────────
    static constexpr int MAX_UNDO = 50;
    std::deque<SceneSnapshot> m_undoStack;
    std::deque<SceneSnapshot> m_redoStack;

    // ── Helpers ───────────────────────────────────────────────────────────────
    // Both return an index into the respective vector, or -1 if nothing is hit.
    int       findNodeUnderCursor(const glm::vec3& w, const std::vector<Node>& nodes) const;
    int       findBeamUnderCursor(int mx, int my,
                                  const std::vector<Node>& nodes,
                                  const std::vector<Beam>& beams,
                                  const glm::mat4& view,
                                  const glm::mat4& proj) const;
    glm::vec3 screenToWorld(int mx, int my,
                            const glm::mat4& view,
                            const glm::mat4& proj) const;

    void applySnapshot(const SceneSnapshot& s,
                       std::vector<Node>& nodes,
                       std::vector<Beam>& beams);

    // Project world position to screen pixel (returns {sx, sy, 0}).
    glm::vec3 worldToScreen(const glm::vec3& w,
                            const glm::mat4& view,
                            const glm::mat4& proj) const;
};
