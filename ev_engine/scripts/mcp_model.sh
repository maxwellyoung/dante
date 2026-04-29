#!/usr/bin/env bash
# mcp_model.sh — High-level model automation with quality gates
# Usage: ./scripts/mcp_model.sh build script.py         # Build model in Blender
#        ./scripts/mcp_model.sh export model_name       # Export GLB from current scene
#        ./scripts/mcp_model.sh deploy model_name       # Copy from Mini to assets/
#        ./scripts/mcp_model.sh info                    # Scene info
#        ./scripts/mcp_model.sh render out.png          # Render preview (1920x1200)
#        ./scripts/mcp_model.sh validate name           # Blender tri/scale + 4-angle renders
#        ./scripts/mcp_model.sh qa model_name           # Run GLB visual QA on deployed asset
#        ./scripts/mcp_model.sh registry-check          # Validate scene/registry/assets sync
#        ./scripts/mcp_model.sh full script.py [name]   # Build + validate + export + deploy + QA + registry check

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ENGINE_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
MINI_HOST="${MINI_HOST:-mini-ts}"
PORT="${BLENDER_PORT:-9877}"
LOCAL_BLENDER="${LOCAL_BLENDER:-/opt/homebrew/bin/blender}"
PREFER_REMOTE_BLENDER="${PREFER_REMOTE_BLENDER:-0}"

have_local_blender() {
    [[ -x "$LOCAL_BLENDER" ]]
}

remote_bridge_ready() {
    ssh -o BatchMode=yes -o ConnectTimeout=5 "$MINI_HOST" \
        "python3 -c \"import socket, sys; s = socket.socket(); s.settimeout(3); code = s.connect_ex(('localhost', ${PORT})); s.close(); sys.exit(0 if code == 0 else 1)\"" \
        >/dev/null 2>&1
}

remote_output_has_error() {
    local output="$1"
    grep -qE 'Traceback|ConnectionRefusedError|^ERROR:' <<< "$output"
}

