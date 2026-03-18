# Gehry Hotel Atrium — Design Document

**Scene:** `build_space_hotel(Scene *s)`
**State:** `STATE_SPACE_HOTEL`
**Position in flow:** Between `STATE_SPACE_LOBBY` and `STATE_SPACE_CORRIDOR`
**Emotional beat:** The Bioshock Infinite "arrival into something impossible" moment

---

## 1. Architecture Concept

The player exits a glass elevator into a **massive, curved-glass atrium** — the central spine of the orbital hotel. This is NOT another enclosed room. It is the first time the player sees the FULL SCALE of where they are.

Think: Bilbao Guggenheim interior meets the ISS cupola, wrapped in Gehry's titanium-panel language.

### Key principles:
- **VAST and VERTICAL** — 50m wide, 80m long, 20m ceiling. The player has never been in a space this large in the game.
- **Curved glass walls** — approximated with 24-32 rotated flat panels forming two sweeping arcs. Earth and stars visible through the glass on both sides.
- **Multiple visible levels** — balconies, walkways, and planted terraces at 5m, 10m, and 15m heights. Only the ground floor is accessible. The upper levels are populated by geometry that implies other guests (furniture, warm light pools, a distant figure).
- **Warm interior vs cold void** — amber/gold light pools on the floor from overhead fixtures. Blue Earth glow washing through the glass. The interior is a WARM ISLAND in the cold of space.
- **Godard accents** — a single red grand piano at the center of the atrium. A blue sculpture near the far end. Bold, singular.
- **The feeling of a train station, an airport, a cathedral** — a transit space, not a dwelling. You are PASSING THROUGH something magnificent.

### Reference images (mental):
- Bilbao Guggenheim: the central atrium with curved titanium walls and skylights
- Gehry's Walt Disney Concert Hall: folded metal sheets catching light at different angles
- The Grand Budapest Hotel: the impossible scale of Anderson's lobby, but VERTICAL
- 2001's Space Station V: the rotation, the implied vastness beyond what you see

---

## 2. Geometry Plan

### Floor plan (top-down, approximate)

```
                    EXIT → corridor door (z = +38)
                         |
    ╭─── glass arc ───╮  |  ╭─── glass arc ───╮
   /                   \ | /                   \
  │   planted          │ │ │          planted   │
  │   terrace          │ │ │          terrace   │
  │                    │ │ │                    │
  │   ATRIUM FLOOR     │ │ │     ATRIUM FLOOR   │
  │   (walkable)       │ │ │     (walkable)     │
  │                    │ │ │                    │
  │        ┌─────┐     │ │ │                    │
  │        │PIANO│     │ │ │     ┌────┐         │
  │        │ RED │     │ │ │     │SCUL│         │
  │        └─────┘     │ │ │     │BLUE│         │
  │                    │ │ │     └────┘         │
  │                    │ │ │                    │
   \                   / | \                   /
    ╰─── glass arc ───╯  |  ╰─── glass arc ───╯
                         |
              SPAWN (z = -2) ← from elevator
```

### Dimensions
- **Total footprint:** 50m wide (x: -25 to +25), 40m deep (z: -4 to +38)
- **Ceiling height:** 20m (creates cavernous vertical drama)
- **Glass arc panels:** 24 panels per side (48 total), each ~2m wide, rotated to form a smooth curve
- **Floor:** single large checkerboard marble plane (1 wall)
- **Ceiling:** hull panel with structural brass ribs (4 walls)
- **End walls:** solid hull, one at spawn end (behind elevator), one at far end with corridor door (4 walls)

### Wall count estimate

