"""
Experiment 06: Standing Ashtray
Mid-century pedestal ashtray with a single cigarette. Atmospheric prop.
Budget: 100 tris.
"""
import bpy
import math

bpy.ops.object.select_all(action='SELECT')
bpy.ops.object.delete()

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

mat_brass = make_mat("EV_Brass", 184, 157, 107, metallic=0.9, roughness=0.3)
mat_marble = make_mat("EV_Marble_Dark", 60, 58, 55, roughness=0.2)
mat_fabric = make_mat("EV_Fabric_Grey", 160, 155, 148, roughness=0.88)

parts = []

# ── Base: heavy marble disc ──
bpy.ops.mesh.primitive_cylinder_add(radius=0.12, depth=0.02, vertices=10, location=(0, 0, 0.01))
base = bpy.context.active_object
base.name = "Ashtray_Base"
base.data.materials.append(mat_marble)
parts.append(base)

# ── Stem: thin brass pole ──
bpy.ops.mesh.primitive_cylinder_add(radius=0.012, depth=0.65, vertices=6, location=(0, 0, 0.345))
stem = bpy.context.active_object
stem.name = "Ashtray_Stem"
stem.data.materials.append(mat_brass)
parts.append(stem)

# ── Tray: shallow brass bowl at top ──
bpy.ops.mesh.primitive_cone_add(radius1=0.10, radius2=0.08, depth=0.04,
    vertices=10, location=(0, 0, 0.695))
tray = bpy.context.active_object
tray.name = "Ashtray_Tray"
tray.data.materials.append(mat_brass)
parts.append(tray)

# ── Rim ring ──
bpy.ops.mesh.primitive_torus_add(
    major_radius=0.10, minor_radius=0.005,
    major_segments=12, minor_segments=4,
    location=(0, 0, 0.715))
rim = bpy.context.active_object
rim.name = "Ashtray_Rim"
rim.data.materials.append(mat_brass)
parts.append(rim)

# ── Cigarette: tiny cylinder at an angle ──
bpy.ops.mesh.primitive_cylinder_add(radius=0.004, depth=0.07, vertices=4,
    location=(0.03, 0, 0.70),
    rotation=(0, math.radians(75), math.radians(20)))
cig = bpy.context.active_object
cig.name = "Ashtray_Cigarette"
cig.data.materials.append(mat_fabric)  # grey-white paper
parts.append(cig)

# ── Join ──
bpy.ops.object.select_all(action='DESELECT')
for p in parts:
    p.select_set(True)
bpy.context.view_layer.objects.active = parts[0]
bpy.ops.object.join()

final = bpy.context.active_object
final.name = "StandingAshtray"
bpy.ops.object.origin_set(type='ORIGIN_GEOMETRY', center='BOUNDS')

tris = sum(max(0, len(p.vertices) - 2) for p in final.data.polygons)
dims = final.dimensions
print(f"[EV] StandingAshtray: {tris} tris, {dims.x:.3f}x{dims.y:.3f}x{dims.z:.3f}m")
