"""
Experiment 07: Gallery View
Load all our new models into a single scene for a visual check.
Arrange them on a shelf/table layout with proper lighting.
"""
import bpy
import math
import os

# ── Clear everything ──
bpy.ops.object.select_all(action='SELECT')
bpy.ops.object.delete()
for c in bpy.data.collections:
    if c.name != 'Scene Collection':
        bpy.data.collections.remove(c)

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

mat_floor = make_mat("Gallery_Floor", 40, 38, 35, roughness=0.9)
mat_wall = make_mat("Gallery_Wall", 220, 218, 212, roughness=0.85)

# ── Gallery room ──
# Floor
bpy.ops.mesh.primitive_plane_add(size=10, location=(0, 0, 0))
floor = bpy.context.active_object
floor.name = "Gallery_Floor"
floor.data.materials.append(mat_floor)

# Back wall
bpy.ops.mesh.primitive_plane_add(size=10, location=(0, -5, 2.5))
wall = bpy.context.active_object
wall.name = "Gallery_Wall"
wall.rotation_euler = (math.radians(90), 0, 0)
wall.data.materials.append(mat_wall)

# ── Load GLBs from /tmp/ ──
models = [
    ("/tmp/floor_lamp.glb",        (-3, -2, 0),   "Floor Lamp"),
    ("/tmp/wine_glass.glb",        (-1.5, -2, 0.75), "Wine Glass"),
    ("/tmp/photograph_frame.glb",  (-0.5, -2, 0.75), "Photo Frame"),
    ("/tmp/standing_ashtray.glb",  (0.5, -2, 0),  "Standing Ashtray"),
    ("/tmp/record_player.glb",     (2, -2, 0.75),   "Record Player"),
    ("/tmp/room_service_tray.glb", (3.5, -2, 0.75), "Room Service Tray"),
]

# Pedestal for small objects
for name, (x, y, z), label in models:
    if z > 0:  # needs a pedestal
        bpy.ops.mesh.primitive_cube_add(size=1, location=(x, y, z/2))
        ped = bpy.context.active_object
        ped.name = f"Pedestal_{label}"
        ped.scale = (0.4, 0.3, z)
        bpy.ops.object.transform_apply(scale=True)
        ped.data.materials.append(mat_wall)

loaded = []
for filepath, (x, y, z), label in models:
    if os.path.exists(filepath):
        bpy.ops.import_scene.gltf(filepath=filepath)
        imported = bpy.context.selected_objects
        for obj in imported:
            obj.location = (x, y, z)
        loaded.append(label)
        print(f"  Loaded: {label}")
    else:
        print(f"  MISSING: {filepath}")

# ── Lighting ──
# Key light (warm)
bpy.ops.object.light_add(type='SUN', location=(3, 3, 5))
key = bpy.context.active_object
key.name = "Key"
key.data.energy = 3
key.data.color = (1.0, 0.95, 0.85)
key.rotation_euler = (math.radians(-45), math.radians(20), 0)

# Fill light (cool)
bpy.ops.object.light_add(type='SUN', location=(-3, 2, 4))
fill = bpy.context.active_object
fill.name = "Fill"
fill.data.energy = 1.5
fill.data.color = (0.85, 0.9, 1.0)
fill.rotation_euler = (math.radians(-60), math.radians(-30), 0)

# ── Camera ──
bpy.ops.object.camera_add(location=(0, -8, 3))
cam = bpy.context.active_object
cam.name = "GalleryCamera"
cam.rotation_euler = (math.radians(75), 0, 0)
cam.data.lens = 35
bpy.context.scene.camera = cam

# ── Render settings ──
bpy.context.scene.render.engine = 'CYCLES'
bpy.context.scene.cycles.samples = 64
bpy.context.scene.render.resolution_x = 1920
bpy.context.scene.render.resolution_y = 1200
bpy.context.scene.render.film_transparent = False

# Set world background to dark
world = bpy.context.scene.world
if not world:
    world = bpy.data.worlds.new("World")
    bpy.context.scene.world = world
world.use_nodes = True
bg = world.node_tree.nodes.get("Background")
if bg:
    bg.inputs["Color"].default_value = (0.02, 0.02, 0.02, 1.0)
    bg.inputs["Strength"].default_value = 0.5

print(f"\n[EV] Gallery scene ready with {len(loaded)} models:")
for l in loaded:
    print(f"  - {l}")
print(f"\nTo render: bpy.ops.render.render(write_still=True)")
print(f"Or use: ./scripts/mcp_model.sh render /tmp/gallery.png")
