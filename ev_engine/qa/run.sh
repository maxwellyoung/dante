#!/bin/bash
# ── EV Automated QA Pipeline ──────────────────────────────────────
# Builds, runs QA, saves timestamped results, diffs against baseline.
# No focus required — window is unfocused + always-run.
#
# Usage:
#   ./qa/run.sh              # Run QA, show results
#   ./qa/run.sh --baseline   # Save current screenshots as baseline
#   ./qa/run.sh --watch      # Re-run every 30s after changes
#   ./qa/run.sh --diff       # Only show diff against baseline
# ──────────────────────────────────────────────────────────────────

set -euo pipefail
cd "$(dirname "$0")/.."

SCREENSHOTS="qa/screenshots"
BASELINE="qa/baseline"
HISTORY="qa/history"
REPORT="qa/report.json"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
CYAN='\033[0;36m'
DIM='\033[0;90m'
RESET='\033[0m'

# ── Functions ─────────────────────────────────────────────────────

build_qa() {
    printf "${DIM}Building QA binary...${RESET}\n"
    make -j4 -s qa 2>&1 | while IFS= read -r line; do
        # Filter out Raylib init noise, keep QA output
        if [[ "$line" == "[QA]"* ]]; then
            # Color code based on content
            if [[ "$line" == *"FAIL"* ]]; then
                printf "${RED}%s${RESET}\n" "$line"
            elif [[ "$line" == *"WARN"* ]]; then
                printf "${YELLOW}%s${RESET}\n" "$line"
            elif [[ "$line" == *"PASS"* ]]; then
                printf "${GREEN}%s${RESET}\n" "$line"
            elif [[ "$line" == *"==="* ]]; then
                printf "${CYAN}%s${RESET}\n" "$line"
            else
                printf "%s\n" "$line"
            fi
        elif [[ "$line" == *"WARNING"* ]] || [[ "$line" == *"Error"* ]]; then
            printf "${YELLOW}%s${RESET}\n" "$line"
        fi
    done
    return ${PIPESTATUS[0]}
}

