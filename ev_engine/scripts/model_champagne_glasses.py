#!/usr/bin/env python3
"""Blender script: Two champagne flutes on a brass tray.
One half-full (golden liquid), one empty. The washing line moment.
~80 tris total. Export as .obj (static prop).

Run via: scp to mini-ts, then blender_send.py this_file.py
"""
import bpy
import math

# Clear scene
bpy.ops.object.select_all(action='SELECT')
bpy.ops.object.delete()

# ── Materials ──
def mat(name, r, g, b, a=1.0, metallic=0, roughness=0.5):
    m = bpy.data.materials.new(name)
    m.use_nodes = True
    bsdf = m.node_tree.nodes["Principled BSDF"]
    bsdf.inputs["Base Color"].default_value = (r/255, g/255, b/255, a)
    bsdf.inputs["Metallic"].default_value = metallic
    bsdf.inputs["Roughness"].default_value = roughness
    return m

mat_glass = mat("MAT_GLASS", 210, 215, 220, 0.4, metallic=0, roughness=0.1)
mat_brass = mat("MAT_BRASS", 184, 157, 107, 1.0, metallic=0.9, roughness=0.3)
mat_gold_liquid = mat("MAT_GOLD", 218, 175, 95, 0.8, metallic=0, roughness=0.2)
mat_lipstick = mat("MAT_RED", 180, 45, 55, 1.0, metallic=0, roughness=0.6)

# ── Tray ──
bpy.ops.mesh.primitive_cylinder_add(radius=0.25, depth=0.02, vertices=12, location=(0, 0, 0.01))
tray = bpy.context.active_object
tray.name = "Tray"
tray.data.materials.append(mat_brass)

# Tray rim
bpy.ops.mesh.primitive_torus_add(major_radius=0.25, minor_radius=0.008,
                                  major_segments=12, minor_segments=4,
                                  location=(0, 0, 0.02))
rim = bpy.context.active_object
rim.name = "TrayRim"
rim.data.materials.append(mat_brass)

# ── Champagne Flute helper ──
def make_flute(name, x, y, has_liquid=False, tilt_deg=0):
    # Base (flat disc)
    bpy.ops.mesh.primitive_cylinder_add(radius=0.025, depth=0.003, vertices=8,
                                         location=(x, y, 0.025))
    base = bpy.context.active_object
    base.name = name + "_Base"
    base.data.materials.append(mat_glass)

    # Stem
    bpy.ops.mesh.primitive_cylinder_add(radius=0.004, depth=0.08, vertices=6,
                                         location=(x, y, 0.065))
    stem = bpy.context.active_object
    stem.name = name + "_Stem"
    stem.data.materials.append(mat_glass)

    # Bowl (cone frustum — wide at top, narrow at bottom)
    bpy.ops.mesh.primitive_cone_add(radius1=0.008, radius2=0.025, depth=0.06,
                                     vertices=8, location=(x, y, 0.135))
    bowl = bpy.context.active_object
    bowl.name = name + "_Bowl"
    bowl.data.materials.append(mat_glass)

    # Liquid inside (if half-full)
    if has_liquid:
        bpy.ops.mesh.primitive_cone_add(radius1=0.006, radius2=0.018, depth=0.035,
                                         vertices=8, location=(x, y, 0.12))
        liquid = bpy.context.active_object
        liquid.name = name + "_Liquid"
        liquid.data.materials.append(mat_gold_liquid)

    # Select all parts and join
    bpy.ops.object.select_all(action='DESELECT')
    for obj in bpy.data.objects:
        if obj.name.startswith(name + "_"):
            obj.select_set(True)
    bpy.context.view_layer.objects.active = base
    bpy.ops.object.join()
    flute = bpy.context.active_object
    flute.name = name

    # Apply tilt
    if tilt_deg != 0:
        flute.rotation_euler[0] = math.radians(tilt_deg)

    return flute

# ── Glass 1: half-full (yours) ──
glass1 = make_flute("Glass_Full", -0.06, 0, has_liquid=True)

# ── Glass 2: empty, slight tilt (hers — untouched) ──
glass2 = make_flute("Glass_Empty", 0.06, 0.02, has_liquid=False, tilt_deg=5)

# ── Wine glass with lipstick mark (separate piece) ──
bpy.ops.mesh.primitive_cylinder_add(radius=0.025, depth=0.003, vertices=8,
                                     location=(0.12, -0.06, 0.025))
wbase = bpy.context.active_object
wbase.name = "Wine_Base"
wbase.data.materials.append(mat_glass)

bpy.ops.mesh.primitive_cylinder_add(radius=0.004, depth=0.06, vertices=6,
                                     location=(0.12, -0.06, 0.055))
wstem = bpy.context.active_object
wstem.name = "Wine_Stem"
wstem.data.materials.append(mat_glass)

# Wine glass bowl (wider than flute)
bpy.ops.mesh.primitive_uv_sphere_add(radius=0.03, segments=8, ring_count=6,
                                      location=(0.12, -0.06, 0.11))
wbowl = bpy.context.active_object
wbowl.name = "Wine_Bowl"
# Cut top half — scale Z to squash into half-sphere
wbowl.scale[2] = 0.7
bpy.ops.object.transform_apply(scale=True)
wbowl.data.materials.append(mat_glass)

# Lipstick mark on rim — small red crescent
bpy.ops.mesh.primitive_cube_add(size=0.01, location=(0.14, -0.06, 0.135))
lip = bpy.context.active_object
lip.name = "Wine_Lipstick"
lip.scale = (1.5, 0.5, 0.3)
bpy.ops.object.transform_apply(scale=True)
lip.data.materials.append(mat_lipstick)

# Join wine glass parts
bpy.ops.object.select_all(action='DESELECT')
for obj in bpy.data.objects:
    if obj.name.startswith("Wine_"):
        obj.select_set(True)
bpy.context.view_layer.objects.active = wbase
bpy.ops.object.join()
wine = bpy.context.active_object
wine.name = "WineGlass_Lipstick"

# ── Export ──
# Select all for export
bpy.ops.object.select_all(action='SELECT')

# Count final geometry
total_verts = 0
total_tris = 0
for obj in bpy.data.objects:
    if obj.type == 'MESH':
        total_verts += len(obj.data.vertices)
        total_tris += sum(max(0, len(p.vertices)-2) for p in obj.data.polygons)
print("Champagne glasses: " + str(total_verts) + " verts, " + str(total_tris) + " tris")

# Export OBJ (static, no animation)
bpy.ops.wm.obj_export(
    filepath='/Users/klaus/champagne_glasses.obj',
    export_selected_objects=True,
    apply_modifiers=True,
    export_normals=True,
    export_colors=True,
)
print("Exported to /Users/klaus/champagne_glasses.obj")
