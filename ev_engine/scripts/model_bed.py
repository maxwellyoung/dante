#!/usr/bin/env python3
"""Blender script: Hotel bed with headboard, pillows, duvet.
The emotional endpoint — 60+ seconds of staring during the bed ritual.
~300 tris. Export as .obj (static).
"""
import bpy

bpy.ops.object.select_all(action='SELECT')
bpy.ops.object.delete()

def mat(name, r, g, b, metallic=0, roughness=0.5):
    m = bpy.data.materials.new(name)
    m.use_nodes = True
    bsdf = m.node_tree.nodes["Principled BSDF"]
    bsdf.inputs["Base Color"].default_value = (r/255, g/255, b/255, 1.0)
    bsdf.inputs["Metallic"].default_value = metallic
    bsdf.inputs["Roughness"].default_value = roughness
    return m

mat_wood = mat("MAT_WOOD", 105, 78, 48, roughness=0.7)
mat_fabric = mat("MAT_FABRIC", 245, 242, 235, roughness=0.9)
mat_velvet = mat("MAT_VELVET", 25, 25, 80, roughness=0.85)
mat_brass = mat("MAT_BRASS", 184, 157, 107, metallic=0.9, roughness=0.3)
mat_cream = mat("MAT_CREAM", 215, 210, 200, roughness=0.8)

# ── Bed frame ──
bpy.ops.mesh.primitive_cube_add(size=1, location=(0, 0.18, 0))
frame = bpy.context.active_object
frame.name = "Bed_Frame"
frame.scale = (3.4, 0.36, 2.2)
bpy.ops.object.transform_apply(scale=True)
frame.data.materials.append(mat_wood)

# ── Mattress ── (slightly rounded top via subdivision)
bpy.ops.mesh.primitive_cube_add(size=1, location=(0, 0.48, 0))
mattress = bpy.context.active_object
mattress.name = "Bed_Mattress"
mattress.scale = (3.2, 0.28, 2.0)
bpy.ops.object.transform_apply(scale=True)
# Add subtle subdivision for softness
mod = mattress.modifiers.new("Subsurf", "SUBSURF")
mod.levels = 1
mod.render_levels = 1
bpy.ops.object.modifier_apply(modifier="Subsurf")
mattress.data.materials.append(mat_fabric)

# ── Duvet ── (slightly rumpled — displaced top face)
bpy.ops.mesh.primitive_cube_add(size=1, location=(0, 0.66, 0.15))
duvet = bpy.context.active_object
duvet.name = "Bed_Duvet"
duvet.scale = (3.0, 0.06, 1.4)
bpy.ops.object.transform_apply(scale=True)
mod = duvet.modifiers.new("Subsurf", "SUBSURF")
mod.levels = 2
bpy.ops.object.modifier_apply(modifier="Subsurf")
duvet.data.materials.append(mat_cream)

# ── Folded edge ──
bpy.ops.mesh.primitive_cube_add(size=1, location=(0, 0.68, -0.6))
fold = bpy.context.active_object
fold.name = "Bed_Fold"
fold.scale = (3.0, 0.04, 0.3)
bpy.ops.object.transform_apply(scale=True)
fold.data.materials.append(mat_cream)

# ── TWO PILLOWS ──
for side in [-0.6, 0.6]:
    bpy.ops.mesh.primitive_cube_add(size=1, location=(side, 0.68, -1.0))
    pillow = bpy.context.active_object
    pillow.name = "Bed_Pillow_" + ("L" if side < 0 else "R")
    pillow.scale = (0.65, 0.18, 0.4)
    bpy.ops.object.transform_apply(scale=True)
    mod = pillow.modifiers.new("Subsurf", "SUBSURF")
    mod.levels = 2
    bpy.ops.object.modifier_apply(modifier="Subsurf")
    pillow.data.materials.append(mat_fabric)

# ── Headboard — tall navy velvet panel ──
bpy.ops.mesh.primitive_cube_add(size=1, location=(0, 1.8, -1.35))
headboard = bpy.context.active_object
headboard.name = "Bed_Headboard"
headboard.scale = (3.6, 2.8, 0.12)
bpy.ops.object.transform_apply(scale=True)
# Slight curve on top edge via subdivision
mod = headboard.modifiers.new("Subsurf", "SUBSURF")
mod.levels = 1
bpy.ops.object.modifier_apply(modifier="Subsurf")
headboard.data.materials.append(mat_velvet)

# ── Brass cap on headboard ──
bpy.ops.mesh.primitive_cube_add(size=1, location=(0, 3.25, -1.32))
cap = bpy.context.active_object
cap.name = "Bed_Cap"
cap.scale = (3.7, 0.06, 0.08)
bpy.ops.object.transform_apply(scale=True)
cap.data.materials.append(mat_brass)

# ── Join all ──
bpy.ops.object.select_all(action='SELECT')
bpy.context.view_layer.objects.active = frame
bpy.ops.object.join()
bed = bpy.context.active_object
bed.name = "Bed"

total_verts = len(bed.data.vertices)
total_tris = sum(max(0, len(p.vertices)-2) for p in bed.data.polygons)
print("Bed: " + str(total_verts) + " verts, " + str(total_tris) + " tris")

bpy.ops.object.select_all(action='SELECT')
bpy.ops.wm.obj_export(
    filepath='/Users/klaus/bed.obj',
    export_selected_objects=True,
    apply_modifiers=True,
    export_normals=True,
    export_colors=True,
)
print("Exported to /Users/klaus/bed.obj")
