# Brendon Chung / Blendo Games -- Reference Brain

Research compiled for game development reference. All information sourced from interviews, GDC talks, critical analysis, and Chung's own statements.

---

## 1. Brendon Chung as Designer

### Background
- Before Blendo, Chung was a **level designer at Pandemic Studios** (worked on The Lord of the Rings: Conquest and Full Spectrum Warrior: Ten Hammers)
- Had been doing hobbyist modding for **about a decade** before Pandemic hired him -- Doom and Quake mods going back to grade school
- When EA shut down Pandemic in **November 2009**, Chung went independent and began work on Flotilla
- Founded Blendo Games in **2010**
- Co-founded **Glitch City**, a Los Angeles indie game collective in Culver City (est. 2013) -- a shared physical workspace for indie devs
- Based in LA, streams development on YouTube/Twitch with detailed process breakdowns

### Key Influences (Cited by Chung)
- **Donald Norman** -- *The Design of Everyday Things*
- **Scott McCloud** -- *Understanding Comics*
- **Liz England** -- "The Door Problem"
- **The Doom Bible** and **Bungie's Marathon** (inspirations for the Barista series)
- **Wong Kar-wai** -- Fallen Angels (1995), Chungking Express (1994) -- "dreamlike tone and logic," people acting in ways that are "compelling and interesting" rather than natural
- **Jean-Luc Godard** -- Pierrot le Fou (1965) -- non-continuous editing, expressionistic style
- **Godfrey Reggio** -- Koyaanisqatsi (1982), Baraka -- poetic slow-motion, sound design; the gunfight scene in Thirty Flights was meant to have a "musical rhythm" inspired by these films
- **Disney's TaleSpin** -- characters from The Jungle Book in a 1920s alt-future sci-fi setting with janky seaplanes; Chung loved the "beautiful, weird, interesting" genre mashup
- **1990s independent cinema boom** -- Robert Rodriguez, Coen brothers, Richard Linklater, Wong Kar-wai; low-budget filmmaking with limited resources

### Design Formative Texts
- The Doom Bible
- Understanding Comics (McCloud)
- The Design of Everyday Things (Norman)

---

## 2. Blendo Games Studio

### How It Operates
- **Primarily a one-person operation**: Chung does all design, art, programming, and business
- Skin Deep (2025) was a notable exception -- collaborative team effort
- Uses **bastardized Kanban board** and **Pomodoro technique** for workflow
- Works from a **standing desk** (Ikea Jerker + anti-fatigue mat) -- "standing up gives just enough discomfort to encourage getting tasks done, as opposed to dilly-dallying"

### Tools (from Uses This interview)
- Intel i5 desktop, 22" Gateway monitor
- Wacom Intuos3 tablet for digital art
- Logitech Z-2300 speakers
- Microsoft Visual C++ 2010 (for Quadrilateral Cowboy)
- idTech engine family (Quake 2/KMQuake II for early work, idTech 4 for QC)

### Development Philosophy
- Streams development publicly -- provides feedback loop and forces accountability
- Open source ethos: released source code for Quadrilateral Cowboy, Gravity Bone, and Thirty Flights of Loving under GPL
- Source code for QC: compact 12MB of C++

### Complete Gameography
1. **Barista series** (mods, 4 games) -- Doom II mod and standalone, experimental FPS design
2. **Citizen Abel** series (Quake 2 mods) -- episodic spy fiction
3. **The Puppy Years** (Half-Life mod)
4. **Gravity Bone** (2008) -- freeware, Quake 2 engine, Citizen Abel universe
5. **Flotilla** (2010) -- space combat, semi-random story events
6. **Air Forte** (2010) -- edutainment (math, geography, vocabulary via plane flying)
7. **Atom Zombie Smasher** (2011) -- real-time strategy, zombie evacuation
8. **Thirty Flights of Loving** (2012) -- first-person narrative, Citizen Abel universe
9. **Quadrilateral Cowboy** (2016) -- hacking/puzzle, Citizen Abel universe
10. **Skin Deep** (2025) -- FPS, espionage on a spaceship, first team project

---

## 3. Design Philosophy

### Cinematic Grammar in Games
Chung's central innovation is bringing **film editing grammar** -- specifically the **cut** -- into first-person interactive spaces. Not cutscenes. Actual temporal jumps while the player retains first-person control.

> The editing here is the cut, the temporal cut, while you're controlling your character in a first-person perspective.

In Thirty Flights of Loving, jump-cuts **dramatically change the mood and tone** of the present scene in an instant, simulating a fractured timeline. The player is jolted forwards and backwards through pieces of a story.

