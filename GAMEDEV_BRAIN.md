# Endearing Void / Dante — Game Dev Brain

Last updated: 2026-03-31

This file is the current single-document brain for the project: what the game is, why it exists, what has been built, what has changed, what still matters, and what tradeoffs are currently shaping development.

## 1. Project Identity

Working project names in this repo:

- `Endearing Void` is the narrative first-person exploration game.
- `Dante` is the repo/workspace name.
- `Infernal Ascent` is the current LÖVE/Lua movement/combat proving-ground lineage still present in the repo.

This repo currently contains multiple overlapping tracks:

- An older `LÖVE` / `Lua` game track at the repo root.
- A newer `Raylib` / `C99` 3D track in `ev_engine/` that is the main Endearing Void implementation direction so far.
- A large research/reference corpus in `references/`.
- Blender and asset-production plans for authored props and shells.

This is not a clean greenfield anymore. It is a live design brain plus multiple implementation attempts converging on the same game.

## 2. What Endearing Void Is

Endearing Void is a short first-person narrative game about arriving at an impossible hotel with three hours to kill before the rest of your life starts.

Core definition, consolidated from the current plans:

- Length target: `15-30 minutes`
- Form: `first-person narrative exploration`
- Primary storytelling medium: `architecture, objects, light, pacing, omission`
- Emotional register: `wonder, melancholy, tenderness, absence`
- Tone constraint: `wonder, not horror`
- Scope philosophy: `brief, dense, replayable, emotionally legible, not over-explained`

The emotional core:

- The suite was booked for two.
- The player arrives alone.
- The room remembers the missing person in every pair: glasses, robes, pillows, chairs, tickets, settings.
- The hotel is the last place the relationship still materially exists.

The design rule underneath all of this:

- The player should understand through being in the space, not through exposition.

## 3. The Game’s Intentions

These are the main intentions that recur across `MASTER_PLAN.md`, `GAMEPLAY_REVISION.md`, and the reference notes:

- Make architecture carry narrative weight.
- Use first-person cinematic grammar, especially cuts, ellipsis, compression, and sudden scale shifts.
- Respect the player’s time.
- Let objects imply biography.
- Avoid explanation when implication is stronger.
- Preserve ambiguity without becoming vague or arbitrary.
- Make movement itself emotionally expressive.
- Build a game that ends before it is fully decoded.

The intended emotional trajectory remains:

- `Arrival`
- `Intimacy`
- `Fracture`
- `Escape / release`

The intended spatial trajectory remains:

- big world / movement / anticipation
- contraction into the hotel
- compression into the room
- rupture into dream / memory / impossible space
- fragmentation into cuts / aftermath / absence

## 4. Design Commandments

The current non-negotiables, restated compactly:

1. Architecture is the primary storytelling medium.
2. Interactions must produce visible physical consequence.
3. Agency is a dial, not a switch.
4. The game establishes a constant, then removes it.
5. Unfinished / raw can be an authored aesthetic register.
6. Inaccessible spaces still communicate.
7. Progression should escalate in ambiguity, not difficulty.
8. Brevity is part of the design.
9. `Twos` are a primary motif.
10. The game should remain melancholy and wondrous, not scary.

## 5. Current Narrative / Spatial Shape

The broad current structure in the plans and engine work is:

- Taxi / arrival
- Threshold / rupture
- Lobby / elevator / glasshouse or observation space
- Corridor of parallel lives
- Suite
- Balcony / window / bed / stars / dream / montage / return

Repeated motifs that matter:

- `twos` becoming `ones`
- objects prepared for two people
- the hotel proceeding with its ritual whether or not the player participates
- cuts that do narrative work
- windows as emotional instruments
- hospitality as both kindness and quiet violence

## 6. What Has Actually Been Built

### Root track: LÖVE / Lua

The root of the repo still contains the older game lineage, including:

- `main.lua`
- `player.lua`
- `proving_ground.lua`
- `fps_mode.lua`
- `endearing_void.lua`

What this means:

- The repo did not begin as a pure EV implementation.
- It includes substantial movement/combat/prototype systems from the Infernal Ascent track.
- Some Endearing Void work exists here as a lower-fidelity narrative prototype.

Important reality:

- The root track is historically valuable and mechanically informative.
- It is no longer the cleanest representation of the intended EV shipping form.

