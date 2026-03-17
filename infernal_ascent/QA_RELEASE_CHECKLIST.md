# Infernal Ascent QA Release Checklist

This is the shared gate for the internal vertical slice.

## Runtime Gate

Run:

```bash
python3 scripts/run_game_checks.py
python3 scripts/run_autoplay_check.py --strict
python3 scripts/run_campaign_smoke.py --profile vertical-slice
```

Required:

- background QA is the default gating path on macOS
- proving-ground autoplay passes its strict room expectations on all variants
- traversal records a clean authored grapple commitment before the room is considered complete
- dash lane records at least one committed dash through the choke before the room is considered complete
- posture lane records one authored roll, readable crouch time, and a clean wind label before the room is considered complete
- weapon tracers render clean ricochet segments without disappearing or folding back into walls
- combat and pressure rooms complete with encounter clear and readable screenshots
- campaign smoke captures `start`, `post_intro`, and `settled` evidence for the key vertical-slice rooms
- Limbo gate, Lust transition, Lust teaching, and Lust gate screenshots are captured and validated by the smoke profile

Foreground QA is optional and used for deliberate visual/feel review:

```bash
python3 scripts/run_game_checks.py --foreground-qa
python3 scripts/run_autoplay_check.py --foreground
python3 scripts/run_campaign_smoke.py --foreground --profile vertical-slice
```

## Asset Gate

Run:

```bash
python3 scripts/audit_player_animation.py --strict
python3 scripts/validate_slice_content.py --require-final-art
```

Required:

- every required player state exists
- no required state remains placeholder
- no strict audit violations remain
- one-shot frame `01` matches the canonical idle seed where required

## Screenshot Review Rubric

Check autoplay and campaign smoke shots for:

- player reads at gameplay scale without magnified-only detail
- no HUD overlap with intros, room labels, or result banners
- hazards, anchors, wind zones, gates, and checkpoints remain legible
- ricochet lines read cleanly enough that a bank shot is understandable at gameplay speed
- crouch silhouette reads at gameplay scale
- roll silhouette reads as a committed grounded burst
- wall cling and wall release remain readable instead of gluey
- no obvious alpha holes, detached islands, or silhouette fragmentation
- player does not disappear into the background
- story beats and objective text remain distinct instead of collapsing into one banner
- Limbo and Lust have distinct room identity without muddying gameplay

## Promotion Rule

Player art promotion must follow this order:

1. generate or edit strip
2. normalize to canonical `64x64`
3. run deterministic cleanup only for holes, detached pixels, and foot alignment
4. rerun audit
5. promote only if strict audit passes
