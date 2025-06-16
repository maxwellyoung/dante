# Todo List for Infernal Ascent

This file tracks the development progress of the game.

## Gameplay & Mechanics

- [ ] Design Circle 4 (Greed) - Challenge based on losing a new ability (e.g., Item Pickup).
- [ ] Design subsequent Circles (Wrath, Heresy, etc.).
- [ ] Implement new environmental hazards (e.g., moving platforms, timed traps).
- [ ] **Stretch Goal:** Implement Grapple Hook as a mid-game ability.

## Art & Aesthetics

- [x] Create a real spritesheet for the player with idle, run, jump, fall, and dash animations.
- [ ] Create a simple tileset for the level geometry to replace the solid-color rectangles.
- [ ] Design distinct visual themes for each Circle of Inferno (e.g., grayscale for Limbo, red shift for Lust).
- [ ] Add more particle effects for wall slides, landing hard, etc.

## Audio

- [ ] Implement a basic sound manager.
- [ ] Add placeholder sound effects for jump, dash, land, death, and ability loss.
- [ ] Compose a chiptune-style theme that gets more distorted with each descent.

## UI & UX

- [ ] Create graphical icons for the abilities in the UI.
- [d] Player should not spawn in a wall or fall immediately.
- [d] Add a pause menu.
- [d] Add a restart button.

## Bugs & Polish

- [d] Fix player disappearing on collision.
- [d] Fix startup crash related to `player.update`.
- [ ] Fine-tune player physics constants (gravity, jump height, speed) for optimal "game feel."
