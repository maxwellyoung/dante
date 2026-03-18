#!/usr/bin/env python3
"""Blender script: Grand piano — lid propped, bench.
Lobby centrepiece. Through-wall muffled piano comes from here. ~250 tris.
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

mat_body = mat("MAT_WOOD", 20, 18, 15, roughness=0.3)  # glossy black
mat_keys_w = mat("MAT_MARBLE", 240, 238, 232, roughness=0.4)
mat_keys_b = mat("MAT_LEATHER", 15, 12, 10, roughness=0.5)
mat_brass = mat("MAT_BRASS", 184, 157, 107, metallic=0.9, roughness=0.3)
mat_bench = mat("MAT_VELVET", 128, 0, 32, roughness=0.85)  # burgundy velvet

# ── Piano body — simplified grand piano silhouette ──
# Main case — elongated curved shape (approximated with scaled cube + subsurf)
bpy.ops.mesh.primitive_cube_add(size=1, location=(0, 0.5, 0))
body = bpy.context.active_object
body.name = "Piano_Body"
body.scale = (1.5, 0.3, 1.0)
bpy.ops.object.transform_apply(scale=True)
mod = body.modifiers.new("Subsurf", "SUBSURF")
mod.levels = 1
bpy.ops.object.modifier_apply(modifier="Subsurf")
body.data.materials.append(mat_body)

# Curved tail (wider at back) — additional wedge
bpy.ops.mesh.primitive_cube_add(size=1, location=(-0.3, 0.5, 0.4))
tail = bpy.context.active_object
tail.name = "Piano_Tail"
tail.scale = (0.8, 0.28, 0.3)
bpy.ops.object.transform_apply(scale=True)
tail.data.materials.append(mat_body)

# ── Legs — 3 turned legs ──
leg_positions = [(-0.55, 0.18, -0.35), (0.55, 0.18, -0.35), (-0.2, 0.18, 0.5)]
for i, (lx, ly, lz) in enumerate(leg_positions):
    bpy.ops.mesh.primitive_cylinder_add(radius=0.03, depth=0.36, vertices=8,
                                         location=(lx, ly, lz))
    leg = bpy.context.active_object
    leg.name = "Piano_Leg_" + str(i)
    leg.data.materials.append(mat_body)
    # Brass caster at bottom
    bpy.ops.mesh.primitive_uv_sphere_add(radius=0.02, segments=6, ring_count=4,
                                          location=(lx, 0.02, lz))
    caster = bpy.context.active_object
    caster.name = "Piano_Caster_" + str(i)
    caster.data.materials.append(mat_brass)

# ── Lid — propped open at angle ──
bpy.ops.mesh.primitive_cube_add(size=1, location=(0, 0.85, 0.15))
lid = bpy.context.active_object
lid.name = "Piano_Lid"
lid.scale = (1.4, 0.02, 0.8)
bpy.ops.object.transform_apply(scale=True)
lid.rotation_euler[0] = math.radians(-25)  # propped open
bpy.ops.object.transform_apply(rotation=True)
lid.data.materials.append(mat_body)

# Lid prop stick
bpy.ops.mesh.primitive_cylinder_add(radius=0.005, depth=0.2, vertices=4,
                                     location=(0.3, 0.75, 0.1))
stick = bpy.context.active_object
stick.name = "Piano_Stick"
stick.rotation_euler[0] = math.radians(-15)
bpy.ops.object.transform_apply(rotation=True)
stick.data.materials.append(mat_brass)

# ── Keyboard — white and black keys ──
# White key bed
bpy.ops.mesh.primitive_cube_add(size=1, location=(0, 0.65, -0.45))
keys_w = bpy.context.active_object
keys_w.name = "Piano_KeysWhite"
keys_w.scale = (1.2, 0.02, 0.12)
bpy.ops.object.transform_apply(scale=True)
keys_w.data.materials.append(mat_keys_w)

# Black keys strip
bpy.ops.mesh.primitive_cube_add(size=1, location=(0, 0.67, -0.42))
keys_b = bpy.context.active_object
keys_b.name = "Piano_KeysBlack"
keys_b.scale = (1.1, 0.02, 0.05)
bpy.ops.object.transform_apply(scale=True)
keys_b.data.materials.append(mat_keys_b)

# ── Music stand ──
bpy.ops.mesh.primitive_cube_add(size=1, location=(0, 0.78, -0.38))
stand = bpy.context.active_object
stand.name = "Piano_MusicStand"
stand.scale = (0.6, 0.15, 0.01)
bpy.ops.object.transform_apply(scale=True)
stand.rotation_euler[0] = math.radians(-10)
bpy.ops.object.transform_apply(rotation=True)
stand.data.materials.append(mat_body)

# ── Pedals — 3 brass rectangles ──
for px in [-0.06, 0, 0.06]:
    bpy.ops.mesh.primitive_cube_add(size=1, location=(px, 0.02, -0.4))
    pedal = bpy.context.active_object
    pedal.name = "Piano_Pedal"
    pedal.scale = (0.03, 0.005, 0.08)
    bpy.ops.object.transform_apply(scale=True)
    pedal.data.materials.append(mat_brass)

# ── Bench — burgundy velvet top, dark legs ──
bpy.ops.mesh.primitive_cube_add(size=1, location=(0, 0.35, -0.75))
bench_top = bpy.context.active_object
bench_top.name = "Piano_BenchTop"
bench_top.scale = (0.8, 0.04, 0.3)
bpy.ops.object.transform_apply(scale=True)
bench_top.data.materials.append(mat_bench)

for bx, bz in [(-0.35, -0.85), (0.35, -0.85), (-0.35, -0.65), (0.35, -0.65)]:
    bpy.ops.mesh.primitive_cylinder_add(radius=0.015, depth=0.3, vertices=6,
                                         location=(bx, 0.18, bz))
    bleg = bpy.context.active_object
    bleg.name = "Piano_BenchLeg"
    bleg.data.materials.append(mat_body)

# ── Join all ──
bpy.ops.object.select_all(action='SELECT')
bpy.context.view_layer.objects.active = body
bpy.ops.object.join()
piano = bpy.context.active_object
piano.name = "Piano"

total_verts = len(piano.data.vertices)
total_tris = sum(max(0, len(p.vertices)-2) for p in piano.data.polygons)
print("Piano: " + str(total_verts) + " verts, " + str(total_tris) + " tris")

bpy.ops.object.select_all(action='SELECT')
bpy.ops.wm.obj_export(
    filepath='/Users/klaus/piano.obj',
    export_selected_objects=True,
    apply_modifiers=True,
    export_normals=True,
    export_colors=True,
)
print("Exported to /Users/klaus/piano.obj")
