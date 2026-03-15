#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
from pathlib import Path
from collections import deque
import hashlib

from PIL import Image, ImageDraw

LOCOMOTION_STATES = {"idle", "run", "jump", "fall"}
ONE_SHOT_STATES = {"hurt", "shoot", "death"}

STRICT_THRESHOLDS = {
    "idle": {
        "foot_drift_range": 1,
        "center_drift_range": 2,
        "height_range": 4,
        "max_width": 32,
        "max_components": 1,
        "max_holes": 1,
        "max_hole_pixels": 1,
    },
    "run": {
        "foot_drift_range": 1,
        "center_drift_range": 4,
        "height_range": 4,
        "max_width": 32,
        "max_components": 1,
        "max_holes": 1,
        "max_hole_pixels": 1,
    },
    "jump": {
        "foot_drift_range": 1,
        "center_drift_range": 2,
        "height_range": 6,
        "max_width": 32,
        "max_components": 1,
        "max_holes": 1,
        "max_hole_pixels": 1,
    },
    "fall": {
        "foot_drift_range": 1,
        "center_drift_range": 2,
        "height_range": 4,
        "max_width": 32,
        "max_components": 1,
        "max_holes": 1,
        "max_hole_pixels": 1,
    },
    "hurt": {
        "foot_drift_range": 1,
        "center_drift_range": 3,
        "height_range": 8,
        "max_width": 34,
        "max_components": 1,
        "max_holes": 2,
        "max_hole_pixels": 2,
    },
    "shoot": {
        "foot_drift_range": 1,
        "center_drift_range": 3,
        "height_range": 8,
        "max_width": 36,
        "max_components": 1,
        "max_holes": 2,
        "max_hole_pixels": 2,
    },
}


def alpha_bounds(image: Image.Image) -> dict[str, float] | None:
    bbox = image.getchannel("A").getbbox()
    if bbox is None:
        return None

    left, top, right, bottom = bbox
    width = right - left
    height = bottom - top
    return {
        "left": left,
        "top": top,
        "right": right,
        "bottom": bottom,
        "width": width,
        "height": height,
        "center_x": left + width / 2,
        "center_y": top + height / 2,
        "foot_y": bottom,
    }


def alpha_topology(image: Image.Image, bbox: tuple[int, int, int, int] | None) -> dict[str, int]:
    if bbox is None:
        return {
            "components": 0,
            "small_components": 0,
            "holes": 0,
            "hole_pixels": 0,
        }

    alpha = image.getchannel("A").crop(bbox)
    width, height = alpha.size
    pixels = alpha.load()
    visited: set[tuple[int, int]] = set()
    component_sizes: list[int] = []

    for y in range(height):
        for x in range(width):
            if pixels[x, y] == 0 or (x, y) in visited:
                continue
            queue = deque([(x, y)])
            visited.add((x, y))
            size = 0
            while queue:
                cx, cy = queue.popleft()
                size += 1
                for nx, ny in ((cx + 1, cy), (cx - 1, cy), (cx, cy + 1), (cx, cy - 1)):
                    if 0 <= nx < width and 0 <= ny < height and pixels[nx, ny] > 0 and (nx, ny) not in visited:
                        visited.add((nx, ny))
                        queue.append((nx, ny))
            component_sizes.append(size)

    transparent_visited: set[tuple[int, int]] = set()
    queue: deque[tuple[int, int]] = deque()
    for x in range(width):
        for y in (0, height - 1):
            if pixels[x, y] == 0 and (x, y) not in transparent_visited:
                transparent_visited.add((x, y))
                queue.append((x, y))
    for y in range(height):
        for x in (0, width - 1):
            if pixels[x, y] == 0 and (x, y) not in transparent_visited:
                transparent_visited.add((x, y))
                queue.append((x, y))

    while queue:
        cx, cy = queue.popleft()
        for nx, ny in ((cx + 1, cy), (cx - 1, cy), (cx, cy + 1), (cx, cy - 1)):
            if 0 <= nx < width and 0 <= ny < height and pixels[nx, ny] == 0 and (nx, ny) not in transparent_visited:
                transparent_visited.add((nx, ny))
                queue.append((nx, ny))

    holes = 0
    hole_pixels = 0
    for y in range(height):
        for x in range(width):
            if pixels[x, y] == 0 and (x, y) not in transparent_visited:
                holes += 1
                queue = deque([(x, y)])
                transparent_visited.add((x, y))
                while queue:
                    cx, cy = queue.popleft()
                    hole_pixels += 1
                    for nx, ny in ((cx + 1, cy), (cx - 1, cy), (cx, cy + 1), (cx, cy - 1)):
                        if 0 <= nx < width and 0 <= ny < height and pixels[nx, ny] == 0 and (nx, ny) not in transparent_visited:
                            transparent_visited.add((nx, ny))
                            queue.append((nx, ny))

    return {
        "components": len(component_sizes),
        "small_components": sum(1 for size in component_sizes if size <= 4),
        "holes": holes,
        "hole_pixels": hole_pixels,
    }


