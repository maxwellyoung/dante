local Palette = {}

Palette.ui = {
    action = { 0.92, 0.55, 0.18 },
    ink = { 0.8, 0.82, 0.9 },
    warning = { 1.0, 0.2, 0.15 },
}

Palette.world = {
    proving_ground = {
        bg = { 0.08, 0.09, 0.11 },
        solid = { 0.28, 0.31, 0.36 },
        accent = { 0.92, 0.55, 0.18 },
        hazard = { 0.88, 0.22, 0.2 },
        sludge = { 0.32, 0.48, 0.24 },
        gate = { 0.92, 0.55, 0.18 },
        locked_gate = { 0.42, 0.22, 0.18 },
    },
    limbo = {
        bg = { 0.1, 0.11, 0.14 },
        solid = { 0.28, 0.31, 0.36 },
        accent = { 0.88, 0.58, 0.24 },
        hazard = { 0.88, 0.22, 0.2 },
        sludge = { 0.32, 0.48, 0.24 },
        gate = { 0.88, 0.58, 0.24 },
        locked_gate = { 0.38, 0.22, 0.18 },
    },
    lust = {
        bg = { 0.22, 0.1, 0.17 },
        solid = { 0.42, 0.2, 0.28 },
        accent = { 0.95, 0.46, 0.56 },
        hazard = { 0.92, 0.28, 0.28 },
        sludge = { 0.32, 0.48, 0.24 },
        gate = { 0.95, 0.46, 0.56 },
        locked_gate = { 0.42, 0.18, 0.24 },
    },
    gluttony = {
        bg = { 0.13, 0.17, 0.09 },
        solid = { 0.3, 0.36, 0.24 },
        accent = { 0.45, 0.75, 0.35 },
        hazard = { 0.88, 0.22, 0.2 },
        sludge = { 0.32, 0.48, 0.24 },
        gate = { 0.45, 0.75, 0.35 },
        locked_gate = { 0.22, 0.34, 0.18 },
    },
}

Palette.enemy = {
    walker_body = { 0.84, 0.28, 0.24 },
    walker_shadow = { 0.18, 0.07, 0.07 },
    walker_face = { 1.0, 0.88, 0.74 },
    harpy_body = { 0.78, 0.76, 0.32 },
    harpy_wing = { 0.26, 0.2, 0.08 },
    harpy_face = { 1.0, 0.95, 0.78 },
    projectile = { 0.95, 0.35, 0.3 },
    telegraph = { 1.0, 0.2, 0.15 },
    telegraph_line = { 1.0, 0.3, 0.2 },
}

Palette.effects = {
    muzzle = { 1.0, 0.86, 0.38 },
    sparks = { 1.0, 1.0, 0.5 },
    ricochet = { 1.0, 0.78, 0.38 },
    dash = { 1.0, 0.82, 0.45 },
    dash_impact_wall = { 1.0, 0.9, 0.55 },
    dash_impact_body = { 0.95, 0.75, 0.48 },
    blood = { 1.0, 0.0, 0.0 },
    dust = { 0.8, 0.72, 0.64 },
    jump_dust = { 0.9, 0.82, 0.72 },
    grapple_latch = { 0.7, 0.85, 1.0 },
    grapple_flash = { 1.0, 1.0, 1.0 },
    stomp_ring = { 1.0, 0.85, 0.3 },
    wall_jump = { 0.85, 0.8, 0.7 },
    ricochet_kill = { 1.0, 0.92, 0.4 },
    ricochet_flash = { 1.0, 1.0, 0.85 },
    rubble = { 0.6, 0.5, 0.4 },
}

return Palette