### `ev_engine/`: Raylib / C99

This is the clearest current implementation of Endearing Void as a 3D authored experience.

Current characteristics:

- `C99`
- `Raylib`
- custom rendering
- custom lighting / post-FX
- custom movement / physics
- authored scenes
- GLB-first asset path
- custom QA / grading / screenshot diffing

Notable systems already present:

- scene registry and per-scene structure
- centralized runtime game context
- custom player movement with advanced mobility
- custom render pipeline and grading
- custom asset registry
- automated QA and grading pipeline

This matters because the project is no longer “just using Raylib.” It is increasingly its own game-specific engine layer built on top of Raylib.

## 7. The Multiple Works in This Repo

This project is currently composed of several distinct but related bodies of work:

### A. The game design work

- `MASTER_PLAN.md`
- `GAMEPLAY_REVISION.md`
- the structural and emotional definition of EV

### B. The reference / inspiration work

- `references/README.md`
- Blendo studies
- Beginner’s Guide studies
- cinematic grammar notes
- design synthesis notes

### C. The engine / implementation work

- root `LÖVE` / `Lua` implementation
- `ev_engine/` `Raylib` / `C99` implementation

### D. The content / asset pipeline work

- `BLENDER_PLAN.md`
- `BLENDER_PIPELINE.md`
- authored props and shells
- GLB production workflow

### E. The prototype race / movement-systems work

In `ev_engine/README.md`, the engine also includes a prototype lab for:

- movement
- shooter
- puzzle

This is strategically important:

- it proves the current engine is not only narrative-space oriented
- it preserves and tests the repo’s strongest movement DNA
- it creates pressure toward “fun in the body,” not just “walk and inspect”

## 8. Inspirations

The current inspiration stack is coherent and strong. It is not random moodboarding.

### Primary game lineage

- `Gravity Bone`
- `Thirty Flights of Loving`
- `The Beginner’s Guide`

### Core design lessons from that lineage

- cut aggressively
- architecture tells story
- lo-fi can be expressive, not merely cheap
- pacing and omission matter more than exposition
- engine constraints can become aesthetic identity
- repetition and motifs accumulate meaning

### Film / literature / adjacent references already informing EV

- `Hotel Chevalier`
- `Godard`
- `Barton Fink`
- `Bolaño`
- `Kaufman`
- `WALL-E`
- `Mirror’s Edge`
- `Bioshock Infinite`
- `Firewatch`

These references are not all equally important. The current center of gravity remains:

- `Blendo` for structure and cuts
- `The Beginner’s Guide` for agency, unfinishedness, and emotional spatial logic
- `Hotel Chevalier` for room-as-relationship

### Existing repo reference docs

The main reference set already lives in `references/`:

- `BRENDON_CHUNG_BLENDO.md`
- `THIRTY_FLIGHTS_OF_LOVING.md`
- `GRAVITY_BONE.md`
- `BLENDO_TECHNIQUES_FOR_EV.md`
- `BLENDO_BLOG_INSIGHTS.md`
- `THE_BEGINNERS_GUIDE.md`
- `BEGINNERS_GUIDE_TECHNIQUES_FOR_EV.md`
- `CINEMATIC_GRAMMAR_IN_GAMES.md`

## 9. The Current Gameplay Correction

One of the most important project insights so far is captured in `GAMEPLAY_REVISION.md`:

- the intended game is not “walk to highlighted thing, press E, read text”
- the intended game is “inhabit a space and understand it through presence”

This is a major correction, not a minor tuning note.

The revision argues for:

- fewer labels
- less confirmatory text
- more spatial discovery
- more silent objects
- more presence-based triggering
- more movement as meaning
- more room agency

This is probably the single most important design correction in the repo so far.

## 10. Movement: A Strength, Not a Side Note

A recurring tension in the project:

- the engine already has very strong movement vocabulary
- the narrative design often risks underusing it

Current / recent movement strengths across the repo:

- air movement
- bunny hopping
- wall running
- sliding
- mantling
- dash
- speed-reactive camera treatment
- strong physicality

This is not a problem by itself. It becomes a problem only if:

- the game asks the player to ignore the strongest system in it

The most productive framing is:

- EV should not become a movement shooter
- but movement should still be part of the emotional language of the space

## 11. Content and Asset Strategy

