# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Philosophy
> "Three hours to kill." — Auckland at 2 AM. A taxi. The Sky Tower. An elevator to orbit. A luxury hotel that has no business being in space. Built in pure C.

**Emotion:** Wonder and melancholy, not horror. Arrival, not escape.
**References:** Godard (Contempt, Pierrot le Fou), Hotel Chevalier, Bioshock Infinite opening, Mirror's Edge, Gravity Bone / Thirty Flights of Loving.
**Rendering:** 1920×1200 native resolution. Procedural materials, film grain, post-FX.

## Commands

```bash
make              # Incremental build
make run          # Build and run
make clean        # Remove object files + binaries
make dev SCENE=STATE_SPACE_SUITE   # Skip title, boot into a scene
make qa           # QA mode: automated screenshots + pixel analysis
make check        # Static analysis (pedantic warnings)
make test         # Headless unit tests (tests/test_main.c)
make count        # Line counts per file
make release      # Stripped -O3 binary
make fmt          # clang-format all source
make dev-watch    # Auto-rebuild + restart on source changes (preserves scene)
make dev-diff     # Quick QA rebuild + diff against baseline
make playtest     # Clean build for external testers (no debug keys, README)
```

Dev mode defaults to `STATE_SPACE_LOBBY` if no SCENE given. QA outputs to `qa/screenshots/` and `qa/report.json`.

## Architecture

### Overview
All mutable game state lives in a single `GameCtx g` struct (`game_ctx.h`). Scene logic is split into per-scene .c files with `load()` and `update()` functions, wired through a function-pointer registry (`scene_registry.c`). This keeps each file small enough for LLMs to work with efficiently.

### File Map
```
game_ctx.h          — GameCtx struct (all state), SceneDesc typedef
scene_registry.c    — const SceneDesc scene_descs[] table (one entry per GameState)

scene_splash.c      — STATE_TITLE
scene_taxi.c        — STATE_CAR, STATE_DRIVING, STATE_RETURN_TAXI
scene_exterior.c    — STATE_HOTEL_EXT
scene_lobby.c       — STATE_LOBBY
scene_elevator.c    — STATE_ELEVATOR
scene_hotel.c       — STATE_HALLWAY, STATE_ROOM, STATE_BATHROOM
scene_space_lobby.c — STATE_SPACE_LOBBY
scene_corridor.c    — STATE_SPACE_CORRIDOR
scene_suite.c       — STATE_SPACE_SUITE (tasks, wrongness, interactions)
scene_balcony.c     — STATE_BALCONY
scene_endings.c     — STATE_BED, STATE_STARS, STATE_HYPERSPACE, STATE_PARIS_DREAM

main.c              — game loop, render pipeline, menu, transitions, debug
scene.c             — geometry builders (add_wall, build_lobby, etc.)
audio.c             — procedural audio synthesis
render.c            — rendering + post-FX shaders
lighting.c          — GLSL lighting + per-scene presets
player.c            — Quake-style movement/physics
npc.c               — Gibbons NPC
ui.c / ui.h         — Spring physics, icons, UI components
config.h            — centralized constants (PI, SAMPLE_RATE, task counts, etc.)

assets/skytower.obj — Sky Tower 3D model (first external mesh)
assets/*.glb, *.obj — Auto-loaded model assets (ModelAsset registry)
scripts/blender_send.py — Blender MCP socket helper (model/rig/export)
scripts/ev_shell_workbench.py — Blender level editor for room shells (Gehry workflow)
```

### 3D Model Asset System

**Auto-loading**: Any `.glb` or `.obj` in `assets/` is auto-loaded at startup into `g.model_assets[]` (max 16). Lighting shader applied to all material slots. GLB animations loaded and ticked each frame.

**Placing models in scenes:**
```c
int taxi = find_model_asset("taxi");  // lookup by filename without extension
if (taxi >= 0) {
    add_model(s, 0, 0, -5,           // position
              1, 1, 1,               // scale
              0,                     // rotation degrees
              taxi,                  // model_index
              MAT_CONCRETE,          // materialId for shader
              (Color){220,200,50,255}); // base color
}
```

**Key types** (`ev_types.h`):
- `ModelAsset` — Model + animations + name + frame tracking
- `SHAPE_MODEL` — ShapeType for walls referencing loaded models
- `model_index` — field on Wall struct, indexes `g.model_assets[]`

**Formats:**
- **Static props** (.obj): 50-200 tris, no textures, no animation
- **Animated models** (.glb): 200-800 tris, simple rigs, max 4 bone influences
- **No UV textures** — GLSL materialId system handles surfaces procedurally
- **Scale**: 1 unit = 1 meter. GLB exports Y-up (matches Raylib).

### Blender Pipeline (Mac Mini)

Blender runs on Mac Mini (`ssh mini-ts`, port 9877). Use `/blender` skill or `scripts/blender_send.py`.

