# Endearing Void — Master Plan

> "You don't need much. Spaces that mean something. A perspective that commits to its constraints. Trust that the player will fill the void. The courage to end before you've explained everything."

This document is the long-term development plan for Endearing Void. Every design decision should answer to this plan. When in doubt, return here.

Last major revision: 2026-03-18

---

## I. What Endearing Void Is

A first-person narrative game about three hours to kill in an impossible hotel. 15-30 minutes long. Built in C99 with Raylib. 480x300 lo-fi. No hand-holding, no fail states, no dialogue trees. Architecture is the story. The void is the point.

**The emotional core:** You came here with someone. They're not here. The suite was booked for two. Everything in the room — the bed, the champagne, the bathrobe on the door — remembers them. You're doing the trip alone. The hotel is the last place the relationship existed. Three hours to kill before the life after starts.

**The emotional trajectory:** Arrival → Intimacy → Fracture → Escape. Taxi → Hotel → Paris Dream → Rapid Cuts. The world contracts (taxi → elevator → suite) then cracks (the dream, the memory) then explodes (Thirty Flights-style rapid cuts, heightened, dramatic, characters reappearing with context).

**The secret the game keeps:** The character was always leaving, not arriving. The trip was planned for two. You came anyway. You never learn exactly what happened between you — a breakup, a death, a quiet dissolution, an argument at the airport. The hotel is the pause between the relationship and whatever comes after. By the end the player knows without being told.