| Element | Count |
|---------|-------|
| Floor (1) + ceiling (1) + ceiling ribs (4) | 6 |
| Glass arc panels (24 per side x 2) | 48 |
| Glass arc brass frames (top/bottom rail per arc, 2 per side) | 4 |
| Vertical mullions between glass panels (23 per side x 2) | 46 |
| End walls (2 hull + 2 cream paneling) | 4 |
| Upper balcony platforms (3 levels, 2 per level) | 6 |
| Balcony railings (brass, 6 sections) | 6 |
| Support columns (8 major, ground to ceiling) | 8 |
| Column brass caps (top + bottom, 8 columns) | 16 |
| Central walkway (brass inlay lines) | 4 |
| Grand piano (body + lid + legs + keys) | 6 |
| Blue sculpture (3-4 stacked/rotated forms) | 4 |
| Planted terraces (raised beds, 4 total) | 8 |
| Plant geometry (stylized — cubes/spheres, 2 per bed) | 8 |
| Seating clusters (3 clusters, ~6 walls each) | 18 |
| Light panels (overhead, 6 warm pools) | 6 |
| Dropped ceiling sections (2 zones) | 8 |
| Earth glow decals (floor + ceiling, 6 total) | 6 |
| Stars behind glass (20 tiny cubes) | 20 |
| Earth spheres (2 — planet + atmosphere rim) | 2 |
| Floating objects (zero-g storytelling, 8-10) | 10 |
| Door frame at exit end | 4 |
| Elevator shaft geometry at spawn end | 6 |
| Volumetric light shafts (transparent boxes) | 6 |
| Warm light fixture geometry (chandeliers/pendants, 3) | 12 |
| Recessed hull panels (4 decorative) | 4 |
| Brass floor compass rose at center | 5 |
| Upper level furniture silhouettes (distant, implied) | 12 |
| Gibbons path breadcrumbs (brass floor arrows) | 4 |
| **TOTAL** | **~300** |

Well under the 800 target. Leaves ~500 walls for future detail passes. The key is that the GLASS ARCS (48 panels + 46 mullions = 94 walls) are the most expensive single element but create the defining visual of the space.

### How to create the illusion of curves

Each glass arc is built from flat panels, rotated with `set_last_rotation()`. The arc follows a circular path:

```c
// Left arc: 24 panels from z=0 to z=36, curving outward
float arc_radius = 30.0f;  // gentle curve
for (int i = 0; i < 24; i++) {
    float angle = -0.4f + i * (0.8f / 24.0f);  // ~46 degree arc
    float px = -arc_radius + cosf(angle) * arc_radius;
    float pz = sinf(angle) * arc_radius + 18;  // center of arc
    float rot_deg = angle * (180.0f / PI);
    add_wall(s, px, 10, pz, 0.08f, 20, 2.2f, void_black);
    set_last_material(s, MAT_GLASS);
    set_last_rotation(s, rot_deg);
}
```

At 960x600, 24 panels per arc reads as a continuous curve. The panels overlap slightly at edges so there are no visible gaps.

### The path Gibbons walks

Gibbons emerges from the elevator area (z = -2), walks along the central axis:

1. **Waypoint 0:** Near spawn (0, 1.6, 0) — "Welcome to the hotel proper."
2. **Waypoint 1:** At the piano (0, 1.6, 12) — pauses, looks at it. "Someone left this here. It doesn't play itself."
3. **Waypoint 2:** Past the sculpture garden (0, 1.6, 24) — "Your suite is just ahead."
4. **Waypoint 3:** At the corridor door (0, 1.6, 36) — gestures. "Through here."

The player can freely explore the atrium. Gibbons waits at each waypoint. The walk is ~40m — at Gibbons' pace (~2.5 m/s with pauses), about 30-40 seconds if the player follows directly. But the space INVITES wandering.

---

## 3. Lighting Plan

### New preset: `LightingPreset_SpaceHotel()`

