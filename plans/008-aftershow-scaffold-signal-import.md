# Plan 008: AFTERSHOW in-engine scaffold + SIGNAL code import

> **Executor instructions**: Follow step by step; verify each step. On any
> STOP condition, stop and report. Update `plans/README.md` when done.
>
> **Drift check (run first)**:
> `git diff --stat 04f970a..HEAD -- ev_engine/src/ev_types.h ev_engine/src/scene_registry.c ev_engine/src/dialog_game.c ev_engine/src/main.c`
> On mismatch with "Current state", STOP.

## Status

- **Priority**: P1 (first Phase 3 item)
- **Effort**: M
- **Risk**: MED (touches the GameState enum — several parallel tables must
  stay in sync; the checklist below enumerates all of them)
- **Depends on**: none technically; ROADMAP gates Phase 3 on EV shipping —
  confirm with the operator before executing ahead of that gate
- **Category**: direction (AFTERSHOW vertical slice, part 1)
- **Planned at**: commit `04f970a`, 2026-07-05

## Why this matters

AFTERSHOW (see `/Users/maxwellyoung/Development/aftershow/README.md`) is the
follow-up game: a Twine prologue (done, playable) hands the player a code
like `SIGNAL-SISIS-3-PLAYED` encoding their choices; the 3D half must ingest
that code so the Green Room permutes on prologue facts. This plan creates
the in-engine scaffold: one new game state (`STATE_AS_GREENROOM`), dev-only
entry, and the code-entry screen that seeds the dialogue memory. Plan 009
builds the room itself; this plan makes it reachable and fact-seeded.

## Current state

Paths relative to `/Users/maxwellyoung/Development/dante/ev_engine`.

- **The code format** (source of truth:
  `/Users/maxwellyoung/Development/aftershow/prologue/aftershow.twee`,
  function `setup.exportCode`):
  `SIGNAL-XXXXX-N-SAVED|PLAYED` where the five X are `S`(suspicious) /
  `I`(innocent) / `-`(unfiled) for exhibits, in order:
  `ep388, absence, tweet, runtime, signoff`; N = suspicion 0..5; suffix
  `SAVED` or `PLAYED` (how the finale was heard).
- **Dialogue memory API** (`src/dialog.h`):
  `void dlg_mem_set(const char *key, float value, float ttl);` — ttl 0 =
  permanent. String-valued facts are stored as interned symbols: use
  `dlg_mem_set("filed_ep388", (float)dlg_intern("sus"), 0)` and rules can
  match `filed_ep388=sus` (the rules parser interns identifiers the same
  way — this exact mechanism is already used by `replied_*` facts; see
  `update_choice_input` in `src/main.c`).
- **Adding a GameState — ALL the parallel tables** (this is the risk; check
  each):
  1. `src/ev_types.h` `GameState` enum (lines ~208–235, 26 entries, ends
     `STATE_SHELL_TEST`). Append AFTER the last entry (QA/dev tables index
     by order).
  2. `src/scene_registry.c` — forward decls + `scene_descs[]` entry
     (`[STATE_AS_GREENROOM] = { .load = as_greenroom_load, .update = as_greenroom_update, .indoor = true }`).
  3. `src/dialog_game.c` `state_names[]` (26 strings — MUST append
     `"as_greenroom"` or every scene fact after the insertion point is
     wrong; this is why we APPEND, never insert).
  4. `src/main.c` — a local `state_names[]` near the debug overlay (search
     `const char *state_names[] = {` and the `< 26` bounds check nearby —
     update both), and the QA `qa_scenes[]` table ONLY if you want QA
     coverage (add it: copy a simple entry like `stars`).
  5. `tests/test_main.c` — asserts `GameState count` (the QA output earlier
     printed `GameState count = 26  OK`; find the assertion and bump it).
- **Scene file convention**: per-scene .c files with `<name>_load()` /
  `<name>_update(float dt)` (repo rule: this is BY DESIGN, one file per
  scene — see `ev_engine/CLAUDE.md` "Adding a New Scene").
- **Dev entry**: `make dev SCENE=STATE_AS_GREENROOM` works automatically
  (Makefile passes `-DDEV_START=$(SCENE)`).
- **Text entry UI**: the title screen has a text-entry mechanism? NO —
  simplest robust input for the code screen: raylib `GetCharPressed()` loop.
  There is spring-physics text display (`show_text`) but no line-input
  widget. Keep the input dead simple (see Step 3).
- **The rules file**: `assets/dialogue/ev.rules` — header documents the
  format. New AFTERSHOW rules go in a NEW file
  `assets/dialogue/aftershow.rules`; `dlg_game_init` currently loads only
  `ev.rules` (see `DLG_RULES_PATH` in `src/dialog_game.c`) — `dlg_load_file`
  is ADDITIVE, so add a second call guarded by `FileExists`.

## Commands you will need

| Purpose | Command | Expected |
|---|---|---|
| Build | `cd ev_engine && EV_MUTE=1 make -j8` | exit 0, no warnings |
| Tests | `make test` | all pass (count assertion updated) |
| Boot the scaffold | `EV_MUTE=1 make -j8 dev SCENE=STATE_AS_GREENROOM` | code-entry screen renders |
| QA regression | `EV_MUTE=1 make -j8 qa` | existing scenes unchanged |

## Scope

**In scope**:
- `ev_engine/src/ev_types.h`, `src/scene_registry.c`, `src/dialog_game.c`,
  `src/main.c` (table sync points listed above ONLY), `tests/test_main.c`
  (count assertion)