Key technique: **exposing the player only to the build-up or aftermath of pivotal action sequences, never the sequences themselves**. The heist, the gunfight, the betrayal -- all happen off-screen. You see only the before and after.

### Pacing and Editing
- Barista 2 was where Chung first experimented with "cutting away from the player's point-of-view to go somewhere else and travel across space and time"
- This cutting technique worked around the constraints of scripted sequences in first-person games
- Film grammar applied: **establishing shots, match cuts, temporal jumps, ellipsis**
- The non-linearity is not player-driven branching -- it is **authored montage**, closer to how a film editor sequences footage

### Subtractive Design
Chung does not use this exact term, but his practice is textbook subtractive:

**Atom Zombie Smasher case study (GDC 2012: "Nuke It From Orbit")**:
- Originally had a complex over-world system: managing district populations, building defenses, convoluted rules
- When he plotted it as a physical board game, realized it "would require like a million monopoly pieces"
- **Stripped it to bare essentials**: simple map, resource management, stage selection
- After removing the "tumorous growth," the game clicked -- "fewer moving parts, and those parts that remained worked well"

**Broader pattern across all games**:
- No HUD in Gravity Bone (weapon slot conspicuously empty)
- No voice-over in Thirty Flights of Loving
- No dialogue in most games
- No combat in Gravity Bone's first mission
- No explicit exposition -- ever

### Game Length and Respecting Player Time
- Gravity Bone: ~15 minutes
- Thirty Flights of Loving: ~13 minutes
- These are complete, satisfying experiences -- not demos or fragments

> PC Gamer: Thirty Flights of Loving "tells a better story in 13 minutes than most games do in 13 hours"

The philosophy: as the gaming audience grows older, games compete for limited free time. A complete experience in one sitting is a feature, not a limitation. Short does not mean incomplete.

### Lo-Fi Aesthetics as Deliberate Choice
- Gravity Bone's blocky art direction was so strong it made the antiquated Quake 2 engine "a moot point"
- Quadrilateral Cowboy was built on modern idTech 4, but "its blocky heart belongs to Quake II" -- the lo-fi look persisted by choice, not limitation
- The cubist, geometric character style (no faces, block hands) is a **signature**, not a compromise
- Connects to his DOS-era computing nostalgia: "There was something crunchy about that experience" -- he wanted to "introduce that experience to people who have never been through that gross technical hell"

### Environmental Storytelling
- No text popups, no voice-over, no cutscenes (in the traditional sense)
- World communicates everything: objects, architecture, spatial relationships
- Chung: he won't read text popups but **will** read the back of cereal boxes -- environmental text must feel authentic, not like game UI
- Players are skilled at recognizing when environmental text design "looks wrong" because they've been surrounded by real product design their entire lives
- In Thirty Flights, the criminal nature of the group is communicated entirely through environment design -- bridging "the disconnect between the player's knowledge and the player's character's knowledge"
- Builds clear rules for text: what's critical vs. incidental, what's skippable, establishing conventions

---

## 4. Cinematic Influences (Deep Dive)

### Wong Kar-wai (Primary Influence)
- Chung watched WKW extensively in high school during the 1990s indie cinema boom
- Key films: **Fallen Angels**, **Chungking Express**, **Days of Being Wild**
- What he takes from WKW: dreamlike tone and logic, unexpected tonal shifts, vibrant color palettes, fragmented narratives involving assassins and romance
- Xavier Cugat tracks used in Gravity Bone are drawn from WKW's soundtracks (e.g., "Maria Elena" from Days of Being Wild)

### Jean-Luc Godard / French New Wave
- **Pierrot le Fou** (1965) -- non-continuous editing, expressionistic style
- Jump cuts as grammar, not error -- the foundational technique of Thirty Flights
- Meaning produced by the **conflict between two different images**, not continuity of two similar ones (Soviet montage lineage)

### Godfrey Reggio (Koyaanisqatsi / Baraka)
- Poetic slow-motion sequences and sound design
- The gunfight scene in Thirty Flights of Loving was specifically designed to have a "musical rhythm" inspired by these films
- Non-narrative cinema: images and sound creating meaning without dialogue

### Broader Film DNA
- The spy genre itself: 1960s Bond aesthetic filtered through South American magical realism
- Film noir atmosphere in Gravity Bone's environments
- The overall approach: games as **short films**, not feature-length blockbusters

---

## 5. Technical Approach

