#!/usr/bin/env bash
# mcp_model.sh — High-level model automation
# Usage: ./scripts/mcp_model.sh build script.py        # Build model in Blender
#        ./scripts/mcp_model.sh export model_name       # Export GLB from current scene
#        ./scripts/mcp_model.sh deploy model_name       # Copy from Mini to assets/
#        ./scripts/mcp_model.sh info                    # Scene info
#        ./scripts/mcp_model.sh render out.png          # Render preview
#        ./scripts/mcp_model.sh full script.py [name]   # Build + export + deploy

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ENGINE_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
MINI_HOST="${MINI_HOST:-mini-ts}"
PORT="${BLENDER_PORT:-9877}"

send_code() {
    local code="$1"
    local tmpfile=$(mktemp /tmp/ev_mcp_XXXXXX.py)
    echo "$code" > "$tmpfile"
    scp -q "$tmpfile" "$MINI_HOST:/tmp/ev_mcp_code.py"
    rm -f "$tmpfile"

    # Ensure sender exists
    ssh -q "$MINI_HOST" "test -f /tmp/ev_blender_sender.py" 2>/dev/null || \
    ssh "$MINI_HOST" "cat > /tmp/ev_blender_sender.py" << 'SENDER'
import socket, json, sys
with open(sys.argv[1]) as f:
    code = f.read()
port = int(sys.argv[2]) if len(sys.argv) > 2 else 9877
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.settimeout(120)
s.connect(("localhost", port))
payload = json.dumps({"type": "execute_code", "params": {"code": code}})
s.sendall(payload.encode())
data = b""
while True:
    try:
        chunk = s.recv(8192)
        if not chunk: break
        data += chunk
    except: break
s.close()
resp = json.loads(data)
if resp.get("status") == "success":
    result = resp.get("result", {})
    if isinstance(result, dict) and result.get("result"):
        print(result["result"].rstrip())
    else:
        print(json.dumps(result, indent=2))
else:
    print("ERROR:", json.dumps(resp, indent=2))
    sys.exit(1)
SENDER

    ssh -o ServerAliveInterval=15 -o ServerAliveCountMax=8 \
        "$MINI_HOST" "python3 /tmp/ev_blender_sender.py /tmp/ev_mcp_code.py $PORT"
}

cmd_info() {
    send_code "
import bpy
objs = [(o.name, o.type) for o in bpy.data.objects]
print(f'Scene: {bpy.context.scene.name}')
print(f'Objects ({len(objs)}):')
for name, typ in objs:
    print(f'  {name} ({typ})')
print(f'Materials: {len(bpy.data.materials)}')
"
}

cmd_build() {
    local script="$1"
    scp -q "$script" "$MINI_HOST:/tmp/ev_mcp_code.py"
    ssh -o ServerAliveInterval=15 "$MINI_HOST" \
        "python3 /tmp/ev_blender_sender.py /tmp/ev_mcp_code.py $PORT"
}

cmd_export() {
    local name="$1"
    send_code "
import bpy, os
bpy.ops.object.select_all(action='SELECT')
bpy.ops.export_scene.gltf(
    filepath='/tmp/${name}.glb',
    export_format='GLB',
    use_selection=True,
    export_apply=True,
    export_animations=False,
    export_yup=True,
)
size = os.path.getsize('/tmp/${name}.glb')
print(f'Exported /tmp/${name}.glb ({size} bytes, {size/1024:.1f} KB)')
"
}

cmd_deploy() {
    local name="$1"
    scp -q "$MINI_HOST:/tmp/${name}.glb" "$ENGINE_DIR/assets/${name}.glb"
    local size=$(stat -f%z "$ENGINE_DIR/assets/${name}.glb" 2>/dev/null || echo "?")
    echo "Deployed: assets/${name}.glb ($size bytes)"
}

cmd_render() {
    local out="${1:-/tmp/ev_preview.png}"
    send_code "
import bpy
bpy.context.scene.render.filepath = '${out}'
bpy.context.scene.render.resolution_x = 960
bpy.context.scene.render.resolution_y = 600
bpy.ops.render.render(write_still=True)
print(f'Rendered to ${out}')
"
    if [[ "$out" == /tmp/* ]]; then
        scp -q "$MINI_HOST:$out" "/tmp/$(basename "$out")"
        echo "Preview saved to /tmp/$(basename "$out")"
    fi
}

cmd_full() {
    local script="$1"
    local name="${2:-$(basename "$script" .py | sed 's/^exp_[0-9]*_//')}"

    echo "═══ Full Pipeline: $name ═══"
    echo ""
    echo "▸ Building..."
    cmd_build "$script"
    echo ""
    echo "▸ Exporting..."
    cmd_export "$name"
    echo ""
    echo "▸ Deploying..."
    cmd_deploy "$name"
    echo ""
    echo "═══ Done: assets/${name}.glb ═══"
}

case "${1:-}" in
    info)    cmd_info ;;
    build)   cmd_build "$2" ;;
    export)  cmd_export "$2" ;;
    deploy)  cmd_deploy "$2" ;;
    render)  cmd_render "${2:-}" ;;
    full)    cmd_full "$2" "${3:-}" ;;
    *)
        echo "Usage: $0 {info|build|export|deploy|render|full} [args]"
        echo ""
        echo "  info                    Show Blender scene info"
        echo "  build script.py         Run modeling script in Blender"
        echo "  export model_name       Export current scene as GLB"
        echo "  deploy model_name       Copy GLB from Mini to assets/"
        echo "  render [output.png]     Render preview"
        echo "  full script.py [name]   Build + export + deploy"
        exit 1
        ;;
esac
