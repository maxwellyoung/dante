#!/usr/bin/env bash
# mcp_deploy.sh — Full pipeline: model in Blender → export → deploy → build
# Usage: ./scripts/mcp_deploy.sh script.py [model_name]
#
# 1. Sends modeling script to Blender via MCP
# 2. Exports as GLB
# 3. Copies to assets/
# 4. Rebuilds the engine
#
# The model_name defaults to the script filename without extension.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ENGINE_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
MINI_HOST="${MINI_HOST:-mini-ts}"
PORT="${BLENDER_PORT:-9877}"

if [[ ! -f "${1:-}" ]]; then
    echo "Usage: $0 script.py [model_name]"
    exit 1
fi

SCRIPT="$1"
MODEL_NAME="${2:-$(basename "$SCRIPT" .py | sed 's/^exp_[0-9]*_//')}"

echo "═══ MCP Deploy Pipeline ═══"
echo "  Script:  $SCRIPT"
echo "  Model:   $MODEL_NAME"
echo ""

# ── Step 1: Model ──
echo "▸ Step 1: Building model in Blender..."
REMOTE_CODE="/tmp/ev_blender_code.py"
scp -q "$SCRIPT" "$MINI_HOST:$REMOTE_CODE"

# Ensure sender exists
ssh "$MINI_HOST" "test -f /tmp/ev_blender_sender.py" 2>/dev/null || \
bash "$SCRIPT_DIR/mcp_send.sh" "$SCRIPT" > /dev/null 2>&1 || true

RESULT=$(ssh -o ServerAliveInterval=15 "$MINI_HOST" "python3 /tmp/ev_blender_sender.py $REMOTE_CODE $PORT" 2>&1)
echo "  $RESULT"

if echo "$RESULT" | grep -q "ERROR"; then
    echo "✗ Modeling failed. Aborting."
    exit 1
fi

# ── Step 2: Export GLB ──
echo ""
echo "▸ Step 2: Exporting GLB..."
EXPORT_SCRIPT="/tmp/ev_blender_export.py"
ssh "$MINI_HOST" "cat > $EXPORT_SCRIPT" << EXPORT_EOF
import bpy, os
bpy.ops.object.select_all(action='SELECT')
bpy.ops.export_scene.gltf(
    filepath='/tmp/${MODEL_NAME}.glb',
    export_format='GLB',
    use_selection=True,
    export_apply=True,
    export_animations=False,
    export_yup=True,
)
size = os.path.getsize('/tmp/${MODEL_NAME}.glb')
print(f"Exported /tmp/${MODEL_NAME}.glb ({size} bytes, {size/1024:.1f} KB)")
EXPORT_EOF

EXPORT_RESULT=$(ssh -o ServerAliveInterval=15 "$MINI_HOST" \
    "python3 /tmp/ev_blender_sender.py $EXPORT_SCRIPT $PORT" 2>&1)
echo "  $EXPORT_RESULT"

if echo "$EXPORT_RESULT" | grep -q "ERROR"; then
    echo "✗ Export failed. Aborting."
    exit 1
fi

# ── Step 3: Deploy to assets/ ──
echo ""
echo "▸ Step 3: Copying to assets/..."
scp -q "$MINI_HOST:/tmp/${MODEL_NAME}.glb" "$ENGINE_DIR/assets/${MODEL_NAME}.glb"
SIZE=$(ls -la "$ENGINE_DIR/assets/${MODEL_NAME}.glb" | awk '{print $5}')
echo "  Deployed: assets/${MODEL_NAME}.glb ($SIZE bytes)"

# ── Step 4: Build ──
echo ""
echo "▸ Step 4: Building engine..."
cd "$ENGINE_DIR"
if make 2>&1 | tail -3; then
    echo ""
    echo "═══ Pipeline Complete ═══"
    echo "  Model '${MODEL_NAME}' is ready to use in scenes."
    echo ""
    echo "  Placement code:"
    echo "    int ${MODEL_NAME} = find_model_asset(\"${MODEL_NAME}\");"
    echo "    if (${MODEL_NAME} >= 0) {"
    echo "        add_model(s, 0, 0, 0, 1,1,1, 0, ${MODEL_NAME}, MAT_BRASS, WHITE);"
    echo "    }"
else
    echo "✗ Build failed."
    exit 1
fi