def summarize_animation(frame_metrics: list[dict[str, float]]) -> dict[str, float]:
    if not frame_metrics:
        return {
            "frame_count": 0,
            "center_drift_range": 0,
            "foot_drift_range": 0,
            "width_range": 0,
            "height_range": 0,
        }

    center_values = [frame["center_drift"] for frame in frame_metrics]
    foot_values = [frame["foot_drift"] for frame in frame_metrics]
    width_values = [frame["width"] for frame in frame_metrics]
    height_values = [frame["height"] for frame in frame_metrics]
    components = [frame.get("components", 1) for frame in frame_metrics]
    small_components = [frame.get("small_components", 0) for frame in frame_metrics]
    holes = [frame.get("holes", 0) for frame in frame_metrics]
    hole_pixels = [frame.get("hole_pixels", 0) for frame in frame_metrics]

    return {
        "frame_count": len(frame_metrics),
        "center_drift_range": max(center_values) - min(center_values),
        "foot_drift_range": max(foot_values) - min(foot_values),
        "width_range": max(width_values) - min(width_values),
        "height_range": max(height_values) - min(height_values),
        "max_abs_center_drift": max(abs(value) for value in center_values),
        "max_abs_foot_drift": max(abs(value) for value in foot_values),
        "max_width": max(width_values),
        "max_height": max(height_values),
        "max_components": max(components),
        "max_small_components": max(small_components),
        "max_holes": max(holes),
        "max_hole_pixels": max(hole_pixels),
    }


def diagnose_animation(summary: dict[str, float]) -> list[str]:
    findings: list[str] = []
    if summary["foot_drift_range"] > 2:
        findings.append(f"foot drift range is {summary['foot_drift_range']:.1f}px")
    if summary["center_drift_range"] > 4:
        findings.append(f"center drift range is {summary['center_drift_range']:.1f}px")
    if summary["height_range"] > 6:
        findings.append(f"height range is {summary['height_range']:.1f}px")
    if summary["max_width"] > 34:
        findings.append(f"max silhouette width is {summary['max_width']:.1f}px")
    if summary["max_components"] > 1:
        findings.append(f"frame connectivity splits into {summary['max_components']} components")
    if summary["max_holes"] > 0:
        findings.append(f"internal transparency holes detected (max {summary['max_holes']})")
    return findings


def file_sha1(path: Path) -> str:
    return hashlib.sha1(path.read_bytes()).hexdigest()


def strict_violations(animation_name: str, animation: dict, manifest_root: Path) -> list[str]:
    frames = animation["frames"]
    summary = animation["summary"]
    violations: list[str] = []

    if animation_name in STRICT_THRESHOLDS:
        thresholds = STRICT_THRESHOLDS[animation_name]
        for key, limit in thresholds.items():
            value = summary[key]
            if value > limit:
                label = key.replace("_", " ")
                violations.append(f"{label} {value:.1f} exceeds {limit:.1f}")

    if animation_name in ONE_SHOT_STATES:
        idle_frame = manifest_root / "idle" / "frame-01.png"
        state_frame = Path(frames[0]["source"])
        if idle_frame.exists() and state_frame.exists() and file_sha1(idle_frame) != file_sha1(state_frame):
            violations.append("frame-01 is not byte-identical to canonical idle seed")

    if animation_name == "death":
        for index, frame in enumerate(frames, start=1):
            if index <= 2 and abs(frame["foot_drift"]) > 1:
                violations.append(f"frame-{index:02d} foot drift {frame['foot_drift']:+.1f}px exceeds 1px")
            if frame["components"] > 1:
                violations.append(f"frame-{index:02d} splits into {frame['components']} components")
            if index <= 2 and frame["holes"] > 2:
                violations.append(f"frame-{index:02d} has {frame['holes']} internal holes")

    if animation_name == "shoot":
        for index, frame in enumerate(frames, start=1):
            if frame["width"] > 36:
                violations.append(f"frame-{index:02d} width {frame['width']:.1f}px exceeds 36px")
            if frame["components"] > 1:
                violations.append(f"frame-{index:02d} splits into {frame['components']} components")

    return violations


