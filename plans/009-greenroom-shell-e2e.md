# Plan 009: The Green Room greybox — shell pipeline end-to-end

> **Executor instructions**: Follow step by step; verify each step. On any
> STOP condition, stop and report. Update `plans/README.md` when done.
> NOTE: Blender authoring steps may require the operator/owner if no Blender
> is reachable — the plan includes a code-only fallback (Step 1B).
>
> **Drift check (run first)**:
> `git diff --stat 04f970a..HEAD -- ev_engine/src/scene_as_greenroom.c ev_engine/src/model_registry.c ev_engine/assets/dialogue/aftershow.rules`
> On mismatch with "Current state", STOP.

## Status

- **Priority**: P1 (Phase 3 core)
- **Effort**: L
- **Risk**: MED (first end-to-end use of the shell system — it has never
  been proven; discovering its gaps is half the value)
- **Depends on**: plans/008-aftershow-scaffold-signal-import.md
- **Category**: direction (AFTERSHOW vertical slice, part 2)
- **Planned at**: commit `04f970a`, 2026-07-05

## Why this matters

The Green Room is AFTERSHOW's central space: the podcast studio the
protagonist's mind has constructed from eight years of audio-only evidence.
Five props anchor memories (fridge, chair, window, corkboard, mics); the
room re-renders colder as suspicion facts rise. This plan builds it as a
playable greybox and, in doing so, proves the engine's shell pipeline
(Blender GLB visual + invisible collision walls) end-to-end for the first
time — the environment-quality unlock for everything after boxes.

## Current state

Paths relative to `/Users/maxwellyoung/Development/dante/ev_engine`.

- Plan 008 delivered `src/scene_as_greenroom.c` (grey box + SIGNAL entry),
  `assets/dialogue/aftershow.rules`, and dlg memory seeded with
  `suspicion` (0–5), `filed_*`, `prologue_imported`.
