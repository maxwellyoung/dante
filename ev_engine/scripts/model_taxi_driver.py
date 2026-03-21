#!/usr/bin/env python3
"""Blender script: Taxi driver — viewed from backseat.
Flat cap, dark coat, hands on wheel. Auckland at 2AM.
Single mesh, export as GLB (avoids OBJ VAO bus error).
~200 tris target.

AXIS CONVENTION: Blender Z-up, Y-forward. export_yup handles conversion.
All locations use (X, Y_forward, Z_height) — Blender native.
"""
import os

import bpy
import math

# Run style bible first
import os
style_path = '/Users/maxwellyoung/Development/dante/assets/blender/ev_style.py'
if os.path.exists(style_path):
    exec(open(style_path).read())
    ev_setup('taxi')

# Clear scene
bpy.ops.object.select_all(action='SELECT')
bpy.ops.object.delete()
# Clear orphan data
for block in bpy.data.meshes:
    if block.users == 0: bpy.data.meshes.remove(block)

def mat(name, r, g, b, metallic=0, roughness=0.5, subsurface=0):
    m = bpy.data.materials.new(name)
    m.use_nodes = True
    bsdf = m.node_tree.nodes["Principled BSDF"]
    bsdf.inputs["Base Color"].default_value = (r/255, g/255, b/255, 1.0)
    bsdf.inputs["Metallic"].default_value = metallic
    bsdf.inputs["Roughness"].default_value = roughness
    return m

mat_coat = mat("Driver_Coat", 40, 36, 30, roughness=0.8)
mat_skin = mat("Driver_Skin", 205, 180, 145, roughness=0.6, subsurface=0.1)
mat_hair = mat("Driver_Hair", 50, 42, 35, roughness=0.9)
mat_cap = mat("Driver_Cap", 42, 40, 36, roughness=0.75)
mat_collar = mat("Driver_Collar", 225, 220, 210, roughness=0.7)
mat_watch = mat("Driver_Watch", 210, 180, 100, metallic=0.8, roughness=0.3)

# Driver seat position in engine coords: x=+0.4, y(height)=~0.84, z(depth)=-0.88
# Blender coords (Z-up, -Y=forward): X=+0.4, Y=+0.88 (forward toward windshield), Z=height
# Note: engine -Z maps to Blender -Y (forward), so engine dz=-0.88 → Blender Y=+0.88
dx = 0.4   # X stays X
dy = 0.88   # engine Z=-0.88 → Blender Y=+0.88 (toward front of car)
# Z = height (was Y in the old wrong script)

def loc(x, height, depth_offset=0):
    """Convert engine-think (x, height, depth_from_seat) to Blender (X, Y, Z).
    depth_offset: negative = toward windshield, positive = toward player."""
    return (x, dy - depth_offset, height)

def scl(sx, sh, sd):
    """Convert engine-think scale (width, height, depth) to Blender (X, Y, Z)."""
    return (sx, sd, sh)

# ── TORSO — the main mass, seen from behind ──
bpy.ops.mesh.primitive_cube_add(size=1, location=loc(dx, 0.84))
torso = bpy.context.active_object
torso.name = "Driver_Torso"
torso.scale = scl(0.36, 0.46, 0.26)
bpy.ops.object.transform_apply(scale=True)
torso.data.materials.append(mat_coat)

# ── SHOULDERS — wider than torso ──
bpy.ops.mesh.primitive_cube_add(size=1, location=loc(dx, 0.96))
shoulders = bpy.context.active_object
shoulders.name = "Driver_Shoulders"
shoulders.scale = scl(0.48, 0.12, 0.24)
bpy.ops.object.transform_apply(scale=True)
shoulders.data.materials.append(mat_coat)

# ── COLLAR — white V visible from behind ──
bpy.ops.mesh.primitive_cube_add(size=1, location=loc(dx, 1.03, -0.02))
collar = bpy.context.active_object
collar.name = "Driver_Collar"
collar.scale = scl(0.26, 0.08, 0.18)
bpy.ops.object.transform_apply(scale=True)
collar.data.materials.append(mat_collar)

# ── NECK ──
bpy.ops.mesh.primitive_cylinder_add(radius=0.07, depth=0.10, vertices=8,
                                     location=loc(dx, 1.08, -0.01))
neck = bpy.context.active_object
neck.name = "Driver_Neck"
neck.data.materials.append(mat_skin)

# ── HEAD — the key silhouette piece ──
bpy.ops.mesh.primitive_cube_add(size=1, location=loc(dx, 1.20, -0.02))
head = bpy.context.active_object
head.name = "Driver_Head"
head.scale = scl(0.21, 0.23, 0.21)
bpy.ops.object.transform_apply(scale=True)
head.data.materials.append(mat_skin)

# ── EARS ──
for side in [-1, 1]:
    bpy.ops.mesh.primitive_cube_add(size=1,
        location=loc(dx + side * 0.13, 1.18, -0.02))
    ear = bpy.context.active_object
    ear.name = "Driver_Ear"
    ear.scale = scl(0.04, 0.08, 0.06)
    bpy.ops.object.transform_apply(scale=True)
    ear.data.materials.append(mat_skin)

# ── HAIR — dark patch on back of head (player sees this most) ──
bpy.ops.mesh.primitive_cube_add(size=1, location=loc(dx, 1.22, -0.11))
hair = bpy.context.active_object
hair.name = "Driver_Hair"
hair.scale = scl(0.20, 0.14, 0.10)
bpy.ops.object.transform_apply(scale=True)
hair.data.materials.append(mat_hair)

