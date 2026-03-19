#!/usr/bin/env bash
# mcp_model.sh — High-level model automation with quality gates
# Usage: ./scripts/mcp_model.sh build script.py        # Build model in Blender
#        ./scripts/mcp_model.sh export model_name       # Export GLB from current scene
#        ./scripts/mcp_model.sh deploy model_name       # Copy from Mini to assets/
#        ./scripts/mcp_model.sh info                    # Scene info
#        ./scripts/mcp_model.sh render out.png          # Render preview (1920x1200)
#        ./scripts/mcp_model.sh validate name           # Tri count + 4-angle renders
#        ./scripts/mcp_model.sh full script.py [name]   # Build + validate + export + deploy

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
bpy.context.scene.render.resolution_x = 1920
bpy.context.scene.render.resolution_y = 1200
bpy.ops.render.render(write_still=True)
print(f'Rendered to ${out}')
"
    if [[ "$out" == /tmp/* ]]; then
        scp -q "$MINI_HOST:$out" "/tmp/$(basename "$out")"
        echo "Preview saved to /tmp/$(basename "$out")"
    fi
}

cmd_validate() {
    local name="$1"
    local qa_dir="$ENGINE_DIR/qa/models"
    mkdir -p "$qa_dir"

    echo "▸ Validating $name..."

    # Get tri count + dimensions + material count from Blender
    send_code "
import bpy, bmesh

# Stats
total_tris = 0
total_verts = 0
mat_names = set()
min_b = [999,999,999]
max_b = [-999,-999,-999]

for obj in bpy.data.objects:
    if obj.type != 'MESH': continue
    me = obj.data
    bm = bmesh.new()
    bm.from_mesh(me)
    bmesh.ops.triangulate(bm, faces=bm.faces)
    total_tris += len(bm.faces)
    total_verts += len(bm.verts)
    bm.free()
    for slot in obj.material_slots:
        if slot.material:
            mat_names.add(slot.material.name)
    # Bounding box in world space
    for v in obj.bound_box:
        wv = obj.matrix_world @ bpy.types.Object.bl_rna.identifier  # skip complex math
        pass

# Simpler bounds via object dimensions
dims = [0,0,0]
for obj in bpy.data.objects:
    if obj.type != 'MESH': continue
    for i in range(3):
        dims[i] = max(dims[i], obj.dimensions[i])

print(f'TRIS: {total_tris}')
print(f'VERTS: {total_verts}')
print(f'MATERIALS: {len(mat_names)} ({", ".join(sorted(mat_names))})')
print(f'DIMENSIONS: {dims[0]:.2f} x {dims[1]:.2f} x {dims[2]:.2f} m')

# Budget check
budget_ok = total_tris <= 1600
print(f'BUDGET: {\"PASS\" if budget_ok else \"OVER\"} ({total_tris}/1600 tris)')
"

    # Render 4 angles (front, side, 3/4, top)
    echo "▸ Rendering validation angles..."
    send_code "
import bpy, math

# Setup camera for validation renders
scene = bpy.context.scene
scene.render.resolution_x = 960
scene.render.resolution_y = 960
scene.render.film_transparent = True

# Find model bounds center
objs = [o for o in bpy.data.objects if o.type == 'MESH']
if objs:
    cx = sum(o.location.x for o in objs) / len(objs)
    cy = sum(o.location.y for o in objs) / len(objs)
    cz = sum(o.location.z for o in objs) / len(objs)
    max_dim = max(max(o.dimensions) for o in objs)
else:
    cx, cy, cz, max_dim = 0, 0, 0, 2

cam_dist = max_dim * 2.5

# Get or create camera
cam = scene.camera
if not cam:
    bpy.ops.object.camera_add()
    cam = bpy.context.active_object
    scene.camera = cam
cam.data.lens = 50

angles = [
    ('front', 0, -cam_dist, max_dim * 0.4),
    ('side',  cam_dist, 0, max_dim * 0.4),
    ('quarter', cam_dist * 0.7, -cam_dist * 0.7, max_dim * 0.6),
    ('top',   0, -0.01, cam_dist),
]

for label, dx, dy, dz in angles:
    cam.location = (cx + dx, cy + dy, cz + dz)
    direction = (cx - cam.location.x, cy - cam.location.y, cz - cam.location.z)
    rot = bpy.types.Object.bl_rna  # placeholder
    # Track to center
    cam.rotation_euler = (0, 0, 0)
    constraint = cam.constraints.get('Track To') or cam.constraints.new(type='TRACK_TO')
    # Simple: use empty as target
    target = bpy.data.objects.get('_validate_target')
    if not target:
        bpy.ops.object.empty_add(location=(cx, cy, cz + max_dim * 0.3))
        target = bpy.context.active_object
        target.name = '_validate_target'
    constraint.target = target
    constraint.track_axis = 'TRACK_NEGATIVE_Z'
    constraint.up_axis = 'UP_Y'
    bpy.context.view_layer.update()

    scene.render.filepath = f'/tmp/ev_validate_${name}_{label}.png'
    bpy.ops.render.render(write_still=True)
    print(f'Rendered {label}: /tmp/ev_validate_${name}_{label}.png')

# Cleanup
if '_validate_target' in bpy.data.objects:
    bpy.data.objects.remove(bpy.data.objects['_validate_target'])
if 'Track To' in cam.constraints:
    cam.constraints.remove(cam.constraints['Track To'])
"

    # Copy renders locally
    for angle in front side quarter top; do
        scp -q "$MINI_HOST:/tmp/ev_validate_${name}_${angle}.png" \
            "$qa_dir/${name}_${angle}.png" 2>/dev/null && \
            echo "  ✓ ${name}_${angle}.png" || true
    done

    echo ""
    echo "═══ Validation renders: qa/models/${name}_*.png ═══"
    echo "Review these before deploying. Run 'deploy' to accept."
}

cmd_full() {
    local script="$1"
    local name="${2:-$(basename "$script" .py | sed 's/^model_//' | sed 's/^exp_[0-9]*_//')}"

    echo "═══ Full Pipeline: $name ═══"
    echo ""
    echo "▸ Building..."
    cmd_build "$script"
    echo ""
    echo "▸ Validating..."
    cmd_validate "$name"
    echo ""
    echo "▸ Exporting..."
    cmd_export "$name"
    echo ""
    echo "▸ Deploying..."
    cmd_deploy "$name"
    echo ""
    echo "═══ Done: assets/${name}.glb ═══"
    echo "Run 'make dev' to see it in-engine."
}

case "${1:-}" in
    info)     cmd_info ;;
    build)    cmd_build "$2" ;;
    export)   cmd_export "$2" ;;
    deploy)   cmd_deploy "$2" ;;
    render)   cmd_render "${2:-}" ;;
    validate) cmd_validate "$2" ;;
    full)     cmd_full "$2" "${3:-}" ;;
    *)
        echo "Usage: $0 {info|build|validate|export|deploy|render|full} [args]"
        echo ""
        echo "  info                    Show Blender scene info"
        echo "  build script.py         Run modeling script in Blender"
        echo "  validate model_name     Tri count + 4-angle renders → qa/models/"
        echo "  export model_name       Export current scene as GLB"
        echo "  deploy model_name       Copy GLB from Mini to assets/"
        echo "  render [output.png]     Render preview (1920x1200)"
        echo "  full script.py [name]   Build + validate + export + deploy"
        exit 1
        ;;
esac
