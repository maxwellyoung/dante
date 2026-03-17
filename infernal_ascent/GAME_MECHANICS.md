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
7. **Dash**
8. **Crouch**
9. **Roll**

## Feel Contract

The committed public kit for the vertical slice is now:

| Verb | Input | Core Use | Current Contract |
| --- | --- | --- | --- |
| Move | `A` / `D` | Constant horizontal commitment | Stable baseline. No new movement verbs beyond the current kit. |
| Jump | `Space` | Gap crossing and recovery | Jump buffer, coyote time, jump cut, wall jump, and grapple release remain core. |
| Shoot | `LMB` | Movement pressure through violence | No standing-still combat mode. Shooting should support lane control and now carries one default wall ricochet for bank shots. |
| Grapple | `E` / `RMB` | Anchor commitment and route compression | Present in Proving Ground and Limbo, removed in Lust. |
| Dash | `LShift` | Burst through one authored moment | Grounded or air burst with one committed use before reset. |
| Crouch | `S` / down | Low-profile bracing | Grounded only. Current gameplay effect is bracing against wind; no hitbox shrink yet. |
| Roll | `C` / `Ctrl` | Fast grounded commitment | Grounded burst with recovery. No i-frames or combat routing yet. |

Reset rules:

- landing restores dash
- wall contact restores dash
- grapple hook state restores dash once released or grounded
- checkpoint and respawn reset every committed verb to a clean readable state

Non-goals for this milestone:

- no new player verbs
- no crouch hitbox shrink yet
- no roll i-frames yet
- no generalized combat cancels beyond what the current kit already exposes

## Current Circle Logic

The active slice is designed around subtracting one real advantage at a time.

The active vertical slice structure is:

| Stage | Goal | Power State | Environmental Hook |
| --- | --- | --- | --- |
| Proving Ground | Tune the full loop | Full committed kit | Authored drills |
| Limbo | Teach the stable verbs | Full committed kit | Authored traversal and combat rooms |
| Circle 1: Lust | Deliver first subtraction | Grapple removed | Wind lanes plus posture control |

## Design Rule

Every circle must follow this pattern:

- constrain or remove one advantage that currently matters
- replace it with one environment rule that is immediately legible
- make the rooms teach, test, then combine that rule

If a future mechanic does not survive this structure, it is not a pillar yet.
