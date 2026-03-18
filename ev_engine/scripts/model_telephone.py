#!/usr/bin/env python3
"""Blender script: Vintage rotary telephone.
Brass body, dark handset. ~120 tris. The phone that rings unanswered.
Export as .obj (static prop).
"""
import bpy
import math

# Clear scene
bpy.ops.object.select_all(action='SELECT')
bpy.ops.object.delete()

# ── Materials ──
def mat(name, r, g, b, metallic=0, roughness=0.5):
    m = bpy.data.materials.new(name)
    m.use_nodes = True
    bsdf = m.node_tree.nodes["Principled BSDF"]
    bsdf.inputs["Base Color"].default_value = (r/255, g/255, b/255, 1.0)
    bsdf.inputs["Metallic"].default_value = metallic
    bsdf.inputs["Roughness"].default_value = roughness
    return m

mat_brass = mat("MAT_BRASS", 184, 157, 107, metallic=0.9, roughness=0.3)
mat_dark = mat("MAT_LEATHER", 35, 30, 25, metallic=0, roughness=0.7)
mat_dial = mat("MAT_CONCRETE", 200, 195, 185, metallic=0, roughness=0.6)

# ── Body ──
# Main body — squashed sphere (rotary phone base)
bpy.ops.mesh.primitive_uv_sphere_add(radius=0.08, segments=10, ring_count=6,
                                      location=(0, 0, 0.04))
body = bpy.context.active_object
body.name = "Phone_Body"
body.scale = (1.0, 1.0, 0.5)
bpy.ops.object.transform_apply(scale=True)
body.data.materials.append(mat_brass)

# Rotary dial on top — flat cylinder
bpy.ops.mesh.primitive_cylinder_add(radius=0.045, depth=0.008, vertices=10,
                                     location=(0, 0, 0.065))
dial = bpy.context.active_object
dial.name = "Phone_Dial"
dial.data.materials.append(mat_dial)

# Dial finger stop
bpy.ops.mesh.primitive_cylinder_add(radius=0.005, depth=0.012, vertices=6,
                                     location=(0.035, 0, 0.07))
stop = bpy.context.active_object
stop.name = "Phone_Stop"
stop.data.materials.append(mat_brass)

# ── Handset cradle ──
# Two prongs
for side in [-1, 1]:
    bpy.ops.mesh.primitive_cylinder_add(radius=0.006, depth=0.03, vertices=6,
                                         location=(side * 0.04, 0, 0.075))
    prong = bpy.context.active_object
    prong.name = "Phone_Prong_" + ("L" if side < 0 else "R")
    prong.data.materials.append(mat_brass)

# ── Handset (resting in cradle) ──
# Earpiece
bpy.ops.mesh.primitive_cylinder_add(radius=0.018, depth=0.025, vertices=8,
                                     location=(-0.04, 0, 0.1))
ear = bpy.context.active_object
ear.name = "Phone_Ear"
ear.data.materials.append(mat_dark)

# Handle (connecting bar)
bpy.ops.mesh.primitive_cylinder_add(radius=0.008, depth=0.08, vertices=6,
                                     location=(0, 0, 0.1))
handle = bpy.context.active_object
handle.name = "Phone_Handle"
handle.rotation_euler[1] = math.radians(90)
bpy.ops.object.transform_apply(rotation=True)
handle.data.materials.append(mat_dark)

# Mouthpiece
bpy.ops.mesh.primitive_cylinder_add(radius=0.016, depth=0.025, vertices=8,
                                     location=(0.04, 0, 0.1))
mouth = bpy.context.active_object
mouth.name = "Phone_Mouth"
mouth.data.materials.append(mat_dark)

# ── Cord (simple curve) ──
bpy.ops.mesh.primitive_cylinder_add(radius=0.003, depth=0.06, vertices=4,
                                     location=(-0.05, 0.02, 0.04))
cord = bpy.context.active_object
cord.name = "Phone_Cord"
cord.rotation_euler[0] = math.radians(60)
bpy.ops.object.transform_apply(rotation=True)
cord.data.materials.append(mat_dark)

# ── Join all ──
bpy.ops.object.select_all(action='SELECT')
bpy.context.view_layer.objects.active = body
bpy.ops.object.join()
phone = bpy.context.active_object
phone.name = "Telephone"

# Count
total_verts = len(phone.data.vertices)
total_tris = sum(max(0, len(p.vertices)-2) for p in phone.data.polygons)
print("Telephone: " + str(total_verts) + " verts, " + str(total_tris) + " tris")

# Export
bpy.ops.object.select_all(action='SELECT')
bpy.ops.wm.obj_export(
    filepath='/Users/klaus/telephone.obj',
    export_selected_objects=True,
    apply_modifiers=True,
    export_normals=True,
    export_colors=True,
)
print("Exported to /Users/klaus/telephone.obj")
