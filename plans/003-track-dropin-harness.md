# Plan 003: Make the bed-ritual track a drop-in WAV (with procedural fallback)

> **Executor instructions**: Follow step by step; verify each step. On any
> STOP condition, stop and report. Update `plans/README.md` when done.
>
> **Drift check (run first)**:
> `git diff --stat 04f970a..HEAD -- ev_engine/src/audio.c ev_engine/src/audio.h ev_engine/src/scene_endings.c`
> On mismatch with "Current state", STOP.

## Status

- **Priority**: P1
- **Effort**: S
- **Risk**: LOW
- **Depends on**: none
- **Category**: dx (unblocks the single remaining content gap)
- **Planned at**: commit `04f970a`, 2026-07-05

## Why this matters

The bed-ritual track is the project's only named content gap ("the emotional
center" — a composed piece the repo owner will write himself; see
`ROADMAP.md` Phase 1.1). Today the bed scene plays `snd_bed_ritual`, a
*procedurally synthesized* 32-second placeholder. This plan makes the real
track a **zero-code drop-in**: if `assets/audio/bed_ritual.wav` exists, it
plays; otherwise the procedural piece plays. The musician iterates by
replacing a file — no rebuild, no programmer.

## Current state

All paths relative to `/Users/maxwellyoung/Development/dante/ev_engine`.

- `src/audio.c`:
  - ~line 996: `static Sound gen_bed_ritual(void);` — the procedural piece.
  - ~line 1055: `audio->snd_bed_ritual = gen_bed_ritual();` in `InitEVAudio`.
  - ~line 1131: `SetSoundVolume(audio->snd_bed_ritual, 0.06f);` with comment
    "THE piece — warm, present, emotional center".
  - File-streamed music already exists and is the pattern to copy
    (~lines 2512–2515 in `LoadFileMusic`):
    `audio->music_suite = LoadMusicStream("assets/audio/lighthouse.wav");`
    plus `UpdateFileMusic(&g.audio)` pumped every frame from main.c.
  - `PlayBedRitual(EVAudio*)` declared in `src/audio.h` (~line 192, comment:
    "the one composed piece — plays once, never repeats"). Its
    implementation sets `bed_ritual_playing` / `bed_ritual_played_once`
    guards (fields ~lines 1085–1086 pattern).
- `src/scene_endings.c` — `bed_load()` calls `PlayBedRitual(&g.audio);`.
- Convention: audio is initialized in `InitEVAudio`; music streams are
  loaded in `LoadFileMusic` and updated via `UpdateFileMusic`. Respect
  `EV_MUTE` (already handled globally via `SetMasterVolume` in InitEVAudio).
- `assets/audio/` currently holds: `lighthouse.wav`, `ambient1_icloud.wav`,
  `ambient3.wav`, `ambient4.wav`. There is NO `bed_ritual.wav` yet — the
  fallback path is the one that must keep working.

## Commands you will need

| Purpose | Command | Expected |
|---|---|---|
| Build | `cd ev_engine && EV_MUTE=1 make -j8` | exit 0, no warnings |
| Tests | `make test` | 31 tests, 0 failed |
| Runtime check (fallback) | `rm -f assets/audio/bed_ritual.wav; EV_MUTE=1 make -j8 dev SCENE=STATE_BED` then Ctrl-C after ~10s (or `pkill -f endearing_void`) | log contains no ERROR; game runs |
| Runtime check (file path) | `cp assets/audio/ambient3.wav assets/audio/bed_ritual.wav` (stand-in), rerun dev SCENE=STATE_BED, then `rm assets/audio/bed_ritual.wav` | log line confirming file track loaded |

Note: `make dev SCENE=...` rebuilds with `-DDEV_START` and runs the game
windowed. Always prefix `EV_MUTE=1` (repo rule).

## Scope

