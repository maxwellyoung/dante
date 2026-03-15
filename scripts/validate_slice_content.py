#!/usr/bin/env python3

from __future__ import annotations

import json
import re
from pathlib import Path


REQUIRED_PLAYER_STATES = ["idle", "run", "jump", "fall", "hurt", "shoot", "death"]
KEY_CAMPAIGN_ROOMS = [
    "limbo_01",
    "limbo_02",
    "limbo_03",
    "limbo_04",
    "limbo_05",
    "circle1_01",
    "circle1_02",
    "circle1_03",
    "circle1_04",
    "circle1_05",
]
KEY_PROVING_GROUND_ROOMS = [
    "prove_traversal",
    "prove_combat",
    "prove_ricochet",
    "prove_grapple",
    "prove_dash",
    "prove_pressure",
    "prove_posture",
]
COMPACT_PROMPTS = ["fall_compact_edit_prompt.txt", "jump_compact_edit_prompt.txt", "run_compact_edit_prompt.txt", "shoot_compact_edit_prompt.txt", "death_compact_edit_prompt.txt"]


def load_manifest(path: Path) -> dict:
    return json.loads(path.read_text(encoding="utf-8"))


def validate_player_assets(manifest: dict, manifest_path: Path, require_final_art: bool = False) -> list[str]:
    errors: list[str] = []
    assets_root = manifest_path.parent
    animations = manifest.get("animations", {})

    missing = [state for state in REQUIRED_PLAYER_STATES if state not in animations]
    if missing:
        errors.append(f"Missing required player states: {', '.join(missing)}")

    for state in REQUIRED_PLAYER_STATES:
        animation = animations.get(state)
        if not animation:
            continue
        if require_final_art and animation.get("placeholder", manifest.get("render", {}).get("placeholder", False)):
            errors.append(f"Player state '{state}' is still marked placeholder.")
        frames = animation.get("frames", [])
        if not frames:
            errors.append(f"Player state '{state}' has no frames.")
            continue
        for index, frame_name in enumerate(frames, start=1):
            expected = f"frame-{index:02d}.png"
            if frame_name != expected:
                errors.append(
                    f"Player state '{state}' frame order is not contiguous: expected {expected}, got {frame_name}."
                )
            frame_path = assets_root / state / frame_name
            if not frame_path.exists():
                errors.append(f"Missing player frame: {frame_path.as_posix()}")

    return errors


def validate_campaign_data(campaign_path: Path) -> list[str]:
    errors: list[str] = []
    text = campaign_path.read_text(encoding="utf-8")
    room_matches = list(re.finditer(r'^\s{12}id = "([^"]+)"', text, flags=re.MULTILINE))

    sections: dict[str, str] = {}
    for index, match in enumerate(room_matches):
        room_id = match.group(1)
        start = match.start()
        end = room_matches[index + 1].start() if index + 1 < len(room_matches) else len(text)
        sections[room_id] = text[start:end]

    for room_id in KEY_CAMPAIGN_ROOMS:
        if room_id not in sections:
            errors.append(f"Campaign room '{room_id}' missing from campaign.lua.")

    for room_id in KEY_CAMPAIGN_ROOMS:
        room_text = sections.get(room_id)
        if room_text is None:
            continue
        if 'room_type = "' not in room_text:
            errors.append(f"Campaign room '{room_id}' is missing room_type.")
        if "qa_expectations = {" not in room_text:
            errors.append(f"Campaign room '{room_id}' is missing qa_expectations.")
        if 'dramatic_question = "' not in room_text:
            errors.append(f"Campaign room '{room_id}' is missing dramatic_question.")

    for room_id in ("limbo_05", "circle1_05"):
        room_text = sections.get(room_id)
        if room_text is None:
            continue
        if "encounter_config = {" not in room_text:
            errors.append(f"Campaign gate room '{room_id}' is missing encounter_config.")
        if (
            "trigger_x" not in room_text
            or "bounds = {" not in room_text
            or "waves = {" not in room_text
            or 'intro_title = "' not in room_text
            or 'intro_subtitle = "' not in room_text
            or 'outro_subtitle = "' not in room_text
        ):
            errors.append(f"Campaign gate room '{room_id}' encounter_config is incomplete.")

    room_text = sections.get("circle1_01")
    if room_text is not None:
        if "beat_config = {" not in room_text:
            errors.append("Campaign room 'circle1_01' is missing beat_config.")
        if "wind_areas = {" not in room_text:
            errors.append("Campaign room 'circle1_01' is missing wind_areas.")
        if "abilities = {" not in room_text or "grapple = false" not in room_text:
            errors.append("Campaign room 'circle1_01' must explicitly disable grapple.")
        for required in ('title = "', 'subtitle = "', "trigger_x", "duration"):
            if required not in room_text:
                errors.append(f"Campaign room 'circle1_01' beat_config is missing {required}.")

    room_text = sections.get("circle1_02")
    if room_text is not None:
        if 'objective_text = "' not in room_text:
            errors.append("Campaign room 'circle1_02' is missing objective_text.")
        if "npc_guidance = {" not in room_text:
            errors.append("Campaign room 'circle1_02' is missing npc_guidance.")
        if "roll = true" not in room_text or "crouch = true" not in room_text:
            errors.append("Campaign room 'circle1_02' must explicitly preserve crouch and roll.")

    for room_id in ("circle1_03", "circle1_04"):
        room_text = sections.get(room_id)
        if room_text is None:
            continue
        if 'objective_text = "' not in room_text:
            errors.append(f"Campaign room '{room_id}' is missing objective_text.")
        if "npc_guidance = {" not in room_text:
            errors.append(f"Campaign room '{room_id}' is missing npc_guidance.")
        if "qa_expectations = {" not in room_text:
            errors.append(f"Campaign room '{room_id}' is missing qa_expectations.")

    return errors