```bash
# Model in Blender, export GLB, fetch to engine
ssh mini-ts 'python3 ~/blender_send.py --export-glb /Users/klaus/taxi.glb'
scp mini-ts:~/taxi.glb assets/
make run  # auto-loads new model
```

### Shell System (Gehry-esque Environments)

For organic/deconstructivist architecture where code-placed boxes won't cut it. The visual mesh is a GLB from Blender; collision uses invisible AABB walls.

**Engine functions:**
```c
// Visual shell — renders GLB mesh, no collision (pair with collision walls)
add_shell(s, "lounge_shell", 0, 0, 0, 1,1,1, 0, MAT_CONCRETE, WHITE);

// Invisible collision volumes — player physics only
add_collision_wall(s, 0, 0, 0, 16, 0.05, 14);   // floor
add_collision_wall(s, -8, 3, 0, 0.15, 6, 14);    // left wall
add_collision_floor(s, 0, 0, 0, 16, 14);          // shorthand
add_collision_ceiling(s, 0, 5, 0, 16, 14);        // shorthand
```

**Blender workflow** (`scripts/ev_shell_workbench.py`):
```python
exec(open('scripts/ev_shell_workbench.py').read())
shell_setup("gehry_lounge", width=16, depth=14, height=6)
# Model curves in 'Shell' collection...
collision_floor(y=0)
collision_walls()
collision_curve("swooping_wall", center=(0,0,0), radius=6, height=5)
collision_ramp("entry_ramp", start_pos=(-5,0,6), end_pos=(-5,2,0), width=3)
shell_preview()   # renders + prints stats
shell_export()    # GLB + complete C code to paste
```

**Pattern:** Gehry buildings have crazy shells but regular interiors. The titanium swoops are the architecture; inside, you still place furniture with `add_sofa()`, `add_desk()`, etc. Shell = visual envelope. Code = everything inside.

### Adding a New Scene
1. Add `STATE_NEW_SCENE` to the `GameState` enum in `ev_types.h`
2. Create `src/scene_newscene.c` with `newscene_load()` and `newscene_update(float dt)`
3. Add entry to `scene_descs[]` in `scene_registry.c`
4. Add geometry builder to `scene.c` / `scene.h` if needed
5. `make` — the Makefile wildcards pick up new .c files automatically

### State Machine
`load_state()` in main.c handles common reset (stop audio, clear state), then dispatches to the scene's `load()` function via the registry. Transitions use `transition_to()` (fade-to-black) or `hard_cut_to()` (Blendo-style instant cut with warm white flash).

```
TITLE → CAR → DRIVING → HOTEL_EXT → LOBBY → ELEVATOR → HYPERSPACE
→ SPACE_LOBBY → SPACE_CORRIDOR → SPACE_SUITE → BALCONY → BED → STARS

Orphaned (dev keys 3/4/6): HALLWAY, ROOM, BATHROOM (Paris hotel)
```

### Rendering Pipeline
```
1920×1200 RenderTexture (render_target)
  ├─ Shadow pass: depth-only FBO (512×512) from key light perspective
  ├─ Earth pass: procedural sphere behind scene geometry (space scenes)
  ├─ Scene pass: lighting shader (key+fill+4 point lights+shadows+materials)
  ├─ Dust motes / zero-g sparkles
  └─ HUD (spring-scaled crosshair + pixel icons)
        ↓
1920×1200 RenderTexture (postfx_target)
  └─ Post-FX shader (CA, grain, bloom, SSAO, dither, scanlines, vignette...)
        ↓
Window (nearest-neighbor upscale, aspect-ratio letterboxed)
```

### Lighting Shader (`lighting.c`)
GLSL 330 embedded as C string literals in `vs_source` / `fs_source`. Modifying the shader means editing multiline string concatenation — be careful with `\n` line endings.

- **13 procedural materials** (materialId 0-12): concrete, marble, wood, carpet, wallpaper, tile, brass, glass, leather, fabric, checkerboard, herringbone, parquet. All computed from world-space UV + noise functions.
- **Shadow mapping**: PCF 3×3 on 512×512 depth texture. Shadow factor partially attenuates key light (×0.7, not full black).
- **Light pools**: Point lights project soft circles on upward-facing floors (`norm.y > 0.9`).
- **Per-scene presets**: `LightingPreset_*()` functions return `SceneLighting` structs (key/fill direction+color, ambient, 4 point lights).

### Scene System (`scene.c`)
All geometry is code — no mesh files except `assets/skytower.obj`. Scenes built with `add_wall()`, `add_cylinder()`, `add_sphere()`, `add_cone()` + composition helpers (`add_chandelier()`, `add_desk()`, etc.). Material tagging happens via `set_last_material()` or auto-detection in `tag_materials_by_color()`.

**Budget:** `MAX_WALLS = 2048`, `MAX_OBJECTS = 64`. Check `make qa` for per-scene wall counts.

