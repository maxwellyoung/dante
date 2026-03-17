# Game Direction

This is the source-of-truth product brief for Dante.

## Product Thesis

Infernal Ascent is a fast, authored action-platformer campaign.

The target feel is:

- Celeste-grade movement clarity
- Vlambeer-grade combat impact
- Dante progression delivered through mechanical subtraction and environmental compensation

The game is not currently being optimized around:

- procedural-first replayability
- a general-purpose framework
- text-heavy narrative delivery

## Canonical Verb Set

The baseline kit the game is being designed around is:

- move
- jump
- jump buffer / coyote time / jump cut
- wall interaction
- projectile fire
- grapple

Anything outside this set is not a pillar until it is implemented and required by content at least twice.

## Current Shipping Target

The first serious milestone is one excellent vertical slice:

- proving ground
- Limbo
- Circle 1
- one guardian / gate transition
- one meaningful power loss
- one compensating environment rule

Default development order:

1. Make the proving ground fun for repeated 30-second runs.
2. Replace placeholder player presentation with gameplay-constrained production art.
3. Ship Limbo plus Circle 1 as an authored slice.
4. Only then expand the campaign and extract broader systems.

## First Three Circle Rules

### Proving Ground

- Purpose: feel tuning and instrumentation
- Player kit: full baseline kit
- Constraint: none
- Success: fun without campaign context

### Limbo

- Purpose: teach the stable baseline kit honestly
- Player kit: full baseline kit
- Constraint: none
- Hook: authored traversal/combat rooms with readable gates
- Success: the player understands run, jump, shoot, grapple, and room rhythm

### Circle 1: Lust

- Purpose: first meaningful subtraction
- Removed / constrained power: grapple
- Replacement hook: wind lanes
- Emotional beat: loss of certainty, replacement through commitment and momentum
- Success: the player can explain how Circle 1 changed play after one run

## Deferred On Purpose

These are parked until the slice proves the need:

- procedural generation as a primary mode
- generalized engine or ECS work
- heavy editor/tooling investment
- dialogue / cutscene systems
- broader multi-circle production
