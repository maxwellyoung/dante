"""Export RoomServiceTray as GLB"""
import bpy, os
bpy.ops.object.select_all(action='SELECT')
bpy.ops.export_scene.gltf(
    filepath='/tmp/room_service_tray.glb',
    export_format='GLB',
    use_selection=True,
    export_apply=True,
    export_animations=False,
    export_yup=True,
)
size = os.path.getsize('/tmp/room_service_tray.glb')
print(f"[EV] Exported room_service_tray.glb ({size} bytes, {size/1024:.1f} KB)")
