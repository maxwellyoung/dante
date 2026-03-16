#!/bin/bash
# qa-check.sh — Automated QA for the EV engine
set -uo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

pass=0
fail=0
total_lines=""

header() { echo ""; echo "══════════════════════════════════════════"; echo "  $1"; echo "══════════════════════════════════════════"; }
ok()     { echo "  ✓ $1"; pass=$((pass + 1)); }
err()    { echo "  ✗ $1"; fail=$((fail + 1)); }

echo ""
echo "  ENDEARING VOID — QA Check"
echo "  $(date '+%Y-%m-%d %H:%M')"

# 1. Clean build
header "Step 1: Clean Build"
if make clean && make 2>&1; then
    ok "Build succeeded"
else
    err "Build failed"
fi

# 2. Static analysis
header "Step 2: Static Analysis (make check)"
if make check 2>&1; then
    ok "Static analysis passed"
else
    err "Static analysis had warnings"
fi

# 3. Headless tests
header "Step 3: Headless Tests (make test)"
if make test 2>&1; then
    ok "All tests passed"
else
    err "Tests failed"
fi

# 4. Line count
header "Step 4: Line Count"
count_output=$(make count 2>&1)
echo "$count_output"
total_lines=$(echo "$count_output" | grep 'total' | awk '{print $1}')
ok "Line count captured"

# 5. TODOs
header "Step 5: TODOs & FIXMEs"
todo_output=$(make todo 2>&1)
echo "$todo_output"
ok "TODO scan complete"

# Summary
header "SUMMARY"
echo ""
echo "  Passed: $pass"
echo "  Failed: $fail"
[ -n "$total_lines" ] && echo "  Total lines: $total_lines"
echo ""

if [ "$fail" -eq 0 ]; then
    echo "  ALL CLEAR."
else
    echo "  $fail step(s) had issues."
fi
echo ""

exit "$fail"
