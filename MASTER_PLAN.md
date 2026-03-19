# Endearing Void — Master Plan

> "You don't need much. Spaces that mean something. A perspective that commits to its constraints. Trust that the player will fill the void. The courage to end before you've explained everything."

This document is the long-term development plan for Endearing Void. Every design decision should answer to this plan. When in doubt, return here.

Last major revision: 2026-03-19

---

## I. What Endearing Void Is

A first-person narrative game about three hours to kill in an impossible hotel. 15-30 minutes long. Built in C99 with Raylib. 1920x1200 native resolution, procedural everything. No hand-holding, no fail states, no dialogue trees. Architecture is the story. The void is the point.

**The emotional core:** You came here with someone. They're not here. The suite was booked for two. Everything in the room — the bed, the champagne, the bathrobe on the door — remembers them. You're doing the trip alone. The hotel is the last place the relationship existed. Three hours to kill before the life after starts.

**The emotional trajectory:** Arrival → Intimacy → Fracture → Escape. Taxi → Hotel → Paris Dream → Rapid Cuts. The world contracts (taxi → elevator → suite) then cracks (the dream, the memory) then explodes (Thirty Flights-style rapid cuts, heightened, dramatic, characters reappearing with context).

**The secret the game keeps:** The character was always leaving, not arriving. The trip was planned for two. You came anyway. You never learn exactly what happened between you — a breakup, a death, a quiet dissolution, an argument at the airport. The hotel is the pause between the relationship and whatever comes after. By the end the player knows without being told.

**What the author knows (the player doesn't):** She left three weeks ago. Not dramatically — she just stopped being there. The trip was her idea. The booking is in her name. You came because cancelling felt like admitting it was over. The photograph face-down on the nightstand is the two of you at a café in Paris. That trip — the last good one — is the Paris dream. Gibbons has seen this before. He was expecting two. He says nothing.

**The spiritual lineage:**
- **Hotel Chevalier** (Anderson) — Two people in a hotel room with unresolved history. The room IS the relationship. The yellow bathrobe. The bruises. Where they stand tells you everything.
- **Firewatch** (Campo Santo) — Henry is running from his wife's illness into a beautiful place. The tower is the hotel — a gorgeous space to avoid your life. The conspiracy is irrelevant; the real story is whether he goes home.
- **Gravity Bone / Thirty Flights of Loving** (Chung) — Architecture IS narrative. Cut everything non-essential. Rapid-cut endings where passing characters suddenly have context.
- **The Beginner's Guide** (Wreden) — Agency is a dial. Unfinished spaces are intentional. Silence after sound is devastating. The lamppost principle: a motif that accumulates meaning through repetition.
- **Godard (Contempt)** — A marriage disintegrating in a beautiful house. Every color choice serves an actual emotional state between two people. Withhold the explanation, give the feeling. The reds.
- **Barton Fink** (Coens) — The hotel as psychological space. Character under pressure from something we don't fully understand. The world slightly wrong in ways that accumulate.
- **Bolaño (2666)** — Amalfitano hangs a geometry textbook on the washing line and leaves it there. It gets rained on. Nobody comments. A person doing something slightly unhinged because they're falling apart in a way they can't articulate. Dignity-in-transit. Characters too aware, too polite, slightly broken.
- **Kaufman** (Anomalisa, Synecdoche) — Unreliable interiority. The character's experience may not be what's happening. Memory and present tense bleed.
- **Bioshock Infinite** — Arriving into something impossible. The scale does the work.
- **WALL-E** (Stanton) — A small person alone in a vast space, surrounded by objects left behind by people who are gone. The loneliness isn't tragic — it's tender. WALL-E collects things he doesn't fully understand (a Rubik's cube, a lighter, a spork) because they're evidence that someone was here. That's what the player does in the suite — turning over the photograph, picking up her book, looking at the sock under the bed. Objects as stand-ins for connection. The first 40 minutes of WALL-E are almost wordless — architecture and objects and one small figure moving through them. The observation deck has WALL-E's scale: a tiny person in an impossible machine, looking at Earth, holding something that belonged to someone else.
- **Mirror's Edge** — Movement that feels good changes everything. Even in exploration, physicality matters.
- **Easy Delivery Co.** — Ambient gradients, no guidance UI, through-wall audio, implied world.

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
TAXI — Auckland. 2:47 AM. Inside the car. City scrolls past.
       Sky Tower grows through the windshield. The driver talks.
       The player's responses are internal — silent choices
       that reveal backstory without breaking the drive.

       THE DETAIL: The second ticket. The booking confirmation.
       "2 guests." The player's first interaction is context.

       Emotional register: anticipation, motion, urban rhythm,
       the particular loneliness of going somewhere familiar
       without the person who made it familiar.
       Audio: city ambient, engine hum, tires on wet road.
       The driver's voice — casual, oblivious, kind.
       The last grounded moment.

  ↓ (auto-transition at ~15s)
HYPERSPACE — The tower becomes a tunnel. Reality breaks.
             Disorienting roll, extreme chromatic aberration.
             Hard cut to silence. Title card burns in:

             E N D E A R I N G   V O I D

             The threshold isn't just spatial — you're leaving
             the world where the thing happened.
             Emotional register: vertigo, threshold crossing, relief.

  ↓ (hard cut to silence)
SPACE_LOBBY — You're in orbit. Small arrivals hall.
              Gibbons appears. He was expecting you.

              THE DETAIL: He glances behind you. Just once.
              Then catches himself. Perfect composure from then on.
              He picks up your bag — one bag, conspicuously light
              for a suite booked for two. He doesn't mention it.
              Bolaño politeness. The kindness of not acknowledging.

              Emotional register: wonder, impossibility, warmth,
              the slight embarrassment of being seen alone.
              Audio: hull resonance, air circulation, insulated quiet.

              This is NOT the big space. This is an arrivals lobby —
              functional, warm, a desk and a chandelier. The scale
              comes next.

  ↓ (Gibbons leads you to the elevator)
ELEVATOR — Warm amber, wood wainscoting, leather handrail.
           Hotel Chevalier energy — two people in a small space
           with unspoken knowledge. Except one of them is staff
           and the knowledge is professional. Gibbons has seen
           this before. Solo arrivals on double bookings.
           He adjusts his tie. The player has nothing to adjust.
           Emotional register: compression, intimacy, anticipation.

           The doors close. The smallest space in the game.
           Then they open.

  ↓ (doors open)
THE GLASSHOUSE — The Bioshock Infinite moment. The doors part
                 and you're standing in a glass envelope.

                 THE FIRST BIG SPACE. The hotel's observation
                 lounge — a communal area where guests come to
                 watch Earth. Massive curved glass walls. Steel
                 ribs. The void in every direction. Earth fills
                 the lower hemisphere. Stars above.

                 And it's populated. Not with people — with
                 EVIDENCE of people. Coats on chairs. Half-finished
                 drinks. A book open face-down. Two coffee cups,
                 one still steaming. Someone's scarf. The room
                 is warm with the residue of other couples who
                 stepped away. You've arrived into their absence.

                 TWO CHAIRS angled toward each other in front of
                 the Earth window. Designed for two people to sit
                 and watch the planet together. Tables set for two.
                 Everywhere: pairs. The hotel is designed for couples.
                 You're alone in the most beautiful room you've
                 ever seen.

                 NOT claustrophobic — agoraphobic. The scale of
                 the space matches the scale of what you're feeling.
                 Emotional register: awe, vertigo, beauty, loneliness.

                 Audio: hull resonance, the creak of glass under
                 pressure, distant sounds of other guests (laughter,
                 conversation muffled through structure), wind-like
                 air circulation. The acoustic signature of VAST.

                 Gibbons doesn't stop. This isn't your destination.
                 He leads you through. You SEE it — the scale hits —
                 and then the world begins to contract.

  ↓ (Gibbons leads you through to the corridor)
