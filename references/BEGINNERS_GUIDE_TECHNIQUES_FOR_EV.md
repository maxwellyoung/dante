# The Beginner's Guide Techniques → Endearing Void Application Guide

A synthesis document. What Wreden proved that EV can use.

---

## The Five Big Lessons

### 1. Narration Colonizes Space — Use That

Emily Short's sharpest insight: a voiceover that tells the player what a room means *before they experience it* strips agency and becomes control. The Beginner's Guide's narrator doesn't explain — he appropriates. Every "let me tell you about this room" is an act of possession.

**For EV:** If the game ever uses voice (phone call, memory, radio), the voice should complicate what the space says, not confirm it. A narrator who says "this room is empty" while the player can see a half-packed suitcase. Godard does this constantly — voice and image contradict. The gap between what you're told and what you see is where meaning lives.

### 2. Architecture Communicates Psychological States

The Beginner's Guide maps emotion onto space more literally than Blendo does. Open spaces = creative freedom. Narrowing corridors = anxiety. Prison cells = entrapment. The Tower = isolation. The progression from social (Counter-Strike map) to solitary (the Tower) is spatial contraction as emotional arc.

**For EV:** The Hotel Chevalier already does this — the taxi is transient, the lobby is public, the hallway narrows, the room is intimate, the balcony opens to infinity, the stars dissolve spatial logic entirely. This is the same emotional trajectory as The Beginner's Guide but inverted: contraction → expansion instead of expansion → contraction. EV moves toward openness. Lean into that.

### 3. Player Agency Is a Dial, Not a Switch

The game's most powerful moments coincide with its most restrictive:
- **Stairs chapter:** Movement speed reduces as you approach the door. The game physically restrains you.
- **Furniture selection:** Your choices are ignored. The game mocks the illusion of agency.
- **The silence:** After 80 minutes of constant narration, the voice stops. Sudden freedom is devastating.

**For EV:** Movement speed changes per scene already exist. Push further. The hallway should feel slower than the lobby. The balcony should feel like release — wider FOV, faster movement, wind audio. The bed should feel like surrender — controls go soft, camera drifts. Each space has its own physics of agency.

### 4. Unfinished Aesthetic Is an Entire Register

Coda's games are prototypes — raw geometry, placeholder textures, dev materials. This isn't failure, it's privacy. The rawness communicates "this wasn't meant to be seen." Robert Yang's analysis reveals the spaces are actually expertly constructed — the amateur appearance is authored, not accidental.

**For EV:** Raylib's rendering constraints give you this for free. But the lesson is deeper: you can shift between registers. Some spaces in the hotel can feel polished (the lobby, the room). Others can feel raw (a service corridor, the roof, a dream sequence). The quality shift IS narrative — it tells the player they've crossed a boundary into somewhere private or unfinished.

### 5. Recurring Motifs Create Meaning Through Repetition, Then Subvert It

The lampposts appear at the end of chapters 7-15. Davey frames them as Coda's signature — evidence of growth. The Tower reveals Davey planted them. A single object becomes loaded with betrayal retroactively.

**For EV:** The hotel can have its own lampposts — objects that appear in every scene, seemingly decorative, that accumulate meaning. A specific color. A specific sound. A pattern on the wallpaper that appears where it shouldn't. The subversion doesn't have to be betrayal — it can be revelation. The object that seemed incidental turns out to be the point.

---

## Specific Techniques to Steal

### Trigger-Volume Narration
Voice lines activated by the player's spatial position. Bounding box checks against player location, triggering audio playback. The narration plays in real-time as you move — some lines are gated (can't progress until they finish), others are not (you can walk ahead). This creates a dynamic relationship between movement and story.

**Implementation:** Trivially implementable in Raylib. Define trigger zones per scene. Associate audio clips. Fire once when the player enters the zone. Some zones block progression, others don't. The ratio of gated-to-free zones controls pacing tension.

### Speed Manipulation as Emotional Tool
The Stairs chapter reduces movement speed as you approach the door. The player physically experiences dread, anticipation, or reluctance through their own locomotion.

**Implementation:** Already partially in EV. Each scene can define a movement speed curve — not just a flat value, but a function of position. Near the balcony door: slow down. On the balcony itself: speed up. In bed: barely move. The player's body teaches them the emotional register of the space.

### Hostile Spaces (The Invisible Maze)
The Tower's invisible maze punishes wall-touching with screen flash and harsh noise. Not death — discomfort. The space is deliberately miserable because it was designed to reject a specific person (Davey). Hostility as authored intent.

**Implementation:** EV doesn't need hostility in the same way, but the principle — that a space can actively resist the player's presence — is powerful. A hallway where the lights flicker when you linger too long. A room that gets quieter the more you explore it. Spaces that communicate "you shouldn't be here" without killing you.