### Engine History
- **Quake 2 / KMQuake II** -- Gravity Bone, early Citizen Abel mods
- **idTech 4** (Doom 3 engine, modified) -- Quadrilateral Cowboy, Thirty Flights of Loving
- Chose older/open engines for: familiarity (decade of Quake modding), open-source availability, constraint-driven design

### Why Older Engines
- Chung's entire career grew from modding Quake 2 and Doom -- these tools are native to him
- Technical constraints produced his aesthetic -- but the aesthetic persisted even when constraints were lifted (QC on idTech 4 still looks like Quake 2)
- The lo-fi constraint forces creative solutions: can't rely on visual fidelity, must communicate through design, color, composition, spatial rhythm
- Released source code under GPL for community use -- believes in open tooling

### Constraint as Design Driver
- DOS-era computing philosophy: "crunchy," deliberate friction
- In Quadrilateral Cowboy, chose "older, clunkier technology" over the "current trend of accessible hacking interfaces"
- Typing as the most "direct analog to the feel of hacking" -- keyboards as primary interface
- Constraint produces specificity, specificity produces style

---

## 6. Key Interviews and Talks

### GDC 2015: "Level Design in a Day: Wayfinding & Storytelling Techniques"
- Available on Internet Archive and GDC Vault
- Draws on Gravity Bone, Thirty Flights, Quadrilateral Cowboy
- Topics: conveying information through architecture, spatial navigation techniques, common level design pitfalls, narrative through environment
- URL: https://archive.org/details/GDC2015Chung

### GDC 2012: "Nuke It From Orbit: The Making of Atom Zombie Smasher"
- Postmortem on AZS development
- Key lesson: cutting the "tumorous" over-world system
- Discusses prototyping, procedural generation, and knowing when to kill your darlings
- GDC Vault: https://gdcvault.com/play/1015688/Nuke-It-From-Orbit-the

### Designer Notes Podcast (Soren Johnson), Episodes 46-47, 2019
- Two-part deep interview
- Part 1: making adventure games inside Doom, cereal box readability vs. text popups, the ending of Gravity Bone
- Part 2: Xbox Live Indie Games, the ending of Atom Zombie Smasher, how streaming development made him work harder
- URLs:
  - https://www.idlethumbs.net/designernotes/episodes/brendon-chung-part-1
  - https://www.idlethumbs.net/designernotes/episodes/brendon-chung-part-2

### "Love in the Time of Shooters" -- Nightmare Mode (2012)
- Conversation around the time of Thirty Flights of Loving's release
- URL: https://nightmaremode.thegamerstrust.com/2012/09/07/love-in-the-time-of-shooters-a-conversation-with-brendon-chung/

### "Player Two" -- Invalid Memory (2020)
- Comprehensive career interview
- Covers early mods through Quadrilateral Cowboy
- URL: https://invalidmemory.wordpress.com/2020/03/17/player-two-an-interview-with-brendon-chung/

### Road to the IGF Interviews (Game Developer / Gamasutra)
- Atom Zombie Smasher: https://www.gamedeveloper.com/design/road-to-the-igf-blendo-games-i-atom-zombie-smasher-i-
- Quadrilateral Cowboy: https://www.gamedeveloper.com/design/road-to-the-igf-blendo-games-i-quadrilateral-cowboy-i-

### Uses This (Tools Interview)
- Hardware/software setup
- URL: https://usesthis.com/interviews/brendon.chung/

### VentureBeat: "The Hard Drive is the Scene of the Crime" (2014)
- Interview about Quadrilateral Cowboy
- URL: https://venturebeat.com/community/2014/06/03/the-hard-drive-is-the-scene-of-the-crime-an-interview-with-brendon-chung-on-quadrilateral-cowboy/

### Kotaku: "Blendo Games Has Been At It For 10 Years" (2020)
- Career retrospective
- Key quote: Chung's output "helped move games forward" through "subversions" that are also "about loving games... poking at them and remixing them into new shapes"
- URL: https://kotaku.com/blendo-games-has-been-at-it-for-10-years-and-has-consi-1840794372

### IndieCade Retrospective
- Panel/event covering full Blendo history
- URL: https://anywhere.indiecade.com/events/retrospective-blendo-games/

---

## 7. The Citizen Abel Universe

### Shared Fiction
The Citizen Abel games form a **shared fictional universe** spanning multiple games and engines:

