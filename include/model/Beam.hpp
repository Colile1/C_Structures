// Copyright (c) 2026 Colile Sibanda. All rights reserved.
// Proprietary — see LICENSE for terms. Unauthorised use prohibited.
#pragma once
#include "Node.hpp"
#include <string>
#include <vector>

// Standard structural material presets.
enum class BeamMaterial {
    STEEL     = 0,   // E = 200 GPa (structural steel)
    ALUMINUM  = 1,   // E =  70 GPa
    CONCRETE  = 2,   // E =  30 GPa (reinforced)
    TIMBER    = 3,   // E =  12 GPa (softwood, along grain)
    CUSTOM    = 4,   // user-defined E
};

inline const char* beamMaterialName(BeamMaterial m) {
    switch (m) {
        case BeamMaterial::STEEL:    return "Steel";
        case BeamMaterial::ALUMINUM: return "Aluminum";
        case BeamMaterial::CONCRETE: return "Concrete";
        case BeamMaterial::TIMBER:   return "Timber";
        default:                     return "Custom";
    }
}

// Default Young's moduli (Pa).
inline float defaultE(BeamMaterial m) {
    switch (m) {
        case BeamMaterial::STEEL:    return 200e9f;
        case BeamMaterial::ALUMINUM: return  70e9f;
        case BeamMaterial::CONCRETE: return  30e9f;
        case BeamMaterial::TIMBER:   return  12e9f;
        default:                     return 200e9f;
    }
}

// A Beam connects two nodes referenced by their indices into the model's
// node vector. Indices (not pointers) are stored so that growing or
// reallocating the node vector can never dangle a beam's endpoints, and so
// connectivity survives a CSV round-trip without float-equality matching.
class Beam {
public:
    // Construct with explicit material (sets default E for that material).
    Beam(int startIdx, int endIdx,
         BeamMaterial mat = BeamMaterial::STEEL,
         float A = 1e-4f,
         float I = 8.33e-9f);

    // Legacy constructor used by existing code / tests (E, A given directly).
    Beam(int startIdx, int endIdx, float youngsModulus, float crossSection);

    int   getStartIdx() const { return startNode; }
    int   getEndIdx()   const { return endNode;   }
    void  setStartIdx(int i)  { startNode = i; }
    void  setEndIdx(int i)    { endNode   = i; }

    // Length and axial stiffness need node positions; the owning node vector
    // is passed in rather than held, keeping Beam free of dangling references.
    float getLength(const std::vector<Node>& nodes)    const;
    float getStiffness(const std::vector<Node>& nodes) const; // AE/L

    float getYoungsModulus() const { return youngsModulus; }
    float getCrossSection()  const { return crossSection; }
    float getMomentOfInertia() const { return momentOfInertia; }
    BeamMaterial getMaterial() const { return material; }

    void setYoungsModulus(float e) { youngsModulus = e; material = BeamMaterial::CUSTOM; }
    void setCrossSection(float a)  { crossSection  = a; }
    void setMomentOfInertia(float i) { momentOfInertia = i; }

    // Apply a material preset — overwrites E to the material default.
    void setMaterial(BeamMaterial m) {
        material      = m;
        if (m != BeamMaterial::CUSTOM)
            youngsModulus = defaultE(m);
    }

private:
    int   startNode;        // index into the node vector
    int   endNode;          // index into the node vector
    float youngsModulus;    // Pa
    float crossSection;     // m²
    float momentOfInertia;  // m⁴  (for future frame-element solver)
    BeamMaterial material;
};
