#!/usr/bin/env python3
"""Potted Plant — Lobby/corridor greenery.
Brass pot with organic leaf canopy. ~250 tris.
The only organic form in a world of boxes and brass.
"""
import bpy, bmesh, math, os

# Clear scene
bpy.ops.object.select_all(action='SELECT')
bpy.ops.object.delete()
for m in list(bpy.data.meshes):
    if m.users == 0: bpy.data.meshes.remove(m)
for m in list(bpy.data.materials):
    if m.users == 0: bpy.data.materials.remove(m)

def mat(name, r, g, b, metal=0, rough=0.5):
    m = bpy.data.materials.new(name)
    m.use_nodes = True
    bsdf = m.node_tree.nodes["Principled BSDF"]
    bsdf.inputs["Base Color"].default_value = (r/255, g/255, b/255, 1)
    bsdf.inputs["Metallic"].default_value = metal
    bsdf.inputs["Roughness"].default_value = rough
    return m

BRASS = mat("EV_Brass", 184, 157, 107, 0.9, 0.3)
WOOD_DARK = mat("EV_Trunk", 80, 55, 30, 0, 0.8)
LEAF_GREEN = mat("EV_Leaf", 55, 85, 45, 0, 0.75)
LEAF_DARK = mat("EV_LeafDark", 35, 60, 30, 0, 0.8)
SOIL = mat("EV_Soil", 60, 45, 30, 0, 0.9)

# ── Pot — tapered brass cylinder ──
bpy.ops.mesh.primitive_cone_add(
    vertices=12, radius1=0.22, radius2=0.16, depth=0.35,
    location=(0, 0, 0.175)
)
pot = bpy.context.active_object
pot.name = "Pot"
pot.data.materials.append(BRASS)

# Pot rim — torus at top
bpy.ops.mesh.primitive_torus_add(
    major_radius=0.22, minor_radius=0.015,
    major_segments=12, minor_segments=6,
    location=(0, 0, 0.35)
)
rim = bpy.context.active_object
rim.name = "PotRim"
rim.data.materials.append(BRASS)

# Soil surface
bpy.ops.mesh.primitive_cylinder_add(
    vertices=12, radius=0.20, depth=0.02,
    location=(0, 0, 0.34)
)
soil = bpy.context.active_object
soil.name = "Soil"
soil.data.materials.append(SOIL)

# ── Trunk — slightly bent cylinder ──
bpy.ops.mesh.primitive_cylinder_add(
    vertices=6, radius=0.025, depth=0.5,
    location=(0, 0, 0.58)
)
trunk = bpy.context.active_object
trunk.name = "Trunk"
trunk.rotation_euler = (math.radians(5), 0, 0)
bpy.ops.object.transform_apply(rotation=True)
trunk.data.materials.append(WOOD_DARK)

# ── Canopy — cluster of overlapping spheres (organic silhouette) ──
canopy_positions = [
    (0.0,  0.0,  0.95, 0.18, LEAF_GREEN),   # center, large
    (0.12, 0.08, 0.88, 0.14, LEAF_DARK),    # front-right
    (-0.10, 0.06, 0.90, 0.15, LEAF_GREEN),  # front-left
    (0.05, -0.10, 0.92, 0.13, LEAF_DARK),   # back
    (-0.08, -0.06, 0.85, 0.12, LEAF_GREEN), # lower back-left
    (0.0,  0.12, 0.82, 0.11, LEAF_DARK),    # lower front
    (0.08, -0.05, 1.02, 0.10, LEAF_GREEN),  # top accent
]

for i, (cx, cy, cz, r, leaf_mat) in enumerate(canopy_positions):
    bpy.ops.mesh.primitive_uv_sphere_add(
        segments=8, ring_count=6, radius=r,
        location=(cx, cy, cz)
    )
    leaf = bpy.context.active_object
    leaf.name = f"Canopy_{i}"
    # Squash slightly for more natural form
    leaf.scale = (1.0 + (i % 3) * 0.1, 1.0 - (i % 2) * 0.1, 0.85)
    bpy.ops.object.transform_apply(scale=True)
    leaf.data.materials.append(leaf_mat)

# ── Join all ──
bpy.ops.object.select_all(action='SELECT')
bpy.context.view_layer.objects.active = pot
bpy.ops.object.join()
pot.name = "PottedPlant"

# Origin at floor center
bpy.context.scene.cursor.location = (0, 0, 0)
bpy.ops.object.origin_set(type='ORIGIN_CURSOR')

# ── Stats ──
tris = sum(len(p.vertices) - 2 for p in pot.data.polygons)
verts = len(pot.data.vertices)
mats = len(pot.data.materials)
print(f"PottedPlant: {verts} verts, {tris} tris, {mats} mats")

# ── Export ──
bpy.ops.object.select_all(action='SELECT')
bpy.ops.export_scene.gltf(
    filepath="/tmp/potted_plant.glb",
    export_format='GLB',
    use_selection=True,
    export_apply=True,
    export_yup=True,
)
size = os.path.getsize("/tmp/potted_plant.glb")
print(f"Exported: /tmp/potted_plant.glb ({size} bytes, {size/1024:.1f} KB)")