1. **Citizen Abel** (Quake 2 mods, episodic) -- intergalactic contract work, comedic tone, built the foundational mythos
2. **The Puppy Years** (Half-Life mod) -- same character/universe
3. **Gravity Bone** (2008) -- set in **Nuevos Aires**, a fictional nondescript South American locale; Citizen Abel is a secret agent performing bizarre missions (feeding an insect to a partygoer, photographing black birds)
4. **Thirty Flights of Loving** (2012) -- **prequel** to Gravity Bone; Abel's assassin from Gravity Bone appears as a passive NPC; heist-gone-wrong narrative
5. **Quadrilateral Cowboy** (2016) -- same universe, extends into hacker/heist territory

### Universe Characteristics
- Spy fiction filtered through magical realism and absurdism
- 1960s aesthetic overlay on a vaguely timeless world
- Blocky, faceless characters across all entries
- Xavier Cugat music as recurring sonic texture
- Recurring themes: espionage, betrayal, the mundane logistics of criminal work
- Characters recur across games in unexpected roles

### Narrative Approach
- No continuity in the traditional sense -- more like a shared **atmosphere** and **tonal universe**
- Each game stands alone but rewards knowledge of the others
- Gravity Bone was originally intended as a direct sequel to the Citizen Abel Quake 2 mods but became "very different in tone"
- The universe allows Chung to explore different genres (spy adventure, heist narrative, hacking puzzle) while maintaining a consistent aesthetic and emotional register

---

## 8. Quadrilateral Cowboy

### Core Design
- **Cyberpunk 1980s setting** -- player is a computer hacker coordinating heists
- Won **Seumas McNally Grand Prize** (IGF 2017) and **Excellence in Design**
- Also won **Grand Jury Award** at IndieCade 2013
- Built on idTech 4 engine; source code released under GPL

### How It Extends Chung's Philosophy

**Player expression through mechanics**: "There's something really special that happens when you let people express themselves through the game world and game items and mechanics." Players write actual code to solve puzzles -- no pre-defined solutions.

**Typing as hacking feel**: "Let's see what we can do with keyboards" -- chose typing as "the most direct analog to the feel of hacking." This is the opposite of the minigame approach to hacking in most games.

**Failure spectrum (inspired by Thief)**: "Largely inspired by how Thief does the failure spectrum -- if a guard sees you and alerts the entire castle, the mission does not end -- it keeps on going. Instead of it being instant failure, the mission parameters just change."

**DOS nostalgia as design pillar**: "The current trend of hacking games, interfaces, UI, and web design is made to be extremely accessible." Chung "wanted to explore the older, clunkier technology." Growing up using DOS, "There was something crunchy about that experience" and he "wanted to try to introduce that experience to people who have never been through that gross technical hell."

**Sandbox problem-solving**: Let the player "experiment in a sandbox and figure out their own solutions to problems."

### Same Universe
- Takes place in the Citizen Abel universe
- Extends the heist/criminal theme into a more systems-driven, puzzle-oriented format
- Retains the signature lo-fi cubist aesthetic

---

## 9. Relevance to Dante / EV Engine

Lessons from Chung applicable to this project:

- **Subtractive design**: Strip to essentials. If it would require "a million monopoly pieces," cut it.
- **Constraint as style**: Lo-fi is not a limitation. It is the look. Commit to it.
- **Environmental communication over UI**: If you can say it with architecture, don't say it with text.
- **Authored pacing**: Short, complete, satisfying. Respect the player's time.
- **Cinematic grammar**: Jump cuts, ellipsis, and montage are viable in interactive spaces -- not just in cutscenes.
- **Tonal consistency across variety**: The Citizen Abel universe shows how a consistent aesthetic and mood can hold together wildly different game genres.
- **Technical constraints produce specificity**: Old engines force creative solutions. Limitations are features.

---

## Sources

