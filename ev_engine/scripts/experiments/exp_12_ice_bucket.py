"""
Experiment 12: Ice Bucket with Champagne Bottle
Classic hotel luxury prop. Bucket on a stand with a bottle poking out.
Budget: 150 tris.
"""
import bpy
import math

bpy.ops.object.select_all(action='SELECT')
bpy.ops.object.delete()

def mat(name, r, g, b, metallic=0.0, roughness=0.7):
    m = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    m.use_nodes = True
    bsdf = m.node_tree.nodes.get("Principled BSDF")
    if not bsdf:
        bsdf = m.node_tree.nodes.new("ShaderNodeBsdfPrincipled")
    bsdf.inputs["Base Color"].default_value = (r/255, g/255, b/255, 1.0)
    bsdf.inputs["Metallic"].default_value = metallic
    bsdf.inputs["Roughness"].default_value = roughness
    m.diffuse_color = (r/255, g/255, b/255, 1.0)
    return m

m_brass = mat("Brass", 184, 157, 107, 0.9, 0.3)
m_glass = mat("Glass", 40, 60, 35, 0.0, 0.15)  # dark green bottle
m_fabric = mat("Fabric", 245, 242, 235, 0.0, 0.9)  # napkin wrap
m_gold = mat("Gold", 210, 180, 120, 0.85, 0.25)  # foil cap

parts = []

# ── Bucket body (truncated cone) ──
bpy.ops.mesh.primitive_cone_add(radius1=0.08, radius2=0.10, depth=0.18,
    vertices=10, location=(0, 0, 0.09))
bucket = bpy.context.active_object
bucket.name = "IB_Bucket"
bucket.data.materials.append(m_brass)
parts.append(bucket)

# ── Bucket rim ──
bpy.ops.mesh.primitive_torus_add(major_radius=0.10, minor_radius=0.006,
    major_segments=12, minor_segments=4, location=(0, 0, 0.18))
rim = bpy.context.active_object
rim.name = "IB_Rim"
rim.data.materials.append(m_brass)
parts.append(rim)

# ── Handle (one side) ──
bpy.ops.mesh.primitive_torus_add(major_radius=0.025, minor_radius=0.004,
    major_segments=8, minor_segments=4, location=(0.10, 0, 0.14))
h1 = bpy.context.active_object
h1.name = "IB_Handle1"
h1.rotation_euler = (math.radians(90), 0, 0)
bpy.ops.object.transform_apply(rotation=True)
h1.data.materials.append(m_brass)
parts.append(h1)

# ── Handle (other side) ──
bpy.ops.mesh.primitive_torus_add(major_radius=0.025, minor_radius=0.004,
    major_segments=8, minor_segments=4, location=(-0.10, 0, 0.14))
h2 = bpy.context.active_object
h2.name = "IB_Handle2"
h2.rotation_euler = (math.radians(90), 0, 0)
bpy.ops.object.transform_apply(rotation=True)
h2.data.materials.append(m_brass)
parts.append(h2)

# ── Champagne bottle (sticking out at slight angle) ──
bpy.ops.mesh.primitive_cylinder_add(radius=0.032, depth=0.25, vertices=8,
    location=(0.01, 0, 0.22), rotation=(math.radians(-8), 0, 0))
bottle = bpy.context.active_object
bottle.name = "IB_Bottle"
bottle.data.materials.append(m_glass)
parts.append(bottle)

# ── Bottle neck (thinner) ──
bpy.ops.mesh.primitive_cylinder_add(radius=0.015, depth=0.08, vertices=6,
    location=(0.01, -0.02, 0.38), rotation=(math.radians(-8), 0, 0))
neck = bpy.context.active_object
neck.name = "IB_Neck"
neck.data.materials.append(m_glass)
parts.append(neck)

# ── Foil cap ──
bpy.ops.mesh.primitive_uv_sphere_add(radius=0.016, segments=6, ring_count=4,
    location=(0.01, -0.025, 0.42))
cap = bpy.context.active_object
cap.name = "IB_Cap"
cap.scale = (1, 1, 0.6)
bpy.ops.object.transform_apply(scale=True)
cap.data.materials.append(m_gold)
parts.append(cap)

# ── Napkin draped over edge ──
bpy.ops.mesh.primitive_cube_add(size=0.08, location=(0.06, 0.04, 0.19))
napkin = bpy.context.active_object
napkin.name = "IB_Napkin"
napkin.scale = (1.5, 0.4, 0.15)
bpy.ops.object.transform_apply(scale=True)
napkin.rotation_euler = (0, 0, math.radians(15))
bpy.ops.object.transform_apply(rotation=True)
# Subdivide for slight softness
mod = napkin.modifiers.new("Sub", "SUBSURF")
mod.levels = 1
bpy.ops.object.modifier_apply(modifier="Sub")
napkin.data.materials.append(m_fabric)
parts.append(napkin)

# ── Join ──
bpy.ops.object.select_all(action='DESELECT')
for p in parts:
    p.select_set(True)
bpy.context.view_layer.objects.active = parts[0]
bpy.ops.object.join()
final = bpy.context.active_object
final.name = "IceBucket"
bpy.ops.object.origin_set(type='ORIGIN_GEOMETRY', center='BOUNDS')

tris = sum(max(0, len(p.vertices) - 2) for p in final.data.polygons)
dims = final.dimensions
print(f"[EV] IceBucket: {tris} tris, {dims.x:.3f}x{dims.y:.3f}x{dims.z:.3f}m")

# ── Export ──
import os
bpy.ops.object.select_all(action='SELECT')
bpy.ops.export_scene.gltf(
    filepath='/tmp/ice_bucket.glb', export_format='GLB',
    use_selection=True, export_apply=True, export_animations=False, export_yup=True)
size = os.path.getsize('/tmp/ice_bucket.glb')
print(f"[EV] Exported: {size/1024:.1f} KB")
