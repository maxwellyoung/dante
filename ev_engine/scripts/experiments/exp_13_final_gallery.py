"""
Final gallery: all 8 new models rendered via Workbench.
Import from GLB files on /tmp/ for accurate representation.
"""
import bpy
import math
import os

bpy.ops.object.select_all(action='SELECT')
bpy.ops.object.delete()

def mat(name, r, g, b):
    m = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    m.diffuse_color = (r/255, g/255, b/255, 1.0)
    return m

m_floor = mat("Floor", 50, 48, 45)
m_ped = mat("Ped", 175, 170, 165)

# Floor
bpy.ops.mesh.primitive_plane_add(size=14, location=(0, 0, 0))
bpy.context.active_object.data.materials.append(m_floor)

models = [
    ("floor_lamp",        -3.5, None,  "Floor Lamp"),
    ("wine_glass",        -2.5, 0.75,  "Wine Glass"),
    ("photograph_frame",  -1.5, 0.75,  "Photo Frame"),
    ("standing_ashtray",  -0.5, None,  "Ashtray"),
    ("record_player",      0.5, 0.75,  "Record Player"),
    ("room_service_tray",  1.5, 0.75,  "Room Service"),
    ("desk_lamp",          2.5, 0.75,  "Desk Lamp"),
    ("ice_bucket",         3.5, 0.75,  "Ice Bucket"),
]

loaded = []
for name, x, ped_h, label in models:
    path = f"/tmp/{name}.glb"
    if not os.path.exists(path):
        print(f"  SKIP: {path}")
        continue

    # Pedestal
    if ped_h:
        bpy.ops.mesh.primitive_cube_add(size=1, location=(x, 0, ped_h/2))
        p = bpy.context.active_object
        p.scale = (0.35, 0.3, ped_h)
        bpy.ops.object.transform_apply(scale=True)
        p.data.materials.append(m_ped)

    # Import GLB
    bpy.ops.import_scene.gltf(filepath=path)
    imported = bpy.context.selected_objects
    for obj in imported:
        obj.location.x += x
        if ped_h:
            obj.location.z += ped_h
    loaded.append(label)

# ── Camera ──
cam_data = bpy.data.cameras.new("Cam")
cam_data.lens = 30  # wider for 8 models
cam_obj = bpy.data.objects.new("Cam", cam_data)
bpy.context.scene.collection.objects.link(cam_obj)
cam_obj.location = (0, -7, 2.5)
cam_obj.rotation_euler = (math.radians(72), 0, 0)
bpy.context.scene.camera = cam_obj

# ── Render ──
bpy.context.scene.render.engine = 'BLENDER_WORKBENCH'
bpy.context.scene.display.shading.light = 'STUDIO'
bpy.context.scene.display.shading.color_type = 'MATERIAL'
bpy.context.scene.display.shading.studio_light = 'studio.sl'
bpy.context.scene.render.filepath = '/tmp/ev_gallery_final.png'
bpy.context.scene.render.resolution_x = 1920
bpy.context.scene.render.resolution_y = 800
bpy.ops.render.render(write_still=True)

size = os.path.getsize('/tmp/ev_gallery_final.png')
print(f"[EV] Final gallery: {len(loaded)} models, {size/1024:.0f} KB")
for l in loaded:
    print(f"  - {l}")
