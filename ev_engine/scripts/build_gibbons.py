#!/usr/bin/env python3
"""
build_gibbons.py — Build, rig, and animate Gibbons for Endearing Void
=====================================================================
Gravity Bone bellhop. Vinyl toy proportions. 1.30m tall.
Chunky geometric character — cubes and cylinders, not organic.

Materials from ev_model_kit.py (locked palette).
Exports GLB with Y-up, 4 animations: Idle(0), Walk(1), Bow(2), Gesture(3)
Note: Raylib loads animations in alphabetical order by name.

Run in Blender via: python3 blender_send.py scripts/build_gibbons.py
"""

import bpy
import bmesh
import math
import mathutils
from mathutils import Vector, Matrix, Euler

# ─────────────────────────────────────────────────────────────────────
# CLEANUP
# ─────────────────────────────────────────────────────────────────────

if bpy.context.active_object and bpy.context.active_object.mode != 'OBJECT':
    bpy.ops.object.mode_set(mode='OBJECT')
bpy.ops.object.select_all(action='SELECT')
bpy.ops.object.delete()
for m in list(bpy.data.meshes):
    bpy.data.meshes.remove(m)
for m in list(bpy.data.materials):
    bpy.data.materials.remove(m)
for a in list(bpy.data.armatures):
    bpy.data.armatures.remove(a)
for act in list(bpy.data.actions):
    bpy.data.actions.remove(act)

print("[GIBBONS] Scene cleared")

# ─────────────────────────────────────────────────────────────────────
# MATERIALS (from ev_model_kit.py locked palette)
# ─────────────────────────────────────────────────────────────────────

def make_mat(name, r, g, b, metallic=0, roughness=0.8):
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    bsdf = mat.node_tree.nodes["Principled BSDF"]
    bsdf.inputs["Base Color"].default_value = (r/255, g/255, b/255, 1)
    bsdf.inputs["Metallic"].default_value = metallic
    bsdf.inputs["Roughness"].default_value = roughness
    return mat

MAT_HEAD     = make_mat("EV_Gibbons_Head",     242, 237, 230, 0.0, 0.15)
MAT_JACKET   = make_mat("EV_Gibbons_Jacket",   184,  71,  46, 0.0, 0.75)
MAT_TROUSERS = make_mat("EV_Gibbons_Trousers",  20,  26,  46, 0.0, 0.80)
MAT_BUTTONS  = make_mat("EV_Gibbons_Buttons",  217, 179,  77, 0.7, 0.45)
MAT_HALO     = make_mat("EV_Gibbons_Halo",     230, 191,  64, 0.85, 0.30)
MAT_EYES     = make_mat("EV_Gibbons_Eyes",     115, 204, 166, 0.0, 0.10)
MAT_CAP      = make_mat("EV_Gibbons_Cap",      200,  50,  42, 0.0, 0.70)
MAT_SHOES    = make_mat("EV_Gibbons_Shoes",     35,  30,  25, 0.0, 0.75)
MAT_TIE      = make_mat("EV_Gibbons_Tie",      195,  45,  40, 0.0, 0.70)
MAT_CUFF     = make_mat("EV_Gibbons_Cuff",      45,  50,  72, 0.0, 0.75)
MAT_CASE     = make_mat("EV_Gibbons_Case",     160, 110,  50, 0.0, 0.70)
MAT_BELT     = make_mat("EV_Gibbons_Belt",      45,  38,  28, 0.0, 0.75)
MAT_MOUTH    = make_mat("EV_Gibbons_Mouth",    140, 100,  75, 0.0, 0.80)

ALL_MATS = [MAT_HEAD, MAT_JACKET, MAT_TROUSERS, MAT_BUTTONS, MAT_HALO,
            MAT_EYES, MAT_CAP, MAT_SHOES, MAT_TIE, MAT_CUFF, MAT_CASE,
            MAT_BELT, MAT_MOUTH]

# ─────────────────────────────────────────────────────────────────────
# GEOMETRY — Gravity Bone vinyl toy bellhop, 1.30m tall
# ─────────────────────────────────────────────────────────────────────
# Blender Z-up. Will export with Y-up (export_yup=True).
# All coordinates in BLENDER space: X=right, Y=forward, Z=up.
# Origin at feet center (0,0,0). Character faces -Y.

parts = []  # (obj, material, bone_name)

def smooth(obj):
    """Apply smooth shading without operator context issues."""
    for poly in obj.data.polygons:
        poly.use_smooth = True

def ensure_object_mode():
    if bpy.context.active_object and bpy.context.active_object.mode != 'OBJECT':
        bpy.ops.object.mode_set(mode='OBJECT')

def add_cube(name, loc, scale, mat, bone, bevel=0.02, subdiv=0):
    """Beveled cube — reads as manufactured, not raw Minecraft box."""
    ensure_object_mode()
    bpy.ops.mesh.primitive_cube_add(size=1, location=loc)
    obj = bpy.context.active_object
    obj.name = name
    obj.scale = scale
    bpy.ops.object.transform_apply(scale=True)
    if bevel > 0:
        mod = obj.modifiers.new("Bevel", "BEVEL")
        mod.width = bevel
        mod.segments = 2
        mod.limit_method = 'ANGLE'
        bpy.ops.object.modifier_apply(modifier="Bevel")
    if subdiv > 0:
        mod = obj.modifiers.new("Subsurf", "SUBSURF")
        mod.levels = subdiv
        mod.render_levels = subdiv
        bpy.ops.object.modifier_apply(modifier="Subsurf")
    smooth(obj)
    parts.append((obj, mat, bone))
    return obj

