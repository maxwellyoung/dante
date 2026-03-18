"""
Experiment 03: Wine Glass with Lipstick Mark
A single wine glass — evidence of the absent partner.
Budget: 80 tris. Narrative object (Phase 2 from the plan).
"""
import bpy
import math

# ── Clear scene ──
bpy.ops.object.select_all(action='SELECT')
bpy.ops.object.delete()

# ── Materials ──
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

mat_glass = make_mat("EV_Glass", 220, 225, 230, roughness=0.05)
mat_red = make_mat("EV_Godard_Red", 204, 20, 13, roughness=0.7)

parts = []

# ── Base: flat disc ──
bpy.ops.mesh.primitive_cylinder_add(radius=0.035, depth=0.005, vertices=10, location=(0, 0, 0.0025))
base = bpy.context.active_object
base.name = "WineGlass_Base"
base.data.materials.append(mat_glass)
parts.append(base)

# ── Stem: thin cylinder ──
bpy.ops.mesh.primitive_cylinder_add(radius=0.005, depth=0.12, vertices=6, location=(0, 0, 0.065))
stem = bpy.context.active_object
stem.name = "WineGlass_Stem"
stem.data.materials.append(mat_glass)
parts.append(stem)

# ── Bowl: truncated cone (wider at top) ──
bpy.ops.mesh.primitive_cone_add(radius1=0.01, radius2=0.04, depth=0.10,
    vertices=10, location=(0, 0, 0.175))
bowl = bpy.context.active_object
bowl.name = "WineGlass_Bowl"
bowl.data.materials.append(mat_glass)
parts.append(bowl)

# ── Rim: thin torus at the top ──
bpy.ops.mesh.primitive_torus_add(
    major_radius=0.04, minor_radius=0.003,
    major_segments=12, minor_segments=4,
    location=(0, 0, 0.225))
rim = bpy.context.active_object
rim.name = "WineGlass_Rim"
rim.data.materials.append(mat_glass)
parts.append(rim)

# ── Lipstick mark: small curved strip on the rim ──
# A tiny flattened cube positioned on one side of the rim
bpy.ops.mesh.primitive_cube_add(size=0.01, location=(0.038, 0, 0.225))
lip = bpy.context.active_object
lip.name = "WineGlass_Lipstick"
lip.scale = (0.5, 2.0, 1.5)
bpy.ops.object.transform_apply(scale=True)
lip.data.materials.append(mat_red)
parts.append(lip)

# ── Join all ──
bpy.ops.object.select_all(action='DESELECT')
for p in parts:
    p.select_set(True)
bpy.context.view_layer.objects.active = parts[0]
bpy.ops.object.join()

final = bpy.context.active_object
final.name = "WineGlass"

# Origin to bottom
bpy.ops.object.origin_set(type='ORIGIN_GEOMETRY', center='BOUNDS')

# Stats
tris = sum(max(0, len(p.vertices) - 2) for p in final.data.polygons)
dims = final.dimensions
print(f"[EV] WineGlass: {tris} tris, {dims.x:.3f}x{dims.y:.3f}x{dims.z:.3f}m")