- **Shell system** (never e2e-tested; docs in `ev_engine/CLAUDE.md` "Shell
  System" and `GEHRY_DESIGN.md`):
  - `add_shell(s, "model_name", x,y,z, sx,sy,sz, rot, MAT_*, color)` —
    renders a registry GLB with no collision (`src/scene.h:71`).
  - `add_collision_wall/floor/ceiling(...)` — invisible physics-only boxes.
  - Blender authoring: `scripts/ev_shell_workbench.py` (`shell_setup`,
    `collision_*`, `shell_export` — export prints the C calls to paste).
    Blender runs locally headless via `scripts/mcp_model.sh full <script> <name>`
    or remotely (Mac Mini, port 9877; `PREFER_REMOTE_BLENDER=1`).
  - Aesthetics are LOCKED in Blender by `ev_style.py` — run it first in any
    Blender session (repo rule).
- **Model registry**: `src/model_registry.c` — 25 entries, hard cap
  `MAX_MODEL_ASSETS 32` in game_ctx.h. Entry pattern: name, path
  (`assets/<name>.glb`), kind, `startup_load`, VAO budget, status. A
  validation script runs in QA (`scripts/validate_model_registry.py` —
  requires source scripts for active GLB assets, so the Blender script must
  be committed as `scripts/model_greenroom_shell.py`).
- **Dialogue**: props speak via `dlg_game_speak_about(who, "interact",
  "<object>", step)`; the suite's E-press dispatch in `src/scene_suite.c`
  (search "Two Bots One Wrench") is the exemplar to copy for the Green
  Room's interact loop. `InteractObject`s are added via `add_object(&scene,
  x,y,z, "name", color, max_steps)` (see `scene_suite.c:193`).
- **Suspicion re-render exemplar**: the Twine prologue's rules
  (`/Users/maxwellyoung/Development/aftershow/prologue/aftershow.twee`,
  rules `int_fridge` vs `int_fridge_cold`) — port those five props' warm and
  cold lines into `aftershow.rules` (same criteria idea:
  `concept=interact object=fridge` baseline; `... suspicion>=3` overrides).
- Speaker: `who narrator` (the fan's interiority). "gibbons" must NOT appear
  in AFTERSHOW rules.

## Commands you will need

| Purpose | Command | Expected |
|---|---|---|
| Build/boot | `EV_MUTE=1 make -j8 dev SCENE=STATE_AS_GREENROOM` | room loads |
| Registry validation | `python3 scripts/validate_model_registry.py` | PASS |
| Blender build (if available) | `./scripts/mcp_model.sh full scripts/model_greenroom_shell.py greenroom_shell` | GLB deployed to assets/ |
| Tests / QA | `make test` / `EV_MUTE=1 make -j8 qa` | green / table clean |

## Scope

**In scope**:
- `ev_engine/src/scene_as_greenroom.c` (replace greybox internals)
- `ev_engine/src/model_registry.c` (+1 entry `greenroom_shell`)
- `ev_engine/scripts/model_greenroom_shell.py` (create — Blender source)
- `ev_engine/assets/greenroom_shell.glb` (generated)
- `ev_engine/assets/dialogue/aftershow.rules` (prop + cold-variant rules)
**Out of scope**: EV scenes, ev.rules, the shell system engine code itself
(if it's broken, that's a STOP+report — fixing it is its own plan), lighting
shader.

## Steps

### Step 1A: Author the shell (Blender path)

Write `scripts/model_greenroom_shell.py`: run `ev_style.py` conventions,
then a one-room studio shell ~7×5×3m — slightly wrong proportions ON
PURPOSE (a room remembered, not measured): ceiling a touch low, one wall
subtly non-parallel. Include window opening on one long wall. Export via the
workbench pattern (`shell_setup("greenroom", width=7, depth=5, height=3)`,
model in the Shell collection, `collision_floor/walls`, `shell_export`).
Deploy with `mcp_model.sh full`. Registry entry: `greenroom_shell`,
`startup_load=false`.

### Step 1B: FALLBACK if no Blender is reachable

Build the room as code boxes (`add_wall`) with the same wrongness
(non-parallel wall = one wall with `rotation_y` 4°), and record in the
commit + plans/README that the shell e2e proof is still owed. Do NOT silently
skip the registry/scripts steps in that case.

**Verify (either path)**: scene boots; player collides with all walls
(walk each direction); if 1A: `validate_model_registry.py` PASS and the GLB
renders with the lighting shader applied.

### Step 2: The five props

Add five `InteractObject`s (`add_object`): `fridge`, `chair`, `window`,
`corkboard`, `mic`, each with simple placeholder geometry (a box/cylinder at
its spot — the shell is the star, props are markers for now). Copy the
suite's E-press dispatch loop into `as_greenroom_update` (facing check +
`dlg_game_remark`-equivalent — but use
`dlg_game_speak_about("narrator", "interact", obj->name, obj->step)`
directly since Gibbons must never speak here).

**Verify**: pressing E at each prop prints a `DIALOG:` log line (rules in
Step 3) or silence with no crash.

### Step 3: Port the prop rules (warm + cold)

Into `assets/dialogue/aftershow.rules`, port from the Twine prologue the
five warm memories (fridge/chair/window/corkboard/mic — e.g. fridge: the
ep-212 seltzer hiss memory) and the three cold variants gated
`suspicion>=3` (fridge unplugged; chairs further apart; the no-train
window). Keep each ≤ 2 sentences (subtitle length). Respect the format
documented at the top of ev.rules; `who narrator` throughout; `norepeat` on
cold variants.

**Verify**: boot with `EV_SIGNAL=SIGNAL-SSSSS-5-PLAYED` → E on fridge gives
the COLD line; boot with `...-0-...` → warm line. (`EV_SIGNAL` from plan
008.)

### Step 4: Light it like evidence

One warm key pool over the desk/mics area, cold spill from the window,
darkness elsewhere (copy the corridor preset structure in `src/lighting.c` —
add `LightingPreset_GreenRoom()` and a header decl beside the others). If
suspicion≥3 at load, drop ambient by a third in `as_greenroom_load` (read
via `dlg_mem_get("suspicion", &found)`).

**Verify**: hero-style screenshot readable in both suspicion states — add
the scene to `qa_scenes[]` (one angle) and check `[QA] as_greenroom` line +
PNG; `zfight:0`.

## Test plan

- Manual interact pass on all five props in both suspicion states.
- QA row + screenshot for the scene; registry validation; `make test`.

## Done criteria

- [ ] Green Room boots via dev, shell (or documented fallback) + collision
- [ ] Five props speak; ≥3 have cold variants that win at suspicion≥3
- [ ] `LightingPreset_GreenRoom` exists; QA row PASS, `zfight:0`
- [ ] Registry validates (1A) or fallback debt recorded (1B)
- [ ] `make test` green; `plans/README.md` row updated

## STOP conditions

- `add_shell` renders nothing or collides wrongly after one honest debug
  attempt with `EV_QA_VIEW=albedo` — the shell system itself is broken;
  report findings (that's the e2e proof doing its job).
- Model registry is at its 32-slot cap.
- The prologue prop rules can't express something (missing fact/feature in
  the rules engine) — report the gap, don't hack the engine here.

## Maintenance notes

- The real (non-placeholder) prop models come later with the Blender
  collaborator — placeholders keep slots and names stable.
- Plan 010 wires this room into the slice night flow; keep the scene's
  entry/exit assumptions minimal (spawn + one exit trigger).
