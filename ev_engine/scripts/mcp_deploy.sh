#!/usr/bin/env bash
# mcp_deploy.sh — Legacy convenience wrapper around mcp_model.sh + make
# Usage: ./scripts/mcp_deploy.sh script.py [model_name]
#
# This keeps older habits working, but the canonical flow now lives in
# scripts/mcp_model.sh:
#   build -> validate -> export -> deploy -> glb_qa -> registry validation
# This wrapper runs that full pipeline, then rebuilds the engine.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ENGINE_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

if [[ ! -f "${1:-}" ]]; then
    echo "Usage: $0 script.py [model_name]"
    exit 1
fi

SCRIPT="$1"
MODEL_NAME="${2:-$(basename "$SCRIPT" .py | sed 's/^exp_[0-9]*_//')}"

echo "═══ MCP Deploy Pipeline (Registry-Aware) ═══"
echo "  Script:  $SCRIPT"
echo "  Model:   $MODEL_NAME"
echo ""

echo "▸ Running Blender -> GLB -> QA -> registry pipeline..."
if ! "$SCRIPT_DIR/mcp_model.sh" full "$SCRIPT" "$MODEL_NAME"; then
    echo "✗ Pipeline failed."
    exit 1
fi

echo ""
echo "▸ Building engine..."
cd "$ENGINE_DIR"
if make 2>&1 | tail -3; then
    echo ""
    echo "═══ Pipeline Complete ═══"
    echo "  Model '$MODEL_NAME' is exported, QA'd, and ready for registry-backed scene use."
    echo ""
    echo "  Next step:"
    echo "    1. Add a ModelRegistryEntry in src/model_registry.c if this is new"
    echo "    2. Place it with find_model_asset(\"$MODEL_NAME\") + add_model()/add_shell()"
else
    echo "✗ Build failed."
    exit 1
fi
