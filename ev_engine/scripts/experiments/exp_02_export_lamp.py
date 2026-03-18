"""Export the FloorLamp as GLB"""
import bpy

# Select the lamp
bpy.ops.object.select_all(action='DESELECT')
obj = bpy.data.objects.get("FloorLamp")
if obj:
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj

    bpy.ops.export_scene.gltf(
        filepath='/tmp/floor_lamp.glb',
        export_format='GLB',
        use_selection=True,
        export_apply=True,
        export_animations=False,
        export_colors=True,
        export_yup=True,
    )
    print(f"[EV] Exported /tmp/floor_lamp.glb")

    # Also get file size
    import os
    size = os.path.getsize('/tmp/floor_lamp.glb')
    print(f"[EV] Size: {size} bytes ({size/1024:.1f} KB)")
else:
    print("[EV] ERROR: FloorLamp not found in scene")
