#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
import math
from pathlib import Path

from PIL import Image


def _lua_quote(value: str) -> str:
    return '"' + value.replace("\\", "\\\\").replace('"', '\\"') + '"'


def _to_lua(value, indent: int = 0) -> str:
    spacer = " " * indent
    inner = " " * (indent + 4)

    if isinstance(value, dict):
        parts = ["{"]
        for key, item in value.items():
            parts.append(f"{inner}{key} = {_to_lua(item, indent + 4)},")
        parts.append(f"{spacer}}}")
        return "\n".join(parts)

    if isinstance(value, list):
        parts = ["{"]
        for item in value:
            parts.append(f"{inner}{_to_lua(item, indent + 4)},")
        parts.append(f"{spacer}}}")
        return "\n".join(parts)

    if isinstance(value, bool):
        return "true" if value else "false"

    if isinstance(value, str):
        return _lua_quote(value)

    return str(value)


def _validate_frame_sequence(frames: list[str]) -> None:
    expected = [f"frame-{index:02d}.png" for index in range(1, len(frames) + 1)]
    if frames != expected:
        raise ValueError(f"Frame list must be contiguous and zero-padded: expected {expected}, got {frames}")


def build_player_atlas(
    source_manifest_path: Path,
    atlas_path: Path,
    runtime_manifest_path: Path,
) -> dict:
    with source_manifest_path.open("r", encoding="utf-8") as handle:
        source_manifest = json.load(handle)

    assets_root = source_manifest_path.parent
    frame_size = source_manifest["frame_size"]
    animation_order = source_manifest["animation_order"]

    packed_frames = []
    runtime_manifest = {
        "frame_size": frame_size,
        "atlas_path": atlas_path.as_posix(),
        "render": source_manifest.get("render", {}),
        "animation_order": animation_order,
        "animations": {},
    }

    for name in animation_order:
        animation = source_manifest["animations"][name]
        frames = animation["frames"]
        _validate_frame_sequence(frames)

        runtime_manifest["animations"][name] = {
            "fps": animation["fps"],
            "loop": animation["loop"],
            "lock_frame_01": animation.get("lock_frame_01", False),
            "placeholder": animation.get("placeholder", False),
            "render": animation.get("render", {}),
            "frames": [],
        }

        for frame_name in frames:
            frame_path = assets_root / name / frame_name
            if not frame_path.exists():
                raise ValueError(f"Missing frame '{frame_name}' for animation '{name}'.")

            frame_image = Image.open(frame_path).convert("RGBA")
            if frame_image.size != (frame_size, frame_size):
                raise ValueError(
                    f"Frame '{frame_path}' must be {frame_size}x{frame_size}, got {frame_image.size}."
                )

            packed_frames.append((name, frame_path, frame_image))

    if not packed_frames:
        raise ValueError("No animation frames found to pack.")

    atlas_columns = math.ceil(math.sqrt(len(packed_frames)))
    atlas_rows = math.ceil(len(packed_frames) / atlas_columns)
    atlas = Image.new("RGBA", (atlas_columns * frame_size, atlas_rows * frame_size), (0, 0, 0, 0))

    for index, (name, frame_path, frame_image) in enumerate(packed_frames):
        cell_x = (index % atlas_columns) * frame_size
        cell_y = (index // atlas_columns) * frame_size
        atlas.alpha_composite(frame_image, (cell_x, cell_y))
        runtime_manifest["animations"][name]["frames"].append(
            {
                "x": cell_x,
                "y": cell_y,
                "w": frame_size,
                "h": frame_size,
                "source": frame_path.as_posix(),
            }
        )

    atlas_path.parent.mkdir(parents=True, exist_ok=True)
    atlas.save(atlas_path)

    runtime_manifest_path.parent.mkdir(parents=True, exist_ok=True)
    runtime_manifest_path.write_text("return " + _to_lua(runtime_manifest) + "\n", encoding="utf-8")
    return runtime_manifest


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Pack canonical player frames into a runtime atlas and Lua manifest.")
    parser.add_argument(
        "--source-manifest",
        type=Path,
        default=Path("assets/player/player_manifest.json"),
        help="Source manifest JSON with animation metadata and frame lists.",
    )
    parser.add_argument(
        "--atlas",
        type=Path,
        default=Path("assets/player/player_atlas.png"),
        help="Output atlas PNG path.",
    )
    parser.add_argument(
        "--runtime-manifest",
        type=Path,
        default=Path("assets/player/player_manifest.lua"),
        help="Output runtime Lua manifest path.",
    )
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    build_player_atlas(args.source_manifest, args.atlas, args.runtime_manifest)


if __name__ == "__main__":
    main()