def add_cylinder(name, loc, radius, depth, mat, bone, verts=12, rot=(0,0,0)):
    """Smooth cylinder."""
    bpy.ops.mesh.primitive_cylinder_add(
        radius=radius, depth=depth, vertices=verts,
        location=loc, rotation=rot
    )
    obj = bpy.context.active_object
    obj.name = name
    smooth(obj)
    parts.append((obj, mat, bone))
    return obj

def add_sphere(name, loc, radius, mat, bone, segments=12, rings=8):
    """UV sphere for head, etc."""
    bpy.ops.mesh.primitive_uv_sphere_add(
        radius=radius, segments=segments, ring_count=rings,
        location=loc
    )
    obj = bpy.context.active_object
    obj.name = name
    smooth(obj)
    parts.append((obj, mat, bone))
    return obj

# ── Proportions (Blender Z-up, ~1.30m total) ──
# Vinyl toy meets Gravity Bone. Chunky but SMOOTH.
# Beveled edges, subdivided organics, smooth shading throughout.

shoe_h = 0.07;  shoe_w = 0.18;  shoe_d = 0.22
shin_h = 0.15;  shin_w = 0.17
thigh_h = 0.15; thigh_w = 0.18
body_w = 0.40;  body_h = 0.32;  body_d = 0.26
shoulder_w = 0.48; shoulder_h = 0.07
upper_arm_w = 0.14; upper_arm_h = 0.14
forearm_w = 0.13; forearm_h = 0.13
hand_s = 0.10
head_r = 0.22   # BIGGER head — Pixar proportions
cap_w = 0.38; cap_h = 0.10
neck_w = 0.14; neck_h = 0.04
tie_w = 0.10; tie_h = 0.22

# ── Vertical stack (Z-up in Blender) ──
foot_z = 0
ankle_z = foot_z + shoe_h
knee_z = ankle_z + shin_h
hip_z = knee_z + thigh_h
torso_mid_z = hip_z + body_h / 2
shoulder_z = hip_z + body_h
neck_top_z = shoulder_z + shoulder_h + neck_h
head_z = neck_top_z + head_r  # sphere center
cap_z = neck_top_z + head_r * 2 + cap_h / 2

total_h = neck_top_z + head_r * 2 + cap_h
print(f"[GIBBONS] Total height: {total_h:.3f}m (target: 1.30m)")

leg_spread = body_w * 0.30

# ── SHOES — rounded, grounded ──
add_cube("Shoe_L", (-leg_spread, 0.02, foot_z + shoe_h/2), (shoe_w, shoe_d, shoe_h), MAT_SHOES, "Foot_L", bevel=0.015)
add_cube("Shoe_R", ( leg_spread, 0.02, foot_z + shoe_h/2), (shoe_w, shoe_d, shoe_h), MAT_SHOES, "Foot_R", bevel=0.015)

# ── LEGS — cylinders for organic limb feel ──
add_cylinder("Shin_L", (-leg_spread, 0, ankle_z + shin_h/2), shin_w/2, shin_h, MAT_TROUSERS, "Shin_L")
add_cylinder("Shin_R", ( leg_spread, 0, ankle_z + shin_h/2), shin_w/2, shin_h, MAT_TROUSERS, "Shin_R")
add_cylinder("Thigh_L", (-leg_spread, 0, knee_z + thigh_h/2), thigh_w/2, thigh_h, MAT_TROUSERS, "Thigh_L")
add_cylinder("Thigh_R", ( leg_spread, 0, knee_z + thigh_h/2), thigh_w/2, thigh_h, MAT_TROUSERS, "Thigh_R")

# ── BELT ──
belt_h = 0.04
add_cube("Belt", (0, 0, hip_z + belt_h/2), (body_w + 0.02, body_d + 0.01, belt_h), MAT_BELT, "Spine", bevel=0.008)
add_cube("Buckle", (0, -body_d/2 - 0.01, hip_z + belt_h/2), (0.04, 0.02, 0.04), MAT_BUTTONS, "Spine", bevel=0.005)

# ── TORSO — beveled + subdivided for soft volume ──
add_cube("Torso", (0, 0, torso_mid_z), (body_w, body_d, body_h), MAT_JACKET, "Spine", bevel=0.03, subdiv=1)

# ── TIE ──
add_cube("Tie", (0, -body_d/2 - 0.005, torso_mid_z + 0.04), (tie_w, 0.02, tie_h), MAT_TIE, "Chest", bevel=0.008)

# ── LAPELS ──
lapel_w = 0.07; lapel_h = 0.24
add_cube("Lapel_L", (-body_w/2 + lapel_w/2 + 0.02, -body_d/2 - 0.004, torso_mid_z + 0.06),
         (lapel_w, 0.015, lapel_h), MAT_CUFF, "Chest", bevel=0.005)
add_cube("Lapel_R", ( body_w/2 - lapel_w/2 - 0.02, -body_d/2 - 0.004, torso_mid_z + 0.06),
         (lapel_w, 0.015, lapel_h), MAT_CUFF, "Chest", bevel=0.005)

# ── POCKET SQUARE ──
add_cube("Pocket", (-body_w/2 + 0.08, -body_d/2 - 0.004, shoulder_z - 0.07),
         (0.10, 0.012, 0.06), MAT_HEAD, "Chest", bevel=0.003)

