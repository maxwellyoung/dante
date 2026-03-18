#!/usr/bin/env python3
"""Blender script: Taxi driver — viewed from backseat.
Flat cap, dark coat, hands on wheel. Auckland at 2AM.
Single mesh, export as GLB (avoids OBJ VAO bus error).
~200 tris target.
"""
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

# Driver sits at x=-0.4, z=-0.88 (matching build_taxi_ride positions)
dx, dz = -0.4, -0.88

# ── TORSO — the main mass, seen from behind ──
bpy.ops.mesh.primitive_cube_add(size=1, location=(dx, 0.84, dz))
torso = bpy.context.active_object
torso.name = "Driver_Torso"
torso.scale = (0.44, 0.50, 0.30)
bpy.ops.object.transform_apply(scale=True)
mod = torso.modifiers.new("Subsurf", "SUBSURF")
mod.levels = 1
bpy.ops.object.modifier_apply(modifier="Subsurf")
torso.data.materials.append(mat_coat)

# ── SHOULDERS — wider than torso ──
bpy.ops.mesh.primitive_cube_add(size=1, location=(dx, 0.96, dz))
shoulders = bpy.context.active_object
shoulders.name = "Driver_Shoulders"
shoulders.scale = (0.58, 0.12, 0.30)
bpy.ops.object.transform_apply(scale=True)
mod = shoulders.modifiers.new("Subsurf", "SUBSURF")
mod.levels = 1
bpy.ops.object.modifier_apply(modifier="Subsurf")
shoulders.data.materials.append(mat_coat)

# ── COLLAR — white V visible from behind ──
bpy.ops.mesh.primitive_cube_add(size=1, location=(dx, 1.03, dz + 0.02))
collar = bpy.context.active_object
collar.name = "Driver_Collar"
collar.scale = (0.26, 0.08, 0.18)
bpy.ops.object.transform_apply(scale=True)
collar.data.materials.append(mat_collar)

# ── NECK ──
bpy.ops.mesh.primitive_cylinder_add(radius=0.07, depth=0.10, vertices=8,
                                     location=(dx, 1.08, dz + 0.01))
neck = bpy.context.active_object
neck.name = "Driver_Neck"
neck.data.materials.append(mat_skin)

# ── HEAD — the key silhouette piece ──
bpy.ops.mesh.primitive_cube_add(size=1, location=(dx, 1.20, dz + 0.02))
head = bpy.context.active_object
head.name = "Driver_Head"
head.scale = (0.24, 0.26, 0.24)
bpy.ops.object.transform_apply(scale=True)
mod = head.modifiers.new("Subsurf", "SUBSURF")
mod.levels = 1
bpy.ops.object.modifier_apply(modifier="Subsurf")
head.data.materials.append(mat_skin)

# ── EARS ──
for side in [-1, 1]:
    bpy.ops.mesh.primitive_cube_add(size=1,
        location=(dx + side * 0.13, 1.18, dz + 0.02))
    ear = bpy.context.active_object
    ear.name = "Driver_Ear"
    ear.scale = (0.04, 0.08, 0.06)
    bpy.ops.object.transform_apply(scale=True)
    ear.data.materials.append(mat_skin)

# ── HAIR — dark patch on back of head (player sees this most) ──
bpy.ops.mesh.primitive_cube_add(size=1, location=(dx, 1.22, dz + 0.11))
hair = bpy.context.active_object
hair.name = "Driver_Hair"
hair.scale = (0.22, 0.18, 0.08)
bpy.ops.object.transform_apply(scale=True)
hair.data.materials.append(mat_hair)

# ── FLAT CAP — the signature ──
bpy.ops.mesh.primitive_cylinder_add(radius=0.15, depth=0.07, vertices=10,
                                     location=(dx, 1.35, dz + 0.02))
cap_body = bpy.context.active_object
cap_body.name = "Driver_CapBody"
cap_body.data.materials.append(mat_cap)

