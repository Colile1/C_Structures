// Copyright (c) 2026 Colile Sibanda. All rights reserved.
// Proprietary — see LICENSE for terms. Unauthorised use prohibited.
//
// wasm/SolverBindings.cpp : Emscripten/Embind facade over the pure solver core
// (Simulator, FrameSimulator, FrameElement, MemberForces, DistributedLoad) so the
// exact native analysis can run in a browser or under Node. No physics lives
// here — this is a thin builder/readback wrapper that owns the node/beam vectors
// and reuses the existing solver classes verbatim (they take std::vector&).

#include <emscripten/bind.h>

#include <memory>
#include <string>
#include <vector>

#include "model/Node.hpp"
#include "model/Beam.hpp"
#include "physics/Simulator.hpp"
#include "physics/FrameSimulator.hpp"
#include "physics/DistributedLoad.hpp"
#include "physics/MemberForces.hpp"
#include "physics/SolveResult.hpp"

// Plain value type returned to JS for any 3-vector (force, moment, displacement).
// A dedicated struct avoids taking pointer-to-member of glm::vec3's anonymous
// union, which is not portable across glm configurations.
struct Vec3 {
    double x = 0.0, y = 0.0, z = 0.0;
};
static Vec3 toVec3(const glm::vec3& v) { return {v.x, v.y, v.z}; }

// Model
// Purpose: a scene the JS caller builds incrementally (nodes, beams, loads) and
//          then solves as a truss or frame, reading results back as Vec3s and
//          internal-force diagrams.
// Inputs:  builder calls (addNode/addBeam/applyForce/…); solveTruss()/solveFrame().
// Output:  displacement/rotation/reaction getters and per-member diagrams that
//          match the native solver bit-for-bit (same code path).
class Model {
public:
    // ── Builders ────────────────────────────────────────────────────────────
    int addNode(double x, double y, double z, JointType jt) {
        m_nodes.emplace_back(static_cast<float>(x), static_cast<float>(y),
                             static_cast<float>(z));
        m_nodes.back().setJointType(jt);
        return static_cast<int>(m_nodes.size()) - 1;
    }

    void applyForce(int nodeIdx, double fx, double fy, double fz) {
        m_nodes.at(nodeIdx).applyForce(
            glm::vec3((float)fx, (float)fy, (float)fz));
    }

    void applyMoment(int nodeIdx, double mx, double my, double mz) {
        m_nodes.at(nodeIdx).applyMoment(
            glm::vec3((float)mx, (float)my, (float)mz));
    }

    int addBeam(int startIdx, int endIdx, double E, double A, double I) {
        Beam beam(startIdx, endIdx, (float)E, (float)A);
        beam.setMomentOfInertia((float)I);
        m_beams.push_back(beam);
        return static_cast<int>(m_beams.size()) - 1;
    }

    void setMomentRelease(int beamIdx, bool atStart, bool atEnd) {
        m_beams.at(beamIdx).setStartMomentRelease(atStart);
        m_beams.at(beamIdx).setEndMomentRelease(atEnd);
    }

    // beamIdx-targeted span load. dir is the global unit direction; w (and w2 for
    // triangular) the intensity (N/m), or N·m for a MOMENT load at fraction pos.
    void addDistributedLoad(int beamIdx, LoadType type, double dx, double dy,
                            double dz, double w, double w2, double pos) {
        DistributedLoad dl;
        dl.beamIdx   = beamIdx;
        dl.type      = type;
        dl.direction = glm::vec3((float)dx, (float)dy, (float)dz);
        dl.w         = (float)w;
        dl.w2        = (float)w2;
        dl.pos       = (float)pos;
        m_loads.push_back(dl);
    }

    void setSelfWeight(bool on) { m_selfWeight = on; }

    // ── Solve ───────────────────────────────────────────────────────────────
    SolveStatus solveTruss() {
        m_frame.reset();
        m_truss = std::make_unique<Simulator>(m_nodes, m_beams);
        SolveResult r = m_truss->solveStaticForces();
        m_lastMessage = r.message;
        return r.status;
    }

    SolveStatus solveFrame() {
        m_truss.reset();
        m_frame = std::make_unique<FrameSimulator>(m_nodes, m_beams);
        m_frame->setDistributedLoads(m_loads);
        m_frame->setSelfWeight(m_selfWeight);
        SolveResult r = m_frame->solve();
        m_lastMessage = r.message;
        return r.status;
    }

    std::string lastMessage() const { return m_lastMessage; }