### The Silence After Sound
After 80 minutes of constant narration, Davey stops talking. The silence is the game's emotional climax. You can only appreciate absence if you've been immersed in presence.

**Implementation:** What is EV's "constant"? If it's ambient sound (hotel hum, Paris outside, radiator clicks), then the stars sequence should be silence. If it's spatial logic (rooms, corridors, doors), then the stars sequence should dissolve geometry. The thing you remove at the climax should be the thing the player took for granted.

### Impossible Architecture (Engine-Coherent)
Keogh's insight: spaces need only cohere within engine logic, not physical logic. Floating corridors, rooms larger than the building, stairs that loop — all valid if they feel intentional within the game's world.

**Implementation:** The stars sequence already breaks spatial logic. But earlier spaces can hint at impossibility — a window that shows a different time of day than the one you entered. A hallway that's longer from one direction than the other. Subtle spatial wrongness that the player feels before they identify.

### Inaccessible Spaces as Design
Concealed or unreachable areas — spaces you can see through a window or hear through a wall but never enter. These communicate as powerfully as accessible spaces because the player fills in the rest.

**Implementation:** The hotel should have rooms the player hears but never enters. Muffled music through a wall. A light under a door that goes out as you approach. The occupied rooms that surround the player's empty one — other lives happening in parallel, inaccessible.

### The Narrator-in-a-Box (Technical)
The Cutting Room Floor reveals: the narrator entity lives in an out-of-bounds black box and is implemented as a `generic_actor` that bleeds when shot. Simple positioned audio source, hidden from the player.

**Implementation:** For EV, audio sources can be placed in the level geometry — a phone on the nightstand, a radio in the lobby, a voice from behind a door. The source has a position in 3D space. Raylib's audio system supports this. The narrator doesn't need to be omniscient — it can be located.

---

## Where The Beginner's Guide Diverges from Blendo

| Principle | Blendo (Gravity Bone / TFoL) | The Beginner's Guide | EV Should... |
|-----------|------------------------------|---------------------|--------------|
| Narration | Zero — trust the space | Constant — then remove it | Use sparingly. Located, not omniscient. Contradicts the space. |
| Pacing | Cinematic — jump cuts, montage | Literary — slow build, gradual reveal | Cinematic. EV is a film, not a novel. But steal the slow build for the room sequence. |
| Player trust | Total — figure it out | Betrayed — you were lied to | Earned. The hotel is legible. The stars are not. Trust → wonder. |
| Architecture | Mid-century modern, Saul Bass | Source dev aesthetic, abstract | Godard-specific. Hotel Chevalier reference. Not generic, not abstract — authored. |
| Emotional payload | The rug pull (sudden death) | The slow betrayal (revelation) | The dissolve (intimacy → infinity). Neither sudden nor slow — gradual, like falling asleep. |
| Agency | Some mechanics (items, platforming) | Walking only | Minimal mechanics (light candle, pour drink, open curtain). Tasks as ritual. |
| Runtime | 13-20 minutes | 90 minutes | 15-30 minutes. Closer to Blendo than Wreden. |

---

## The Deeper Synthesis: What All Three Games Share

Gravity Bone, Thirty Flights of Loving, and The Beginner's Guide all prove the same thing:

**You don't need much.**

No inventory systems. No skill trees. No crafting. No dialogue wheels. No save points. No fail states (Gravity Bone has one death, and it's the point). No HUD. No tutorials.

What you need:
- Spaces that mean something
- A perspective that commits to its constraints
- Trust that the player will fill the void
- The courage to end before you've explained everything

EV already has all four. The research confirms: the approach is sound. Now execute.

---

## Key References to Revisit

When stuck on EV design decisions about narration, pacing, or spatial storytelling:

1. **Emily Short** — Narration as colonization: [emshort.blog](https://emshort.blog/2015/10/05/the-beginners-guide-davey-wreden-and-intimacy-inside-games/)
2. **Brendan Keogh** — Walking as complicity: [brkeogh.com](https://brkeogh.com/2015/10/03/on-the-beginners-gude/)
3. **The Gemsbok** — Progression as ambiguity escalation: [thegemsbok.com](https://thegemsbok.com/art-reviews-and-articles/beginners-guide-davey-wreden-analysis-critique/)
4. **Robert Yang** — Material-narrative dissonance (via New Yorker): expert construction disguised as amateur work
5. **Davey Wreden** — "What fire dies when you feed it?": [medium.com](https://medium.com/@HelloCakebread/game-of-the-year-cb4214f98c13)
6. **Super Jump** — Interpretation as violence, the lamppost as lie: [superjumpmagazine.com](https://www.superjumpmagazine.com/the-beginners-guide-video-games-and-the-death-of-the-author/)
7. **TCRF** — Cut content, narrator-in-a-box, "Damn..." easter egg: [tcrf.net](https://tcrf.net/The_Beginner%27s_Guide)
