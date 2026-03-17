# ARCHITECTURE.md — EV Engine Implementation Guide

Pure C99 on Raylib. ~8000 lines across 8 source files. No ECS, no scene graph, no asset pipeline. Everything is code.

## 1. State Machine

Central `GameState` enum (18 states) in `ev_types.h`. The global `state` variable drives all logic.

### load_state(GameState s) — src/main.c:405

Every scene transition goes through `load_state()`. It does, in order:

1. `StopAllAudio()` — nuclear kill on all looping sounds
2. Reset timers, tasks, text, flash state
3. `ApplyVisualStyle()` — restore PostFX baseline from current visual style
4. `switch(s)` — per-scene setup:
   - `build_*(&scene)` — construct geometry
   - `init_player(&player, scene.spawn)` — reset player at spawn
   - `StartAmbient()` / `Play*()` — start scene audio
   - `SetSceneLighting()` — apply lighting preset
   - `SetPostFX*()` — per-scene PostFX overrides (grain, warmth, CA, exposure)
   - NPC init (waypoints, dialogue) for space scenes
5. `fade_alpha = 1.0` — scene starts black, fades in
6. `UpdateShadowMatrix()` — per-scene shadow radius

### Transition Types

```c
transition_to(STATE_X)        // Fade to black at speed 2.0, 0.3s hold, fade in. Plays door sound.
transition_to_slow(STATE_X, speed)  // Same but custom fade speed. BED gets 1.0s hold.
hard_cut_to(STATE_X)          // Instant. No fade. Warm white flash (0.12s decay).
                              // Plays snd_hard_cut_punch. Restores master volume.
```

The fade system in the game loop (`main.c:1540-1556`):
- `fade_alpha` lerps toward `fade_target` at `fade_speed`
- When `fade_alpha` reaches 1.0 (full black), `hold_timer` starts counting down
- When hold expires, `load_state(next_state)` fires and `fade_target` flips to 0.0

### State Flow

```
TITLE ─enter─→ CAR ─13.5s─→ HYPERSPACE ─6s─→ SPACE_LOBBY
                                                    │
                                                    ├─ elevator ─→ SPACE_CORRIDOR
                                                    │                    │
                                                    │                    └─→ SPACE_SUITE
                                                    │                           │
                                                    │                   all tasks done
                                                    │                           │
                                                    │                    ┌──────┘
                                                    │                    ▼
                                                    │               BALCONY ─14s─→ BED ─→ STARS

Orphan scenes (dev keys): HOTEL_EXT, LOBBY, HALLWAY, ROOM, BATHROOM
Paris dream: ROOM → BATHROOM → ROOM (round-trip with state preservation)
```

## 2. Scene Authoring

All geometry is procedural — no mesh files except `assets/skytower.obj`.

### Core Workflow

```c
void build_my_scene(Scene *s) {
    memset(s, 0, sizeof(Scene));           // Always start clean
    s->surface = SURFACE_MARBLE;           // Footstep material
    s->fog_color = (Color){20,18,25,255};
    s->fog_density = 0.003f;

    // Floor
    add_wall(s, 0, -0.1f, 0, 20, 0.2f, 20, PAL_MARBLE);
    set_last_material(s, MAT_MARBLE);

    // Table + lamp (composition helper + manual)
    add_desk(s, 2, 0, 3, 0.0f, PAL_WOOD);     // helper: creates legs + top
    add_wall(s, 2, 0.9f, 3, 0.1f, 0.4f, 0.1f, PAL_BRASS);  // lamp base
    set_last_material(s, MAT_BRASS);
    add_cone(s, 2, 1.2f, 3, 0.3f, 0.2f, (Color){240,230,200,200}); // shade

    // Interactable
    add_object(s, 2, 0.9f, 3, "lamp", PAL_BRASS, 2);  // 2 steps to complete

    // Exit trigger
    s->exit_pos = (Vector3){0, 1.6f, -8};
    s->has_exit = true;
    s->spawn = (Vector3){0, 1.6f, 8};

    tag_materials_by_color(s);  // auto-detect brass, marble, wood, glass, wallpaper
}
```

### Primitive Functions (scene.h)

