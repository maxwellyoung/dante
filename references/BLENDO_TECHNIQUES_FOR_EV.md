# Blendo Games Techniques → Endearing Void Application Guide

A synthesis document. How to steal from Brendon Chung correctly.

---

## The Three Big Lessons

### 1. Cut Everything That Isn't Essential
Thirty Flights never shows the heist. Gravity Bone never explains the spy's backstory. The void between scenes is where the player's imagination does the real work.

**For EV:** The taxi ride is a cut — you don't drive to the hotel, you arrive. The game flow (TITLE → TAXI → ARRIVAL → HOTEL_EXT → LOBBY → HALLWAY → ROOM → BALCONY → BED → STARS) already uses this. Every transition should feel like a Godard jump cut. Ask: "Would Chung show this, or cut away?"

### 2. Architecture Is Narrative
Chung's spaces tell stories without a single word of dialogue. The furnace room suitcase in Gravity Bone. The ammunition-littered bar hideout in Thirty Flights. Objects in space = story.

**For EV:** The hotel room's five multi-step tasks ARE the narrative. A half-empty glass, a rumpled bed, a candle to light — each object tells Walter's story. The lobby's architecture should guide the player up to the room the way Disneyland guides you through a ride. No waypoints. No arrows. Architecture.

### 3. Lo-Fi Is Not a Limitation — It's the Point
Chung could have used any engine. He chose Quake 2. The blockiness, the flat shading, the simplified characters — these are aesthetic commitments, not compromises. They force the player to project meaning.

**For EV:** 480x300 is not "retro for retro's sake." It forces bold shapes, strong color, visible consequence. If a detail isn't 3+ pixels wide, it doesn't exist. This is Dieter Rams via Quake.

---

## Specific Techniques to Steal

### The Jump Cut (Thirty Flights)
Instantaneous scene transition with no fade, no loading screen, no explanation. The player walks through a door and is somewhere entirely different.

**Implementation:** EV already does this with scene transitions. Make them MORE abrupt. The TAXI → ARRIVAL → HOTEL_EXT sequence should feel like you're being edited, not like you're watching a cutscene. Consider: can any current fade-transitions become hard cuts?

### The Photo Montage (Gravity Bone)
At the moment of death, a life flashes before your eyes — moments the game never showed you. Reveals the gap between what you played and who the character was.

**Implementation:** The BED → STARS transition. As Walter lies in bed watching phosphorescent stars, what if brief flashes of other moments intrude? Not death, but the edge of sleep — that liminal state where memories surface involuntarily. Consistent with EV's "wonder, not horror" principle.

### Environmental Storytelling Without Text (Both Games)
Neither game has text popups or UI explanations. Objects, spatial relationships, and architecture communicate everything.

**Implementation:** EV already follows this ("No hand-holding: No task counter, no exit beacon, no tutorial text"). Push further. Can any remaining textual elements be replaced with spatial ones? The room's tasks shouldn't feel like a checklist — they should feel like inhabiting a space.

### Musical Rhythm in Scene Pacing (Thirty Flights)
The gunfight scene was designed with "musical rhythm" from Koyaanisqatsi. Pacing as music, not as gameplay loop.

**Implementation:** EV's procedural audio should create rhythmic texture. Footstep cadence changes per surface (already in). The question: does each SCENE have a rhythm? The lobby should feel different from the hallway, which should feel different from the room. Not just in sound — in how the player moves through them.

### The Rug Pull (Gravity Bone)
Teach the player rules, then break them. Build safety, then take it away.

**Implementation:** EV's approach is gentler — wonder, not violence. But the principle applies. The hotel feels contained, safe, warm. The balcony opens to the infinite Paris night. The stars sequence breaks spatial logic entirely. The "rug pull" in EV is scale — from intimate hotel room to cosmic void. Same technique, different register.

### Theme Park Wayfinding (Thirty Flights)
Spaces that are "aesthetically gorgeous, infused with story, suitable for people to walk through, and understandable so people don't get lost."

**Implementation:** Every EV space should pass this test:
1. Is it gorgeous at 480x300?
2. Does it tell part of Walter's story?
3. Can you walk through it without getting stuck?
4. Do you know where to go without being told?

### Subtractive Design (All Blendo Games)
Strip to essentials. If it would require "a million monopoly pieces," cut it.

**Implementation:** Regular audit. For every feature: "Does this earn its place?" If removing it doesn't break the experience, remove it. EV's CLAUDE.md already codifies this as "Ethical reduction (Rams): Remove until it breaks, add back one thing."

---

## The Spiritual Connection

Gravity Bone and Thirty Flights of Loving are spy stories about criminals in a fictional South American city. Endearing Void is a story about a pilot in a Paris hotel. The genre is different. The technique is the same:

**Short-form. Environmental. Lo-fi. Authored. Every second earned.**

Both Blendo games and EV share:
- First-person perspective as identity (you ARE the character)
- Mundane actions as narrative (delivering a drink / lighting a candle)
- Architecture as storytelling (the embassy / the hotel)
- Constraint as aesthetic (Quake 2 / 480x300 C99)
- Brevity as respect (13 minutes / one night)
- The gap between what you do and what you feel

The difference: Blendo games are about **looking back** (heists gone wrong, lives already lived, deaths already happened). EV is about **looking forward** (what happens when the three hours are up, what the void holds, what the stars mean).

Both trust the player to fill the void with meaning.

---

## Key References to Revisit

When stuck on EV design decisions, consult:

1. **GDC 2015 talk** — Wayfinding & Storytelling: [archive.org/details/GDC2015Chung](https://archive.org/details/GDC2015Chung)
2. **Designer Notes podcast** — Ep. 46-47: [idlethumbs.net/designernotes/episodes/brendon-chung-part-1](https://www.idlethumbs.net/designernotes/episodes/brendon-chung-part-1)
3. **Thirty Flights source code** — [github.com/blendogames/thirtyflightsofloving](https://github.com/blendogames/thirtyflightsofloving)
4. **Gravity Bone source code** — [github.com/blendogames/gravitybone](https://github.com/blendogames/gravitybone)
5. **Critical Path video** — "Creating a Frictionless Experience"
6. **GDC 2012 talk** — "Nuke It From Orbit" (subtractive design case study): [gdcvault.com/play/1015688](https://gdcvault.com/play/1015688/Nuke-It-From-Orbit-the)