# ── BUTTONS — small spheres ──
for i, bz in enumerate([torso_mid_z + 0.10, torso_mid_z - 0.04]):
    for j, bx in enumerate([0.06, -0.06]):
        add_sphere(f"Button_{i}_{j}", (bx, -body_d/2 - 0.012, bz),
                   0.018, MAT_BUTTONS, "Chest", segments=8, rings=6)

# ── SHOULDERS — rounded ──
add_cube("Shoulders", (0, 0, shoulder_z + shoulder_h/2), (shoulder_w, body_d, shoulder_h), MAT_JACKET, "Chest", bevel=0.02)

# ── ARMS — cylinders for proper limb read ──
arm_x = shoulder_w/2 + upper_arm_w/2

add_cylinder("UpperArm_L", (-arm_x, 0, shoulder_z - upper_arm_h/2), upper_arm_w/2, upper_arm_h, MAT_JACKET, "UpperArm_L")
add_cylinder("Forearm_L",  (-arm_x, 0, shoulder_z - upper_arm_h - forearm_h/2), forearm_w/2, forearm_h, MAT_CUFF, "Forearm_L")
add_sphere("Hand_L", (-arm_x, 0, shoulder_z - upper_arm_h - forearm_h - hand_s/2), hand_s/2, MAT_HEAD, "Hand_L", segments=8, rings=6)

add_cylinder("UpperArm_R", ( arm_x, 0, shoulder_z - upper_arm_h/2), upper_arm_w/2, upper_arm_h, MAT_JACKET, "UpperArm_R")
add_cylinder("Forearm_R",  ( arm_x, 0, shoulder_z - upper_arm_h - forearm_h/2), forearm_w/2, forearm_h, MAT_CUFF, "Forearm_R")
add_sphere("Hand_R", ( arm_x, 0, shoulder_z - upper_arm_h - forearm_h - hand_s/2), hand_s/2, MAT_HEAD, "Hand_R", segments=8, rings=6)

# ── BRIEFCASE — beveled leather ──
case_w = 0.26; case_h = 0.18; case_d = 0.10
case_z = shoulder_z - upper_arm_h - forearm_h - hand_s - case_h/2
add_cube("Briefcase", (-arm_x, 0, case_z), (case_d, case_w, case_h), MAT_CASE, "Hand_L", bevel=0.01)
add_cube("Latch", (-arm_x, -case_w/2 - 0.008, case_z + case_h * 0.3), (0.03, 0.02, 0.04), MAT_BUTTONS, "Hand_L", bevel=0.004)
add_cube("CaseHandle", (-arm_x, 0, case_z + case_h/2 + 0.015), (0.02, 0.08, 0.02), MAT_BELT, "Hand_L", bevel=0.003)

# ── NECK — smooth cylinder ──
add_cylinder("Neck", (0, 0, shoulder_z + shoulder_h + neck_h/2), neck_w/2, neck_h, MAT_HEAD, "Neck")

# ── HEAD — SPHERE (the signature vinyl toy look) ──
add_sphere("Head", (0, 0, head_z), head_r, MAT_HEAD, "Head", segments=16, rings=12)

# ── EYES — bigger, more expressive ──
eye_z = head_z + head_r * 0.15
eye_x = head_r * 0.40
eye_y = -head_r * 0.88
add_sphere("Eye_L", (-eye_x, eye_y, eye_z), 0.038, MAT_EYES, "Head", segments=10, rings=8)
add_sphere("Eye_R", ( eye_x, eye_y, eye_z), 0.038, MAT_EYES, "Head", segments=10, rings=8)
# Pupils — dark dots inside eyes
MAT_PUPIL = make_mat("EV_Gibbons_Pupil", 15, 12, 10, 0.0, 0.90)
add_sphere("Pupil_L", (-eye_x, eye_y - 0.02, eye_z), 0.018, MAT_PUPIL, "Head", segments=6, rings=4)
add_sphere("Pupil_R", ( eye_x, eye_y - 0.02, eye_z), 0.018, MAT_PUPIL, "Head", segments=6, rings=4)

# ── EYEBROWS — expressive, on face bones ──
brow_z = eye_z + 0.045
brow_y = eye_y + 0.005
add_cube("Brow_L", (-eye_x, brow_y, brow_z), (0.05, 0.012, 0.015), MAT_BELT, "Brow_L", bevel=0.004)
add_cube("Brow_R", ( eye_x, brow_y, brow_z), (0.05, 0.012, 0.015), MAT_BELT, "Brow_R", bevel=0.004)

# ── NOSE — small sphere, reads as character ──
nose_z = head_z - head_r * 0.05
nose_y = -head_r * 0.95
add_sphere("Nose", (0, nose_y, nose_z), 0.025, MAT_HEAD, "Head", segments=8, rings=6)

# ── MOUTH — on jaw bone for open/close ──
mouth_z = head_z - head_r * 0.40
add_cube("Mouth", (0, eye_y - 0.01, mouth_z), (0.06, 0.01, 0.015), MAT_MOUTH, "Jaw", bevel=0.003)

# ── CAP — beveled, sits on sphere head ──
add_cube("Cap", (0, 0, neck_top_z + head_r * 1.6), (cap_w, cap_w, cap_h), MAT_CAP, "Head", bevel=0.015)
# Cap brim — forward overhang
brim_z = neck_top_z + head_r * 1.6 - cap_h * 0.35
add_cube("Brim", (0, -head_r * 0.6, brim_z),
         (cap_w + 0.08, cap_w * 0.5, 0.025), MAT_CAP, "Head", bevel=0.008)
