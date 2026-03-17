# RALPH — Render → Assess → Loop → Patch → Headless QA

Automated quality assurance loop for the Endearing Void engine.

## Quick Start

```bash
# First time: capture baseline screenshots
./scripts/ralph.sh --render
./scripts/check-regression.sh --update-baseline

# After making changes: full assessment
./scripts/ralph.sh --assess

# Watch for file changes and auto-assess
./scripts/ralph.sh --watch

# Generate a prompt for coding agents to fix issues
./scripts/ralph.sh --patch-prompt
```

## Commands

| Command | What it does |
|---------|-------------|
| `ralph.sh --render` | Build with QA_MODE, screenshot all 14 scenes |
| `ralph.sh --assess` | Health check + visual regression + QA analysis |
| `ralph.sh --loop N` | Render → assess, repeat N times (0 = continuous) |
| `ralph.sh --patch-prompt` | Structured failure report for coding agents |
| `ralph.sh --watch` | Poll `src/` every 3s, auto-assess on change |

## Individual Scripts

| Script | Purpose |
|--------|---------|
| `scripts/screenshot-scenes.sh` | Wraps `make qa` to capture hero+spawn PNGs |
| `scripts/check-regression.sh` | ImageMagick pixel diff against baseline |
| `scripts/health-check.sh` | Compile check + crash detection + JSON report |

## How It Works

### Render
Uses the existing `QA_MODE` compile flag (`make qa`). The QA block in `main.c`:
- Iterates all 14 scene states
- Sets custom camera angles (hero + spawn shots)
- Renders 8 frames per angle through the full PostFX pipeline
- Captures PNGs to `qa/screenshots/`
- Writes pixel analysis to `qa/report.json`

No separate headless binary needed. `QA_MODE` uses `FLAG_WINDOW_ALWAYS_RUN` so the GPU renders even without focus.

### Assess
Three checks run in sequence:

1. **Health Check** (`health-check.sh`): Compiles with `make qa`, captures any build errors, checks for crash indicators in output. Writes `qa/health-latest.json`.

2. **Visual Regression** (`check-regression.sh`): Compares `qa/screenshots/*_hero.png` against `qa/baseline/` using ImageMagick `compare`. Threshold: <1% pixel diff = pass. Generates diff images in `qa/diffs/`.

3. **QA Analysis** (`qa/analyze.py`): Scores each scene across 6 pillars (composition, lighting, materials, color, interaction, anti-patterns) on a 1-5 scale. Produces letter grades.

### Patch Prompt
Generates a markdown document describing all failures, ready to paste into a coding agent. Includes compile errors, per-scene status, QA issues with specific metrics, regression results, and current grades.

## Files

```
qa/
  screenshots/          Current scene screenshots (hero + spawn per scene)
  baseline/             Baseline for regression comparison
  diffs/                Visual diff PNGs (only when regressions detected)
  report.json           Raw QA metrics per scene
  grades.json           Letter grades per scene and pillar
  health-latest.json    Compile/crash status
  history/              Timestamped archives of past runs
  analyze.py            Scoring engine (the harsh critic)
  run.sh                Original QA runner (still works independently)
scripts/
  ralph.sh              RALPH orchestrator
  screenshot-scenes.sh  Scene screenshotter
  check-regression.sh   Visual regression checker
  health-check.sh       Compile/crash checker
```

## Workflow

### Development Loop
```bash
# Terminal 1: watch mode
./scripts/ralph.sh --watch

# Terminal 2: edit source
vim src/scene_room.c
# Save → RALPH auto-assesses within 3s
```

### CI / Batch Mode
```bash
# Run 3 iterations of render→assess
./scripts/ralph.sh --loop 3

# Generate patch prompt and pipe to agent
./scripts/ralph.sh --patch-prompt > /tmp/patch.md
```

### Baseline Management
```bash
# Set current screenshots as the new baseline
./scripts/check-regression.sh --update-baseline

# Or via the original runner
./qa/run.sh --baseline
```

## Prerequisites

- **Raylib** + **clang** (already configured in Makefile)
- **ImageMagick** for visual regression (`brew install imagemagick`)
- **Python 3** for analysis (`analyze.py`)
- A display server (macOS desktop) — QA_MODE needs a GPU context

## Integration with Existing QA

RALPH builds on top of the existing `make qa` / `qa/run.sh` pipeline. The original commands still work:

```bash
make qa          # Build + run QA (same as ralph --render)
make qa-full     # Full pipeline via run.sh
make qa-baseline # Save baseline via run.sh
make qa-watch    # Watch via run.sh
```

RALPH adds: structured health checks, ImageMagick pixel regression, JSON health reports, patch prompt generation, and the orchestrator loop.
