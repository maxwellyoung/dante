#!/usr/bin/env bash
# model_build.sh — Legacy compatibility wrapper over the canonical model pipeline
#
# Usage: ./scripts/model_build.sh script.py model_name [VERSION]
#
# The authoritative path is scripts/mcp_model.sh full:
#   build -> validate -> export -> deploy -> glb_qa -> registry validation
# This wrapper keeps older habits working and prints the same final outcome.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ENGINE_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

if [[ ! -f "${1:-}" ]]; then
    echo "Usage: $0 script.py model_name [VERSION]"
    exit 1
fi

SCRIPT="$1"
MODEL_NAME="${2:-$(basename "$SCRIPT" .py | sed 's/^build_//' | sed 's/^model_//' | sed 's/^exp_[0-9]*_//')}"
VERSION="${3:-1}"

echo "═══ BUILD: $MODEL_NAME (v${VERSION}) ═══"
echo ""

if ! VERSION="$VERSION" "$SCRIPT_DIR/mcp_model.sh" full "$SCRIPT" "$MODEL_NAME"; then
    echo ""
    echo "❌ BUILD FAILED"
    exit 1
fi

echo ""
echo "═══ DONE: $MODEL_NAME ═══"
echo "  Canonical pipeline completed via scripts/mcp_model.sh full"
