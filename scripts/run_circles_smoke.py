#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
import os
import shutil
import subprocess
from pathlib import Path


DEFAULT_CIRCLES = list(range(0, 9))
CAPTURE_POINTS = [("start", 0.35), ("settled", 2.2)]


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
    parser = argparse.ArgumentParser(description="Boot each circles room for a short smoke test.")
    parser.add_argument("--circles", nargs="*", type=int, default=DEFAULT_CIRCLES)
    parser.add_argument("--seconds", type=float, default=2.5)
    parser.add_argument("--no-screenshots", action="store_true", help="Disable circle screenshot capture.")
    parser.add_argument("--shots-dir", default="tmp/circles-smoke-shots")
    parser.add_argument("--report", default="tmp/circles-smoke-report.json")
    parser.set_defaults(background=True)
    parser.add_argument("--background", dest="background", action="store_true", help="Run smoke checks in background window mode.")
    parser.add_argument("--foreground", dest="background", action="store_false", help="Run smoke checks in a normal foreground window.")
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
    for circle in args.circles:
        circle_result = {"circle": circle, "shots": [], "pass": True, "fail_reasons": []}
        if not args.no_screenshots:
            for label, delay in CAPTURE_POINTS:
                screenshot_path = shots_dir / f"circle{circle:02d}-{label}.png"
                command = [
                    str(love_bin),
                    ".",
                    "--circles",
                    f"--circle={circle}",
                    f"--qa-shot={screenshot_path}",
                    f"--qa-shot-delay={max(0.05, delay):.2f}",
                ]
                if args.background:
                    command.append("--qa-background")
                returncode = run_capture(command, repo, max(args.seconds + 2, delay + 2))
                if returncode != 0:
                    print(f"circle{circle}_{label}_capture_failed ({returncode})")
                    return returncode
                circle_result["shots"].append(screenshot_path.as_posix())
                if not screenshot_path.exists():
                    circle_result["pass"] = False
                    circle_result["fail_reasons"].append(f"missing_capture:{label}")

        command = [str(love_bin), ".", "--circles", f"--circle={circle}"]
        if args.background:
            command.append("--qa-background")
        returncode = run_capture(command, repo, args.seconds)
        if returncode not in (0, -15):
            print(f"circle{circle}_smoke_failed ({returncode})")
            return returncode or 1
        print(f"circle{circle}_smoke_ok")
        results.append(circle_result)

    report_path.parent.mkdir(parents=True, exist_ok=True)
    overall_pass = all(circle["pass"] for circle in results)
    report_path.write_text(json.dumps({"overall_pass": overall_pass, "circles": results}, indent=2), encoding="utf-8")

    if not overall_pass:
        print("circles_smoke_validation_failed")
        return 2

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
