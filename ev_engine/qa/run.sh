#!/bin/bash
# ── EV Automated QA Pipeline ──────────────────────────────────────
# Build + screenshot + analyze + grade. No focus required.
#
# Usage:
#   ./qa/run.sh              # Full pipeline: build → screenshot → analyze → grade
#   ./qa/run.sh --baseline   # Save current as baseline for diffing
#   ./qa/run.sh --watch      # Re-run on source changes
#   ./qa/run.sh --diff       # Diff against baseline
#   ./qa/run.sh --grade      # Re-run analyzer only (no rebuild)
#   ./qa/run.sh --history    # Show past run scores
# ──────────────────────────────────────────────────────────────────

set -euo pipefail
cd "$(dirname "$0")/.."

SCREENSHOTS="qa/screenshots"
BASELINE="qa/baseline"
HISTORY="qa/history"
REPORT="qa/report.json"
GRADES="qa/grades.json"
ANALYZER="qa/analyze.py"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
CYAN='\033[0;36m'
DIM='\033[0;90m'
BOLD='\033[1m'
RESET='\033[0m'

# ── Functions ─────────────────────────────────────────────────────

build_qa() {
    printf "${DIM}Building QA binary...${RESET}\n"
    make -j4 -s qa 2>&1 | while IFS= read -r line; do
        if [[ "$line" == "[QA]"* ]]; then
            if [[ "$line" == *"FAIL"* ]] && [[ "$line" != *"FAIL "*"walls"* ]]; then
                printf "${RED}%s${RESET}\n" "$line"
            elif [[ "$line" == *"COMPOSITION"* ]] || [[ "$line" == *"LIGHTING"* ]] || \
                 [[ "$line" == *"MATERIAL"* ]] || [[ "$line" == *"COLOR"* ]] || \
                 [[ "$line" == *"ANTI-PATTERN"* ]] || [[ "$line" == *"INTERACTION"* ]] || \
                 [[ "$line" == *"BUDGET"* ]]; then
                printf "${YELLOW}    %s${RESET}\n" "$line"
            elif [[ "$line" == *"PASS"* ]]; then
                printf "${GREEN}%s${RESET}\n" "$line"
            elif [[ "$line" == *"==="* ]] || [[ "$line" == *"───"* ]]; then
                printf "${CYAN}%s${RESET}\n" "$line"
            else
                printf "%s\n" "$line"
            fi
        elif [[ "$line" == *"WARNING"* ]] || [[ "$line" == *"Error"* ]]; then
            printf "${DIM}%s${RESET}\n" "$line"
        fi
    done
    return ${PIPESTATUS[0]}
}

run_analyzer() {
    if [ -f "$REPORT" ]; then
        printf "\n${BOLD}${CYAN}── ANALYSIS ──${RESET}\n"
        python3 "$ANALYZER" "$REPORT"
        local exit_code=$?

        # Print grade summary
        if [ -f "$GRADES" ]; then
            local grade=$(python3 -c "import json; print(json.load(open('$GRADES'))['overall']['grade'])" 2>/dev/null || echo "?")
            local pct=$(python3 -c "import json; print(json.load(open('$GRADES'))['overall']['pct'])" 2>/dev/null || echo "?")
            if [[ "$grade" == "A" ]]; then
                printf "${GREEN}${BOLD}Overall: $grade ($pct%%)${RESET}\n"
            elif [[ "$grade" == "B" ]]; then
                printf "${GREEN}Overall: $grade ($pct%%)${RESET}\n"
            elif [[ "$grade" == "C" ]]; then
                printf "${YELLOW}Overall: $grade ($pct%%)${RESET}\n"
            else
                printf "${RED}Overall: $grade ($pct%%)${RESET}\n"
            fi
        fi
        return $exit_code
    else
        printf "${RED}No report found. Run QA first.${RESET}\n"
        return 1
    fi
}

