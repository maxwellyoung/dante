# Cinematic Grammar in Games — Reference

How film editing techniques translate to interactive first-person experiences. Compiled from Blendo Games analysis, critical writing, and game design discourse.

---

## The Problem

Cinema edits out time when nothing important happens. Games show everything — the whole journey.

> "While cinema edits out the time when nothing or unimportant action is taking place, video games typically show everything and the whole journey." — PopMatts

Most games fill dead time with traversal, combat encounters, or collectibles. Chung's solution: **cut it**.

---

## Film Techniques → Game Equivalents

### The Jump Cut (Godard, Breathless 1960)
**Film:** Remove frames within a continuous shot, creating jarring temporal leaps.
**Game:** Teleport the player from one scene/time to another without transition. Walk through a door in a bar → emerge in an airport. First-person perspective never breaks.

**Key insight:** The jump cut in games is more disorienting than in film because the player has agency. You expect the world to respond continuously to your movement. When it doesn't, the violation of spatial expectation becomes the narrative device.

### Ellipsis (Standard editing)
**Film:** Skip over time. Character leaves house → cut → character arrives at office.
**Game:** Skip the boring traversal. The heist → cut → the aftermath. Hours of waiting → clock jumps.

**Key insight:** Games can represent ellipsis spatially. A clock on the wall changes. Characters appear where they weren't. Objects have moved. The player infers the passage of time from environmental changes.

### The Smash Cut (Comedy / horror technique)
**Film:** Abrupt cut from one scene to a dramatically different one for emotional contrast.
**Game:** Thirty Flights: intimate bar scene → mid-gunfight chaos. The tonal whiplash IS the storytelling.

### Montage (Eisenstein, Soviet cinema)
**Film:** Juxtapose images to create meaning that neither image has alone.
**Game:** Gravity Bone's death montage. The juxtaposition of spy gameplay with personal photographs creates meaning: this character had a life you never saw.

### The Establishing Shot (Standard grammar)
**Film:** Wide shot of a location before cutting to action within it.
**Game:** The first moments in a new space before interaction becomes possible. EV's HOTEL_EXT scene — you see the hotel before you enter it. The building IS the establishing shot.

### Match Cut (2001: A Space Odyssey, bone → satellite)
**Film:** Cut from one image to a visually similar but contextually different one.
**Game:** Walk through a doorway shaped like X → emerge in a space shaped like X but in a different time/place. Visual continuity, contextual discontinuity.

---

## Why First-Person Makes This Powerful

In film, the camera is a third party — you watch characters experience narrative.
In first-person games, YOU are the camera. Cuts happen to you. Disorientation is yours. When Gravity Bone's spy is shot, the player's spatial ownership of the character makes it personal.

**The critical difference:** Film editing is done TO the viewer. Game editing is done WITH the player's complicity — they walked through the door, they pressed the button. They chose to be in the next scene. The cut feels both imposed and earned.

---

## Games in the Lineage

| Game | Year | Technique | From |
|---|---|---|---|
| Gravity Bone | 2008 | Death montage, environmental storytelling | Blendo |
| Thirty Flights of Loving | 2012 | In-gameplay jump cuts, ellipsis | Blendo |
| The Stanley Parable | 2013 | Narrative misdirection, spatial subversion | Davey Wreden |
| Gone Home | 2013 | Environmental storytelling, absence as narrative | Fullbright |
| Virginia | 2016 | Full jump-cut narrative, no dialogue | Variable State |
| Paratopic | 2018 | Lo-fi fragmented narrative, PS1 aesthetic | Arbitrary Metric |
| Norco | 2022 | Southern Gothic environmental storytelling | Geography of Robots |

---

## Application to EV

EV's scene flow is already cinematic:
```
TITLE → TAXI (auto) → ARRIVAL (auto) → HOTEL_EXT (walk)
→ LOBBY → HALLWAY → ROOM (tasks) → BALCONY (Paris)
→ BED (2D) → STARS (title card)
```

### Current Cuts
- TITLE → TAXI: Smash cut (silence → taxi interior)
- TAXI → ARRIVAL: Ellipsis (travel time compressed)
- ARRIVAL → HOTEL_EXT: Match cut (exiting taxi → standing outside)
- HOTEL_EXT → LOBBY: Spatial continuity (walking through door)
- ROOM → BALCONY: Spatial continuity (opening balcony door)
- BALCONY → BED: Ellipsis + tonal shift (Paris night → interior intimacy)
- BED → STARS: Jump cut (hotel room → cosmic void)

### Opportunities
- The BED → STARS transition is the most Blendo moment. It should be the most abrupt cut in the game. No fade. Hotel room → void. The scale shift IS the narrative climax.
- Consider: can the LOBBY → HALLWAY → ROOM sequence use time jumps? Clocks changing, light shifting, suggesting Walter has been here longer than the player realizes?
- The five room tasks could use micro-ellipsis — complete one task, cut to a slightly different room state (candle now burned lower, light shifted, something moved). Time passes between interactions.

---

## The Golden Rule

> "The space between the cuts suggests a wealth of information." — On Godard

What you don't show matters more than what you do. The cut is not an absence — it's an invitation for the player to fill the gap with their own imagination. Trust them.
