#!/usr/bin/env python3

from __future__ import annotations

from pathlib import Path
import sys

SCRIPT_DIR = Path(__file__).resolve().parent
if str(SCRIPT_DIR) not in sys.path:
    sys.path.insert(0, str(SCRIPT_DIR))

from build_sprite_prompt import build_prompt


PROMPTS = {
    "idle": {
        "slot_count": 3,
        "action": "Frames 2 through 3 show a subtle alert idle loop: a small breathing shift, slight cloth settle, and tiny head/shoulder adjustment while preserving the same compact stance.",
        "constraints": "no weapon, no scenery, no floor, no glow, no atmospheric haze, no impact effects, no dramatic cape shapes, no giant robe volume, no blurry details",
    },
    "run": {
        "slot_count": 6,
        "action": "Frames 2 through 6 form a compact six-frame run cycle with a clear forward lean, readable stride, tight arm swing, and stable body mass. Keep the silhouette athletic and game-readable rather than illustrative.",
        "constraints": "no weapon, no scenery, no floor, no glow, no atmospheric haze, no impact effects, no giant robe flare, no blurry details",
    },
    "jump": {
        "slot_count": 3,
        "action": "Frames 2 through 3 show a committed jump takeoff sequence: crouch compression into lift, then the airborne takeoff pose. Keep the body compact and the silhouette readable.",
        "constraints": "no weapon, no scenery, no floor, no glow, no atmospheric haze, no exaggerated squash-and-stretch, no blurry details",
    },
    "fall": {
        "slot_count": 3,
        "action": "Frames 2 through 3 show a controlled falling loop with a clear downward read, legs gathered under the body, and a compact in-air silhouette that contrasts cleanly against the jump pose.",
        "constraints": "no weapon, no scenery, no floor, no glow, no atmospheric haze, no dramatic cloth expansion, no blurry details",
    },
    "hurt": {
        "slot_count": 4,
        "action": "Frames 2 through 4 show a short hurt recoil from a hit, torso pulled back, head jolted, brief pain expression, then slight recovery. Keep the same compact gameplay silhouette in every frame.",
        "constraints": "no weapon, no scenery, no floor, no glow, no atmospheric haze, no impact effects, no shadows outside the sprite contours, no collage, no poster layout, no blurry details",
    },
    "shoot": {
        "slot_count": 3,
        "action": "Frames 2 through 3 show a quick ranged firing snap with a stronger torso twist, off-hand counterbalance, and short recoil return while keeping the body compact and planted.",
        "constraints": "no muzzle flash, no projectile effects, no weapon larger than the hand silhouette, no scenery, no floor, no glow, no atmospheric haze, no blurry details",
    },
    "death": {
        "slot_count": 4,
        "action": "Frames 2 through 4 show a one-way death collapse: impact, knees buckling, then the body dropping into a final defeated pose. Preserve the same body scale and avoid excessive theatrics.",
        "constraints": "no blood splash, no scenery, no floor, no glow, no atmospheric haze, no dramatic ragdoll distortion, no blurry details",
    },
}


def main() -> None:
    prompts_dir = Path("assets/player/prompts")
    prompts_dir.mkdir(parents=True, exist_ok=True)

    for animation_name, config in PROMPTS.items():
        prompt = build_prompt(
            animation_name=animation_name,
            slot_count=config["slot_count"],
            slot_size=256,
            canvas_size=1024,
            seed_slot=0,
            facing="right-facing",
            action=config["action"],
            constraints=config["constraints"],
        )
        (prompts_dir / f"{animation_name}_edit_prompt.txt").write_text(prompt + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
