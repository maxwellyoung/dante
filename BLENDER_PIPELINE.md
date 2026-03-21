# Blender Pipeline: 10x Quality System

> "Every model in Endearing Void exists because a box couldn't say it."

## The Problem This Solves

Before: 8 scripts, each with its own `mat()` helper, ad-hoc RGB values, no validation, no shared form language. Models were correct but inconsistent — a bed's fabric white was `(245, 242, 235)` in one script and undefined in another.

After: One shared toolkit (`ev_model_kit.py`) that enforces locked materials, validates tri budgets, checks scale, and outputs engine placement code. Every model speaks the same visual language.

## Quick Start

```python
# Every prop model script starts the same way:
import sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from ev_model_kit import *

kit = EVModelKit()
kit.begin("ModelName", tri_budget=200)

# Build with kit primitives + kit materials (NEVER define your own)
kit.rounded_cube("Seat", (0, 0.38, 0), (0.5, 0.08, 0.5),
                 material=LEATHER_NAVY, subdivisions=1)
kit.cylinder("Leg", (0.2, 0.175, 0.2),
             radius=0.02, depth=0.35, material=WOOD_DARK)

# Export runs validation automatically
kit.export_glb("/Users/klaus/model_name.glb")
kit.preview_placement()  # prints C code for scene placement
```

Runtime note: the engine is effectively `GLB`-first now. `OBJ` remains useful for legacy/debug assets, but production props and shells should export as `GLB` so they play nicely with the current Raylib/macOS VAO limits and multi-material rendering path.

## The 12 Rules of Form

| # | Rule | Enforcement |
|---|------|-------------|
| 1 | **Silhouette First** | Design in wireframe. If the outline doesn't read, no amount of detail fixes it. |
| 2 | **3-Pixel Rule** | Nothing smaller than 3px at 960x600. Kit warns on dimensions < 0.05m. |
| 3 | **Subdivision = Emotion** | Only soften surfaces the player stares at or touches. Legs, frames, rails stay sharp. |
| 4 | **Material Honesty** | Use kit materials only. `WOOD_DARK` not "brownish color." Kit blocks non-EV_ materials on export. |
| 5 | **Warm Neutrals, One Accent** | Base: cream/navy/wood/brass. Max ONE accent (Godard red, Earth blue). Kit counts accents. |
| 6 | **Scale Is Sacred** | 1 unit = 1 meter. Reference `SCALE_REF` dict. Kit warns on outlier dimensions. |
| 7 | **Join Everything** | One object per model, multi-material via slots. `kit.join_all()` handles this. |
| 8 | **Blender Colors Carry** | Authored `GLB` assets should preserve Blender material color intent; `OBJ` is fallback-only. |
| 9 | **Y-Up Export** | GLB exports Y-up automatically. OBJ rotated in engine. |
| 10 | **Origin at Floor Center** | `kit.join_all()` sets origin to bottom-center automatically. |
| 11 | **No Hidden Geometry** | Kit checks for loose vertices. No internal faces. |
| 12 | **Less Is More** | If a box says it better, keep the box. |

## Material Palette (Locked)

### Architectural
| Kit Constant | RGB | Engine ID | Use |
|-------------|-----|-----------|-----|
| `CONCRETE` | 180,175,168 | MAT_CONCRETE | Walls, floors |
| `MARBLE_WHITE` | 240,238,232 | MAT_MARBLE | Lobby floors |
| `WOOD_DARK` | 105,78,48 | MAT_WOOD | Furniture frames |
| `WOOD_LIGHT` | 165,130,90 | MAT_WOOD | Light accents |
| `TILE_WHITE` | 235,232,225 | MAT_TILE | Bathrooms |

### Metal
| Kit Constant | RGB | Engine ID | Use |
|-------------|-----|-----------|-----|
| `BRASS` | 184,157,107 | MAT_BRASS | Fixtures, buttons |
| `BRASS_DULL` | 160,140,95 | MAT_BRASS | Aged, worn |
| `BRASS_GOLD` | 210,180,120 | MAT_BRASS | Polished accents |

### Soft
| Kit Constant | RGB | Engine ID | Use |
|-------------|-----|-----------|-----|
| `LEATHER_NAVY` | 25,25,80 | MAT_LEATHER | Chairs, upholstery |
| `LEATHER_TAN` | 140,110,75 | MAT_LEATHER | Suitcases |
| `FABRIC_WHITE` | 245,242,235 | MAT_FABRIC | Bedsheets |
| `FABRIC_CREAM` | 215,210,200 | MAT_FABRIC | Duvet |
| `VELVET_NAVY` | 25,25,80 | MAT_VELVET | Headboard, luxury |

