# Endearing Void — Gameplay Revision

> The master plan is right about what the game IS. This document is about what the game DOES — the second-to-second experience. Where the fun lives.

Written: 2026-03-19

---

## The Problem

The master plan says "architecture is the primary storytelling medium" and "these are not puzzles, they are facts about a room." But the implementation is 40+ show_text() calls. Every object has a label. The sock says "Not yours." The wardrobe says "Two robes. Different sizes." The game doesn't trust the player to see two robes and understand what two robes means.

**What we built:** Walk to glowing thing → Press E → Read text → Walk to next glowing thing.

**What the plan describes:** Inhabit a space. Notice things. Understand through architecture.

These are different games. The first is a text adventure with 3D graphics. The second is a spatial experience. We need to become the second.

---

## I. The Three Types of Fun in EV

### 1. Movement Fun (already built, underused)

The engine has Quake air strafing, bunny hopping, wall running, sliding, mantling, momentum physics. This is a *world-class* movement system sitting inside a game that asks you to walk to a lamp and press E.

**The opportunity:** Movement IS interaction. The game already does this in three places — and they're the best moments:

- **Observation window approach** — speed slows, FOV narrows, grain clears. The game physically pulls you toward Earth. You never press E. The approach IS the interaction.
- **Corridor windows** — slowing near windows, compression toward the suite door. The hallway teaches you to look sideways.
- **Bed speed zone** — movement softens near the bed. The room teaches you to be still.

**What's missing:** These moments are passive — the game manipulates YOU. The player never gets to discover what their movement system can do in this space.

**Proposals:**

a) **The balcony railing.** Low gravity (0.3x). Brass railing at waist height. The player can jump and land ON the railing. No prompt. No text. Just: the physics allow it. Standing on a brass rail at the edge of a space station looking down at Earth. The game doesn't acknowledge it. The player found a moment.

b) **The corridor bunny hop.** Gibbons walks at 3.5 speed. The player walks at 4.5. If the player bunny-hops ahead of Gibbons, he stops. Waits. Says nothing. Resumes when you're near. The movement system creates a character moment — you're nervous, you're fidgeting with physics, and Gibbons is patient.

c) **Zero-g drift.** After the bed ritual takes control and the player lies down — when it hard-cuts to cleaned suite — what if for 3 seconds before the cut, gravity goes to zero? You drift. Just slightly. The bed, the ceiling, the room floating. Not a mechanic. A feeling. Then hard cut.

d) **The return taxi momentum.** The player has been moving through the entire game. The return taxi is the first time they're still for an extended period. The contrast should be physical — the camera settles, the FOV narrows to resting state, the bob stops. Stillness as the final movement state.

### 2. Discovery Fun (not built yet)

Right now, the player who walks straight to the four task objects has the same experience as the player who examines everything. There's no reason to explore.

**The principle:** Reward curiosity with SPATIAL revelation, not text.

**Proposals:**

a) **Silent objects.** Half the interactable objects should say NOTHING when pressed. Open the wardrobe — see two robes. No text. The player's brain does the work. Open the adjoining door — see the dark room with one pillow. No text. The silence IS the content.

KEEP text for: booking confirmation ("2 guests."), photograph ("Paris. Before all this."), room service card ("Two handwritings. A disagreement about the cheese plate."). These are READING — the player is reading an object. Text for reading is diegetic.

REMOVE text for: sock, wine glass, book, bath, wardrobe, suitcase, phone. These are SEEING. You see a sock under a bed. You don't need text to tell you it's "Not yours."

b) **The window you weren't supposed to find.** Somewhere in the suite — behind the bathroom, or through the adjoining room — there's a window that looks out at nothing. Not Earth. Not space. Just void. Pure black. No stars. The player has to actively explore to find it. No text. No interaction. It's just there. On second playthrough they go looking for it.

c) **The sound that moves.** In the corridor, footsteps behind the wall. Not the muffled piano (stationary). FOOTSTEPS — matching your pace. Walk, they walk. Stop, they stop. Run, they run. Start again, beat of silence, then they start. Never acknowledged. Another person in a parallel corridor you can never reach. The player discovers this by experimenting with movement.

d) **Scale reward.** The observation window in the space lobby. If the player crouches (Ctrl) at the glass, the FOV widens and the camera lowers — you're small, looking up at Earth. The game doesn't tell you to do this. But the perspective shift — from standing human to crouching child — changes the emotional register completely. You found a moment.

e) **The gravity tells.** In low-g space scenes, loose objects should drift slightly. The champagne in the glass. Gibbons' briefcase. Dust motes (already there). But also: if the player jumps, objects near them bob upward slightly in their wake. Air displacement in micro-gravity. The physics are alive. The player experiments. Each experiment teaches them something about the space.

