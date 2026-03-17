#!/bin/bash
# ── RALPH: Visual regression checker ──────────────────────────────
# Compares latest screenshots against baseline using ImageMagick.
#
# Usage:
#   ./scripts/check-regression.sh              # Diff against baseline
#   ./scripts/check-regression.sh --update-baseline  # Set current as baseline
set -euo pipefail
cd "$(dirname "$0")/.."

SCREENSHOTS="qa/screenshots"
BASELINE="qa/baseline"
DIFF_DIR="qa/diffs"
CYAN='\033[0;36m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
RED='\033[0;31m'
DIM='\033[0;90m'
RESET='\033[0m'

# ── Update baseline mode ──
if [[ "${1:-}" == "--update-baseline" ]]; then
    if [ ! -d "$SCREENSHOTS" ] || [ -z "$(ls -A "$SCREENSHOTS" 2>/dev/null)" ]; then
        printf "${RED}[RALPH] No screenshots to baseline. Run screenshot-scenes.sh first.${RESET}\n"
        exit 1
    fi
    mkdir -p "$BASELINE"
    cp "$SCREENSHOTS"/*_hero.png "$BASELINE/" 2>/dev/null || true
    cp "$SCREENSHOTS"/*_spawn.png "$BASELINE/" 2>/dev/null || true
    count=$(ls -1 "$BASELINE"/*.png 2>/dev/null | wc -l | tr -d ' ')
    printf "${GREEN}[RALPH] Baseline updated: %s images in %s/${RESET}\n" "$count" "$BASELINE"
    exit 0
fi

# ── Diff mode ──
printf "${CYAN}[RALPH] ── Visual Regression Check ──${RESET}\n"

if [ ! -d "$BASELINE" ] || [ -z "$(ls -A "$BASELINE" 2>/dev/null)" ]; then
    printf "${YELLOW}[RALPH] No baseline found. Establishing baseline from current screenshots...${RESET}\n"
    if [ ! -d "$SCREENSHOTS" ] || [ -z "$(ls -A "$SCREENSHOTS" 2>/dev/null)" ]; then
        printf "${RED}[RALPH] No screenshots either. Run screenshot-scenes.sh first.${RESET}\n"
        exit 1
    fi
    mkdir -p "$BASELINE"
    cp "$SCREENSHOTS"/*_hero.png "$BASELINE/" 2>/dev/null || true
    cp "$SCREENSHOTS"/*_spawn.png "$BASELINE/" 2>/dev/null || true
    printf "${GREEN}[RALPH] Baseline established. Run again after changes to compare.${RESET}\n"
    exit 0
fi

if ! command -v compare &>/dev/null; then
    printf "${RED}[RALPH] ImageMagick 'compare' not found. Install: brew install imagemagick${RESET}\n"
    exit 1
fi

mkdir -p "$DIFF_DIR"

total=0
passed=0
failed=0
new_count=0
missing=0
results=""

# Compare hero shots
for baseline_img in "$BASELINE"/*_hero.png; do
    [ -f "$baseline_img" ] || continue
    name=$(basename "$baseline_img")
    scene_name="${name%_hero.png}"
    current="$SCREENSHOTS/$name"
    total=$((total + 1))

    if [ ! -f "$current" ]; then
        results="${results}  ${RED}MISSING${RESET}  ${scene_name}\n"
        missing=$((missing + 1))
        continue
    fi

    # Compute pixel difference (AE = absolute error count)
    diff_file="$DIFF_DIR/${scene_name}_diff.png"
    # compare returns 1 if images differ, 2 on error
    ae=$(compare -metric AE "$baseline_img" "$current" "$diff_file" 2>&1 || true)
    # Extract numeric value (compare outputs to stderr)
    ae_num=$(echo "$ae" | grep -oE '^[0-9]+' || echo "0")

    # Get total pixels for percentage
    total_px=$((480 * 300))
    if [ "$ae_num" -gt 0 ]; then
        pct=$(echo "scale=2; $ae_num * 100 / $total_px" | bc 2>/dev/null || echo "?")
    else
        pct="0"
    fi

    # Threshold: <1% pixel diff = pass
    if [ "$ae_num" -eq 0 ]; then
        results="${results}  ${GREEN}PASS${RESET}     ${scene_name}  (identical)\n"
        passed=$((passed + 1))
        rm -f "$diff_file"
    elif [ "$ae_num" -lt 1440 ]; then  # 1% of 480*300
        results="${results}  ${GREEN}PASS${RESET}     ${scene_name}  (${pct}% diff, ${ae_num}px)\n"
        passed=$((passed + 1))
    else
        results="${results}  ${RED}FAIL${RESET}     ${scene_name}  (${pct}% diff, ${ae_num}px)  → diffs/${scene_name}_diff.png\n"
        failed=$((failed + 1))
    fi
done

# Check for new screenshots not in baseline
for current_img in "$SCREENSHOTS"/*_hero.png; do
    [ -f "$current_img" ] || continue
    name=$(basename "$current_img")
    if [ ! -f "$BASELINE/$name" ]; then
        scene_name="${name%_hero.png}"
        results="${results}  ${YELLOW}NEW${RESET}      ${scene_name}\n"
        new_count=$((new_count + 1))
    fi
done

printf "%b" "$results"
printf "\n${CYAN}Summary:${RESET} %d total | ${GREEN}%d passed${RESET} | ${RED}%d failed${RESET} | ${YELLOW}%d new${RESET} | %d missing\n" \
    "$total" "$passed" "$failed" "$new_count" "$missing"

# Exit code: 0 if all passed, 1 if any failed/missing
[ "$failed" -eq 0 ] && [ "$missing" -eq 0 ]
