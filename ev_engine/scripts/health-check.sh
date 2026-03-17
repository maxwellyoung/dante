#!/bin/bash
# ── RALPH: Compile + crash health check ──────────────────────────
# Checks compilation and runs QA mode to verify all scenes load cleanly.
#
# Usage:
#   ./scripts/health-check.sh
set -euo pipefail
cd "$(dirname "$0")/.."

HEALTH_JSON="qa/health-latest.json"
CYAN='\033[0;36m'
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[0;33m'
DIM='\033[0;90m'
RESET='\033[0m'

printf "${CYAN}[RALPH] ── Health Check ──${RESET}\n"

compile_ok="true"
compile_errors=""
qa_exit=0
scene_results="[]"
timestamp=$(date -u +%Y-%m-%dT%H:%M:%SZ)

# ── Step 1: Compile check ──
printf "${DIM}[RALPH] Compiling...${RESET}\n"
rm -f src/*.qa.o
if compile_output=$(make -j4 qa 2>&1); then
    compile_ok="true"
    printf "  ${GREEN}Compile: OK${RESET}\n"
    
    # Parse QA output for per-scene results
    # QA_MODE prints: [QA] scene_name PASS/FAIL  walls:N  ...
    scene_json_entries=""
    while IFS= read -r line; do
        if [[ "$line" =~ ^\[QA\][[:space:]]+([a-z_]+)[[:space:]]+(PASS|FAIL) ]]; then
            sname="${BASH_REMATCH[1]}"
            sstatus="${BASH_REMATCH[2]}"
            if [ -n "$scene_json_entries" ]; then
                scene_json_entries="${scene_json_entries},"
            fi
            scene_json_entries="${scene_json_entries}{\"name\":\"${sname}\",\"status\":\"${sstatus}\"}"
            if [ "$sstatus" = "PASS" ]; then
                printf "  ${GREEN}  %-20s OK${RESET}\n" "$sname"
            else
                printf "  ${YELLOW}  %-20s ISSUES${RESET}\n" "$sname"
            fi
        fi
    done <<< "$compile_output"
    
    # Check if QA exited with issues
    total_issues=$(echo "$compile_output" | grep -oP '\d+(?= issue)' | head -1 || echo "0")
    if [ -z "$total_issues" ]; then total_issues=0; fi
    
    scene_results="[${scene_json_entries}]"
    
    # Check for stderr/crash indicators
    if echo "$compile_output" | grep -qi "segfault\|abort\|signal\|crash\|SIGSEGV\|SIGABRT"; then
        printf "  ${RED}CRASH detected in QA run!${RESET}\n"
        qa_exit=1
    fi
else
    compile_ok="false"
    compile_errors=$(echo "$compile_output" | grep -E "error:|undefined" | head -20)
    printf "  ${RED}Compile: FAILED${RESET}\n"
    printf "${DIM}%s${RESET}\n" "$compile_errors"
    qa_exit=1
fi

# ── Step 2: Check screenshots exist ──
screenshot_count=0
if [ -d "qa/screenshots" ]; then
    screenshot_count=$(ls -1 qa/screenshots/*.png 2>/dev/null | wc -l | tr -d ' ')
fi
printf "  Screenshots: %s\n" "$screenshot_count"

# ── Step 3: Check report exists ──
report_ok="false"
if [ -f "qa/report.json" ]; then
    report_ok="true"
    report_scenes=$(python3 -c "import json; print(json.load(open('qa/report.json'))['scene_count'])" 2>/dev/null || echo "0")
    printf "  Report: %s scenes\n" "$report_scenes"
else
    printf "  ${YELLOW}Report: not generated${RESET}\n"
fi

# ── Write health JSON ──
# Escape compile errors for JSON
escaped_errors=$(echo "$compile_errors" | python3 -c "import sys,json; print(json.dumps(sys.stdin.read()))" 2>/dev/null || echo '""')

cat > "$HEALTH_JSON" <<EOF
{
  "timestamp": "$timestamp",
  "compile": {
    "ok": $compile_ok,
    "errors": $escaped_errors
  },
  "qa_exit_code": $qa_exit,
  "screenshots": $screenshot_count,
  "report_generated": $report_ok,
  "scenes": $scene_results,
  "total_issues": ${total_issues:-0}
}
EOF

printf "\n  ${DIM}Written: %s${RESET}\n" "$HEALTH_JSON"

if [ "$compile_ok" = "true" ] && [ "$qa_exit" -eq 0 ]; then
    printf "\n${GREEN}[RALPH] Health: ALL CLEAR${RESET}\n"
else
    printf "\n${RED}[RALPH] Health: ISSUES FOUND${RESET}\n"
fi

exit $qa_exit
