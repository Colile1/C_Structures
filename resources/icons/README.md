# C_Structures — Icon Library

A coordinated set of icons for the structural-analysis UI. Every component is drawn in
three matched **view modes** so the user can pick whichever reads best for them:

| Folder | View mode | Audience | Style |
|--------|-----------|----------|-------|
| `symbols/` | Traditional 2D schematic | Engineers / students | Standard civil-engineering line symbols (supports with hatching, section outlines, load arrows). Matches what appears in textbooks and on drawings. |
| `realistic/2d/` | Shaded flat elevation | Non-technical users | Material-shaded side views with gradients and depth — reads like a photo of the real component, but flat. |
| `realistic/3d/` | Isometric render | Non-technical users | Pseudo-3D isometric renders with light/shadow faces and ground shadow, for an at-a-glance "what is this physically" feel. |

All icons are **SVG** (`viewBox 0 0 128 128`): resolution-independent, themeable, and tiny.
File names are identical across the three trees, so switching view mode is just a path-prefix
swap (see `index.html`).

## Browse them

Open **`index.html`** in any browser. It shows every icon grouped by category with a
Symbols / Realistic-2D / Realistic-3D toggle and a name filter. No build step or server needed.

## Catalogue

### Joints & supports (`joints/`) — 8
`free`, `fixed` (encastré), `pin_xy` (pinned), `roller_x`, `roller_y`, `roller_z`,
`internal_hinge`, `rigid` (moment connection).

The first six map **1:1** to the `JointType` enum in `include/model/Node.hpp`
(`FREE, FIXED, PIN_XY, ROLLER_X, ROLLER_Y, ROLLER_Z`). `internal_hinge` and `rigid`
are standard connection types reserved for a future frame-element solver.

### Beam sections & types (`beams/`) — 15
**Cross-sections:** `i_beam`, `h_column`, `channel_c`, `angle_l`, `t_section`,
`box_rhs`, `pipe_chs`, `solid_rect`, `solid_round`.
**Structural types:** `simply_supported`, `cantilever`, `fixed_both`, `continuous`,
`overhanging`, `truss`.

### Forces & force types (`forces/`) — 9
`point_load`, `udl` (uniformly distributed), `triangular_load`, `moment` (couple),
`tension`, `compression`, `shear`, `reaction`, `self_weight`.

Colour convention follows the renderer: **blue = tension**, **red = compression / applied load**,
**purple = moment**, slate = self-weight/reaction.

## Using an icon in the app

These SVGs are intended for the (planned) ImGui/HTML side panels and palette. For ImGui,
rasterise to PNG at the needed size (e.g. 32/48 px) and load as a texture; for any web/HTML
overlay, reference the SVG directly. A `manifest.json` lists every component and its human-readable
title for programmatic loading.

## Regenerating

The whole set is produced by a single deterministic generator (kept out of the repo by default).
Re-running it overwrites all three trees so the variants always stay in sync. Edit the palette
constants or drawing functions to restyle the entire library at once.

© 2026 Colile Sibanda. Proprietary — see project `LICENSE`.