    // ── Readback ──────────────────────────────────────────────────────────────
    // Translation (m): truss displacement or frame translation, whichever solved.
    Vec3 displacement(int nodeIdx) const {
        if (m_frame) return toVec3(m_frame->getNodeTranslations().at(nodeIdx));
        if (m_truss) return toVec3(m_truss->getNodeDisplacements().at(nodeIdx));
        return {};
    }

    // Rotation (rad) about global axes — frame solve only (zero for a truss).
    Vec3 rotation(int nodeIdx) const {
        if (m_frame) return toVec3(m_frame->getNodeRotations().at(nodeIdx));
        return {};
    }

    Vec3 reactionForce(int nodeIdx) const {
        if (m_frame) return toVec3(m_frame->getNodeReactionForce(nodeIdx));
        if (m_truss) return toVec3(m_truss->getNodeReaction(nodeIdx));
        return {};
    }

    // Reaction moment (N·m) — frame solve only.
    Vec3 reactionMoment(int nodeIdx) const {
        if (m_frame) return toVec3(m_frame->getNodeReactionMoment(nodeIdx));
        return {};
    }

    // Signed axial force in a member (tension +) — truss solve.
    double beamAxialForce(int beamIdx) const {
        if (m_truss) return m_truss->getBeamForce(m_beams.at(beamIdx));
        return 0.0;
    }

    // Internal-force diagram sampled along a member (frame solve): nSamples
    // stations from the start node to the end node, in the member's local frame.
    std::vector<InternalForces> memberDiagram(int beamIdx, int nSamples) const {
        if (!m_frame) return {};
        const Beam& beam = m_beams.at(beamIdx);
        const float L    = beam.getLength(m_nodes);
        auto p           = m_frame->getMemberEndForces(beam);
        SpanLoad load    = m_frame->getMemberSpanLoad(beam);
        return sampleMember(p, L, nSamples, load);
    }

private:
    std::vector<Node>            m_nodes;
    std::vector<Beam>            m_beams;
    std::vector<DistributedLoad> m_loads;
    bool                         m_selfWeight = false;
    std::string                  m_lastMessage;

    // Constructed at solve time; reference m_nodes/m_beams, whose addresses are
    // stable for the lifetime of this Model, so the held pointers stay valid.
    std::unique_ptr<Simulator>      m_truss;
    std::unique_ptr<FrameSimulator> m_frame;
};

// ── Embind registration ───────────────────────────────────────────────────────
using namespace emscripten;

EMSCRIPTEN_BINDINGS(c_structures_solver) {
    value_object<Vec3>("Vec3")
        .field("x", &Vec3::x)
        .field("y", &Vec3::y)
        .field("z", &Vec3::z);

    value_object<InternalForces>("InternalForces")
        .field("N",  &InternalForces::N)
        .field("Vy", &InternalForces::Vy)
        .field("Vz", &InternalForces::Vz)
        .field("T",  &InternalForces::T)
        .field("My", &InternalForces::My)
        .field("Mz", &InternalForces::Mz);

    register_vector<InternalForces>("VectorInternalForces");

    enum_<JointType>("JointType")
        .value("FREE",     JointType::FREE)
        .value("FIXED",    JointType::FIXED)
        .value("PIN_XY",   JointType::PIN_XY)
        .value("ROLLER_X", JointType::ROLLER_X)
        .value("ROLLER_Y", JointType::ROLLER_Y)
        .value("ROLLER_Z", JointType::ROLLER_Z);

    enum_<LoadType>("LoadType")
        .value("UDL",        LoadType::UDL)
        .value("TRIANGULAR", LoadType::TRIANGULAR)
        .value("MOMENT",     LoadType::MOMENT);

    enum_<SolveStatus>("SolveStatus")
        .value("OK",        SolveStatus::OK)
        .value("MECHANISM", SolveStatus::MECHANISM)
        .value("FAILED",    SolveStatus::FAILED);

    class_<Model>("Model")
        .constructor<>()
        .function("addNode",            &Model::addNode)
        .function("applyForce",         &Model::applyForce)
        .function("applyMoment",        &Model::applyMoment)
        .function("addBeam",            &Model::addBeam)
        .function("setMomentRelease",   &Model::setMomentRelease)
        .function("addDistributedLoad", &Model::addDistributedLoad)
        .function("setSelfWeight",      &Model::setSelfWeight)
        .function("solveTruss",         &Model::solveTruss)
        .function("solveFrame",         &Model::solveFrame)
        .function("lastMessage",        &Model::lastMessage)
        .function("displacement",       &Model::displacement)
        .function("rotation",           &Model::rotation)
        .function("reactionForce",      &Model::reactionForce)
        .function("reactionMoment",     &Model::reactionMoment)
        .function("beamAxialForce",     &Model::beamAxialForce)
        .function("memberDiagram",      &Model::memberDiagram);
}
