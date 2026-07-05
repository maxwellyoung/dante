# Plan 002: Apply the locked visual style as the shipping default

> **Executor instructions**: Follow step by step; verify each step. On any
> STOP condition, stop and report. Update `plans/README.md` when done.
>
> **Drift check (run first)**:
> `git diff --stat 04f970a..HEAD -- ev_engine/src/render.c ev_engine/src/main.c ev_engine/qa/lookdev/LOOKLOCK.md`
> On in-scope drift, re-verify "Current state" excerpts; mismatch = STOP.

## Status

- **Priority**: P1
- **Effort**: S
- **Risk**: MED (changes the look of every frame; fully reversible — one int)
- **Depends on**: plans/001-scene-craft-readability.md; **and a human
  decision** (see gate in Step 1)
- **Category**: direction (visual identity)
- **Planned at**: commit `04f970a`, 2026-07-05

## Why this matters

The game currently ships the "Clean Hotel/16mm" look (style index 0). A
Grickle/Puzzle-Agent style (luma cel bands + paper grain + inked contrast,
style index 9, toggle Shift+0) was built and compared side-by-side; the
repo's standing recommendation (`ev_engine/qa/lookdev/LOOKLOCK.md`) is to
lock Grickle because it makes the blockout geometry read as deliberate
illustration. Locking one look unlocks the per-scene lighting polish pass —
tuning light under a look that later changes is wasted work.

## Current state

- `ev_engine/src/render.c` — `visual_styles[STYLE_COUNT]` table; index 9 is
  `{"Grickle", 0.85f, 0.0f, 0.95f, 0.5f, 0.45f, 0.05f, {1.03f,1.0f,0.94f}, 0.15f,0.0f,0.0f, 0, 1, 0.5f, 7.0f}`
  (last field = `cel`, the luma-band count).
- `ev_engine/src/game_ctx.h` — `int current_style;` inside `GameCtx` (zero-
  initialized → style 0 is the current default). There is NO explicit
  assignment of a default style at startup in `src/main.c` — the C zero-init
  is the default. Verify with: `grep -n "current_style = " ev_engine/src/main.c`
  (expect only the Shift+number handler and settings-menu cycling, no init).
- `ev_engine/qa/lookdev/LOOKLOCK.md` — the decision document. Comparisons in
  `qa/lookdev/compare/`. Regenerate: `EV_MUTE=1 make qa` then
  `EV_MUTE=1 EV_QA_STYLE=9 make qa` (env renders the whole QA pass in a style).
- Known Grickle cost (documented in LOOKLOCK.md): dark scenes crush — the
  cel bands eat shadow detail. Mitigation is a fill-floor lift per dark scene.
- Styles persist across scenes and remain switchable via Shift+1..9, Shift+0
  (debug keys ship — repo rule).

## Commands you will need

| Purpose | Command | Expected |
|---|---|---|
| Build | `cd ev_engine && EV_MUTE=1 make -j8` | exit 0 |
| QA under a style | `EV_MUTE=1 EV_QA_STYLE=9 make -j8 qa` | `[QA]` table renders in that style |
| Tests | `make test` | 31 tests, 0 failed |

## Scope

**In scope**:
- `ev_engine/src/main.c` (one default-style assignment at startup)
- `ev_engine/src/render.c` (Grickle row tuning ONLY if a scene demands it)
- `ev_engine/src/lighting.c` + per-scene `set_exposure` calls in
  `ev_engine/src/scene_*.c` (dark-scene fill lift under the locked look)
- `ev_engine/qa/lookdev/LOOKLOCK.md` (record the final decision)
- `ev_engine/CLAUDE.md` (update the "Default" row of the style table)

**Out of scope**: the postfx shader source; the other 9 style rows; dialogue,
physics, scene geometry.

## Steps

### Step 1: THE GATE — read the decision

Open `ev_engine/qa/lookdev/LOOKLOCK.md`. Proceed ONLY if it contains an
explicit decision line from the repo owner (e.g. "DECISION: Grickle" or
"DECISION: 16mm"), or the operator dispatching you states the decision.
**If absent: STOP.** Do not lock a look on the recommendation alone — the
owner said he'd play both first.

### Step 2: Set the default style

If the decision is 16mm/default: skip to Step 4 (record + close out).
If Grickle: in `src/main.c`, immediately after `game_ctx_init(&g);` in
`main()`, add `g.current_style = 9;  // locked look — Grickle (LOOKLOCK.md)`.
Keep Shift+N switching untouched.

**Verify**: `EV_MUTE=1 make -j8 qa` → screenshots in `qa/screenshots/` show
cel-banded rendering without setting `EV_QA_STYLE`.

### Step 3: Dark-scene fill pass under the locked look

For each spine scene whose hero screenshot crushes to illegibility under the
locked look (inspect all `qa/screenshots/*_hero.png`; expected offenders:
`space_corridor`, `bed`, `hyperspace`, `balcony`): raise the scene's
`.ambient` floor in its `LightingPreset_*` (small steps, ~+0.03) and/or its
`set_exposure` (+0.02..0.06) until the hero shot holds silhouette detail in
the darkest quadrant. Iterate: edit → `EV_MUTE=1 make -j8 qa` → view PNG.
Do not raise any scene's QA luma above ~120 (the mood is dark-warm, not lit).

**Verify**: every spine-scene hero PNG shows readable primary silhouettes;
`[QA]` rows for spine scenes all PASS; `zfight:0` everywhere.

### Step 4: Record and document

Append to `LOOKLOCK.md`: the decision, date, default flipped (or kept), and
which scenes received fill lifts with values. Update the style table in
`ev_engine/CLAUDE.md` (mark the locked style as "Default. LOCKED <date>").

**Verify**: `git diff` shows only in-scope files; `make test` passes.

## Test plan

- `make test` (ABI/dialog intact), `make check` clean.
- Full QA pass at the locked default; visually flip through every
  `*_hero.png` once — this is the shipping look now.

## Done criteria

- [ ] Decision line exists in LOOKLOCK.md and the default matches it
- [ ] `make qa` (no env overrides) renders the locked look
- [ ] All spine scenes PASS QA; `zfight:0`
- [ ] CLAUDE.md style table updated
- [ ] `plans/README.md` row updated

## STOP conditions

- No decision recorded (Step 1) — this is the expected initial state; report
  "waiting on look decision" and mark the plan BLOCKED in the index.
- Grickle default causes any scene's QA to FAIL that plan 001 left passing,
  and one round of fill-lift doesn't fix it.
- You find yourself wanting to edit the postfx shader — out of scope; report.

## Maintenance notes

- The ink-outline shader pass (ROADMAP Phase 4) composes with this; whoever
  builds it should re-check dark scenes.
- If the owner reverses the decision later, reverting is the one assignment
  in main.c plus this doc trail.