- `ev_engine/src/scene_as_greenroom.c` (create)
- `ev_engine/src/dialog_game.c` (load aftershow.rules additively)
- `ev_engine/assets/dialogue/aftershow.rules` (create, minimal)
**Out of scope**: the EV spine (no transition from any EV scene into
AFTERSHOW states), the Twine prologue, shell/Blender content (plan 009),
menu/title changes.

## Steps

### Step 1: Register the state (all five sync points)

Append `STATE_AS_GREENROOM` to the enum; update registry, both
`state_names[]` tables, bounds checks, and the test count. Create
`src/scene_as_greenroom.c` with a minimal load (grey box room via
`add_wall` floor + 4 walls + a point light, spawn set) and update (just
`update_player`). Model the file on `src/scene_glasshouse.c`'s structure.

**Verify**: `make test` passes; `EV_MUTE=1 make -j8 dev
SCENE=STATE_AS_GREENROOM` boots into the grey room; `make qa` table
unchanged for existing scenes.

### Step 2: SIGNAL parser (pure, testable)

Create the parser in `src/dialog.c`-adjacent pure style — put
`bool as_parse_signal(const char *code, char filed[5], int *suspicion, bool *saved)`
in `src/scene_as_greenroom.c` as a static (or a small
`src/aftershow.c` if you prefer one home for AFTERSHOW glue — then add it to
the file map comment in CLAUDE.md). Grammar (case-insensitive, tolerate
whitespace): `SIGNAL-[SI-]{5}-[0-5]-(SAVED|PLAYED)`. On success, seed:

```
for i, name in {ep388, absence, tweet, runtime, signoff}:
    if filed[i] != '-': dlg_mem_set("filed_<name>", (float)dlg_intern(filed[i]=='S' ? "sus" : "ok"), 0);
dlg_mem_set("suspicion", (float)suspicion, 0);
dlg_mem_set("prologue_saved", saved ? 1 : 0, 0);
dlg_mem_set("prologue_imported", 1, 0);
```

**Verify**: add a headless test — extend `tests/test_dialog.c` ONLY if the
parser lives in a raylib-free file it can include; otherwise verify via the
in-game path in Step 3 plus a `printf` self-test guarded by `#ifdef QA_MODE`
run during QA (`[AS] signal parse tests: 6/6`). Cases: valid full code;
lowercase; all-dashes exhibits; suspicion 0 and 5; malformed (missing
segment) → false.

### Step 3: Code-entry screen

In `as_greenroom_load`, set a scene-local `entering_code = true` (file-scope
static). While true, `as_greenroom_update` collects `GetCharPressed()` into
a buffer (accept alnum + '-', uppercase it, backspace support, max 24
chars), renders it centered via the existing text draw conventions
(lower-third style — copy fonts/colors from `draw_dialogue`'s constants in
main.c; the scene can draw in the HUD phase — check how other scenes draw
2D: they don't; simplest is `show_text(buffer)` each frame which reuses the
vignette text path, acceptable for a dev scaffold). ENTER: parse; on success
`entering_code = false` and fire `dlg_game_speak("narrator", "enter")`; on
failure flash the text red for a second (tint via show_text is fixed —
acceptable: just append " ?" to the buffer display).
Also support env skip: `EV_SIGNAL=SIGNAL-...` pre-seeds and skips the screen
(demo/QA ergonomics).

**Verify**: boot the scene, type `SIGNAL-SISIS-3-PLAYED`, ENTER → a
narrator line fires (Step 4's rule) and the log shows
`DIALOG: loaded ... aftershow.rules` at init.

### Step 4: aftershow.rules seed + additive load

In `dlg_game_init` (src/dialog_game.c), after the ev.rules load:
`if (FileExists("assets/dialogue/aftershow.rules")) dlg_load_file(...)` with
the same TraceLog pattern. Create `assets/dialogue/aftershow.rules` with a
header comment (copy ev.rules' header style) and two rules:

```
rule as_enter_imported
who narrator
criteria concept=enter scene=as_greenroom prologue_imported=1
say "The room your mind built. It was waiting up."
norepeat
end

rule as_enter_suspicious
who narrator
criteria concept=enter scene=as_greenroom prologue_imported=1 suspicion>=3
say "The room your mind built. Colder than you left it."
norepeat
end
```

(The second out-scores the first when suspicion ≥3 — the permutation proof.)

**Verify**: entering with `-3-` code prints the "Colder" line; with `-0-`
the plain line. `make test` + full QA still green.

## Test plan

- Parser cases (Step 2 list) automated one way or the other.
- Manual: both suspicion branches verified via EV_SIGNAL env boots.
- Regression: `make qa` table byte-similar for pre-existing scenes;
  `make test` passes.

## Done criteria

- [ ] `STATE_AS_GREENROOM` reachable via `make dev`, absent from EV spine
- [ ] All five enum-sync points updated (grep `SHELL_TEST` neighbors to confirm)
- [ ] SIGNAL codes parse and seed dlg memory; suspicion branch demonstrable
- [ ] aftershow.rules loads additively; ev.rules untouched
- [ ] `make test`, `make check`, `make qa` all green
- [ ] `plans/README.md` row updated

## STOP conditions

- Any sync table listed in "Current state" doesn't exist where described.
- The enum count assertion in tests fails after a careful bump — search
  `GameState` in tests/test_main.c and fix ONLY count assertions; if other
  code hard-asserts 26, report it.
- Operator has not confirmed executing ahead of the "EV ships first" gate.

## Maintenance notes

- Plan 009 replaces the grey box with the shell-built Green Room; keep
  load/update structure clean for that handoff.
- The Twine prologue's exhibit names are the contract
  (`aftershow/systems/RULES_FORMAT.md`); if the prologue changes its export,
  this parser and fact names must move in lockstep.
