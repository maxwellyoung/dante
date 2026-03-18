"""
Experiment 11: Desk Lamp (Anglepoise-style)
An articulated desk lamp — for the space suite desk next to the telephone.
More complex form with angled arms. Budget: 120 tris.
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
m_brass_d = mat("BrassDull", 160, 140, 95, 0.7, 0.45)
m_fabric = mat("Fabric", 215, 210, 200, 0.0, 0.85)

parts = []

# ── Weighted base ──
bpy.ops.mesh.primitive_cylinder_add(radius=0.08, depth=0.025, vertices=10, location=(0, 0, 0.0125))
base = bpy.context.active_object
base.name = "DL_Base"
base.data.materials.append(m_brass_d)
parts.append(base)

# ── Lower arm (angled up at 60°) ──
bpy.ops.mesh.primitive_cylinder_add(radius=0.006, depth=0.25, vertices=4,
    location=(0, 0, 0.13), rotation=(math.radians(-15), 0, 0))
arm1 = bpy.context.active_object
arm1.name = "DL_LowerArm"
arm1.data.materials.append(m_brass)
parts.append(arm1)

# ── Joint ball ──
bpy.ops.mesh.primitive_uv_sphere_add(radius=0.012, segments=8, ring_count=4,
    location=(0, 0.035, 0.255))
j1 = bpy.context.active_object
j1.name = "DL_Joint1"
j1.data.materials.append(m_brass)
parts.append(j1)

# ── Upper arm (angled forward and up) ──
bpy.ops.mesh.primitive_cylinder_add(radius=0.006, depth=0.20, vertices=4,
    location=(0, 0.09, 0.32), rotation=(math.radians(40), 0, 0))
arm2 = bpy.context.active_object
arm2.name = "DL_UpperArm"
arm2.data.materials.append(m_brass)
parts.append(arm2)

# ── Joint ball 2 ──
bpy.ops.mesh.primitive_uv_sphere_add(radius=0.012, segments=8, ring_count=4,
    location=(0, 0.14, 0.34))
j2 = bpy.context.active_object
j2.name = "DL_Joint2"
j2.data.materials.append(m_brass)
parts.append(j2)

# ── Shade (cone, pointing down) ──
bpy.ops.mesh.primitive_cone_add(radius1=0.06, radius2=0.02, depth=0.06,
    vertices=8, location=(0, 0.15, 0.32))
shade = bpy.context.active_object
shade.name = "DL_Shade"
shade.rotation_euler = (math.radians(160), 0, 0)
bpy.ops.object.transform_apply(rotation=True)
shade.data.materials.append(m_fabric)
parts.append(shade)

# ── Join ──
bpy.ops.object.select_all(action='DESELECT')
for p in parts:
    p.select_set(True)
bpy.context.view_layer.objects.active = parts[0]
bpy.ops.object.join()
final = bpy.context.active_object
final.name = "DeskLamp"
bpy.ops.object.origin_set(type='ORIGIN_GEOMETRY', center='BOUNDS')

tris = sum(max(0, len(p.vertices) - 2) for p in final.data.polygons)
dims = final.dimensions
print(f"[EV] DeskLamp: {tris} tris, {dims.x:.3f}x{dims.y:.3f}x{dims.z:.3f}m")

# ── Export ──
import os
bpy.ops.object.select_all(action='SELECT')
bpy.ops.export_scene.gltf(
    filepath='/tmp/desk_lamp.glb', export_format='GLB',
    use_selection=True, export_apply=True, export_animations=False, export_yup=True)
size = os.path.getsize('/tmp/desk_lamp.glb')
print(f"[EV] Exported: {size/1024:.1f} KB")