```c
SceneLighting LightingPreset_SpaceHotel(void) {
    // Massive glass atrium — Earth blue floods from both sides
    // Warm amber pools from overhead fixtures create islands of comfort
    // Key: steep overhead, slightly warm — the chandeliers define the space
    // Fill: Earth blue from the glass walls — the void is always present
    return (SceneLighting){
        .keyDir = Vector3Normalize((Vector3){0.0f, -0.9f, -0.1f}),
        .keyColor = {0.65f, 0.75f, 0.95f},       // Earth blue wash — cool, vast
        .fillDir = Vector3Normalize((Vector3){0.4f, 0.3f, 0.0f}),
        .fillColor = {0.08f, 0.12f, 0.22f},      // deep blue bounce off glass
        .ambient = {0.06f, 0.07f, 0.10f},         // VERY LOW — drama through darkness
        // 4 point lights: 2 warm overhead pools, 1 Earth glow, 1 piano spotlight
        .pointPos = {
            {-8, 18, 10},      // warm chandelier left
            {8, 18, 24},       // warm chandelier right
            {0, 0.5f, 18},     // Earth glow on floor (central)
            {0, 4, 12},        // piano warm pool
        },
        .pointColor = {
            {1.2f, 0.90f, 0.55f},   // amber chandelier
            {1.1f, 0.85f, 0.50f},   // amber chandelier (slightly different)
            {0.3f, 0.55f, 0.85f},   // Earth blue floor wash
            {1.0f, 0.75f, 0.40f},   // warm spotlight on piano
        },
        .pointRadius = {20.0f, 20.0f, 25.0f, 8.0f},
        .reflectionColor = {0.25f, 0.40f, 0.65f},   // blue Earth on glass + brass
        .bouncePos = {
            {0, 0.1f, 18},     // floor bounce (Earth blue)
            {0, 18, 18},       // ceiling bounce (warm from fixtures)
            {-20, 10, 18},     // glass wall bounce (cold void)
        },
        .bounceColor = {
            {0.06f, 0.10f, 0.18f},
            {0.15f, 0.12f, 0.07f},
            {0.03f, 0.05f, 0.10f},
        },
    };
}
```

### Lighting strategy

- **Key light** comes steep from above — mimics the overhead chandeliers. Creates long shadows from columns and furniture that stripe across the floor.
- **Fill** is deep blue — the glass walls let the void in. Without it the space would be too warm and cozy. The blue REMINDS you where you are.
- **Ambient is extremely low** (0.06-0.10) — this makes the warm light pools feel like EVENTS. Walking from shadow to light pool is the rhythm of the space.
- **Point light 0-1** (warm chandeliers) define two "zones" of warmth — the piano area and the sculpture area. Between them is a cooler transition.
- **Point light 2** (Earth glow) is the largest radius — a cool blue wash across the center floor that competes with the warm pools. This is the "Earth is below you" constant.
- **Point light 3** (piano spot) — a tight warm pool that makes the red piano the focal point.

### Earth glow simulation

Six floor decals, graduated in alpha, spreading from the glass walls inward:
- Near glass: `(Color){50, 110, 190, 50}` — strong blue wash
- Mid floor: `(Color){40, 90, 160, 25}` — fading
- Center: `(Color){30, 70, 130, 10}` — barely there

Matching ceiling reflections at 30% of floor intensity.

Two Earth spheres placed behind the glass arcs, at different positions (left arc: lower, rising; right arc: higher) so the planet's position shifts as you walk through — the station is rotating.

### Glass material behavior

`MAT_GLASS` on the arc panels creates a slight transparency and specular sheen. The void_black (`PAL_PORTHOLE`) base color means the glass reads as a dark mirror most of the time — until you look toward Earth, then the blue glow bleeds through. This is the correct behavior: glass should be mostly dark with punctuated brilliance.

---

## 4. Scene Colors and Atmosphere

```c
s->fog_color = (Color){4, 6, 14, 255};   // near-black with blue tint — vast space, not haze
s->fog_density = 0.0004f;                 // VERY low — you can see the full 40m depth
                                          // but distant elements soften into the void
s->surface = SURFACE_MARBLE;              // footstep sound: marble clicks in a cathedral
s->floor_color = PAL_MARBLE_A;
s->ceiling_color = PAL_HULL;
```

The fog density is critical. At 0.001 (space lobby value), a 40m room would fog out. At 0.0004, the far wall is slightly hazed — creating aerial perspective without losing the view.

---

## 5. Furniture and Landmarks

### The Red Piano (center-left, z = 12)

The single most important object in the atrium. A grand piano in Godard red (`PAL_RED`), placed off-center on the main axis. No one is playing it. It exists as a BOLD COLOR EVENT in the neutral space.

Construction (~6 walls):
- Body: 2.0m x 0.4m x 1.5m, `PAL_RED`, `MAT_WOOD`
- Lid (propped open): angled panel, same red
- 3 legs: brass cylinders
- Keys: thin white+black strip (visible at 960x600 as contrast band)

The piano has a light panel beneath it — a warm pool that makes it glow on the marble floor. It is the warmest point in the room.