# Cap band — gold trim
add_cube("CapBand", (0, 0, brim_z), (cap_w + 0.015, cap_w + 0.015, 0.028), MAT_HALO, "Head", bevel=0.005)

# ── HALO — torus ring floating above cap ──
bpy.ops.mesh.primitive_torus_add(
    major_radius=0.18, minor_radius=0.012,
    major_segments=24, minor_segments=8,
    location=(0, 0, neck_top_z + head_r * 2 + cap_h + 0.04)
)
halo = bpy.context.active_object
halo.name = "Halo"
smooth(halo)
parts.append((halo, MAT_HALO, "Head"))

print(f"[GIBBONS] {len(parts)} parts created")

# ─────────────────────────────────────────────────────────────────────
# JOIN ALL PARTS INTO ONE MESH
# ─────────────────────────────────────────────────────────────────────

# First, set up material slots and assign per-part materials
# We need to track which faces belong to which bone for vertex groups

# Step 1: Add all materials to first object
main_obj = parts[0][0]
bpy.context.view_layer.objects.active = main_obj

# Collect unique materials and create index
mat_index = {}
for mat in ALL_MATS:
    if mat.name not in mat_index:
        mat_index[mat.name] = len(mat_index)
        if mat not in main_obj.data.materials.values():
            main_obj.data.materials.append(mat)

# Apply material to each part's faces
for obj, mat, bone in parts:
    if obj == main_obj:
        # Assign material to all faces of main object
        mi = mat_index.get(mat.name, 0)
        for face in obj.data.polygons:
            face.material_index = mi
    else:
        # Add all materials to this object too (needed for join)
        while len(obj.data.materials) < len(ALL_MATS):
            obj.data.materials.append(None)
        for i, m in enumerate(ALL_MATS):
            obj.data.materials[i] = m
        mi = mat_index.get(mat.name, 0)
        for face in obj.data.polygons:
            face.material_index = mi

# Step 2: Join
ensure_object_mode()
ensure_object_mode(); bpy.ops.object.select_all(action='DESELECT')
for obj, mat, bone in parts:
    obj.select_set(True)
bpy.context.view_layer.objects.active = main_obj
bpy.ops.object.join()

gibbons_mesh = bpy.context.active_object
gibbons_mesh.name = "Gibbons"

# Clean up unused material slots
# (keep all — engine reads material slots)

# Set origin to bottom center
bpy.ops.object.origin_set(type='ORIGIN_GEOMETRY', center='BOUNDS')
bbox = [gibbons_mesh.matrix_world @ Vector(c) for c in gibbons_mesh.bound_box]
min_z = min(v.z for v in bbox)
cursor_save = bpy.context.scene.cursor.location.copy()
center = gibbons_mesh.location.copy()
bpy.context.scene.cursor.location = (center.x, center.y, min_z)
bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
bpy.context.scene.cursor.location = cursor_save

# Move mesh so origin is at world origin
gibbons_mesh.location = (0, 0, 0)

# Count tris
tri_count = sum(max(0, len(p.vertices) - 2) for p in gibbons_mesh.data.polygons)
vert_count = len(gibbons_mesh.data.vertices)
print(f"[GIBBONS] Joined mesh: {tri_count} tris, {vert_count} verts")

# ─────────────────────────────────────────────────────────────────────
# ARMATURE
# ─────────────────────────────────────────────────────────────────────

# Create armature
bpy.ops.object.armature_add(enter_editmode=True, location=(0, 0, 0))
arm_obj = bpy.context.active_object
arm_obj.name = "Gibbons_Armature"
arm = arm_obj.data
arm.name = "Gibbons_Rig"

# Remove default bone
for b in arm.edit_bones:
    arm.edit_bones.remove(b)

def add_bone(name, head, tail, parent_name=None, connect=False):
    """Add a bone in edit mode. Coords in Blender Z-up space."""
    bone = arm.edit_bones.new(name)
    bone.head = head
    bone.tail = tail
    if parent_name and parent_name in arm.edit_bones:
        bone.parent = arm.edit_bones[parent_name]
        bone.use_connect = connect
    return bone

# ── Bone positions (Blender Z-up) ──
# Adjusted to match mesh proportions

root_z = hip_z  # root at hip level

# Spine chain
add_bone("Root",    (0, 0, 0),      (0, 0, root_z))
add_bone("Hips",    (0, 0, root_z), (0, 0, root_z + 0.05), "Root", True)
add_bone("Spine",   (0, 0, root_z + 0.05), (0, 0, torso_mid_z), "Hips", True)
add_bone("Chest",   (0, 0, torso_mid_z), (0, 0, shoulder_z), "Spine", True)
add_bone("Neck",    (0, 0, shoulder_z + shoulder_h), (0, 0, neck_top_z), "Chest")
add_bone("Head",    (0, 0, neck_top_z), (0, 0, neck_top_z + head_r * 2 + cap_h), "Neck", True)

# Face bones — expressions
add_bone("Brow_L",  (-eye_x, 0, brow_z), (-eye_x, 0, brow_z + 0.03), "Head")
add_bone("Brow_R",  ( eye_x, 0, brow_z), ( eye_x, 0, brow_z + 0.03), "Head")
add_bone("Jaw",     (0, 0, mouth_z),      (0, 0, mouth_z - 0.03), "Head")