- [Blendo Games Official Site](https://blendogames.com/about.htm)
- [Blendo Games -- Wikipedia](https://en.wikipedia.org/wiki/Blendo_Games)
- [Player Two: An Interview with Brendon Chung -- Invalid Memory](https://invalidmemory.wordpress.com/2020/03/17/player-two-an-interview-with-brendon-chung/)
- [Brendon Chung's Gravity Bone and Thirty Flights of Loving -- 050nor Substack](https://050nor.substack.com/p/brendon-chungs-gravity-bone-and-thirty-flights-of-loving)
- [Assignment: Thirty Flights of Loving and Cinematic Language](https://intermittentmechanism.blog/2016/12/06/assignment-thirty-flights-of-loving/)
- [On Thirty Flights of Loving -- This Cage is Worms](https://thiscageisworms.com/2012/09/10/on-thirty-flights-of-loving/)
- [Narrative Analysis: Thirty Flights of Loving -- Shape of Play](https://shapeofplay.wordpress.com/2013/02/21/thirty-flights-of-loving-analysis/)
- [Thirty Flights of Betrayal -- The Half-Way Tree](https://thehalfwaytree.wordpress.com/2016/08/18/thirty-flights-of-betrayal-an-analysis-of-the-themes-in-thirty-flights-of-loving/)
- [GDC 2015: Wayfinding & Storytelling Techniques -- Internet Archive](https://archive.org/details/GDC2015Chung)
- [GDC 2012: Trimming Atom Zombie Smasher -- Game Developer](https://www.gamedeveloper.com/design/gdc-2012-trimming-down-i-atom-zombie-smasher-i-s-tumorous-design)
- [GDC Vault: Nuke It From Orbit](https://gdcvault.com/play/1015688/Nuke-It-From-Orbit-the)
- [Designer Notes 46: Brendon Chung Part 1](https://www.idlethumbs.net/designernotes/episodes/brendon-chung-part-1)
- [Designer Notes 47: Brendon Chung Part 2](https://www.idlethumbs.net/designernotes/episodes/brendon-chung-part-2)
- [Love in the Time of Shooters -- Nightmare Mode](https://nightmaremode.thegamerstrust.com/2012/09/07/love-in-the-time-of-shooters-a-conversation-with-brendon-chung-on-quadrilateral-cowboy-game)
- [Uses This / Brendon Chung](https://usesthis.com/interviews/brendon.chung/)
- [Gravity Bone -- Wikipedia](https://en.wikipedia.org/wiki/Gravity_Bone)
- [Gravity Bone -- From Quake 2 Mod to Independent Studio](https://dondeq2.com/2020/03/17/gravity-bone-by-brendon-chung-from-quake-2-mod-to-independent-games-development-studio/)
- [Gravity Bone Was a Better Spy Movie -- PC Gamer](https://www.pcgamer.com/gravity-bone-was-a-better-spy-movie-than-most-spy-movies/)
- [Thirty Flights of Loving -- Wikipedia](https://en.wikipedia.org/wiki/Thirty_Flights_of_Loving)
- [Thirty Flights Tells a Better Story in 13 Minutes -- PC Gamer](https://www.pcgamer.com/thirty-flights-of-loving-does-more-with-story-in-13-minutes-than-most-games-do-in-13-hours/)
- [Quadrilateral Cowboy -- Wikipedia](https://en.wikipedia.org/wiki/Quadrilateral_Cowboy)
- [Q&A: Creating Quadrilateral Cowboy -- Game Developer](https://www.gamedeveloper.com/design/q-a-creating-the-hacker-cyberpunk-game-i-quadrilateral-cowboy-i-)
- [Brendon Chung Explains Quadrilateral Cowboy -- Inverse](https://www.inverse.com/article/18884-quadrilateral-cowboy-brendon-chung-interview)
- [Brendon Chung on QC and the Joys of Programming -- Kill Screen](https://www.killscreen.com/brendon-chung-quadrilateral-cowboy-and-joys-programming/)
- [TaleSpin Inspired His New Heist Game -- Vice](https://www.vice.com/en_us/article/ppvdjv/developer-brendon-chung-on-quadrilateral-cowboy-game)
- [The Curious Case of Quadrilateral Cowboy -- Engadget](https://www.engadget.com/2016-04-29-quadrilateral-cowboy.html)
- [Quadrilateral Cowboy Takes Grand Prize at IGF 2017 -- Game Developer](https://www.gamedeveloper.com/business/-i-quadrilateral-cowboy-i-takes-grand-prize-at-the-2017-igf-awards)
- [Quadrilateral Cowboy Source Code Released -- PC Gamer](https://www.pcgamer.com/quadrilateral-cowboys-source-code-has-been-released-to-the-public/)
- [Skin Deep -- Engadget](https://www.engadget.com/skin-deep-brendon-chung-new-fps-steam-annapurna-192141277.html)
- [Blendo Games 10 Years Retrospective -- Kotaku](https://kotaku.com/blendo-games-has-been-at-it-for-10-years-and-has-consi-1840794372)
- [IndieCade Retrospective: Blendo Games](https://anywhere.indiecade.com/events/retrospective-blendo-games/)
- [Glitch City -- Game Developer](https://www.gamedeveloper.com/business/we-built-glitch-city-lessons-learned-by-la-s-indie-dev-collective)
- [Gravity Bone -- TV Tropes](https://tvtropes.org/pmwiki/pmwiki.php/VideoGame/GravityBone)
