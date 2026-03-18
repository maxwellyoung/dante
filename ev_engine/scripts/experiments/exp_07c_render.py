"""Add camera and render — single script"""
import bpy
import math

# List what's in scene
print("Objects in scene:")
for obj in bpy.data.objects:
    print(f"  {obj.name} ({obj.type}) at {[round(v,2) for v in obj.location]}")

# Delete existing cameras
for obj in bpy.data.objects:
    if obj.type == 'CAMERA':
        bpy.data.objects.remove(obj)

# Create camera
cam_data = bpy.data.cameras.new("RenderCam")
cam_data.lens = 35
cam_obj = bpy.data.objects.new("RenderCam", cam_data)
bpy.context.scene.collection.objects.link(cam_obj)
cam_obj.location = (0, -8, 3)
cam_obj.rotation_euler = (math.radians(75), 0, 0)
bpy.context.scene.camera = cam_obj

print(f"Camera set: {bpy.context.scene.camera.name}")

# Render settings
bpy.context.scene.render.filepath = '/tmp/ev_gallery.png'
bpy.context.scene.render.resolution_x = 960
bpy.context.scene.render.resolution_y = 600
bpy.context.scene.render.engine = 'BLENDER_EEVEE'

# Render
bpy.ops.render.render(write_still=True)

import os
size = os.path.getsize('/tmp/ev_gallery.png')
print(f"[EV] Rendered: {size/1024:.0f} KB")
