#!/usr/bin/env python3
"""Full space suite shell — visual architecture only.

This is the authored shell envelope for the suite. It intentionally leaves
movable furniture, decals, interactables, and collision to scene code.
"""

import math
import os

import bpy


RW, RD, RH = 14.0, 12.0, 5.0
WIN_W, WIN_CZ = 7.0, -0.5
DOOR_Z, DOOR_HALF = -3.5, 0.65
BATH_X, BATH_Z = RW / 2 + 2.5, 2.5


def clear_scene():
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete(use_global=False)
    for mesh in list(bpy.data.meshes):
        if mesh.users == 0:
            bpy.data.meshes.remove(mesh)
    for material in list(bpy.data.materials):
        if material.users == 0:
            bpy.data.materials.remove(material)


def make_mat(name, rgba, roughness=0.6, metallic=0.0):
    mat = bpy.data.materials.get(name)
    if not mat:
        mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    bsdf = mat.node_tree.nodes.get("Principled BSDF")
    bsdf.inputs["Base Color"].default_value = rgba
    bsdf.inputs["Roughness"].default_value = roughness
    bsdf.inputs["Metallic"].default_value = metallic
    return mat


def add_box(name, loc, size, mat):
    bpy.ops.mesh.primitive_cube_add(size=2.0, location=loc)
    obj = bpy.context.active_object
    obj.name = name
    obj.scale = (size[0] / 2.0, size[1] / 2.0, size[2] / 2.0)
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    obj.data.materials.clear()
    obj.data.materials.append(mat)
    return obj


def add_torus(name, loc, major_radius, minor_radius, rotation, mat):
    bpy.ops.mesh.primitive_torus_add(
        major_radius=major_radius,
        minor_radius=minor_radius,
        location=loc,
        rotation=rotation,
        major_segments=48,
        minor_segments=12,
    )
    obj = bpy.context.active_object
    obj.name = name
    obj.data.materials.clear()
    obj.data.materials.append(mat)
    return obj


def select_all_meshes():
    bpy.ops.object.select_all(action="DESELECT")
    meshes = [obj for obj in bpy.data.objects if obj.type == "MESH"]
    for obj in meshes:
        obj.select_set(True)
    if meshes:
        bpy.context.view_layer.objects.active = meshes[0]


clear_scene()

hull = make_mat("EV_Shell_Hull", (0.19, 0.21, 0.26, 1.0), roughness=0.85)
cream = make_mat("EV_Shell_Cream", (0.86, 0.84, 0.80, 1.0), roughness=0.75)
brass = make_mat("EV_Shell_Brass", (0.72, 0.62, 0.42, 1.0), roughness=0.32, metallic=0.75)
glass = make_mat("EV_Shell_Glass", (0.06, 0.08, 0.14, 0.85), roughness=0.08, metallic=0.0)
navy = make_mat("EV_Shell_Navy", (0.12, 0.13, 0.28, 1.0), roughness=0.7)

# Ceiling slab
add_box("Ceiling", (0, RH - 0.06, 0), (RW, 0.12, RD), hull)

# Front wall split around entry door
add_box("FrontWall_L", (-2.2, RH / 2, RD / 2), (8.2, RH, 0.16), hull)
add_box("FrontWall_R", (5.8, RH / 2, RD / 2), (2.1, RH, 0.16), hull)
add_box("EntryHeader", (3.0, RH - 0.45, RD / 2), (1.4, RH - 2.6, 0.16), hull)

# Back wall with clerestory slot and headboard zone
add_box("BackWall_Lower", (0, 1.25, -RD / 2), (RW, 2.5, 0.16), hull)
add_box("BackWall_Upper", (0, RH - 0.16, -RD / 2), (RW, 0.32, 0.16), hull)
add_box("ClerestoryFrameTop", (0, RH - 0.48, -RD / 2 + 0.03), (6.2, 0.08, 0.08), brass)
add_box("ClerestoryFrameBottom", (0, RH - 0.92, -RD / 2 + 0.03), (6.2, 0.08, 0.08), brass)
add_box("HeadboardPanel", (0, 1.8, -5.55), (3.6, 2.8, 0.10), navy)

