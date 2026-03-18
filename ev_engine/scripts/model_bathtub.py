#!/usr/bin/env python3
"""Blender script: Freestanding bathtub — porcelain, curved.
The shape boxes can never achieve. ~200 tris.
"""
import bpy
import math

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

mat_porcelain = mat("MAT_MARBLE", 240, 238, 232, roughness=0.2)
mat_brass = mat("MAT_BRASS", 184, 157, 107, metallic=0.9, roughness=0.3)
mat_inner = mat("MAT_TILE", 245, 242, 238, roughness=0.15)

# ── Tub body — elongated torus cut in half ──
# Outer shell
bpy.ops.mesh.primitive_cylinder_add(radius=0.4, depth=0.5, vertices=16,
                                     location=(0, 0.35, 0))
outer = bpy.context.active_object
outer.name = "Tub_Outer"
outer.scale = (1.8, 1, 1)
bpy.ops.object.transform_apply(scale=True)
# Subdivision for smoothness
mod = outer.modifiers.new("Subsurf", "SUBSURF")
mod.levels = 1
bpy.ops.object.modifier_apply(modifier="Subsurf")
outer.data.materials.append(mat_porcelain)

# ── Inner cavity (slightly smaller, darker) ──
bpy.ops.mesh.primitive_cylinder_add(radius=0.34, depth=0.42, vertices=16,
                                     location=(0, 0.38, 0))
inner = bpy.context.active_object
inner.name = "Tub_Inner"
inner.scale = (1.7, 1, 0.92)
bpy.ops.object.transform_apply(scale=True)
mod = inner.modifiers.new("Subsurf", "SUBSURF")
mod.levels = 1
bpy.ops.object.modifier_apply(modifier="Subsurf")
inner.data.materials.append(mat_inner)

# ── Rim — torus ring around top edge ──
bpy.ops.mesh.primitive_torus_add(major_radius=0.4, minor_radius=0.025,
                                  major_segments=16, minor_segments=6,
                                  location=(0, 0.6, 0))
rim = bpy.context.active_object
rim.name = "Tub_Rim"
rim.scale = (1.8, 1, 1)
bpy.ops.object.transform_apply(scale=True)
rim.data.materials.append(mat_porcelain)

# ── Feet — 4 claw feet (simple spheres) ──
feet_positions = [(-0.5, 0.06, -0.25), (0.5, 0.06, -0.25),
                  (-0.5, 0.06, 0.25), (0.5, 0.06, 0.25)]
for i, (fx, fy, fz) in enumerate(feet_positions):
    bpy.ops.mesh.primitive_uv_sphere_add(radius=0.04, segments=6, ring_count=4,
                                          location=(fx, fy, fz))
    foot = bpy.context.active_object
    foot.name = "Tub_Foot_" + str(i)
    foot.data.materials.append(mat_brass)

# ── Faucet — simple brass pipe ──
bpy.ops.mesh.primitive_cylinder_add(radius=0.015, depth=0.15, vertices=6,
                                     location=(0, 0.68, -0.3))
faucet_v = bpy.context.active_object
faucet_v.name = "Tub_Faucet_V"
faucet_v.data.materials.append(mat_brass)

# Spout
bpy.ops.mesh.primitive_cylinder_add(radius=0.012, depth=0.08, vertices=6,
                                     location=(0, 0.75, -0.28))
spout = bpy.context.active_object
spout.name = "Tub_Spout"
spout.rotation_euler[0] = math.radians(90)
bpy.ops.object.transform_apply(rotation=True)
spout.data.materials.append(mat_brass)

# ── Handles — two brass knobs ──
for side in [-0.04, 0.04]:
    bpy.ops.mesh.primitive_uv_sphere_add(radius=0.015, segments=6, ring_count=4,
                                          location=(side, 0.72, -0.32))
    knob = bpy.context.active_object
    knob.name = "Tub_Knob"
    knob.data.materials.append(mat_brass)

# ── Join all ──
bpy.ops.object.select_all(action='SELECT')
bpy.context.view_layer.objects.active = outer
bpy.ops.object.join()
tub = bpy.context.active_object
tub.name = "Bathtub"

total_verts = len(tub.data.vertices)
total_tris = sum(max(0, len(p.vertices)-2) for p in tub.data.polygons)
print("Bathtub: " + str(total_verts) + " verts, " + str(total_tris) + " tris")

bpy.ops.object.select_all(action='SELECT')
bpy.ops.wm.obj_export(
    filepath='/Users/klaus/bathtub.obj',
    export_selected_objects=True,
    apply_modifiers=True,
    export_normals=True,
    export_colors=True,
)
print("Exported to /Users/klaus/bathtub.obj")
