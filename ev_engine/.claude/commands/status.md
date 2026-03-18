Project status dashboard. Quick overview of everything.

1. Line counts: `wc -l src/*.c src/*.h | sort -n`
2. Git status: `git status --short`
3. Recent commits: `git log --oneline -10`
4. Build check: `make 2>&1 | tail -1`
5. Last QA results: `cat qa/report.json | jq '.scenes[] | {name, status, walls, material_types, luma, issues}'`
6. TODO/FIXME count: `grep -rn 'TODO\|FIXME' src/`

Format as a clean dashboard.
