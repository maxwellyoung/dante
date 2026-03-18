Run the full QA pipeline for the EV engine. This does:

1. `make clean && make` — verify 0 errors, 0 warnings
2. `make check` — pedantic static analysis, all files must say OK
3. `make test` — headless unit tests must pass
4. `make qa` — automated screenshot + pixel analysis of all 14 scenes

After QA runs, read ALL hero screenshots and report:
- Any scenes that FAIL
- Visual anomalies (rogue geometry, z-fighting, black screens, wrong camera angles)
- Luma/contrast/material coverage concerns
- Wall counts (rebuilt scenes should be 150+ walls, critical scenes 300+)

If any step fails, stop and fix the issue before continuing.

The QA pipeline screenshots go to `qa/screenshots/` and the report to `qa/report.json`.