def build_contact_sheet(
    animation_name: str,
    frame_paths: list[Path],
    frame_metrics: list[dict[str, float]],
    frame_size: int,
    feet_anchor: int,
    out_path: Path,
    zoom: int = 10,
) -> None:
    thumb = frame_size * zoom
    gap = 32
    header = 92
    footer = 110
    width = len(frame_paths) * (thumb + gap) + gap
    height = header + thumb + footer
    sheet = Image.new("RGBA", (width, height), (14, 17, 24, 255))
    draw = ImageDraw.Draw(sheet)

    draw.text((20, 18), f"{animation_name.upper()} audit", fill=(232, 236, 242, 255))
    draw.text((20, 44), "orange = alpha bounds | cyan = frame center | red = bbox center | amber = feet anchor", fill=(180, 188, 198, 255))

    for index, frame_path in enumerate(frame_paths):
        frame = Image.open(frame_path).convert("RGBA")
        metrics = frame_metrics[index]
        base_x = gap + index * (thumb + gap)
        base_y = header

        draw.rectangle((base_x - 2, base_y - 2, base_x + thumb + 2, base_y + thumb + 2), outline=(86, 102, 128, 255), width=2)

        for line in range(frame_size + 1):
            x = base_x + line * zoom
            y = base_y + line * zoom
            draw.line((x, base_y, x, base_y + thumb), fill=(32, 38, 48, 255), width=1)
            draw.line((base_x, y, base_x + thumb, y), fill=(32, 38, 48, 255), width=1)

        frame_large = frame.resize((thumb, thumb), resample=Image.Resampling.NEAREST)
        sheet.alpha_composite(frame_large, (base_x, base_y))

        left = base_x + int(metrics["left"] * zoom)
        top = base_y + int(metrics["top"] * zoom)
        right = base_x + int(metrics["right"] * zoom)
        bottom = base_y + int(metrics["bottom"] * zoom)
        draw.rectangle((left, top, right, bottom), outline=(248, 188, 48, 255), width=2)

        center_line_x = base_x + frame_size * zoom / 2
        bbox_center_x = base_x + metrics["center_x"] * zoom
        feet_y = base_y + feet_anchor * zoom
        draw.line((center_line_x, base_y, center_line_x, base_y + thumb), fill=(76, 212, 255, 170), width=2)
        draw.line((bbox_center_x, base_y, bbox_center_x, base_y + thumb), fill=(255, 91, 91, 180), width=2)
        draw.line((base_x, feet_y, base_x + thumb, feet_y), fill=(255, 180, 0, 180), width=2)

        text_y = base_y + thumb + 10
        draw.text((base_x, text_y), f"frame {index + 1}", fill=(232, 236, 242, 255))
        draw.text((base_x, text_y + 22), f"bbox {int(metrics['width'])}x{int(metrics['height'])}", fill=(190, 198, 208, 255))
        draw.text((base_x, text_y + 44), f"center drift {metrics['center_drift']:+.1f}px", fill=(190, 198, 208, 255))
        draw.text((base_x, text_y + 66), f"foot drift {metrics['foot_drift']:+.1f}px", fill=(190, 198, 208, 255))

    out_path.parent.mkdir(parents=True, exist_ok=True)
    sheet.save(out_path)


