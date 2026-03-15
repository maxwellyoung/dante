#!/usr/bin/env python3

import argparse
import json
import os
import shutil
import subprocess
import sys
import time
from pathlib import Path


def build_command(repo: Path, args: argparse.Namespace) -> list[str]:
    love_bin = Path("/Applications/love.app/Contents/MacOS/love")
    if not love_bin.exists():
        raise SystemExit(f"Missing LÖVE binary at {love_bin}")

    command = [
        str(love_bin),
        ".",
        "--autoplay",
        f"--autoplay-report={args.report}",
        f"--autoplay-variant={args.variant}",
    ]
    if args.background:
        command.append("--qa-background")
    if not args.audio:
        command.append("--autoplay-no-screenshots" if args.no_screenshots else f"--autoplay-shots={args.shots}")
    else:
        command.append("--autoplay-audio")
        if args.no_screenshots:
            command.append("--autoplay-no-screenshots")
        else:
            command.append(f"--autoplay-shots={args.shots}")
    return command


def summarize(report: dict) -> str:
    totals = report.get("totals", {})
    shots_fired = totals.get("shots_fired", 0)
    shots_hit = totals.get("shots_hit", 0)
    accuracy = 0.0 if shots_fired == 0 else (shots_hit / shots_fired) * 100
    lines = [
        f"total_time={report.get('total_time', 0):.2f}s",
        f"shots={shots_hit}/{shots_fired} ({accuracy:.1f}%)",
        f"grapples={totals.get('grapple_latches', 0)}/{totals.get('grapples_fired', 0)} latched",
        f"banks={totals.get('shot_ricochets', 0)} bank_kills={totals.get('shot_bank_kills', 0)}",
        f"rolls={totals.get('rolls', 0)} crouch_time={totals.get('crouch_time', 0):.2f}s",
        f"deaths={totals.get('deaths', 0)}",
    ]

    for room in report.get("rooms", []):
        room_shots = room.get("shots_fired", 0)
        room_hits = room.get("shots_hit", 0)
        room_accuracy = 0.0 if room_shots == 0 else (room_hits / room_shots) * 100
        lines.append(
            f"{room.get('room')}: {room.get('result')} | "
            f"time={room.get('duration', 0):.2f}s | "
            f"shots={room_hits}/{room_shots} ({room_accuracy:.1f}%) | "
            f"banks={room.get('shot_ricochets', 0)} | "
            f"bank_kills={room.get('shot_bank_kills', 0)} | "
            f"grapple_latches={room.get('grapple_latches', 0)} | "
            f"rolls={room.get('rolls', 0)} | "
            f"crouch_time={room.get('crouch_time', 0):.2f}s | "
            f"pass={room.get('pass', True)}"
        )
        if room.get("fail_reasons"):
            lines.append(f"  fail_reasons={', '.join(room['fail_reasons'])}")
    return "\n".join(lines)


def screenshot_count(path: Path) -> int:
    if not path.exists():
        return 0
    return sum(1 for item in path.iterdir() if item.is_file() and item.suffix.lower() == ".png")


def stop_process(process: subprocess.Popen) -> int:
    if process.poll() is not None:
        return process.returncode or 0

    process.terminate()
    try:
        return process.wait(timeout=5)
    except subprocess.TimeoutExpired:
        process.kill()
        return process.wait(timeout=5)


def run_autoplay_process(
    command: list[str],
    repo: Path,
    report_path: Path,
    shots_path: Path,
    capture_screenshots: bool,
) -> int:
    env = os.environ.copy()
    if "--qa-background" in command:
        env["DANTE_QA_NO_ACTIVATE"] = "1"
    process = subprocess.Popen(command, cwd=repo, env=env)
    deadline = time.monotonic() + 30
    stable_count = -1
    stable_ticks = 0

    while time.monotonic() < deadline:
        returncode = process.poll()
        if returncode is not None:
            return returncode

        report_ready = report_path.exists()
        shot_total = screenshot_count(shots_path) if capture_screenshots else 0

        if report_ready:
            if not capture_screenshots:
                return stop_process(process)

            if shot_total == stable_count and shot_total > 0:
                stable_ticks += 1
            else:
                stable_count = shot_total
                stable_ticks = 0

            if stable_ticks >= 2:
                return stop_process(process)

        time.sleep(1)

    print("Autoplay timed out before outputs stabilized.", file=sys.stderr)
    return stop_process(process)


def main() -> int:
    parser = argparse.ArgumentParser(description="Run muted autoplay QA for Infernal Ascent.")
    parser.add_argument("--report", default="tmp/autoplay-report.json")
    parser.add_argument("--shots", default="tmp/autoplay-shots")
    parser.add_argument("--audio", action="store_true", help="Allow autoplay audio output.")
    parser.add_argument("--no-screenshots", action="store_true", help="Disable screenshot capture.")
    parser.add_argument("--keep-old", action="store_true", help="Do not clear previous outputs first.")
    parser.set_defaults(background=True)
    parser.add_argument("--background", dest="background", action="store_true", help="Run QA in background window mode.")
    parser.add_argument("--foreground", dest="background", action="store_false", help="Run QA in a normal foreground window.")
    parser.add_argument("--variant", default="a", choices=("a", "b", "c"), help="Autoplay route variant.")
    parser.add_argument("--strict", action="store_true", help="Fail when report expectations are not met.")
    parser.add_argument("--profile", default="proving-ground", help="Expected evaluation profile.")
    args = parser.parse_args()

    repo = Path(__file__).resolve().parents[1]
    report_path = repo / args.report
    shots_path = repo / args.shots

    if not args.keep_old:
        report_path.unlink(missing_ok=True)
        if shots_path.exists():
            shutil.rmtree(shots_path)

    command = build_command(repo, args)
    returncode = run_autoplay_process(command, repo, report_path, shots_path, not args.no_screenshots)
    if returncode != 0:
        return returncode

    if not report_path.exists():
        print("Autoplay finished without writing a report.", file=sys.stderr)
        return 1

    report = json.loads(report_path.read_text())
    if report.get("variant") != args.variant:
        print(f"Autoplay wrote variant={report.get('variant')} but expected {args.variant}.", file=sys.stderr)
        return 1
    if report.get("profile") != args.profile:
        print(f"Autoplay wrote profile={report.get('profile')} but expected {args.profile}.", file=sys.stderr)
        return 1
    print(summarize(report))
    if not args.no_screenshots:
        print(f"screenshots={shots_path} ({screenshot_count(shots_path)} png)")
    print(f"report={report_path}")
    if args.strict and not report.get("overall_pass", False):
        print("Autoplay report failed strict expectations.", file=sys.stderr)
        return 2
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
