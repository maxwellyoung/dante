"""Render the gallery scene — quick preview at 960x600"""
import bpy

# Ensure camera is set
cam = bpy.data.objects.get("GalleryCamera")
if not cam:
    for obj in bpy.data.objects:
        if obj.type == 'CAMERA':
            cam = obj
            break
if cam:
    bpy.context.scene.camera = cam
else:
    print("[EV] ERROR: No camera!")

bpy.context.scene.render.filepath = '/tmp/ev_gallery.png'
bpy.context.scene.render.resolution_x = 960
bpy.context.scene.render.resolution_y = 600
bpy.context.scene.render.engine = 'BLENDER_EEVEE'

bpy.ops.render.render(write_still=True)

import os
size = os.path.getsize('/tmp/ev_gallery.png')
print(f"[EV] Rendered gallery: {size/1024:.0f} KB")