### 3. Emotional Fun (partially built, needs restraint)

The wrongness progression is the best emotional mechanic: after each task, something shifts. Photograph moves. Bathroom door cracks. Earth glow shifts. The room is gaslighting you.

**What works:** Changes you're not sure happened. Was the photograph always there? Did that door move? The uncertainty IS the emotion.

**What doesn't work:** Text that CONFIRMS the emotion. "She would have loved this." tells the player what to feel. The two chairs on the balcony facing the void tell the player what to feel WITHOUT text. The chairs are better.

**Proposals:**

a) **More wrongness, less confirmation.** After task 2, the second champagne glass has moved — not gone, just 3 inches to the left. After task 3, her book is open to a different page. After task 4, the second pillow has an indent. None of this is announced. The player either notices or doesn't. On second playthrough they notice everything.

b) **The clock as emotional meter.** Already built — clock decelerates as tasks complete. But the player should be able to HEAR the deceleration. Make the tick louder. Make the space between ticks stretch. Time is literally slowing. The room is holding you.

c) **Temperature as feeling.** The thermostat interaction is a compromise temperature — 22°, too warm for you, too cool for her. When the player changes it, the post-processing shifts (warmth). But what if changing it also changes the AUDIO? Warmer = more reverb, softer edges. Cooler = crisper, harder. The room sounds different at her temperature vs. yours. The player discovers this by experimenting.

d) **The phone that stops.** It rings at 30s. Currently unanswered. What if: the player can walk toward it. As they get within 2 meters, the ringing slows. Within 1 meter, it stops. Not because they answered — because their proximity killed it. The interaction is the failure to interact. Stand next to a silent phone in an empty hotel room. The game doesn't explain why it stopped.

---

## II. The Revised Task Model

### Current: Press E Four Times
Lamp → Champagne → Desk → Bed. Each is "walk to, press E, watch light change, task complete."

### Proposed: Inhabit the Room
The suite shouldn't have "tasks." It should have SPACES that respond to presence.

**The bed zone.** Walk near the bed. Movement slows. Stay for 5 seconds. The covers pull back (geometry change — no button press). Stay for 10 seconds. The camera begins to lower. The player is choosing to lie down by being still. The bed ritual triggers not from pressing E but from STAYING. Surrender through inaction.

**The window zone.** Stand at the window for 15 seconds. Earth rotates enough to see New Zealand. The player doesn't know this will happen. They're just looking. The game rewards looking by showing them home.

