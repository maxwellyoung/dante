# Plan 007: Ship kit — reproducible release build + itch.io page checklist

> **Executor instructions**: Follow step by step; verify each step. On any
> STOP condition, stop and report. Update `plans/README.md` when done.
> NOTE: this plan PREPARES the release; it does not publish. Publishing is a
> human action (account, page, upload button).
>
> **Drift check (run first)**:
> `git diff --stat 04f970a..HEAD -- ev_engine/Makefile`
> On mismatch with "Current state", STOP.

## Status

- **Priority**: P2
- **Effort**: S
- **Risk**: LOW
- **Depends on**: 001, 002, 003, 005, 006 (ship gate = their done criteria)
- **Category**: dx / direction (Phase 1.8: ship it)
- **Planned at**: commit `04f970a`, 2026-07-05

## Why this matters

ROADMAP Phase 1 ends "itch.io, free, solo credit. Credits roll = Phase 1
done." Everything about the release should be one command + one upload, so
shipping is an afternoon, not a project. This also forces the last technical
questions (app naming, macOS Gatekeeper reality) to be answered once.

## Current state

- `ev_engine/Makefile` — `release:` target exists (search it): clean
  `-O3 -DNDEBUG` build, strips binary → `endearing_void_release`. No
  packaging, no assets bundling. `playtest:` target shows the
  bundle-assets pattern to copy.
- The game is a plain macOS arm64 binary + `assets/` folder (no .app bundle,
  no code signing). Reality for itch on macOS: unsigned binaries trigger
  Gatekeeper; the standard indie mitigation is documenting
  right-click-open, and/or shipping via the itch.io app (which quarantine-
  strips). Signing/notarization is OUT of scope (needs the owner's Apple
  Developer identity).
- Screenshots for the page already exist: `qa/screenshots/*_hero.png`
  (1920×1200) and `qa/lookdev/` comparisons; demo stills in `qa/demo/` if
  plan 004 has run.
- Credits: solo — Maxwell Young (repo rule: Adam Van der Voorn is no longer
  on the project; do not credit anyone else).

## Commands you will need

| Purpose | Command | Expected |
|---|---|---|
| Build ship bundle | `cd ev_engine && make ship` (created in Step 1) | `dist/endearing-void-macos.zip` |
| Smoke the bundle | `cd dist/endearing_void && ./endearing_void` (manual, ~1 min) | boots to TITLE |
| Tests first | `make test && EV_MUTE=1 make -j8 qa` | pass / table clean |

## Scope

**In scope**: `ev_engine/Makefile` (new `ship:` target), `ev_engine/SHIP.md`
(create: the checklist), `ev_engine/dist/` (generated, add to .gitignore if
one exists — check `git check-ignore dist` first).
**Out of scope**: code signing, notarization, Windows/Linux builds (defer;
raylib makes them feasible later), price (free — decided), any gameplay file.

## Steps

### Step 1: `make ship`

New Makefile target modeled on `playtest:`: clean release build (reuse the
`release:` recipe flags), then assemble
`dist/endearing_void/` = binary (named `endearing_void`) + `assets/` +
`README.txt` (controls, "unsigned build: right-click → Open the first
time", one-line premise, credit line "a game by Maxwell Young") — then
`cd dist && zip -r endearing-void-macos.zip endearing_void`.

**Verify**: target runs from clean checkout state; zip exists; unzipping
elsewhere and launching boots to TITLE (manual).

### Step 2: SHIP.md checklist

Write `ev_engine/SHIP.md`:

- Pre-flight gates (all must be green): `make test`; `make qa` spine PASS +
  zfight:0; `EV_QA_RUN=rush` + `linger` COMPLETE (plan 005); playtest ×3
  done (plan 006); the track present (plan 003 drop-in, file exists);
  look locked (plan 002).
- Page assets: pick 5 hero PNGs (list the exact current best: suite, lobby
  Grickle, corridor, taxi, balcony) + cover image spec (630×500 itch
  requirement — crop from a hero shot).
- Page copy skeleton: one-line hook ("Three hours to kill in a hotel that
  has no business being in space."), 3-sentence description, controls,
  15-minute duration (use plan 005's measured number), content notes,
  macOS-unsigned note.
- Upload steps: itch dashboard → new project → upload zip → macOS checkbox
  → free / no minimum → publish. (Human performs these.)
- Post-ship: tag `git tag ev-1.0 && git push --tags` (human confirms).

**Verify**: file exists; every referenced artifact path in it is real
(`ls` each).

## Test plan

The Step 1 manual smoke + pre-flight gate list IS the test plan. Nothing
automated beyond existing suites.

## Done criteria

- [ ] `make ship` produces a zip whose unzipped contents boot on this Mac
- [ ] `SHIP.md` exists; all referenced paths verified
- [ ] `dist/` ignored by git (or explicitly committed-empty decision noted)
- [ ] `plans/README.md` row updated

## STOP conditions

- The release build emits warnings or the stripped binary fails to boot.
- Anything suggests the zip needs signing to run locally at all (would
  indicate macOS policy changed) — report, don't attempt signing.

## Maintenance notes

- When AFTERSHOW ships later, clone this target; keep EV's zip name stable
  for devlog links.
- Windows/Linux: raylib cross-builds are the known path; deferred until
  anyone asks.
