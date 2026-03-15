#!/usr/bin/env python3

from __future__ import annotations

import argparse
from pathlib import Path

from PIL import Image


def align_frame(path: Path, foot_anchor: int) -> int:
    image = Image.open(path).convert("RGBA")
    bbox = image.getchannel("A").getbbox()
    if bbox is None:
        return 0

    _, _, _, bottom = bbox
    delta = foot_anchor - bottom
    if delta == 0:
        return 0

    shifted = Image.new("RGBA", image.size, (0, 0, 0, 0))
    shifted.alpha_composite(image, (0, delta))
    shifted.save(path)
    return delta


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Shift canonical sprite frames vertically so the visible feet hit a target anchor.")
    parser.add_argument("--dir", dest="directory", type=Path, required=True, help="Directory containing frame-XX.png files.")
    parser.add_argument("--foot-anchor", type=int, required=True, help="Target bottom pixel for the visible alpha bbox.")
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    frame_paths = sorted(args.directory.glob("frame-*.png"))
    if not frame_paths:
        raise SystemExit(f"No frame-*.png files found in {args.directory}")

    for frame_path in frame_paths:
        delta = align_frame(frame_path, args.foot_anchor)
        print(f"{frame_path.name}: shift_y={delta}")


if __name__ == "__main__":
    main()
