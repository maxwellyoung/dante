#!/usr/bin/env python3

from __future__ import annotations

import argparse
from collections import deque
from pathlib import Path

from PIL import Image


def alpha_bbox(image: Image.Image) -> tuple[int, int, int, int] | None:
    return image.getchannel("A").getbbox()


def collect_components(alpha: Image.Image) -> list[list[tuple[int, int]]]:
    width, height = alpha.size
    pixels = alpha.load()
    visited: set[tuple[int, int]] = set()
    components: list[list[tuple[int, int]]] = []

    for y in range(height):
        for x in range(width):
            if pixels[x, y] == 0 or (x, y) in visited:
                continue
            queue = deque([(x, y)])
            visited.add((x, y))
            component: list[tuple[int, int]] = []
            while queue:
                cx, cy = queue.popleft()
                component.append((cx, cy))
                for nx, ny in ((cx + 1, cy), (cx - 1, cy), (cx, cy + 1), (cx, cy - 1)):
                    if 0 <= nx < width and 0 <= ny < height and pixels[nx, ny] > 0 and (nx, ny) not in visited:
                        visited.add((nx, ny))
                        queue.append((nx, ny))
            components.append(component)
    return components


def collect_holes(alpha: Image.Image) -> list[list[tuple[int, int]]]:
    width, height = alpha.size
    pixels = alpha.load()
    exterior: set[tuple[int, int]] = set()
    queue: deque[tuple[int, int]] = deque()

    for x in range(width):
        for y in (0, height - 1):
            if pixels[x, y] == 0 and (x, y) not in exterior:
                exterior.add((x, y))
                queue.append((x, y))
    for y in range(height):
        for x in (0, width - 1):
            if pixels[x, y] == 0 and (x, y) not in exterior:
                exterior.add((x, y))
                queue.append((x, y))

    while queue:
        cx, cy = queue.popleft()
        for nx, ny in ((cx + 1, cy), (cx - 1, cy), (cx, cy + 1), (cx, cy - 1)):
            if 0 <= nx < width and 0 <= ny < height and pixels[nx, ny] == 0 and (nx, ny) not in exterior:
                exterior.add((nx, ny))
                queue.append((nx, ny))

    holes: list[list[tuple[int, int]]] = []
    for y in range(height):
        for x in range(width):
            if pixels[x, y] != 0 or (x, y) in exterior:
                continue
            queue = deque([(x, y)])
            exterior.add((x, y))
            hole: list[tuple[int, int]] = []
            while queue:
                cx, cy = queue.popleft()
                hole.append((cx, cy))
                for nx, ny in ((cx + 1, cy), (cx - 1, cy), (cx, cy + 1), (cx, cy - 1)):
                    if 0 <= nx < width and 0 <= ny < height and pixels[nx, ny] == 0 and (nx, ny) not in exterior:
                        exterior.add((nx, ny))
                        queue.append((nx, ny))
            holes.append(hole)
    return holes


def fill_small_holes(image: Image.Image, max_hole_pixels: int) -> int:
    bbox = alpha_bbox(image)
    if bbox is None:
        return 0

    crop = image.crop(bbox)
    alpha = crop.getchannel("A")
    pixels = crop.load()
    filled = 0

    for hole in collect_holes(alpha):
        if len(hole) > max_hole_pixels:
            continue
        for x, y in hole:
            neighbors = []
            for nx, ny in ((x + 1, y), (x - 1, y), (x, y + 1), (x, y - 1)):
                if 0 <= nx < crop.width and 0 <= ny < crop.height and pixels[nx, ny][3] > 0:
                    neighbors.append(pixels[nx, ny])
            if neighbors:
                r = round(sum(color[0] for color in neighbors) / len(neighbors))
                g = round(sum(color[1] for color in neighbors) / len(neighbors))
                b = round(sum(color[2] for color in neighbors) / len(neighbors))
                a = round(sum(color[3] for color in neighbors) / len(neighbors))
                pixels[x, y] = (r, g, b, a)
                filled += 1

    image.paste((0, 0, 0, 0), bbox)
    image.alpha_composite(crop, (bbox[0], bbox[1]))
    return filled


