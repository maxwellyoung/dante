# Blender Model Plan — Endearing Void

> Every model serves the story. If a box says it better, keep the box.

## Infrastructure

- **Blender**: Mac Mini (`mini-ts:9877`), BlenderMCP addon
- **Pipeline**: Blender → export .glb/.obj → `scp` to `ev_engine/assets/` → auto-loads at startup
- **Engine API**: `find_model_asset("name")` + `add_model(scene, ...)` or direct NPC replacement
- **Helper**: `ev_engine/scripts/blender_send.py`
- **Constraints**: <1MB per model, <2000 tris, materialId-based shading, 1 unit = 1 meter

## Philosophy

The game is 960×600. Procedural cube geometry IS the aesthetic — Gravity Bone, not Pixar. Models should only replace geometry where **silhouette** or **narrative weight** demands it. A bed that's just rectangles says "bed." A bed with a headboard curve, rumpled duvet edge, and pillow indent says "someone slept here alone."

**Model when:**
- The object is an emotional anchor (bed, bathtub, champagne glass)
- Curved silhouette is critical (bathtub, piano, Gibbons)
- The player stares at it for extended time (bed ritual, balcony chairs)
- Boxes literally can't communicate the shape (telephone receiver, telescope)

**Keep procedural when:**
- Architectural (walls, doors, frames, columns, floors) — the engine excels at this
- Flat/rectangular objects (books, tickets, postcards, suitcases)
- Background/atmosphere (buildings, streetlights, parked cars)
- Anything the player passes quickly

---

## Phase 1: Gibbons (IN PROGRESS)

**Priority: HIGHEST** — the only character in the game. Present in 4 scenes.

| Task | Status | Notes |
|------|--------|-------|
| v1 model (Pixar bellhop) | DONE | 15MB, 64 meshes — too heavy |
| Decimate to <2000 tris | IN PROGRESS | Blender agent running |
| Rig with simple armature | TODO | ~20 bones: spine, arms, legs, head, hat |
| Idle animation | TODO | Breathing, weight shift, occasional tie adjust |
| Walk cycle | TODO | For waypoint pathing (existing npc.c system) |
| Gesture animation | TODO | Arm extended toward door (NPC_GESTURING behavior) |
| Replace cube-person draw | TODO | Swap `draw_npc()` to use GLB model |
| Per-material rendering | TODO | Skin=MAT_CONCRETE, jacket=MAT_FABRIC, buttons=MAT_BRASS |

**Scenes**: Space Lobby, Corridor, Suite, Lobby (Earth)

**Integration approach**: Modify `draw_npc()` in `npc.c` to optionally use the GLB model. Keep cube-person as fallback. Use existing NPC position/rotation — just change what's drawn.

---

## Phase 2: Narrative Objects — The Twos

These are the emotional core. Each tells the story of absence.

### 2A: Champagne Glasses (Suite coffee table)

Two flutes on a brass tray. One half-full, one empty. The washing line moment.

| Detail | Spec |
|--------|------|
| Tris | ~80 total (two simple flute shapes) |
| Materials | MAT_GLASS (stems), MAT_BRASS (tray) |
| Why model | Flute silhouette — cylinder+cone can't do the narrow stem |
| Scenes | Suite, cleaned suite (one removed) |
| Export | .obj (static, no animation) |

### 2B: Wine Glass with Lipstick

The glass she drank from. Red crescent on the rim.

| Detail | Spec |
|--------|------|
| Tris | ~40 |
| Why model | Lipstick mark needs to read at 960×600 — geometric crescent |
| Scenes | Suite (removed in cleaned variant) |

### 2C: Telephone (Suite desk / corridor)

Rotary or vintage handset. Rings unanswered. Interaction object.

| Detail | Spec |
|--------|------|
| Tris | ~120 |
| Materials | MAT_BRASS (body), MAT_LEATHER (handset grip) |
| Why model | Handset cradle shape is impossible with boxes |
| Scenes | Suite desk, corridor (through-wall ring) |

### 2D: Photograph Frame

Face-down on nightstand. You can never flip it.

| Detail | Spec |
|--------|------|
| Tris | ~20 |
| Why model | Slight frame thickness, glass reflection hint |
| Keep procedural? | Probably. Current box reads fine. LOW PRIORITY. |

---

## Phase 3: Scene-Defining Furniture

Objects the player interacts with or stares at during key moments.

### 3A: Bed (Suite — the emotional endpoint)

The bed ritual is 60+ seconds of staring. It needs to be beautiful.

| Detail | Spec |
|--------|------|
| Tris | ~300 |
| Key shapes | Headboard curve (navy velvet), pillow indents, duvet fold edge |
| Materials | MAT_VELVET (headboard), MAT_FABRIC (duvet, pillows), MAT_WOOD (frame) |
| Why model | Pillow softness, duvet rumple, headboard curve — boxes can't do it |
| Scenes | Suite, cleaned suite (one pillow centered), Paris dream |
| Variants | Two pillows (suite), one pillow centered (cleaned), pillow indent (Paris) |

### 3B: Bathtub (Paris hotel bathroom)

Curved porcelain. Freestanding. The shape boxes can never achieve.

| Detail | Spec |
|--------|------|
| Tris | ~200 |
| Materials | MAT_MARBLE (porcelain exterior), MAT_TILE (interior) |
| Why model | THE reason to model — the curve is the whole point |
| Scenes | Bathroom |

### 3C: Grand Piano (Lobby — through-wall audio source)

Lid propped open. The muffled piano sound comes from this direction.