| Function | What it adds |
|---|---|
| `add_wall(s, x,y,z, w,h,d, color)` | Axis-aligned box. Center at (x,y,z). |
| `add_cylinder(s, x,y,z, diameter, height, color)` | Cylinder. |
| `add_sphere(s, x,y,z, diameter, color)` | Sphere. |
| `add_cone(s, x,y,z, diameter, height, color)` | Cone. |
| `add_skytower(s, x,y,z, scale, color)` | Sky Tower .obj mesh. |
| `add_object(s, x,y,z, name, color, max_steps)` | Interactable with radius 2.0. |

### Post-add Modifiers

```c
set_last_material(s, MAT_BRASS);   // Override material on last wall
set_last_rotation(s, 45.0f);       // Y-axis rotation in degrees
set_last_decal(s);                 // Polygon offset for overlays
```

### Composition Helpers

`add_chandelier()`, `add_desk()`, `add_sofa()`, `add_chair()`, `add_dining_table()`, `add_fireplace()`, `add_bar_counter()`, `add_rug()`, `add_bookshelf()`, `add_picture_frame()`, `add_window()`, `add_door_frame()`, `add_wainscoting()`, `add_column_row()`, `add_baseboard()`, `add_crown_molding()`, `add_arch_doorframe()`, `add_corridor_door()`.

### tag_materials_by_color (scene.c:161)

Auto-assigns materials to untagged walls (material == MAT_CONCRETE) by color heuristic:
- Brass: r>150, g>100, r>g>b
- Marble: all channels >170, close together
- Wood: brown range, r<160
- Glass: all <20
- Wallpaper: all >220

Call after all geometry is placed. Manual `set_last_material()` takes priority.

### Budget

`MAX_WALLS = 2048`, `MAX_OBJECTS = 64`. Overflow prints a warning and drops the wall. QA mode reports per-scene wall counts and flags >85% usage.

## 3. Interaction System

### E-Press Pipeline

Every scene with interactables follows the same pattern (`main.c`, repeated per state):

```c
if (IsKeyPressed(KEY_E)) {
    for (int i = 0; i < scene.object_count; i++) {
        InteractObject *obj = &scene.objects[i];
        if (!obj->active || obj->done) continue;
        float dist = Vector3Distance(player.camera.position, obj->pos);
        if (dist < obj->radius) {                          // within 2.0 units
            Vector3 to = Vector3Normalize(Vector3Subtract(obj->pos, player.camera.position));
            Vector3 look = Vector3Normalize(Vector3Subtract(player.camera.target, player.camera.position));
            if (Vector3DotProduct(to, look) > 0.5f) {      // facing it (~60 deg cone)
                obj->step++;
                PlayInteract(&audio, get_interact_sound(obj->name));
                kick_camera(&player, -0.01f, 0.005f);
                // ... per-step geometry changes ...
                if (obj->step >= obj->max_steps) {
                    obj->done = true;
                    obj->reward_timer = 1.5f;
                    tasks_done++;
                    PlayRewardSound(&audio);
                    // warmth, fog, exposure ramp
                }
                break;
            }
        }
    }
}
```

### Sound Mapping (get_interact_sound, main.c:126)

Maps object names to `InteractSoundType`: lamp/ashtray/bell → CLICK, drawers/bed/desk → FABRIC, candles → FLAME, champagne → CORK_POP, wineglass → GLASS_CLINK.

### Multi-Step Rituals (interaction_timers[], interaction_phases[])

Space suite has 4 ritual slots. When a step triggers a ritual:
1. Set `interaction_phases[slot] = 1`, `interaction_timers[slot] = duration`
2. Each frame, timer counts down. During countdown: animate lighting/camera
3. When timer hits 0: `interaction_phases[slot] = 2` (done)

Example — champagne pour (slot 1): 2.0s timer, golden liquid rises in glass via `add_wall()` each frame, bubbles float in zero-g.

### Visible Consequence Pattern

Every completed interaction adds geometry or changes lighting. Examples:
- Lamp: `add_light_panel()` + `SetPointLight()` — golden glow appears
- Candles: 3 flame boxes + warm point light
- Bed: bright sheet rectangle + chocolate box
- Drawers: colored clothing items laid out
- Champagne: glass, liquid, floating bubbles (spheres)

### Warmth Progression

