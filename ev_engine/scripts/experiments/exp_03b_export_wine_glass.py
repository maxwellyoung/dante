"""Export WineGlass as GLB"""
import bpy, os
bpy.ops.object.select_all(action='SELECT')
bpy.ops.export_scene.gltf(
    filepath='/tmp/wine_glass.glb',
    export_format='GLB',
    use_selection=True,
    export_apply=True,
    export_animations=False,
    export_yup=True,
)
size = os.path.getsize('/tmp/wine_glass.glb')
print(f"[EV] Exported wine_glass.glb ({size} bytes, {size/1024:.1f} KB)")
