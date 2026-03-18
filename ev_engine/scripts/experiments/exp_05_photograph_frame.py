"""
Experiment 05: Photograph Frame
A small standing photo frame — the photograph IS the narrative.
What's in the photo? The player never sees it clearly. Just a shape. Two people.
Budget: 30 tris. Tiny object, huge emotional weight.
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

mat_brass = make_mat("EV_Brass_Dull", 160, 140, 95, metallic=0.7, roughness=0.45)
mat_glass = make_mat("EV_Glass_Warm", 220, 200, 180, roughness=0.08)
mat_fabric = make_mat("EV_Fabric_Cream", 215, 210, 200, roughness=0.85)

parts = []

# ── Frame: thin brass rectangle (portrait orientation) ──
# Outer frame
bpy.ops.mesh.primitive_cube_add(size=1, location=(0, 0, 0.08))
frame = bpy.context.active_object
frame.name = "Photo_Frame"
frame.scale = (0.10, 0.005, 0.13)
bpy.ops.object.transform_apply(scale=True)
frame.data.materials.append(mat_brass)
parts.append(frame)

# ── Glass/photo surface (slightly recessed) ──
bpy.ops.mesh.primitive_cube_add(size=1, location=(0, 0.001, 0.08))
photo = bpy.context.active_object
photo.name = "Photo_Surface"
photo.scale = (0.085, 0.002, 0.115)
bpy.ops.object.transform_apply(scale=True)
photo.data.materials.append(mat_glass)
parts.append(photo)

# ── Back stand: angled support leg ──
bpy.ops.mesh.primitive_cube_add(size=1, location=(0, -0.03, 0.06))
stand = bpy.context.active_object
stand.name = "Photo_Stand"
stand.scale = (0.008, 0.04, 0.08)
bpy.ops.object.transform_apply(scale=True)
stand.rotation_euler = (math.radians(15), 0, 0)
bpy.ops.object.transform_apply(rotation=True)
stand.data.materials.append(mat_brass)
parts.append(stand)

# ── Join ──
bpy.ops.object.select_all(action='DESELECT')
for p in parts:
    p.select_set(True)
bpy.context.view_layer.objects.active = parts[0]
bpy.ops.object.join()

final = bpy.context.active_object
final.name = "PhotographFrame"
bpy.ops.object.origin_set(type='ORIGIN_GEOMETRY', center='BOUNDS')

tris = sum(max(0, len(p.vertices) - 2) for p in final.data.polygons)
dims = final.dimensions
print(f"[EV] PhotographFrame: {tris} tris, {dims.x:.3f}x{dims.y:.3f}x{dims.z:.3f}m")