SPACE_CORRIDOR — Kubrick corridor. Narrower now. The contraction
                 has begun. Gibbons leads. Doors to other lives.

                 Muffled piano from Room Four. TV murmur from Room Six.
                 The gentleman in Six orders room service for two.
                 Every night. Sends half back.
                 Parallel footsteps behind the wall.

                 Each door is a relationship you can hear but not reach.
                 The player passes through other people's intimacy.

                 THE IMPOSSIBLE DOOR: One door is ajar. Beyond it,
                 raw hull. Not a room. The hotel isn't finished.
                 Or was never meant to go that far.

                 Emotional register: anticipation, intimacy, other lives.
                 Audio: footsteps (yours, Gibbons', the ghost's),
                 through-wall sounds, the approach to your door.

                 THE CONTRACTION:
                 Glasshouse (20m wide) → Corridor (4.5m wide)
                 → Suite (one room). The architecture compresses
                 until it's just you and the bed and the empty glass.
                 This is the emotional arc made spatial.

  ↓ (Gibbons opens your door)
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

              THE INTERACTION MODEL:
              Two systems run in parallel:

              1. PRESENCE ZONES — dwell timers trigger interactions
                 by being near objects. The room responds to WHERE
                 YOU STAND and HOW LONG YOU STAY. The bed zone (8s),
                 the champagne zone (3s), the window zone (15s),
                 the desk zone (5s). Movement IS the game verb.

              2. E-PRESS — fallback for players who want buttons.
                 Both systems coexist. The room doesn't care how.

              THE RITUALS:

              CHAMPAGNE — Walk near the tray. The cork pops
              automatically. The room was expecting you. One glass
              fills. The second stays empty. THIS IS THE
              WASHING-LINE MOMENT. The empty glass just sits there
              for the rest of the game. Nobody comments. It's the
              most violent image in the game and it's completely still.

              BATH — Enter the bathroom. Water starts running.
              Steam on the glass. Earth rotating through the fog.
              The bath is big enough for two. You sit on the edge.
              The warmth of the water is the warmth of the
              relationship — present, surrounding, slowly cooling.

              DESK — Crouch near it. The lamp turns on. Her margin
              notes become visible in the light. You're reading
              by lamplight. The interaction is slowing down enough
              to see detail.

              BED — Stay near the bed. Movement slows. The covers
              pull back. The camera begins to lower. The player is
              choosing to lie down by being still. The bed ritual
              triggers not from pressing E but from STAYING.
              Surrender through inaction.

              The ceiling. The game takes control — agency surrendered.
              The indent on the other pillow. The composed music
              triggers. One piece. Once. Never repeats.
              This is the emotional center of the game.

              THE SUITE REVEALS ITSELF:
              After each interaction, something shifts.
              1 task: Photograph moves 3cm
              2 tasks: Bathroom door cracks, amber light spills
              3 tasks: Earth glow shifts, champagne glass drifts
              4 tasks: Second pillow gains indent — depression + shadow

              The wrongness isn't the hotel. It's your situation.

              SILENT OBJECTS (no text, visual consequence only):
              - Wardrobe → opens, two robes visible
              - Wine glass → lipstick mark visible
              - Book → drops boarding pass as geometry
              - Sock → under the bed, just there
              - Phone → screen lights up, playlist visible
              - Suitcase → tags still on
              - Bath → water runs, steam appears

              DIEGETIC TEXT (kept — you're READING something):
              - Ticket: "2 guests."
              - Photograph: "Paris. Before all this."
              - Room service card: two handwritings, cheese plate argument
              - Postcard: half-written, addressed to no one
              - Guest book: two names, one crossed out
              - Newspaper: "ORBITAL SUITE OPENS — THREE HOURS TO KILL"

              BOLAÑO DETAILS:
              The real Bolaño move isn't citation — it's the
              mundane object that resists interpretation.
              - One sock under the bed. Not yours. There it is.
                The most mundane object in the game. The most real.
                This is the geometry textbook on the washing line.
                Not a reference TO Bolaño. The thing itself.
              - Room service menu with margin notes — two people
                comparing orders, a tiny disagreement resolved
                with an arrow
              - Half-written postcard, handwriting changes mood
```

### The Balcony (From the Suite)
```
BALCONY — Step outside. A small private balcony off the suite.

          NOT the glasshouse — you've already seen that.
          This is intimate. Personal. The Chevalier balcony.

          A narrow platform. Brass railing. Glass infill panels.
          Two chairs side by side. A small table. Ashtray.
          Telescope pointing at Earth. The cigarette.

          The difference: the glasshouse was public, populated
          with evidence of other people. This is YOURS.
          Your chairs. Your view. Your cigarette.
          The two chairs here are the two chairs from the
          glasshouse — same design, private edition.
          Designed for you and her.

          Low gravity — 0.3x. The player can jump and land
          on the railing. No prompt. No text.

          Portman in the yellow robe looking at Paris,
          except here it's the whole planet and you're looking
          at the place where your life is. Down there, somewhere,
          is the apartment.

          Agency drains over 14 seconds. The player loses control
          slowly. The game is taking you to the Paris dream.

  ↓ (hard cut at 14s)
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

Triggered after the bed ritual. The game decides.

Rapid cuts — jump cuts between spaces. Each 2-5 seconds.
Some cuts are single frames. Time collapses.
The "passing characters now have context" feeling.

THE MONTAGE (sequence):

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

ENDEARING VOID
A game by Maxwell Young

Credits.
The game is over. The feeling is not.
```

### The Return (Second Playthrough)
```
RETURN_TAXI — Auckland. 5:52 AM. Dawn light.
             Same car. Driver doesn't talk.
             Gibbons is in the back seat. Going home.
             "Good hotel?" — Yeah. / ...
             The game loops to TITLE.

Second play: everything changes.

SUITE STARTS CLEANED: One pillow. One robe. One glass.
Photograph already face-up. You arrive into the aftermath.
The hotel remembered. Housekeeping erased her.

CORRIDOR IS SHORTER: All Z-positions scaled to 65%.
32m → ~21m. "Same corridor. Shorter this time."
Piano silent (Four stopped playing).
Room service tray gone (Six checked out).
No parallel footsteps. The corridor is emptier.

TAXI IS SILENT: No driver dialogue. No choices.
"Same city. Same hour." → "You know the way."

GIBBONS HAS DIFFERENT LINES: Every scene.
He knows you came back.

First play: you discover twos, then watch them become ones.
Second play: you arrive into ones, and REMEMBER the twos.
The emotional direction reverses. Loss → Grief.
```

---

## IV. Characters

### Gibbons (The Bellhop)
Cube-person. Geometric, segmented limbs, red bellhop cap. Formal, not-quite-human. A Bolaño character — too aware, too polite, slightly broken. Has been waiting in hotel lobbies so long he's become furniture. Adjusts his tie. Carries a briefcase he never opens.

**What Gibbons knows:** The suite was booked for two. He's seen this before — solo arrivals on double bookings. People who came anyway. He doesn't judge. He's kind about it because kindness is the only dignified response. He was a version of this person once, maybe. Or he just understands service as a form of care.

**Arc:** Gibbons was expecting two. He sees one. He glances behind you, once, then never again. He carries your one bag with the same ceremony he'd carry two. He leads you to the suite. He bows. He leaves. In the ending montage he reappears — in the taxi, going home. He's already leaving too. Brief eye contact. He knew the whole time. Not what happened to her. Just that you came alone and the room was ready for two and that was enough to know.

**Design:** Waypoint-based navigation. Walk cycle with character — purposeful, slightly formal. Pauses at thresholds. Gestures you through. Per-waypoint dialogue. Physics modes: ghost (walks through geometry) vs grounded. Drawing uses macros for local-space positioning relative to NPC yaw.

**Technical:** `npc.c` — segmented cube-person with per-limb positioning. GLB model with animations loaded at startup. Fallback to procedural cube-person if GLB missing. Waypoint system with dwell times and dialogue triggers.

**Key Lines (First Play):**
- Lobby: "Ah. We have your reservation."
- Corridor: "Someone in Four plays that same nocturne every evening."
- Corridor: "The gentleman in Six orders for two. Every night. Sends half back."
- Corridor: "Yours is at the end. I left the curtains open."

**Key Lines (Return):**
- Lobby: "Welcome back."
- Corridor: "Four stopped playing last week. Nobody noticed."
- Corridor: "Six checked out. The trays stopped coming."
- Corridor: "Yours is as you left it."

### The Player Character
No name. No dialogue. No visible body. Came here with someone who isn't here. The trip was planned together — the booking, the room service card, the champagne. You came anyway. Why? The game never says. The player decides: grief, stubbornness, a need to prove something, an inability to cancel. Maybe all of them. The hotel is the pause between the relationship and whatever comes after. Kaufman interiority — we see through their eyes but can't trust what we're seeing. The Paris dream might be memory or invention or wish. By the end the player knows without being told.

### Her (The Absent)
Never seen. Never named. Never described physically. Present only through objects: her book, her robe, her toothbrush, her coat, her handwriting on the room service card, the indent on the pillow, the photograph where she's laughing. The player assembles her from traces. Every player assembles a different person. She exists in the negative space. The game's most important character is the one who isn't there.

---

## V. Key Scenes — Architecture & Design

### The Taxi (STATE_CAR → STATE_DRIVING)
The grounded scene. Auckland at dawn. Wet streets. The Sky Tower in the distance growing larger.

**Purpose:** Establish the real world before breaking it. Plant the first "two" — the second ticket.

**Architecture:** Interior of a car. Dashboard, windshield, side windows. City scrolling past as parallax layers. The Sky Tower growing in the windshield — the first architecture the player sees. Simple, intimate, constrained.

**Light:** Dawn. Golden hour through wet glass. The last warm light that comes from the sun.

**Audio:** City ambient, engine hum, tires on wet road. Phase-driven driver dialogue — each beat waits for the previous to complete. Choices are internal monologue.

**Movement:** None. The player is a passenger. Camera bobs gently with the car. The first and last time they have no agency.

**Interaction:** The prologue choices from the title screen color the driver's lines. The player listens, responds in their head. The driver doesn't know. He's just making conversation at 2:47 AM.

**The "twos":** Second ticket on the seat. Booking confirmation: "2 guests."

### The Space Lobby (STATE_SPACE_LOBBY)
The arrival. Earth through glass. Gibbons appears.

**Architecture:** The lobby of a luxury space hotel. High ceiling, observation windows, reception desk, chandelier, columns. Architectural reference: a grand hotel lobby transplanted into orbit. The Overlook's ambition with Kubrick's precision. Not organic — geometric, intentional, warm.

**Light:** Earth-glow through observation windows — blue-white wash across the floor. Warm amber interior pools from chandeliers and wall sconces. The contrast between cold space-light and warm hotel-light is the scene's visual thesis.

**Audio:** Hull resonance. Air circulation. Insulated quiet. The first space sound — the absence of weather, traffic, the world. Footsteps echo differently here. Marble surface.

**Movement:** Full movement restored after the taxi's stillness. The scale invites sprinting. The lobby rewards exploration with spatial reveals — Earth visible from different angles, the depth of the station visible through structural gaps.

**Gibbons:** Appears at the far end. Walks to the player. "Ah. We have your reservation." Picks up the one bag. Leads to the corridor.

**The "twos":** Two chairs in the lobby, angled toward each other. Tables set for two. Couple sounds through walls.

### The Space Corridor (STATE_SPACE_CORRIDOR)
Kubrick hallway. Gibbons leads. Other lives behind doors.

**Architecture:** 4.5m wide, 32m long curved arc. 8 segments with a gentle radius-40m bend. Porthole windows on alternating sides. Three doors — Room Four (piano), Room Six (TV, orders for two), Room at the end (yours). Carpet runner, brass trim, ceiling conduits. Kubrick's Overlook corridor meets 2001's Discovery.

**Architectural references:**
- **2001: A Space Odyssey** — the centrifuge corridor, curved floor, institutional precision
- **The Shining** — the Overlook's corridors, carpet patterns, symmetry that unsettles
- **Ocean liners** — portholes, brass fittings, the romance of transit

**Light:** Dark by design. Warm amber ceiling strips every other segment. Door-light spill: amber from Four (reading lamp), blue from Six (TV flicker), darkness from yours. The light under each door tells a story about the person inside.

**Audio:** Through-wall sounds are the scene's primary narrative tool.
- Room Four: muffled nocturne (first play). Silent (return).
- Room Six: TV murmur + running water. TV goes silent if you stand near the door for 6 seconds — they heard you. Or turned it off to sleep.
- Room service tray outside Six: two plates, one untouched (first play only).
- Parallel footsteps: someone behind the wall. Matches your pace. 0.4s delay. Stop when you stop. Start when you start. Panned to opposite wall. First play only.

**Movement:** Full physics. Gibbons walks at 3.5. Player walks at 4.5. If you bunny-hop ahead, Gibbons slows. Waits. Says nothing. Infinite patience. The movement system creates a character moment.

**Spatial mechanics:**
- Speed slows near windows (px > 1.5f → 35% velocity reduction)
- Spatial compression: player velocity subtly increases if moving toward exit
- Silence zones near doors (master volume fades)
- Dark door proximity increases grain and drops exposure

**The "twos":** Room service tray with two plates. The piano played for someone. The TV watched by someone. You and Gibbons — two people, one leading, one following. Then one.

**Technical:** 500+ walls. Curved geometry via 8-segment arc. Point lights on doors. Dynamic spatial audio. Gibbons waypoint system. Second playthrough: all Z-positions scaled to 65%.

### The Space Suite (STATE_SPACE_SUITE)
60% of playtime. Where the game lives or dies.

**Architecture:** Generous, glass-fronted, warm. One room with bathroom and balcony access. King-size bed centered, two nightstands. Desk with lamp. Champagne tray. Wardrobe. Bathroom door. Balcony door. The room is a portrait of the relationship — designed for two, occupied by one.

**Architectural references:**
- **Hotel Chevalier** — the room where everything is communicated through where people stand and what they touch
- **Barton Fink** — the hotel room as psychological space, wallpaper peeling, the world accumulating wrongness
- **Firewatch** — a beautiful space you went to instead of dealing with your life

**Light:** Warm. SetPostFXWarmth ranges 0→1 as the player completes tasks. The room literally warms. Clock tick decelerates. The room is holding you.

**Audio:** Intimate, warm, slightly muffled. Clock tick as heartbeat (decelerates with task completion). Hotel hum. Through the wall: muffled sounds from the corridor. The composed music triggers during the bed ritual — one piece, once, never repeats.

**Movement:** Presence zones with dwell timers:
- Bed zone: 8s → covers pull back, camera lowers
- Champagne zone: 3s → cork pops, one glass fills
- Window zone: 15s → Earth rotates to show NZ, grain clears
- Desk zone: 5s → lamp turns on, margin notes visible
- E-press works as fallback for all zones

**The "twos":** Everything. Two pillows, two robes, two glasses, two nightstands, shoes for two, thermostat at compromise temperature (22°), her book, her bookmark, two handwritings on the room service card. The room IS the motif.

**Wrongness progression:**
| Tasks Done | What Shifts |
|---|---|
| 1 | Photograph moves 3cm |
| 2 | Bathroom door cracks, amber light spills |
| 3 | Earth glow shifts, champagne glass drifts, her book shifts position |
| 4 (all) | Second pillow gains indent — depression + shadow. The most subtle. |

### The Paris Dream (STATE_PARIS_DREAM)
A memory. The last good trip. Hard cut mid-suite. B&W Godard grain.

**Architecture:** Rebuild of HALLWAY/ROOM/BATHROOM in B&W. Compressed geometry. Dream logic — doors lead where they shouldn't. Proportions slightly off. The hallway is longer than it should be. A real Parisian hotel — small, warm, ground floor. Rain on the window. Traffic outside.

**Architectural references:**
- **Contempt** — the apartment in Capri where the marriage ends
- **Breathless** — the Paris apartment, long takes, Jean Seberg's Herald Tribune
- **Hotel Chevalier** — the specific Parisian hotel room

**Light:** High contrast B&W. One color survives — Godard red. A scarf, a book cover, a flower in a glass. Memory preserves color selectively.

**Audio:** Compressed, muffled. Rain on glass. Distant traffic. The acoustic signature of a real city after the silence of space. The contrast is devastating.

**Her presence (the difference):**
- Her coat on the chair
- Coffee for two on the table — one cup half-drunk
- The indent on the other pillow, fresh
- The bathroom door is ajar. Steam. She just showered.
- Her toothbrush in the bathroom glass

**Duration:** 60-90 seconds. No longer. Hard cut back to the suite.

---

## VI. The Glasshouse — Full Design

The hotel's observation lounge. The first big space the player experiences. The Bioshock Infinite arrival moment. A new scene state: STATE_GLASSHOUSE (or repurpose STATE_SPACE_LOBBY).

### The Vision
The elevator doors open. The player steps into a glass envelope suspended in space. Earth fills the lower hemisphere. Stars above. The void in every direction. This is the hotel's communal observation lounge — where guests come to sit and watch the planet.

**The emotional function:** This is NOT intimacy — this is AWE. The WALL-E moment: a tiny person in a vast machine, looking at Earth. The space is populated with evidence of other couples — coats on chairs, half-finished drinks, two coffee cups. The player walks through other people's intimacy on the way to their own empty suite. The contraction begins here: Glasshouse (20m) → Corridor (4.5m) → Suite (one room) → Bed (one pillow). The architecture compresses until it's just you.

**The structural role:** The glasshouse solves the claustrophobia problem. The player's first impression of the hotel is enormous, beautiful, breathtaking. By the time they reach the suite, the smaller space feels intimate rather than trapped. The contraction IS the emotional arc made spatial.

### Architectural Design

**Form:** A Gehry-esque glass shell — not a box, not a dome, but an asymmetric glass envelope with structural steel ribs. Think Fondation Louis Vuitton's glass sails or the DZ Bank atrium. The glass panels are not flat planes — they curve, overlap, breathe. Some panels are frosted. Some are perfectly clear. The steel structure is visible — honestly expressed, not hidden.

**Scale:** Larger than the current 8m x 6m balcony. 20m x 14m interior volume. The glass extends upward 8-10m. You're inside a jewel box floating in space.

**Materials:**
- Glass panels: MAT_GLASS — transparent, slight blue tint, reflections of interior light
- Steel ribs: MAT_BRASS — warm metallic, structural, honest
- Floor: MAT_TILE — dark hull grating with brass drainage strips
- Back wall: MAT_CONCRETE — the hull of the station, the one solid wall

**Key architectural elements:**
- **Glass envelope** — the Blender shell. Organic curves impossible in code geometry. The visual spectacle.
- **Steel ribs** — visible structure. X-bracing, curved mullions, tension cables. The glass sits in a frame, not hanging free.
- **Floor plane** — solid. Dark. The only thing between you and the void.
- **Back wall** — the suite exterior. Door back inside. The warm light spilling through = the only warm light source.
- **Railing** — brass, waist-height, at the forward edge. Glass infill panels. Posts at 1m intervals. The player can stand ON the railing in 0.3x gravity.

**The furniture (code geometry inside the shell):**
The glasshouse is a communal lounge. It's populated with evidence of OTHER people's relationships — couples who were just here, stepped away, will return. You pass through their intimacy.

- Multiple seating clusters: pairs of chairs angled toward each other, small tables between them
- Evidence of presence: a coat draped on a chair, a half-finished drink, a book open face-down, two coffee cups (one still steaming), someone's scarf
- A telescope on a tripod. Pointing at Earth. Brass. Communal — not yours.
- A bar or service counter along the back wall — glasses, bottles, the infrastructure of hospitality
- The "twos" planted at scale: every table is set for two. Every cluster is a pair. The room is designed for couples. You're walking through it alone.

**NOT interactable.** The player passes through. They see. They don't touch. This is Gibbons' pace, not theirs. The interactions come later, in the suite. Here the game is purely visual — scale, beauty, loneliness.

### The View

**Earth as Rothko field:** Not a ball — a HORIZON. Five layers of overlapping color fields meeting at edges. The planet is a vast, slowly-rotating presence filling the lower view.
- Layer 1: Glass boundary — barely visible. The membrane between life and death.
- Layer 2: Atmosphere — bright blue-white band at the horizon. The hero visual.
- Layer 3: Planet surface — dark ocean, land masses, slightly green-shifted patches.
- Layer 3b: City lights — BIG clusters. Auckland identifiable. Sky Tower beacon.
- Layer 4: Stars — dense field above the atmosphere line.

**Solar system context:** Distant station modules visible as lit rectangles. Connecting trusses. The hotel is part of a larger structure. The player is in a machine.

### Technical Approach: The Gehry Shell System

**Why the shell system:** The glass envelope's curves can't be done with `add_wall()` boxes. The Gehry shell system (`ev_shell_workbench.py`) was designed for exactly this.

**The split:**
- **Visual**: Blender-modeled glass shell → GLB → `add_shell()` — renders with MAT_GLASS, no collision
- **Collision**: Invisible AABB boxes — floor, back wall, side boundaries. Simple rectangular physics.
- **Furniture**: Code geometry — `add_wall()`, `add_cylinder()`, `add_sphere()` for chairs, table, telescope.
- **Earth layers**: Code geometry — overlapping wall planes at varying depths.

**Blender workflow:**
```python
exec(open('scripts/ev_shell_workbench.py').read())
shell_setup("observation_deck", width=20, depth=14, height=10)
# Model glass panels — curved surfaces, steel ribs, asymmetric envelope
# The shell is the WOW — spend time here
collision_floor(y=0)
collision_wall("back_wall", pos=(0, 5, 7), size=(20, 10, 0.3))
collision_wall("left_bound", pos=(-10, 5, 0), size=(0.3, 10, 14))
collision_wall("right_bound", pos=(10, 5, 0), size=(0.3, 10, 14))
shell_preview()
shell_export()
```

**Performance:** The glass shell is the largest visual element. Budget: 3000-5000 tris. The glass material is cheap (no diffuse texture, just fresnel + transparency). Stars and Earth are flat planes. Point lights minimal — the scene is lit by Earth-glow and suite-spill.

### Spatial Mechanics

**Low gravity (0.3x):** The player floats. Jumps are higher. Landing is slower. The physics communicate "you're in space" without text.

**Railing discovery:** Jump → land on railing → no prompt, no text. The physics just allow it. Standing on a brass rail at the edge of a space station looking down at Earth. FOV opens to 90°. Grain drops. Clarity at the edge. The wind (audio) picks up. CA increases. The game rewards the player who experiments.

**Agency drain:** Over 14 seconds, `control_mult` fades from 0.3 to 0. The player loses the ability to move. The game is taking them to the Paris dream. Not sudden — gradual. The last thing the player controls is where they look.

**Post-cigarette:** After the cigarette interaction, warmth drops (1.0 → 0.6), grain increases (0.5 → 0.65). The moment passed. The room gets colder. The architecture speaks.

### The Scene Across the Game

**First visit (Act 1 — arrival):** Wonder. The elevator doors open. The scale hits. Earth below. Evidence of other couples everywhere. Gibbons leads you through. You don't stop. The game's biggest, most beautiful space — and you're passing through it on the way to your room.

**Montage shot #9:** 2-5 second cut. The two chairs by the Earth window. Empty now. The coats are gone, the drinks cleared. The same space, depopulated. The hotel is closing down. Hold. Cut.

**Return playthrough:** The lounge is emptier. Fewer coats. Fewer drinks. The hotel is emptying. "Six checked out." The world is contracting even before you reach the corridor.

---

## VII. Characters — Deep Design

### Gibbons — Full Behavioral Spec

**Visual design:** Geometric cube-person. Segmented limbs. Red bellhop cap. Dark suit. Brass buttons. Briefcase (never opened). No face — a flat plane with minimal suggestion. Not humanoid enough to be uncanny. Abstract enough that the player projects warmth onto him.

**Movement vocabulary:**
- Walk: purposeful, slightly stiff, 3.5 units/sec
- Pause at thresholds: 0.8s dwell before entering new spaces
- Tie adjust: periodic idle animation. Nervous habit masked as professional composure.
- Bow: at suite door. Formal. Brief. Then exit.
- Glance: once, at lobby entrance. Looks behind the player. A head rotation. Then never again.

**Dialogue design (lower-third, House of Cards style):**
- Speaker name: small caps, warm gold, letter-spaced
- Text: warm white, typewriter reveal
- Gradient scrim at bottom — cinematic, not UI
- Left-aligned at margin 60px. Never centered. Never in a box.

**Per-scene behavior:**
| Scene | Waypoints | Behavior | Key Line |
|-------|-----------|----------|----------|
| Space Lobby | 3 (entrance → desk → exit) | Waiting, then leading | "Ah. We have your reservation." |
| Corridor | 3 (entrance → mid → suite door) | Walking ahead, pausing at doors | "Yours is at the end." |
| Suite | 1 (door → bow → exit) | Opens door, presents room, leaves | No words. A bow. |
| Return Taxi | 1 (back seat) | Sitting, looking forward | "Good hotel?" |

### Her — Object Manifest

She is never seen. She is everywhere. Her presence is the negative space the game is built around.

**Objects that prove she existed:**
| Object | Location | What it communicates |
|--------|----------|---------------------|
| Second ticket | Taxi seat | Two were coming |
| Her book | Suite nightstand | She was reading. She stopped. |
| Boarding pass bookmark | Inside the book | Her name. A real person. A real flight. |
| Photograph face-down | Suite nightstand | A memory hidden. Turnable. |
| Two robes (different sizes) | Suite wardrobe | She was expected. Specifically. |
| Two champagne glasses | Suite tray | A ritual. Shared. |
| Room service card | Suite desk | Two handwritings. A disagreement about cheese. |
| Thermostat at 22° | Suite wall | A negotiated temperature. Compromise. |
| Her coat | Paris dream chair | She was just here. Just missed. |
| Her toothbrush | Paris dream bathroom | The most mundane. The most real. |
| Pillow indent | Suite bed / Paris bed | Memory or reality. The game doesn't say. |
| One sock | Under suite bed | Not yours. There it is. |

---

## VIII. The Object Inventory — Complete

Every object in the game exists because it tells the story. Nothing decorative.

### Objects That Prove "Two"
| Object | Scene | What It Says |
|--------|-------|-------------|
| Second ticket / booking confirmation ("2 guests") | Taxi | The trip was planned for two |
| One bag (conspicuously light) | Lobby | She didn't come |
| Two chairs angled together | Observation Deck / Hotel | The hotel is designed for couples |
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
| One sock under the bed | Suite | Not yours. There it is. The most mundane. The most real. The actual Bolaño object. |
| Half-written postcard to no one | Suite | Handwriting changes mood midway. Unreadable at 1920x1200. That's the point. |
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

## IX. Movement & Physics — The Body as Instrument

### The Movement System

Quake-style air strafing, bunny hopping (50ms friction grace), wall running, ledge mantling, momentum slides, dashing. All tuning lives in `PhysicsConfig` (59 parameters) with defaults in `physics_default()`.

This is a world-class movement system sitting inside a narrative game. That's not a contradiction — it's the point. Movement IS interaction. Where you stand, how fast you move, how long you stay — this is the input.

### The Movement Vocabulary

| Action | Input | Emotional Register |
|--------|-------|-------------------|
| Walk slowly | WASD | Default exploration. The room at human pace. |
| Sprint | Shift | Nervous energy. Lobbies and corridors reward this — spaces open at speed. |
| Slide | Ctrl + Sprint | The corridor has a long straight. Sliding = arriving with momentum. |
| Crouch | Ctrl (still) | Smallness. At the window = looking up at Earth like a child. At the bed = eye level with the pillow indent. |
| Jump | Space | Low gravity makes this meaningful. The balcony jump = standing on the railing. |
| Bunny hop | Sprint + jump rhythm | Nervous fidgeting. Gibbons waits patiently. Character through physics. |
| Stand still | Nothing | The hardest action. The bed zone. The window zone. The game rewards stillness with revelation. |
| Look up | Mouse | The Sky Tower from below. The suite ceiling. The stars from the balcony. Awe is vertical. |
| Look down | Mouse | The pillow indent. The sock under the bed. The second ticket. Intimacy is downward. |

### Per-Scene Physics

| Scene | Gravity | Speed Mult | FOV | Character |
|-------|---------|-----------|-----|-----------|
| Taxi | N/A | 0 | 70 | Stillness. Passenger. |
| Lobby | 1.0x | 1.0 | 70 | Full agency. Scale invites speed. |
| Corridor | 1.0x | 1.0 (0.65 near windows) | 70 | Modulated. Windows slow you. |
| Suite | 1.0x | 0.8-1.0 (zone-dependent) | 70 | Intimate. Softens near bed. |
| Observation Deck | 0.3x | 0.3 | 70-90 | Float. Railing discovery. Agency drains. |
| Paris Dream | 1.0x | 0.7 | 65 | Compressed. Dream-slow. Claustrophobic FOV. |
| Montage | N/A | 0 | Variable | No control. Camera only. |

### Discovery Through Movement

The player who experiments with the movement system is rewarded:

- **Railing stand (Observation Deck):** Jump in 0.3x gravity → land on brass railing → FOV opens, grain drops, CA increases. No prompt. The physics just allow it.
- **Crouch at observation glass (Lobby):** FOV widens, camera lowers. You're small, looking up at Earth like a child. Changed emotional register.
- **Bunny hop ahead of Gibbons (Corridor):** He stops. Waits. Says nothing. Resumes when you're near. The movement system creates a character moment.
- **Sprint through corridor:** Spatial compression accelerates — you arrive at the suite faster. The architecture rewards decisiveness.
- **Stand still at window (Suite, 15s):** Earth rotates enough to see New Zealand. The game rewards looking by showing you home.

---

## X. Audio Architecture — The Invisible Building

### Principles

100% procedural. Every sound synthesized from sine waves, noise, and envelopes at 44100Hz. No audio files (except the composed music piece). Drones are 20-32 second loops with reverb tails.

**The rule:** Sound tells the stories geometry can't. Through-wall sounds create the presence of inaccessible lives. The hotel hums with other people's relationships.

### Per-Scene Acoustic Signatures

| Scene | Character | Key Sounds |
|-------|-----------|-----------|
| Taxi | Urban, enclosed, warm | Engine hum, tires on wet road, city ambient, driver voice |
| Hyperspace | Disorienting, overwhelming | Extreme frequency sweep, distortion, then sudden silence |
| Space Lobby | Vast, reverberant, wonder | Hull resonance, air circulation, marble footsteps |
| Corridor | Intimate, populated, alive | Through-wall piano, TV murmur, running water, parallel footsteps, your footsteps, Gibbons' footsteps |
| Suite | Intimate, warm, heartbeat | Clock tick (decelerating), hotel hum, through-wall sounds, champagne cork, water running |
| Observation Deck | Wind, void, exposure | Wind ambient, gusts, hull creaking, the silence between gusts |
| Paris Dream | Compressed, real, grounded | Rain on glass, traffic, compressed reverb, the sounds of a real city |
| Montage | Fragments, silence between | Three notes from the suite piece. Silence. Silence. Silence. |

### The Composed Music Piece

One piece. Once. During the bed ritual. The emotional center of the game.

**Direction:** NOT ambient. NOT sad. A Maxwell Young original — ambient on a drone foundation with a three-note melodic figure buried at ~60% emergence. The melody is always there. Lying in bed is the moment the player gets still enough to hear it.

**The three notes:** Write them first. A simple, unresolved figure — not a hook, not a resolution. Something that ends on a breath rather than a landing. Build the full piece around them. When the three notes return in the montage as a fragment, the player's body recognizes them before their brain does.

**The constraint:** Was already playing when you arrived. You just noticed. The drone foundation was always under the clock tick, under the hotel hum. The bed ritual is the moment you're still enough to hear it.

**Technical:** Non-looping. File-based (swap lighthouse.wav for the final track). 35% volume under clock tick + hotel hum. Three-note callback in montage.

### Through-Wall Audio (Commandment 6)

The hotel's invisible population. Lives you hear but never reach.

| Sound | Source | Scene | Meaning |
|-------|--------|-------|---------|
| Muffled nocturne | Room Four | Corridor | Someone plays every evening. First play only. |
| TV murmur | Room Six | Corridor | Goes silent if you stand near for 6 seconds. They heard you. |
| Running water | Room Six | Corridor | Bathroom sounds. Another life in parallel. |
| Footsteps above | Corridor ceiling | Corridor | Match your pace. 0.4s delay. First play only. |
| Phone ringing | Suite wall | Suite | Rings at 30s. Fades as you approach. Stops at 0.8m. |
| Distant voices | Corridor walls | Corridor | Ambient murmur. Other couples. Other lives. |
| Room service tray | Outside Room Six | Corridor | Two plates. One untouched. First play only. |

---

## XI. Visual Language — Materials, Light, Color

### Rendering Pipeline

```
1920×1200 RenderTexture (render_target)
  ├─ Shadow pass: depth-only FBO (512×512) from key light perspective
  ├─ Earth pass: procedural sphere behind scene geometry (space scenes)
  ├─ Scene pass: GLSL lighting shader
  │   ├─ Key light + fill light (directional)
  │   ├─ 4 point lights (per-scene positioned)
  │   ├─ PCF 3×3 shadow mapping
  │   └─ 13 procedural materials
  ├─ Dust motes / zero-g sparkles
  └─ HUD (spring-scaled crosshair + pixel icons)
        ↓
1920×1200 RenderTexture (postfx_target)
  └─ Post-FX shader:
     chromatic aberration, film grain, bloom, SSAO,
     dither, scanlines, vignette, warmth, saturation,
     contrast, exposure
        ↓
Window (nearest-neighbor upscale, aspect-ratio letterboxed)
```

### The 13 Procedural Materials

All computed from world-space UV + noise functions in GLSL. No texture files.

| ID | Material | Where It Lives | Character |
|----|----------|---------------|-----------|
| 0 | Concrete | Hull, ceilings, structure | Cold, industrial, honest |
| 1 | Marble | Lobby floor, bathroom | Luxurious, reflective, cold-warm |
| 2 | Wood | Furniture, paneling | Warm, domestic, alive |
| 3 | Carpet | Corridor, suite floor | Soft, sound-absorbing, hotel |
| 4 | Wallpaper | Corridor panels, suite walls | Domestic, pattern, enclosure |
| 5 | Tile | Bathroom, observation deck floor | Grid, precision, wet |
| 6 | Brass | Trim, fixtures, Gibbons' buttons | Warm metallic, status, time |
| 7 | Glass | Windows, observation deck shell | Transparent, boundary, fragile |
| 8 | Leather | Chairs, elevator handrail | Intimate, body, luxury |
| 9 | Fabric | Bed, robes, curtains | Soft, domestic, her |
| 10 | Checkerboard | Paris dream floor | Memory, pattern, Godard |
| 11 | Herringbone | Lobby accent | Pattern, status, craftsmanship |
| 12 | Parquet | Suite floor accent | Warm, domestic, real |

### Color Palette

Defined in `palette.h` as `PAL_*` constants.

**Foundation:** Neutral warm whites. Not yellow. Not grey. The warmth comes from specific accents, not a tint on everything.

**Accent colors:**
- **PAL_RED** — Godard red. Doors, the Paris dream's surviving color. Passion, memory.
- **PAL_BLUE** — French blue. Contrast to red. TV light under doors. Melancholy.
- **PAL_BRASS** — Warm gold. Trim, fixtures, Gibbons' uniform. Hotel luxury.
- **PAL_NAVY** — Deep blue-black. The void. Space. Absence.
- **PAL_CREAM** — Warm white. Wallpaper, linen, the hotel's surface warmth.

**Anti-pattern:** NEVER yellow-tint everything. NEVER grey-on-grey in space. NEVER desaturate for sadness. The palette is warm and bold. The void is beautiful.

### Visual Styles (Shift+1-9)

| Key | Style | Character |
|-----|-------|-----------|
| Shift+1 | 16mm Film | Default. Warm neutral, slight bloom, architectural. The game's voice. |
| Shift+2 | PS1 | Ordered dithering, color quantization, 2x pixels. Nostalgia. |
| Shift+3 | Noir | Near-mono, crushed blacks, heavy vignette, sharp. Anxiety. |
| Shift+4 | CRT | Scanlines, bloom, phosphor glow. Hotel TV energy. |
| Shift+5 | Godard | Saturated, contrasty, red push. The Paris dream's natural style. |
| Shift+6 | VHS | Heavy grain+CA, warm mud, scanlines. Memory deteriorating. |
| Shift+7 | Neon | Oversaturated, bloom heavy, teal-orange tint. The hotel at night. |
| Shift+8 | Woodcut | Extreme dither, near-mono, posterized. Abstraction. |
| Shift+9 | Raw | No post-FX. Naked geometry and lighting. Truth. |

### Lighting Presets

Each scene has a `LightingPreset_*()` function returning a `SceneLighting` struct. Key light direction + color, fill light direction + color, ambient level, 4 positioned point lights.

**Key emotional lighting:**
- **Lobby:** Earth-glow (blue-white) through glass + warm amber interior = wonder
- **Corridor:** Near-dark. Door spill only. Each door = a warm pool in darkness = other lives
- **Suite:** Warm, getting warmer. Warmth increases with tasks. The room holds you.
- **Observation Deck:** Cold space-light from above + warm suite-spill from behind = isolation + home
- **Paris Dream:** High contrast B&W. Window light. Film noir energy.

---

## XII. Technical Architecture

### Engine Overview

20,000+ lines of C99. Raylib for windowing, input, rendering primitives. Everything else custom: scene system, physics, audio synthesis, NPC, UI, lighting shader, post-FX shader.

### File Map

```
game_ctx.h          — GameCtx struct (ALL mutable state), SceneDesc typedef
ev_types.h          — Enums, structs (Wall, Scene, Player, PhysicsConfig, etc.)
config.h            — Constants (PI, SAMPLE_RATE, MAX_WALLS, task counts)
palette.h           — PAL_* color constants
scale.h             — 1920×1200 visibility dimensions

scene_registry.c    — const SceneDesc scene_descs[] (function pointers per state)
main.c              — Game loop, render pipeline, menu, transitions, debug
scene.c             — Geometry builders (build_lobby, build_space_suite, etc.)
render.c            — Rendering + post-FX shaders + draw helpers
lighting.c          — GLSL shader (embedded C strings), per-scene presets
audio.c             — 100% procedural audio synthesis
player.c            — Quake-style movement/physics
npc.c               — Gibbons NPC (waypoints, dialogue, drawing)
ui.c / ui.h         — Spring physics, icons, UI components
skybox.c            — GPU procedural skyboxes (5 types)

scene_splash.c      — STATE_TITLE + prologue
scene_taxi.c        — STATE_CAR, STATE_DRIVING, STATE_RETURN_TAXI
scene_exterior.c    — STATE_HOTEL_EXT
scene_lobby.c       — STATE_LOBBY
scene_elevator.c    — STATE_ELEVATOR
scene_hotel.c       — STATE_HALLWAY, STATE_ROOM, STATE_BATHROOM
scene_space_lobby.c — STATE_SPACE_LOBBY
scene_corridor.c    — STATE_SPACE_CORRIDOR
scene_suite.c       — STATE_SPACE_SUITE
scene_balcony.c     — STATE_BALCONY (→ observation deck)
scene_endings.c     — STATE_BED, STATE_STARS, STATE_HYPERSPACE, STATE_PARIS_DREAM
scene_montage.c     — STATE_MONTAGE (rapid-cut ending)
```

### Adding a New Scene

1. Add `STATE_NEW_SCENE` to the `GameState` enum in `ev_types.h`
2. Create `src/scene_newscene.c` with `newscene_load()` and `newscene_update(float dt)`
3. Add entry to `scene_descs[]` in `scene_registry.c`
4. Add geometry builder to `scene.c` / `scene.h` if needed
5. `make` — the Makefile wildcards pick up new .c files automatically

### State Machine

`load_state()` in main.c handles common reset (stop audio, clear state), then dispatches to the scene's `load()` function via the registry. Transitions use `transition_to()` (fade-to-black) or `hard_cut_to()` (Blendo-style instant cut with warm white flash).

```
TAXI → HYPERSPACE (+ title card) → SPACE_LOBBY (meet Gibbons)
→ ELEVATOR → GLASSHOUSE (the big reveal) → SPACE_CORRIDOR
→ SPACE_SUITE → BALCONY → PARIS_DREAM → CLEANED_SUITE → BED
→ MONTAGE → STARS → RETURN_TAXI → TITLE (loop)

The contraction: 20m wide → 4.5m wide → one room → one bed → one pillow
```

### The 3D Model Asset System

**Auto-loading**: Any `.glb` or `.obj` in `assets/` is auto-loaded at startup into `g.model_assets[]` (max 16). Lighting shader applied to all material slots. GLB animations loaded and ticked each frame.

**Models completed (12):** Gibbons, Sky Tower, bathtub, sofa, elevator car, piano, champagne glasses, bed, armchair, reception desk, chandelier, potted plant.

**Formats:**
- Static props (.obj): 50-200 tris, no textures, no animation
- Animated models (.glb): 200-800 tris (up to 5000 at 1920x1200), simple rigs
- No UV textures — GLSL materialId system handles surfaces procedurally
- Scale: 1 unit = 1 meter. GLB exports Y-up.

### The Gehry Shell System

For organic/deconstructivist architecture where code-placed boxes won't cut it.

**Architecture — visual/collision split:**
- **Visual**: GLB mesh from Blender → `add_shell()` — renders with material, no collision
- **Collision**: Invisible AABB boxes → `add_collision_wall()` — physics only, not rendered
- **Furniture**: Code geometry inside the shell — `add_wall()`, `add_cylinder()`, etc.

**Engine functions:**
```c
add_shell(s, "name", x,y,z, sx,sy,sz, rot, mat, color);   // visual only
add_collision_wall(s, x,y,z, w,h,d);                        // physics only
add_collision_floor(s, x,y,z, w,d);                          // floor shorthand
add_collision_ceiling(s, x,y,z, w,d);                        // ceiling shorthand
```

**Blender workbench:** `scripts/ev_shell_workbench.py` on Mac Mini (ssh mini-ts, port 9877).

**Pattern:** Gehry buildings have crazy shells but regular interiors. The titanium swoops are the architecture; inside, you still place furniture with code. Shell = visual envelope. Code = everything inside.

### Build System

```bash
make              # Incremental build
make run          # Build and run
make clean        # Remove object files + binaries
make dev SCENE=STATE_SPACE_SUITE   # Skip title, boot into a scene
make qa           # QA mode: automated screenshots + pixel analysis
make check        # Static analysis
make test         # Headless unit tests
make count        # Line counts per file
make release      # Stripped -O3 binary
make fmt          # clang-format all source
make dev-watch    # Auto-rebuild + restart on source changes
```

---

## XIII. The Blender Pipeline

### Philosophy

Model only when silhouette or narrative weight demands it. If a box says it better, keep the box. No UV textures — materialId system handles surfaces.

### Infrastructure

- **Blender** runs on Mac Mini (ssh mini-ts, Tailscale: 100.124.74.30)
- **BlenderMCP** on port 9877 (raw TCP socket)
- **Style bible:** `assets/blender/ev_style.py` — locked materials, lighting, camera. Run FIRST every session.
- **Model toolkit:** `scripts/ev_model_kit.py` — shared form primitives, validation gates, auto-export.
- **QA pipeline:** `glb_qa.sh` → Three.js viewer → automated 6-criteria scoring → screenshot overlay.

### Model Pipeline

1. **Reference first.** Find a real-world photo. Study proportions.
2. **Self-contained script.** Each model has its own .py build script.
3. **Style bible first.** `exec(open('ev_style.py').read())` before anything.
4. **Build → Score → Iterate.** QA screenshots at 4 angles. Score threshold ratchets per version.
5. **Export GLB.** Y-up. 1 unit = 1 meter.
6. **SCP to engine.** `scp mini-ts:~/model.glb assets/`
7. **Auto-loaded.** `make run` picks it up.

### Model Budget (1920×1200)

| Category | Tri Budget | Examples |
|----------|-----------|---------|
| Props | 500-1500 | Champagne glasses (780), piano (620) |
| Furniture | 1000-3000 | Bed (2840), armchair (912), sofa (1628) |
| Architectural | 500-2000 | Chandelier (1032), elevator car (468) |
| Characters | 1000-3000 | Gibbons (animated GLB) |
| Shells | 3000-5000 | Observation deck (planned) |

### Remaining Models

**Priority (narrative objects):**
- [ ] Wine glass with lipstick mark
- [ ] Telephone
- [ ] Photograph frame (turnable)
- [ ] Her book (with boarding pass)

**Priority (observation deck shell):**
- [ ] Glass envelope — Gehry curves, steel ribs
- [ ] First real shell system validation

**Future:**
- [ ] Additional chairs/furniture
- [ ] Floor lamp
- [ ] Steering wheel (taxi)

---

## XIV. Development Phases — Revised

### Phase 1: The Suite (Depth) — IN PROGRESS
**Goal:** Make the Space Suite the best room in any walking simulator.

- [x] Room-for-two layout — bed, two pillows, two robes, two glasses, her book, photograph
- [x] Presence zone system — dwell timers + E-press fallback
- [x] Wrongness progression — 4-stage shifts per task
- [x] Silent objects — wardrobe, wine glass, book, sock, phone, suitcase, bath
- [x] Diegetic text — ticket, photograph, room service card, postcard, guest book, newspaper
- [x] Per-position speed curves — slow near window, soft near bed
- [ ] Cork pop sound (procedural)
- [ ] The empty glass persistence — champagne poured into one, second stays empty forever
- [ ] The adjoining door — second room, booked, empty, optional
- [ ] Bed ritual — full agency surrender, composed music trigger, hold
- [ ] Cleaned suite variant — `build_space_suite_cleaned()` — one of everything

### Phase 2: The Glasshouse (The Arrival Reveal) — NEXT
**Goal:** Build the first big space. The Bioshock Infinite moment. The WALL-E scale.
**Gate:** Phase 1 must be playable end-to-end before this begins. The suite is the game. The glasshouse is the spectacle. Don't let spectacle block substance. The shell system hasn't been validated e2e yet — the first task here is a test shell, not the final design.

- [ ] Shell system e2e validation — test shell in Blender → GLB → engine → collision → playable
- [ ] Blender shell model — glass envelope, steel ribs, asymmetric Gehry curves
- [ ] Collision volumes — floor, back wall, side boundaries
- [ ] Earth view — Rothko layers visible through glass, city lights, atmosphere
- [ ] Furniture in code — seating clusters for couples, evidence of presence (coats, drinks, books)
- [ ] Gibbons pathfinding through the space — he leads, doesn't stop
- [ ] No player interaction — this is a pass-through, not a habitable space (yet)
- [ ] "Twos" planted at scale — every table for two, every cluster a pair
- [ ] Audio: vast reverb, hull resonance, distant guest sounds, air circulation
- [ ] Montage shot — the same space, emptied, two chairs alone
- [ ] Return playthrough variant — fewer coats, fewer drinks, hotel emptying

### Phase 3: The Corridor + Elevator (Pacing)
**Goal:** The journey to the suite should feel like a narrative.

- [x] Corridor geometry — 8-segment curved arc, 3 doors, portholes
- [x] Through-wall audio — piano, TV, running water, parallel footsteps
- [x] Gibbons pathfinding and dialogue
- [x] Door-listening mechanic — TV goes silent after 6s proximity
- [x] Spatial compression — velocity increase toward exit
- [x] Second playthrough: shorter corridor, missing sounds
- [ ] Elevator scene — warm amber, Chevalier energy, Gibbons
- [ ] Gibbons glance — looks behind you once in the lobby

### Phase 4: The Paris Dream (The Memory)
**Goal:** Build the mid-suite hard cut. 60-90 seconds.

- [ ] Rebuild Paris scenes in B&W — compressed geometry, dream logic
- [ ] Her presence — coat, coffee, pillow indent, steam, toothbrush
- [ ] The book — same novel, different page
- [ ] Godard red — one color object survives the B&W
- [ ] Memory window — Eiffel Tower / Sky Tower / neither
- [ ] Hard cut in — no transition, instant
- [ ] Hard cut back — same suite position, same empty glass

### Phase 5: The Ending (Rapid Cuts)
**Goal:** Build the Thirty Flights montage.

- [x] Montage system — fixed camera positions per scene, timed cuts
- [x] Jump cut system — instant scene transitions
- [ ] The cleaned room — suite reset shot
- [ ] Full montage sequence (13 shots)
- [ ] Single-frame cuts for peak intensity
- [ ] Gibbons in the taxi — going home, eye contact
- [ ] The photograph face up — she's laughing, hold
- [ ] Final void — empty glass, silence, three notes, credits

### Phase 6: The Composed Music
**Goal:** Write the three notes. Build the piece. Ship the heart.

- [ ] Write the three-note figure — unresolved, ends on a breath
- [ ] Build the drone foundation — was always playing, you just noticed
- [ ] Record the full piece — Maxwell Young original
- [ ] Integrate at 35% volume under clock tick + hotel hum
- [ ] Three-note callback in montage
- [ ] Test: drop the track at 35% with clock tick. The room will tell you.

### Phase 7: Polish & Release
**Goal:** Ship. Short and dense, not long and sprawling.

- [ ] Playtesting — 5-10 people, watch silently, note pauses/rushes/confusion
- [ ] Does the player understand by the end? Without being told?
- [ ] Performance — locked 60fps, 1920×1200
- [ ] The observation deck performance — largest scene, needs optimization
- [ ] Platform targets — macOS first, then Windows/Linux via Raylib
- [ ] Runtime target — 15-30 minutes first playthrough. If longer, cut.
- [ ] Store page — "Three hours to kill." One screenshot. One sentence.

---

## XV. What Not to Do

These are temptations. Resist them.

- **Don't add combat, puzzles, or fail states.** The game is about inhabiting a space, not conquering it.
- **Don't add a map or minimap.** Architecture guides the player. If they get lost, the architecture is wrong.
- **Don't add collectibles.** Objects exist to tell stories, not to be collected.
- **Don't add dialogue.** If the game ever uses voice, it should be located (phone, radio, PA system), unreliable, and contradicted by the space. Never omniscient narration. Never "she" or "we" spoken aloud.
- **Don't explain what happened.** The player doesn't need to know. The room tells them someone was supposed to be here. That's enough.
- **Don't show her.** She exists through objects. The moment you render her you collapse the player's version into yours.
- **Don't make it melodramatic.** No tears. No dramatic music swells. No rain-on-window-while-sad. The champagne glass is empty. That's the drama. Restraint.
- **Don't add more scenes to increase runtime.** Add depth to existing scenes instead.
- **Don't chase realism.** 1920×1200 with procedural textures IS the aesthetic.
- **Don't explain the hotel.** Why is there a hotel in orbit? The mystery is the point.
- **Don't explain the Paris dream.** It's a memory. Memories don't announce themselves.
- **Don't make the hotel small.** The observation deck is enormous. The suite is generous. Agoraphobia, not claustrophobia.
- **Don't make Gibbons creepy.** He's warm, formal, dignified. Kindness is the only dignified response.
- **Don't make the absence horror.** It's a breakup. The most common tragedy. That's why it works.
- **Don't over-read your own influences.** Reference Blendo and Godard without pretending to channel them. Maybe you just like making hotels.
- **Don't use creepy ambient drones.** If it sounds scary, it IS scary. Wonder, not dread.
- **Don't make tiny detail objects.** If it's not 3+ pixels at 1920×1200, scale it up or remove it.
- **Don't reuse Paris audio for space.** Space needs its own acoustic character.
- **Don't confirm emotions with text.** "She would have loved this" — NEVER. The two chairs say it. Don't say it for them.
- **Don't run parallel agents on the same files.** Sequential only. Learned the hard way.

---

## XVI. The North Star

When everything else is noise, return to this:

**The game is one night in an impossible hotel. You came here with someone. She's not here. The suite was booked for two. You pour one glass of champagne. You lie in a bed made for two. A memory of Paris interrupts — her coat on the chair, her toothbrush in the glass — and then you're back. The game fractures into rapid cuts. Every pair becomes a single. Gibbons reappears in the taxi, already leaving. The hotel has already cleaned the room. One pillow. One robe. One glass. The void remains.**

Everything in the game serves that sequence. Everything that doesn't serve it gets cut.

Every second earned.