def audit_manifest(manifest_path: Path, out_dir: Path) -> dict:
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    assets_root = manifest_path.parent
    frame_size = manifest["frame_size"]
    base_render = manifest.get("render", {})
    report = {
        "frame_size": frame_size,
        "manifest": manifest_path.as_posix(),
        "animations": {},
    }

    for animation_name in manifest["animation_order"]:
        animation = manifest["animations"][animation_name]
        animation_render = animation.get("render", {})
        feet_anchor = animation_render.get("feet_anchor", base_render.get("feet_anchor", frame_size))
        frame_paths = [assets_root / animation_name / frame_name for frame_name in animation["frames"]]
        frame_metrics = []

        for frame_path in frame_paths:
            image = Image.open(frame_path).convert("RGBA")
            bounds = alpha_bounds(image)
            if bounds is None:
                raise ValueError(f"{frame_path} has no visible pixels.")
            topology = alpha_topology(image, image.getchannel("A").getbbox())

            bounds["center_drift"] = bounds["center_x"] - frame_size / 2
            bounds["foot_drift"] = bounds["foot_y"] - feet_anchor
            bounds["source"] = frame_path.as_posix()
            bounds.update(topology)
            frame_metrics.append(bounds)

        summary = summarize_animation(frame_metrics)
        findings = diagnose_animation(summary)
        report["animations"][animation_name] = {
            "feet_anchor": feet_anchor,
            "frames": frame_metrics,
            "summary": summary,
            "findings": findings,
            "strict_violations": strict_violations(animation_name, {
                "frames": frame_metrics,
                "summary": summary,
            }, assets_root),
        }

        build_contact_sheet(
            animation_name,
            frame_paths,
            frame_metrics,
            frame_size,
            feet_anchor,
            out_dir / f"{animation_name}_audit.png",
        )

    return report


def write_report(report: dict, out_dir: Path) -> None:
    out_dir.mkdir(parents=True, exist_ok=True)
    (out_dir / "audit_report.json").write_text(json.dumps(report, indent=2), encoding="utf-8")

    lines = ["# Player Animation Audit", ""]
    for animation_name, animation in report["animations"].items():
        summary = animation["summary"]
        findings = animation["findings"]
        strict_violations_list = animation.get("strict_violations", [])
        lines.append(f"## {animation_name}")
        lines.append(
            f"- foot drift range: {summary['foot_drift_range']:.1f}px"
        )
        lines.append(
            f"- center drift range: {summary['center_drift_range']:.1f}px"
        )
        lines.append(
            f"- height range: {summary['height_range']:.1f}px"
        )
        lines.append(
            f"- width range: {summary['width_range']:.1f}px"
        )
        lines.append(
            f"- max components: {summary['max_components']}"
        )
        lines.append(
            f"- max holes: {summary['max_holes']}"
        )
        if findings:
            for finding in findings:
                lines.append(f"- finding: {finding}")
        else:
            lines.append("- finding: no threshold breaches")
        if strict_violations_list:
            for violation in strict_violations_list:
                lines.append(f"- strict: {violation}")
        else:
            lines.append("- strict: pass")
        lines.append("")

    (out_dir / "audit_report.md").write_text("\n".join(lines), encoding="utf-8")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Audit player animation bounds, drift, and silhouette consistency.")
    parser.add_argument(
        "--manifest",
        type=Path,
        default=Path("assets/player/player_manifest.json"),
        help="Source JSON manifest describing canonical player frames.",
    )
    parser.add_argument(
        "--out-dir",
        type=Path,
        default=Path("tmp/player_audit"),
        help="Directory for the audit report and 10x contact sheets.",
    )
    parser.add_argument(
        "--strict",
        action="store_true",
        help="Exit non-zero if any required animation breaches strict thresholds.",
    )
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    report = audit_manifest(args.manifest, args.out_dir)
    write_report(report, args.out_dir)
    strict_failures = []
    for animation_name, animation in report["animations"].items():
        for violation in animation.get("strict_violations", []):
            strict_failures.append(f"{animation_name}: {violation}")

    if args.strict and strict_failures:
        for failure in strict_failures:
            print(f"strict_fail {failure}")
        raise SystemExit(1)

    print(f"audit_ok {args.out_dir}")


if __name__ == "__main__":
    main()
