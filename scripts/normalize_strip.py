#!/usr/bin/env python3

from __future__ import annotations

import argparse
import shutil
from pathlib import Path

from PIL import Image


def _slot_boxes(image_size: tuple[int, int], slot_count: int, slot_size: int) -> list[tuple[int, int, int, int]]:
    width, height = image_size
    strip_width = slot_count * slot_size
    left = (width - strip_width) // 2
    top = (height - slot_size) // 2
    return [
        (left + (index * slot_size), top, left + ((index + 1) * slot_size), top + slot_size)
        for index in range(slot_count)
    ]


def _sprite_bounds(image: Image.Image, box: tuple[int, int, int, int]) -> tuple[int, int, int, int] | None:
    crop = image.crop(box)
    alpha_box = crop.getchannel("A").getbbox()
    if alpha_box is None:
        return None
    return (
        box[0] + alpha_box[0],
        box[1] + alpha_box[1],
        box[0] + alpha_box[2],
        box[1] + alpha_box[3],
    )


def _image_alpha_bounds(image: Image.Image) -> tuple[int, int, int, int] | None:
    alpha_box = image.getchannel("A").getbbox()
    if alpha_box is None:
        return None
    return alpha_box


def normalize_strip(
    strip_path: Path,
    output_dir: Path,
    *,
    slot_count: int,
    slot_size: int,
    frame_size: int = 64,
    padding: int = 6,
    foot_anchor: int | None = None,
    seed_path: Path | None = None,
    lock_frame_01: bool = False,
) -> list[Path]:
    strip = Image.open(strip_path).convert("RGBA")
    boxes = _slot_boxes(strip.size, slot_count, slot_size)
    bounds = [_sprite_bounds(strip, box) for box in boxes]

    if any(bound is None for bound in bounds):
        raise ValueError("Every slot must contain visible sprite pixels before normalization.")

    bounds = [bound for bound in bounds if bound is not None]
    max_width = max(bound[2] - bound[0] for bound in bounds)
    max_height = max(bound[3] - bound[1] for bound in bounds)

    seed_anchor_x = frame_size / 2
    if seed_path is not None:
        seed = Image.open(seed_path).convert("RGBA")
        if seed.size != (frame_size, frame_size):
            raise ValueError("Seed frame must already match the canonical frame size.")
        seed_bounds = _image_alpha_bounds(seed)
        if seed_bounds is not None:
            seed_anchor_x = (seed_bounds[0] + seed_bounds[2]) / 2
            if foot_anchor is None:
                foot_anchor = seed_bounds[3]

    foot_anchor = foot_anchor if foot_anchor is not None else frame_size - padding
    available_width = frame_size - (padding * 2)
    available_height = foot_anchor - padding
    scale = min(available_width / max_width, available_height / max_height)

    output_dir.mkdir(parents=True, exist_ok=True)
    frame_paths: list[Path] = []

    for index, bound in enumerate(bounds, start=1):
        sprite = strip.crop(bound)
        resized = sprite.resize(
            (
                max(1, round(sprite.width * scale)),
                max(1, round(sprite.height * scale)),
            ),
            resample=Image.Resampling.NEAREST,
        )

        frame = Image.new("RGBA", (frame_size, frame_size), (0, 0, 0, 0))
        paste_x = round(seed_anchor_x - (resized.width / 2))
        min_x = padding
        max_x = frame_size - padding - resized.width
        if max_x < min_x:
            paste_x = (frame_size - resized.width) // 2
        else:
            paste_x = max(min_x, min(max_x, paste_x))
        paste_y = round(foot_anchor - resized.height)
        frame.alpha_composite(resized, (paste_x, paste_y))

        frame_path = output_dir / f"frame-{index:02d}.png"
        frame.save(frame_path)
        frame_paths.append(frame_path)

    if lock_frame_01:
        if seed_path is None:
            raise ValueError("--lock-frame-01 requires --seed.")
        seed = Image.open(seed_path).convert("RGBA")
        if seed.size != (frame_size, frame_size):
            raise ValueError("Locked seed frame must already match the canonical frame size.")
        shutil.copyfile(seed_path, output_dir / "frame-01.png")

    return frame_paths


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Normalize a raw GPT strip into canonical fixed-size animation frames.")
    parser.add_argument("--strip", required=True, type=Path, help="Path to the raw strip PNG.")
    parser.add_argument("--output-dir", required=True, type=Path, help="Directory for normalized frame PNGs.")
    parser.add_argument("--slot-count", type=int, required=True, help="Number of frame slots in the strip.")
    parser.add_argument("--slot-size", type=int, required=True, help="Size of each frame slot in the raw strip.")
    parser.add_argument("--frame-size", type=int, default=64, help="Canonical output frame size.")
    parser.add_argument("--padding", type=int, default=6, help="Transparent padding around the sprite.")
    parser.add_argument("--foot-anchor", type=int, default=None, help="Pixel Y position of the shared foot anchor.")
    parser.add_argument("--seed", type=Path, default=None, help="Optional canonical seed frame used when locking frame 01.")
    parser.add_argument("--lock-frame-01", action="store_true", help="Replace frame-01 with the exact seed frame.")
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    normalize_strip(
        args.strip,
        args.output_dir,
        slot_count=args.slot_count,
        slot_size=args.slot_size,
        frame_size=args.frame_size,
        padding=args.padding,
        foot_anchor=args.foot_anchor,
        seed_path=args.seed,
        lock_frame_01=args.lock_frame_01,
    )


if __name__ == "__main__":
    main()
