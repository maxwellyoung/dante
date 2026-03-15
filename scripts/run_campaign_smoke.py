#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
import os
import subprocess
from pathlib import Path
import shutil


DEFAULT_ROOMS = list(range(1, 12))
VERTICAL_SLICE_PROFILE = {
    1: {"captures": [("start", 0.35), ("post_intro", 1.8), ("settled", 3.0)]},
    2: {"captures": [("start", 0.35), ("post_intro", 1.4), ("settled", 2.4)]},
    3: {"captures": [("start", 0.35), ("post_intro", 1.3), ("settled", 2.6)]},
    4: {"captures": [("start", 0.35), ("post_intro", 1.4), ("settled", 2.6)]},
    5: {"captures": [("start", 0.35), ("post_intro", 1.9), ("settled", 3.6)]},
    6: {"captures": [("start", 0.35), ("post_intro", 1.8), ("settled", 3.6)]},
    7: {"captures": [("start", 0.35), ("post_intro", 1.5), ("settled", 2.8)]},
    8: {"captures": [("start", 0.35), ("post_intro", 1.7), ("settled", 3.0)]},
    9: {"captures": [("start", 0.35), ("post_intro", 1.7), ("settled", 3.0)]},
    10: {"captures": [("start", 0.35), ("post_intro", 1.9), ("settled", 3.8)]},
    11: {"captures": [("start", 0.35), ("post_intro", 2.0), ("settled", 4.0)]},
}

def run_capture(command: list[str], cwd: Path, seconds: float) -> int:
    env = os.environ.copy()
    if "--qa-background" in command:
        env["DANTE_QA_NO_ACTIVATE"] = "1"
    process = subprocess.Popen(command, cwd=cwd, env=env)
    try:
        process.wait(timeout=seconds)
    except subprocess.TimeoutExpired:
        process.terminate()
        try:
            process.wait(timeout=5)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait(timeout=5)
    return process.returncode or 0


def main() -> int:
    parser = argparse.ArgumentParser(description="Boot key campaign rooms for a short smoke test.")
    parser.add_argument("--rooms", nargs="*", type=int, default=DEFAULT_ROOMS)
    parser.add_argument("--seconds", type=float, default=2.5)
    parser.add_argument("--no-screenshots", action="store_true", help="Disable room screenshot capture.")
    parser.add_argument("--shots-dir", default="tmp/campaign-smoke-shots")
    parser.add_argument("--report", default="tmp/campaign-smoke-report.json")
    parser.set_defaults(background=True)
    parser.add_argument("--background", dest="background", action="store_true", help="Run smoke checks in background window mode.")
    parser.add_argument("--foreground", dest="background", action="store_false", help="Run smoke checks in a normal foreground window.")
    parser.add_argument("--profile", default="vertical-slice", help="Validation profile to apply.")
    args = parser.parse_args()

    repo = Path(__file__).resolve().parents[1]
    love_bin = Path("/Applications/love.app/Contents/MacOS/love")
    if not love_bin.exists():
        print(f"Missing LÖVE binary at {love_bin}")
        return 1

    shots_dir = repo / args.shots_dir
    report_path = repo / args.report
    if shots_dir.exists() and not args.no_screenshots:
        shutil.rmtree(shots_dir)
    if not args.no_screenshots:
        shots_dir.mkdir(parents=True, exist_ok=True)

    results = []
    profile = VERTICAL_SLICE_PROFILE if args.profile == "vertical-slice" else {}

    for room in args.rooms:
        room_result = {"room": room, "shots": [], "pass": True, "fail_reasons": []}

        if not args.no_screenshots:
            capture_points = profile.get(room, {}).get("captures")
            if not capture_points:
                capture_points = [("start", 0.35), ("settled", max(args.seconds - 0.3, 1.8))]
            for label, delay in capture_points:
                screenshot_path = shots_dir / f"room{room:02d}-{label}.png"
                command = [
                    str(love_bin),
                    ".",
                    "--campaign",
                    f"--room={room}",
                    f"--qa-shot={screenshot_path}",
                    f"--qa-shot-delay={max(0.05, delay):.2f}",
                ]
                if args.background:
                    command.append("--qa-background")
                returncode = run_capture(command, repo, max(args.seconds + 2, delay + 2))
                if returncode != 0:
                    print(f"campaign_room{room}_{label}_capture_failed ({returncode})")
                    return returncode
                room_result["shots"].append(screenshot_path.as_posix())
                if not screenshot_path.exists():
                    room_result["pass"] = False
                    room_result["fail_reasons"].append(f"missing_capture:{label}")

        command = [str(love_bin), ".", "--campaign", f"--room={room}"]
        if args.background:
            command.append("--qa-background")
        returncode = run_capture(command, repo, args.seconds)
        if returncode not in (0, -15):
            print(f"campaign_room{room}_smoke_failed ({returncode})")
            return returncode or 1
        print(f"campaign_room{room}_smoke_ok")
        results.append(room_result)

    report_path.parent.mkdir(parents=True, exist_ok=True)
    overall_pass = all(room["pass"] for room in results)
    report_path.write_text(json.dumps({"profile": args.profile, "overall_pass": overall_pass, "rooms": results}, indent=2), encoding="utf-8")

    if not overall_pass:
        print("campaign_smoke_validation_failed")
        return 2

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
