#!/usr/bin/env python3
"""Send a Blender Python script to the BlenderMCP addon via TCP socket.

Usage:
    python3 blender_send.py script.py          # execute a .py file
    python3 blender_send.py -c "import bpy; ..." # inline code
    python3 blender_send.py --info             # get scene info
    python3 blender_send.py --render out.png   # render to file

Expects Blender + BlenderMCP addon listening on localhost:9877.
Run from Mini directly, or via: ssh mini-ts 'python3 ~/path/to/blender_send.py ...'
"""

import socket
import json
import sys
import os

HOST = "localhost"
PORT = int(os.environ.get("BLENDER_PORT", 9877))
TIMEOUT = int(os.environ.get("BLENDER_TIMEOUT", 120))


def send(payload: dict) -> dict:
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.settimeout(TIMEOUT)
    s.connect((HOST, PORT))
    s.sendall(json.dumps(payload).encode("utf-8"))
    data = b""
    while True:
        try:
            chunk = s.recv(8192)
            if not chunk:
                break
            data += chunk
        except socket.timeout:
            break
    s.close()
    if not data:
        return {"status": "error", "message": "No response (is Blender active?)"}
    try:
        return json.loads(data)
    except json.JSONDecodeError:
        return {"status": "error", "message": f"Bad response: {data[:200]}"}


def execute_code(code: str) -> dict:
    return send({"type": "execute_code", "params": {"code": code}})


def get_scene_info() -> dict:
    return send({"type": "get_scene_info"})


def render_preview(output_path: str) -> dict:
    code = f"""
import bpy
bpy.context.scene.render.filepath = '{output_path}'
bpy.ops.render.render(write_still=True)
"""
    return execute_code(code)


def export_obj(output_path: str) -> dict:
    code = f"""
import bpy
bpy.ops.wm.obj_export(
    filepath='{output_path}',
    export_selected_objects=True,
    apply_modifiers=True,
    export_normals=True,
    export_colors=True,
)
"""
    return execute_code(code)


def export_glb(output_path: str) -> dict:
    code = f"""
import bpy
bpy.ops.export_scene.gltf(
    filepath='{output_path}',
    export_format='GLB',
    use_selection=True,
    export_apply=True,
    export_animations=True,
    export_skins=True,
    export_morph=True,
    export_colors=True,
    export_yup=True,
)
"""
    return execute_code(code)


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print(__doc__)
        sys.exit(1)

    arg = sys.argv[1]

    if arg == "--info":
        result = get_scene_info()
    elif arg == "--render":
        path = sys.argv[2] if len(sys.argv) > 2 else "/Users/klaus/preview.png"
        result = render_preview(path)
    elif arg == "--export-obj":
        path = sys.argv[2] if len(sys.argv) > 2 else "/Users/klaus/export.obj"
        result = export_obj(path)
    elif arg == "--export-glb":
        path = sys.argv[2] if len(sys.argv) > 2 else "/Users/klaus/export.glb"
        result = export_glb(path)
    elif arg == "-c":
        code = sys.argv[2]
        result = execute_code(code)
    elif os.path.isfile(arg):
        with open(arg) as f:
            code = f.read()
        result = execute_code(code)
    else:
        print(f"Unknown argument: {arg}")
        sys.exit(1)

    print(json.dumps(result, indent=2))
