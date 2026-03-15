# Player Art Spec

This is the locked production spec for Dante's first capture-ready player sprite set.

## Goal

The sprite must read as a compact action-platformer character at native gameplay scale.

If the sprite only looks good in the magnified preview, it failed.

## Hard Requirements

- Frame size: `64x64`
- Facing: right-facing side view
- Background: transparent
- Feet anchor: low and stable across every animation
- Body height target: roughly `36-42px` inside the `64x64` frame
- Silhouette: compact, readable, and game-first
- Palette: restrained, warm earth + infernal accents that sit cleanly against Limbo and Lust backgrounds

## Character Identity

- Compact pilgrim / infernal climber silhouette
- Red bandana or hood accent
- Warm tan skin
- Dark vest or tunic over lighter undershirt
- Brown boots
- Readable face, but never portrait-scale

## Animation Rules

- `idle`: calm but alert, compact stance
- `run`: clear forward lean, readable stride, no giant robe-like volume
- `jump`: committed upward takeoff pose
- `fall`: controlled descent, readable downward state
- `shoot`: clear firing pose with brief recoil, no giant muzzle theatrics
- `hurt`: recoil and pain read instantly without breaking proportions
- `death`: collapse reads clearly as final and one-shot

## Generation Rules

- Generate strips, not frame-by-frame collage edits
- Keep one shared scale across a strip
- Keep feet alignment stable via normalization
- Favor silhouette readability over costume detail
- Avoid exaggerated perspective, painterly rendering, soft blur, atmospheric haze, or poster composition

## Review Gate

A state passes only if:

1. it reads at native gameplay scale
2. feet do not visibly wobble frame-to-frame
3. the state meaning is obvious in motion without UI or labels

Any failing state stays placeholder.

## Strict QA Thresholds

These are enforced by [audit_player_animation.py](/Users/maxwellyoung/Development/dante/scripts/audit_player_animation.py).

- locomotion:
  - foot drift `<= 1px`
  - center drift `<= 2px` for `idle`, `jump`, and `fall`
  - center drift `<= 4px` for `run`
  - width `<= 32px`
  - height range `<= 4px`, except `jump <= 6px`
  - connected components `<= 1`
  - internal holes `<= 1px`
- one-shots:
  - `frame-01` must match the canonical idle seed when the state starts from idle
  - foot drift `<= 1px` unless the state intentionally collapses
  - connected components `<= 1` for upright frames
  - internal holes `<= 2px`
  - `shoot` width `<= 36px`
  - `death` may widen or flatten only after the early collapse frames

## Promotion Pipeline

Use the promotion script instead of ad hoc manual promotion:

```bash
python3 scripts/promote_player_state.py --state shoot --strip output/imagegen/player/shoot_compact_live.png --slot-count 3 --strict
```

Pipeline order:

1. generate or edit a strip
2. normalize to canonical `64x64`
3. apply deterministic cleanup only for holes, detached pixels, and foot alignment
4. rerun audit
5. promote only if the strict gate passes