# Left leg
add_bone("Thigh_L", (-leg_spread, 0, hip_z),    (-leg_spread, 0, knee_z), "Hips")
add_bone("Shin_L",  (-leg_spread, 0, knee_z),   (-leg_spread, 0, ankle_z), "Thigh_L", True)
add_bone("Foot_L",  (-leg_spread, 0, ankle_z),  (-leg_spread, 0.10, ankle_z - 0.02), "Shin_L", True)

# Right leg
add_bone("Thigh_R", ( leg_spread, 0, hip_z),    ( leg_spread, 0, knee_z), "Hips")
add_bone("Shin_R",  ( leg_spread, 0, knee_z),   ( leg_spread, 0, ankle_z), "Thigh_R", True)
add_bone("Foot_R",  ( leg_spread, 0, ankle_z),  ( leg_spread, 0.10, ankle_z - 0.02), "Shin_R", True)

# Left arm
ua_z = shoulder_z - upper_arm_h/2
add_bone("UpperArm_L", (-arm_x, 0, shoulder_z), (-arm_x, 0, shoulder_z - upper_arm_h), "Chest")
add_bone("Forearm_L",  (-arm_x, 0, shoulder_z - upper_arm_h), (-arm_x, 0, shoulder_z - upper_arm_h - forearm_h), "UpperArm_L", True)
add_bone("Hand_L",     (-arm_x, 0, shoulder_z - upper_arm_h - forearm_h), (-arm_x, 0, shoulder_z - upper_arm_h - forearm_h - hand_s), "Forearm_L", True)

# Right arm
add_bone("UpperArm_R", ( arm_x, 0, shoulder_z), ( arm_x, 0, shoulder_z - upper_arm_h), "Chest")
add_bone("Forearm_R",  ( arm_x, 0, shoulder_z - upper_arm_h), ( arm_x, 0, shoulder_z - upper_arm_h - forearm_h), "UpperArm_R", True)
add_bone("Hand_R",     ( arm_x, 0, shoulder_z - upper_arm_h - forearm_h), ( arm_x, 0, shoulder_z - upper_arm_h - forearm_h - hand_s), "Forearm_R", True)

bpy.ops.object.mode_set(mode='OBJECT')

bone_names = [b.name for b in arm_obj.data.bones]
print(f"[GIBBONS] Armature: {len(bone_names)} bones: {bone_names}")

# ─────────────────────────────────────────────────────────────────────
# VERTEX GROUP ASSIGNMENT (rigid skinning — each vert to one bone)
# ─────────────────────────────────────────────────────────────────────

# Parent mesh to armature with empty groups
gibbons_mesh.select_set(True)
arm_obj.select_set(True)
bpy.context.view_layer.objects.active = arm_obj
bpy.ops.object.parent_set(type='ARMATURE_NAME')

# Now assign vertices based on which part they came from
# We stored bone assignments in 'parts' list
# After join, vertices are in order of parts

bpy.context.view_layer.objects.active = gibbons_mesh

# Create vertex groups for all bones
for bname in bone_names:
    if bname not in gibbons_mesh.vertex_groups:
        gibbons_mesh.vertex_groups.new(name=bname)

# Track vertex offset as we go through original parts
# Each cube has 8 verts, each cylinder has (verts * 2 + 2)
vert_offset = 0
for obj, mat, bone_name in parts:
    # Count verts this part contributed
    # Cubes: 8 verts (after apply scale, before join)
    # Cylinders: vertices * 2 + 2 (caps + sides)
    # We need to figure out vert count per part

    # Since we applied transforms before join, the vert count is deterministic
    # Cube = 8 verts, Cylinder with N sides = N*2+2 verts (with cap fill)
    # Actually Blender cylinders: N*2 verts for sides + N fill verts per cap = N*4? No...
    # Default cylinder_add creates: vertices (sides) * 2 for top/bottom rings
    # With fill_type='NGON' (default): 2 extra verts for cap centers? No, ngon uses ring verts.
    # Actually: cylinder with N vertices = N*2 verts (top ring + bottom ring), no center vert for ngon
    pass

# The vertex counting approach is fragile. Let's use spatial assignment instead.
# For each vertex, find which bone's region it belongs to based on position.

mesh = gibbons_mesh.data

# Get mesh bounds for reference
all_verts = [v.co for v in mesh.vertices]

# Spatial bone assignment: assign each vertex to nearest bone by region
# This is more robust than counting vertices in join order

