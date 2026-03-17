#!/bin/bash
# ── RALPH: Screenshot all scenes via QA mode ──────────────────────
# Builds with QA_MODE, runs the binary, captures hero+spawn screenshots.
# This wraps `make qa` which already handles the full pipeline.
#
# Usage:
#   ./scripts/screenshot-scenes.sh           # Build + screenshot all scenes
#   ./scripts/screenshot-scenes.sh --skip-build  # Re-run without rebuilding
set -euo pipefail
cd "$(dirname "$0")/.."

SCREENSHOTS="qa/screenshots"
CYAN='\033[0;36m'
GREEN='\033[0;32m'
RED='\033[0;31m'
DIM='\033[0;90m'
RESET='\033[0m'

printf "${CYAN}[RALPH] ── Screenshot Scenes ──${RESET}\n"

if [[ "${1:-}" != "--skip-build" ]]; then
    printf "${DIM}[RALPH] Building QA binary...${RESET}\n"
    # Clean old QA objects to force fresh build
    rm -f src/*.qa.o
fi

mkdir -p "$SCREENSHOTS"

# `make qa` builds with -DQA_MODE, runs the binary, and exits.
# The QA_MODE block in main.c iterates all scenes, captures hero+spawn PNGs,
# writes qa/report.json, and exits cleanly.
if make -j4 qa 2>&1 | tee /tmp/ev_qa_build.log; then
    count=$(ls -1 "$SCREENSHOTS"/*.png 2>/dev/null | wc -l | tr -d ' ')
    printf "${GREEN}[RALPH] Screenshots captured: %s files in %s/${RESET}\n" "$count" "$SCREENSHOTS"
else
    printf "${RED}[RALPH] QA build/run failed. See output above.${RESET}\n"
    exit 1
fi
