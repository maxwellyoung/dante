# Player Art Pipeline

This repo now treats `assets/player/<animation>/frame-XX.png` as the source of truth for player animation.

## Runtime shape

- Canonical frames are fixed at `64x64` with transparent backgrounds.
- `assets/player/player_manifest.json` defines animation order, frame lists, FPS, looping, and optional `lock_frame_01`.
- The manifest is also the source of truth for render metrics:
  - `render.scale`
  - `render.feet_anchor`
  - `render.draw_offset`
  - `render.placeholder`
- `scripts/build_player_atlas.py` packs those frames into:
  - `assets/player/player_atlas.png`
  - `assets/player/player_manifest.lua`

The game loads the generated Lua manifest at runtime. If you change any frame PNGs or the source manifest JSON, rebuild the atlas before launching the game.

## Workflow

1. Extract or choose a shipped seed frame.
2. Build a GPT edit canvas around it.
3. Generate a full strip in one image request.
4. Normalize the strip into canonical `64x64` frames with one shared scale.
5. Rebuild the runtime atlas and preview the result.
6. Launch the game and verify the animation in context.

## Commands

Build a reference canvas from a canonical seed frame:

```bash
python3 scripts/make_reference_canvas.py \
  --seed assets/player/idle/frame-01.png \
  --output tmp/hurt_reference.png \
  --slot-count 4 \
  --slot-size 256
```

Normalize a raw GPT strip into canonical frames:

```bash
python3 scripts/normalize_strip.py \
  --strip tmp/hurt_raw.png \
  --output-dir assets/player/hurt \
  --slot-count 4 \
  --slot-size 256 \
  --frame-size 64 \
  --seed assets/player/idle/frame-01.png \
  --lock-frame-01
```

Pack the runtime atlas and Lua manifest:

```bash
python3 scripts/build_player_atlas.py
```

Preview the imported animations without loading a level:

```bash
./run.sh --preview-player
```

Run the pipeline tests:

```bash
python3 -m unittest scripts.test_player_pipeline
```

## Notes

- Use one shared scale per strip. Do not rescale each frame independently.
- Keep feet anchored near the bottom of the frame so taller poses add height instead of shrinking the character.
- Use `lock_frame_01` for sequences like `hurt` where the first frame should remain the shipped idle sprite.
- Placeholder states must stay marked as `render.placeholder = true` until they survive gameplay-scale review.
- The target is a compact gameplay sprite, not a portrait. If the preview only looks good when magnified, the asset failed.
