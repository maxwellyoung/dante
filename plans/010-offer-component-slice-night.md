# Plan 010: Generic reply-window component + the ten-minute slice night

> **Executor instructions**: Follow step by step; verify each step. On any
> STOP condition, stop and report. Update `plans/README.md` when done.
>
> **Drift check (run first)**:
> `git diff --stat 04f970a..HEAD -- ev_engine/src/main.c ev_engine/src/dialog.h ev_engine/src/dialog_game.c ev_engine/src/game_ctx.h`
> On mismatch with "Current state", STOP.

## Status

- **Priority**: P2
- **Effort**: L
- **Risk**: MED (new UI component + two new scenes)
- **Depends on**: plans/008, plans/009
- **Category**: direction (AFTERSHOW vertical slice, part 3 — the loop)
- **Planned at**: commit `04f970a`, 2026-07-05

## Why this matters

AFTERSHOW's core verb is *replying — or not*: timed prompts where silence is
a recorded answer (the Firewatch radio, transplanted to feeds and DMs). EV
prototyped half of it (`show_choice_timed` on the taxi backstory choices);
this plan promotes it to a reusable concept-firing component and builds the
slice: **one night, ten minutes** — kitchen → feed terminal → Green Room →
one exhibit → out — permuting on prologue facts. ROADMAP: "If the slice
doesn't produce the feeling, iterate here before building more."

## Current state

Paths relative to `/Users/maxwellyoung/Development/dante/ev_engine`.

- **Timed choice (exemplar to generalize)** — `src/main.c`:
  - `void show_choice_timed(const char *question, const char *a, const char *b, float window)`
    (search it) + fields in `GameCtx` (`src/game_ctx.h`: `choice_timer`,
    `choice_window`, `choice_active`, `choice_cursor`, `choice_result`,
    `choice_confirmed`, `choice_question/a/b`).
  - `update_choice_input()` handles arrows + confirm + the timeout branch
    (timeout writes `backstory[-1]` and increments the `choices_ignored`
    dlg fact via `dlg_mem_set` — this backstory write is EV-specific and
    must NOT happen for AFTERSHOW offers; see Step 1).
  - `draw_choice()` renders lower-third + the burning timer line.
- **Dialogue API** (`src/dialog.h`): `dlg_game_speak(who, concept)`,
  `dlg_mem_set(key, value, ttl)`, `dlg_intern(name)`. Facts pattern used by
  the taxi: on timeout `choices_ignored += 1`. The AFTERSHOW convention
  (from `aftershow/systems/RULES_FORMAT.md`): resolving an offer sets
  `replied_<concept> = a|b` (interned symbol) or `ignored_<concept> = 1`,
  then fires the concept so rules respond.
- **Scenes** (after 008/009): `STATE_AS_GREENROOM` exists. This plan adds
  `STATE_AS_KITCHEN` — enum-sync checklist is in plan 008 "Current state"
  (five parallel tables; follow it exactly, appending after
  STATE_AS_GREENROOM).
- **The night flow** (design, fixed): AS_KITCHEN (2 a.m., kettle, a feed
  terminal on the counter) → interact with terminal → 2–3 feed posts appear
  as dialogue lines, one offers a timed reply (post about ep 388) →
  hard_cut_to Green Room (`hard_cut_to` is the Blendo cut, declared in
  scene files as `void hard_cut_to(GameState s);`) → one exhibit beat
  (props from 009 + one narrator line keyed to the feed choice) → exit
  trigger → back to kitchen, kettle boiled, night over (fade to title or
  hold on a final line). Total target ≤ 10 minutes.
- Severity gradient rule (`references/TONE_SEVERITY_GRADIENT.md`): kitchen/
  feed can be wry; Green Room beats play straight.

## Commands you will need

| Purpose | Command | Expected |
|---|---|---|
| Build/boot | `EV_MUTE=1 make -j8 dev SCENE=STATE_AS_KITCHEN` | kitchen loads |
| Tests | `make test` | all pass (state count bumped again) |
| QA | `EV_MUTE=1 make -j8 qa` | pre-existing rows unchanged |

## Scope

**In scope**:
- `src/main.c` + `src/game_ctx.h` + `src/dialog.h` + `src/dialog_game.c`
  (the offer component)
- `src/scene_as_kitchen.c` (create) + enum-sync tables + `tests/test_main.c`
  count
- `src/scene_as_greenroom.c` (exit trigger + the feed-choice-keyed line)
- `assets/dialogue/aftershow.rules` (feed posts, replies, night lines)
**Out of scope**: EV spine and ev.rules; the taxi's existing
`show_choice_timed` call sites (must keep working unchanged); prologue.

## Steps

### Step 1: `dlg_game_offer` — the generic component