save_baseline() {
    mkdir -p "$BASELINE"
    if [ -d "$SCREENSHOTS" ] && [ "$(ls -A $SCREENSHOTS 2>/dev/null)" ]; then
        cp "$SCREENSHOTS"/*.png "$BASELINE/"
        cp "$REPORT" "$BASELINE/report.json" 2>/dev/null || true
        printf "${GREEN}Baseline saved: %d screenshots${RESET}\n" "$(ls $BASELINE/*.png 2>/dev/null | wc -l)"
    else
        printf "${RED}No screenshots to baseline. Run QA first.${RESET}\n"
        exit 1
    fi
}

diff_screenshots() {
    if [ ! -d "$BASELINE" ]; then
        printf "${YELLOW}No baseline found. Run: ./qa/run.sh --baseline${RESET}\n"
        return
    fi

    printf "\n${CYAN}── Screenshot Diff vs Baseline ──${RESET}\n"
    local changes=0
    local new=0
    local missing=0

    for img in "$SCREENSHOTS"/*.png; do
        name=$(basename "$img")
        base="$BASELINE/$name"
        if [ ! -f "$base" ]; then
            printf "${GREEN}  NEW:     %s${RESET}\n" "$name"
            ((new++)) || true
        elif ! cmp -s "$img" "$base"; then
            # Calculate byte-level difference percentage
            local size_a=$(stat -f%z "$img" 2>/dev/null || stat -c%s "$img" 2>/dev/null)
            local size_b=$(stat -f%z "$base" 2>/dev/null || stat -c%s "$base" 2>/dev/null)
            local diff_pct=$(( (size_a > size_b ? size_a - size_b : size_b - size_a) * 100 / (size_a > 0 ? size_a : 1) ))
            printf "${YELLOW}  CHANGED: %s (size delta: %d%%)${RESET}\n" "$name" "$diff_pct"
            ((changes++)) || true
        fi
    done

    # Check for removed scenes
    for base_img in "$BASELINE"/*.png; do
        [ -f "$base_img" ] || continue
        name=$(basename "$base_img")
        if [ ! -f "$SCREENSHOTS/$name" ]; then
            printf "${RED}  MISSING: %s${RESET}\n" "$name"
            ((missing++)) || true
        fi
    done

    if [ $changes -eq 0 ] && [ $new -eq 0 ] && [ $missing -eq 0 ]; then
        printf "${GREEN}  No visual changes detected.${RESET}\n"
    else
        printf "\n  %d changed, %d new, %d missing\n" "$changes" "$new" "$missing"
    fi

    # JSON report diff
    if [ -f "$BASELINE/report.json" ] && [ -f "$REPORT" ]; then
        printf "\n${CYAN}── Metric Changes ──${RESET}\n"
        # Compare key metrics using python (available on macOS)
        python3 -c "
import json, sys
try:
    with open('$BASELINE/report.json') as f: old = json.load(f)
    with open('$REPORT') as f: new = json.load(f)
    old_scenes = {s['name']: s for s in old['scenes']}
    new_scenes = {s['name']: s for s in new['scenes']}
    for name, ns in sorted(new_scenes.items()):
        if name not in old_scenes: continue
        os = old_scenes[name]
        diffs = []
        for key in ['luma', 'black_pct', 'color_variance', 'walls', 'issues']:
            if key in ns and key in os:
                ov, nv = os[key], ns[key]
                if isinstance(nv, (int, float)) and ov != nv:
                    delta = nv - ov
                    sign = '+' if delta > 0 else ''
                    diffs.append(f'{key}: {ov}->{nv} ({sign}{delta:.0f})')
        if diffs:
            print(f'  {name}: {\"  \".join(diffs)}')
    old_total = old.get('total_issues', 0)
    new_total = new.get('total_issues', 0)
    if old_total != new_total:
        delta = new_total - old_total
        arrow = '↑' if delta > 0 else '↓'
        print(f'\n  Total issues: {old_total} -> {new_total} {arrow}')
except Exception as e:
    print(f'  (diff unavailable: {e})')
" 2>/dev/null || printf "  ${DIM}(python3 required for metric diff)${RESET}\n"
    fi
}

save_history() {
    mkdir -p "$HISTORY"
    local ts=$(date +%Y%m%d_%H%M%S)
    local dir="$HISTORY/$ts"
    mkdir -p "$dir"
    cp "$SCREENSHOTS"/*.png "$dir/" 2>/dev/null || true
    cp "$REPORT" "$dir/report.json" 2>/dev/null || true
    printf "${DIM}Archived: %s${RESET}\n" "$dir"

    # Keep only last 20 runs
    local count=$(ls -d "$HISTORY"/20* 2>/dev/null | wc -l)
    if [ "$count" -gt 20 ]; then
        ls -d "$HISTORY"/20* | head -n $(( count - 20 )) | xargs rm -rf
    fi
}

watch_mode() {
    printf "${CYAN}Watching for changes (Ctrl+C to stop)...${RESET}\n\n"
    local last_hash=""
    while true; do
        # Hash all source files
        local hash=$(cat src/*.c src/*.h 2>/dev/null | md5 2>/dev/null || md5sum src/*.c src/*.h 2>/dev/null | md5sum)
        if [ "$hash" != "$last_hash" ]; then
            last_hash="$hash"
            printf "\n${CYAN}── Change detected: $(date +%H:%M:%S) ──${RESET}\n"
            build_qa || true
            diff_screenshots
            save_history
        fi
        sleep 5
    done
}

# ── Main ──────────────────────────────────────────────────────────

case "${1:-run}" in
    --baseline|-b)
        build_qa || true
        save_baseline
        ;;
    --watch|-w)
        watch_mode
        ;;
    --diff|-d)
        diff_screenshots
        ;;
    --history|-h)
        if [ -d "$HISTORY" ]; then
            printf "${CYAN}QA History:${RESET}\n"
            for dir in $(ls -d "$HISTORY"/20* 2>/dev/null | tail -10); do
                ts=$(basename "$dir")
                issues=$(python3 -c "import json; print(json.load(open('$dir/report.json'))['total_issues'])" 2>/dev/null || echo "?")
                printf "  %s  issues: %s\n" "$ts" "$issues"
            done
        else
            printf "${YELLOW}No history yet.${RESET}\n"
        fi
        ;;
    run|"")
        build_qa || true
        diff_screenshots
        save_history
        ;;
    *)
        printf "Usage: ./qa/run.sh [--baseline|--watch|--diff|--history]\n"
        exit 1
        ;;
esac