The Blender docs define a deliberately small, controlled authored asset strategy.

Key current principles:

- author models only when boxes cannot say the thing well enough
- keep material language locked and coherent
- stay low poly and silhouette-driven
- rely on engine lighting and material systems, not detailed textures
- treat the environment/prop split deliberately

Current asset budget guidance from `BLENDER_PLAN.md`:

- approximately `16` modeled assets planned in the main budget table
- approximately `3280` tris total in the target plan
- approximately `490KB` new asset budget
- approximately `730KB` total including the existing Sky Tower asset

This is a very disciplined content stance and should remain one of the project’s strengths.

## 12. Testing and Production Discipline

One of the most underrated strengths of the current `ev_engine` work is its QA discipline.

Already present:

- headless tests
- static analysis
- screenshot capture by scene
- report generation
- quality grading
- diffing against baselines
- archived QA history
- clean playtest packaging

This is unusually mature for a small narrative game prototype and should not be casually discarded.

It means the current engine is not just “hobby code.” It has real production thought in it.

## 13. Engine Situation: Where Things Stand

There are currently three meaningful engine positions:

### Position A: stay on the root LÖVE/Lua track

Pros:

- already runs on the current Mac setup
- existing code in place
- lightweight and fast to tweak

Cons:

- not the clearest path toward the intended authored 3D EV
- split identity with the rest of the repo
- lower confidence as the long-term EV implementation

### Position B: continue with `ev_engine` on Raylib/C99

Pros:

- already substantial
- runs on Mac
- aligns with current authored EV direction
- has custom systems tailored to this game
- strong QA infrastructure

Cons:

- content authoring is still more manual than an editor-native workflow
- migration cost rises with every new custom system
- this is increasingly “our own engine,” not just Raylib

### Position C: migrate to `s&box`

Pros:

- modern editor tooling
- scenes, components, systems
- strong content-authoring leverage
- built-in broader engine features
- emotionally appealing target platform / workflow

Cons:

- officially Windows-first as of 2026-03-31
- poor fit for current Mac-only development
- would require another rewrite, not just an engine swap
- some current EV-specific strengths would still need reimplementation

## 14. Current `s&box` Reality

The current emotional desire is real:

- `s&box` feels like a compelling future home for EV
- especially for scene editing, content iteration, and authored first-person spaces

But the current practical reality is:

- it is still officially Windows-focused
- it is not a reliable current Mac development target

Relevant official references:

- [s&box Status](https://sbox.game/dev/doc/about/status/)
- [s&box Getting Started](https://sbox.game/dev/doc/about/getting-started/)
- [s&box Development](https://sbox.game/dev/doc/about/getting-started/development/)
- [s&box GameObject](https://sbox.game/dev/doc/scene/gameobject/)
- [s&box Components](https://sbox.game/dev/doc/scene/components/)
- [s&box Networking & Multiplayer](https://sbox.game/dev/doc/systems/networking-multiplayer/)
- [s&box Cloud Assets](https://sbox.game/dev/doc/assetsresources/cloud-assets/)

Current strategic conclusion:

- `s&box` is attractive
- `s&box` is not the practical Mac answer today
- waiting for a Windows device is currently more rational than trying to force a Mac port contribution

## 15. Gap Between Current EV Engine and `s&box`

Current rough assessment:

- `core game architecture`: moderate gap
- `movement / feel`: small-to-moderate gap
- `rendering identity`: moderate gap
- `content authoring workflow`: very large gap
- `physics / engine breadth / built-in systems`: large gap
- `networking`: massive gap
- `EV-specific QA discipline`: current EV engine is unusually strong here

Important nuance:

- `s&box` is far ahead as a general-purpose modern platform
- but `ev_engine` is already strong in project-specific feel, custom look, and quality discipline

This means a future migration would be:

- not a trivial port
- not a total restart either
- mostly a rewrite of engine-layer code
- but with design, content, references, and emotional shape carrying forward

## 16. Migration Cost Reality

What would carry forward cleanly:

- the game’s emotional premise
- scene list and structural flow
- asset concepts
- reference corpus
- motifs and design commandments
- visual direction
- many authored props and environment ideas

What would partly carry forward:

- state flow
- interaction design
- movement intent
- scene logic
- post-processing targets

What would likely need full or major rewrite in a different engine:

- custom player controller implementation
- scene construction plumbing
- runtime state architecture
- render pipeline
- shader stack
- QA harness integration
- editor/content workflow assumptions

So the current Raylib/C path is not “wasted because it is custom.”
But it is also not “free to migrate later.”

## 17. Costs

There are several different kinds of cost in this project.

### Emotional cost

- engine indecision drains momentum
- rewriting attractive foundations can feel virtuous while actually stalling the game
- building on Mac while wanting `s&box` on Windows creates background dissatisfaction

### Technical cost

- each new EV-specific subsystem increases lock-in to `ev_engine`
- each rewrite discards some engine-specific implementation effort
- every content pipeline change creates additional maintenance burden

### Scope cost

- too many tracks can fragment focus
- overbuilding engine features can crowd out content
- overexplaining can crowd out mystery

### Opportunity cost

- continuing deep engine work in `ev_engine` may be wasteful if the real intention is to ship in `s&box`
- waiting too passively for new hardware risks freezing the project

## 18. Current Strategic Tension

The main tension right now is:

- the project emotionally wants `s&box`
- the current hardware/workflow reality favors `ev_engine`

That creates three plausible near-term stances:

### Stance 1: keep pushing `ev_engine`

Best if:

- the goal is to keep playable progress moving now on Mac
- the team wants to discover the game before committing to a future engine

### Stance 2: freeze heavy engine work and wait for Windows hardware

Best if:

- the emotional engine decision is already basically made
- rewriting more `ev_engine` systems would feel like sunk-cost trap

### Stance 3: use the waiting period for content and design only

Best if:

- the team wants to preserve momentum without deepening platform commitment

This third option is currently the most balanced one if a Windows device is expected soon.

## 19. What Should Still Be Built Even If the Engine Changes

These are safe investments regardless of engine:

- final scene list and scene purposes
- shot list and transition map
- room-by-room object bible
- motif tracking document
- Gibbons dialogue audit
- diegetic text audit
- all reference synthesis
- prop list and asset specs
- shell/blockout designs
- interaction timing maps
- sound motif list
- second-playthrough deltas
- playtest questions

These are all engine-portable forms of progress.

## 20. What Should Be Avoided Right Now

Until engine direction is fully settled, avoid:

- large new engine subsystems with no direct scene payoff
- polishing placeholder interactions that violate the revised design philosophy
- writing explanatory text that props up weak spatial storytelling
- producing high-detail assets that exceed the visual budget
- adding systems just because they are “cool” in isolation

## 21. Recommended Current Working Position

Current best synthesis:

- Endearing Void is real and already well-defined.
- The project’s strongest achievements so far are in design clarity, references, movement feel, asset discipline, and QA rigor.
- `ev_engine` is already substantial enough that it deserves respect as real work, not disposable prototyping.
- `s&box` remains the most emotionally attractive future home, mainly because of authoring workflow and engine leverage.
- Because `s&box` is still a poor Mac fit, the smartest short-term path is probably to avoid deepening custom engine lock-in while continuing engine-portable game design and content work.

Put bluntly:

- do not confuse “more engine work” with “more game”
- do not throw away the strong design brain already built here
- do not let the perfect future setup stop all present progress

## 22. Canon

If there is conflict between ad hoc implementation choices and the higher-level intent, the intended canon should currently be:

1. `MASTER_PLAN.md`
2. `GAMEPLAY_REVISION.md`
3. `references/` synthesis docs
4. current playable implementation details

That means:

- implementation should answer to design, not the reverse
- the revised “inhabit the space” direction should outrank legacy “press E and read text” patterns

## 23. Open Questions

These are the main unresolved questions still shaping the project:

- Is the next serious implementation push happening in `ev_engine` or after waiting for Windows hardware?
- How much of the movement depth should remain central in EV versus ambient?
- How much authored dialogue should Gibbons retain?
- Which interactions should be presence-based versus explicit?
- How far should the impossible architecture escalate before the ending?
- How much of the current prototype-lab ethos belongs in the final EV experience?

## 24. The Core Reminder

The project is not fundamentally about:

- a hotel
- shaders
- movement tech
- sadness as aesthetic
- or engine choice by itself

It is about:

- one person arriving in a space prepared for two
- and understanding the absence through the physical world

If a future decision strengthens that, it belongs.
If it distracts from that, it probably does not.