# ── FLAT CAP — the signature ──
bpy.ops.mesh.primitive_cylinder_add(radius=0.15, depth=0.07, vertices=10,
                                     location=loc(dx, 1.35, -0.02))
cap_body = bpy.context.active_object
cap_body.name = "Driver_CapBody"
cap_body.scale = (1.05, 0.95, 0.85)
bpy.ops.object.transform_apply(scale=True)
cap_body.data.materials.append(mat_cap)

# Cap brim — extends forward (toward windshield = negative depth_offset)
bpy.ops.mesh.primitive_cube_add(size=1, location=loc(dx, 1.33, 0.12))
brim = bpy.context.active_object
brim.name = "Driver_Brim"
brim.scale = scl(0.32, 0.025, 0.16)
bpy.ops.object.transform_apply(scale=True)
brim.data.materials.append(mat_cap)

# ── LEFT ARM — reaching to wheel ──
# Upper arm
bpy.ops.mesh.primitive_cube_add(size=1, location=loc(dx - 0.22, 0.84, 0.05))
l_upper = bpy.context.active_object
l_upper.name = "Driver_LUpperArm"
l_upper.scale = scl(0.13, 0.28, 0.13)
bpy.ops.object.transform_apply(scale=True)
l_upper.data.materials.append(mat_coat)

# Forearm — reaching forward toward wheel
bpy.ops.mesh.primitive_cube_add(size=1, location=loc(dx - 0.20, 0.76, 0.28))
l_fore = bpy.context.active_object
l_fore.name = "Driver_LForearm"
l_fore.scale = scl(0.11, 0.14, 0.22)
bpy.ops.object.transform_apply(scale=True)
l_fore.data.materials.append(mat_coat)

# Left hand on wheel
bpy.ops.mesh.primitive_cube_add(size=1, location=loc(dx - 0.16, 0.74, 0.42))
l_hand = bpy.context.active_object
l_hand.name = "Driver_LHand"
l_hand.scale = scl(0.10, 0.07, 0.08)
bpy.ops.object.transform_apply(scale=True)
l_hand.data.materials.append(mat_skin)

# Watch — brass glint
bpy.ops.mesh.primitive_cube_add(size=1, location=loc(dx - 0.19, 0.74, 0.34))
watch = bpy.context.active_object
watch.name = "Driver_Watch"
watch.scale = scl(0.04, 0.03, 0.06)
bpy.ops.object.transform_apply(scale=True)
watch.data.materials.append(mat_watch)

# ── RIGHT ARM — resting ──
bpy.ops.mesh.primitive_cube_add(size=1, location=loc(dx + 0.22, 0.84, -0.02))
r_upper = bpy.context.active_object
r_upper.name = "Driver_RUpperArm"
r_upper.scale = scl(0.13, 0.28, 0.13)
bpy.ops.object.transform_apply(scale=True)
r_upper.data.materials.append(mat_coat)

bpy.ops.mesh.primitive_cube_add(size=1, location=loc(dx + 0.24, 0.70, -0.08))
r_fore = bpy.context.active_object
r_fore.name = "Driver_RForearm"
r_fore.scale = scl(0.11, 0.22, 0.11)
bpy.ops.object.transform_apply(scale=True)
r_fore.data.materials.append(mat_coat)

bpy.ops.mesh.primitive_cube_add(size=1, location=loc(dx + 0.24, 0.56, -0.08))
r_hand = bpy.context.active_object
r_hand.name = "Driver_RHand"
r_hand.scale = scl(0.10, 0.07, 0.08)
bpy.ops.object.transform_apply(scale=True)
r_hand.data.materials.append(mat_skin)

# ── CENTER SEAM — visible from behind ──
bpy.ops.mesh.primitive_cube_add(size=1, location=loc(dx, 0.80, -0.15))
seam = bpy.context.active_object
seam.name = "Driver_Seam"
seam.scale = scl(0.02, 0.40, 0.02)
bpy.ops.object.transform_apply(scale=True)
seam.data.materials.append(mat_hair)  # dark line

# ── JOIN ALL into single mesh ──
bpy.ops.object.select_all(action='SELECT')
# Deselect non-mesh objects (lights, cameras from ev_setup)
for obj in bpy.data.objects:
    if obj.type != 'MESH':
        obj.select_set(False)

meshes = [o for o in bpy.data.objects if o.type == 'MESH']
if meshes:
    bpy.context.view_layer.objects.active = meshes[0]
    bpy.ops.object.join()
    driver = bpy.context.active_object
    driver.name = "TaxiDriver"

    # Apply all transforms
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)

    total_verts = len(driver.data.vertices)
    total_tris = sum(max(0, len(p.vertices)-2) for p in driver.data.polygons)
    print(f"TaxiDriver: {total_verts} verts, {total_tris} tris")

    # Select only the driver for export
    bpy.ops.object.select_all(action='DESELECT')
    driver.select_set(True)
    bpy.context.view_layer.objects.active = driver

    # Export as GLB — export_yup converts Blender Z-up to Raylib Y-up
    export_path = os.environ.get("EV_EXPORT_PATH", '/Users/klaus/taxi_driver.glb')
    bpy.ops.export_scene.gltf(
        filepath=export_path,
        export_format='GLB',
        use_selection=True,
        export_apply=True,
        export_animations=False,
        export_yup=True,
    )
    print(f"Exported to {export_path}")
    print("Run: ./scripts/mcp_model.sh full scripts/model_taxi_driver.py taxi_driver")
