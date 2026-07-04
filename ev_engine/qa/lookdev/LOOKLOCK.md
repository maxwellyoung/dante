# Look lock — 16mm vs Grickle

Comparisons in `qa/lookdev/compare/` (top = 16mm/default, bottom = Grickle).
Regenerate any time: `make qa` then `EV_QA_STYLE=9 make qa` (env var renders
the whole QA pass under any style, 0–9).

## What the shots say

- **Suite**: the decision image. 16mm is soft and photographic — competent,
  anonymous. Grickle is illustrated: cel bands turn gradients into shapes,
  the procedural wall squiggles read as pen work, the blockout reads as
  *drawn on purpose*. This is the Puzzle Agent register and it makes our
  cheapest geometry look intentional.
- **Corridor**: exposes the cost. Grickle crushes already-dark scenes to
  near-black — bands eat shadow detail. The corridor is barely readable in
  BOTH looks at the hero angle (a lighting problem before a style problem),
  but Grickle punishes it harder.
- **Taxi/lobby**: Grickle's warm-paper tint suits the Auckland-night interiors.

## Recommendation

**Lock Grickle** — with the proviso the roadmap already carries: dark scenes
need a lighting retune under it (raise fill floor ~0.05 or add one bounce
light per dark scene so the bottom band isn't pure black). The style is doing
exactly what we hired it for: covering blockout flaws by reframing them as
illustration. 16mm keeps a gradient's worth of detail but spends it looking
like an unfinished photoreal game; Grickle spends the same pixels looking
like a finished illustrated one.

Maxwell decides: play both (Shift+1 vs Shift+0) for ten minutes each in the
suite and corridor. If Grickle wins, `g.current_style` default flips to 9 and
the corridor/bed lighting retune becomes the next look task.

## Corridor diagnosis (2026-07-05 loop)

Hull walls are warm gray (RGB 55–90) yet render black at the hero angle:
light transport, not materials. Steep overhead key (0,-0.9,-0.2) puts the
whole interior in shadow-factor; amber pools (r≈17 at y=2) don't reach the
far hull at grazing angles. Ambient floor raised 0.10→0.13 and the far pool
warmed/enlarged this session (luma 35→34, insufficient). Needs an in-engine
session with F1/F4 after look lock: candidate fixes — shallower key angle
down the corridor axis, +1 bounce fill from the floor strips, or hull
material warmth lift. Don't polish before the look is locked (Phase 1.4).
