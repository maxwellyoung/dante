# Endearing Void — Master Plan

> "You don't need much. Spaces that mean something. A perspective that commits to its constraints. Trust that the player will fill the void. The courage to end before you've explained everything."

This document is the long-term development plan for Endearing Void. Every design decision should answer to this plan. When in doubt, return here.

---

## I. What Endearing Void Is

A first-person narrative game about three hours to kill in an impossible hotel. 15-30 minutes long. Built in C99 with Raylib. 480x300 lo-fi. No hand-holding, no fail states, no dialogue trees. Architecture is the story. The void is the point.

**The emotional trajectory:** Arrival → Intimacy → Infinity. Taxi → Hotel → Stars. The world contracts (taxi → corridor → room) then explodes open (balcony → sky → void). The player experiences wonder, not fear. Melancholy, not depression. The feeling of being alone in a beautiful place at 2 AM and knowing it won't last.

**The spiritual lineage:**
- **Gravity Bone / Thirty Flights of Loving** — Architecture IS narrative. Cut everything non-essential. Lo-fi is a commitment.
- **The Beginner's Guide** — Agency is a dial. Unfinished spaces are intentional. Silence after sound is devastating.
- **Godard** — Voice contradicts image. Bold color against neutral. The audience is not always comfortable.

---

## II. Design Commandments

These are non-negotiable. Derived from all research.

### 1. Architecture Is the Primary Storytelling Medium
No cutscenes. No text dumps. No exposition. Every room tells its story through spatial composition, material, light, and the objects placed within it. If a narrative beat can't be communicated through architecture, it doesn't belong in the game. (Chung, Wreden, Keogh)

### 2. Every Interaction Has Visible Physical Consequence
Light a candle → see flames. Pour champagne → hear the glass fill. Open curtains → the room transforms. If the player can't see the change in a screenshot, the interaction doesn't count. (Chung, Vlambeer)

### 3. Player Agency Is a Dial, Not a Switch
Movement speed, FOV, camera freedom, and control responsiveness should all vary by scene and emotional beat. The hallway is slower than the lobby. The balcony is wider than the room. The bed surrenders control. Restriction at emotional peaks, release at climaxes. (Wreden, Beginner's Guide)

### 4. The Constant, Then Its Absence
Identify what the player takes for granted — ambient sound, spatial logic, the hum of the hotel — and remove it at the climax. The stars sequence should be silence after sound, void after architecture, stillness after motion. (Wreden, Emily Short)

### 5. Unfinished Is an Aesthetic Register, Not a Bug
Raylib's rendering constraints are features. Raw geometry, procedural textures, visible seams — these communicate authenticity and intimacy. Some spaces can shift between polished (lobby, suite) and raw (service corridor, dream sequence, the void). The quality shift IS narrative. (Wreden, Yang, Beginner's Guide)

### 6. Inaccessible Spaces Communicate
Rooms the player hears but never enters. Music through a wall. A light under a door that goes out. Other lives happening in parallel, forever out of reach. The hotel is populated by absence. (Short, Keogh, Gemsbok)

