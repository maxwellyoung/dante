# Game Mechanics

This file documents the mechanics that are actually part of the active roadmap.

## Baseline Kit

The baseline player kit is:

1. **Move**
2. **Jump**
3. **Jump buffer / coyote / jump cut**
4. **Wall interaction**
5. **Projectile fire**
6. **Grapple**

## Current Circle Logic

The game is not currently designed around losing hypothetical abilities like dash or double jump.

The active vertical slice structure is:

| Stage | Goal | Power State | Environmental Hook |
| --- | --- | --- | --- |
| Proving Ground | Tune the loop | Full baseline kit | None |
| Limbo | Teach the stable kit | Full baseline kit | Authored traversal and combat rooms |
| Circle 1: Lust | Deliver first subtraction | Grapple constrained / removed | Wind lanes |

## Design Rule

Every circle must follow this pattern:

- constrain or remove one advantage that currently matters
- replace it with one environment rule that is immediately legible
- make the rooms teach, test, then combine that rule

If a future mechanic does not survive this structure, it is not a pillar yet.