for v in mesh.vertices:
    vx, vy, vz = v.co.x, v.co.y, v.co.z

    bone = "Spine"  # default

    # ── Feet ──
    if vz < ankle_z + 0.01:
        bone = "Foot_L" if vx < 0 else "Foot_R"
    # ── Shins ──
    elif vz < knee_z + 0.01 and abs(vx) > 0.02:
        bone = "Shin_L" if vx < 0 else "Shin_R"
    # ── Thighs ──
    elif vz < hip_z + 0.02 and abs(vx) > 0.02:
        bone = "Thigh_L" if vx < 0 else "Thigh_R"
    # ── Head region ──
    elif vz > neck_top_z - 0.02 and abs(vx) < head_r * 2:
        bone = "Head"
    # ── Neck ──
    elif vz > shoulder_z + shoulder_h - 0.01 and vz < neck_top_z + 0.02 and abs(vx) < neck_w:
        bone = "Neck"
    # ── Arms (lateral check) ──
    elif abs(vx) > body_w/2 + 0.02:
        side = "L" if vx < 0 else "R"
        # Vertical position within arm
        if vz > shoulder_z - upper_arm_h/2:
            bone = f"UpperArm_{side}"
        elif vz > shoulder_z - upper_arm_h - forearm_h/2:
            bone = f"Forearm_{side}"
        else:
            bone = f"Hand_{side}"
    # ── Shoulders + upper chest ──
    elif vz > shoulder_z - 0.02:
        bone = "Chest"
    # ── Belt region ──
    elif vz < hip_z + belt_h + 0.02 and vz >= hip_z - 0.01:
        bone = "Hips"
    # ── Torso ──
    elif vz >= hip_z:
        bone = "Spine"
    # ── Below hips (shouldn't happen much) ──
    else:
        bone = "Root"

    # Assign to vertex group
    if bone in gibbons_mesh.vertex_groups:
        vg = gibbons_mesh.vertex_groups[bone]
        vg.add([v.index], 1.0, 'REPLACE')

print(f"[GIBBONS] Vertex groups assigned (rigid skinning)")

# ─────────────────────────────────────────────────────────────────────
# ANIMATIONS — 4 actions, named alphabetically for Raylib load order
# Raylib loads in alphabetical order: Bow=0, Gesture=1, Idle=2, Walk=3
# ─────────────────────────────────────────────────────────────────────

bpy.context.view_layer.objects.active = arm_obj

FPS = 24
PI = math.pi

def create_action(name, frame_count):
    """Create a new action and set it active."""
    action = bpy.data.actions.new(name=name)
    action.use_fake_user = True
    arm_obj.animation_data_create()
    arm_obj.animation_data.action = action
    bpy.context.scene.frame_start = 1
    bpy.context.scene.frame_end = frame_count
    return action

def key_bone_rot(bone_name, frame, euler_deg):
    """Keyframe a bone's rotation (XYZ Euler, in degrees)."""
    bpy.ops.object.mode_set(mode='POSE')
    pbone = arm_obj.pose.bones.get(bone_name)
    if not pbone:
        print(f"[GIBBONS] WARNING: bone '{bone_name}' not found")
        return
    pbone.rotation_mode = 'XYZ'
    pbone.rotation_euler = (
        math.radians(euler_deg[0]),
        math.radians(euler_deg[1]),
        math.radians(euler_deg[2])
    )
    pbone.keyframe_insert(data_path="rotation_euler", frame=frame)
    bpy.ops.object.mode_set(mode='OBJECT')

def key_bone_loc(bone_name, frame, loc):
    """Keyframe a bone's location offset."""
    bpy.ops.object.mode_set(mode='POSE')
    pbone = arm_obj.pose.bones.get(bone_name)
    if not pbone:
        return
    pbone.location = loc
    pbone.keyframe_insert(data_path="location", frame=frame)
    bpy.ops.object.mode_set(mode='OBJECT')

def reset_pose():
    """Reset all bones to rest pose."""
    bpy.ops.object.mode_set(mode='POSE')
    bpy.ops.pose.select_all(action='SELECT')
    bpy.ops.pose.rot_clear()
    bpy.ops.pose.loc_clear()
    bpy.ops.object.mode_set(mode='OBJECT')

# ── ACTION 1: "A_Bow" (alphabetically first → anim index 0) ──
# Torso tilts forward ~30°, head follows, arms stay, briefcase swings
# 48 frames = 2 seconds at 24fps

create_action("A_Bow", 48)
reset_pose()

# Frame 1: upright
key_bone_rot("Spine", 1, (0, 0, 0))
key_bone_rot("Chest", 1, (0, 0, 0))
key_bone_rot("Head",  1, (0, 0, 0))
key_bone_rot("Neck",  1, (0, 0, 0))
key_bone_loc("Root",  1, (0, 0, 0))

# Frame 12: bowing down (Blender: rotation around X tilts forward = positive X)
# In Blender Z-up, tilting forward = rotating around local X axis
key_bone_rot("Spine", 12, (25, 0, 0))
key_bone_rot("Chest", 12, (10, 0, 0))
key_bone_rot("Head",  12, (5, 0, 0))
key_bone_loc("Root",  12, (0, 0, -0.02))

# Frame 24: hold bow
key_bone_rot("Spine", 24, (25, 0, 0))
key_bone_rot("Chest", 24, (10, 0, 0))
key_bone_rot("Head",  24, (5, 0, 0))
key_bone_loc("Root",  24, (0, 0, -0.02))

# Frame 36: coming back up
key_bone_rot("Spine", 36, (8, 0, 0))
key_bone_rot("Chest", 36, (3, 0, 0))
key_bone_rot("Head",  36, (0, 0, 0))
key_bone_loc("Root",  36, (0, 0, -0.005))

# Frame 48: back to upright
key_bone_rot("Spine", 48, (0, 0, 0))
key_bone_rot("Chest", 48, (0, 0, 0))
key_bone_rot("Head",  48, (0, 0, 0))
key_bone_loc("Root",  48, (0, 0, 0))