### 7. Progression Escalates in Ambiguity, Not Difficulty
Early spaces are legible — room numbers, signage, clear paths. Later spaces erode certainty — the corridor that shouldn't connect, the window showing the wrong time, the room that's larger inside than outside. The game teaches the player its rules, then quietly breaks them. (Gemsbok, Beginner's Guide)

### 8. Brevity Is Respect
15-30 minutes. Every second earned. No padding, no backtracking, no filler. The game ends before it's explained everything. Trust the player to carry the meaning forward after the screen goes dark. (Chung, Wreden)

### 9. Recurring Motifs Create Meaning Through Repetition
An object, a color, a sound, a spatial pattern that appears in every scene. Seemingly decorative. Accumulating weight. Potentially subverted. The player should notice it on the second playthrough and realize it was there all along. (Beginner's Guide lamppost principle)

### 10. Wonder, Not Horror. Always.
Bold color, not desaturation. Warmth, not cold. Open sky, not closed ceiling. If it sounds scary, it IS scary — fix it. The void is beautiful and melancholy, never threatening. (Maxwell's taste, consistently enforced)

---

## III. The Game Flow — Definitive Structure

### Act 1: Arrival (Contraction)
The world narrows. Public → private. Movement → stillness.

```
TITLE
  ↓ (Enter)
TAXI — Auckland, 2 AM. Inside the car. City scrolls past.
       Sky Tower grows in the windshield. ~15 seconds.
       Emotional register: anticipation, motion, urban rhythm.
       Audio: city ambient, engine hum, tires on wet road.
  ↓ (auto-transition)
HYPERSPACE — The tower becomes a tunnel. Reality breaks.
             Disorienting roll, extreme chromatic aberration.
             Hard cut to silence.
             Emotional register: vertigo, threshold crossing.
  ↓ (hard cut)
SPACE_LOBBY — You're in orbit. Earth through the observation window.
              Gibbons (cube-person bellboy) appears, leads you.
              The hotel exists. It shouldn't, but it does.
              Emotional register: wonder, impossibility, warmth.
              Audio: hull resonance, air circulation, insulated quiet.
  ↓ (walk to exit)
SPACE_CORRIDOR — Portholes show Earth receding.
                 Long walk. Gibbons ahead of you.
                 Emotional register: awe becoming routine.
                 The impossible is already normalizing.
```

### Act 2: Intimacy (The Room)
The world becomes one room. The player inhabits it.

```
SPACE_SUITE — Your room. 4 interactive tasks:
              Lamp (light transforms the space)
              Champagne (celebration for no one)
              Desk (something left behind by a previous guest)
              Bed (the final task — you're done)
              Gibbons bows. Deactivates. You're alone.
              Emotional register: ritual, solitude, care.
              Audio: room drone, clock tick, fabric sounds.

              [KEY DESIGN BEAT: This is where the game lives.
               The tasks are not objectives. They are rituals.
               The player is preparing a space for sleep.
               The slowness is the point.]
```

### Act 3: Infinity (Expansion)
The world explodes open. Private → cosmic. Stillness → transcendence.

```
BALCONY — Step outside. Earth fills the view.
          One interaction: cigarette. Contemplative.
          Emotional register: scale, loneliness, beauty.
          Audio: void wind, no city, no hotel hum.

          [KEY DESIGN BEAT: The balcony is where the
           "constant" begins to dissolve. The hotel sounds
           are gone. The architecture opens. The player
           feels the void for the first time.]
  ↓ (return to room, approach bed)
BED — Lie down. Phosphorescent stars on the ceiling.
      Controls go soft. Camera drifts.
      The room dissolves.
      Emotional register: surrender, liminal, falling asleep.
  ↓ (fade)
STARS — The void. Credits. Silence.
        Emotional register: infinity, release.
        The game is over. The feeling is not.
```

### The Paris Hotel (Future / Parallel)
The orphaned Paris scenes (HALLWAY, ROOM, BATHROOM) represent the original concept — a terrestrial hotel. These are preserved for potential reintegration as:
- **A dream sequence** — the player falls asleep in the space suite and wakes up in a Paris hotel that dissolves
- **A parallel path** — the game can be played as either Auckland→Space or Auckland→Paris, each with its own emotional register
- **A second playthrough reward** — after completing the space path, the Paris path unlocks

Decision deferred. The Paris scenes are ~350 lines of solid craft. They'll find their place.

---

## IV. Development Phases

### Phase 1: The Room (Current → Next)
**Goal:** Make the Space Suite the best room in any walking simulator.

The suite is where the game lives or dies. 60% of playtime should be here. Every task should feel like a ritual, not a checklist.

- [ ] **Interaction depth.** Each of the 4 tasks should have multi-step progression with visible consequence. Lamp: click → warm light floods room → shadows shift. Champagne: pop → pour → glass on nightstand catches light. Desk: open → discover something left by a previous guest → close. Bed: pull covers → lie down → ceiling stars appear.
- [ ] **Cork pop sound.** The champagne interaction needs a distinctive procedural sound (TODO already in main.c).
- [ ] **Object storytelling.** Add 2-3 non-interactive objects that tell stories by existing. A half-packed suitcase. A postcard face-down on the desk. Shoes by the door. Not interactable — just present. Architecture as narrative.
- [ ] **Room acoustic refinement.** The suite should have its own audio character distinct from corridor and lobby. Intimate, warm, slightly muffled. The clock tick should feel like a heartbeat.
- [ ] **Speed curve.** Implement per-position movement speed modulation in the suite. Near the window: slower. Near the bed: softer. The room teaches you to be still.

### Phase 2: The Corridor (Pacing)
**Goal:** The walk from lobby to suite should feel like a journey, not a loading screen.

- [ ] **Inaccessible rooms.** Add doors along the corridor that the player passes. Some have light underneath. One has muffled music. One goes dark as you approach. Other lives, forever out of reach. (Short, Keogh)
- [ ] **Gibbons refinement.** The NPC should feel like a real bellboy — purposeful, slightly formal, not quite human. His walk cycle should have character. He should pause at the suite door and gesture you in.
- [ ] **Porthole moments.** The Earth views through corridor portholes should shift as you walk — the planet rotates, clouds move. Subtle proof that time is passing and the hotel is real.
- [ ] **Spatial impossibility (subtle).** The corridor should be slightly longer from one end than the other. Not enough to notice consciously — enough to feel. Ambiguity escalation begins here. (Gemsbok)

### Phase 3: The Lobby (First Impression)
**Goal:** The lobby should sell the impossible premise in 30 seconds.

- [ ] **Earth through observation window.** This is the first thing the player sees after hyperspace. It must be the most beautiful thing in the game. Atmosphere glow, cloud patterns, city lights on the night side.
- [ ] **Architectural grandeur at 480x300.** Big shapes. Columns. A chandelier. Brass and marble. The lobby says "this is a real hotel" despite floating in orbit. Bold Rodkin shapes, not tiny details.
- [ ] **Ambient life.** Sounds of a hotel that has other guests — distant elevator ding, muffled conversation, footsteps on marble from somewhere you can't see. The player is not the only one here. But they never see anyone except Gibbons.
- [ ] **Newspaper interaction.** Already exists. Ensure it tells a story. What's in the news at an orbital hotel? Not exposition — texture.

### Phase 4: The Transitions (Emotional Architecture)
**Goal:** Every transition between scenes should carry emotional weight.

- [ ] **Taxi → Hyperspace.** The hard cut from Auckland reality to impossible wormhole. This is the game's biggest tonal shift. It should feel like Gravity Bone's rug pull — the rules change without warning. Ensure the chromatic aberration and disorientation are calibrated for wonder, not nausea.
- [ ] **Hyperspace → Space Lobby.** Hard cut to silence. The most important transition in the game. After sensory overload, absolute calm. The player's ears ring with the absence. (Beginner's Guide: silence after sound)
- [ ] **Suite → Balcony.** The door opens. The hotel sounds disappear. The FOV widens. Movement speeds up slightly. The void is outside. This is the breath before the ending.
- [ ] **Balcony → Bed → Stars.** The dissolve. Controls soften. Architecture dissolves. The game lets go of the player gently.

### Phase 5: Sound Design (The Invisible Architecture)
**Goal:** Sound tells the stories that geometry can't.

- [ ] **Per-scene acoustic signature.** Every space should have a unique ambient drone that the player learns unconsciously. Lobby: expansive, reverberant. Corridor: narrow, directional. Suite: intimate, warm. Balcony: void wind, no reflections. Stars: silence.
- [ ] **Composed music.** The game needs at least one composed piece — not a procedural drone, but a melody. It should appear once, in the suite, and never repeat. The moment it plays should be the emotional center of the game.
- [ ] **Inaccessible room sounds.** Music through walls. Conversation through floors. Running water. A phone ringing once, then stopping. The hotel is alive around you.
- [ ] **The removal.** Whatever the game's ambient constant is (hotel hum, air circulation, the subtle always-present sound), it should disappear completely during the stars sequence. The player should feel its absence physically.

### Phase 6: The Recurring Motif (The Lamppost)
**Goal:** Plant an object/pattern that accumulates meaning across the entire game.

- [ ] **Choose the motif.** Candidates: a specific color (Godard red appears in unexpected places). A specific object (a particular lamp, a particular fabric pattern). A specific sound (a three-note sequence). A spatial pattern (a doorframe shape that repeats).
- [ ] **Place it everywhere.** In the taxi. In the lobby. In the corridor. In the suite. On the balcony. In the stars. Always present, never highlighted.
- [ ] **Decide on subversion (or not).** The Beginner's Guide subverts its lamppost — reveals it as a lie. EV doesn't need that specific move. The motif can simply accumulate — becoming the player's private recognition, their proof that the hotel is authored. Or it can shift — the color changes, the sound transposes, the object breaks. Decision deferred until the motif is chosen and placed.

### Phase 7: The Paris Hotel (Integration)
**Goal:** Determine whether and how the orphaned Paris scenes rejoin the game.

- [ ] **Playtest the space path as-is.** Is 15-30 minutes enough? Does the game feel complete without Paris?
- [ ] **If yes:** Paris becomes a second playthrough variant. Same structure, different setting. Terrestrial warmth vs. orbital wonder. The game is "three hours to kill" in two different hotels.
- [ ] **If no:** Paris integrates as a dream sequence or memory. The player falls asleep in the space suite and briefly inhabits the Paris room. A Godard jump cut — you're in Paris now, you were always in Paris, you were never in Paris. Then back to orbit. The dream is the game's emotional core.
- [ ] **Either way:** The Paris scenes need their own acoustic signature, their own lighting, their own motif placement. They are not leftovers — they are craft.

### Phase 8: Polish & Release
**Goal:** Ship. Short and dense, not long and sprawling.

- [ ] **Playtesting.** 5-10 people who've never seen it. Watch them play silently. Note where they pause, where they rush, where they get lost, where they stop and look.
- [ ] **Performance.** Locked 60fps. No dropped frames. The lo-fi aesthetic demands smoothness — if the frame rate stutters, the illusion breaks.
- [ ] **Platform targets.** macOS first (already working). Windows, Linux via Raylib cross-compilation. Itch.io release.
- [ ] **Runtime target.** 15-30 minutes first playthrough. No more. If it's longer, cut.
- [ ] **Price.** Free or $3-5. The game earns respect through craft, not length.
- [ ] **README / store page.** "Three hours to kill." One screenshot. One sentence. Trust the work.

---

## V. Technical Priorities

Ordered by impact on the player's experience.

### High Priority
1. **Per-position movement speed curves.** Implement speed-as-function-of-position in key scenes (suite, corridor, balcony). The most impactful emotional tool available at zero rendering cost.
2. **Inaccessible room audio.** Add muffled sounds behind corridor doors. Audio sources positioned in 3D space behind the walls. Raylib supports this.
3. **Interaction multi-step refinement.** Each of the 4 suite tasks should have 2-3 visible stages, not just on/off.
4. **Composed music system.** One track. Triggered in the suite at the right moment. Fades naturally.

### Medium Priority
5. **Subtle spatial impossibilities.** Corridor length asymmetry. Window time-of-day inconsistency. Rooms that feel slightly wrong without the player being able to identify why.
6. **Non-interactive storytelling objects.** Suitcase, postcard, shoes. Scene dressing that implies narrative without interaction.
7. **Gibbons character polish.** Walk cycle personality. The bow at the suite door. Formal, geometric, not-quite-human.
8. **Visual motif system.** The recurring object/color/pattern. Requires design decision before implementation.

### Lower Priority (Long-term)
9. **Paris integration.** Requires playtesting to determine approach.
10. **Additional visual style presets.** The 9 existing presets are sufficient. Only add if a specific scene demands a unique look.
11. **Cross-platform builds.** Windows/Linux. Raylib makes this straightforward but it's not urgent.
12. **Advanced post-FX.** Bloom, depth-of-field for the stars sequence. Only if they serve the emotional beat.

---

## VI. What Not to Do

These are temptations. Resist them.

- **Don't add combat, puzzles, or fail states.** The game is about inhabiting a space, not conquering it.
- **Don't add a map or minimap.** Architecture guides the player. If they get lost, the architecture is wrong.
- **Don't add collectibles.** Objects exist to tell stories, not to be collected.
- **Don't add dialogue.** If the game ever uses voice, it should be located (phone, radio, PA system), unreliable, and contradicted by the space. Never omniscient narration.
- **Don't add more scenes to increase runtime.** Add depth to existing scenes instead. The suite should be richer, not the game longer.
- **Don't chase realism.** 480x300 with procedural textures IS the aesthetic. Don't add PBR, don't add normal maps, don't add real textures. The lo-fi is the commitment.
- **Don't explain the hotel.** Why is there a hotel in orbit? Why did the Sky Tower become a wormhole? The player doesn't need answers. The mystery is the point. (Wreden: "Would you simply let them be what they are.")
- **Don't over-read your own influences.** Reference Blendo and Godard without pretending to channel them. Maybe you just like making hotels. (Beginner's Guide: "Maybe he just likes making prisons.")

---

## VII. The North Star

When everything else is noise, return to this:

**The game is one night in an impossible hotel. The player arrives, prepares a room, steps onto the balcony, sees the Earth, lies down, and falls asleep. The game ends. The void remains.**

Everything in the game serves that sequence. Everything that doesn't serve it gets cut. Ethical reduction. Brevity as respect. Architecture as narrative. The courage to end before you've explained everything.

Every second earned.
