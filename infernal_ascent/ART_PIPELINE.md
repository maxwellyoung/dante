# Player Art Pipeline

This repo now treats `assets/player/<animation>/frame-XX.png` as the source of truth for player animation.

## Locked Art Spec

Use `assets/player/PLAYER_ART_SPEC.md` as the non-negotiable target before generating or importing any new player state.

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
4. Normalize the strip into canonical `64x64` frames with one shared scale and the shipped seed as the placement anchor.
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

Generate a reusable strip-edit prompt so the request stays consistent between runs:

```bash
python3 scripts/build_sprite_prompt.py \
  --animation hurt \
  --action "Frames 2 through 4 show a short hurt recoil from a hit, torso pulled back, brief pain expression, then slight recovery." \
  --output tmp/hurt_prompt.txt
```

Generate the full prompt set for every required player state:

```bash
python3 scripts/build_all_sprite_prompts.py
```

Run the actual image edit through the bundled OpenAI Image CLI once `OPENAI_API_KEY` is set:

```bash
export CODEX_HOME="${CODEX_HOME:-$HOME/.codex}"
export IMAGE_GEN="$CODEX_HOME/skills/imagegen/scripts/image_gen.py"

python3 scripts/make_reference_canvas.py \
  --seed assets/player/idle/frame-01.png \
  --output tmp/imagegen/hurt_reference.png \
  --slot-count 4 \
  --slot-size 256

python3 "$IMAGE_GEN" edit \
  --image tmp/imagegen/hurt_reference.png \
  --prompt-file assets/player/prompts/hurt_edit_prompt.txt \
  --no-augment \
  --size 1024x1024 \
  --quality high \
  --input-fidelity high \
  --background transparent \
  --output-format png \
  --out output/imagegen/player/hurt_raw.png
```

If you only want to validate the payload before making a live API call, add `--dry-run` to the final command.

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

Run slice/content validation:

```bash
python3 scripts/validate_slice_content.py
```

## State Production Checklist

Before a player state stops being placeholder art:

1. Generate at least two strip candidates from an approved seed frame.
2. Normalize candidates with one shared scale and the seed anchor.
3. Compare the imported state in the preview inspector at native scale first.
4. Rebuild the atlas and run runtime checks before clearing `placeholder`.
5. Keep the accepted prompt file and raw strip output for regeneration provenance.

## Notes

- Use one shared scale per strip. Do not rescale each frame independently.
- Keep feet anchored near the bottom of the frame so taller poses add height instead of shrinking the character.
- Pass the shipped seed frame into normalization even when frame 01 is not locked. The importer now uses that seed to derive the canonical horizontal anchor and default foot anchor.
- Use `lock_frame_01` for sequences like `hurt` where the first frame should remain the shipped idle sprite.
- The target is a compact gameplay sprite, not a portrait. If the preview only looks good when magnified, the asset failed.
- Live GPT image generation requires `OPENAI_API_KEY`. Without it, prompt/spec prep and validation still work, but live sprite generation is blocked.

Accepted state notes:

- `hurt`: keep `lock_frame_01` so the sequence starts on the exact shipped idle frame.
- `shoot`: prefer the stronger torso-twist variant over a flatter brace if the firing read gets muddy.
- `death`: keep the final pose low and clearly terminal so it does not read like an extended hurt reaction.