save_baseline() {
    mkdir -p "$BASELINE"
    if [ -d "$SCREENSHOTS" ] && [ "$(ls -A $SCREENSHOTS 2>/dev/null)" ]; then
        cp "$SCREENSHOTS"/*.png "$BASELINE/"
        cp "$REPORT" "$BASELINE/report.json" 2>/dev/null || true
        cp "$GRADES" "$BASELINE/grades.json" 2>/dev/null || true
        printf "${GREEN}Baseline saved: %d screenshots${RESET}\n" "$(ls $BASELINE/*.png 2>/dev/null | wc -l)"
    else
        printf "${RED}No screenshots to baseline. Run QA first.${RESET}\n"
        exit 1
    fi
}

diff_screenshots() {
    if [ ! -d "$BASELINE" ]; then
        return
    fi

    printf "\n${CYAN}── Diff vs Baseline ──${RESET}\n"
    local changes=0 new=0 missing=0

    # Only diff primary screenshots (not hero/spawn variants)
    for img in "$SCREENSHOTS"/*_hero.png; do
        [ -f "$img" ] || continue
        name=$(basename "$img")
        base="$BASELINE/$name"
        if [ ! -f "$base" ]; then
            printf "${GREEN}  NEW:     %s${RESET}\n" "$name"
            ((new++)) || true
        elif ! cmp -s "$img" "$base"; then
            printf "${YELLOW}  CHANGED: %s${RESET}\n" "$name"
            ((changes++)) || true
        fi
    done

    for base_img in "$BASELINE"/*_hero.png; do
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
        printf "  %d changed, %d new, %d missing\n" "$changes" "$new" "$missing"
    fi

    # Grade comparison
    if [ -f "$BASELINE/grades.json" ] && [ -f "$GRADES" ]; then
        printf "\n${CYAN}── Grade Changes ──${RESET}\n"
        python3 -c "
import json
try:
    with open('$BASELINE/grades.json') as f: old = json.load(f)
    with open('$GRADES') as f: new = json.load(f)
    og = old['overall']
    ng = new['overall']
    if og['grade'] != ng['grade'] or og['pct'] != ng['pct']:
        arrow = '↑' if ng['pct'] > og['pct'] else '↓'
        print(f\"  Overall: {og['grade']} ({og['pct']}%) → {ng['grade']} ({ng['pct']}%) {arrow}\")
    for p in ['composition','lighting','materials','color','interaction','antipatterns']:
        oa = old.get('pillars',{}).get(p,{}).get('avg',0)
        na = new.get('pillars',{}).get(p,{}).get('avg',0)
        if abs(oa - na) >= 0.3:
            arrow = '↑' if na > oa else '↓'
            print(f'  {p:14s}  {oa:.1f} → {na:.1f} {arrow}')
    for name in sorted(set(list(old.get('scenes',{}).keys()) + list(new.get('scenes',{}).keys()))):
        os = old.get('scenes',{}).get(name,{})
        ns = new.get('scenes',{}).get(name,{})
        ot = os.get('total',0)
        nt = ns.get('total',0)
        if ot != nt:
            arrow = '↑' if nt > ot else '↓'
            print(f'  {name:18s}  {os.get(\"grade\",\"?\")} ({ot}) → {ns.get(\"grade\",\"?\")} ({nt}) {arrow}')
except Exception as e:
    print(f'  (diff error: {e})')
" 2>/dev/null || true
    fi
}

save_history() {
    mkdir -p "$HISTORY"
    local ts=$(date +%Y%m%d_%H%M%S)
    local dir="$HISTORY/$ts"
    mkdir -p "$dir"
    cp "$SCREENSHOTS"/*_hero.png "$dir/" 2>/dev/null || true
    cp "$REPORT" "$dir/report.json" 2>/dev/null || true
    cp "$GRADES" "$dir/grades.json" 2>/dev/null || true
    printf "${DIM}Archived: %s${RESET}\n" "$dir"

    local count=$(ls -d "$HISTORY"/20* 2>/dev/null | wc -l)
    if [ "$count" -gt 20 ]; then
        ls -d "$HISTORY"/20* | head -n $(( count - 20 )) | xargs rm -rf
    fi
}

watch_mode() {
    printf "${CYAN}Watching for changes (Ctrl+C to stop)...${RESET}\n\n"
    local last_hash=""
    while true; do
        local hash=$(cat src/*.c src/*.h 2>/dev/null | md5 2>/dev/null || md5sum src/*.c src/*.h 2>/dev/null | md5sum)
        if [ "$hash" != "$last_hash" ]; then
            last_hash="$hash"
            printf "\n${CYAN}── Change detected: $(date +%H:%M:%S) ──${RESET}\n"
            build_qa || true
            run_analyzer || true
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
        run_analyzer || true
        save_baseline
        ;;
    --watch|-w)
        watch_mode
        ;;
    --diff|-d)
        diff_screenshots
        ;;
    --grade|-g)
        run_analyzer
        ;;
    --history|-h)
        if [ -d "$HISTORY" ]; then
            printf "${CYAN}QA History:${RESET}\n"
            for dir in $(ls -d "$HISTORY"/20* 2>/dev/null | tail -10); do
                ts=$(basename "$dir")
                if [ -f "$dir/grades.json" ]; then
                    grade=$(python3 -c "import json; g=json.load(open('$dir/grades.json')); print(f\"{g['overall']['grade']} ({g['overall']['pct']}%)\")" 2>/dev/null || echo "?")
                else
                    grade="?"
                fi
                printf "  %s  %s\n" "$ts" "$grade"
            done
        else
            printf "${YELLOW}No history yet.${RESET}\n"
        fi
        ;;
    run|"")
        build_qa || true
        run_analyzer || true
        diff_screenshots
        save_history
        ;;
    *)
        printf "Usage: ./qa/run.sh [--baseline|--watch|--diff|--grade|--history]\n"
        exit 1
        ;;
esac