### The Blue Sculpture (center-right, z = 24)

Three stacked/offset geometric forms in `PAL_BLUE` — a cube, a tilted cylinder, a sphere. Yves Klein blue against the grey hull and cream. Placed on a low brass plinth.

Approximately 4-5 walls. Pure bold shape — reads as a Noguchi-meets-Judd installation.

### Planted Terraces

Four raised beds (2 per side) with stylized vegetation: green cubes and spheres on soil-colored bases. These read as "someone tends this space." Life persists in orbit.

### Seating Clusters

Three clusters of 2-3 seats each, scattered along the atrium:
1. Near spawn: two cream chairs facing each other (conversation area)
2. Near piano: a navy sofa facing the glass wall (Earth-watching)
3. Near sculpture: a single Godard red armchair (solitary contemplation)

Each cluster has a small brass side table and a warm light panel overhead.

---

## 6. The Upper Levels (Inaccessible, Visual Only)

Three balcony levels at y = 5, 10, 15. Each is a simple platform (hull-colored) with a brass railing, extending 3m from the glass walls.

Key details on upper levels (visible but distant):
- **Level 1 (5m):** Warm light pool, empty chair, a book on a table. Someone was reading.
- **Level 2 (10m):** A single warm lamp. No furniture visible — just the glow.
- **Level 3 (15m):** Near-dark. A faint silhouette of a coat rack. The hotel extends beyond what you can see.

These are cheap (6 platform walls + 6 railings + a few detail objects = ~20 walls) but create an enormous sense of vertical scale.

---

## 7. Floating Objects (Zero-G Storytelling)

Consistent with space_lobby and space_suite — objects drift in micro-gravity:

- A napkin tumbling at y = 6 (above the player, catching chandelier light)
- A champagne cork at y = 3.5 (someone celebrated)
- A single rose petal at y = 8 (from the planted terraces — nature adapting)
- A room service menu at y = 2 (brass-colored, spinning implied)
- A pen near the piano at y = 4 (the pianist left in a hurry)

Each is 1-2 walls. Total: ~10 walls.

---

## 8. Game State Integration

### New enum value

```c
// In ev_types.h, between STATE_SPACE_LOBBY and STATE_SPACE_CORRIDOR:
STATE_SPACE_HOTEL,
```

### Flow change

Current:
```
HYPERSPACE → SPACE_LOBBY → (elevator) → SPACE_CORRIDOR → SPACE_SUITE
```

Proposed:
```
HYPERSPACE → SPACE_LOBBY → (elevator) → STATE_SPACE_HOTEL → SPACE_CORRIDOR → SPACE_SUITE
```

The elevator ride from the lobby now delivers you to the hotel atrium instead of directly to the corridor. The corridor entrance is at the FAR END of the atrium — Gibbons leads you through.

### Transition mechanics

**Entering:** Hard cut from elevator (same as current `STATE_ELEVATOR` with `elevator_to_corridor = true`). The elevator deposits you at z = -2, facing +z (into the atrium).

**Exiting:** Player reaches the corridor door at z = 36. Gibbons must have arrived there first (he waits). Proximity trigger (dist < 2.0f) → `transition_to(STATE_SPACE_CORRIDOR)`.

### Audio

- `PlayArrivalThump()` — deep bass impact on arrival
- `PlayAirlockHiss()` — pressurization
- `PlayEarthPresence()` — 30Hz sub-bass (Earth is visible everywhere here)
- New ambient drone: `DRONE_SPACE_HOTEL` or reuse `DRONE_SPACE_LOBBY` (similar acoustic: vast, reverberant, with warm undertones)
- Distant piano note: a single sustained note every 15-20 seconds. Not a melody. Just the piano's presence. (`PlayCommsChatter` equivalent but tuned as a piano overtone.)
- Footstep echo: marble surface in a 20m-ceiling space = pronounced reverb tail

### Player physics
```c
player.gravity_mult = 0.4f;   // orbital gravity persists
```

### Post-FX
```c
set_exposure(-0.08f);          // slightly darker than lobby — more dramatic
SetPostFXGrain(&postfx, 0.35f);
SetPostFXCA(&postfx, 2.0f);
```

---

## 9. Scene Function Signature

