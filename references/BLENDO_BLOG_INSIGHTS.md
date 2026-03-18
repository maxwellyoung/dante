# Blendo Games Blog Insights for EV Engine

Extracted from [blendogames.com/news](https://blendogames.com/news/) — filtered for relevance to Endearing Void development.

---

## 1. The 80/20 Art Density Rule

**Source:** [Skin Deep: Art Density](https://blendogames.com/news/post/2025-07-02-skindeep_art/) (Jul 2025)

Chung defines art density as "how busy or detailed something is." His guideline: **80% flat undetailed surfaces, 20% tight pockets of detail.** Not literal measurements — a philosophy.

### Key Principles
- Low density = production sustainability + gameplay clarity + stylistic continuity
- High-density details "should be used very sparingly"
- Detail concentrates at edges: "paint chipping, light fixtures on the perimeter"
- **Greyscale blur test**: convert to greyscale, apply blur — if something still screams for attention, it's too busy for a background surface
- Wallpaper iteration: an ornate botanical pattern proved "too attention-grabbing" for a wall. Toned down while keeping the direction.
- Exceptions are fine: "sometimes it makes sense to bend the rules, sometimes you just kinda run out of time, sometimes it's fun to just do whatever"

### EV Application
At 960×600, this is even more critical. The procedural materials (concrete, marble, wood) are the 80%. Brass fixtures, light pools, interactive objects are the 20%. The space suite should feel mostly calm surface with specific focal points.

---

## 2. Storyboarding as Communication Tool

**Source:** [Skin Deep: Storyboards](https://blendogames.com/news/post/2025-06-30-skindeep_storyboards/) (Jun 2025)

Storyboards are created "when needed, as a tool to express specific ideas" — not for the entire game.

### Workflow
1. Call with stakeholders — review scripts and implementation ideas
2. Text documentation — Google Docs describing how sequences function
3. **Blockout construction** — intentionally rough scripted version, rapid iteration
4. Final brushwork — detailed version once direction solidifies

### Cinematic Techniques Used
- **Match cuts** — visual transitions between scenes (gantry/door alignment)
- **Freeze-frame effects** — including zoom techniques from the 2014 Wolfenstein trailer
- **Split-screen** — proposed but cut due to technical constraints
- **Ragdoll physics** — evolving implementations based on feasibility

### Key Quote
> "A hazy 10-minute discussion that was all words usually became a crystal clear 3-minute discussion if you just drew a little sketch."

### EV Application
The hard cuts in EV (taxi → exterior → lobby) are essentially match cuts and temporal jumps. The blockout-first approach validates building scenes rough then refining. Match cuts between the elevator doors closing → hyperspace → space lobby doors opening.

---

## 3. The Thirty Flights Prototype & Brain Percolation

**Source:** [Thirty Flights of Loving prototype](https://blendogames.com/news/post/2013-10-01-thirty-flights-of-loving-prototype/) (Oct 2013)

The prototype emerged around 2008 during Gravity Bone development. Early experiments included "grappling hooks, hacking minigames, handgrenade trick throws, granular door opening." One prototype explored "the idea of cutting and editing" — which became Thirty Flights.

### Key Insight: The Drawer
When development resumed in 2013 (~5 years later), the gap was beneficial:
> "Take an all-new perspective, gained months' worth of brain percolation, and will have let go of some preciousness and preconceptions about the work-in-progress."

> "The common adage is 'kill your babies,' but I'd append to that: **'and keep 'em in your drawer.'**"

### Gun as Depth-of-Field Test
The prototype included a gun "mostly because I wanted to see how well I could fake some depth-of-field effects." The gun model was "a couple of quad planes." Lo-fi experimentation driving visual innovation.

### EV Application
The orphaned Paris hotel scenes (hallway, room, bathroom) are in the drawer. The granular door opening prototype became the real thing in QC — EV's interaction system (E-press = visible consequence) comes from this lineage.

---

## 4. NPC Brain Architecture

**Source:** [Skin Deep: Enemy Brains](https://blendogames.com/news/post/2025-07-07-skindeepenemybrains/) (Jul 2025)

Built on idTech4, designed for "lovably dim" space pirates inspired by Porco Rosso and TaleSpin.

### Interest Point System (from Mark of the Ninja)
- **Core mechanic**: "Whenever anything makes a noise, or makes a smell, or exists, it can create an 'Interest Point'" that NPCs perceive and react to
- A thrown teapot → temporary noise points (1 sec) + permanent visual markers
- Player movement → smell points every second, lasting 8 seconds, creating natural trail-following
- **Coordination**: closest enemy = "Investigator" (approaches directly), others = "Overwatch" (stop at line-of-sight)

### Anti-Thrashing Systems
- "Ownership" tracking — prevents re-investigating same point
- "DuplicateRadius" — ignores nearby duplicate points
- 1-second cooldown between switching attention
- Priority values (0-10): gunshots > falling objects
- Smells penetrate doors, sounds don't

### Vision System
- Cone-based vision was hard to tune → shifted to **vision rectangle**
- Tapered angles near the enemy facilitate stealth approaches
- Head-mounted orientation creates vertical challenges
- Combat narrows blind-spots (incentivizes staying undetected)

### IdleTasks
Separate system for idle objectives (e.g., injured enemies seek health stations). Chung admits this should have been integrated into Interest Points.

### EV Application
Gibbons uses waypoint-based navigation already. The Interest Point concept could inform future NPC reactions — e.g., Gibbons noticing when the player interacts with objects, responding to sounds from other rooms. The "lovably dim" design goal aligns perfectly with EV's wonder-not-horror philosophy.

---

## 5. Carpet Tech (Shell Fur Rendering)

**Source:** [Carpet Tech](https://blendogames.com/news/post/2018-11-08-carpet_tech/) (Nov 2018)

A "shell fur" approach: **eleven flat planes stacked** with three 1024×1024 textures to simulate carpet depth.

### Technique
- Five bottom layers, three middle, three top
- Texture gets more sparse rising higher → fiber height variation
- Dark overlay patterns create "distressed areas, valleys, matted-down parts"
- Baked lighting: dark at base → bright at surface (depth through color, not dynamic lighting)
- Compatible with **1996-era rendering** while looking good in modern contexts
- Generalizes to "animal fur, hair, grass, foliage"

### EV Application
EV already has a procedural carpet material (materialId 3). This stacked-plane technique is an alternative approach that could add depth to the hotel carpet without shader complexity — though at 960×600 the current procedural approach may read better.

---

## 6. Far Cry 2: Systemic Design Philosophy

**Source:** [On Far Cry 2](https://blendogames.com/news/post/2016-09-26-on-far-cry-2/) (Sep 2016)

### Continuous Simulation
"Guns degrade. Weather patterns cause stormy days and sunny days. Enemies carry their wounded mates to safety." The world refuses to pause — you can be shot while looking at the map.

### Failure as Narrative
Instead of reloading on death, the buddy system transforms failure into story. Companions rescue you, and eventually they permanently fall. Treats "a failure state and sees it as an opportunity."

### "What, Not How"
Specifying objectives while leaving approach to the player. This restraint "frees up room for the player to join in, participate, and share the authorship."

### EV Application
EV's warmth progression (tasks gradually warm the room) is a continuous simulation. The suite should feel lived-in and reactive. The "what, not how" philosophy applies to the bed ritual — give the player tasks, not instructions. The suite changes because the player acts, not because the game tells them to.

---

## 7. Sketches & the Perfect Gem Problem

**Source:** [Skin Deep: Sketches](https://blendogames.com/news/post/2025-04-28-skindeepsketches/) (Apr 2025)

### Design Interests
- "Games that have unexpected verbs"
- Player bodies as "cumbersome and uncooperative" rather than smooth capsules
- Simulation details: injury, inventory representation, tool physicality

### The Gap Between Idea and Engine
> "An idea can exist as a perfect gem in your mind or on paper."

Actual games demand different approaches regarding "needs/requirements/demands." The process involves "figuring out how something would physically appear/move/open/use/etc" before engine implementation.

### EV Application
EV's Quake-style physics (air strafing, bunny hopping, wall running) are the "cumbersome and uncooperative" body made into a feature. The 59-parameter PhysicsConfig is exactly the kind of detailed embodiment Chung sketches toward. "Unexpected verbs" = what EV does with E-press interactions that change geometry.

---

## 8. On Decades: The Myth of the Lone Creator

**Source:** [On Decades](https://blendogames.com/news/post/2020-01-01-on_decades/) (Jan 2020)

### Anti-Solitary Genius
Chung challenges the romanticized lone creator myth. His earlier thinking was "poisonous." Genuine independence means building support networks: "nurturing and giving to a support network wide enough to also help you when you need it."

### Creative Honesty
Through streaming development: "everything goes wrong and bursts into flames" sometimes. Viewing this honesty as essential to managing expectations and preventing burnout.

### Unchanged Core
After 27 years (5 at Pandemic, 12 hobbyist modding, 10 indie): "excited to make the thing, constantly being baffled by the thing, and putting things together one microscopic step at a time."

---

## 9. Automation & Friction Reduction

**Source:** [Automation Tools](https://blendogames.com/news/post/2021-04-05-automation-tools/) (Apr 2021)

> "Mental fatigue sucks. An annoying manual process may only take several seconds, but compounded dozens of times every week, over months and months, will ultimately have a toll."

Goal: "reduce friction, reduce manual processes, and reduce opportunities for errors and mistakes."

Custom tools built: Build Machine (anyone can compile), Steam/Itch uploaders (GUI wrappers), Discord changelog bot, Trello sync.

### EV Application
`make dev SCENE=STATE_SPACE_SUITE` is exactly this philosophy. The QA pipeline (`make qa`), dev-watch auto-rebuild, and playtest builds are friction reduction. Keep investing here.

---

## 10. Source Code as Legacy

**Source:** [Skin Deep: Source Code release](https://blendogames.com/news/post/2025-06-26-skindeepsourcecode/) (Jun 2025)

Skin Deep is built on idTech 4 (Doom 3, 2004) via Dhewm3, with "a large amount of additions and changes" to gameplay code. Released under GPL. Four programmers contributed.

Chung has now open-sourced: Gravity Bone, Thirty Flights of Loving, Quadrilateral Cowboy, and Skin Deep. The open-source commitment is a consistent thread — tooling and code as community infrastructure.

---

## 11. Game Release Wisdom

**Source:** [Game Release Miscellanea](https://blendogames.com/news/post/2025-12-01-miscellanea/) (Dec 2025)

### Practical Shipping Notes
- Don't hardcode WASD — AZERTY and other layouts exist
- Decimal parsing: `7.5` vs `7,5` across regions can corrupt data
- Save location: `%appdata%\YourGameName`, subfolder per Steam ID
- Aggressive code commenting — "revisiting code years later becomes manageable"
- Automated crash reporting (Google Forms + stack traces)
- Keep video settings local, not cloud-synced (Steam Deck users)

### Philosophy
> "It's a miracle that any game actually works."

---

## 12. Small Scope as Discipline

**Source:** [Antibody One](https://blendogames.com/news/post/2025-12-09-antibody_one/) (Dec 2025)

Antibody One: 10-20 minute cellular biology game, ~4 months part-time. Used to learn ECS programming.

> "The best way to learn new tech is to make _and ship_ a small project that uses it."

Built on code from Flotilla, Air Forte, Atom Zombie Smasher — reuse and improve, don't start from scratch.

### EV Application
EV itself is this philosophy — learning Raylib + custom C rendering by building and shipping a specific, scoped experience. The 15-20 minute target runtime is Blendo-scale.

---

## Summary: What EV Should Take from the Blog

| Principle | Blog Source | EV Application |
|-----------|-----------|----------------|
| 80/20 density | Art Density | Procedural materials = 80% calm, interactive objects = 20% focal |
| Match cuts | Storyboards | Elevator doors → hyperspace → lobby doors |
| Kill babies, keep drawer | TFOL Prototype | Orphaned Paris scenes are valid drawer material |
| Interest Points for NPCs | Enemy Brains | Gibbons could react to player sounds/interactions |
| "What, not how" | Far Cry 2 | Suite tasks: give objectives, not instructions |
| Unexpected verbs | Sketches | E-press = geometry change, not flag toggle |
| Friction reduction | Automation | Keep investing in make dev/qa/playtest pipeline |
| Ship small | Antibody One | 15-20 min complete experience > sprawling incomplete one |
