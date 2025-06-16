# GAME_MECHANICS.md

## Player Abilities (Paradiso State)

The player starts with a full suite of responsive, "game-feel" focused abilities.

1.  **Move:** Standard left/right movement. Quick acceleration and deceleration.
2.  **Jump:** A variable-height jump based on how long the button is held.
3.  **Double Jump:** A second jump can be initiated at any point in the air. This has a distinct visual/audio cue.
4.  **Dash:** A fixed-distance, 8-directional air-dash with a brief cooldown. The player's momentum is halted before the dash, providing precise control.
    - _Controls: Arrow keys for direction + a dedicated dash button._
5.  **Wall Cling/Slide:** The player can cling to walls and slide down them at a controlled speed. This also serves as a staging point for a wall-jump.

## Controls (Default)

- **Arrow Keys / WASD:** Move Left/Right
- **Space / C:** Jump / Double Jump
- **Z / X / K:** Dash
- **Shift (Hold):** Wall Cling (when against a wall)

## Core Loop: Grace is Subtraction

The player descends through nine circles. At the gate of each circle, a Guardian takes one ability. The level design of the subsequent circle is built around the _absence_ of this ability, forcing creative problem-solving.

### Order of Loss (Tentative)

| Circle       | Sin       | Ability Lost    | Design Consequence                                                                                      |
| :----------- | :-------- | :-------------- | :------------------------------------------------------------------------------------------------------ |
| **Limbo**    | (Passage) | **Double Jump** | Platforms require more precise, committed single jumps. Verticality is restricted.                      |
| **Lust**     | Lust      | **Dash**        | Traversal is slower. Gaps must be cleared with momentum, not bursts. No more panic-dodging.             |
| **Gluttony** | Gluttony  | **Wall Cling**  | Can no longer scale sheer surfaces or recover from a missed jump easily. Fall paths are more punishing. |

| ... _and so on._

This structure ensures the game's difficulty curve is inverted. It becomes less about execution (complex button combos) and more about observation and planning.
