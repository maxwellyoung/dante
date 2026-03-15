#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
import os
import shutil
import subprocess
from pathlib import Path

from make_reference_canvas import build_reference_canvas


IMAGE_GEN = Path("/Users/maxwellyoung/.codex/skills/imagegen/scripts/image_gen.py")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Generate multiple candidate strips for a player state and rank them by strict audit results."
    )
    parser.add_argument("--state", required=True, help="Animation state, for example jump or shoot.")
    parser.add_argument("--prompt-file", type=Path, required=True, help="Prompt text file for the image edit.")
    parser.add_argument("--slot-count", type=int, required=True, help="Number of slots in the strip.")
    parser.add_argument("--slot-size", type=int, default=256, help="Size of each slot in the reference canvas.")
    parser.add_argument("--variants", type=int, default=3, help="How many candidate strips to generate.")
    parser.add_argument(
        "--seed",
        type=Path,
        default=Path("assets/player/idle/frame-01.png"),
        help="Canonical idle seed frame.",
    )
    parser.add_argument(
        "--work-dir",
        type=Path,
        default=Path("tmp/player_candidates"),
        help="Directory for references, candidate strips, and reports.",
    )
    return parser.parse_args()


def run(command: list[str], *, env: dict[str, str]) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        command,
        text=True,
        capture_output=True,
        check=False,
        env=env,
    )


def load_audit_summary(audit_json_path: Path, state: str) -> dict:
    report = json.loads(audit_json_path.read_text(encoding="utf-8"))
    animation = report["animations"][state]
    return {
        "findings": animation["findings"],
        "strict_violations": animation["strict_violations"],
        "summary": animation["summary"],
    }


def main() -> int:
    args = parse_args()
    if not IMAGE_GEN.exists():
        raise SystemExit(f"Missing image generation CLI: {IMAGE_GEN}")
    if not os.getenv("OPENAI_API_KEY"):
        raise SystemExit("OPENAI_API_KEY must be set in the environment.")

    uv = shutil.which("uv")
    if uv is None:
        raise SystemExit("uv is required for candidate generation.")

    state_dir = args.work_dir / args.state
    state_dir.mkdir(parents=True, exist_ok=True)
    reference_path = state_dir / "reference.png"
    build_reference_canvas(
        args.seed,
        reference_path,
        slot_count=args.slot_count,
        slot_size=args.slot_size,
        slot_index=0,
    )

    env = os.environ.copy()
    results = []

    for index in range(1, args.variants + 1):
        strip_path = state_dir / f"{args.state}_candidate_{index:02d}.png"
        candidate_work_dir = state_dir / f"candidate_{index:02d}"
        if candidate_work_dir.exists():
            shutil.rmtree(candidate_work_dir)

        image_command = [
            uv,
            "run",
            "--with",
            "openai",
            "python",
            str(IMAGE_GEN),
            "edit",
            "--image",
            str(reference_path),
            "--prompt-file",
            str(args.prompt_file),
            "--quality",
            "high",
            "--background",
            "transparent",
            "--output-format",
            "png",
            "--input-fidelity",
            "high",
            "--no-augment",
            "--out",
            str(strip_path),
            "--force",
        ]
        image_result = run(image_command, env=env)
        if image_result.returncode != 0:
            results.append(
                {
                    "variant": index,
                    "strip": strip_path.as_posix(),
                    "status": "generation_failed",
                    "stdout": image_result.stdout,
                    "stderr": image_result.stderr,
                }
            )
            continue

        promote_command = [
            "python3",
            "scripts/promote_player_state.py",
            "--state",
            args.state,
            "--strip",
            str(strip_path),
            "--slot-count",
            str(args.slot_count),
            "--slot-size",
            str(args.slot_size),
            "--work-dir",
            str(candidate_work_dir),
        ]
        promote_result = run(promote_command, env=env)
        audit_json_path = candidate_work_dir / args.state / "audit" / "audit_report.json"
        summary = load_audit_summary(audit_json_path, args.state) if audit_json_path.exists() else None
        results.append(
            {
                "variant": index,
                "strip": strip_path.as_posix(),
                "status": "ok" if promote_result.returncode == 0 else "audit_failed",
                "stdout": promote_result.stdout,
                "stderr": promote_result.stderr,
                "audit": summary,
            }
        )

    sortable = []
    for item in results:
        audit = item.get("audit") or {}
        summary = audit.get("summary") or {}
        violations = audit.get("strict_violations") or []
        sortable.append(
            (
                len(violations),
                summary.get("max_hole_pixels", 9999),
                summary.get("max_components", 9999),
                summary.get("height_range", 9999),
                summary.get("max_width", 9999),
                item,
            )
        )
    sortable.sort(key=lambda entry: entry[:5])
    ranked = [entry[-1] for entry in sortable]

    report = {
        "state": args.state,
        "prompt_file": args.prompt_file.as_posix(),
        "reference": reference_path.as_posix(),
        "results": ranked,
    }
    report_path = state_dir / "candidate_report.json"
    report_path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")

    print(f"candidate_report={report_path}")
    if ranked:
        best = ranked[0]
        audit = best.get("audit") or {}
        print(f"best_variant={best['variant']}")
        print(f"best_strip={best['strip']}")
        for violation in (audit.get("strict_violations") or []):
            print(f"strict {violation}")
        if not (audit.get("strict_violations") or []):
            print("strict pass")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
