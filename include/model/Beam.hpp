// Copyright (c) 2026 Colile Sibanda. All rights reserved.
// Proprietary — see LICENSE for terms. Unauthorised use prohibited.
#pragma once
#include "Node.hpp"
#include <string>

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

class Beam {
public:
    // Construct with explicit material (sets default E for that material).
    Beam(Node* start, Node* end,
         BeamMaterial mat = BeamMaterial::STEEL,
         float A = 1e-4f,
         float I = 8.33e-9f);

    // Legacy constructor used by existing code / tests (E, A given directly).
    Beam(Node* start, Node* end, float youngsModulus, float crossSection);

    Node* getStart() const;
    Node* getEnd()   const;
    float getLength()    const;
    float getStiffness() const; // AE/L

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
    Node* startNode;
    Node* endNode;
    float youngsModulus;    // Pa
    float crossSection;     // m²
    float momentOfInertia;  // m⁴  (for future frame-element solver)
    BeamMaterial material;
};
