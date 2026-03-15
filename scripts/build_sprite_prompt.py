#!/usr/bin/env python3

from __future__ import annotations

import argparse
from pathlib import Path


PROMPT_TEMPLATE = """Intended use: candidate production spritesheet for a 2D side-view platformer {animation_name} animation review. Edit the provided transparent reference-canvas image into a single horizontal {slot_count}-frame {animation_name} spritesheet. The existing sprite in the {seed_slot} slot is the exact shipped seed frame and must remain the starting frame for this sequence: same compact hero, same {facing} side view, same silhouette family, same proportions, same palette discipline, same readable face, same outfit identity.
Composition: keep the image transparent, keep exactly one row of {slot_count} equal {slot_size}x{slot_size} frame slots laid out left to right across the {canvas_size}x{canvas_size} canvas, centered vertically, with no overlap between frame slots, no extra characters, no labels, and no UI.
Action: frame 1 stays as the approved starting pose. {action}
Consistency: keep body size, head size, limb proportions, and costume proportions consistent across all frames. Favor animation readability over exaggerated perspective.
Style: authentic 16-bit pixel art, crisp pixel clusters, stepped shading, restrained palette, production game asset, not concept art.
Constraints: {constraints}
Keep wide transparent empty space outside the frame slots."""


def ordinal_slot(index: int) -> str:
    lookup = {
        0: "leftmost",
        1: "second",
        2: "third",
        3: "fourth",
    }
    return lookup.get(index, f"slot {index + 1}")


def build_prompt(
    *,
    animation_name: str,
    slot_count: int,
    slot_size: int,
    canvas_size: int,
    seed_slot: int,
    facing: str,
    action: str,
    constraints: str,
) -> str:
    return PROMPT_TEMPLATE.format(
        animation_name=animation_name,
        slot_count=slot_count,
        slot_size=slot_size,
        canvas_size=canvas_size,
        seed_slot=ordinal_slot(seed_slot),
        facing=facing,
        action=action.strip(),
        constraints=constraints.strip(),
    )


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Build a reusable strip-edit prompt for GPT/Image sprite animation generation."
    )
    parser.add_argument("--animation", required=True, help="Animation name, for example hurt or shoot.")
    parser.add_argument("--action", required=True, help="Frame progression description after frame 1.")
    parser.add_argument(
        "--constraints",
        default="no scenery, no floor, no glow, no atmospheric haze, no impact effects, no shadows outside the sprite contours, no collage, no poster layout, no blurry details",
        help="Comma-separated hard constraints for the image edit.",
    )
    parser.add_argument("--slot-count", type=int, default=4, help="Number of slots in the strip.")
    parser.add_argument("--slot-size", type=int, default=256, help="Size of each slot in the edit canvas.")
    parser.add_argument("--canvas-size", type=int, default=1024, help="Square edit canvas size.")
    parser.add_argument("--seed-slot", type=int, default=0, help="Zero-based slot index containing the shipped seed frame.")
    parser.add_argument("--facing", default="right-facing", help="Character facing description.")
    parser.add_argument("--output", type=Path, default=None, help="Optional output text file path.")
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    prompt = build_prompt(
        animation_name=args.animation,
        slot_count=args.slot_count,
        slot_size=args.slot_size,
        canvas_size=args.canvas_size,
        seed_slot=args.seed_slot,
        facing=args.facing,
        action=args.action,
        constraints=args.constraints,
    )
    if args.output is not None:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(prompt + "\n", encoding="utf-8")
    else:
        print(prompt)


if __name__ == "__main__":
    main()
