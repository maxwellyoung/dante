# Plan 004: Demo-reel harness — stills proving the dialogue/reactivity systems

> **Executor instructions**: Follow step by step; verify each step. On any
> STOP condition, stop and report. Update `plans/README.md` when done.
>
> **Drift check (run first)**:
> `git diff --stat 04f970a..HEAD -- ev_engine/src/main.c ev_engine/src/dialog.h ev_engine/src/dialog_game.c ev_engine/assets/dialogue/ev.rules`
> On mismatch with "Current state", STOP.

## Status

- **Priority**: P1
- **Effort**: M
- **Risk**: LOW (additive QA-mode code path; no gameplay changes)
- **Depends on**: none
- **Category**: dx / direction (visibility of invisible systems)
- **Planned at**: commit `04f970a`, 2026-07-05

## Why this matters

The repo has a Valve-lineage contextual dialogue system (facts → rules →
most-specific-wins), permutation endings, carry reactions, and walk-and-talk
NPC dialogue — and **no visible evidence any of it works**. The owner said
so explicitly. This plan adds `EV_QA_DEMO=1`: a scripted pass that stages
each reactive feature and saves a captioned screenshot (subtitle rendered
into the frame) to `qa/demo/`. Output doubles as collaborator-pitch
material.

## Current state

All paths relative to `/Users/maxwellyoung/Development/dante/ev_engine`.

- QA mode: build flavor `make qa` compiles with `-DQA_MODE`; `main()` in
  `src/main.c` has a `#ifdef QA_MODE` section (search `"EV QA"`). It already
  reads env overrides there: `EV_QA_STYLE` (search for it, ~line 645).
- Screenshot machinery: macro `QA_RENDER_SHOT_EX(cam_pos, cam_target, out_path, scene_idx)`
  in `src/main.c` (search the name; ~line 1300): positions the camera,
  renders 8 frames (scene → postfx), exports PNG from the postfx target.
  **It does NOT render 2D overlays** — the demo must add the dialogue
  overlay into the captured target (Step 2).
- Dialogue display state lives in `GameCtx` (`src/game_ctx.h`, search
  "── Dialogue system ──"): `dlg_speaker`, `dlg_text`, `dlg_timer`,
  `dlg_chars_per_sec`, `dlg_fade`, `dlg_active`. Rendering:
  `static void draw_dialogue(void)` in `src/main.c` (search it) draws the
  lower-third from those fields at full reveal when
  `dlg_timer * dlg_chars_per_sec >= strlen(dlg_text)`; `dlg_fade` must be
  ~1.0 for visibility (set it directly in the demo).
- Dialogue engine API (`src/dialog.h`):
  - `bool dlg_game_speak(const char *who, const char *concept);`
  - `bool dlg_game_speak_about(const char *who, const char *concept, const char *object, float step);`
  - `void dlg_mem_set(const char *key, float value, float ttl);`
  - `dlg_game_init()` loads `assets/dialogue/ev.rules` (45 rules at planning
    time; count printed in the log line `DIALOG: loaded N rules`).
  - Speaking calls `show_dialogue()` which populates the GameCtx fields
    above — so after a successful `dlg_game_speak`, setting
    `g.dlg_timer = 999; g.dlg_fade = 1.0f;` makes `draw_dialogue()` render
    the full line.
- Facts that drive the staged beats (all real, verify in
  `assets/dialogue/ev.rules`):
  - `concept=waypoint scene=space_lobby waypoint=0` → "The room's been ready
    for some time." (rule `sl_tour_ready`)
  - `concept=interact object=wineglass step=1` → wineglass line
    (rule `int_wineglass`; suite scene)
  - `concept=idle scene=space_suite carrying=her_book` → geometry-book line
    (rule `carry_book_suite`; ALSO gated `random<60` — see Step 3 note)
  - `concept=enter scene=bed tasks_done<1` → "You lie down in a room that
    never learned your name." (rule `end_bed_untouched`)
  - `concept=enter scene=bed tasks_done>=4 photograph_flipped=1` →
    the photo+ritual permutation (rule `end_bed_photo_ritual`)
  - `concept=idle scene=space_lobby choices_ignored>=2` → "Quiet ride up,
    I'm told." (rule `idle_quiet_ride`)
- `carrying=<tag>` fact comes from `build_query` reading
  `g.grab.state == GRAB_CARRYING` + `g.scene.walls[g.grab.wall_index].tag`
  (`src/dialog_game.c`, search "Carry facts"). Tagged walls exist in the
  suite build (`set_last_tag(s, "her_book")` etc. in `src/scene.c`).
  `photograph_flipped` reads `g.photograph_flipped`. `tasks_done` reads
  `g.tasks_done`. Never-twice: rules without `resay` go quiet after all
  variants are heard — a fresh process has fresh state, so one demo run per
  process is safe; do NOT fire the same rule twice expecting a line.
- Env pattern to copy: the `EV_QA_STYLE` block in main.c QA section.

## Commands you will need

| Purpose | Command | Expected |
|---|---|---|
| Build+run demo | `cd ev_engine && mkdir -p qa/demo && EV_MUTE=1 EV_QA_DEMO=1 make -j8 qa` | demo PNGs written to `qa/demo/` |
| Normal QA unaffected | `EV_MUTE=1 make -j8 qa` | usual `[QA]` table, no demo files |
| Tests | `make test` | 31 tests, 0 failed |

## Scope

**In scope**:
- `ev_engine/src/main.c` (QA_MODE section only: a `run_demo_reel()` function
  + env gate call)