def validate_proving_ground_data(path: Path) -> list[str]:
    errors: list[str] = []
    text = path.read_text(encoding="utf-8")
    room_matches = list(re.finditer(r'^\s{12}id = "([^"]+)"', text, flags=re.MULTILINE))
    sections: dict[str, str] = {}
    for index, match in enumerate(room_matches):
        room_id = match.group(1)
        start = match.start()
        end = room_matches[index + 1].start() if index + 1 < len(room_matches) else len(text)
        sections[room_id] = text[start:end]

    for room_id in KEY_PROVING_GROUND_ROOMS:
        room_text = sections.get(room_id)
        if room_text is None:
            errors.append(f"Proving-ground room '{room_id}' missing from proving_ground.lua.")
            continue
        if "qa_expectations = {" not in room_text:
            errors.append(f"Proving-ground room '{room_id}' is missing qa_expectations.")
        if 'recovery_hint = "' not in room_text:
            errors.append(f"Proving-ground room '{room_id}' is missing recovery_hint.")

    return errors


def validate_player_prompts(prompts_dir: Path) -> list[str]:
    errors: list[str] = []
    for state in REQUIRED_PLAYER_STATES:
        prompt_path = prompts_dir / f"{state}_edit_prompt.txt"
        if not prompt_path.exists():
            errors.append(f"Missing sprite prompt: {prompt_path.as_posix()}")
    for prompt_name in COMPACT_PROMPTS:
        prompt_path = prompts_dir / prompt_name
        if not prompt_path.exists():
            errors.append(f"Missing compact QA prompt: {prompt_path.as_posix()}")
    return errors


def main() -> int:
    import argparse

    parser = argparse.ArgumentParser(description="Validate slice content metadata and player assets.")
    parser.add_argument(
        "--require-final-art",
        action="store_true",
        help="Fail if any required player state is still marked placeholder.",
    )
    args = parser.parse_args()

    repo = Path(__file__).resolve().parents[1]
    player_manifest_path = repo / "assets/player/player_manifest.json"
    prompts_dir = repo / "assets/player/prompts"
    campaign_path = repo / "campaign.lua"
    proving_ground_path = repo / "proving_ground.lua"

    errors: list[str] = []
    errors.extend(validate_player_assets(load_manifest(player_manifest_path), player_manifest_path, args.require_final_art))
    errors.extend(validate_player_prompts(prompts_dir))
    errors.extend(validate_campaign_data(campaign_path))
    errors.extend(validate_proving_ground_data(proving_ground_path))

    if errors:
        for error in errors:
            print(f"ERROR: {error}")
        return 1

    print("slice_content_ok")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
