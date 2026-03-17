# Todo

## Vertical Slice

- [x] Make proving ground the primary tuning surface.
- [x] Add room-based proving-ground variants.
- [x] Add authored vertical-slice campaign rooms for Limbo and Circle 1.
- [x] Add first subtraction rule: grapple constrained in Circle 1.
- [x] Add first replacement hook: wind lanes.
- [ ] Tune the proving ground until the loop is fun for repeated 30-second runs.
- [ ] Tune crouch bracing and roll commitment until the posture lane feels intentional, not bolted on.
- [ ] Tune Limbo so it cleanly teaches run, jump, shoot, and grapple.
- [ ] Tune Circle 1 so the loss of grapple reads instantly and the wind/posture rooms feel intentional.

## Presentation

- [x] Keep the preview inspector honest about gameplay scale.
- [x] Flag placeholder player states in the manifest pipeline.
- [x] Replace placeholder player art with compact gameplay-readable sprites.
- [ ] Establish a restrained production palette for world, hazards, enemies, and UI.
- [x] Lock a written player art spec and generate reusable prompts for every required player state.

## Story Through Mechanics

- [x] Add circle metadata for removed power, hook, and transition beat.
- [x] Add a stronger guardian/gate presentation for the Limbo -> Circle 1 transition.
- [ ] Define Circle 2 only after the slice is stable.

## DX

- [x] Add repo-level `luacheck` and `stylua` config.
- [x] Add lightweight content validation for room metadata and missing assets.
- [x] Add campaign smoke commands to the main validation workflow.
- [ ] Add touched-file lint and format commands to the main workflow.
- [ ] Keep procedural generation files parked until the authored slice proves the need.
