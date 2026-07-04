# SFX triage — 2026-07-05

**Tooling:** `EV_DUMP_SFX=1 ./endearing_void` exports all 49 generated sounds
to `qa/sfx/*.wav` (hook: `finish_wave()` in audio.c). Analysis script output
below; rerun the numbers any time.

## Finding: the synthesis is technically clean

49/49 sounds: no clipping, no DC offset, sane peaks (all ≤0.5), proper
fade-outs. The two flagged start-transients (cork_pop, elevator_ding) are
*supposed* to start loud — they're strikes. **There are no bugs to fix.**

Which means the "our SFX is shit" problem is aesthetic: sine-and-noise
synthesis sounds thin because it IS thin — no body, no room, no material
complexity. That's a sound-design problem, and per the roadmap it needs a
deliberate direction decision (Phase 4: keep procedural purity vs sampled
sources), not blind parameter tuning by an agent without ears.

## What to do with this folder (Maxwell, ~10 minutes)

Open `qa/sfx/` in Finder, spacebar through all 49, and rank the five worst.
Candidates to listen for based on play-frequency (heard most = hurts most):
`step_marble`, `step_carpet`, `step_wood` (every second of play),
`click` (every interaction), `elevator_ding`, `door_sound`.

Write the five names at the bottom of this file; the next loop iteration
retunes exactly those (louder body via layered detuned partials, a touch of
convolution-ish tail) or swaps them to sampled one-shots if Phase 4 lands
that way.

## The five worst (fill in):

1.
2.
3.
4.
5.
