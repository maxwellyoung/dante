#!/bin/bash
# ── EV Full E2E QA Pipeline ─────────────────────────────────────────
# Multi-angle screenshots + 6-profile analysis + flow testing.
# The whole thing. No corners cut.
#
# Usage:
#   ./qa/run_e2e.sh              # Full pipeline: build → capture → analyze → grade
#   ./qa/run_e2e.sh --baseline   # Save current as baseline for regression
#   ./qa/run_e2e.sh --watch      # Re-run on source changes
#   ./qa/run_e2e.sh --diff       # Diff against baseline
#   ./qa/run_e2e.sh --grade      # Re-run analyzer only (no rebuild)
#   ./qa/run_e2e.sh --history    # Show past run scores
#   ./qa/run_e2e.sh --quick      # Standard QA (fewer angles, faster)
# ────────────────────────────────────────────────────────────────────

set -euo pipefail
cd "$(dirname "$0")/.."

SCREENSHOTS="qa/screenshots"
BASELINE="qa/baseline"
HISTORY="qa/history"
REPORT="qa/e2e_report.json"
COMPAT_REPORT="qa/report.json"
GRADES="qa/e2e_grades.json"
COMPAT_GRADES="qa/grades.json"
ANALYZER="qa/analyze_e2e.py"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
CYAN='\033[0;36m'
MAGENTA='\033[0;35m'
DIM='\033[0;90m'
BOLD='\033[1m'
RESET='\033[0m'

# ── Functions ─────────────────────────────────────────────────────

build_e2e() {
    local mode="${1:-e2e}"
    printf "${BOLD}${CYAN}── Building EV QA ($mode mode) ──${RESET}\n"

    local start_time=$(date +%s)

    if [ "$mode" = "e2e" ]; then
        make -j4 -s qa-e2e 2>&1 | _filter_output
    else
        make -j4 -s qa 2>&1 | _filter_output
    fi
    local exit_code=${PIPESTATUS[0]}

    local end_time=$(date +%s)
    local elapsed=$(( end_time - start_time ))
    printf "${DIM}Build + capture: ${elapsed}s${RESET}\n"

    return $exit_code
}

_filter_output() {
    while IFS= read -r line; do
        if [[ "$line" == "[QA]"* ]]; then
            if [[ "$line" == *"FAIL"* ]] && [[ "$line" != *"FAIL "*"walls"* ]]; then
                printf "${RED}%s${RESET}\n" "$line"
            elif [[ "$line" == *"COMPOSITION"* ]] || [[ "$line" == *"LIGHTING"* ]] || \
                 [[ "$line" == *"MATERIAL"* ]] || [[ "$line" == *"COLOR"* ]] || \
                 [[ "$line" == *"ANTI-PATTERN"* ]] || [[ "$line" == *"INTERACTION"* ]] || \
                 [[ "$line" == *"BUDGET"* ]] || [[ "$line" == *"GEOMETRY"* ]] || \
                 [[ "$line" == *"COLLISION"* ]] || [[ "$line" == *"ANOMALY"* ]] || \
                 [[ "$line" == *"REGRESSION"* ]] || [[ "$line" == *"CONSISTENCY"* ]]; then
                printf "${YELLOW}    %s${RESET}\n" "$line"
            elif [[ "$line" == *"PASS"* ]]; then
                printf "${GREEN}%s${RESET}\n" "$line"
            elif [[ "$line" == *"==="* ]] || [[ "$line" == *"──"* ]]; then
                printf "${CYAN}%s${RESET}\n" "$line"
            elif [[ "$line" == *"OK"* ]]; then
                printf "${GREEN}%s${RESET}\n" "$line"
            else
                printf "%s\n" "$line"
            fi
        elif [[ "$line" == *"WARNING"* ]] || [[ "$line" == *"Error"* ]]; then
            printf "${DIM}%s${RESET}\n" "$line"
        fi
    done
}

