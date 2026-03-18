"""
Batch builder: builds multiple models in sequence, exports each as GLB.
Each model is built fresh (scene cleared), exported, then the next begins.
Results are printed as a summary at the end.
"""
import bpy
import math
import os

results = []

def make_mat(name, r, g, b, metallic=0.0, roughness=0.7):
    mat = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    mat.use_nodes = True
    bsdf = mat.node_tree.nodes.get("Principled BSDF")
    if not bsdf:
        bsdf = mat.node_tree.nodes.new("ShaderNodeBsdfPrincipled")
    bsdf.inputs["Base Color"].default_value = (r/255, g/255, b/255, 1.0)
    bsdf.inputs["Metallic"].default_value = metallic
    bsdf.inputs["Roughness"].default_value = roughness
    return mat

def clear_scene():
    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.object.delete()

def join_and_export(parts, name, export_path):
    bpy.ops.object.select_all(action='DESELECT')
    for p in parts:
        p.select_set(True)
    bpy.context.view_layer.objects.active = parts[0]
    bpy.ops.object.join()
    final = bpy.context.active_object
    final.name = name
    bpy.ops.object.origin_set(type='ORIGIN_GEOMETRY', center='BOUNDS')

    tris = sum(max(0, len(p.vertices) - 2) for p in final.data.polygons)
    dims = final.dimensions

    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.export_scene.gltf(
        filepath=export_path,
        export_format='GLB',
        use_selection=True,
        export_apply=True,
        export_animations=False,
        export_yup=True,
    )
    size = os.path.getsize(export_path)
    results.append(f"  {name}: {tris} tris, {dims.x:.3f}x{dims.y:.3f}x{dims.z:.3f}m, {size/1024:.1f}KB")
    return final


# ═══════════════════════════════════════════════════════════
# MODEL 1: Photograph Frame
# ═══════════════════════════════════════════════════════════
clear_scene()
mat_brass = make_mat("EV_Brass_Dull", 160, 140, 95, metallic=0.7, roughness=0.45)
mat_glass = make_mat("EV_Glass_Warm", 220, 200, 180, roughness=0.08)

parts = []

# Frame
bpy.ops.mesh.primitive_cube_add(size=1, location=(0, 0, 0.08))
obj = bpy.context.active_object
obj.name = "PF_Frame"
obj.scale = (0.10, 0.005, 0.13)
bpy.ops.object.transform_apply(scale=True)
obj.data.materials.append(mat_brass)
parts.append(obj)

# Photo surface
bpy.ops.mesh.primitive_cube_add(size=1, location=(0, 0.001, 0.08))
obj = bpy.context.active_object
obj.name = "PF_Photo"
obj.scale = (0.085, 0.002, 0.115)
bpy.ops.object.transform_apply(scale=True)
obj.data.materials.append(mat_glass)
parts.append(obj)

# Stand
bpy.ops.mesh.primitive_cube_add(size=1, location=(0, -0.03, 0.06))
obj = bpy.context.active_object
obj.name = "PF_Stand"
obj.scale = (0.008, 0.04, 0.08)
bpy.ops.object.transform_apply(scale=True)
obj.rotation_euler = (math.radians(15), 0, 0)
bpy.ops.object.transform_apply(rotation=True)
obj.data.materials.append(mat_brass)
parts.append(obj)

join_and_export(parts, "PhotographFrame", "/tmp/photograph_frame.glb")


# ═══════════════════════════════════════════════════════════
# MODEL 2: Standing Ashtray
# ═══════════════════════════════════════════════════════════
clear_scene()
mat_brass2 = make_mat("EV_Brass", 184, 157, 107, metallic=0.9, roughness=0.3)
mat_marble = make_mat("EV_Marble_Dark", 60, 58, 55, roughness=0.2)
mat_grey = make_mat("EV_Fabric_Grey", 160, 155, 148, roughness=0.88)

parts = []

# Base
bpy.ops.mesh.primitive_cylinder_add(radius=0.12, depth=0.02, vertices=10, location=(0, 0, 0.01))
obj = bpy.context.active_object
obj.name = "SA_Base"
obj.data.materials.append(mat_marble)
parts.append(obj)

