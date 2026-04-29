#!/usr/bin/env python3
"""Shared helpers for backfilled EV model source scripts.

These scripts are intentionally conservative: they import the current runtime
asset into a clean Blender scene, then re-export it through the canonical GLB
pipeline so every active registry asset has a reproducible source entrypoint.
"""

import os
from pathlib import Path

import bpy


SCRIPT_DIR = Path(__file__).resolve().parent
ASSET_DIR = SCRIPT_DIR.parent / "assets"


def _clear_scene():
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete(use_global=False)
    for mesh in list(bpy.data.meshes):
        if mesh.users == 0:
            bpy.data.meshes.remove(mesh)
    for material in list(bpy.data.materials):
        if material.users == 0:
            bpy.data.materials.remove(material)


def _select_imported_objects(objects):
    bpy.ops.object.select_all(action="DESELECT")
    for obj in objects:
        obj.select_set(True)
    if objects:
        bpy.context.view_layer.objects.active = objects[0]


def _import_gltf(asset_path: Path):
    before = set(bpy.data.objects)
    bpy.ops.import_scene.gltf(filepath=str(asset_path))
    imported = [obj for obj in bpy.data.objects if obj not in before]
    if not imported:
        raise RuntimeError(f"No objects imported from {asset_path}")
    return imported


def _export_gltf(asset_name: str, export_animations: bool = False):
    export_path = os.environ.get("EV_EXPORT_PATH", f"/Users/klaus/{asset_name}.glb")
    bpy.ops.export_scene.gltf(
        filepath=export_path,
        export_format="GLB",
        export_yup=True,
        export_apply=True,
        use_selection=True,
        export_animations=export_animations,
    )
    print(f"[EV] Exported: {export_path}")
    return export_path


def _print_prop_hint(asset_name: str, material: str, color: str):
    print("")
    print("[EV] Placement code:")
    print(f'    int {asset_name} = find_model_asset("{asset_name}");')
    print(f"    if ({asset_name} >= 0) {{")
    print("        add_model(s, 0, 0, 0,")
    print("                  1, 1, 1,")
    print("                  0,")
    print(f"                  {asset_name},")
    print(f"                  {material},")
    print(f"                  {color});")
    print("    }")


def _print_shell_hint(asset_name: str, material: str, color: str):
    print("")
    print("[EV] Shell placement code:")
    print(f'    add_shell(s, "{asset_name}", 0, 0, 0, 1, 1, 1, 0, {material}, {color});')
    print("    // Pair with explicit add_collision_wall/floor/ceiling calls in scene code.")


def backfill_asset(asset_name: str, *, material: str = "MAT_CONCRETE",
                   color: str = "WHITE", kind: str = "prop",
                   export_animations: bool = False):
    asset_path = ASSET_DIR / f"{asset_name}.glb"
    if not asset_path.exists():
        raise FileNotFoundError(f"Missing runtime asset: {asset_path}")

    _clear_scene()
    imported = _import_gltf(asset_path)
    _select_imported_objects(imported)
    _export_gltf(asset_name, export_animations=export_animations)

    if kind == "shell":
        _print_shell_hint(asset_name, material, color)
    else:
        _print_prop_hint(asset_name, material, color)
