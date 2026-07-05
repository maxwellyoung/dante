# Plan 006: Couch-playtest kit — package, protocol, and a local run log

> **Executor instructions**: Follow step by step; verify each step. On any
> STOP condition, stop and report. Update `plans/README.md` when done.
>
> **Drift check (run first)**:
> `git diff --stat 04f970a..HEAD -- ev_engine/Makefile ev_engine/src/main.c`
> On mismatch with "Current state", STOP.

## Status

- **Priority**: P1
- **Effort**: S
- **Risk**: LOW
- **Depends on**: none (001/002 improve what testers see, but don't block)
- **Category**: dx (Remo's couch method, operationalized)
- **Planned at**: commit `04f970a`, 2026-07-05

## Why this matters

Firewatch's reactivity was built by watching testers on a couch and
supporting what they invented (the boombox-in-the-lake came from a tester).
ROADMAP Phase 1.7 calls for three observed playtests. The build exists
(`make playtest`); what's missing is the kit around it: an observation
protocol, a feedback template, and a **local-only run log** (scene timings +
interactions) so a session leaves analyzable residue. No telemetry, no
network — a text file next to the binary.

## Current state

Paths relative to `/Users/maxwellyoung/Development/dante/ev_engine`.

- `Makefile` target `playtest:` (search it): clean build with
  `-DNDEBUG -DPLAYTEST`, copies binary + assets into `playtest/`, writes a
  README.txt asking testers 3 questions. `-DPLAYTEST` compiles OUT debug
  keys (e.g. the F6 reload guard in `src/dialog_game.c` is
  `#ifndef PLAYTEST`).
- `src/main.c` — the state machine: `load_state(GameState s)` (search it) is
  the single choke point every scene change passes through. `g.state_time`
  accumulates per scene; `g.total_time` global. Interactions all pass
  `PlayInteract(...)` or set `g.interact_freeze = 0.05f` (suite dispatch).
- There is currently NO run logging of any kind.
- Repo rule: no network calls, no analytics. The log must be a plain local
  file the tester can read themselves.

## Commands you will need

| Purpose | Command | Expected |
|---|---|---|
| Build kit | `cd ev_engine && EV_MUTE=1 make playtest` | `playtest/` folder: binary, assets, README.txt |
| Run it | `cd playtest && ./endearing_void` (audio ON for real tests) | game runs from TITLE |
| Tests | `make test` | 31 tests, 0 failed |

## Scope

**In scope**:
- `ev_engine/src/main.c` — run-log writes in `load_state` (+ a session
  open/close), guarded to write ONLY under `-DPLAYTEST` or an env var
- `ev_engine/Makefile` — extend the `playtest:` target's README generation
- `ev_engine/playtest/PROTOCOL.md` — created by the target (observer's sheet)
**Out of scope**: any network/telemetry, gameplay changes, debug keys.

## Steps

### Step 1: Run log

In `load_state`, when `PLAYTEST` is defined OR `getenv("EV_RUNLOG")`, append
one line to `run_log.txt` in the working directory:
`<total_time seconds> ENTER <state name> (prev <state name> after <state_time>s)`.
Use the existing state-name table pattern (there is a `state_names[]` array
in `src/dialog_game.c` and another local one in main.c — reuse main.c's or
declare the mapping locally; do NOT include dialog.h more broadly for this).
Open with a session header line (date via time(NULL), build note) on first
write; flush per line (fopen append / fclose each write is fine at this
frequency).

**Verify**: `EV_RUNLOG=1 EV_MUTE=1 make -j8 dev SCENE=STATE_SPACE_LOBBY`,
quit after ~20s → `run_log.txt` contains the session header + ENTER lines.

### Step 2: Extend the playtest package

In the Makefile `playtest:` recipe, additionally write
`playtest/PROTOCOL.md` (heredoc-style like the existing README lines) with
the observer protocol:

1. Say nothing once the game starts. No hints, no "did you see…".
2. Note verbatim: where they got lost, what they tried that the game
   ignored, anything they said aloud, where they smiled.
3. After: 3 questions — What was that about? What will you remember
   tomorrow? Where were you bored?
4. Collect `run_log.txt` from the playtest folder.
5. After each session file one issue per pain point; support ONE invented
   behavior within the week (the boombox rule).

Also append to README.txt: "The game writes run_log.txt (scene timings,
local only, no network). Please send it back with your answers."

**Verify**: `make playtest` → both files present with the content above;
running the playtest binary produces `run_log.txt` (PLAYTEST define path).

## Test plan

- One simulated session: run the playtest binary 60s, traverse 2+ scenes via
  normal play (or the title menu), confirm log contents match movements.
- `make test` unchanged; normal (non-playtest) build writes NO log file
  (`rm -f run_log.txt; EV_MUTE=1 make -j8 dev SCENE=STATE_LOBBY`, quit;
  assert no `run_log.txt`).

## Done criteria

- [ ] `make playtest` emits binary + assets + README.txt + PROTOCOL.md
- [ ] Playtest binary writes `run_log.txt`; dev/normal builds don't (unless EV_RUNLOG=1)
- [ ] `make test` passes; `make check` clean
- [ ] `plans/README.md` row updated

## STOP conditions

- `load_state` has grown a different structure than described (drift).
- Any temptation to add more instrumentation (heatmaps, input capture) —
  out of scope; one file, scene lines only.

## Maintenance notes

- After the three sessions, the owner triages notes into issues; the
  "support one invented behavior" rule is the point of the whole exercise.
- If EV ships with the log enabled, keep the README disclosure line.