# Bow expressions: humble pleased smile, brows up
key_bone_rot("Brow_L", 1, (0, 0, 0)); key_bone_rot("Brow_R", 1, (0, 0, 0))
key_bone_rot("Jaw", 1, (0, 0, 0))
key_bone_rot("Brow_L", 12, (15, 0, 0)); key_bone_rot("Brow_R", 12, (15, 0, 0))  # brows up — humble
key_bone_rot("Jaw", 12, (-5, 0, 0))  # slight smile
key_bone_rot("Brow_L", 36, (10, 0, 0)); key_bone_rot("Brow_R", 36, (10, 0, 0))
key_bone_rot("Jaw", 36, (-3, 0, 0))
key_bone_rot("Brow_L", 48, (0, 0, 0)); key_bone_rot("Brow_R", 48, (0, 0, 0))
key_bone_rot("Jaw", 48, (0, 0, 0))

print("[GIBBONS] A_Bow action created (48 frames)")

# ── ACTION 2: "B_Gesture" (anim index 1) ──
# Right arm extends forward invitingly, slight body lean
# 36 frames = 1.5 seconds

create_action("B_Gesture", 36)
reset_pose()

# Frame 1: rest
key_bone_rot("UpperArm_R", 1, (0, 0, 0))
key_bone_rot("Forearm_R",  1, (0, 0, 0))
key_bone_rot("Chest",      1, (0, 0, 0))
key_bone_rot("Head",       1, (0, 0, 0))

# Frame 10: arm extending forward and slightly up
# In Blender Z-up: extending arm forward = rotating around X (positive lifts forward)
key_bone_rot("UpperArm_R", 10, (60, 0, -15))
key_bone_rot("Forearm_R",  10, (20, 0, 0))
key_bone_rot("Chest",      10, (0, 5, 0))  # slight turn toward gesture
key_bone_rot("Head",       10, (0, 5, 0))

# Frame 24: hold
key_bone_rot("UpperArm_R", 24, (60, 0, -15))
key_bone_rot("Forearm_R",  24, (20, 0, 0))
key_bone_rot("Chest",      24, (0, 5, 0))
key_bone_rot("Head",       24, (0, 5, 0))

# Frame 36: return
key_bone_rot("UpperArm_R", 36, (0, 0, 0))
key_bone_rot("Forearm_R",  36, (0, 0, 0))
key_bone_rot("Chest",      36, (0, 0, 0))
key_bone_rot("Head",       36, (0, 0, 0))

# Gesture expressions: encouraging, one brow up, mouth open
key_bone_rot("Brow_L", 1, (0, 0, 0)); key_bone_rot("Brow_R", 1, (0, 0, 0))
key_bone_rot("Jaw", 1, (0, 0, 0))
key_bone_rot("Brow_L", 10, (8, 0, 0)); key_bone_rot("Brow_R", 10, (20, 0, 0))  # asymmetric — character!
key_bone_rot("Jaw", 10, (-8, 0, 0))  # mouth open — "right this way"
key_bone_rot("Brow_L", 24, (8, 0, 0)); key_bone_rot("Brow_R", 24, (20, 0, 0))
key_bone_rot("Jaw", 24, (-6, 0, 0))
key_bone_rot("Brow_L", 36, (0, 0, 0)); key_bone_rot("Brow_R", 36, (0, 0, 0))
key_bone_rot("Jaw", 36, (0, 0, 0))

print("[GIBBONS] B_Gesture action created (36 frames)")

# ── ACTION 3: "C_Idle" (anim index 2) ──
# Breathing, weight shift, subtle head movement, tie fidget
# 96 frames = 4 seconds loop

create_action("C_Idle", 96)
reset_pose()

for f in range(1, 97, 4):
    t = (f - 1) / 96.0  # 0..1
    # Breathing — exaggerated, visible
    breathe = math.sin(t * 2 * PI * 3) * 3.0
    # Weight shift — BIG lateral lean (floppy, alive)
    weight = math.sin(t * 2 * PI) * 5.0
    # Head wander — looking around with personality
    head_y = math.sin(t * 2 * PI * 0.7) * 8.0
    head_x = math.sin(t * 2 * PI * 1.3) * 4.0

    key_bone_rot("Spine", f, (breathe, 0, weight * 0.5))
    key_bone_rot("Chest", f, (breathe * 0.5, 0, -weight * 0.3))
    key_bone_rot("Head",  f, (head_x, head_y, weight * 0.2))
    key_bone_rot("Hips",  f, (0, 0, weight * 0.8))
    key_bone_loc("Root",  f, (weight * 0.005, 0, breathe * 0.002))

    # Tie fidget — right arm lifts, more dramatic
    tie_fidget = 0
    cycle = (t * 4) % 1.0
    if 0.3 < cycle < 0.5:
        tie_fidget = math.sin((cycle - 0.3) / 0.2 * PI) * 25
    key_bone_rot("UpperArm_R", f, (tie_fidget * 0.4, 0, -tie_fidget * 0.1))
    key_bone_rot("Forearm_R",  f, (tie_fidget * 1.2, 0, 0))

    # Left arm briefcase sway
    case_sway = math.sin(t * 2 * PI * 0.5) * 5
    key_bone_rot("UpperArm_L", f, (case_sway, 0, 0))

    # Face — contemplative baseline, occasional surprise
    brow_base = math.sin(t * 2 * PI * 0.4) * 3  # slow gentle brow movement
    # Surprise spike at ~75% through loop
    surprise = 0
    if 0.7 < t < 0.8:
        surprise = math.sin((t - 0.7) / 0.1 * PI) * 18
    key_bone_rot("Brow_L", f, (brow_base + surprise, 0, 0))
    key_bone_rot("Brow_R", f, (brow_base + surprise * 0.8, 0, 0))  # asymmetric
    # Jaw — slight movements, opens on surprise
    jaw_move = math.sin(t * 2 * PI * 0.6) * 2 + surprise * 0.4
    key_bone_rot("Jaw", f, (-jaw_move, 0, 0))