# Stem
bpy.ops.mesh.primitive_cylinder_add(radius=0.012, depth=0.65, vertices=6, location=(0, 0, 0.345))
obj = bpy.context.active_object
obj.name = "SA_Stem"
obj.data.materials.append(mat_brass2)
parts.append(obj)

# Tray
bpy.ops.mesh.primitive_cone_add(radius1=0.10, radius2=0.08, depth=0.04, vertices=10, location=(0, 0, 0.695))
obj = bpy.context.active_object
obj.name = "SA_Tray"
obj.data.materials.append(mat_brass2)
parts.append(obj)

# Rim
bpy.ops.mesh.primitive_torus_add(major_radius=0.10, minor_radius=0.005, major_segments=12, minor_segments=4, location=(0, 0, 0.715))
obj = bpy.context.active_object
obj.name = "SA_Rim"
obj.data.materials.append(mat_brass2)
parts.append(obj)

# Cigarette
bpy.ops.mesh.primitive_cylinder_add(radius=0.004, depth=0.07, vertices=4,
    location=(0.03, 0, 0.70), rotation=(0, math.radians(75), math.radians(20)))
obj = bpy.context.active_object
obj.name = "SA_Cig"
obj.data.materials.append(mat_grey)
parts.append(obj)

join_and_export(parts, "StandingAshtray", "/tmp/standing_ashtray.glb")


# ═══════════════════════════════════════════════════════════
# MODEL 3: Record Player (bonus — for the suite)
# ═══════════════════════════════════════════════════════════
clear_scene()
mat_wood = make_mat("EV_Wood_Dark", 105, 78, 48, roughness=0.7)
mat_brass3 = make_mat("EV_Brass_Dull", 160, 140, 95, metallic=0.7, roughness=0.45)
mat_leather = make_mat("EV_Leather_Dark", 45, 35, 25, roughness=0.75)

parts = []

# Cabinet body
bpy.ops.mesh.primitive_cube_add(size=1, location=(0, 0, 0.06))
obj = bpy.context.active_object
obj.name = "RP_Cabinet"
obj.scale = (0.25, 0.20, 0.06)
bpy.ops.object.transform_apply(scale=True)
obj.data.materials.append(mat_wood)
parts.append(obj)

# Lid (slightly angled open)
bpy.ops.mesh.primitive_cube_add(size=1, location=(0, -0.095, 0.12))
obj = bpy.context.active_object
obj.name = "RP_Lid"
obj.scale = (0.24, 0.003, 0.09)
bpy.ops.object.transform_apply(scale=True)
obj.rotation_euler = (math.radians(-60), 0, 0)
bpy.ops.object.transform_apply(rotation=True)
obj.data.materials.append(mat_wood)
parts.append(obj)

# Platter disc
bpy.ops.mesh.primitive_cylinder_add(radius=0.08, depth=0.005, vertices=16, location=(0, 0, 0.0925))
obj = bpy.context.active_object
obj.name = "RP_Platter"
obj.data.materials.append(mat_leather)  # dark rubber/felt
parts.append(obj)

# Spindle
bpy.ops.mesh.primitive_cylinder_add(radius=0.005, depth=0.02, vertices=6, location=(0, 0, 0.105))
obj = bpy.context.active_object
obj.name = "RP_Spindle"
obj.data.materials.append(mat_brass3)
parts.append(obj)

# Tonearm base
bpy.ops.mesh.primitive_cylinder_add(radius=0.012, depth=0.015, vertices=6, location=(0.09, -0.06, 0.10))
obj = bpy.context.active_object
obj.name = "RP_ArmBase"
obj.data.materials.append(mat_brass3)
parts.append(obj)

# Tonearm
bpy.ops.mesh.primitive_cylinder_add(radius=0.003, depth=0.12, vertices=4,
    location=(0.04, -0.02, 0.11), rotation=(0, math.radians(80), math.radians(-30)))
obj = bpy.context.active_object
obj.name = "RP_Arm"
obj.data.materials.append(mat_brass3)
parts.append(obj)

join_and_export(parts, "RecordPlayer", "/tmp/record_player.glb")


# ═══════════════════════════════════════════════════════════
# SUMMARY
# ═══════════════════════════════════════════════════════════
print("[EV] ═══ Batch Build Complete ═══")
for r in results:
    print(r)
print(f"[EV] Total models: {len(results)}")
