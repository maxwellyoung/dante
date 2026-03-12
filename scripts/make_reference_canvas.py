#!/usr/bin/env python3

from __future__ import annotations

import argparse
from pathlib import Path

from PIL import Image


def build_reference_canvas(
    seed_path: Path,
    output_path: Path,
    *,
    canvas_size: int = 1024,
    slot_count: int = 4,
    slot_size: int = 256,
    slot_index: int = 0,
    upscale: int | None = None,
) -> None:
    seed = Image.open(seed_path).convert("RGBA")
    if upscale is None:
        upscale = max(1, min(slot_size // seed.width, slot_size // seed.height))

    scaled = seed.resize((seed.width * upscale, seed.height * upscale), resample=Image.Resampling.NEAREST)
    canvas = Image.new("RGBA", (canvas_size, canvas_size), (0, 0, 0, 0))

    strip_width = slot_count * slot_size
    strip_left = (canvas_size - strip_width) // 2
    strip_top = (canvas_size - slot_size) // 2
    slot_left = strip_left + (slot_index * slot_size)
    paste_x = slot_left + (slot_size - scaled.width) // 2
    paste_y = strip_top + (slot_size - scaled.height) // 2

    canvas.alpha_composite(scaled, (paste_x, paste_y))
    output_path.parent.mkdir(parents=True, exist_ok=True)
    canvas.save(output_path)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Build a transparent reference canvas for GPT sprite strip edits.")
    parser.add_argument("--seed", required=True, type=Path, help="Path to a canonical seed frame PNG.")
    parser.add_argument("--output", required=True, type=Path, help="Output PNG path.")
    parser.add_argument("--canvas-size", type=int, default=1024, help="Square canvas size.")
    parser.add_argument("--slot-count", type=int, default=4, help="Number of frame slots in the strip.")
    parser.add_argument("--slot-size", type=int, default=256, help="Size of each frame slot.")
    parser.add_argument("--slot-index", type=int, default=0, help="Which slot should receive the seed sprite.")
    parser.add_argument("--upscale", type=int, default=None, help="Optional nearest-neighbor upscale factor.")
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    build_reference_canvas(
        args.seed,
        args.output,
        canvas_size=args.canvas_size,
        slot_count=args.slot_count,
        slot_size=args.slot_size,
        slot_index=args.slot_index,
        upscale=args.upscale,
    )


if __name__ == "__main__":
    main()
