#!/usr/bin/env python3

import argparse
import subprocess
import sys
from pathlib import Path


RUNTIME_FILES = [
    "main.lua",
    "autoplay.lua",
    "player.lua",
    "level.lua",
    "ui.lua",
    "projectiles.lua",
    "proving_ground.lua",
    "campaign.lua",
    "background.lua",
    "sfx.lua",
    "effects.lua",
    "enemies.lua",
    "enemy.lua",
    "harpy.lua",
    "encounter_controller.lua",
    "scene_contract.lua",
    "runtime_services.lua",
]


def run_step(label: str, command: list[str], cwd: Path) -> int:
    print(f"\n== {label} ==")
    print(" ".join(command))
    result = subprocess.run(command, cwd=cwd, check=False)
    return result.returncode


def main() -> int:
    parser = argparse.ArgumentParser(description="Run the fast Dante runtime validation loop.")
    parser.add_argument("--skip-autoplay", action="store_true", help="Skip the autoplay proving-ground pass.")
    parser.add_argument("--audio", action="store_true", help="Allow autoplay audio output.")
    parser.add_argument("--no-screenshots", action="store_true", help="Disable autoplay screenshot capture.")
    parser.add_argument("--strict-art", action="store_true", help="Fail if the strict player animation audit finds violations.")
    parser.add_argument("--no-campaign-screenshots", action="store_true", help="Disable campaign smoke screenshot capture.")
    parser.set_defaults(background_qa=True)
    parser.add_argument("--background-qa", dest="background_qa", action="store_true", help="Run QA windows in background mode.")
    parser.add_argument("--foreground-qa", dest="background_qa", action="store_false", help="Run QA windows in the foreground.")
    args = parser.parse_args()

    repo = Path(__file__).resolve().parents[1]

    validation_command = ["python3", "scripts/validate_slice_content.py"]
    if run_step("slice-content", validation_command, repo) != 0:
        return 1

    audit_command = ["python3", "scripts/audit_player_animation.py"]
    if args.strict_art:
        audit_command.append("--strict")
    if run_step("player-audit", audit_command, repo) != 0:
        return 1

    luacheck_command = ["luacheck", *RUNTIME_FILES]
    if run_step("luacheck", luacheck_command, repo) != 0:
        return 1

    lua_paths = ", ".join(f'"{path}"' for path in RUNTIME_FILES)
    load_script = (
        f'for _, path in ipairs({{{lua_paths}}}) do '
        "local chunk, err = loadfile(path); "
        "assert(chunk, err); "
        "end"
    )
    lua_command = ["lua", "-e", load_script]
    if run_step("lua-load", lua_command, repo) != 0:
        return 1

    if not args.skip_autoplay:
        for variant in ("a", "b", "c"):
            autoplay_command = [
                "python3",
                "scripts/run_autoplay_check.py",
                "--variant",
                variant,
                "--report",
                f"tmp/autoplay-report-{variant}.json",
                "--strict",
            ]
            if args.audio:
                autoplay_command.append("--audio")
            if args.no_screenshots:
                autoplay_command.append("--no-screenshots")
            else:
                autoplay_command.extend(["--shots", f"tmp/autoplay-shots-{variant}"])
            if args.background_qa:
                autoplay_command.append("--background")
            if run_step(f"autoplay-{variant}", autoplay_command, repo) != 0:
                return 1

    campaign_smoke_command = ["python3", "scripts/run_campaign_smoke.py", "--profile", "vertical-slice"]
    if args.no_campaign_screenshots:
        campaign_smoke_command.append("--no-screenshots")
    if args.background_qa:
        campaign_smoke_command.append("--background")
    if run_step("campaign-smoke", campaign_smoke_command, repo) != 0:
        return 1

    circles_smoke_command = ["python3", "scripts/run_circles_smoke.py"]
    if args.no_campaign_screenshots:
        circles_smoke_command.append("--no-screenshots")
    if args.background_qa:
        circles_smoke_command.append("--background")
    if run_step("circles-smoke", circles_smoke_command, repo) != 0:
        return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
