#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
import shutil
from pathlib import Path

from audit_player_animation import audit_manifest, write_report
from build_player_atlas import build_player_atlas
from cleanup_player_frames import cleanup_directory
from normalize_strip import normalize_strip


def load_manifest(path: Path) -> dict:
    return json.loads(path.read_text(encoding="utf-8"))


def animation_config(manifest: dict, state: str) -> dict:
    animation = manifest["animations"].get(state)
    if animation is None:
        raise ValueError(f"Unknown animation state: {state}")
    return animation


def copy_promoted_frames(source_dir: Path, target_dir: Path, *, frame_count: int) -> None:
    target_dir.mkdir(parents=True, exist_ok=True)
    for frame_index in range(1, frame_count + 1):
        frame_name = f"frame-{frame_index:02d}.png"
        shutil.copyfile(source_dir / frame_name, target_dir / frame_name)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Normalize, cleanup, audit, and optionally promote a player animation strip."
    )
    parser.add_argument("--state", required=True, help="Animation state to update, for example shoot.")
    parser.add_argument("--strip", type=Path, required=True, help="Raw GPT strip PNG to normalize.")
    parser.add_argument(
        "--source-manifest",
        type=Path,
        default=Path("assets/player/player_manifest.json"),
        help="Canonical player source manifest JSON.",
    )
    parser.add_argument(
        "--seed",
        type=Path,
        default=Path("assets/player/idle/frame-01.png"),
        help="Canonical idle seed frame used for alignment and lock-frame-01 states.",
    )
    parser.add_argument("--slot-count", type=int, required=True, help="Number of slots in the raw strip.")
    parser.add_argument("--slot-size", type=int, default=256, help="Size of each slot in the raw strip.")
    parser.add_argument("--frame-size", type=int, default=64, help="Canonical output frame size.")
    parser.add_argument("--padding", type=int, default=6, help="Transparent padding budget in the normalized frame.")
    parser.add_argument("--foot-anchor", type=int, default=None, help="Override target visible foot anchor.")
    parser.add_argument("--fill-holes", type=int, default=2, help="Fill interior holes up to this many pixels.")
    parser.add_argument(
        "--remove-components",
        type=int,
        default=4,
        help="Remove detached components up to this many pixels.",
    )
    parser.add_argument(
        "--work-dir",
        type=Path,
        default=Path("tmp/player_promotions"),
        help="Directory for normalized candidates and audit output.",
    )
    parser.add_argument("--promote", action="store_true", help="Copy cleaned frames into assets/player/<state> and rebuild atlas.")
    parser.add_argument("--strict", action="store_true", help="Fail if the resulting state still has strict audit violations.")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    manifest = load_manifest(args.source_manifest)
    animation = animation_config(manifest, args.state)
    render = animation.get("render", {})
    foot_anchor = args.foot_anchor
    if foot_anchor is None:
        foot_anchor = render.get("feet_anchor", manifest.get("render", {}).get("feet_anchor", args.frame_size))

    state_work_dir = args.work_dir / args.state
    normalized_dir = state_work_dir / "normalized"
    audit_dir = state_work_dir / "audit"
    if normalized_dir.exists():
        shutil.rmtree(normalized_dir)
    normalized_dir.mkdir(parents=True, exist_ok=True)

    lock_frame_01 = animation.get("lock_frame_01", False)
    normalize_strip(
        args.strip,
        normalized_dir,
        slot_count=args.slot_count,
        slot_size=args.slot_size,
        frame_size=args.frame_size,
        padding=args.padding,
        foot_anchor=foot_anchor,
        seed_path=args.seed,
        lock_frame_01=lock_frame_01,
    )
    cleanup_directory(
        normalized_dir,
        foot_anchor=foot_anchor,
        max_hole_pixels=args.fill_holes,
        max_component_pixels=args.remove_components,
        skip_frame_01=lock_frame_01,
        center_x=args.frame_size // 2,
    )

    candidate_manifest = json.loads(json.dumps(manifest))
    candidate_manifest["animations"][args.state]["frames"] = animation["frames"]
    candidate_assets_root = state_work_dir / "candidate_assets"
    if candidate_assets_root.exists():
        shutil.rmtree(candidate_assets_root)
    shutil.copytree(args.source_manifest.parent, candidate_assets_root)
    target_candidate_dir = candidate_assets_root / args.state
    if target_candidate_dir.exists():
        shutil.rmtree(target_candidate_dir)
    copy_promoted_frames(normalized_dir, target_candidate_dir, frame_count=len(animation["frames"]))

    candidate_manifest_path = candidate_assets_root / args.source_manifest.name
    candidate_manifest_path.write_text(json.dumps(candidate_manifest, indent=2) + "\n", encoding="utf-8")

    report = audit_manifest(candidate_manifest_path, audit_dir)
    write_report(report, audit_dir)
    state_report = report["animations"][args.state]
    violations = state_report.get("strict_violations", [])

    print(f"state={args.state}")
    print(f"normalized={normalized_dir}")
    print(f"audit={audit_dir}")
    for finding in state_report["findings"]:
        print(f"finding {finding}")
    if violations:
        for violation in violations:
            print(f"strict {violation}")
    else:
        print("strict pass")

    if args.promote:
        destination = args.source_manifest.parent / args.state
        copy_promoted_frames(normalized_dir, destination, frame_count=len(animation["frames"]))
        build_player_atlas(
            args.source_manifest,
            args.source_manifest.parent / "player_atlas.png",
            args.source_manifest.parent / "player_manifest.lua",
        )
        print(f"promoted {destination}")

    if args.strict and violations:
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