`tasks_done / TASK_COUNT` → `SetPostFXWarmth()` (0→1). Fog density decreases. Exposure increases. The room physically warms as you interact.

## 4. Audio Synthesis

All procedural. `SAMPLE_RATE = 44100`. No audio files except 4 music tracks.

### Sound Architecture (audio.c)

Every sound follows: `gen_wave(samples)` → write PCM → `LoadSoundFromWave()` → `UnloadWave()`.

Typical synthesis recipe:
```
CONTACT layer: high-freq sine with fast exponential decay (transient)
BODY layer:    low-freq sine, slower decay (resonance)
DEBRIS/NOISE:  random samples with decay (texture)
REVERB:        delay-line feedback (d[i] += d[i-delay] * gain)
```

### Key Sounds

| Sound | Recipe |
|---|---|
| Marble step | 2200Hz click + 180Hz body + noise debris + 2-tap reverb |
| Carpet step | 45Hz thud + fabric noise, minimal reverb (carpet deadens) |
| Wood step | 400Hz tap + 80Hz body + creak harmonic + 2-tap reverb |
| Lobby drone | 32-bar Satie-style melody (E4 descending), 4 sections (ABAB'), detuned harmonics |
| Room drone | Same melody one octave lower, piano-like with hammer noise attack |
| Reward chime | C5+E5+G5 chord, 2s sustain, warm reverb |

### Drone Architecture

Drones are 20-32 second loops built from note sequences. Each note: sine fundamental + detuned 2nd/3rd/5th harmonics + noise burst on attack + exponential decay. Loop crossfade at boundaries. `StartAmbient()` plays looping, `StopAmbient()` stops.

### Through-Wall Audio

`snd_muffled_piano`, `snd_distant_voices`, `snd_footsteps_above` — filtered/quiet loops that create presence of inaccessible lives behind walls. Played per-scene in `load_state()`.

### Clock Deceleration

`SetClockRate(rate)` → `SetSoundPitch(snd_clock, rate)`. Each task in the space suite drops rate by 0.2. Clock slowing = time running out.

### File Music

4 tracks loaded from `assets/audio/`: `music_suite` ("lighthouse"), `music_balcony` ("unsaid"), `music_corridor` ("stt"), `music_title` ("ambient1"). Loaded via `LoadFileMusic()`, streamed via `UpdateFileMusic()` each frame.

## 5. Lighting & Materials

### Shader (lighting.c)

GLSL 330 vertex+fragment shaders embedded as C string concatenation (`vs_source`, `fs_source`). To edit: modify the string literals, watch `\n` line endings.

### Lighting Model

- **Key light**: half-Lambert diffuse, shadow-attenuated (shadow * 0.7, not full black)
- **Fill light**: soft counter-direction, half intensity
- **4 point lights**: inverse-square attenuation with smoothstep falloff. Floor pools on `norm.y > 0.9` surfaces
- **Rim light**: Fresnel pow(3) * 0.18
- **Specular**: Blinn-Phong pow(32), per-material multiplier (`specMult`)
- **Self-lit**: bright surfaces (luma > 0.7) glow softly
- **AO**: normal-based (upward-facing = brighter)
- **Fog**: exponential quadratic distance fog, mixed with `fogColor`

### 13 Procedural Materials (materialId 0-12)

All computed from world-space UV + noise/fbm functions:

| ID | Name | Technique | specMult |
|---|---|---|---|
| 0 | Concrete | fbm noise, subtle pitting | 0.08 |
| 1 | Marble | Sine veins + fbm variation | 0.35 |
| 2 | Wood | Sine grain + ring noise | 0.12 |
| 3 | Carpet | Dense fbm, fully matte | 0.00 |
| 4 | Wallpaper | Damask: sin*sin repeat pattern | 0.05 |
| 5 | Tile | fract grid + grout step function | 0.25 |
| 6 | Brass | High-freq noise shimmer | 0.60 |
| 7 | Glass | Fine noise, subtle | 0.50 |
| 8 | Leather | fbm + fine grain noise | 0.18 |
| 9 | Fabric | Warp/weft step pattern | 0.02 |
| 10 | Checkerboard | floor(uv*2) mod 2 | 0.30 |
| 11 | Herringbone | Row-shifted planks + grain | 0.15 |
| 12 | Parquet | Alternating grain direction tiles | 0.14 |

To add a new material: add enum to `MaterialType` in `ev_types.h`, add `else if (materialId == N)` block in `fs_source` in `lighting.c`, set `specMult`.

### Shadow Mapping

512x512 depth FBO. PCF 3x3 kernel. Bias 0.005. Shadow factor attenuates key light by 0.7 (not full black — architectural, not horror). `UpdateShadowMatrix()` takes key light direction + scene center + radius (per-scene, set in `load_state()`).

### Per-Scene Presets

`LightingPreset_*()` functions return `SceneLighting` structs: key/fill direction+color, ambient RGB, 4 point light positions/colors/radii. Called via `SetSceneLighting()` in `load_state()`.

## 6. Rendering Pipeline

### Two-Pass System (main.c game loop + render.c)

```
render_target (480x300, TEXTURE_FILTER_POINT)
  ├─ Shadow pass: draw_shadow_pass() → depth-only 512x512 FBO
  ├─ Earth pass: draw_earth() → procedural sphere (space scenes only)
  ├─ Scene pass: draw_scene_3d() → lighting shader, all geometry
  ├─ Particles: draw_dust_motes() or draw_zero_g_sparkles()
  └─ HUD: draw_hud() → crosshair, interact prompts

postfx_target (480x300)
  └─ draw_postfx() → post-processing shader

Window (960x600 default, nearest-neighbor upscale, aspect-ratio letterboxed)
```

### PostFX Shader (render.c)

Uniform-driven, all effects composable:
1. Pixelation (chunky pixel scaling)
2. Barrel distortion (speed-reactive)
3. Chromatic aberration (edge-distance RGB split)
4. Sharpening (unsharp mask)
5. Bloom (5x5 kernel, luminance threshold)
6. SSAO (8-sample radial luma comparison)
7. Exposure (exp2)
8. Saturation + warmth color grading
9. Contrast S-curve
10. Posterization (color quantization)
11. Ordered dithering (4x4 Bayer matrix)
12. Film grain (hash-based per-pixel noise)
13. CRT scanlines + phosphor bleed
14. Vignette (edge darkening)
15. Flash (scene-cut white flash, exponential decay)

### Visual Style Presets (render.c, Shift+1-9)

9 presets in `visual_styles[]`: 16mm Film (default), PS1, Noir, CRT, Godard, VHS, Neon, Woodcut, Raw. Each sets saturation, CA, contrast, vignette, grain, exposure_bias, tint, dither, scanline, bloom, posterize, pixelate, sharpen.

## 7. NPC System

### Setup (npc.c)

```c
Vector3 waypoints[] = {{3,1.6f,2}, {6,1.6f,-3}, {0,1.6f,-4}};
init_npc(&gibbons, start_pos, waypoints, 3, speed, wait_radius);
// or init_npc_grounded() for physics-enabled mode

static const char *lines[] = {"Line 1", "Line 2", "Line 3"};
npc_set_dialogue(&gibbons, lines, 3, 3.0f);  // 3s per line
```

### Update Loop (update_npc)

1. Move toward current waypoint (XZ only, physics handles Y)
2. Arrive within 0.3 units → `waiting = true`
3. While waiting: face player (smooth yaw lerp), deliver dialogue line (timer-based)
4. After dialogue + player within `wait_radius` → advance to next waypoint
5. After final waypoint → `active = false`

### Behavior States

`NPC_WALKING` (moving), `NPC_WAITING` (at waypoint), `NPC_READING` (idle >5s, head down), `NPC_SITTING` (on sofa, base_y lowered).

### Drawing (draw_npc)

Uses local-space macros:
- `P(lx, ly, lz)` — transforms local coords by NPC position + yaw rotation + walk bob
- `D(m, color, materialId)` — sets model color and shader material
- `DRAW(px,py,pz, sx,sy,sz)` — draws cube at local position with scale

Gibbons is ~30 draw calls: shoes, shins, thighs, belt, torso, lapels, tie, pocket square, shoulders, buttons, arms (upper+forearm+hand), briefcase, neck, head, eyes, mouth, bellboy cap. Idle quirks: weight shift, watch glance, head wander, breathing, tie adjustment. Walk: leg swing, arm counter-swing, torso twist, hip sway. Bow animation on waypoint arrival.

Blob shadow: flattened cylinder beneath feet, alpha fades with height.

## 8. Physics

### PhysicsConfig (ev_types.h:17, 59 parameters)

All movement tuning in one struct. `physics_default()` returns canonical values. Key defaults: walk 4.5, sprint 8.5, gravity 18.0, jump 5.5, player_radius 0.45.

### Quake-Style Movement (player.c)

**Core**: `accelerate()` projects velocity onto wish direction. Only adds speed if below cap along that axis. Low `air_speed_cap` (1.0) enables air strafing — perpendicular movement always adds speed while forward saturates.

**Ground**: `apply_friction()` decelerates. Then `accelerate()` toward wish direction at `ground_accel`.

**Air**: Minimal friction (0.2). `accelerate()` with `air_speed_cap = 1.0` — the Quake trick.

### Special Moves

| Move | Trigger | Mechanic |
|---|---|---|
| Bunny hop | Jump within 50ms of landing | Friction skipped during `bhop_friction_window`, speed preserved. Cap at 16.0. |
| Wall run | Airborne + fast + side wall + W held | Reduced gravity (wall_run_gravity * gravity_mult). Timer-based, up to 0.8s (longer in low gravity). |
| Wall jump | Space during wall run | Horizontal impulse (5.0) along wall normal + vertical (5.0). |
| Mantle | Near jump apex + W held + ledge in reach | Quadratic ease-out pull-up to ledge_y. Forward push on completion. |
| Slide | Ctrl + moving + grounded + speed > 60% walk | Locked direction, reduced friction (1.5 vs 6.0). Perpendicular steering. |
| Crouch jump | Jump from slide | +1.5 to jump impulse. |
| Dash | Q | 18.0 speed impulse for 0.15s, 0.6s cooldown. Locks direction. |

### control_mult — Agency Dial

`player.control_mult` scales `wish_speed` and mouse sensitivity. 1.0 = full control. 0.0 = frozen. Used for:
- Paris dream: 0.5 (dream movement)
- Balcony: starts 0.3, fades to 0.0 over 14s
- Post-task removal: 1.0→0.0 over 4s before balcony cut

### gravity_mult

1.0 = Earth. 0.5 = suite. 0.4 = space lobby/corridor. 0.3 = balcony. Affects jump height (grav_jump bonus), wall run duration (grav_bonus), and fall speed.

### Collision (collide_and_slide)

Two-pass for corners. Handles all shapes:
- **Cube (AABB)**: Fast path for `rotation_y == 0`. Rotated cubes transform player into local space.
- **Cylinder/Cone**: Circle push in XZ plane.
- **Sphere**: Full 3D push.

Returns `CollisionInfo` with wall normal for wall-running detection.

## 9. QA Pipeline

### Build & Run

```bash
make qa           # Compile with -DQA_MODE, run, output to qa/
./qa/run.sh       # Full pipeline: build → screenshots → analyze → grade → archive
./qa/run.sh --diff      # Diff current against baseline
./qa/run.sh --baseline  # Save current as baseline
./qa/run.sh --history   # Show run history
```

### QA_MODE (main.c:962-1401)

When compiled with `-DQA_MODE`:
1. Iterates all scenes in `qa_scenes[]` (14 entries)
2. Each scene gets 2 screenshots: "hero" (best composition) and "spawn" (first impression)
3. Per-screenshot pixel analysis:
   - Luminance stats (mean, black%, bright%, mid-tone%)
   - Color variance, hue bucket count, warmth (R-B bias)
   - Quadrant luma analysis (contrast ratio, L/R balance, T/B balance)
   - Edge density (Sobel-lite gradient detection)
   - Material breakdown (types used, coverage %)
   - Object reachability from spawn
4. Flags issues: unlit scenes, monotone color, low contrast, near-overflow budgets, horror vibes, grey-on-grey in space
5. Outputs `qa/report.json` + `qa/screenshots/*.png`

### analyze.py

Python script that reads `qa/report.json` and assigns letter grades (A-F) per scene based on lighting, composition, material variety, and anti-pattern checks. Compares against `qa/baseline/` for regression detection.