print("[GIBBONS] C_Idle action created (96 frames)")

# ── ACTION 4: "D_Walk" (anim index 3) ──
# Chunky walk cycle — big steps, arm swing, hip sway, head bob
# 24 frames = 1 second loop (one full stride)

create_action("D_Walk", 24)
reset_pose()

for f in range(1, 25):
    t = (f - 1) / 24.0  # 0..1
    phase = t * 2 * PI

    # Legs — BIG opposing swing (Pixar walk — exaggerated, bouncy)
    leg_swing = 40  # degrees — really kick those legs
    l_thigh = math.sin(phase) * leg_swing
    r_thigh = math.sin(phase + PI) * leg_swing
    # Shins bend back dramatically on lift
    l_shin = max(0, math.sin(phase) * 35)
    r_shin = max(0, math.sin(phase + PI) * 35)
    # Feet flex
    l_foot = math.sin(phase) * 12
    r_foot = math.sin(phase + PI) * 12

    key_bone_rot("Thigh_L", f, (l_thigh, 0, 0))
    key_bone_rot("Shin_L",  f, (-l_shin, 0, 0))
    key_bone_rot("Foot_L",  f, (l_foot, 0, 0))
    key_bone_rot("Thigh_R", f, (r_thigh, 0, 0))
    key_bone_rot("Shin_R",  f, (-r_shin, 0, 0))
    key_bone_rot("Foot_R",  f, (r_foot, 0, 0))

    # Arms — big counter-swing with forearm lag
    arm_swing = 30
    key_bone_rot("UpperArm_L", f, (math.sin(phase + PI) * arm_swing, 0, 0))
    key_bone_rot("Forearm_L",  f, (max(0, math.sin(phase + PI)) * 15, 0, 0))
    key_bone_rot("UpperArm_R", f, (math.sin(phase) * arm_swing, 0, 0))
    key_bone_rot("Forearm_R",  f, (max(0, math.sin(phase)) * 15, 0, 0))

    # Torso counter-rotation — more twist
    twist = math.sin(phase) * 8
    key_bone_rot("Spine", f, (0, 0, twist))
    key_bone_rot("Chest", f, (0, 0, -twist * 0.6))

    # Hip sway — exaggerated waddle
    hip_sway = math.sin(phase) * 6
    key_bone_rot("Hips", f, (0, 0, hip_sway))

    # Head bob + look direction — bouncy, alive
    bob = abs(math.sin(phase * 2)) * 4
    head_look = math.sin(phase * 0.5) * 3  # slow look side to side
    key_bone_rot("Head", f, (-bob, head_look, 0))
    key_bone_rot("Neck", f, (0, 0, -hip_sway * 0.3))

    # Root bounce — big, cartoon-y
    bounce = abs(math.sin(phase * 2)) * 0.03
    lateral = math.sin(phase) * 0.01  # side-to-side waddle
    key_bone_loc("Root", f, (lateral, 0, bounce))

    # Walk face — determined, slight frown, jaw set
    key_bone_rot("Brow_L", f, (-5, 0, 0))  # brows down — focused
    key_bone_rot("Brow_R", f, (-5, 0, 0))
    key_bone_rot("Jaw", f, (-2, 0, 0))  # mouth slightly open from effort

print("[GIBBONS] D_Walk action created (24 frames)")

# ─────────────────────────────────────────────────────────────────────
# FINALIZE — pack NLA tracks, set rest pose, export
# ─────────────────────────────────────────────────────────────────────

# Reset to rest pose
reset_pose()

# Clear current action
arm_obj.animation_data.action = None

# Push all actions to NLA strips (UN-MUTED so they export)
nla_tracks = arm_obj.animation_data.nla_tracks
for action in bpy.data.actions:
    track = nla_tracks.new()
    track.name = action.name
    strip = track.strips.new(action.name, int(action.frame_range[0]), action)
    strip.action = action
    track.mute = False  # MUST be unmuted for glTF export

print(f"[GIBBONS] {len(bpy.data.actions)} actions pushed to NLA (unmuted)")

# ── EXPORT (raw Z-up — engine handles coord conversion) ──
export_path = "/Users/klaus/gibbons.glb"

ensure_object_mode(); bpy.ops.object.select_all(action='DESELECT')
gibbons_mesh.select_set(True)
arm_obj.select_set(True)
bpy.context.view_layer.objects.active = arm_obj

bpy.ops.export_scene.gltf(
    filepath=export_path,
    export_format='GLB',
    use_selection=True,
    export_apply=False,
    export_animations=True,
    export_skins=True,
    export_morph=False,
    export_yup=False,      # Raw Z-up — engine applies -90° X rotation
    export_nla_strips=True,
    export_nla_strips_merged_animation_name="",
)

print(f"[GIBBONS] Exported: {export_path}")

# ── VALIDATION ──
import os
file_size = os.path.getsize(export_path)
print(f"[GIBBONS] File size: {file_size / 1024:.1f} KB")
print(f"[GIBBONS] Tris: {tri_count}, Bones: {len(bone_names)}, Actions: {len(bpy.data.actions)}")
print(f"[GIBBONS] Animations: {sorted([a.name for a in bpy.data.actions])}")
print(f"[GIBBONS] ✓ BUILD COMPLETE")
