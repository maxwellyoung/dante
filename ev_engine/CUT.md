# THE CUT — Endearing Void, tight

*The complaint this answers: "it's very uneasy to play... I would much rather a
tight Thirty Flights of Loving style experience with a beginning middle and
end." Correct. This is the shipping scope. Everything not on the spine is
dev-gated or dead.*

## The shape

**12–15 minutes. One unbroken line. No dead ends, no backtracking, no menus
mid-flow. Jump cuts do the connective tissue** (`hard_cut_to` — Blendo grammar,
already built). The player never wonders where to go; they wonder what they're
feeling.

```
BEGINNING — Arrival (≈4 min)
  TITLE → CAR (backstory choices) → DRIVING → HOTEL_EXT → LOBBY
  → ELEVATOR → HYPERSPACE → SPACE_LOBBY (the Earth reveal)

MIDDLE — Intimacy (≈6 min)
  GLASSHOUSE → SPACE_CORRIDOR (Gibbons' parallel lives)
  → SPACE_SUITE (the ritual: lamp, champagne, desk, bed — the room warms)

END — Fracture / Release (≈4 min)
  BALCONY → PARIS_DREAM (the fracture) → CLEANED_SUITE (the hotel reset the
  room; the trip never happened, institutionally) → BED (permutation line
  fires here) → STARS → MONTAGE (Thirty Flights rapid cuts) → RETURN_TAXI
  → TITLE (the loop closes) 
```
*(Endgame order verified against the actual wiring 2026-07-05; STARS was
orphaned — bed cut straight to montage — now restored: bed → stars → montage.)*

Every scene on the spine earns its place by advancing the one story: booked
for two, arrived as one.

## Keep / cut / gate

| State | Verdict |
|---|---|
| TITLE, CAR, DRIVING, HOTEL_EXT, LOBBY, ELEVATOR, HYPERSPACE | **Spine** |
| SPACE_LOBBY, GLASSHOUSE, SPACE_CORRIDOR, SPACE_SUITE | **Spine** |
| BALCONY, PARIS_DREAM, CLEANED_SUITE, BED, STARS, MONTAGE, RETURN_TAXI | **Spine** |
| HALLWAY, ROOM, BATHROOM (Paris hotel) | **Cut from flow** — dev keys only. Orphaned, high defect count, not on the spine |
| PROTO_LAB / MOVEMENT / SHOOTER / PUZZLE | Dev keys only (already are). The Quake feel lives here |
| SHELL_TEST | Dev only |

## The feel contract (changes landed today)

- **Movement**: `physics_narrative()` on every non-prototype scene — ground
  accel 14 / friction 11 (intent, not drift), no air-strafing, no bhop,
  gravity 24, step height 0.28 (stairs yes, furniture no), bob/tilt halved,
  speed-FOV and speed-shake off. The Quake profile still runs the prototypes.
- **Gravity**: the permanent 0.3–0.5 moon-float in space scenes is gone.
  Station gravity is 0.85–0.9 (subliminal lightness). The low-g moment is now
  an arrival *beat*: space lobby starts 0.4 and settles to 0.9 as
  PlayGravitySettle plays.
- **Z-fighting**: `scene_zfight_report()` counts coplanar same-normal face
  pairs per scene in QA (`zfight:` column); `scene_auto_decal()` runs on
  every scene load — thin flush trim (≤0.12) auto-marks as decal, exact
  duplicate walls deactivate. 819 detected → 457 auto-fixed. Residuals are
  mostly perpendicular abutments and room-boundary end faces; fix by hand
  only where visible, or let shell rebuilds retire them.
- **Collision**: substepping already prevents tunneling; the narrative step
  height stops the walk-onto-furniture bug. Remaining known issue: steppable
  skip can pop the camera on 0.28+ ledges — watch in playtest.

## Definition of done

1. One sitting, start → credits, no debug keys, no softlocks: three external
   playtests (playtest build exists: `make playtest`).
2. `zfight:` ≤ 5 on every spine scene at QA.
3. The track (bed ritual piece) — the one content gap.
4. Every spine scene reviewed once under the shipping visual style.

## What "a nice gameplay loop" means here

EV's loop is the ritual: notice → touch → the room answers → the room warms.
It's a mood loop, not a mechanics loop — that's the Thirty Flights lineage,
and at 15 minutes it doesn't need more. The mechanics-loop game is AFTERSHOW,
and it inherits this engine tightened.