cmd_full_local() {
    local script="$1"
    local name="$2"
    local qa_dir="$ENGINE_DIR/qa/models"
    local wrapper
    local script_abs
    local local_export="/tmp/${name}.glb"

    mkdir -p "$qa_dir"
    script_abs="$(cd "$(dirname "$script")" && pwd)/$(basename "$script")"
    wrapper="$(mktemp /tmp/ev_local_full_XXXXXX.py)"

    cat > "$wrapper" <<'PY'
import bmesh
import bpy
import os
import runpy
import sys
from mathutils import Vector

script_path = sys.argv[-1]
name = os.environ["EV_MODEL_NAME"]
export_path = os.environ["EV_EXPORT_PATH"]

os.environ["EV_EXPORT_PATH"] = export_path
runpy.run_path(script_path, run_name="__main__")

if not os.path.exists(export_path):
    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.export_scene.gltf(
        filepath=export_path,
        export_format='GLB',
        export_yup=True,
        export_apply=True,
        export_animations=True,
    )
    print(f"[EV] Exported fallback GLB: {export_path}")

mesh_objects = [obj for obj in bpy.data.objects if obj.type == 'MESH']
depsgraph = bpy.context.evaluated_depsgraph_get()

total_tris = 0
total_verts = 0
mat_names = set()
min_b = Vector((float('inf'), float('inf'), float('inf')))
max_b = Vector((float('-inf'), float('-inf'), float('-inf')))

for obj in mesh_objects:
    eval_obj = obj.evaluated_get(depsgraph)
    mesh = eval_obj.to_mesh()

    bm = bmesh.new()
    bm.from_mesh(mesh)
    bmesh.ops.triangulate(bm, faces=bm.faces)
    total_tris += len(bm.faces)
    total_verts += len(bm.verts)
    bm.free()

    eval_obj.to_mesh_clear()

    for slot in obj.material_slots:
        if slot.material:
            mat_names.add(slot.material.name)

    for corner in obj.bound_box:
        world_corner = obj.matrix_world @ Vector(corner)
        min_b.x = min(min_b.x, world_corner.x)
        min_b.y = min(min_b.y, world_corner.y)
        min_b.z = min(min_b.z, world_corner.z)
        max_b.x = max(max_b.x, world_corner.x)
        max_b.y = max(max_b.y, world_corner.y)
        max_b.z = max(max_b.z, world_corner.z)

if mesh_objects:
    dims = max_b - min_b
else:
    dims = Vector((0.0, 0.0, 0.0))

print(f"TRIS: {total_tris}")
print(f"VERTS: {total_verts}")
print(f"MATERIALS: {len(mat_names)} ({', '.join(sorted(mat_names))})")
print(f"DIMENSIONS: {dims.x:.2f} x {dims.y:.2f} x {dims.z:.2f} m")
print(f"BUDGET: {'PASS' if total_tris <= 1600 else 'OVER'} ({total_tris}/1600 tris)")

scene = bpy.context.scene
scene.render.resolution_x = 960
scene.render.resolution_y = 960
scene.render.film_transparent = True

if mesh_objects:
    center = (min_b + max_b) * 0.5
    max_dim = max(dims.x, dims.y, dims.z, 1.0)
else:
    center = Vector((0.0, 0.0, 0.0))
    max_dim = 2.0

cam_dist = max_dim * 2.5
cam = scene.camera
if not cam:
    bpy.ops.object.camera_add()
    cam = bpy.context.active_object
    scene.camera = cam
cam.data.lens = 50
cam.data.clip_start = 0.01
cam.data.clip_end = 500.0

angles = [
    ("front", Vector((0.0, -cam_dist, max_dim * 0.35))),
    ("side", Vector((cam_dist, 0.0, max_dim * 0.35))),
    ("quarter", Vector((cam_dist * 0.7, -cam_dist * 0.7, max_dim * 0.55))),
    ("top", Vector((0.01, -0.01, cam_dist))),
]
target = center + Vector((0.0, 0.0, max_dim * 0.1))

for label, offset in angles:
    cam.location = center + offset
    direction = target - cam.location
    cam.rotation_euler = direction.to_track_quat('-Z', 'Y').to_euler()
    bpy.context.view_layer.update()

    render_path = f"/tmp/ev_validate_{name}_{label}.png"
    scene.render.filepath = render_path
    bpy.ops.render.render(write_still=True)
    print(f"Rendered {label}: {render_path}")
PY

    echo "▸ Using local headless Blender..."
    EV_MODEL_NAME="$name" EV_EXPORT_PATH="$local_export" \
        "$LOCAL_BLENDER" -b --python "$wrapper" -- "$script_abs"
    rm -f "$wrapper"

    for angle in front side quarter top; do
        if [[ -f "/tmp/ev_validate_${name}_${angle}.png" ]]; then
            cp "/tmp/ev_validate_${name}_${angle}.png" "$qa_dir/${name}_${angle}.png"
            echo "  ✓ ${name}_${angle}.png"
        fi
    done

    cp "$local_export" "$ENGINE_DIR/assets/${name}.glb"
    local size
    size=$(stat -f%z "$ENGINE_DIR/assets/${name}.glb" 2>/dev/null || echo "?")
    echo "Deployed: assets/${name}.glb ($size bytes)"

    echo ""
    echo "▸ GLB visual QA..."
    cmd_qa "$name"
    echo ""
    echo "▸ Registry validation..."
    cmd_registry_check
    echo ""
    echo "═══ Done: assets/${name}.glb ═══"
    echo "Run 'make dev' or 'make qa' to see it in-engine."
}

ensure_sender() {
    local sender_tmp
    sender_tmp=$(mktemp /tmp/ev_sender_XXXXXX.py)
    cat > "$sender_tmp" << 'SENDER'
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
    scp -q "$sender_tmp" "$MINI_HOST:/tmp/ev_blender_sender.py"
    rm -f "$sender_tmp"
}