**In scope**: `ev_engine/src/audio.c`, `ev_engine/src/audio.h`. Nothing else.
**Out of scope**: `scene_endings.c` (its `PlayBedRitual` call stays exactly
as-is — the switch happens inside audio.c), the Makefile, all other scenes'
music.

## Steps

### Step 1: Load the optional stream

In `LoadFileMusic` (src/audio.c ~2512), following the existing pattern, add:

```c
audio->bed_ritual_file_present = FileExists("assets/audio/bed_ritual.wav");
if (audio->bed_ritual_file_present) {
    audio->music_bed_ritual = LoadMusicStream("assets/audio/bed_ritual.wav");
    audio->music_bed_ritual.looping = false;   // plays once, never repeats
    TraceLog(LOG_INFO, "[EV] Bed ritual: file track loaded (bed_ritual.wav)");
} else {
    TraceLog(LOG_INFO, "[EV] Bed ritual: procedural fallback (drop assets/audio/bed_ritual.wav to replace)");
}
```

Add the two fields (`Music music_bed_ritual; bool bed_ritual_file_present;`)
to the `EVAudio` struct in `src/audio.h`, next to the existing `music_*`
fields. Mirror the unload pattern in `UnloadFileMusic`.

**Verify**: `EV_MUTE=1 make -j8` → exit 0, no warnings.

### Step 2: Route PlayBedRitual through the file when present

In the `PlayBedRitual` implementation (src/audio.c): if
`bed_ritual_file_present`, `PlayMusicStream(audio->music_bed_ritual)` and set
the same `bed_ritual_playing`/`bed_ritual_played_once` guards the procedural
path sets; else existing behavior. In `UpdateFileMusic`, add
`if (audio->bed_ritual_file_present && IsMusicStreamPlaying(audio->music_bed_ritual)) UpdateMusicStream(audio->music_bed_ritual);`
Ensure `StopAllAudio` also stops this stream (find it in audio.c; every
scene load calls it — the bed track must not leak into STARS unless the
design comment says otherwise; it must NOT: hard cuts kill audio, that is the
existing contract).

**Verify**: both runtime checks in the commands table behave as described
(fallback silent-clean; stand-in file logs "file track loaded").

### Step 3: Volume + docs

Match the file path's volume to the mix: `SetMusicVolume(music_bed_ritual, 0.5f)`
as a starting point (the procedural one runs at 0.06 SetSoundVolume — file
tracks are typically mastered hotter; 0.5 is the other streams' ballpark —
check `SetMusicVolume` calls near LoadFileMusic and match them). Document the
drop-in in `ev_engine/CLAUDE.md` Audio System section: one sentence — "Drop
`assets/audio/bed_ritual.wav` to replace the procedural bed piece; volume in
PlayBedRitual."

**Verify**: `make test` passes; `make check` → All files clean.

## Test plan

No headless unit tests cover audio (raylib-bound). The two runtime checks in
the commands table are the test. Confirm in logs: exactly one of the two
TraceLog lines appears per run depending on file presence.

## Done criteria

- [ ] Build + `make test` + `make check` all clean
- [ ] With no wav: procedural fallback logs and plays (game runs, no ERROR)
- [ ] With a wav present: file streams, plays once, stops on scene cut
- [ ] `scene_endings.c` untouched (`git diff --name-only` excludes it)
- [ ] `plans/README.md` row updated

## STOP conditions

- `PlayBedRitual` implementation does not exist or has no
  `bed_ritual_played_once` guard (drift — the once-only contract moved).
- `StopAllAudio` doesn't exist or scene loads no longer call it.
- Adding fields to `EVAudio` breaks `tests/test_main` struct checks — report
  the size assertion rather than editing the test.

## Maintenance notes

- When the real track lands, the volume constant is the only knob expected
  to need touching.
- VO later (ROADMAP Phase 4) will reuse exactly this drop-in pattern per
  vox ID; keep this implementation clean as the exemplar.
