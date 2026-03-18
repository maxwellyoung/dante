"""
Experiment 01: Floor Lamp — built entirely via MCP
A mid-century floor lamp for the space suite. Arc lamp silhouette.
Budget: 150 tris
"""
import bpy
import math

# ── Clear scene ──
bpy.ops.object.select_all(action='SELECT')
bpy.ops.object.delete()

# ── Materials (locked palette) ──
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
mat_fabric = make_mat("EV_Fabric_Cream", 215, 210, 200, roughness=0.85)
mat_marble = make_mat("EV_Marble_Dark", 60, 58, 55, roughness=0.2)

parts = []

# ── Base: heavy marble disc ──
bpy.ops.mesh.primitive_cylinder_add(radius=0.18, depth=0.03, vertices=12, location=(0, 0, 0.015))
base = bpy.context.active_object
base.name = "FloorLamp_Base"
base.data.materials.append(mat_marble)
parts.append(base)

# ── Stem: thin brass pole ──
bpy.ops.mesh.primitive_cylinder_add(radius=0.012, depth=1.5, vertices=6, location=(0, 0, 0.78))
stem = bpy.context.active_object
stem.name = "FloorLamp_Stem"
stem.data.materials.append(mat_brass)
parts.append(stem)

# ── Arc section: angled brass tube ──
# Tilt the top portion of the arc
bpy.ops.mesh.primitive_cylinder_add(radius=0.01, depth=0.4, vertices=6,
    location=(0.1, 0, 1.65),
    rotation=(0, math.radians(25), 0))
arc = bpy.context.active_object
arc.name = "FloorLamp_Arc"
arc.data.materials.append(mat_brass)
parts.append(arc)

# ── Shade: truncated cone (fabric) ──
bpy.ops.mesh.primitive_cone_add(radius1=0.15, radius2=0.08, depth=0.2,
    vertices=10, location=(0.2, 0, 1.8))
shade = bpy.context.active_object
shade.name = "FloorLamp_Shade"
shade.data.materials.append(mat_fabric)
parts.append(shade)

# ── Joint: small brass sphere ──
bpy.ops.mesh.primitive_uv_sphere_add(radius=0.02, segments=8, ring_count=6, location=(0, 0, 1.53))
joint = bpy.context.active_object
joint.name = "FloorLamp_Joint"
joint.data.materials.append(mat_brass)
parts.append(joint)

# ── Join all ──
bpy.ops.object.select_all(action='DESELECT')
for p in parts:
    p.select_set(True)
bpy.context.view_layer.objects.active = parts[0]
bpy.ops.object.join()

final = bpy.context.active_object
final.name = "FloorLamp"

# Origin to bottom center
bpy.ops.object.origin_set(type='ORIGIN_GEOMETRY', center='BOUNDS')

# Count tris
tris = sum(max(0, len(p.vertices) - 2) for p in final.data.polygons)
mats = len(final.data.materials)
dims = final.dimensions

print(f"[EV] FloorLamp built: {tris} tris, {mats} materials, {dims.x:.2f}x{dims.y:.2f}x{dims.z:.2f}m")