send_code() {
    local code="$1"
    local tmpfile=$(mktemp /tmp/ev_mcp_XXXXXX.py)
    local output
    echo "$code" > "$tmpfile"
    scp -q "$tmpfile" "$MINI_HOST:/tmp/ev_mcp_code.py"
    rm -f "$tmpfile"
    ensure_sender

    output=$(ssh -o ServerAliveInterval=15 -o ServerAliveCountMax=8 \
        "$MINI_HOST" "python3 /tmp/ev_blender_sender.py /tmp/ev_mcp_code.py $PORT" 2>&1) || {
        printf '%s\n' "$output" >&2
        return 1
    }

    printf '%s\n' "$output"
    if remote_output_has_error "$output"; then
        return 1
    fi
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
    local output
    scp -q "$script" "$MINI_HOST:/tmp/ev_mcp_code.py"
    ensure_sender
    output=$(ssh -o ServerAliveInterval=15 "$MINI_HOST" \
        "python3 /tmp/ev_blender_sender.py /tmp/ev_mcp_code.py $PORT" 2>&1) || {
        printf '%s\n' "$output" >&2
        return 1
    }

    printf '%s\n' "$output"
    if remote_output_has_error "$output"; then
        return 1
    fi
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
from mathutils import Vector

mesh_objects = [obj for obj in bpy.data.objects if obj.type == 'MESH']
depsgraph = bpy.context.evaluated_depsgraph_get()

total_tris = 0
total_verts = 0
mat_names = set()
min_b = Vector((float('inf'), float('inf'), float('inf')))
max_b = Vector((float('-inf'), float('-inf'), float('-inf')))

for obj in mesh_objects:
    eval_obj = obj.evaluated_get(depsgraph)
    mesh = eval_obj.to_mesh()

    bm = bmesh.new()
    bm.from_mesh(mesh)
    bmesh.ops.triangulate(bm, faces=bm.faces)
    total_tris += len(bm.faces)
    total_verts += len(bm.verts)
    bm.free()

    eval_obj.to_mesh_clear()

    for slot in obj.material_slots:
        if slot.material:
            mat_names.add(slot.material.name)

    for corner in obj.bound_box:
        world_corner = obj.matrix_world @ Vector(corner)
        min_b.x = min(min_b.x, world_corner.x)
        min_b.y = min(min_b.y, world_corner.y)
        min_b.z = min(min_b.z, world_corner.z)
        max_b.x = max(max_b.x, world_corner.x)
        max_b.y = max(max_b.y, world_corner.y)
        max_b.z = max(max_b.z, world_corner.z)

if mesh_objects:
    dims = max_b - min_b
else:
    dims = Vector((0.0, 0.0, 0.0))

print(f'TRIS: {total_tris}')
print(f'VERTS: {total_verts}')
print(f'MATERIALS: {len(mat_names)} ({\", \".join(sorted(mat_names))})')
print(f'DIMENSIONS: {dims.x:.2f} x {dims.y:.2f} x {dims.z:.2f} m')

budget_ok = total_tris <= 1600
print(f'BUDGET: {\"PASS\" if budget_ok else \"OVER\"} ({total_tris}/1600 tris)')
"

    # Render 4 angles (front, side, 3/4, top)
    echo "▸ Rendering validation angles..."
    send_code "
import bpy
from mathutils import Vector

scene = bpy.context.scene
scene.render.resolution_x = 960
scene.render.resolution_y = 960
scene.render.film_transparent = True

mesh_objects = [obj for obj in bpy.data.objects if obj.type == 'MESH']
min_b = Vector((float('inf'), float('inf'), float('inf')))
max_b = Vector((float('-inf'), float('-inf'), float('-inf')))

for obj in mesh_objects:
    for corner in obj.bound_box:
        world_corner = obj.matrix_world @ Vector(corner)
        min_b.x = min(min_b.x, world_corner.x)
        min_b.y = min(min_b.y, world_corner.y)
        min_b.z = min(min_b.z, world_corner.z)
        max_b.x = max(max_b.x, world_corner.x)
        max_b.y = max(max_b.y, world_corner.y)
        max_b.z = max(max_b.z, world_corner.z)

if mesh_objects:
    center = (min_b + max_b) * 0.5
    dims = max_b - min_b
    max_dim = max(dims.x, dims.y, dims.z, 1.0)
else:
    center = Vector((0.0, 0.0, 0.0))
    max_dim = 2.0

cam_dist = max_dim * 2.5

cam = scene.camera
if not cam:
    bpy.ops.object.camera_add()
    cam = bpy.context.active_object
    scene.camera = cam
cam.data.lens = 50
cam.data.clip_start = 0.01
cam.data.clip_end = 500.0

angles = [
    ('front', Vector((0.0, -cam_dist, max_dim * 0.35))),
    ('side', Vector((cam_dist, 0.0, max_dim * 0.35))),
    ('quarter', Vector((cam_dist * 0.7, -cam_dist * 0.7, max_dim * 0.55))),
    ('top', Vector((0.01, -0.01, cam_dist))),
]

target = center + Vector((0.0, 0.0, max_dim * 0.1))

for label, offset in angles:
    cam.location = center + offset
    direction = target - cam.location
    cam.rotation_euler = direction.to_track_quat('-Z', 'Y').to_euler()
    bpy.context.view_layer.update()

    scene.render.filepath = f'/tmp/ev_validate_${name}_{label}.png'
    bpy.ops.render.render(write_still=True)
    print(f'Rendered {label}: /tmp/ev_validate_${name}_{label}.png')
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

cmd_qa() {
    local name="$1"
    local asset_path="assets/${name}.glb"

    if [[ ! -f "$ENGINE_DIR/$asset_path" ]]; then
        echo "Missing deployed asset: $asset_path"
        echo "Run '$0 deploy ${name}' first."
        exit 1
    fi

    (
        cd "$ENGINE_DIR"
        ./scripts/glb_qa.sh "$asset_path"
    )
}

cmd_registry_check() {
    (
        cd "$ENGINE_DIR"
        python3 scripts/validate_model_registry.py
    )
}

cmd_full() {
    local script="$1"
    local name="${2:-$(basename "$script" .py | sed 's/^model_//' | sed 's/^exp_[0-9]*_//')}"

    echo "═══ Full Pipeline: $name ═══"
    echo ""
    if have_local_blender && [[ "$PREFER_REMOTE_BLENDER" != "1" ]]; then
        cmd_full_local "$script" "$name"
        return
    fi

    if ! remote_bridge_ready; then
        if have_local_blender; then
            cmd_full_local "$script" "$name"
            return
        fi
        echo "Remote Blender bridge is unavailable and no local Blender fallback is available."
        exit 1
    fi

    echo "▸ Building..."
    if ! cmd_build "$script"; then
        if have_local_blender; then
            echo ""
            cmd_full_local "$script" "$name"
            return
        fi
        echo ""
        echo "Remote Blender build failed and no local Blender fallback is available."
        exit 1
    fi
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
    echo "▸ GLB visual QA..."
    cmd_qa "$name"
    echo ""
    echo "▸ Registry validation..."
    cmd_registry_check
    echo ""
    echo "═══ Done: assets/${name}.glb ═══"
    echo "Run 'make dev' or 'make qa' to see it in-engine."
}

case "${1:-}" in
    info)     cmd_info ;;
    build)    cmd_build "$2" ;;
    export)   cmd_export "$2" ;;
    deploy)   cmd_deploy "$2" ;;
    render)   cmd_render "${2:-}" ;;
    validate) cmd_validate "$2" ;;
    qa)       cmd_qa "$2" ;;
    registry-check) cmd_registry_check ;;
    full)     cmd_full "$2" "${3:-}" ;;
    *)
        echo "Usage: $0 {info|build|validate|export|deploy|qa|registry-check|render|full} [args]"
        echo ""
        echo "  info                    Show Blender scene info"
        echo "  build script.py         Run modeling script in Blender"
        echo "  validate model_name     Tri count + 4-angle renders → qa/models/"
        echo "  export model_name       Export current scene as GLB"
        echo "  deploy model_name       Copy GLB from Mini to assets/"
        echo "  qa model_name           Run GLB visual QA on assets/model_name.glb"
        echo "  registry-check          Validate scene/registry/assets sync"
        echo "  render [output.png]     Render preview (1920x1200)"
        echo "  full script.py [name]   Build + validate + export + deploy + QA + registry"
        echo "                          Prefers local headless Blender; set PREFER_REMOTE_BLENDER=1 to use Mini first"
        exit 1
        ;;
esac