run_analyzer() {
    local report="${1:-$REPORT}"
    if [ ! -f "$report" ]; then
        # Fall back to standard report
        report="$COMPAT_REPORT"
    fi
    if [ -f "$report" ]; then
        printf "\n${BOLD}${MAGENTA}── MULTI-PROFILE ANALYSIS ──${RESET}\n"
        python3 "$ANALYZER" "$report"
        local exit_code=$?

        # Print grade summary
        local grades_file="$GRADES"
        [ -f "$grades_file" ] || grades_file="$COMPAT_GRADES"
        if [ -f "$grades_file" ]; then
            local grade=$(python3 -c "import json; print(json.load(open('$grades_file'))['overall']['grade'])" 2>/dev/null || echo "?")
            local pct=$(python3 -c "import json; print(json.load(open('$grades_file'))['overall']['pct'])" 2>/dev/null || echo "?")
            if [[ "$grade" == "A" ]]; then
                printf "\n${GREEN}${BOLD}  FINAL GRADE: $grade ($pct%%)${RESET}\n"
            elif [[ "$grade" == "B" ]]; then
                printf "\n${GREEN}  FINAL GRADE: $grade ($pct%%)${RESET}\n"
            elif [[ "$grade" == "C" ]]; then
                printf "\n${YELLOW}  FINAL GRADE: $grade ($pct%%)${RESET}\n"
            else
                printf "\n${RED}  FINAL GRADE: $grade ($pct%%)${RESET}\n"
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
        cp "$REPORT" "$BASELINE/e2e_report.json" 2>/dev/null || true
        cp "$COMPAT_REPORT" "$BASELINE/report.json" 2>/dev/null || true
        cp "$GRADES" "$BASELINE/e2e_grades.json" 2>/dev/null || true
        cp "$COMPAT_GRADES" "$BASELINE/grades.json" 2>/dev/null || true
        local count=$(ls "$BASELINE"/*.png 2>/dev/null | wc -l | tr -d ' ')
        printf "${GREEN}Baseline saved: $count screenshots${RESET}\n"
    else
        printf "${RED}No screenshots to baseline. Run QA first.${RESET}\n"
        exit 1
    fi
}

diff_screenshots() {
    if [ ! -d "$BASELINE" ]; then
        printf "${YELLOW}No baseline to diff against. Run with --baseline first.${RESET}\n"
        return
    fi

    printf "\n${CYAN}── Diff vs Baseline ──${RESET}\n"
    local changes=0 new=0 missing=0

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
        printf "${GREEN}  No visual changes.${RESET}\n"
    else
        printf "  %d changed, %d new, %d missing\n" "$changes" "$new" "$missing"
    fi

    # Grade comparison
    local old_grades="$BASELINE/e2e_grades.json"
    [ -f "$old_grades" ] || old_grades="$BASELINE/grades.json"
    local new_grades="$GRADES"
    [ -f "$new_grades" ] || new_grades="$COMPAT_GRADES"

    if [ -f "$old_grades" ] && [ -f "$new_grades" ]; then
        printf "\n${CYAN}── Grade Changes ──${RESET}\n"
        python3 -c "
import json
try:
    with open('$old_grades') as f: old = json.load(f)
    with open('$new_grades') as f: new = json.load(f)
    og = old['overall']
    ng = new['overall']
    if og.get('grade') != ng.get('grade') or og.get('pct') != ng.get('pct'):
        arrow = '+' if ng.get('pct',0) > og.get('pct',0) else '-'
        print(f\"  Overall: {og.get('grade','?')} ({og.get('pct','?')}%) -> {ng.get('grade','?')} ({ng.get('pct','?')}%) {arrow}\")
    # Profile changes
    for p in new.get('profiles', {}):
        oa = old.get('profiles',{}).get(p,{}).get('avg',0)
        na = new.get('profiles',{}).get(p,{}).get('avg',0)
        name = new['profiles'][p].get('name', p)
        if abs(oa - na) >= 0.3:
            arrow = '+' if na > oa else '-'
            print(f'  {name:16s}  {oa:.1f} -> {na:.1f} {arrow}')
    # Scene changes
    for name in sorted(set(list(old.get('scenes',{}).keys()) + list(new.get('scenes',{}).keys()))):
        os = old.get('scenes',{}).get(name,{})
        ns = new.get('scenes',{}).get(name,{})
        oc = os.get('composite', os.get('total',0))
        nc = ns.get('composite', ns.get('total',0))
        if isinstance(oc, (int,float)) and isinstance(nc, (int,float)) and abs(oc - nc) >= 0.3:
            arrow = '+' if nc > oc else '-'
            print(f'  {name:18s}  {os.get(\"grade\",\"?\")} -> {ns.get(\"grade\",\"?\")} {arrow}')
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
    cp "$REPORT" "$dir/e2e_report.json" 2>/dev/null || true
    cp "$COMPAT_REPORT" "$dir/report.json" 2>/dev/null || true
    cp "$GRADES" "$dir/e2e_grades.json" 2>/dev/null || true
    cp "$COMPAT_GRADES" "$dir/grades.json" 2>/dev/null || true
    printf "${DIM}Archived: %s${RESET}\n" "$dir"

    # Keep last 20
    local count=$(ls -d "$HISTORY"/20* 2>/dev/null | wc -l | tr -d ' ')
    if [ "$count" -gt 20 ]; then
        ls -d "$HISTORY"/20* | head -n $(( count - 20 )) | xargs rm -rf
    fi
}

show_history() {
    if [ -d "$HISTORY" ]; then
        printf "${CYAN}QA History (last 10 runs):${RESET}\n"
        for dir in $(ls -d "$HISTORY"/20* 2>/dev/null | tail -10); do
            ts=$(basename "$dir")
            # Try e2e grades first, fall back to standard
            local gfile="$dir/e2e_grades.json"
            [ -f "$gfile" ] || gfile="$dir/grades.json"
            if [ -f "$gfile" ]; then
                info=$(python3 -c "
import json
g = json.load(open('$gfile'))
grade = g['overall']['grade']
pct = g['overall']['pct']
profiles = g.get('profiles', {})
profile_str = ' '.join(f\"{p.get('name','')[:3]}:{p.get('grade','?')}\" for p in profiles.values()) if profiles else ''
print(f'{grade} ({pct}%)  {profile_str}')
" 2>/dev/null || echo "?")
            else
                info="?"
            fi
            printf "  %s  %s\n" "$ts" "$info"
        done
    else
        printf "${YELLOW}No history yet.${RESET}\n"
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
            build_e2e "e2e" || true
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
        build_e2e "e2e" || true
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
        show_history
        ;;
    --quick|-q)
        # Standard QA (fewer angles, faster)
        build_e2e "standard" || true
        run_analyzer "$COMPAT_REPORT" || true
        diff_screenshots
        save_history
        ;;
    run|"")
        build_e2e "e2e" || true
        run_analyzer || true
        diff_screenshots
        save_history
        ;;
    *)
        printf "Usage: ./qa/run_e2e.sh [--baseline|--watch|--diff|--grade|--history|--quick]\n"
        exit 1
        ;;
esac
