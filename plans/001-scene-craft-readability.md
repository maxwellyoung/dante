# Plan 001: Fix the remaining scene-readability defects (corridor blemishes, suite/room QA fails)

> **Executor instructions**: Follow this plan step by step. Run every
> verification command and confirm the expected result before moving on. If
> anything in "STOP conditions" occurs, stop and report — do not improvise.
> When done, update this plan's row in `plans/README.md`.
>
> **Drift check (run first)**:
> `git diff --stat 04f970a..HEAD -- ev_engine/src/scene.c ev_engine/src/scene_corridor.c ev_engine/src/lighting.c ev_engine/src/scene_suite.c ev_engine/src/scene_hotel.c`
> If any in-scope file changed, compare "Current state" excerpts against live
> code first; on mismatch, STOP.

## Status

- **Priority**: P1
- **Effort**: M
- **Risk**: LOW (visual tuning; QA screenshots verify every change)
- **Depends on**: none
- **Category**: tech-debt (visual craft)
- **Planned at**: commit `04f970a`, 2026-07-05

## Why this matters

The space corridor was just rebuilt (geometry straightened, Kubrick light
pools) and now *reads as a hallway* for the first time, but three visible
defects remain in its hero shot, and two scenes (`room`, `space_suite`) FAIL
the QA contrast/luma rubric. These are the last objectively-measurable visual
defects on the shipping path (ev_engine/CUT.md defines the spine). Fixing
them is prerequisite to the look-lock pass (plan 002) — tune the look on
clean scenes, not broken ones.

## Current state

All work happens in `/Users/maxwellyoung/Development/dante/ev_engine`.

- `src/scene.c` — `build_space_corridor()` (search for that name; ~line 4410).
  Was an arc, straightened 2026-07-05: segments now march along +z, corridor
  spans z 0..32, width 4.5 (x ±2.25). Contains porthole/window assemblies
  whose positions derive from formerly-arc coordinates.
- `src/scene_corridor.c` — scene load/update. `set_exposure(0.02f)` at load.
- `src/lighting.c` — `LightingPreset_SpaceCorridor()`: dim warm key, 4 amber
  point pools at z = 2/9/16/24, radius 5.5–7.
- QA: `EV_MUTE=1 make -j8 qa` prints one line per scene, e.g.
  `[QA] space_corridor  PASS  walls:400 ... luma:84  contrast:1.2:1 ... zfight:0`
  Current values: corridor luma 84 / contrast 1.2; `room` FAIL; `space_suite`
  FAIL (contrast 1.2:1 — rubric wants more tonal separation).
- Known corridor hero-shot defects (view `qa/screenshots/space_corridor_hero.png`
  after any `make qa`):
  1. Two large BLACK faceted cones/octagons intrude at mid-wall height —
     porthole viewport shrouds positioned for the old arc. Find them in
     `build_space_corridor` near `a_win` / `win_cx` variables (search `a_win`).
  2. The far end wall renders as a flat bright cream rectangle — no trim, no
     door, reads unfinished. The suite door should be there (the exit is at
     z≈32: `s->exit_pos`).
  3. Left wall shows a patch of dithered checker squares (search
     `imp_cx` / "impossible" in the build function — a dither-wall experiment
     now mispositioned).
- Debug views exist: `EV_QA_VIEW=albedo` (flat colors, no lighting),
  `EV_QA_VIEW=normals`, `EV_QA_VIEW=key` (key light only) — set as env with
  any `make qa` run. Use albedo to distinguish authored-color problems from
  lighting problems. `EV_QA_STYLE=<0-9>` renders QA under a visual style.
- Convention: geometry is code — `add_wall(s, x, y, z, w, h, d, color)` etc.
  (see `src/scene.h`). Flush overlays must be decals (`add_wall_decal` or
  `set_last_decal`) but an automated pass (`scene_auto_decal`, runs on every
  scene load) already handles thin trim; keep `zfight:0` in QA output.
- Reading scene screenshots: after `make qa`, PNGs are in `qa/screenshots/`
  (`<scene>_hero.png` is the main angle).

## Commands you will need

| Purpose | Command | Expected on success |
|---|---|---|
| Build | `cd ev_engine && EV_MUTE=1 make -j8` | exit 0, no warnings |
| Tests | `cd ev_engine && make test` | `31 tests, 0 failed` (and test_main passes) |
| QA + screenshots | `cd ev_engine && EV_MUTE=1 make -j8 qa` | per-scene `[QA]` lines; exit code may be 1 while any scene FAILs — that's the signal you're fixing |
| Albedo view | `EV_MUTE=1 EV_QA_VIEW=albedo make -j8 qa` | screenshots show flat colors |
| Static analysis | `cd ev_engine && make check` | `All files clean.` |

## Scope

**In scope** (only files you may modify):
- `ev_engine/src/scene.c` (only within `build_space_corridor`, `build_space_suite`, and the Paris-hotel `build_*` used by `room` — identify via `scene_registry.c` load functions)
- `ev_engine/src/scene_corridor.c`, `ev_engine/src/scene_suite.c`, `ev_engine/src/scene_hotel.c` (exposure/postfx values only)
- `ev_engine/src/lighting.c` (only `LightingPreset_SpaceCorridor`, `LightingPreset_SpaceSuite`, and the preset used by `room`)

