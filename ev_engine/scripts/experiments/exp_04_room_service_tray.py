"""
Experiment 04: Room Service Tray
Silver cloche dome on a tray — untouched room service. Atmospheric prop.
Budget: 120 tris.
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
mat_fabric = make_mat("EV_Fabric_White", 245, 242, 235, roughness=0.9)

parts = []

# ── Tray: flat oval disc ──
bpy.ops.mesh.primitive_cylinder_add(radius=0.2, depth=0.015, vertices=16, location=(0, 0, 0.0075))
tray = bpy.context.active_object
tray.name = "Tray_Base"
tray.scale = (1.3, 1.0, 1.0)
bpy.ops.object.transform_apply(scale=True)
tray.data.materials.append(mat_brass)
parts.append(tray)

# ── Tray lip: slightly larger ring ──
bpy.ops.mesh.primitive_torus_add(
    major_radius=0.24, minor_radius=0.008,
    major_segments=16, minor_segments=4,
    location=(0, 0, 0.015))
lip = bpy.context.active_object
lip.name = "Tray_Lip"
lip.scale = (1.1, 0.85, 1.0)
bpy.ops.object.transform_apply(scale=True)
lip.data.materials.append(mat_brass)
parts.append(lip)

# ── Cloche dome: hemisphere ──
bpy.ops.mesh.primitive_uv_sphere_add(radius=0.14, segments=12, ring_count=6, location=(0, 0, 0.015))
dome = bpy.context.active_object
dome.name = "Tray_Dome"
# Cut bottom half by scaling Z and moving up
dome.scale = (1.0, 1.0, 0.7)
bpy.ops.object.transform_apply(scale=True)
dome.location = (0, 0, 0.08)
dome.data.materials.append(mat_brass)
parts.append(dome)

# ── Dome handle: small sphere on top ──
bpy.ops.mesh.primitive_uv_sphere_add(radius=0.018, segments=8, ring_count=4, location=(0, 0, 0.2))
handle = bpy.context.active_object
handle.name = "Tray_Handle"
handle.data.materials.append(mat_brass)
parts.append(handle)

# ── Napkin: small folded fabric next to tray ──
bpy.ops.mesh.primitive_cube_add(size=0.1, location=(0.3, 0, 0.025))
napkin = bpy.context.active_object
napkin.name = "Tray_Napkin"
napkin.scale = (1.2, 0.8, 0.15)
bpy.ops.object.transform_apply(scale=True)
# Subdivide for softness
mod = napkin.modifiers.new("Subsurf", "SUBSURF")
mod.levels = 1
mod.render_levels = 1
bpy.ops.object.modifier_apply(modifier="Subsurf")
napkin.data.materials.append(mat_fabric)
parts.append(napkin)

# ── Join ──
bpy.ops.object.select_all(action='DESELECT')
for p in parts:
    p.select_set(True)
bpy.context.view_layer.objects.active = parts[0]
bpy.ops.object.join()

final = bpy.context.active_object
final.name = "RoomServiceTray"
bpy.ops.object.origin_set(type='ORIGIN_GEOMETRY', center='BOUNDS')

tris = sum(max(0, len(p.vertices) - 2) for p in final.data.polygons)
dims = final.dimensions
print(f"[EV] RoomServiceTray: {tris} tris, {dims.x:.3f}x{dims.y:.3f}x{dims.z:.3f}m")
