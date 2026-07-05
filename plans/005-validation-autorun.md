# Plan 005: Automated validation runs — rush-run and linger-run must both hold

> **Executor instructions**: Follow step by step; verify each step. On any
> STOP condition, stop and report. Update `plans/README.md` when done.
>
> **Drift check (run first)**:
> `git diff --stat 04f970a..HEAD -- ev_engine/src/main.c ev_engine/src/scene_registry.c`
> On mismatch with "Current state", STOP.

## Status

- **Priority**: P2
- **Effort**: M
- **Risk**: LOW (QA-mode only)
- **Depends on**: none (001 recommended first so failures are real)
- **Category**: tests
- **Planned at**: commit `04f970a`, 2026-07-05

## Why this matters

Remo's Firewatch validation practice: play the whole game rushing everything
and never engaging, and it must still hold (no softlocks, no nonsense).
Nobody can hand-play EV on every commit. The QA harness already validates
individual scene transitions; this plan adds a **full-spine simulated run**:
programmatically advance through the entire CUT.md spine with time
acceleration, assert every transition fires within a budget, and report a
total-duration estimate (the 12–15 minute target is currently a guess).

## Current state

Paths relative to `/Users/maxwellyoung/Development/dante/ev_engine`.

- The spine (CUT.md): TITLE → CAR → DRIVING → HOTEL_EXT → LOBBY → ELEVATOR →
  HYPERSPACE → SPACE_LOBBY → (ELEVATOR) → GLASSHOUSE → SPACE_CORRIDOR →
  SPACE_SUITE → BALCONY → PARIS_DREAM → CLEANED_SUITE → BED → STARS →
  MONTAGE → RETURN_TAXI → TITLE.
- QA flow test exists in `src/main.c` QA/e2e section: a table of
  `{from, to, "name", "name"}` transitions (search `STATE_HYPERSPACE,     STATE_SPACE_LOBBY`)
  driven per-pair (load `from`, run frames, assert `to` reached). It does
  NOT chain a full run and does NOT simulate player movement toward exits.
- Scene updates run via `scene_descs[g.state].update(dt)`
  (`src/scene_registry.c`). Many transitions require the player to reach an
  exit position (`g.scene.exit_pos`, `has_exit`) or an NPC sequence to
  complete; some are timers (`g.state_time > N`).
- Player position can be driven directly: `g.player.camera.position` — the
  existing QA code does exactly this (search `g.player.camera.position =` in
  the QA section).
- Timers accelerate naturally by calling the update function with large-ish
  `dt` steps (0.1f) many times — but several scenes cap behavior per-frame;
  prefer many small steps (dt=1/30) in a loop with a frame budget.
- Gibbons-gated scenes (space_lobby, glasshouse, corridor): advance requires
  the player near him repeatedly — simulate by teleporting the player to
  `g.gibbons.pos` every few frames until `!g.gibbons.active`, then to
  `g.scene.exit_pos`.

## Commands you will need

| Purpose | Command | Expected |
|---|---|---|
| Run validation | `cd ev_engine && EV_MUTE=1 EV_QA_RUN=rush make -j8 qa` | `[RUN] COMPLETE` line with per-scene timings |
| Linger variant | `EV_MUTE=1 EV_QA_RUN=linger make -j8 qa` | same, longer durations |
| Tests | `make test` | 31 tests, 0 failed |

## Scope

**In scope**: `ev_engine/src/main.c` (QA_MODE section: `run_spine_validation()`
+ env gate), `ev_engine/qa/` (a `run_report.txt` output file is fine).
**Out of scope**: any scene file, the registry, gameplay logic. If a scene
cannot be completed without changing gameplay code, that is a FINDING to
report, not a thing to fix here.

## Steps

### Step 1: Harness skeleton

In the QA_MODE section of main.c add `run_spine_validation(const char *mode)`
gated on `getenv("EV_QA_RUN")`. Start at `load_state(STATE_CAR)` (skip
TITLE — it needs menu input). Loop: call `scene_descs[g.state].update(dt)`
with dt = 1/30 plus the same per-frame supports the real loop does in QA
(check what the existing flow test calls per frame and mirror it — at
minimum `update_dialogue`/`dlg_game_update` are NOT required for transitions;
omit anything that needs audio device state if it crashes, and note it).

Per frame, drive the player: teleport toward `g.gibbons.pos` while
`g.gibbons.active`, else toward `g.scene.exit_pos` when `has_exit`, in 0.5m
steps (not instant — proximity triggers use distance thresholds).
Record `g.state` changes with frame counts. `rush` mode: exactly the above.
`linger` mode: before seeking exits, hold position 30 simulated seconds per
scene.

**Budget**: if a state persists > 4 simulated minutes, print
`[RUN] STUCK in <state_name> after 240s` and abort with nonzero.

**Verify**: `EV_QA_RUN=rush` reaches at least SPACE_SUITE without STUCK.

### Step 2: Handle the suite

The suite exits via task/ritual progress. QA already has
`suite_apply_ritual_progress_for_qa(int tasks, bool window_revealed, bool bath_running)`
(declared near the top of main.c). In the harness, when entering
STATE_SPACE_SUITE in rush mode call it with (4, true, false) after 10
simulated seconds, then seek the exit; in linger mode after 60s.

**Verify**: run proceeds past the suite to BALCONY and onward.

### Step 3: Full chain + report

Assert the visited state sequence matches the spine ordering above (states
may repeat ELEVATOR). On completion print `[RUN] COMPLETE mode=<m>
sim_minutes=<total>` plus a per-scene table, and write it to
`qa/run_report.txt`. Nonzero exit if STUCK or if sequence deviates.

**Verify**: both modes COMPLETE; rush sim_minutes is plausibly 8–20 (record
whatever it is — this is the first real measurement of game length).

## Test plan

The harness is the test. Also confirm `make qa` without `EV_QA_RUN` is
byte-identical in behavior (env-gated).

## Done criteria

- [ ] `EV_QA_RUN=rush` and `=linger` both print `[RUN] COMPLETE` with the
      full spine sequence and write `qa/run_report.txt`
- [ ] STUCK detection demonstrably works (temporarily break a transition
      locally to see it fire, then revert — do not commit the break)
- [ ] Plain `make qa` unchanged; `make test` passes
- [ ] `plans/README.md` row updated

## STOP conditions

- A spine transition genuinely cannot fire under simulation (e.g. requires
  raw key input with no state-level alternative): report the scene and the
  input required — that's a real finding about the game, not harness failure.
- The harness needs > ~250 lines in main.c — step back and report; it may
  belong in its own file, which changes scope.

## Maintenance notes

- CUT.md's "Definition of done" gains teeth: run both modes before any
  release tag. Wire into `make playtest` later if desired (deferred).
- The duration number feeds the itch page copy (plan 007).
