# New Game Design Ideas

This document is for brainstorming and outlining new concepts that may or may not be related to "Infernal Ascent."

---

## Idea 1: [Working Title]

### High-Concept Pitch

(A one-sentence hook for the game.)

### Core Mechanics

(List the primary verbs and gameplay systems.)

### Design Pillars

(What are the 3-4 core principles guiding the design?)

### Why This Is Interesting

(What makes this idea stand out from other games?)

---

## Idea 2: [Working Title]

### High-Concept Pitch

(A one-sentence hook for the game.)

### Core Mechanics

(List the primary verbs and gameplay systems.)

### Design Pillars

(What are the 3-4 core principles guiding the design?)

### Why This Is Interesting

(What makes this idea stand out from other games?)

### Brutal Design Critique

**1. Subtractive Metroidvania ≠ fun by default.**
If each descent strips power, you risk a _reverse difficulty curve_: early zones are fireworks, later zones a limp jog. Players tolerate loss only when (a) the trade-off yields fresh mental puzzles or (b) emotional stakes explode. Map explicit "compensation loops" per circle (e.g. new environmental affordances, emergent routes that only _less-powerful_ move-sets can exploit). Otherwise it's just masochism cosplay.

**2. "Micro" + "Dense world" collide.**
One screen per circle _and_ Animal-Well tier secrets? Either scope balloons or secrets become toddler-obvious. Decide: do you want bite-sized purity (Spelunky tutorial room vibes) or labyrinthine intrigue? Pick one; hybrid ≈ beige.

**3. Tactile perfection is a black hole.**
Celeste's feel was half a decade of iteration by a team of platformer lifers. Solo dev? Sneak-scope: lock mechanics _early_, then freeze them. Resist infinite tweaking or you'll ship in 2077 (the year, not the meme).

**4. Narrative via ability loss needs readable telegraphy.**
If the dash vanishes in Circle VI but level geometry still screams "dash here", players feel robbed. Bake gradient signposting: emerging platform patterns that _invite_ the reduced kit. Also ensure the "why" lands: a seven-word epigram or visual gag at the transition point beats cryptic tercets nobody reads.

**5. Symbolic challenge ≠ Kaizo traps.**
Tie each sin's punishments to its vice: Lust → magnetic pull to hazards, Greed → currency that weighs you down, etc. Avoid generic spike spam. Metaphor or bust.

**6. Melancholy empowerment is knife-edge tone.**
Music, palette, and FX must counterbalance frustration. Think _bleeding-sunset pastels + downtuned harp arps_, not edgy chiptune EDM.

**7. Toolchain reality check.**
Love2D/SDL2 gives you god-tier control, but you'll roll your own everything—collision, input, scene graph—aka yak farm. Godot 4 (GDExtension for low-level feel) might be saner: hot-reload, built-in physics, export to Switch if dreams grow.

---

#### Suggested Next Steps

1. **Ability-Loss Matrix** – draft a table: Circle, Removed Power, New Environmental Hook, Emotional Beat, Anticipated Player Skill-Test. If any row's hook feels weaker than the loss, redesign.

2. **Vertical Slice Prototype** – one circle (say, Lust). Hard-lock controls, art placeholder. Measure: ⟨fun with reduced kit?⟩ If no, nuke concept early.

3. **Playtest Loop** – micro-demo to 5 friends. Watch frustration  drop-off curve. Record _time-to-flow_ versus _deaths per canto_. Adjust tightness, not cruelty.

4. **Production Guardrails** – 6-month target? Cap total circles at 3–4. "Inferno bingo" is less important than polished feel.

Rip into these and ping back once the matrix exists; we'll savage the specifics.
