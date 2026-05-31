// Copyright (c) 2026 Colile Sibanda. All rights reserved.
// Proprietary — see LICENSE for terms. Unauthorised use prohibited.
#pragma once
#include <vector>
#include "../model/Node.hpp"
#include "../model/Beam.hpp"

// Pre-built example structures. Each clears nodes/beams and loads a
// textbook-quality example the user can solve immediately.
//
//  0 = Simple Beam     (pin left, roller right, midpoint load)
//  1 = Triangle Truss  (two fixed bases, apex under downward load)
//  2 = Portal Frame    (two fixed columns + horizontal beam, side load)
//  3 = Cantilever      (fixed wall, free tip, tip load)

void loadTemplate(int idx,
                  std::vector<Node>& nodes,
                  std::vector<Beam>& beams);
