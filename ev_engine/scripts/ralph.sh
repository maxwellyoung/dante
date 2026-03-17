#!/bin/bash
# ── RALPH — Render → Assess → Loop → Patch → Headless QA ─────────
# Orchestrator for the EV engine QA loop.
#
# Usage:
#   ralph.sh --render          # Screenshot all scenes
#   ralph.sh --assess          # Run regression + health check, print summary
#   ralph.sh --loop [N]        # Render → assess → report, repeat N times (0 = continuous)
#   ralph.sh --patch-prompt    # Output structured failure prompt for coding agents
#   ralph.sh --watch           # Watch src/ for changes, auto-rebuild + assess
#   ralph.sh --help            # Show this help
set -euo pipefail
cd "$(dirname "$0")/.."

SCRIPTS="scripts"
CYAN='\033[0;36m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
RED='\033[0;31m'
BOLD='\033[1m'
DIM='\033[0;90m'
RESET='\033[0m'

# ── Functions ─────────────────────────────────────────────────────

do_render() {
    printf "\n${BOLD}${CYAN}══════════════════════════════════════════════════${RESET}\n"
    printf "${BOLD}${CYAN}  RALPH — RENDER${RESET}\n"
    printf "${BOLD}${CYAN}══════════════════════════════════════════════════${RESET}\n\n"
    bash "$SCRIPTS/screenshot-scenes.sh"
}

do_assess() {
    printf "\n${BOLD}${CYAN}══════════════════════════════════════════════════${RESET}\n"
    printf "${BOLD}${CYAN}  RALPH — ASSESS${RESET}\n"
    printf "${BOLD}${CYAN}══════════════════════════════════════════════════${RESET}\n\n"

    local health_ok=true
    local regression_ok=true

    # Health check (compile + crash)
    printf "${CYAN}── Health Check ──${RESET}\n"
    if bash "$SCRIPTS/health-check.sh"; then
        printf "${GREEN}Health: PASS${RESET}\n"
    else
        printf "${RED}Health: FAIL${RESET}\n"
        health_ok=false
    fi

    # Visual regression
    printf "\n${CYAN}── Visual Regression ──${RESET}\n"
    if bash "$SCRIPTS/check-regression.sh"; then
        printf "${GREEN}Regression: PASS${RESET}\n"
    else
        printf "${YELLOW}Regression: CHANGES DETECTED${RESET}\n"
        regression_ok=false
    fi

    # Analyzer (if report exists)
    if [ -f "qa/report.json" ]; then
        printf "\n${CYAN}── Analysis ──${RESET}\n"
        python3 qa/analyze.py qa/report.json || true
    fi

    # Summary
    printf "\n${BOLD}${CYAN}══════════════════════════════════════════════════${RESET}\n"
    if $health_ok && $regression_ok; then
        printf "${GREEN}  RALPH ASSESS: ALL CLEAR${RESET}\n"
    elif $health_ok; then
        printf "${YELLOW}  RALPH ASSESS: VISUAL CHANGES (compile OK)${RESET}\n"
    else
        printf "${RED}  RALPH ASSESS: ISSUES FOUND${RESET}\n"
    fi
    printf "${BOLD}${CYAN}══════════════════════════════════════════════════${RESET}\n"
}