**The champagne zone.** Walk near the tray. The cork pops automatically (you're here, the champagne was waiting). One glass fills. The second stays empty. No button. The room was going to do this with or without you.

**The desk zone.** Sit at the desk (crouch near it). The desk lamp turns on. Her margin notes become visible in the light. You're reading by lamplight. The interaction is slowing down enough to see detail.

**The bath.** Enter the bathroom, close to the tub. Water starts running. Steam appears. Earth blurs through the fogged glass. You don't turn on the bath — the room was expecting you.

**The logic:** The suite was prepared for two. Everything was going to happen — the champagne, the bath, the bed turned down. You arriving alone doesn't stop the room's program. The hotel doesn't know the trip changed. The interactions feel automatic because THEY WERE. For two people. One showed up.

### Why This Is Better

1. **No "press E" breaks flow.** The player moves through the room naturally. Things happen AROUND them.
2. **Movement IS the game verb.** Where you stand, how long you stay, how fast you move — this is the input.
3. **The room has agency.** The hotel prepared this. You're witnessing a romantic evening that was set up for two people, playing out with one.
4. **Discovery is spatial.** You find things by being in places, not by pressing buttons.
5. **Second playthrough changes.** Knowing the zones, the player can rush through (the room still triggers — you can't stop it) or linger (each zone has more to show if you stay longer).

---

## III. What to Cut

### Text to Remove
These objects should be SILENT. Pressing E opens/reveals them. No text.

| Object | Current Text | Why Remove |
|--------|-------------|------------|
| Sock | "Not yours." | You can see it's not yours. It's a women's sock. |
| Wine glass | "Not your shade." | You can see the lipstick mark. |
| Book | "Her bookmark. A boarding pass." | You can see the boarding pass sticking out. Make it blue against the red cover. |
| Bath | "Big enough for two." | You can see the bath is big. |
| Wardrobe | "Two robes. Different sizes." | You can see two robes. |
| Phone | "47 songs. Her queue. Still counting down." | Too specific. A phone with a playlist open. Let the player read the screen (tiny text, unreadable at distance — the Bolaño postcard principle). |
| Suitcase | "Packed for two weeks. Never opened." | You can see the tags are still on. |
| Balcony | "Two chairs." | You can see two chairs. |
| Balcony | "She would have loved this." | NEVER. The player thinks this. Don't say it for them. |
| Exterior | "The rain doesn't know it's 2 AM." | Poetic but breaking the architecture-first rule. Cut. |
| Space lobby | "Oh." | The FOV change and speed manipulation already say this. |

### Text to Keep
These involve READING — the player reads a physical object. Text is diegetic.

| Object | Text | Why Keep |
|--------|------|----------|
| Ticket | "Booking confirmation. 2 guests." | You're reading a ticket. |
| Newspaper | "ORBITAL SUITE OPENS — THREE HOURS TO KILL" | You're reading a headline. |
| Photograph | "Paris. Before all this." | You're reading the back of a photo. Or remembering. |
| Room service card | "Two handwritings. A disagreement about the cheese plate." | You're reading a card. |
| Postcard | "Addressed to no one." | You're reading an address line. |
| Guest book | "Two names on the booking. One crossed out." | You're reading a book. |

### Gibbons Text to Audit
Gibbons speaks. That's fine — he's a character. But some lines tell the player what to feel:

**Keep:** "Ah. We have your reservation." / "The gentleman in Six orders for two." / "The champagne was in the booking." / "Time." / "Good hotel?" — These are CHARACTER. He's speaking as himself.

**Cut or rethink:** "The corridor's longer than it looks." — This is the game explaining itself. Let the corridor BE longer. / "The window. Most people need a moment." — Let the window hit. Don't pre-explain the feeling.

---

## IV. The Movement Vocabulary

What the player can DO, physically, and what it means emotionally:

| Action | Input | Emotional Register |
|--------|-------|-------------------|
| Walk slowly | Analog/WASD | Default exploration. The room at human pace. |
| Sprint | Shift | Nervous energy. The lobby and corridor reward this — spaces open up at speed. |
| Slide | Ctrl + Sprint | The corridor has a long straight. Sliding through it = arriving with momentum. |
| Crouch | Ctrl (still) | Smallness. At the window = looking up at Earth like a child. At the bed = eye level with the pillow indent. |
| Jump | Space | Low gravity makes this meaningful. The balcony jump = standing on the railing. |
| Bunny hop | Sprint + jump rhythm | Nervous fidgeting. Gibbons waits patiently. |
| Stand still | Nothing | The hardest action. The bed zone. The window zone. The game rewards stillness with revelation. |
| Look up | Mouse | The Sky Tower from below. The suite ceiling. The stars from the balcony. Awe is vertical. |
| Look down | Mouse | The pillow indent. The sock under the bed. The second ticket. Intimacy is downward. |

---

## V. Second Playthrough Design

The game loops. On second play, the player knows everything. The design question is: what changes?

### What Already Works
- Gibbons has different dialogue (built)
- Prologue skipped (built)
- Taxi is silent (built)
- lobby_visit_count tracks returns (built)

### What Should Change Spatially
1. **The corridor is shorter.** Literally fewer meters. Gibbons said "shorter this time" — make it true. Remove one door. The hotel is emptying.
2. **Room Six's tray is gone.** "Six checked out. The trays stopped coming." — remove the geometry. The absence of the tray is the interaction now.
3. **The muffled piano is silent.** "Four stopped playing." — cut the audio source. The corridor is quieter. The player notices what's missing.
4. **The suite has one pillow.** On second play, the hotel already cleaned. One pillow. One robe on the hook. The champagne has one glass. The player arrives into the CLEANED version. The "twos becoming ones" that was the montage ending is now the starting state. You're replaying the aftermath.
5. **The photograph is face-up.** You turned it over last time. The hotel remembers. She's laughing. You see it immediately.

### What This Does
First play: you discover twos, then watch them become ones.
Second play: you arrive into ones, and REMEMBER the twos.

The emotional direction reverses. First play is loss. Second play is grief — which is the memory of what you lost.

---

## VI. The Fun Summary

**Movement is the primary input.** Where you stand, how fast you move, how long you stay.

**Architecture is the primary output.** Geometry changes, light shifts, spatial revelation.

**Text is rare and diegetic.** Only when the player is READING something.

**Discovery rewards curiosity.** Silent objects, hidden windows, parallel footsteps, physics experiments.

**The room has its own agenda.** The hotel prepared a romantic evening for two. It plays out whether you participate or not. You're a guest at your own absence.

**Second playthrough is a different game.** The hotel remembered. The corridor emptied. The room was cleaned. You're not discovering loss — you're revisiting it.

**The fun is in understanding.** Not in pressing E.
