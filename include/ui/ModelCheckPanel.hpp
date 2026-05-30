// Copyright (c) 2026 Colile Sibanda. All rights reserved.
// Proprietary — see LICENSE for terms. Unauthorised use prohibited.
#pragma once
#include <vector>
#include "../model/Node.hpp"
#include "../model/Beam.hpp"

// ui/ModelCheckPanel.hpp : Dear ImGui window showing the determinacy/stability
// pre-check (counts + plain-language verdict). Display-only.
void renderModelCheckPanel(const std::vector<Node>& nodes,
                           const std::vector<Beam>& beams);