do_patch_prompt() {
    printf "\n${BOLD}${CYAN}══════════════════════════════════════════════════${RESET}\n"
    printf "${BOLD}${CYAN}  RALPH — PATCH PROMPT${RESET}\n"
    printf "${BOLD}${CYAN}══════════════════════════════════════════════════${RESET}\n\n"

    # Run assess first to get fresh data
    bash "$SCRIPTS/health-check.sh" >/dev/null 2>&1 || true

    echo "---BEGIN PATCH PROMPT---"
    echo ""
    echo "# EV Engine — Automated QA Failure Report"
    echo ""
    echo "The following issues were detected by RALPH (automated QA)."
    echo "Fix these issues in the EV engine codebase at \`ev_engine/src/\`."
    echo ""

    # Compile errors
    if [ -f "qa/health-latest.json" ]; then
        compile_ok=$(python3 -c "import json; print(json.load(open('qa/health-latest.json'))['compile']['ok'])" 2>/dev/null || echo "true")
        if [ "$compile_ok" = "False" ] || [ "$compile_ok" = "false" ]; then
            echo "## Compile Errors"
            echo ""
            echo '```'
            python3 -c "import json; print(json.load(open('qa/health-latest.json'))['compile']['errors'])" 2>/dev/null || echo "(could not read errors)"
            echo '```'
            echo ""
        fi

        # Per-scene failures
        echo "## Scene Status"
        echo ""
        python3 -c "
import json
with open('qa/health-latest.json') as f:
    h = json.load(f)
for s in h.get('scenes', []):
    status = '✅' if s['status'] == 'PASS' else '❌'
    print(f\"- {status} **{s['name']}**: {s['status']}\")
" 2>/dev/null || echo "(could not parse health JSON)"
        echo ""
    fi

    # QA analysis issues
    if [ -f "qa/report.json" ]; then
        echo "## QA Analysis Issues"
        echo ""
        python3 -c "
import json
with open('qa/report.json') as f:
    r = json.load(f)
for s in r['scenes']:
    if s['issues'] > 0:
        print(f\"### {s['name'].upper()} ({s['issues']} issues)\")
        # Reconstruct issues from metrics
        if s.get('edge_density', 99) < 5 and not s.get('dark_by_design'):
            print(f\"- COMPOSITION: edge density {s['edge_density']:.1f}% (too low)\")
        if s.get('luma', 99) < 10 and not s.get('dark_by_design'):
            print(f\"- LIGHTING: luma {s['luma']:.1f} (nearly invisible)\")
        if s.get('black_pct', 0) > 60 and not s.get('dark_by_design'):
            print(f\"- LIGHTING: {s['black_pct']:.0f}% black pixels\")
        if s.get('hue_buckets', 99) < 3 and not s.get('dark_by_design'):
            print(f\"- COLOR: only {s['hue_buckets']} hue buckets (monotone)\")
        if s.get('material_coverage', 100) < 25 and s.get('walls', 0) > 20:
            print(f\"- MATERIALS: {s['material_coverage']:.0f}% non-concrete\")
        if s.get('interact_count', 0) == 0 and s['name'] not in ('hallway', 'hotel_ext', 'elevator', 'hyperspace', 'taxi', 'space_corridor'):
            print(f\"- INTERACTION: no interactive objects\")
        print()
" 2>/dev/null || echo "(could not parse report)"
    fi

    # Visual regression
    if [ -d "qa/baseline" ] && [ -n "$(ls -A qa/baseline 2>/dev/null)" ]; then
        echo "## Visual Regression"
        echo ""
        bash "$SCRIPTS/check-regression.sh" 2>/dev/null | sed 's/\x1b\[[0-9;]*m//g' || echo "(no regression data)"
        echo ""
    fi

    # Grades summary
    if [ -f "qa/grades.json" ]; then
        echo "## Current Grades"
        echo ""
        python3 -c "
import json
with open('qa/grades.json') as f:
    g = json.load(f)
print(f\"Overall: {g['overall']['grade']} ({g['overall']['pct']}%)\")
print()
for p, v in g.get('pillars', {}).items():
    print(f\"- {p}: {v['grade']} ({v['avg']}/5)\")
print()
print('Per-scene:')
for name, v in sorted(g.get('scenes', {}).items()):
    print(f\"- {name}: {v['grade']} ({v['total']}/30)\")
" 2>/dev/null || echo "(could not parse grades)"
        echo ""
    fi

    echo "## Context"
    echo ""
    echo "- Engine: C99 + Raylib, custom renderer"
    echo "- Scene files: \`src/scene_*.c\` (build functions)"
    echo "- Lighting presets: \`src/lighting.c\` (LightingPreset_* functions)"
    echo "- Main game loop: \`src/main.c\`"
    echo "- Types/enums: \`src/ev_types.h\`"
    echo "- Build: \`make\` (normal) | \`make qa\` (QA screenshots)"
    echo ""
    echo "---END PATCH PROMPT---"
}

do_loop() {
    local max_loops=${1:-0}
    local loop=0

    while true; do
        loop=$((loop + 1))
        printf "\n${BOLD}${CYAN}══ RALPH LOOP %d ══${RESET}\n" "$loop"

        do_render
        do_assess

        if [ "$max_loops" -gt 0 ] && [ "$loop" -ge "$max_loops" ]; then
            printf "\n${CYAN}[RALPH] Completed %d loop(s).${RESET}\n" "$loop"
            break
        fi

        printf "\n${DIM}[RALPH] Loop %d complete. Waiting 10s before next iteration...${RESET}\n" "$loop"
        printf "${DIM}[RALPH] Press Ctrl+C to stop.${RESET}\n"
        sleep 10
    done
}

do_watch() {
    printf "${CYAN}[RALPH] ── Watch Mode ──${RESET}\n"
    printf "${DIM}[RALPH] Watching src/ for changes (polling every 3s)...${RESET}\n"
    printf "${DIM}[RALPH] Press Ctrl+C to stop.${RESET}\n\n"

    local last_hash=""
    # Initial hash
    last_hash=$(find src/ -name '*.c' -o -name '*.h' | sort | xargs cat 2>/dev/null | md5 2>/dev/null || echo "")

    while true; do
        sleep 3
        local current_hash
        current_hash=$(find src/ -name '*.c' -o -name '*.h' | sort | xargs cat 2>/dev/null | md5 2>/dev/null || echo "")

        if [ "$current_hash" != "$last_hash" ] && [ -n "$current_hash" ]; then
            last_hash="$current_hash"
            printf "\n${CYAN}[RALPH] ── Change detected: $(date +%H:%M:%S) ──${RESET}\n"
            do_assess || true
        fi
    done
}

show_help() {
    cat <<'EOF'
RALPH — Render → Assess → Loop → Patch → Headless QA

Usage:
  ralph.sh --render          Screenshot all scenes via QA mode
  ralph.sh --assess          Run health check + regression + analysis
  ralph.sh --loop [N]        Render → assess, repeat N times (0 = continuous)
  ralph.sh --patch-prompt    Output structured failure prompt for coding agents
  ralph.sh --watch           Watch src/ for changes, auto-assess on save
  ralph.sh --help            Show this help

The RALPH loop:
  1. RENDER:  Build with QA_MODE, capture screenshots of all 14 scenes
  2. ASSESS:  Compile check, crash detection, visual regression, QA analysis
  3. LOOP:    Repeat render→assess to track progress across iterations
  4. PATCH:   Generate a structured prompt describing all failures
  5. QA:      The headless analysis grades composition, lighting, materials,
              color, interaction, and anti-patterns on a 1-5 scale per scene

Files:
  qa/screenshots/     Current scene screenshots (hero + spawn)
  qa/baseline/        Baseline screenshots for regression comparison
  qa/diffs/           Visual diff images (when regression detected)
  qa/report.json      Raw QA metrics per scene
  qa/grades.json      Letter grades per scene and pillar
  qa/health-latest.json  Compile/crash status
EOF
}

# ── Main ──────────────────────────────────────────────────────────

case "${1:-}" in
    --render|-r)
        do_render
        ;;
    --assess|-a)
        do_assess
        ;;
    --loop|-l)
        do_loop "${2:-0}"
        ;;
    --patch-prompt|-p)
        do_patch_prompt
        ;;
    --watch|-w)
        do_watch
        ;;
    --help|-h|"")
        show_help
        ;;
    *)
        printf "${RED}Unknown option: %s${RESET}\n" "$1"
        show_help
        exit 1
        ;;
esac