- `ev_engine/qa/demo/DEMO.md` (create — one line per still explaining what
  it proves)
**Out of scope**: dialog.c / dialog_game.c / ev.rules (read-only for this
plan), the normal QA table logic, any gameplay file.

## Steps

### Step 1: Add the gate and skeleton

In main.c's QA_MODE section, after the existing QA table run (find where the
per-scene loop completes, before cleanup), add:

```c
if (getenv("EV_QA_DEMO")) run_demo_reel();
```

and a `static void run_demo_reel(void)` above `main()` (or forward-declared)
in the same QA_MODE `#ifdef`.

**Verify**: builds clean; `EV_MUTE=1 make -j8 qa` (without the env) behaves
exactly as before.

### Step 2: One reusable capture helper

Inside `run_demo_reel`, write a local helper that: (a) `load_state(<scene>)`,
zeroes `g.fade_alpha`/`g.fade_target`, (b) positions
`g.player.camera.position/.target` for the shot, (c) stages facts (see per-
beat list), (d) calls the `dlg_game_*` function and asserts it returned true
(if false, `printf("[DEMO] MISS: <beat>\n")` and continue), (e) sets
`g.dlg_timer = 999.0f; g.dlg_fade = 1.0f;`, (f) renders like
`QA_RENDER_SHOT_EX` does BUT with the dialogue overlay: after `draw_postfx`
into `g.postfx_target`, wrap `BeginTextureMode(g.postfx_target);
draw_dialogue(); EndTextureMode();` then export the texture to
`qa/demo/NN_<slug>.png` (copy the LoadImageFromTexture/ImageFlipVertical/
ExportImage lines from the macro).

**Verify**: temporary single beat (space_lobby waypoint) produces
`qa/demo/01_waypoint_tour.png` with the subtitle visible in the frame.

### Step 3: Stage the eight beats

| # | slug | scene | staging | call |
|---|---|---|---|---|
| 01 | waypoint_tour | STATE_SPACE_LOBBY | camera facing Gibbons spawn (2,1.6,4) | `dlg_game_speak("gibbons","waypoint")` after setting `g.gibbons.current_waypoint=0; g.gibbons.waiting=true;` |
| 02 | interact_wineglass | STATE_SPACE_SUITE | camera at suite center | `dlg_game_speak_about("gibbons","interact","wineglass",1)` |
| 03 | carry_book | STATE_SPACE_SUITE | find wall with `tag=="her_book"` (loop `g.scene.walls`), set `g.grab.state=GRAB_CARRYING; g.grab.wall_index=<i>;` | `dlg_game_speak("gibbons","idle")` — retry up to 8 times until it returns true (the rule has `random<60`); reset `g.grab.state=GRAB_NONE` after |
| 04 | ending_untouched | STATE_BED | `g.tasks_done=0; g.photograph_flipped=false;` | `dlg_game_speak("narrator","enter")` |
| 05 | ending_full_ritual | STATE_BED | `g.tasks_done=4; g.photograph_flipped=true;` | same call — DIFFERENT line proves permutation |
| 06 | quiet_ride | STATE_SPACE_LOBBY | `dlg_mem_set("choices_ignored",2,0);` | `dlg_game_speak("gibbons","idle")` (retry as 03) |
| 07 | thermostat_gag | STATE_SPACE_SUITE | — | `dlg_game_speak_about("gibbons","interact","thermostat",3)` |
| 08 | corridor_walkandtalk | STATE_SPACE_CORRIDOR | Gibbons at wp0 as in 01 | `dlg_game_speak("gibbons","waypoint")` |

Beat 04 and 05 both hit `concept=enter` rules marked `norepeat` — they are
DIFFERENT rules so both fire once each in one process; order 04 before 05.
(`end_bed_base` may win a tie only if the specific rules reject — if beat 04
prints a different line than expected, capture it anyway and note it in
DEMO.md; every permutation is valid by design.)

**Verify**: `EV_MUTE=1 EV_QA_DEMO=1 make -j8 qa` writes ≥7 of 8 PNGs
(count: `ls qa/demo/*.png | wc -l`), each with a visible lower-third line;
`[DEMO] MISS` lines, if any, are printed and explained.

### Step 4: Write DEMO.md + wire convenience target

`qa/demo/DEMO.md`: title, one line per still (what system it proves, which
rule fired). Add Makefile target `demo:` that runs
`EV_MUTE=1 EV_QA_DEMO=1 $(MAKE) qa` (copy the style of the `factsheet:`
target).

**Verify**: `make demo` reproduces the folder from scratch.

## Test plan

- The harness itself is the test; its assertion mode is the `[DEMO] MISS`
  line. All 8 beats firing = the dialogue system's integration test.
- `make test` and plain `make qa` unchanged.

## Done criteria

- [ ] `make demo` produces 8 captioned PNGs + DEMO.md in `qa/demo/`
- [ ] Beats 04 vs 05 show different narrator lines (permutation proof)
- [ ] Plain `make qa` output unchanged (diff the `[QA]` table before/after)
- [ ] `make test` passes; build has zero warnings
- [ ] `plans/README.md` row updated

## STOP conditions

- `QA_RENDER_SHOT_EX` or `draw_dialogue` doesn't exist by those names (drift).
- `dlg_game_speak("narrator","enter")` returns false for BOTH bed beats —
  the rules or facts moved; report rather than forcing lines.
- Any change required outside main.c's QA section.

## Maintenance notes

- New reactive features should add a beat here — the demo reel is the living
  proof sheet. Reviewer: check the stills, not just the exit code.
- These PNGs are collaborator-pitch material; keep slugs human-readable.
