# CLAUDE.md - Endearing Void Engine

## Philosophy
> "Three hours to kill." — Auckland at 2 AM. A taxi. The Sky Tower. An elevator to orbit. A luxury hotel that has no business being in space. Built in pure C.

## The Endearing Void Aesthetic
- **Palette:** Neutral warm whites + specific Pantone accents (French red, French blue, brass)
- **Rendering:** 480x300 lo-fi, PS1 point filtering, Raylib
- **References:** Godard (Contempt, Pierrot le Fou), Hotel Chevalier, Bioshock Infinite opening, Mirror's Edge
- **Emotion:** Wonder and melancholy, not horror. Arrival, not escape.

## Motion (adapted from Studio Manifesto)
- Physics-first: camera kick with quadratic decay, spring-like FOV transitions
- Head bob amplitude scales with speed (0.04 walk, 0.08 sprint)
- View tilt on strafe (1.5° walk, 2.5° sprint) — Mirror's Edge lean
- Smooth acceleration/deceleration on all movement (no instant start/stop)
- Never exceed 300ms for any visual feedback

## Tech Stack

- **Language:** C99
- **Graphics:** Raylib 5.5 + custom GLSL 330 shaders
- **Audio:** Procedural (all sounds generated from code, no external files)
- **Build:** Make (clang, -Wall -Wextra -std=c99 -O2)
- **Platform:** macOS (Apple Silicon), targeting cross-platform later

## Commands

```bash
make            # Build (incremental)
make clean      # Clean object files
make run        # Build and run
./endearing_void  # Run directly
```

## Key Files

```
src/
├── main.c       # Game loop, state machine, input routing, vignette rendering
├── ev_types.h   # Shared types: Wall, InteractObject, Player, Scene, GameState
├── player.c/h   # Movement, collision, camera (Mirror's Edge: sprint, tilt, momentum)
├── scene.c/h    # Scene construction (all geometry in code), procedural textures
├── render.c/h   # 3D drawing, post-FX shader (chromatic aberration, fake AO, saturation)
├── audio.c/h    # Procedural audio (footsteps per surface, ambient per space, chimes)
└── lighting.c/h # Two-light GLSL shader (key + fill + rim + fog)
```

## Game Flow

```
TITLE → CAR (Auckland taxi) → DRIVING (arrival) → HOTEL_EXT (Sky Tower)
→ LOBBY (terminal) → ELEVATOR (ascent to orbit)
→ SPACE_LOBBY (observation window) → SPACE_CORRIDOR (portholes)
→ SPACE_SUITE (4 tasks: lamp, desk, champagne, bed)
→ BALCONY (Earth view) → BED (phosphorescent stars) → STARS (credits)

Orphaned (dev keys only): HALLWAY, BATHROOM, ROOM (Paris hotel)
```

## Debug Keys

| Key | Action |
|-----|--------|
| F1 | Wireframe toggle |
| F3 | Debug overlay (FPS, walls, position, state, key hint) |
| F4 | Noclip (fly through walls, Space=up, Ctrl=down) |
| 0 | Jump to: Taxi |
| 1 | Jump to: Hotel Exterior |
| 2 | Jump to: Lobby |
| 3 | Jump to: Hallway (orphaned) |
| 4 | Jump to: Room (Paris, orphaned) |
| 5 | Jump to: Balcony |
| 6 | Jump to: Bathroom (orphaned) |
| 7 | Jump to: Space Lobby |
| 8 | Jump to: Space Corridor |
| 9 | Jump to: Space Suite |

## Visual Style Presets (Shift+Number)

| Key | Style | Character |
|-----|-------|-----------|
| Shift+1 | Default | 16mm Godard, warm neutral, architectural |
| Shift+2 | Noir | Desaturated, heavy contrast, deep shadows |
| Shift+3 | Godard | Saturated reds, cool blues, contrasty (Contempt) |
| Shift+4 | Polaroid | Faded, warm, low contrast, soft |
| Shift+5 | Kubrick | Cold, clinical, sharp, low grain |
| Shift+6 | VHS | Heavy grain, heavy CA, warm/muddy |
| Shift+7 | Moonlight | Blue-shifted, desaturated, dreamlike |
| Shift+8 | Bleach Bypass | Desaturated + high contrast (Se7en) |
| Shift+9 | Raw | No post-FX, geometry and lighting only |

Styles persist across scene changes. Shader parameters: saturation, CA strength, contrast, vignette, grain, exposure bias, RGB tint. Defined in `render.c` as `visual_styles[]`.

## Design Principles

- **Ethical reduction** (Rams): Remove until it breaks, add back one thing
- **Bold shapes at 480x300** (Rodkin): Details must be BIG. If it's not visible as a 3px element, it's not visible.
- **Diegetic interaction** (Remo): Sounds come from objects, not UI. Clicks, fabric, flame — not chimes.
- **Visible consequence** (Chung): Every interaction changes the world. Light a candle → see flames.
- **The void is the point** (Wreden): "Three hours to kill." The game is about waiting, not completing.
- **No hand-holding**: No task counter, no exit beacon, no tutorial text. Architecture guides the player.

## Anti-Patterns (Never)

- Horror vibes — this is wonder, not dread
- Creepy ambient drones — if it sounds scary, it IS scary, fix it
- Tiny detail objects at 480x300 — they're invisible, scale up or remove
- Yellow-tinting everything — neutral base, SPECIFIC accent colors
- Interactive feedback that's only shader-level — if you can't see it in a screenshot, it doesn't count
- Grey-on-grey in space — hull walls must pop against void, not blend into fog
- Earth glow as a whisper — it's the emotional anchor, make it a landmark
- Reusing Paris audio for space — space needs its own acoustic character

## Current State (March 2026)

Working full game loop with all scenes. Space scenes have their own procedural drones (hull resonance, air circulation, insulated quiet). Task counts are split: Paris room has 5, space suite has 4. State resets are centralized in `load_state()`.

**Legacy/dev-only scenes:** HALLWAY, ROOM, BATHROOM (Paris hotel). Accessible via dev keys 3/4/6. ~350 lines of craft preserved for potential reintegration. Not in the main game flow.

Needs: better geometry variety, real composed music, more rooms/spaces, polished ending, sound design depth. This is a years-long project — quality over speed.