**Out of scope** (do NOT touch):
- `src/render.c` visual styles table, the post-FX shader, `src/dialog*.c`,
  `src/player.c`, the Makefile, any prototype scene, `qa/` scripts.
- Do not re-introduce any curvature to the corridor. The straightening is
  deliberate (see commit `04f970a` message).

## Git workflow

- Work on the current branch (`codex/ev-engine-hardening`) unless the operator
  says otherwise. One commit per step, imperative messages, NO AI attribution
  lines (repo rule — see `/Users/maxwellyoung/Development/CLAUDE.md`).

## Steps

### Step 1: Remove or re-seat the corridor porthole shrouds

In `build_space_corridor`, locate the window/porthole assembly (search
`a_win`). Its x-position derives from zeroed arc math. Either (a) re-seat the
porthole assemblies flush INTO the side walls at x = ±2.25 (outer face just
outside the wall plane, opening flush with the inner face, z positions at
pool troughs z≈5.5 and z≈12.5), or (b) if re-seating produces anything
visually ambiguous, delete the porthole assemblies entirely — the corridor
reads fine without them and CUT.md prefers cut over broken.

**Verify**: `EV_MUTE=1 make -j8 qa` then view
`qa/screenshots/space_corridor_hero.png` — no black cones/octagons anywhere
in frame. `[QA] space_corridor` still shows `zfight:0`.

### Step 2: Finish the far end wall

At the corridor's far end (z≈32), the exit wall is a bare cream rectangle.
Add: a door-sized panel (≈1.1 × 2.2) centered on the exit position with the
same navy/blue door color used by the guest doors earlier in the build
(search `door` in the function for the exemplar), brass trim strip
(`PAL_BRASS` from `src/palette.h`), and a small warm light pool above it —
either move the 4th preset point light (z 24 → 30) in
`LightingPreset_SpaceCorridor` or reuse an existing point via
`SetPointLightIdx` in `scene_corridor.c` load. The end wall is the
destination; it should be the warmest thing in frame (wayfinding-by-light).

**Verify**: `make qa`; hero screenshot shows a framed door at the vanishing
point with warm light; `zfight:0` holds.

### Step 3: Fix or remove the dither patch on the left wall

Search `imp_` in `build_space_corridor`. Whatever this "impossible room"
dither experiment was, it currently renders as random checker squares
mid-wall. If its intent is documented nearby (comments), re-seat it; if not,
remove the block. Note what you did in the commit message.

**Verify**: hero screenshot: left wall is clean paneling; QA line unchanged
or improved.

### Step 4: Clear the `room` QA FAIL

Run `EV_MUTE=1 make -j8 qa 2>&1 | grep '\[QA\] room'` and read the appended
issue lines (the QA prints the failing heuristic beneath the scene row).
Typical cause: luma/contrast band. Adjust ONLY exposure/lighting preset/fog
values for that scene (its load function is in `scene_registry.c` →
`room_load` in `src/scene_hotel.c`) until PASS. Do not restructure geometry.

**Verify**: `[QA] room  PASS`.

### Step 5: Clear the `space_suite` QA FAIL

Same procedure. The suite preset is `LightingPreset_SpaceSuite` in
`src/lighting.c` (comment says "Keep the room readable first"). Contrast
1.2:1 means everything sits in one tonal band — raise key/pool contrast
rather than overall brightness (darken troughs slightly + brighten pools
slightly). The suite's warmth arc (`SetPostFXWarmth` rises as tasks complete)
must keep working — don't touch `suite_apply_ritual_progress_for_qa` or task
logic.

**Verify**: `[QA] space_suite  PASS`, and
`EV_MUTE=1 make -j8 qa` exits 0 **except** for prototype-scene FAILs
(proto_movement / proto_shooter are exempt per `ev_engine/CUT.md` — if they
are the only FAILs left, this plan's QA goal is met).

## Test plan

No unit tests apply (visual work). The test battery is:
- `make test` still passes (31 dialog tests + struct checks) — proves no
  header/ABI breakage.
- `make qa` per-scene table: corridor/suite/room PASS, `zfight:0` everywhere.
- Visual inspection of `space_corridor_hero.png`, `space_suite_hero.png`,
  `room_hero.png` against the defect list above.

## Done criteria

- [ ] `EV_MUTE=1 make -j8` exits 0 with zero warnings
- [ ] `make test` → `31 tests, 0 failed`
- [ ] `make qa` rows: `space_corridor PASS`, `space_suite PASS`, `room PASS`, all `zfight:0`
- [ ] Corridor hero: no black shrouds, finished end door, clean left wall
- [ ] `git status` shows only in-scope files modified
- [ ] `plans/README.md` row updated

## STOP conditions

- `build_space_corridor` no longer contains `a_win`/`imp_` code (drift).
- Any change causes `zfight:` > 0 in a spine scene and a second attempt
  doesn't clear it.
- Clearing `room`/`space_suite` FAILs appears to require geometry
  restructuring rather than lighting/exposure tuning.
- The QA rubric itself looks wrong for a scene (e.g. suite is *supposed* to
  be low-contrast pre-ritual): report, don't chase the metric — the repo
  owner has explicitly rejected QA-grade-chasing (`AUDIT_2026-07.md` P0#2).

## Maintenance notes

- Plan 002 (look lock) re-tunes exposure per scene under the locked style;
  keep your exposure changes minimal so 002 has room.
- Reviewer should eyeball the three hero PNGs, not just the QA numbers.
