#!/usr/bin/env bash
# model_build.sh — Single command: build on Mini → deploy → QA score
#
# Usage: ./scripts/model_build.sh /tmp/build_armchair_v4.py armchair [VERSION]
#        ./scripts/model_build.sh script.py model_name 3
#
# Steps:
#   1. scp script to Mini
#   2. Execute in Blender via MCP
#   3. scp GLB back to assets/
#   4. Run QA scoring
#
# Requires: Mini reachable (ssh mini-ts), Blender MCP on 9877

set -euo pipefail

SCRIPT="$1"
MODEL_NAME="${2:-$(basename "$SCRIPT" .py | sed 's/^build_//')}"
VERSION="${3:-1}"

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ENGINE_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
MINI="mini-ts"
PORT=9877

echo "═══ BUILD: $MODEL_NAME (v${VERSION}) ═══"
echo ""

# 1. Upload script
echo "▸ Uploading to Mini..."
scp -q "$SCRIPT" "$MINI:/tmp/build_${MODEL_NAME}.py"

# 2. Build in Blender
echo "▸ Building in Blender..."
RESULT=$(ssh -o ConnectTimeout=10 "$MINI" \
    "python3 /tmp/ev_blender_sender.py /tmp/build_${MODEL_NAME}.py $PORT" 2>&1)
echo "$RESULT" | grep -E "verts|tris|Exported|ERROR" || true

# Check for errors
if echo "$RESULT" | grep -q "ERROR\|error"; then
    echo ""
    echo "❌ BUILD FAILED"
    echo "$RESULT"
    exit 1
fi

# 3. Deploy
echo ""
echo "▸ Deploying to assets/..."
# Find the GLB export path from the script
EXPORT_PATH=$(grep "filepath=" "$SCRIPT" | grep "\.glb" | head -1 | sed "s/.*filepath=['\"]//;s/['\"].*//" )
if [ -n "$EXPORT_PATH" ]; then
    scp -q "$MINI:$EXPORT_PATH" "$ENGINE_DIR/assets/${MODEL_NAME}.glb"
else
    scp -q "$MINI:/tmp/${MODEL_NAME}*.glb" "$ENGINE_DIR/assets/${MODEL_NAME}.glb" 2>/dev/null || {
        echo "❌ Could not find GLB on Mini"
        exit 1
    }
fi
echo "  ✓ assets/${MODEL_NAME}.glb ($(stat -f%z "$ENGINE_DIR/assets/${MODEL_NAME}.glb") bytes)"

# 4. QA Score
echo ""
echo "▸ Scoring..."
VERSION="$VERSION" "$SCRIPT_DIR/glb_qa.sh" "assets/${MODEL_NAME}.glb" 2>&1 | grep -v "^═══ Screenshots"

echo ""
echo "═══ DONE: $MODEL_NAME ═══"