Add to dialog.h/dialog_game.c:
`bool dlg_game_offer(const char *who, const char *concept, const char *a, const char *b, float window);`
Implementation strategy: reuse the existing GameCtx choice fields/UI (so
`draw_choice` and input handling render it for free) plus a new
`g.choice_offer_concept` (interned id, -1 when the choice is a legacy
backstory one). In `update_choice_input`:
- if `choice_offer_concept >= 0`: on confirm →
  `dlg_mem_set("replied_<concept>", (float)dlg_intern(cursor==0?"a":"b"), 0)`;
  on timeout → `dlg_mem_set("ignored_<concept>", 1, 0)`; both paths then
  `dlg_game_speak(offer_who, concept)` and DO NOT touch `backstory[]` or
  `choices_ignored`. Store `offer_who` alongside (small static or GameCtx
  field). Build the key names with snprintf into a stack buffer (concepts
  are short; bound-check).
- else: existing behavior byte-for-byte.

**Verify**: `make test` (existing dialog tests untouched); taxi choices
still behave (boot `SCENE=STATE_CAR`, watch a timed choice appear and time
out with no crash; `run_log`/facts unaffected apart from legacy behavior).

### Step 2: STATE_AS_KITCHEN

Enum-sync per plan 008's checklist (append after AS_GREENROOM; bump test
count). Scene: small dark kitchen — code boxes are fine (counter, kettle
prop via `add_object(&g.scene, ..., "kettle", ...)`, a terminal object
`"terminal"`). Kettle E-press: `PlayInteract` + a narrator wry line via
rules (`concept=interact object=kettle scene=as_kitchen`). Terminal
E-press: fire the feed sequence (Step 3).

**Verify**: boots via dev; both props respond.

### Step 3: The feed beat

On terminal interact, run a small scripted sequence in
`as_kitchen_update` (file-scope state machine, pattern: the suite's
interaction_phases): three `dlg_game_speak("narrator","feed_post")` beats
paced ~4s apart — author 3 rules in aftershow.rules with `remember
feed_step+=1` so each query matches the next post (criteria
`feed_step<1`, `feed_step=1`, `feed_step=2` — most-specific-wins makes this
a sequence). The final post triggers
`dlg_game_offer("narrator", "thread_388", "Reply.", "Close the tab.", 8.0f)`.
Author the three resolution rules (`replied_thread_388=a`, `=b`,
`ignored_thread_388=1`) — the ignored line is the quietly devastating one.
After resolution (any path), `hard_cut_to(STATE_AS_GREENROOM)` after 2s.
`EV_SIGNAL` env (plan 008) must still work for direct Green Room boots.

**Verify**: full kitchen beat plays all three ways (reply / close / let it
time out) with distinct lines; cut lands in the Green Room.

### Step 4: Close the night

Green Room: after ≥2 prop interactions, enable an exit trigger (door
position; copy the corridor's `exit_pos` distance-check pattern) →
`hard_cut_to(STATE_AS_KITCHEN)` with a `night_over=1` dlg fact set before
the cut; kitchen's `concept=enter` rule gated `night_over=1` delivers the
closing line (severity: play it straight), then after 8s fade to
STATE_TITLE via `transition_to`. Add one Green-Room narrator line gated on
the feed choice (`replied_thread_388=a` vs `ignored_...`) — the slice's
permutation proof.

**Verify**: full loop kitchen→feed→greenroom→kitchen→title, under 10
minutes unrushed (time it once manually); permutation line differs across
two runs with different feed choices.

## Test plan

- `make test` green (count bump; dialog core untouched or extended-tested).
- Manual matrix: feed reply / close / ignore × suspicion 0 / 5 → 6 short
  runs; note each variant line observed in the commit message.
- QA regression for pre-existing scenes.

## Done criteria

- [ ] `dlg_game_offer` exists; taxi legacy behavior unchanged
- [ ] Silence on the feed prompt sets `ignored_thread_388` and gets its own line
- [ ] Full night loop completes and permutes on prologue + feed facts
- [ ] All enum-sync points + tests updated; QA regression clean
- [ ] `plans/README.md` row updated

## STOP conditions

- Reusing the GameCtx choice fields conflicts with an active EV choice in
  the same frame (shouldn't be reachable — AFTERSHOW states never overlap
  the taxi; if it somehow is, report).
- The feed sequence needs more than the rules engine offers (e.g. timed
  multi-line scripting beyond `remember feed_step+=1`) — report the gap.
- Ten-minute target overshoots 2× — report; cutting is a design call.

## Maintenance notes

- This slice is the go/no-go artifact for AFTERSHOW (ROADMAP Phase 3.5):
  after it exists, the owner plays it and decides feel before more content.
- The offer component is the one to promote into the Twine⇄engine shared
  vocabulary doc (`aftershow/systems/RULES_FORMAT.md`) once stable.