# Left wall split around main window
add_box("LeftWall_Back", (-RW / 2, RH / 2, -4.5), (0.18, RH, 2.7), hull)
add_box("LeftWall_Front", (-RW / 2, RH / 2, 3.5), (0.18, RH, 3.0), hull)
add_box("WindowGlass", (-RW / 2 + 0.06, RH / 2, WIN_CZ), (0.03, RH - 0.8, WIN_W), glass)
add_box("WindowFrameTop", (-RW / 2 + 0.09, RH - 0.3, WIN_CZ), (0.05, 0.15, WIN_W + 0.3), brass)
add_box("WindowFrameBottom", (-RW / 2 + 0.09, 0.3, WIN_CZ), (0.05, 0.15, WIN_W + 0.3), brass)
add_box("WindowFrameBack", (-RW / 2 + 0.09, RH / 2, WIN_CZ - WIN_W / 2 - 0.1), (0.05, RH - 0.6, 0.15), brass)
add_box("WindowFrameFront", (-RW / 2 + 0.09, RH / 2, WIN_CZ + WIN_W / 2 + 0.1), (0.05, RH - 0.6, 0.15), brass)
for i in range(1, 4):
    mz = WIN_CZ - WIN_W / 2 + i * (WIN_W / 4.0)
    add_box(f"WindowMullion_{i}", (-RW / 2 + 0.10, RH / 2, mz), (0.03, RH - 0.9, 0.06), brass)
add_box("WindowCrossbar", (-RW / 2 + 0.10, RH * 0.33, WIN_CZ), (0.03, 0.06, WIN_W), brass)

# Right wall split around porthole and two doors
wall_front_z = (RD / 2 + (DOOR_Z + DOOR_HALF)) / 2.0
wall_front_d = RD / 2 - (DOOR_Z + DOOR_HALF)
wall_back_z = (-RD / 2 + (DOOR_Z - DOOR_HALF)) / 2.0
wall_back_d = (DOOR_Z - DOOR_HALF) - (-RD / 2)
add_box("RightWall_Front", (RW / 2, RH / 2, wall_front_z), (0.18, RH, wall_front_d), hull)
add_box("RightWall_Back", (RW / 2, RH / 2, wall_back_z), (0.18, RH, wall_back_d), hull)
add_box("RightDoorHeader", (RW / 2, RH - 0.5, DOOR_Z), (0.18, RH - 2.7, DOOR_HALF * 2), hull)
add_torus("PortholeRing", (RW / 2 - 0.10, RH * 0.55, -1.5), 1.45, 0.10, (0, math.radians(90), 0), brass)
add_torus("PortholeInner", (RW / 2 - 0.08, RH * 0.55, -1.5), 1.18, 0.06, (0, math.radians(90), 0), navy)

# Wallpaper/trim accents
add_box("LeftCreamBack", (-RW / 2 + 0.17, RH * 0.4, -RD / 2 + 1.5), (0.04, RH * 0.8, 2.5), cream)
add_box("LeftCreamFront", (-RW / 2 + 0.17, RH * 0.4, RD / 2 - 1.5), (0.04, RH * 0.8, 2.5), cream)
add_box("RightCreamFront", (RW / 2 - 0.17, RH * 0.4, wall_front_z), (0.04, RH * 0.8, wall_front_d - 0.5), cream)
add_box("RightCreamBack", (RW / 2 - 0.17, RH * 0.4, wall_back_z), (0.04, RH * 0.8, wall_back_d - 0.5), cream)

# Bathroom shell volume beyond service wall
add_box("BathroomBack", (BATH_X, 1.5, BATH_Z - 2.0), (4.0, 3.0, 0.16), cream)
add_box("BathroomFar", (BATH_X + 2.0, 1.5, BATH_Z), (0.16, 3.0, 4.0), cream)
add_box("BathroomCeiling", (BATH_X, 3.0, BATH_Z), (4.0, 0.12, 4.0), hull)
add_box("BathroomMirrorFrame", (BATH_X - 0.8, 1.4, BATH_Z + 1.49), (1.9, 1.2, 0.04), brass)

select_all_meshes()

export_path = os.environ.get("EV_EXPORT_PATH", "/Users/klaus/space_suite_shell.glb")
bpy.ops.export_scene.gltf(
    filepath=export_path,
    export_format="GLB",
    export_yup=True,
    export_apply=True,
    use_selection=True,
    export_animations=False,
)

print(f"[EV] Exported: {export_path}")
print("")
print("[EV] Shell placement code:")
print('    add_shell(s, "space_suite_shell", 0, 0, 0, 1, 1, 1, 0, MAT_CONCRETE, WHITE);')
print("    // Pair with explicit add_collision_wall/floor/ceiling calls in build_space_suite().")
