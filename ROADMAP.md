# ROADMAP — from here to something novel

*The single prioritized plan. Synthesizes AUDIT_2026-07.md, ev_engine/CUT.md,
the Campo Santo triad (Ruskin's machine → Vanaman's permutation shape →
Remo's practice), and the AFTERSHOW thesis. When priorities conflict, this
document wins. Last revised: 2026-07-05.*

## The honest frame

The gap between "this slop" and Campo Santo is not systems anymore. As of
this week the stack runs the same lineage Firewatch ran: a fact/rule dialogue
engine (Ruskin), permutation endings (Vanaman), reactive triggers, hot-reload
authoring, VO-ready line IDs, a QA harness with defect counters. Campo Santo
shipped Firewatch with ~10 people in 2 years out of an apartment, and Remo's
talk is explicit about how: **clear priority order, cut everything that
models the job instead of the feeling, observe real players, say yes to the
player, edit down.** The remaining gap is craft-per-scene and finished-ness —
closed by shipping small, watching people play, and iterating. That is the
plan.

**North star:** ship ENDEARING VOID tight (the proof of craft), then build
AFTERSHOW (the novel one — a parasocial investigation where *listening* is
the verb, one fact vocabulary spanning a text prologue and a 3D game).
Nobody has made that. That's the Campo Santo move: Gravity Bone begat Thirty
Flights begat Firewatch.

---

## PHASE 1 — Ship EV (the next 4–6 weeks of sessions)

*Scope is ev_engine/CUT.md: 12–15 min, one spine, beginning/middle/end.
Priority order within the phase — top item first, always.*

1. **The track.** The bed-ritual piece is the only content gap on record
   (happy love song in a sad place — Chevalier move). Everything else is
   polish; this is absence. Maxwell-only task; everything below can proceed
   in parallel.
2. **Spine enforcement.** Wire the CUT.md flow end-to-end and play it
   whole: TITLE → … → STARS with no dead ends, no debug keys. Fix what
   breaks. Include the **validation playthrough** (Remo): one full run
   rushing everything, one lingering everywhere — both must hold.
3. **The stopwatch rule.** Speedrun each leg; every Gibbons waypoint line
   must fit the *minimum* walk time to his next stop (`dlg` durations vs
   measured legs). Cut lines to fit. Editing improves them.
4. **Look lock.** Decide the shipping style: 16mm default vs Grickle. Then
   one lighting pass per spine scene *under the locked look* — fewer,
   stronger key lights (bands and shadows want direction, not fill).
5. **Z-fight hand pass.** `zfight:` ≤ 5 per spine scene (auto-decal already
   took 819→~360 total; residuals are perpendicular abutments — fix visible
   ones, extend trim 1cm into walls).
6. **SFX triage pass.** Not the rework — the mute-worthy offenders only.
   Rank every sound by cringe, silence or soften the worst five, retune
   footsteps (most-heard sound in the game). Full audio direction is Phase 3.
7. **Couch playtests ×3** (Remo's apartment method): playtest build, watch
   them play in person or over screen-share, say nothing, take notes. After
   each: fix the top 3 pain points, add one "say yes" reaction a tester
   invented (their boombox-in-the-lake).
8. **Ship it.** itch.io, free, solo credit. Credits roll = Phase 1 done.

**Definition of done** (from CUT.md): three clean external runs, zfight gate,
the track, every scene reviewed once under the locked look.

---

## PHASE 2 — Reactivity depth (engine work, interleaves with Phase 1)

*Remo's principles the engine doesn't have yet. Each is small; together they
are the difference between "dialogue system" and "the game says yes."*

1. **Carry facts — the boombox move.** The grab system exists; the rules
   can't see it. Add `carrying=<object>` + `carried_into=<scene>` facts to
   `build_query()`. One line of plumbing, then writers can do: carry the
   wineglass to the balcony → Gibbons notices. Highest reactivity-per-hour
   in the codebase.
2. **The facts registry.** `make factsheet`: dump every fact key ever set or
   checked (symbol table + ev.rules scan) with where-used. Prevents the
   Delilah Brain failure (two names for one fact = false negatives hours in).
   Naming discipline note in ev.rules header: `gibbons_knows_x` ≠ `x_happened`
   — different states, both legal, never accidental.
3. **Reply windows** (Do You Copy / Firewatch): ~~prototype on EV's phone~~
   DONE, but landed differently: the phone's authored beat (ring fades as you
   approach, then just stops) is better than any prompt, and MASTER_PLAN bans
   dialogue trees in EV — so the phone stays pure. Silence-is-an-answer landed
   on the existing taxi backstory choices instead: `show_choice_timed()`, a
   thin burning line, timeout records backstory -1 + `choices_ignored` fact
   (Gibbons: "Quiet ride up, I'm told."). The full concept-firing offer
   component is deferred to Phase 3 where AFTERSHOW's feeds need it — don't
   build tools before content needs them (Remo).
4. **Reactive music manager.** Interaction-valley detection (time since last
   dialogue/interact > N, none imminent) → duck or inject ambient motif.
   `audio.c` already synthesizes; this is a conductor, ~100 lines.
5. **Never-twice posture.** Global default: a heard variant never replays
   across the run (session memory already tracks; flip the default, `resay`
   becomes the opt-in). Cheap now, mandatory once VO exists.

---

## PHASE 3 — AFTERSHOW vertical slice (after EV ships)

1. **Green Room greybox** — shell system end-to-end (its first real test),
   under the locked look. One room, five props, all speakable
   (`concept=interact`), re-rendering with suspicion facts like the
   prologue's rules do.
2. **The import.** `SIGNAL-XXXXX-N` code entry seeds `dlg_mem` — the Twine
   prologue's filed exhibits become the 3D game's opening state. Gravity
   Bone hard cut across mediums, made real.
3. **Reply-window mechanic** promoted from EV prototype to the core loop:
   feeds, DMs, the email draft — silence always recorded.
4. **Single-source rules compiler** (`.rules` → ev.rules + Twine JSON) the
   moment content is authored twice. Not before.
5. **Slice = one night**: kitchen → feed → Green Room → one exhibit → out.
   Ten minutes, permutes on prologue facts. If the slice doesn't produce
   the feeling, iterate here before building more.

---

## PHASE 4 — Production (only after the slice proves it)

- **Audio direction decision**: procedural-purity was EV's constraint;
  AFTERSHOW may want sampled/recorded sources (podcast audio IS the fiction).
  Revisit `audio.c` philosophy deliberately, not by drift.
- **VO**: cast Gibbons + narrator (EV re-release with voice?) and AFTERSHOW's
  hosts. `vox_sheet.py` is the pipeline; add `vox` clause + audio-length
  subtitle timing when first takes exist.
- **Ink outline shader** (depth/normal Sobel) — completes the storybook look.
- **Writer's-room cadence** (Remo): before writing any AFTERSHOW chapter, a
  session that solves plot + design together; GAMEDEV_BRAIN is the standing
  minutes.

---

## STANDING RULES (from the triad, non-negotiable)

1. **Priority order is the tiebreak**: narrative integrity > reactivity >
   visual fidelity > mechanical breadth. (Remo: you can only negotiate
   trade-offs consistently with clear priorities.)
2. **Cut what models the job, not the feeling.** Prototypes stay parked.
3. **Everything is canon** — no fail states, no reloads; support the
   possibility space instead.
4. **Every permutation must be a valid story** (Vanaman). The untouched-room
   ending is not a punishment.
5. **Writers implement without programmers** — extend tools
   (`build_query()`, rules format), never special-case content in code.
6. **Watch real players**; support what they invent within a week.
7. **Silence is data** — unanswered prompts, untouched props, skipped rooms
   all set facts.
8. **Edit down.** The stopwatch rule generalizes: shorter is better in every
   medium we touch.
9. **Muted testing** (EV_MUTE=1), zfight gate ≤5, no QA-grade chasing.
10. **The severity gradient** (Firewatch/Monkey Island): story beats carry
    full weight; the road between them carries play. If the player can walk
    away from it, it can be funny; if the game takes control, it cannot.
    (references/TONE_SEVERITY_GRADIENT.md)
11. **Ship small, then bigger** — Gravity Bone → Thirty Flights → Firewatch.
    EV → AFTERSHOW prologue (done) → AFTERSHOW.

## Immediate next actions (this week, in order)

1. Maxwell: the track. / Agent: spine enforcement run + stopwatch audit.
2. Carry facts (Phase 2.1 — one session).
3. Look lock decision session: same scene, 16mm vs Grickle, side by side.
4. First couch playtest scheduled.
5. PR #1 merged (needs Maxwell's explicit word) + aftershow repo created.