**What the author knows (the player doesn't):** She left three weeks ago. Not dramatically — she just stopped being there. The trip was her idea. The booking is in her name. You came because cancelling felt like admitting it was over. The photograph face-down on the nightstand is the two of you at a café in Paris. That trip — the last good one — is the Paris dream. Gibbons has seen this before. He was expecting two. He says nothing.

**The spiritual lineage:**
- **Hotel Chevalier** — Two people in a hotel room with unresolved history. The room IS the relationship. The yellow bathrobe. The bruises. Where they stand tells you everything.
- **Firewatch** — Henry is running from his wife's illness into a beautiful place. The tower is the hotel — a gorgeous space to avoid your life. The conspiracy is irrelevant; the real story is whether he goes home.
- **Gravity Bone / Thirty Flights of Loving** — Architecture IS narrative. Cut everything non-essential. Rapid-cut endings where passing characters suddenly have context.
- **The Beginner's Guide** — Agency is a dial. Unfinished spaces are intentional. Silence after sound is devastating.
- **Godard (Contempt)** — A marriage disintegrating in a beautiful house. Every color choice serves an actual emotional state between two people. Withhold the explanation, give the feeling.
- **Barton Fink** — The hotel as psychological space. Character under pressure from something we don't fully understand. The world slightly wrong in ways that accumulate.
- **Bolaño (2666)** — Amalfitano hangs a geometry textbook on the washing line and leaves it there. It gets rained on. Nobody comments. A person doing something slightly unhinged because they're falling apart in a way they can't articulate. Dignity-in-transit. Characters too aware, too polite, slightly broken.
- **Kaufman** — Unreliable interiority. The character's experience may not be what's happening. Memory and present tense bleed.
- **Bioshock Infinite** — Arriving into something impossible. The scale does the work.

---

## II. Design Commandments

These are non-negotiable.

### 1. Architecture Is the Primary Storytelling Medium
No cutscenes. No text dumps. No exposition. Every room tells its story through spatial composition, material, light, and the objects placed within it. A room booked for two with one person in it tells you everything. If a narrative beat can't be communicated through architecture, it doesn't belong in the game. (Chung, Wreden, Keogh)

### 2. Every Interaction Has Visible Physical Consequence
Light a candle → see flames. Pour champagne → hear the glass fill, see the second glass stay empty. Open curtains → the room transforms. Run a bath → steam on the window obscures Earth. If the player can't see the change in a screenshot, the interaction doesn't count. (Chung, Vlambeer)

### 3. Player Agency Is a Dial, Not a Switch
Movement speed, FOV, camera freedom, and control responsiveness should all vary by scene and emotional beat. The hallway is slower than the lobby. The balcony is wider than the room. The bed surrenders control. Restriction at emotional peaks, release at climaxes. (Wreden, Beginner's Guide)

### 4. The Constant, Then Its Absence
The other person. Every scene carries evidence of them — the second ticket, the second glass, the second robe, the second pillow. The player takes it for granted. In the ending sequence, it's gone. The room is reset. One pillow. One glass. One robe. The hotel has already cleaned you out. (Wreden, Emily Short)

### 5. Unfinished Is an Aesthetic Register, Not a Bug
Raylib's rendering constraints are features. Raw geometry, procedural textures, visible seams — these communicate authenticity and intimacy. Some spaces can shift between polished (lobby, suite) and raw (service corridor, dream sequence, the void). The quality shift IS narrative. (Wreden, Yang, Beginner's Guide)

### 6. Inaccessible Spaces Communicate
Rooms the player hears but never enters. Music through a wall. A light under a door that goes out. Other lives happening in parallel — couples, probably. The hotel is populated by the relationships you can hear but not reach. (Short, Keogh, Gemsbok)

### 7. Progression Escalates in Ambiguity, Not Difficulty
Early spaces are legible — room numbers, signage, clear paths. Later spaces erode certainty — the corridor that shouldn't connect, the window showing the wrong time, the room that's larger inside than outside. The suite starts concrete and slowly becomes psychological. The game teaches the player its rules, then quietly breaks them. (Gemsbok, Beginner's Guide)

### 8. Brevity Is Respect
15-30 minutes. Every second earned. No padding, no backtracking, no filler. The game ends before it's explained everything. Trust the player to carry the meaning forward after the screen goes dark. (Chung, Wreden)

### 9. Twos — The Recurring Motif
The game is full of twos and the player is one. Two tickets. Two glasses. Two robes. Two pillows. Two chairs angled toward each other. The motif is pairs — seemingly decorative in the first scene, accumulating weight, devastating by the suite. Second playthrough the player sees it everywhere. The motif is the relationship. (Beginner's Guide lamppost principle)

### 10. Wonder, Not Horror. Always.
Bold color, not desaturation. Warmth, not cold. Open sky, not closed ceiling. The void is beautiful and melancholy, never threatening. The relationship ended, but it wasn't a tragedy — it was a real thing between two people that stopped being. Grief without melodrama. (Maxwell's taste, consistently enforced)

---

## III. The Game Flow — Definitive Structure

### Act 1: Arrival (The Pull Forward)
Each space surprises. Gravity Bone energy — the sequence of spaces pulling you forward. But underneath the wonder, the absence. Everything is scaled for two.

```
TITLE
  ↓ (Enter)
TAXI — Auckland, dawn. Inside the car. City scrolls past.
       Sky Tower grows in the windshield. ~15 seconds.

       THE DETAIL: A second ticket on the seat beside you.
       Or a booking confirmation visible — "2 guests."
       The player's first interaction: picking it up,
       putting it in their pocket. Pocketing evidence.

       Emotional register: anticipation, motion, urban rhythm,
       the particular loneliness of going somewhere familiar
       without the person who made it familiar.
       Audio: city ambient, engine hum, tires on wet road.
       The last grounded moment.

  ↓ (auto-transition)
HYPERSPACE — The tower becomes a tunnel. Reality breaks.
             Disorienting roll, extreme chromatic aberration.
             Hard cut to silence.
             The threshold isn't just spatial — you're leaving
             the world where the thing happened.
             Emotional register: vertigo, threshold crossing, relief.

  ↓ (hard cut)
SPACE_LOBBY — You're in orbit. Earth through the observation window.
              Gibbons appears. He was expecting you.

              THE DETAIL: He glances behind you. Just once.
              Then catches himself. Perfect composure from then on.
              He picks up your bag — one bag, conspicuously light
              for a suite booked for two. He doesn't mention it.
              Bolaño politeness. The kindness of not acknowledging.

              Emotional register: wonder, impossibility, warmth,
              the slight embarrassment of being seen alone.
              Audio: hull resonance, air circulation, insulated quiet.

  ↓ (Gibbons leads you)
ELEVATOR — Warm amber, wood wainscoting, leather handrail.
           Hotel Chevalier energy — two people in a small space
           with unspoken knowledge. Except one of them is staff
           and the knowledge is professional. Gibbons has seen
           this before. Solo arrivals on double bookings.
           He adjusts his tie. The player has nothing to adjust.
           Emotional register: compression, intimacy, anticipation.

  ↓ (doors open)
HOTEL — The Gehry moment. Massive open-plan glass environment.
        Bioshock Infinite energy — you arrive into something
        impossible and the scale does the work. Curved glass walls.
        Earth visible below. You're tiny in a vast beautiful room.
        The void outside is vaster still.

        THE DETAIL: You pass a pair of chairs angled toward
        each other in front of the Earth window. Designed for
        two people to sit and watch the planet together.
        Other corridors, other wings — ambient sounds of
        couples, laughter muffled through walls. You're alone
        in the most beautiful room you've ever seen.

        NOT claustrophobic — agoraphobic. The scale of
        the space matches the scale of what you're feeling.
        Emotional register: awe, vertigo, beauty, loneliness.

  ↓ (Gibbons leads you through to your suite)
```

### Act 2: Intimacy (The Suite)
The world becomes one room. The hotel IS the game. Three hours to kill.

```
SPACE_SUITE — Your room. The Barton Fink room. The Hotel Chevalier room.

              The door opens. The room is gorgeous.
              Warm, glass-fronted, generous. King-size bed.
              And immediately, unmistakably, a room for two.

              TWO PILLOWS on the bed.
              TWO ROBES on the bathroom door — different sizes.
              TWO CHAMPAGNE GLASSES on the tray.
              A BOOK on the nightstand that isn't yours —
              her book. A novel she was reading. Spine cracked
              at page 120-something. She was halfway through.
              A PHOTOGRAPH face-down. The two of you in Paris.
              A room service card in YOUR handwriting from weeks ago,
              when you planned this together. The hotel prepared
              everything you asked for. Both of you asked for.

              THE RITUALS:

              BATH — Run the water. Steam on the glass.
              Earth rotating through the fog. The bath is
              big enough for two. You sit on the edge.
              Maybe you get in. The Chevalier moment:
              doing an intimate thing in a space designed
              for intimacy, alone. The warmth of the water
              is the warmth of the relationship — present,
              surrounding, slowly cooling.

              CHAMPAGNE — Pop the cork (procedural sound).
              Two glasses on the tray. You pour one.
              The second glass stays empty.
              THIS IS THE WASHING-LINE MOMENT.
              The empty glass just sits there for the rest
              of the game. Nobody comments. It's the most
              violent image in the game and it's completely still.

              BALCONY — Step outside. The void. Earth below.
              The Chevalier balcony — Portman in the yellow robe
              looking at Paris, except here it's the whole planet
              and you're looking at the place where your life is.
              Down there, somewhere, is the apartment with her
              things still in it, or not. The rail. The cigarette
              if you want it. The scale of what you're seeing
              matched by the scale of what you're feeling.

              BED — Pull back the covers. Lie down. The ceiling.
              The game takes control — agency surrendered.
              The player can't get up for a few seconds.
              The room goes quiet. The indent on the other
              pillow — did housekeeping not smooth it, or
              is that your memory? The composed music triggers
              here. One piece. Once. Never repeats.
              This is the emotional center of the game.
              You're lying in a bed booked for two and the music
              plays and the Earth rotates and the game holds you.

              ACCUMULATION — THE SUITE REVEALS ITSELF:
              Not that the hotel is sinister. The room keeps
              reminding you. The wrongness isn't the hotel.
              It's your situation.
              - The photograph: the two of you, a Paris café.
                Face down when you arrived. You can turn it over.
                Or not.
              - The adjoining door: leads to a second room.
                Also booked. Also empty. Also prepared for arrival.
                Her room? A room you'd share? The hotel doesn't
                know the trip changed.
              - The book: her book. You can pick it up. The bookmark
                is a boarding pass. Her name on it.
              - The thermostat: set to 22°. A compromise temperature.
                Slightly too warm for you, slightly too cool for her.
                You can change it. The room adjusts. Nothing stops you.
              - Shoes by the door: arranged for two. Yours and a pair
                of hotel slippers, still wrapped. Nobody coming to
                unwrap them.

              These are not puzzles. They are facts about a room.
              The player decides how much to look at.

              BOLAÑO DETAILS (the abstract, the unhinged-mundane):
              - A geometry textbook open on the desk, spine cracked,
                as if someone was studying orbital mechanics for
                a conversation they never had. (Direct 2666 reference.)
              - The room service menu has handwritten notes in the
                margin — two people comparing what they'd order.
                Some items circled twice. A small disagreement
                about the cheese plate, resolved with an arrow.
              - A postcard, half-written, addressed to no one.
                The handwriting changes mood halfway through —
                starts formal, ends in a scrawl that might be
                a single word repeated. Unreadable at 480x300.
                That's the point.
              - A sock. One sock. Under the bed. Not yours.
                The most mundane object in the game.
                The most real.

              FIREWATCH PARALLEL: Henry goes to the tower to avoid
              his wife's illness. The player goes to the hotel to —
              what? Say goodbye to the version of life where this
              trip was happening as planned? Prove they can still
              do it alone? Punish themselves with beauty? The game
              doesn't decide. The architecture communicates all
              of these simultaneously.

              Mirror's Edge physicality still present — sprint,
              FOV zoom, momentum. Even in slow exploration,
              movement that feels good changes everything.

              One composed music piece plays here. Once. Never repeats.
              This is the emotional center of the game.
```

### The Paris Dream (Hard Cut, Mid-Suite)
This is the crack. This is a memory.

```
PARIS DREAM — Hard cut. No transition. No warning. No fade.
              You were in the suite. Now you're somewhere else.

              B&W. Godard grain. High contrast. A Parisian hotel
              room — small, warm, real. Not orbital. Ground floor.
              Rain on the window. Traffic outside.

              THE DIFFERENCE: the other person WAS here.
              Evidence everywhere.
              - Her coat on the chair.
              - Coffee for two on the table — one cup half-drunk.
              - The indent on the other pillow, fresh.
              - The bathroom door is ajar. Steam. She just showered.

              But you're alone in the memory too. You walk through
              the rooms — hallway, bedroom, bathroom — looking for
              her in a memory where she's already left. Just missed.
              Always in the next room. The compressed geometry is
              how you remember it: smaller than it was, doors in
              wrong places, the window showing a street that might
              not be the right street.

              THE WASHING-LINE MOMENT: Her toothbrush is still in
              the bathroom glass. A detail so mundane and specific
              it can't be symbolic. It's just there. On second
              playthrough it's unbearable.

              A BOOK — the same novel from the suite nightstand,
              but here it's on the bed, open, further along.
              She was reading it here first. This is where the
              bookmark boarding pass came from.

              ABSTRACT DETAIL: The hallway is slightly longer than
              it should be. The bedroom window shows the Eiffel
              Tower, then on second look, the Sky Tower. Then
              neither. Memory edits itself.

              ABSTRACT DETAIL: A red object — a scarf, a book
              cover, a flower in a glass — the only color in
              the B&W space. Godard's red. It doesn't mean
              anything except that memory preserves color
              selectively. You remember what mattered.

              Duration: 60-90 seconds. No longer.
              Hard cut back to the suite.
              Same spot. Same position. The champagne glass
              is still empty. The game continues as if nothing
              happened. The player knows it did.

              DO NOT explain it.
              DO NOT have any character acknowledge it.
              It just happened. Like a memory does.
```

### Act 3: The Ending (Rapid Cuts)
Thirty Flights of Loving. Heightened. Dramatic. Now the cuts have content.

```
The ending is NOT a slow fade.
The ending is a MONTAGE.

Triggered after the bed ritual, or after the player has
inhabited the suite long enough. The game decides.

Rapid cuts — jump cuts between spaces. Each 2-5 seconds.
Some cuts are single frames. Time collapses.
The "passing characters now have context" feeling.

THE MONTAGE (draft sequence):

1. The taxi — the second ticket on the seat. Hold. Cut.
2. The lobby — Gibbons glancing behind you. Cut.
3. The suite — two robes on the door. Cut.
4. Paris — her coat on the chair. Cut.
5. The balcony — Earth, alone. Hold. Cut.
6. The elevator — going down this time. Cut.
7. The suite — but the room is different. Cleaned.
   One pillow. One robe. One glass. The hotel has
   already erased her. The room has moved on
   even if you haven't. Cut.
8. Paris — the toothbrush in the glass. Single frame. Cut.
9. The hotel — the two chairs by the window. Empty. Cut.
10. Gibbons — in the taxi. Going home. Already leaving.
    Brief eye contact. He adjusts his tie.
    He's been kind about it the entire time. Cut.
11. The photograph — face up now. The two of you.
    Paris. A café. She's laughing. Hold.
12. The bed — the indent on the other pillow.
    Hold. Longer than comfortable.
13. The void. The empty glass. Silence.

The music from the suite — the one piece that played
once — returns as a fragment. Three notes. Then silence.

Credits.
The game is over. The feeling is not.
```

---

## IV. Characters

### Gibbons (The Bellhop)
Cube-person. Formal, geometric, not-quite-human. Bolaño character — too aware, too polite, slightly broken. Has been waiting in hotel lobbies so long he's become furniture. Adjusts his tie. Carries a briefcase he never opens.

**What Gibbons knows:** The suite was booked for two. He's seen this before — solo arrivals on double bookings. People who came anyway. He doesn't judge. He's kind about it because kindness is the only dignified response. He was a version of this person once, maybe. Or he just understands service as a form of care.

**Arc:** Gibbons was expecting two. He sees one. He glances behind you, once, then never again. He carries your one bag with the same ceremony he'd carry two. He leads you to the suite. He bows. He leaves. In the ending montage he reappears — in the taxi, going home. He's already leaving too. Brief eye contact. He knew the whole time. Not what happened to her. Just that you came alone and the room was ready for two and that was enough to know.

**Design:** Waypoint pathing. Walk cycle with character — purposeful, slightly formal. Pauses at thresholds. Gestures you through. His presence is warm. His absence is felt.

### The Player Character
No name. No dialogue. Came here with someone who isn't here. The trip was planned together — the booking, the room service card, the champagne. You came anyway. Why? The game never says. The player decides: grief, stubbornness, a need to prove something, an inability to cancel. Maybe all of them. The hotel is the pause between the relationship and whatever comes after. Kaufman interiority — we see through their eyes but can't trust what we're seeing. The Paris dream might be memory or invention or wish. By the end the player knows without being told.

### Her (The Absent)
Never seen. Never named. Never described physically. Present only through objects: her book, her robe, her toothbrush, her coat, her handwriting on the room service card, the indent on the pillow, the photograph where she's laughing. The player assembles her from traces. Every player assembles a different person. She exists in the negative space. The game's most important character is the one who isn't there.

---

## V. Key Scenes — Design Notes

### The Taxi
The grounded scene. Auckland at dawn. Wet streets. The Sky Tower in the distance growing larger.

- **Purpose:** Establish the real world before breaking it. Plant the first "two" — the second ticket.
- **Player action:** Pick up the booking confirmation / second ticket. Pocket it. First interaction is hiding evidence.
- **Audio:** City ambient, engine hum, tires on wet road. A song on the radio — something warm, something you'd listen to together. It cuts off when hyperspace begins.
- **Light:** Dawn. Golden hour through wet glass. The last warm light that comes from the sun.

### The Hotel (Gehry Space)
The arrival moment. The most architecturally ambitious space.

- **Scale:** Massive. Open plan. Curved glass walls. The opposite of claustrophobia.
- **Reference:** Gehry interiors — organic curves, unexpected materials. Bilbao atrium energy.
- **Light:** Earth-glow through glass. Warm interior pools. Godard red accents at scale.
- **The relationship layer:** Pairs everywhere. Two chairs. Tables set for two. Corridor sounds of other guests — couples, laughter, muffled conversation. You pass through other people's intimacy on the way to your empty suite.
- **Purpose:** Sells the premise AND the loneliness. You arrive into something impossible and beautiful and shared by everyone except you.
- **Transition:** Gibbons leads you through. You see the hotel is full of people living the trip you were supposed to have.

### The Space Suite (The Room for Two)
60% of playtime. Where the game lives or dies.

- **Not small.** Generous, glass-fronted, warm. But one room. The compression after the Gehry expanse.
- **Designed for two.** King bed, two pillows, two robes, two glasses, her book, the photograph. The room is a portrait of the relationship. Architecture as memory.
- **Rituals over objectives.** Bath, champagne, balcony, bed. Each is an act of inhabiting a space designed for intimacy, alone. Multi-step with visible consequence.
- **The Bolaño objects.** The geometry textbook. The room service card with two handwritings. The half-written postcard. The sock under the bed. Details so specific and mundane they resist interpretation. They're just there. Like things people leave behind.
- **Speed curve.** Movement slows near the window. Softens near the bed. The room teaches you to be still.
- **Audio.** Intimate, warm, slightly muffled. Clock tick as heartbeat. The hotel hum that you don't notice until it's gone. Through the wall: muffled music, someone else's TV, a phone ringing unanswered. Other lives.
- **The adjoining door.** Leads to a second room, also booked, also empty. The hotel doesn't know the trip changed. Walking into it is optional. It's worse if you do.

### The Paris Dream
A memory. The last good trip. Hard cut mid-suite. B&W Godard grain.

- **Duration:** 60-90 seconds. No longer. A short film within the film.
- **Tone:** Not nightmarish — wistful. Kaufman interiority. You're walking through a memory looking for someone who was there but isn't in the memory either. She just left the room. Steam in the bathroom. Her coat on the chair. Coffee still warm.
- **Design:** Rebuild HALLWAY/ROOM/BATHROOM in B&W. Compress geometry. Dream logic — doors lead where they shouldn't. Proportions slightly off. The hallway is longer than it should be.
- **The red object:** One color survives the B&W — a scarf, a book cover, a flower. Godard red. Memory preserves color selectively.
- **The toothbrush:** In the bathroom glass. The most mundane detail. The most devastating on replay.
- **The window:** Shows the Eiffel Tower, then the Sky Tower, then neither. Memory edits itself.
- **The book:** Same novel as the suite nightstand, but open to a different page. Further along. She was reading it here. The timeline is real even if the geometry isn't.
- **Rule:** Never explained. Never acknowledged. Hard cut back to the suite. Same spot. Same empty glass. The game continues.

### The Ending Sequence
Rapid-cut montage. The game's most emotionally demanding sequence.

- **Structure:** Jump cuts between locations recontextualized by what the player now understands. Every "two" the player noticed becomes a "one."
- **Pacing:** Thirty Flights timing — each cut 2-5 seconds. Some cuts are single frames.
- **The cleaned room:** The suite appears in the montage, but reset. One pillow. One robe. One glass. The hotel has moved on. Housekeeping has erased her. The most violent cut in the sequence.
- **Gibbons:** In the taxi. Going home. Already leaving. Brief eye contact. He adjusts his tie.
- **The photograph:** Face up. The two of you. She's laughing. The game's only clear image of happiness. Hold.
- **Final shot:** The void. The empty glass. Silence. The music returns — three notes of the suite piece, then nothing. Credits.

---

## VI. The Object Inventory

Every object in the game exists because it tells the story. Nothing decorative. This is the complete list of narrative objects and what they communicate.

### Objects That Prove "Two"
| Object | Scene | What It Says |
|--------|-------|-------------|
| Second ticket / booking confirmation ("2 guests") | Taxi | The trip was planned for two |
| One bag (conspicuously light) | Lobby | She didn't come |
| Two chairs angled together | Hotel | The hotel is designed for couples |
| Two pillows | Suite | The bed was made for two |
| Two robes (different sizes) | Suite | She was expected |
| Two champagne glasses | Suite | The ritual was shared |
| Her book (spine cracked, mid-read) | Suite | She was real, she was here in spirit |
| Photograph face-down (Paris café) | Suite | A specific shared memory, hidden |
| Room service card (two handwritings) | Suite | You planned this together |
| Thermostat at 22° (compromise temp) | Suite | A negotiated intimacy |
| Hotel slippers, still wrapped | Suite | Nobody coming to use them |
| Shoes arranged for two | Suite | The room was set before you arrived |
| Adjoining room (booked, empty) | Suite | The hotel's version of her |
| Her coat on the chair | Paris dream | She was just here |
| Coffee for two, one half-drunk | Paris dream | She was just here |
| Pillow indent (fresh) | Paris dream | She was just here |
| Her toothbrush | Paris dream | The most mundane, the most real |
| Boarding pass bookmark (her name) | Suite + Paris | The timeline connects |

### Bolaño Objects (The Abstract-Mundane)
These resist interpretation. They're the geometry textbook on the washing line. Don't explain them.

| Object | Scene | Character |
|--------|-------|-----------|
| Geometry textbook open on desk | Suite | Someone studying orbital mechanics for a conversation that never happened |
| Half-written postcard to no one | Suite | Handwriting changes mood midway. Unreadable at 480x300. That's the point. |
| One sock under the bed | Suite | The most mundane object in the game. The most real. |
| Cheese plate disagreement (margin notes) | Suite | A tiny, lived argument. Resolved with an arrow. |
| Red object in B&W space | Paris dream | Memory preserves color selectively |
| Window showing wrong city | Paris dream | Memory edits itself |

### Objects in the Montage (Recontextualized)
| Object | Original Context | Montage Context |
|--------|-----------------|----------------|
| Second ticket | Planted, pocketed | Seen again — now you know why |
| Two robes | Suite detail | Cut to one robe — the hotel cleaned up |
| Empty champagne glass | Ritual | Still there. Still empty. The game's final image. |
| Photograph | Face down | Face up. She's laughing. The only image of happiness. |
| Pillow indent | Ambiguous | The last cut before the void |

---

## VII. Development Phases

### Phase 1: The Suite (Depth)
**Goal:** Make the Space Suite the best room in any walking simulator. The room for two with one person in it.

- [ ] Room-for-two layout — king bed, two pillows, two robes, two glasses, her book, photograph
- [ ] Interaction depth — each ritual (bath, champagne, balcony, bed) with multi-step progression
- [ ] Cork pop sound (procedural)
- [ ] The empty glass — champagne poured into one, second stays empty, persists all game
- [ ] Object storytelling — photograph (turnable), book (with bookmark), room service card, postcard
- [ ] The adjoining door — second room, booked, empty, optional to enter
- [ ] Bolaño objects — geometry textbook, sock, postcard, margin notes
- [ ] Room acoustic refinement — intimate signature distinct from all other scenes
- [ ] Per-position speed curves — slow near window, soft near bed
- [ ] Bed ritual — lie down, agency surrender, composed music trigger, hold

### Phase 2: The Hotel (The Gehry Space)
**Goal:** Build the arrival reveal. Beautiful AND lonely.

- [ ] Design the Gehry space — curved glass walls, massive open plan, Earth visible below
- [ ] Pairs everywhere — two chairs, tables for two, couple-scale architecture
- [ ] Lighting — Earth-glow through glass, warm interior pools, Godard red accents at scale
- [ ] Gibbons pathfinding through the hotel space to the suite
- [ ] Implied hotel life — other corridors, muffled laughter, couples' ambient sounds
- [ ] The agoraphobia moment — vast and beautiful and you're alone in it

### Phase 3: The Corridor + Elevator (Pacing)
**Goal:** The journey from lobby to suite should feel like a narrative. Gibbons and the player, alone together.

- [ ] Elevator scene — warm amber, Chevalier energy, Gibbons's professional silence
- [ ] Gibbons glance — looks behind you once in the lobby, then never again
- [ ] Inaccessible rooms — doors with light underneath, muffled music, other lives (other couples)
- [ ] Gibbons character polish — walk cycle, formal gestures, the bow, carrying one bag
- [ ] Porthole moments — Earth rotating, time passing

### Phase 4: The Paris Dream (The Memory)
**Goal:** Build the mid-suite hard cut. 60-90 seconds of wistful memory.

- [ ] Rebuild Paris scenes in B&W — HALLWAY, ROOM, BATHROOM
- [ ] Her presence — coat, coffee, pillow indent, steam, toothbrush
- [ ] The book — same novel, different page, further along
- [ ] Dream logic geometry — doors lead wrong, proportions off, hallway too long
- [ ] Godard red — one color object survives the B&W
- [ ] Memory window — Eiffel Tower / Sky Tower / neither
- [ ] Hard cut in — no transition, no fade, instant
- [ ] Hard cut back — same suite position, same empty glass, no acknowledgment

### Phase 5: The Ending (Rapid Cuts)
**Goal:** Build the Thirty Flights montage. Every "two" becomes "one."

- [ ] Jump cut system — instant scene transitions with 2-5 second holds
- [ ] The cleaned room — suite reset: one pillow, one robe, one glass. The hotel moved on.
- [ ] Montage sequence — taxi ticket, Gibbons glance, robes, Paris coat, balcony, elevator down
- [ ] Gibbons in the taxi — going home, eye contact, already leaving
- [ ] The photograph face up — she's laughing, hold
- [ ] Single-frame cuts for peak intensity
- [ ] Final void — empty glass, silence, three music notes, credits
- [ ] The pillow indent — last image before void

### Phase 6: Sound Design (The Invisible Architecture)
**Goal:** Sound tells the stories geometry can't. The hotel hums with other people's lives.

- [ ] Per-scene acoustic signatures — lobby (expansive), elevator (compressed), suite (intimate), hotel (vast), dream (muffled/compressed)
- [ ] Composed music — one piece, triggered once in the bed ritual, the emotional center
- [ ] Three-note callback — the piece returns as a fragment in the ending montage
- [ ] Inaccessible room sounds — music through walls, muffled laughter, a phone ringing, running water
- [ ] The removal — the hotel hum disappears in the ending. Silence after constant.
- [ ] Dream audio — compressed, B&W sonic palette, rain on glass, distant traffic
- [ ] Taxi radio — a song. Something warm. Cuts off at hyperspace.
- [ ] Suite sounds — clock tick, champagne pour into one glass, the cork

### Phase 7: The Recurring Motif (Twos)
**Goal:** Plant pairs in every scene. The game is full of twos and the player is one.

- [ ] Taxi: second ticket
- [ ] Lobby: Gibbons's glance behind you
- [ ] Elevator: two people (you and Gibbons), then one (just you)
- [ ] Hotel: chairs for two, tables for two, couple sounds
- [ ] Suite: everything — glasses, robes, pillows, shoes, the book
- [ ] Paris: coffee for two, coat + your coat, two-person bed
- [ ] Montage: every two becomes one. The cleaned room. The reset.

### Phase 8: Polish & Release
**Goal:** Ship. Short and dense, not long and sprawling.

- [ ] Playtesting — 5-10 people, watch silently, note pauses/rushes/confusion
- [ ] Does the player understand by the end? Without being told?
- [ ] Performance — locked 60fps, lo-fi demands smoothness
- [ ] The Gehry space performance — largest scene, needs careful optimization
- [ ] Platform targets — macOS first, then Windows/Linux via Raylib
- [ ] Runtime target — 15-30 minutes first playthrough. If longer, cut.
- [ ] Price — free or $3-5. The game earns respect through craft, not length.
- [ ] Store page — "Three hours to kill." One screenshot. One sentence. Trust the work.

---

## VIII. What Not to Do

These are temptations. Resist them.

- **Don't add combat, puzzles, or fail states.** The game is about inhabiting a space, not conquering it.
- **Don't add a map or minimap.** Architecture guides the player. If they get lost, the architecture is wrong.
- **Don't add collectibles.** Objects exist to tell stories, not to be collected.
- **Don't add dialogue.** If the game ever uses voice, it should be located (phone, radio, PA system), unreliable, and contradicted by the space. Never omniscient narration. Never "she" or "we" spoken aloud.
- **Don't explain what happened.** The player doesn't need to know if it was a breakup, a death, a dissolution. The room tells them someone was supposed to be here. That's enough.
- **Don't show her.** She exists through objects. The moment you render her — even as a silhouette, even in a dream — you collapse the player's version of her into yours. She stays in the negative space.
- **Don't make it melodramatic.** No tears. No dramatic music swells. No rain-on-window-while-sad. The champagne glass is empty. That's the drama. Restraint is what makes it hit.
- **Don't add more scenes to increase runtime.** Add depth to existing scenes instead.
- **Don't chase realism.** 480x300 with procedural textures IS the aesthetic.
- **Don't explain the hotel.** Why is there a hotel in orbit? The mystery is the point.
- **Don't explain the Paris dream.** It's a memory. Memories don't announce themselves.
- **Don't make the hotel small.** The Gehry space is enormous. The suite is generous. Agoraphobia, not claustrophobia.
- **Don't make Gibbons creepy.** He's warm, formal, dignified. He's kind about it because kindness is the only dignified response.
- **Don't make the absence horror.** She's not dead (probably). She's not a ghost. It's not a twist. It's a breakup. It's the most common tragedy in the world and that's why it works.
- **Don't over-read your own influences.** Reference Blendo and Godard without pretending to channel them. Maybe you just like making hotels.

---

## IX. The North Star

When everything else is noise, return to this:

**The game is one night in an impossible hotel. You came here with someone. She's not here. The suite was booked for two. You pour one glass of champagne. You lie in a bed made for two. A memory of Paris interrupts — her coat on the chair, her toothbrush in the glass — and then you're back. The game fractures into rapid cuts. Every pair becomes a single. Gibbons reappears in the taxi, already leaving. The hotel has already cleaned the room. One pillow. One robe. One glass. The void remains.**

Everything in the game serves that sequence. Everything that doesn't serve it gets cut.

Every second earned.
