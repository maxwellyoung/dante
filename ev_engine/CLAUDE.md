# CLAUDE.md - Endearing Void Engine

## Philosophy
> "Three hours to kill." — A first-person exploration of a Paris hotel at 2 AM, built in pure C.

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
TITLE → TAXI (auto-cinematic) → ARRIVAL (auto) → HOTEL_EXT (3D, walk to entrance)
→ LOBBY (3D) → HALLWAY (3D) → ROOM (3D, 5 multi-step tasks) → BALCONY (3D, Paris)
→ BED (2D, phosphorescent stars) → STARS (2D, title card)
```

## Debug Keys

| Key | Action |
|-----|--------|
| F1 | Wireframe toggle |
| F3 | Debug overlay (FPS, walls, position, state) |
| F4 | Noclip (fly through walls, Space=up, Ctrl=down) |

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
- Adding Claude attribution to commits

## Current State (March 2026)

Working full game loop with all scenes. Needs: better geometry variety, real composed music, more rooms/spaces, polished ending, sound design depth. This is a years-long project — quality over speed.