```c
void build_space_hotel(Scene *s) {
    memset(s, 0, sizeof(Scene));
    // BOUNDS: 50m x 42m, curved glass atrium — the Gehry hotel
    // Bilbao in orbit. The impossible space you didn't know you were inside.
    s->surface = SURFACE_MARBLE;

    // ... geometry ...

    s->spawn = (Vector3){0, 1.6f, -2};        // elevator deposit point
    s->exit_pos = (Vector3){0, 1.6f, 36};     // corridor door at far end
    s->has_exit = true;

    s->fog_color = (Color){4, 6, 14, 255};
    s->fog_density = 0.0004f;
}
```

---

## 10. Moment-by-Moment Experience

**0s — Hard cut from elevator.** Black. Silence. Then:

**0.5s — Eyes open.** You are standing at the edge of the largest space you have seen in this game. The ceiling is 20m above. Glass walls curve away on both sides, and through them: EARTH. Blue, enormous, slowly rotating. The floor is marble. The air is warm amber from distant chandeliers.

**2s — Gibbons speaks.** "Welcome to the hotel proper." He begins walking. You can follow or not.

**5s — You take a step.** Your footstep echoes. The marble surface rings in a space this tall. You realize you can HEAR the scale.

**10s — The piano.** Red. Impossibly red against everything else. A warm pool of light around it. Gibbons pauses here. "Someone left this here. It doesn't play itself." You approach. The warmth is palpable — the lighting shifts as you enter the point light's radius.

**20s — The glass walls.** You walk toward one. Earth fills the panel. You can see the curvature of the glass — the mullions break it into vertical strips. Between the panels, the void is absolute black. Stars are pinpricks. The glass is cold (blue light). You are warm (amber behind you). This contrast IS the scene.

**30s — The upper levels.** You look up. Balconies. A warm light on level 1. An empty chair. Who sits there? How do they get there? The hotel is larger than you thought.

**40s — The far end.** Gibbons waits by a door. "Your suite is just ahead." You can still turn back. The piano is behind you, glowing. Earth is everywhere.

**Exit.** You step through. Hard cut to corridor. The atrium is gone. You will never return to it. (Or will you — mezzanine wardrobe secret, if preserved.)

---

## 11. Risks and Mitigations

| Risk | Mitigation |
|------|-----------|
| 50m room reads as empty at 960x600 | Fog density at 0.0004 adds aerial perspective. Planted terraces + seating clusters break up the floor. Upper levels add vertical fill. |
| Glass panels look like flat walls, not curves | 24 panels per arc at ~7.5 degree increments. At render distance, the faceting is invisible. Brass mullions between panels sell the "glass house" read. |
| Too many walls if details expand | Core structure is ~300 walls. Even at 600 walls we have headroom. Discipline: no tiny props. Everything must read at 3+ pixels. |
| Player gets lost in large space | Gibbons walks the path. Brass floor inlays point toward the exit. The corridor door at z=36 has a warm light panel above it — a beacon. |
| Earth glow overwhelms warm pools | Earth glow decals are low-alpha (10-50). Point lights for warm pools are high-radius (20). The warm WINS at close range; blue wins at distance. |
| 20m ceiling too tall for lighting shader | Shadow map at 2048 covers it. Point light radius at 20-25 reaches floor from y=18. Ambient is low so shadow falloff is dramatic, not problematic. |

---

## 12. Open Questions

1. **Should the piano be interactable?** Pressing E could play a single chord. Or it could be purely visual — a Godard color event, not a game mechanic.
2. **Secret area?** The upper levels could have a parkour route (wall run the glass panels → mantle balcony railing). Reward: a closer view of Earth. This would use ~30 additional walls for stepping stones.
3. **NPC on upper level?** A distant figure watching from level 2 — another geometric cube-person, sitting in a chair, never moving. You cannot reach them. They are evidence of other lives.
4. **Return visit?** If the player returns via the wardrobe secret (STATE_ROOM → STATE_SPACE_LOBBY → mezzanine), should they be able to reach the hotel atrium from the lobby? This would require a door/connection between lobby and atrium.
5. **Piano music bleed?** When the player is in STATE_SPACE_CORRIDOR, should they hear a very faint piano note through the walls? This would connect the spaces acoustically.
