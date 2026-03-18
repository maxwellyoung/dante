# Architecture Refactor Plan

## Status: READY TO IMPLEMENT
Build baseline: 0 warnings, 0 errors, all tests pass.
Prerequisite: No other sessions editing src/ files simultaneously.

## Current State (3496 lines in main.c)
- ~100 file-scope statics (lines 22-167)
- Menu system (~180 lines, 170-318)
- Transitions (~50 lines, 320-347)
- Nudge/debug (~60 lines, 349-403)
- load_state (~500 lines, 405-903)
- Vignette text (~15 lines, 910-921)
- main() + game loop (~2500 lines, 924-3496)

## Phase 1: GameCtx Struct
Create game_ctx.h. Move all file-scope statics into single GameCtx struct.
~1700 reference updates (mechanical find-replace).

## Phase 2: Extract QA (~450 lines → qa.c)
Move entire #ifdef QA_MODE block. Self-contained, zero risk.

## Phase 3: Extract Menu (~180 lines → menu.c/menu.h)
Move pause/settings menu. Make g non-static.

## Phase 4: Extract Cinema (if cinema code exists)
Move cinematic FX. Currently ~0 lines (may have been inlined/removed).

## Phase 5: Scene Registry
Function pointer table indexed by GameState.
Split load_state switch + update switch into per-scene files.
Group by act: bookend, terrestrial, space.

## Execution: ONE phase at a time, build+test after each.