### Accent (MAX ONE per model)
| Kit Constant | RGB | Engine ID | Use |
|-------------|-----|-----------|-----|
| `GODARD_RED` | 204,20,13 | MAT_FABRIC | THE red |
| `EARTH_BLUE` | 51,102,191 | MAT_GLASS | Earth glow |

## Form Decision Tree

```
Does the object need CURVES that boxes can't do?
├── NO → Keep it procedural (add_wall, add_cylinder in scene.c)
└── YES → Does the player stare at it for > 5 seconds?
    ├── YES → Model it. Subdivide organic surfaces. (bed, chairs, bathtub)
    └── NO → Model it, but keep it sharp. No subdivision. (telephone, suitcase)
```

## Subdivision Guide

| Surface Type | Subdiv Level | Examples |
|-------------|-------------|---------|
| Structural (legs, frames, rails) | 0 | Chair legs, bed frame, table |
| Manufactured (panels, doors) | 0 or bevel | Headboard trim, brass cap |
| Semi-soft (cushion edges, armrests) | 1 | Armrest pad, seat edge |
| Soft (pillows, duvet, upholstery) | 2 | Pillows, duvet fold |
| Organic (skin-adjacent) | 2-3 | Gibbons head (but SSS does the work) |

## Validation Gates

`kit.export_glb()` runs these checks automatically:

1. **Tri count** — Must be under `tri_budget`. Blocks export if over.
2. **Scale check** — Warns if largest dimension > 5m or < 0.05m.
3. **Material compliance** — All materials must use `EV_` prefix (kit materials only).
4. **Accent count** — Warns if more than 1 accent material used.
5. **Loose vertices** — Checks for hidden/orphaned geometry.

If validation fails, `kit.export_glb()` is blocked. `OBJ` export still exists for legacy/debug workflows, but it is no longer the main runtime target.

## Full Pipeline

```
1. Write prop script using `ev_model_kit`
2. Send to Blender:     `python3 blender_send.py model_foo.py`
3. Validate + export:   `kit.export_glb("~/foo.glb")`
4. Fetch to engine:     `scp mini-ts:~/foo.glb ev_engine/assets/`
5. Register it:         add a `ModelRegistryEntry` in `src/model_registry.c`
6. Place in scene:      `find_model_asset("foo") + add_model()` or `add_shell()`
7. Visual QA:           `./scripts/glb_qa.sh assets/foo.glb`
8. Registry gate:       `python3 scripts/validate_model_registry.py`
```

`./scripts/mcp_model.sh full model_foo.py foo` now runs that sequence end to end: Blender build, Blender-side validation renders, GLB export, deploy to `assets/`, GLB visual QA, then repo registry validation. It prefers local headless Blender when available, and you can force the Mini-first path with `PREFER_REMOTE_BLENDER=1`.

## Environments vs Props

There are two distinct 3D paths:

- Props and characters: use `ev_model_kit.py`, `ev_suite_workbench.py`, and `mcp_model.sh`.
- Environment shells: use `ev_shell_workbench.py`, export a shell `GLB`, then pair it with explicit collision volumes via `add_shell()` + `add_collision_wall()`.

For shell acceptance, use `make qa-shell`. That pass isolates the production elevator shell and captures doorway, corner, floor, ceiling, and panel views so shell/collision alignment can be checked without rerunning the full 17-scene sweep.

That split is deliberate. The engine is already strong at procedural architecture, so full room shells should be reserved for spaces where silhouette and spatial weirdness are the point, not as a blanket replacement for every room.

## Pixar's "Form" Applied to 960x600

Pixar's principle: every surface should communicate what the object IS and what it's MADE OF through shape alone, before lighting or color.

At 960x600, this means:
- **Mass** — Heavy things have wide bases (bathtub, armchair). Light things taper (champagne flute, telescope).
- **Age** — New things have sharp edges (brass cap). Old things have soft edges (worn leather, rounded wood).
- **Warmth** — Inviting things curve toward you (chair seat, pillow). Cold things are flat or angular (tile, glass).
- **Use** — Things that are used show it (seat indent, pillow dent, duvet fold). Unused things are perfect.

These read in silhouette. You don't need texture — the procedural shader handles that. You need SHAPE.
