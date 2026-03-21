#!/usr/bin/env python3

from __future__ import annotations

import re
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SRC_DIR = ROOT / "src"
ASSET_DIR = ROOT / "assets"
SCRIPT_DIR = ROOT / "scripts"
REGISTRY_C = SRC_DIR / "model_registry.c"
REGISTRY_H = SRC_DIR / "model_registry.h"
SOURCE_EXEMPTIONS: set[str] = set()

ENTRY_RE = re.compile(
    r'\{"([^"]+)",\s*"([^"]+)",\s*(MODEL_KIND_\w+),\s*(true|false),\s*(\d+),\s*(true|false),\s*(MODEL_STATUS_\w+)\}'
)
REF_RE = re.compile(r'find_model_asset\("([^"]+)"\)')
SHELL_REF_RE = re.compile(r'add_shell\([^,]+,\s*"([^"]+)"')
BUDGET_RE = re.compile(r"#define\s+MODEL_STARTUP_VAO_BUDGET\s+(\d+)")
COLLISION_RE = re.compile(r'add_collision_(?:wall|floor|ceiling)\(')


def fail(message: str, errors: list[str]) -> None:
    errors.append(message)


def main() -> int:
    errors: list[str] = []

    registry_text = REGISTRY_C.read_text()
    header_text = REGISTRY_H.read_text()
    entries = []
    for match in ENTRY_RE.finditer(registry_text):
        name, path, kind, startup, vao, preserve, status = match.groups()
        entries.append(
            {
                "name": name,
                "path": path,
                "kind": kind,
                "startup_load": startup == "true",
                "estimated_vao_cost": int(vao),
                "preserve_blender_colors": preserve == "true",
                "status": status,
            }
        )

    if not entries:
        fail("no model registry entries parsed", errors)

    budget_match = BUDGET_RE.search(header_text)
    if not budget_match:
        fail("missing MODEL_STARTUP_VAO_BUDGET define", errors)
        budget = 0
    else:
        budget = int(budget_match.group(1))

    registry_by_name = {}
    for entry in entries:
        if entry["name"] in registry_by_name:
            fail(f"duplicate registry entry '{entry['name']}'", errors)
        registry_by_name[entry["name"]] = entry

    scene_refs = set()
    shell_refs: dict[str, list[tuple[Path, int]]] = {}
    for path in SRC_DIR.glob("*.c"):
        text = path.read_text()
        scene_refs.update(REF_RE.findall(text))

        lines = text.splitlines()
        for i, line in enumerate(lines):
            for match in SHELL_REF_RE.finditer(line):
                name = match.group(1)
                shell_refs.setdefault(name, []).append((path, i))
                start = max(0, i - 60)
                end = min(len(lines), i + 61)
                window = "\n".join(lines[start:end])
                if not COLLISION_RE.search(window):
                    fail(f"shell '{name}' in {path.name}:{i + 1} has no nearby authored collision", errors)

    for ref in sorted(scene_refs):
        if ref not in registry_by_name:
            fail(f"scene reference '{ref}' is not present in model registry", errors)

    for ref, sites in sorted(shell_refs.items()):
        entry = registry_by_name.get(ref)
        if not entry:
            fail(f"shell reference '{ref}' is not present in model registry", errors)
            continue
        if entry["kind"] != "MODEL_KIND_SHELL":
            fail(f"add_shell() uses non-shell asset '{ref}' ({entry['kind']})", errors)

    for entry in entries:
        if entry["status"] != "MODEL_STATUS_ACTIVE" or entry["kind"] != "MODEL_KIND_SHELL":
            continue
        if entry["name"] not in shell_refs:
            fail(f"active shell asset '{entry['name']}' is not placed via add_shell()", errors)

    for entry in entries:
        if entry["status"] != "MODEL_STATUS_ACTIVE":
            continue
        asset_path = ROOT / entry["path"]
        if not asset_path.exists():
            fail(f"active registry asset missing: {entry['name']} -> {entry['path']}", errors)
        if entry["path"].endswith(".glb") and entry["name"] not in SOURCE_EXEMPTIONS:
            source_path = SCRIPT_DIR / f"model_{entry['name']}.py"
            if not source_path.exists():
                fail(f"active GLB asset '{entry['name']}' has no source script at scripts/model_{entry['name']}.py", errors)

    startup_cost = sum(
        entry["estimated_vao_cost"]
        for entry in entries
        if entry["status"] == "MODEL_STATUS_ACTIVE" and entry["startup_load"]
    )
    if startup_cost > budget:
        fail(f"startup VAO cost {startup_cost} exceeds budget {budget}", errors)

    obj_exemptions = {"skytower"}
    for ref in sorted(scene_refs):
        entry = registry_by_name.get(ref)
        if not entry:
            continue
        if entry["path"].endswith(".obj") and ref not in obj_exemptions:
            fail(f"scene-referenced model '{ref}' still points to OBJ asset '{entry['path']}'", errors)

    asset_glbs = {path.stem for path in ASSET_DIR.glob("*.glb")}
    registry_glbs = {
        entry["name"]
        for entry in entries
        if entry["path"].endswith(".glb")
    }
    for glb_name in sorted(asset_glbs - registry_glbs):
        fail(f"GLB asset '{glb_name}.glb' has no registry entry", errors)

    if errors:
        print("[validate_model_registry] FAIL")
        for error in errors:
            print(f"  - {error}")
        return 1

    print("[validate_model_registry] PASS")
    print(f"  registry entries: {len(entries)}")
    print(f"  scene references: {len(scene_refs)}")
    print(f"  startup VAO cost: {startup_cost}/{budget}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