| Detail | Spec |
|--------|------|
| Tris | ~250 |
| Materials | MAT_WOOD (body, dark), MAT_BRASS (pedals, hinges) |
| Why model | Lid angle, curved body — piano silhouette is iconic |
| Scenes | Lobby |

### 3D: Desk Lamp (Suite, Paris room)

Brass arm, cone shade, warm pool of light. Interaction object.

| Detail | Spec |
|--------|------|
| Tris | ~60 |
| Why model | Current cone+cylinder is decent. MEDIUM PRIORITY. |
| Keep procedural? | Maybe. The cone shade already reads. |

### 3E: Armchairs (Suite — two chairs facing Earth)

The gut-punch pair. Two chairs angled toward the window. For watching Earth together.

| Detail | Spec |
|--------|------|
| Tris | ~150 each |
| Materials | MAT_LEATHER (seat), MAT_WOOD (frame) |
| Why model | Curved armrests, seat cushion — reads as "chair" not "box stack" |
| Scenes | Suite (pair), balcony (pair), lobby |

---

## Phase 4: Atmospheric Props

Lower priority. These add density but don't carry narrative weight.

### 4A: Luggage / Suitcase
- ~100 tris. Leather corners, brass clasps, blue airline tag
- Scenes: Suite entry, lobby, corridor (floating in zero-g)
- Narrative: packed for two — her clothes visible if lid open

### 4B: Luggage Cart (Gibbons' prop)
- ~150 tris. Brass frame, rubber wheels
- Scenes: Lobby, space lobby
- Narrative: Gibbons' tool, his identity

### 4C: Telescope (Balcony)
- ~100 tris. Brass tube on tripod
- Scenes: Balcony (pointing at Earth)
- Interaction: look through it — Earth fills the view

### 4D: Room Service Cart
- ~120 tris. Brass frame, white cloth, covered dishes
- Scenes: Corridor (parked outside a door)

### 4E: Standing Ashtray
- ~40 tris. Brass pedestal, glass bowl
- Scenes: Corridor, lobby

---

## Phase 5: Vehicles & Architecture

### 5A: Taxi Interior (Dashboard detail)
- Current procedural is excellent (313 walls, Michael Mann palette)
- Model ONLY the steering wheel (curved shape) — ~40 tris
- Everything else stays procedural

### 5B: Elevator Panel
- ~60 tris. Button grid, brass frame, floor indicator
- Current procedural is fine. LOW PRIORITY.

### 5C: Sky Tower
- Already loaded as .obj (9402 verts, 3134 tris)
- Could decimate to ~1000 tris for better perf
- LOW PRIORITY — it's a silhouette, reads fine

---

## Production Schedule

```
PHASE 1: Gibbons           ████████░░  (v1 done, decimating, rig+anims next)
PHASE 2: Narrative Objects  ░░░░░░░░░░  (glasses, telephone — high impact, low effort)
PHASE 3: Furniture          ░░░░░░░░░░  (bed first, then bathtub, piano)
PHASE 4: Atmospheric        ░░░░░░░░░░  (luggage, cart, telescope)
PHASE 5: Vehicles           ░░░░░░░░░░  (steering wheel only)
```

### Batch Workflow (per session)

1. Connect to Blender: `ssh mini-ts`, verify port 9877
2. Model in Blender (scripted via `blender_send.py` or manual)
3. Render preview: `--render ~/preview.png`, scp + review
4. Export: `--export-glb ~/model.glb` or `--export-obj ~/model.obj`
5. Deploy: `scp mini-ts:~/model.glb ev_engine/assets/`
6. Place in scene: `add_model()` call in scene builder
7. QA: `make qa` — check wall counts, no overflow

### Material Assignment Per Model

Name Blender materials to match engine IDs:

```
Skin         → MAT_CONCRETE (0)  — flat, subtle noise
Marble       → MAT_MARBLE (1)    — veined, polished
Wood         → MAT_WOOD (2)      — grain, warm
Brass/Gold   → MAT_BRASS (6)     — metallic, high spec
Glass        → MAT_GLASS (7)     — reflective
Leather      → MAT_LEATHER (8)   — subtle grain
Fabric       → MAT_FABRIC (9)    — visible weave
Velvet       → MAT_VELVET (13)   — view-dependent sheen
```

For multi-material models (Gibbons), draw each mesh group with its own `SetMaterialId()` call — same pattern as `npc.c`'s `D()` macro.

---

## Total Budget

| Category | Count | Avg Tris | Total Tris | Total Size Est |
|----------|-------|----------|------------|----------------|
| Gibbons | 1 | 1500 | 1500 | ~200KB |
| Narrative objects | 4 | 60 | 240 | ~50KB |
| Furniture | 5 | 200 | 1000 | ~150KB |
| Atmospheric | 5 | 100 | 500 | ~80KB |
| Vehicles | 1 | 40 | 40 | ~10KB |
| **Total** | **16** | — | **~3280** | **~490KB** |

Plus existing skytower.obj (239KB). Total asset budget: **~730KB**. The current 15MB Gibbons is 30x over budget — decimation is critical.

---

## Anti-Patterns

- **Don't model what boxes say better.** Walls are walls. Doors are doors.
- **Don't add UV textures.** The GLSL materialId system IS the look.
- **Don't over-detail.** At 960×600, a 50-tri chair reads the same as a 500-tri chair.
- **Don't model for Blender renders.** Model for the engine's lighting shader.
- **Don't animate what code animates.** Floating objects, bobbing, breathing — all code-side.
- **Don't forget scale.** 1 unit = 1 meter. A chair is 0.45m seat height, not 45.