### Z-Fighting Prevention
The engine uses a **decal system** with OpenGL polygon offset to prevent z-fighting. Any geometry that sits flush against another surface MUST be marked as a decal.

**Rules:**
1. **Flush-against-wall geometry** (baseboards, crown molding, wainscoting panels, picture canvases): auto-marked as decal in composition helpers
2. **Floor overlays** (puddles, light shafts, rugs, stains): use `add_wall_decal()` or `add_wall()` + `set_last_decal()`
3. **Blob shadows**: rendered with stronger polygon offset (`-2.0, -2.0`) in their own GL state block
4. **Clip planes**: tightened to near=0.05, far=300 (`rlSetClipPlanes` in main.c) — ~20× better depth precision than defaults
5. **Never place two `add_wall()` calls at the same position** without marking one as decal
6. Use `add_wall_decal()` for one-off decals instead of `add_wall()` + `set_last_decal()`
7. Constants `Z_DECAL_BIAS`, `Z_DECAL_BIAS2`, `Z_TRIM_BIAS` in `config.h` for manual y-offsets when needed

### Audio System (`audio.c`)
100% procedural — every sound synthesized from sine waves, noise, and envelopes at `SAMPLE_RATE = 44100`. No audio files. Drones are 20-32 second loops with reverb tails. Through-wall sounds (muffled piano, distant voices, footsteps above) create presence of inaccessible lives.

### Physics (`player.c`, `ev_types.h`)
Quake-style air strafing, bunny hopping (50ms friction grace), wall running, ledge mantling, momentum slides, dashing. All tuning lives in `PhysicsConfig` (59 parameters) with defaults in `physics_default()`.

### NPC System (`npc.c`)
Gibbons: geometric cube-person with segmented limbs. Waypoint-based navigation, per-waypoint dialogue, physics modes (ghost vs grounded). Drawing uses macros (`P()`, `D()`, `DRAW()`) for local-space positioning relative to NPC yaw.

## Key Conventions

- **1920×1200 visibility rule**: If it's not 3+ pixels at render resolution, scale it up or remove it. See `scale.h` for canonical dimensions.
- **Color palette**: `palette.h` defines `PAL_*` constants. Neutral warm whites + specific accents (French red, French blue, brass gold).
- **Warmth progression**: `SetPostFXWarmth()` ranges 0→1 as the player completes tasks. The room literally warms.
- **Interaction = visible consequence**: Every E-press must change geometry or lighting, not just set a flag.
- **Spring physics for UI**: Crosshair scale, text entry, title animation all use mass-spring-damper (k=280, d=26, m=0.9).

## Visual Style Presets (Shift+1-9)

| Key | Style | Character |
|-----|-------|-----------|
| Shift+1 | 16mm Film | Default. Warm neutral, slight bloom, architectural |
| Shift+2 | PS1 | Ordered dithering, color quantization, 2× pixels |
| Shift+3 | Noir | Near-mono, crushed blacks, heavy vignette, sharp |
| Shift+4 | CRT | Scanlines, bloom, phosphor glow |
| Shift+5 | Godard | Saturated, contrasty, red push, French New Wave |
| Shift+6 | VHS | Heavy grain+CA, warm mud, scanlines |
| Shift+7 | Neon | Oversaturated, bloom heavy, teal-orange tint |
| Shift+8 | Woodcut | Extreme dither, near-mono, posterized |
| Shift+9 | Raw | No post-FX. Naked geometry and lighting |

Defined in `render.c` as `visual_styles[]`. Styles persist across scene changes.

## Debug Keys

| Key | Action |
|-----|--------|
| F1 | Wireframe toggle |
| F3 | Debug overlay (FPS, walls, position, state, speed bar, movement mode) |
| F4 | Noclip fly mode (Space=up, Ctrl=down) |
| F5 | Nudge mode — select and reposition walls with arrow keys |
| 0-9 | Jump to scene (0=Taxi, 1=Exterior, 2=Lobby, ..., 9=Space Suite) |

## Anti-Patterns (Never)

- Horror vibes — wonder, not dread
- Creepy ambient drones — if it sounds scary, it IS scary
- Tiny detail objects at 1920×1200 — invisible, scale up or remove
- Yellow-tinting everything — neutral base, SPECIFIC accent colors
- Shader-only interaction feedback — must be visible in a screenshot
- Grey-on-grey in space — hull must pop against void
- Earth glow as whisper — it's the emotional anchor, make it a landmark
- Reusing Paris audio for space — space needs its own acoustic character

## Design Principles

- **Ethical reduction** (Rams): Remove until it breaks, add back one thing
- **Bold shapes** (Rodkin): Details must be BIG at 1920×1200
- **Diegetic interaction** (Remo): Sounds from objects, not UI
- **Visible consequence** (Chung): Every interaction changes the world
- **The void is the point** (Wreden): The game is about waiting, not completing
- **No hand-holding**: No task counter, no exit beacon, no tutorial text