def remove_small_components(image: Image.Image, max_component_pixels: int) -> int:
    bbox = alpha_bbox(image)
    if bbox is None:
        return 0

    crop = image.crop(bbox)
    alpha = crop.getchannel("A")
    pixels = crop.load()
    removed = 0

    components = collect_components(alpha)
    if not components:
        return 0

    largest = max(len(component) for component in components)
    for component in components:
        if len(component) == largest or len(component) > max_component_pixels:
            continue
        for x, y in component:
            pixels[x, y] = (0, 0, 0, 0)
            removed += 1

    image.paste((0, 0, 0, 0), bbox)
    image.alpha_composite(crop, (bbox[0], bbox[1]))
    return removed


def align_to_foot_anchor(image: Image.Image, foot_anchor: int) -> int:
    bbox = alpha_bbox(image)
    if bbox is None:
        return 0

    bottom = bbox[3]
    delta = foot_anchor - bottom
    if delta == 0:
        return 0

    shifted = Image.new("RGBA", image.size, (0, 0, 0, 0))
    shifted.alpha_composite(image, (0, delta))
    image.paste(shifted)
    return delta


def align_to_center(image: Image.Image, center_x: int) -> int:
    bbox = alpha_bbox(image)
    if bbox is None:
        return 0

    current_center = bbox[0] + ((bbox[2] - bbox[0]) / 2)
    delta = round(center_x - current_center)
    if delta == 0:
        return 0

    shifted = Image.new("RGBA", image.size, (0, 0, 0, 0))
    shifted.alpha_composite(image, (delta, 0))
    image.paste(shifted)
    return delta


def cleanup_directory(
    directory: Path,
    foot_anchor: int,
    max_hole_pixels: int,
    max_component_pixels: int,
    skip_frame_01: bool,
    center_x: int | None = None,
) -> list[dict[str, int | str]]:
    results = []
    for frame_path in sorted(directory.glob("frame-*.png")):
        if skip_frame_01 and frame_path.name == "frame-01.png":
            results.append({"frame": frame_path.name, "filled": 0, "removed": 0, "shift_y": 0})
            continue

        image = Image.open(frame_path).convert("RGBA")
        filled = fill_small_holes(image, max_hole_pixels)
        removed = remove_small_components(image, max_component_pixels)
        shift_x = align_to_center(image, center_x) if center_x is not None else 0
        shift_y = align_to_foot_anchor(image, foot_anchor)
        image.save(frame_path)
        results.append(
            {
                "frame": frame_path.name,
                "filled": filled,
                "removed": removed,
                "shift_x": shift_x,
                "shift_y": shift_y,
            }
        )
    return results


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Apply deterministic cleanup to canonical player frames.")
    parser.add_argument("--dir", dest="directory", type=Path, required=True, help="Directory containing frame-XX.png files.")
    parser.add_argument("--foot-anchor", type=int, required=True, help="Target visible foot bottom.")
    parser.add_argument("--max-hole-pixels", type=int, default=1, help="Fill interior holes up to this size.")
    parser.add_argument("--max-component-pixels", type=int, default=4, help="Remove detached components up to this size.")
    parser.add_argument("--skip-frame-01", action="store_true", help="Do not mutate frame-01.")
    parser.add_argument("--center-x", type=int, default=None, help="Optional target visible bbox center.")
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    results = cleanup_directory(
        args.directory,
        args.foot_anchor,
        args.max_hole_pixels,
        args.max_component_pixels,
        args.skip_frame_01,
        center_x=args.center_x,
    )
    for result in results:
        print(
            f"{result['frame']}: filled={result['filled']} removed={result['removed']} shift_x={result.get('shift_x', 0)} shift_y={result['shift_y']}"
        )


if __name__ == "__main__":
    main()
