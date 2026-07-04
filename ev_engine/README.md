# ENDEARING VOID

A first-person exploration of a Paris hotel at 2 AM.

By **Maxwell Young**

[screenshot coming]

---

## Build & Run

```bash
make          # build
make run      # build and run
make dev SCENE=STATE_PROTO_LAB        # boot straight into the prototype selector
make dev SCENE=STATE_PROTO_MOVEMENT   # boot the movement slice
make dev SCENE=STATE_PROTO_SHOOTER    # boot the shooter slice
make dev SCENE=STATE_PROTO_PUZZLE     # boot the puzzle slice
```

Requires [Raylib](https://www.raylib.com/) installed via Homebrew (`brew install raylib`).

## Controls

| Key | Action |
|-----|--------|
| WASD | Move |
| Mouse | Look |
| MMB | Grapple to authored prototype anchors |
| Shift | Sprint |
| Space | Jump |
| E | Interact |
| Q | Dash |
| R | Restart current prototype slice |
| Tab | Return to prototype lab from a slice |
| P | Open prototype lab from the title screen |
| Esc | Pause menu, including a jump to Prototype Lab |
| F1 | Wireframe |
| F3 | Debug overlay |
| F4 | Noclip |

## The Game

You arrive at a small Paris hotel with three hours to kill before your flight. It is 2 AM. The city sparkles outside. You explore the lobby, climb the stairs, find your room. There are small things to do. None of them matter. All of them matter.

## Prototype Lab

The 3D track now includes a separate prototype race inside the same engine:

- `STATE_PROTO_MOVEMENT` tests route readability and movement chaining.
- `STATE_PROTO_SHOOTER` tests mobility plus hitscan pressure.
- `STATE_PROTO_PUZZLE` tests anchor-routing / bridge-link spatial reasoning.

The selector is now a real lab shell with four modes:

- `Play Slice`
- `Review Metrics`
- `Compare Last Runs`
- `Reset Slice Scores`

Each slice records `last run`, `best run`, and a shared debrief:
`Would Replay`, `Readability`, `Mechanical Depth`, `Ship Confidence`,
`Best Moment`, and `Main Friction`.

Slice-specific combat/prototype mechanics:

- Movement: safe route, fast route, mastery route, timed finish window, scramble recovery, route anchor.
- Shooter: pulse fire, breach fire, authored grapple anchors, ricochet plates, heat management, threat nodes, moving safe pockets, route tools.
- Puzzle: relay chain, router reroute, staged bridge activation, pulsing final crossing, readable anchor language.

## Tech

C99. Raylib. Custom GLSL lighting. Procedural audio. 480x300 rendered to window.

---

All rights reserved.