# Cap brim — extends forward
bpy.ops.mesh.primitive_cube_add(size=1, location=(dx, 1.33, dz - 0.12))
brim = bpy.context.active_object
brim.name = "Driver_Brim"
brim.scale = (0.28, 0.025, 0.14)
bpy.ops.object.transform_apply(scale=True)
brim.data.materials.append(mat_cap)

# ── LEFT ARM — reaching to wheel ──
# Upper arm
bpy.ops.mesh.primitive_cube_add(size=1, location=(dx - 0.22, 0.84, dz - 0.05))
l_upper = bpy.context.active_object
l_upper.name = "Driver_LUpperArm"
l_upper.scale = (0.13, 0.28, 0.13)
bpy.ops.object.transform_apply(scale=True)
l_upper.data.materials.append(mat_coat)

# Forearm — reaching forward
bpy.ops.mesh.primitive_cube_add(size=1, location=(dx - 0.20, 0.76, dz - 0.28))
l_fore = bpy.context.active_object
l_fore.name = "Driver_LForearm"
l_fore.scale = (0.11, 0.14, 0.22)
bpy.ops.object.transform_apply(scale=True)
l_fore.data.materials.append(mat_coat)

# Left hand on wheel
bpy.ops.mesh.primitive_cube_add(size=1, location=(dx - 0.16, 0.74, dz - 0.42))
l_hand = bpy.context.active_object
l_hand.name = "Driver_LHand"
l_hand.scale = (0.10, 0.07, 0.08)
bpy.ops.object.transform_apply(scale=True)
l_hand.data.materials.append(mat_skin)

# Watch — brass glint
bpy.ops.mesh.primitive_cube_add(size=1, location=(dx - 0.19, 0.74, dz - 0.34))
watch = bpy.context.active_object
watch.name = "Driver_Watch"
watch.scale = (0.04, 0.03, 0.06)
bpy.ops.object.transform_apply(scale=True)
watch.data.materials.append(mat_watch)

# ── RIGHT ARM — resting ──
bpy.ops.mesh.primitive_cube_add(size=1, location=(dx + 0.22, 0.84, dz + 0.02))
r_upper = bpy.context.active_object
r_upper.name = "Driver_RUpperArm"
r_upper.scale = (0.13, 0.28, 0.13)
bpy.ops.object.transform_apply(scale=True)
r_upper.data.materials.append(mat_coat)

bpy.ops.mesh.primitive_cube_add(size=1, location=(dx + 0.24, 0.70, dz + 0.08))
r_fore = bpy.context.active_object
r_fore.name = "Driver_RForearm"
r_fore.scale = (0.11, 0.22, 0.11)
bpy.ops.object.transform_apply(scale=True)
r_fore.data.materials.append(mat_coat)

bpy.ops.mesh.primitive_cube_add(size=1, location=(dx + 0.24, 0.56, dz + 0.08))
r_hand = bpy.context.active_object
r_hand.name = "Driver_RHand"
r_hand.scale = (0.10, 0.07, 0.08)
bpy.ops.object.transform_apply(scale=True)
r_hand.data.materials.append(mat_skin)

# ── CENTER SEAM — visible from behind ──
bpy.ops.mesh.primitive_cube_add(size=1, location=(dx, 0.80, dz + 0.15))
seam = bpy.context.active_object
seam.name = "Driver_Seam"
seam.scale = (0.02, 0.40, 0.02)
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
    print("TaxiDriver: " + str(total_verts) + " verts, " + str(total_tris) + " tris")

    # Select only the driver for export
    bpy.ops.object.select_all(action='DESELECT')
    driver.select_set(True)
    bpy.context.view_layer.objects.active = driver

    # Export as GLB (single file, avoids OBJ VAO bus error)
    bpy.ops.export_scene.gltf(
        filepath='/Users/klaus/taxi_driver.glb',
        export_format='GLB',
        use_selection=True,
        export_apply=True,
        export_animations=False,
        export_yup=True,
    )
    print("Exported to /Users/klaus/taxi_driver.glb")
