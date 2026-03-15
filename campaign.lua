-- campaign.lua
-- Micro-trial campaign: short sharp rooms, hard cuts, one rule per room.
--
-- DESIGN PRINCIPLES (Miyamoto / Vanaman / Fish):
-- - Player should ALWAYS see where they need to go
-- - Obstacles create choices, not dead ends
-- - Wide platforms, generous margins — controls aren't Celeste-tight
-- - Each room teaches ONE thing through play, not text
-- - If death feels unfair, the room is wrong, not the player

return {
    id = "infernal_ascent",
    title = "INFERNAL ASCENT",
    subtitle = "Each circle strips a verb. Each trial proves the new rule.",
    rooms = {
        -- ============================================================
        -- LIMBO — Full kit. Learn the verbs. 5 micro-trials.
        -- ============================================================

        -- LIMBO 1: Cross — wide platforms descending like stairs over lava.
        -- Generous landing zones. Teaches: basic jump + air control.
        -- The player can see the glowing zone from spawn.
        {
            id = "limbo_01",
            title = "LIMBO",
            subtitle = "Cross.",
            trial_rule = "REACH THE OTHER SIDE",
            mode = "campaign",
            room_type = "trial",
            circle_id = "limbo",
            hard_cut = true,
            completion = "reach_zone",
            reach_zone = { x = 22 * 32, y = 5 * 32, width = 64, height = 96 },
            trial_timer = 10,
            fragments_required = 0,
            show_gate = false,
            bg = { 0.1, 0.11, 0.14 },
            solid_color = { 0.28, 0.31, 0.36 },
            accent_color = { 0.88, 0.58, 0.24 },
            abilities = { shoot = true, grapple = true, dash = true },
            map = {
                "1111111111111111111111111111",
                "1                          1",
                "1                          1",
                "1                          1",
                "1 P                        1",
                "1 1111                     1",
                "1                 1111     1",
                "1        1111              1",
                "1                    11111 1",
                "1                          1",
                "1 22222222222222222222222222 1",
                "1111111111111111111111111111",
            },
        },

        -- LIMBO 2: Climb — wide chimney, walls close enough to bounce between.
        -- No ledges blocking the center. Just two walls and momentum.
        -- Generous width so mistimed jumps don't fail instantly.
        {
            id = "limbo_02",
            title = "LIMBO",
            subtitle = "Climb.",
            trial_rule = "WALL JUMP TO THE TOP",
            mode = "campaign",
            room_type = "trial",
            circle_id = "limbo",
            hard_cut = true,
            completion = "reach_zone",
            reach_zone = { x = 3 * 32, y = 1 * 32, width = 128, height = 48 },
            trial_timer = 12,
            fragments_required = 0,
            show_gate = false,
            bg = { 0.1, 0.11, 0.14 },
            solid_color = { 0.28, 0.31, 0.36 },
            accent_color = { 0.88, 0.58, 0.24 },
            abilities = { shoot = true, grapple = true, dash = true },
            -- Just two walls. Nothing in between. Pure wall-jump rhythm.
            map = {
                "1111111111",
                "1        1",
                "1        1",
                "1        1",
                "1        1",
                "1        1",
                "1        1",
                "1        1",
                "1        1",
                "1        1",
                "1        1",
                "1   P    1",
                "1  1111  1",
                "1111111111",
            },
        },

        -- LIMBO 3: Shoot — flat arena, enemies come to you. No platforming
        -- pressure, just learn to aim and fire. Wide floor, nowhere to fall.
        {
            id = "limbo_03",
            title = "LIMBO",
            subtitle = "Shoot.",
            trial_rule = "KILL EVERYTHING",
            mode = "campaign",
            room_type = "trial",
            circle_id = "limbo",
            hard_cut = true,
            completion = "kill_all",
            trial_timer = 14,
            fragments_required = 0,
            show_gate = false,
            bg = { 0.1, 0.11, 0.14 },
            solid_color = { 0.28, 0.31, 0.36 },
            accent_color = { 0.88, 0.58, 0.24 },
            abilities = { shoot = true, grapple = true, dash = true },
            encounter_config = {
                trigger_x = 0,
                bounds = { left = 0, right = 24 * 32 },
                waves = {
                    {
                        spawns = {
                            { x = 18 * 32, y = 6 * 32, type = "walker" },
                            { x = 20 * 32, y = 6 * 32, type = "walker" },
                            { x = 19 * 32, y = 2 * 32, type = "harpy" },
                        },
                    },
                },
            },
            map = {
                "111111111111111111111111",
                "1                      1",
                "1                      1",
                "1                      1",
                "1  P                   1",
                "1                      1",
                "1            111       1",
                "1                      1",
                "1111111111111111111111111",
            },
        },

        -- LIMBO 4: Chain — two big platforms with a wide gap. One grapple
        -- anchor in the middle. Simple: jump, grapple, land. Generous
        -- landing platform on the far side.
        {
            id = "limbo_04",
            title = "LIMBO",
            subtitle = "Chain.",
            trial_rule = "GRAPPLE ACROSS",
            mode = "campaign",
            room_type = "trial",
            circle_id = "limbo",
            hard_cut = true,
            completion = "reach_zone",
            reach_zone = { x = 20 * 32, y = 4 * 32, width = 64, height = 96 },
            trial_timer = 10,
            fragments_required = 0,
            show_gate = false,
            bg = { 0.1, 0.11, 0.14 },
            solid_color = { 0.28, 0.31, 0.36 },
            accent_color = { 0.88, 0.58, 0.24 },
            abilities = { shoot = true, grapple = true, dash = true },
            map = {
                "11111111111111111111111111",
                "1                        1",
                "1            A           1",
                "1                        1",
                "1 P                      1",
                "1 1111       A    111111 1",
                "1                        1",
                "1        A               1",
                "1                        1",
                "1 22222222222222222222222 1",
                "11111111111111111111111111",
            },
        },

        -- LIMBO 5: Survive — big open room with a raised platform in the
        -- center for safety. Enemies come from sides. You can always retreat
        -- to high ground. Teaches: movement under pressure.
        {
            id = "limbo_05",
            title = "LIMBO",
            subtitle = "Survive.",
            trial_rule = "STAY ALIVE",
            mode = "campaign",
            room_type = "trial",
            circle_id = "limbo",
            hard_cut = true,
            completion = "survive",
            trial_timer = 12,
            fragments_required = 0,
            show_gate = false,
            bg = { 0.12, 0.13, 0.17 },
            solid_color = { 0.3, 0.33, 0.38 },
            accent_color = { 0.92, 0.6, 0.26 },
            abilities = { shoot = true, grapple = true, dash = true },
            encounter_config = {
                trigger_x = 0,
                bounds = { left = 0, right = 22 * 32 },
                waves = {
                    {
                        spawns = {
                            { x = 17 * 32, y = 7 * 32, type = "walker" },
                            { x = 15 * 32, y = 2 * 32, type = "harpy" },
                        },
                        delay_after_clear = 0.4,
                    },
                    {
                        spawns = {
                            { x = 4 * 32, y = 7 * 32, type = "walker" },
                            { x = 17 * 32, y = 7 * 32, type = "walker" },
                            { x = 10 * 32, y = 2 * 32, type = "harpy" },
                        },
                    },
                },
            },
            map = {
                "1111111111111111111111",
                "1                    1",
                "1                    1",
                "1                    1",
                "1  P                 1",
                "1  11   111111  11   1",
                "1                    1",
                "1                    1",
                "1111111111111111111111",
            },
        },

        -- ============================================================
        -- LUST — Grapple stripped. Wind replaces recovery.
        -- Wider everything. Wind is the new tool, not precision.
        -- ============================================================

        -- LUST 1: Cross without grapple — wide gap but with platforms
        -- you can wall-jump off of. More forgiving than Limbo 4 was
        -- WITH the grapple — the loss should feel like "different", not "harder".
        {
            id = "lust_01",
            title = "LUST",
            subtitle = "The chain is taken.",
            trial_rule = "NO GRAPPLE. CROSS.",
            mode = "campaign",
            room_type = "trial",
            circle_id = "lust",
            hard_cut = true,
            completion = "reach_zone",
            reach_zone = { x = 22 * 32, y = 4 * 32, width = 64, height = 96 },
            trial_timer = 12,
            removed_ability = "grapple",
            fragments_required = 0,
            show_gate = false,
            bg = { 0.2, 0.1, 0.16 },
            solid_color = { 0.4, 0.2, 0.28 },
            accent_color = { 0.95, 0.46, 0.56 },
            abilities = { shoot = true, grapple = false, dash = true },
            map = {
                "1111111111111111111111111111",
                "1                          1",
                "1                          1",
                "1                          1",
                "1 P               11111111 1",
                "1 1111                     1",
                "1          11              1",
                "1                          1",
                "1     11        11         1",
                "1                          1",
                "1 22222222222222222222222222 1",
                "1111111111111111111111111111",
            },
        },

        -- LUST 2: Wind — tailwind pushes you across a wide room.
        -- Big platforms to land on. Wind does the work, you just steer.
        -- Teaches: wind is your friend, not your enemy.
        {
            id = "lust_02",
            title = "LUST",
            subtitle = "Wind.",
            trial_rule = "RIDE THE TAILWIND",
            mode = "campaign",
            room_type = "trial",
            circle_id = "lust",
            hard_cut = true,
            completion = "reach_zone",
            reach_zone = { x = 24 * 32, y = 4 * 32, width = 64, height = 96 },
            trial_timer = 10,
            removed_ability = "grapple",
            fragments_required = 0,
            show_gate = false,
            bg = { 0.2, 0.1, 0.16 },
            solid_color = { 0.4, 0.2, 0.28 },
            accent_color = { 0.95, 0.46, 0.56 },
            abilities = { shoot = true, grapple = false, dash = true },
            wind_areas = {
                {
                    x = 2 * 32, y = 1 * 32,
                    width = 24 * 32, height = 8 * 32,
                    force_x = 420,
                    label = "Tailwind",
                },
            },
            map = {
                "11111111111111111111111111111",
                "1                           1",
                "1                           1",
                "1                           1",
                "1 P                  11111  1",
                "1 1111                      1",
                "1          1111             1",
                "1                   1111    1",
                "1     1111                  1",
                "1                           1",
                "1 222222222222222222222222222 1",
                "11111111111111111111111111111",
            },
        },

        -- LUST 3: Brace — headwind pushes you back. But the corridor is
        -- wide and flat. Crouch to halve the wind. Just walk forward.
        -- Simple, but teaches the mechanic clearly.
        {
            id = "lust_03",
            title = "LUST",
            subtitle = "Brace.",
            trial_rule = "CROUCH INTO THE WIND",
            mode = "campaign",
            room_type = "trial",
            circle_id = "lust",
            hard_cut = true,
            completion = "reach_zone",
            reach_zone = { x = 22 * 32, y = 4 * 32, width = 64, height = 96 },
            trial_timer = 12,
            removed_ability = "grapple",
            fragments_required = 0,
            show_gate = false,
            bg = { 0.2, 0.1, 0.16 },
            solid_color = { 0.4, 0.2, 0.28 },
            accent_color = { 0.95, 0.46, 0.56 },
            abilities = { shoot = true, grapple = false, dash = true, crouch = true },
            wind_areas = {
                {
                    x = 4 * 32, y = 2 * 32,
                    width = 18 * 32, height = 5 * 32,
                    force_x = -480,
                    label = "Headwind",
                },
            },
            map = {
                "1111111111111111111111111111",
                "1                          1",
                "1                          1",
                "1 P                        1",
                "1                     11   1",
                "1                          1",
                "1                          1",
                "1111111111111111111111111111",
            },
        },

        -- LUST 4: Bank — one walker on a platform you can't reach directly.
        -- Wall behind it to bounce shots off. Crosswind makes direct shots
        -- drift. Teaches: ricochet is a tool, not a trick.
        {
            id = "lust_04",
            title = "LUST",
            subtitle = "Bank.",
            trial_rule = "USE THE WALLS",
            mode = "campaign",
            room_type = "trial",
            circle_id = "lust",
            hard_cut = true,
            completion = "kill_all",
            trial_timer = 14,
            removed_ability = "grapple",
            fragments_required = 0,
            show_gate = false,
            bg = { 0.22, 0.1, 0.17 },
            solid_color = { 0.42, 0.2, 0.28 },
            accent_color = { 0.95, 0.46, 0.56 },
            abilities = { shoot = true, grapple = false, dash = true },
            wind_areas = {
                {
                    x = 6 * 32, y = 1 * 32,
                    width = 14 * 32, height = 8 * 32,
                    force_x = -250,
                    label = "Crosswind",
                },
            },
            encounter_config = {
                trigger_x = 0,
                bounds = { left = 0, right = 22 * 32 },
                waves = {
                    {
                        spawns = {
                            { x = 17 * 32, y = 4 * 32, type = "walker" },
                            { x = 18 * 32, y = 1 * 32, type = "harpy" },
                        },
                    },
                },
            },
            map = {
                "111111111111111111111111",
                "1                      1",
                "1                      1",
                "1  P                   1",
                "1  1111      11111111111",
                "1                      1",
                "1                      1",
                "1                      1",
                "111111111111111111111111",
            },
        },

        -- LUST 5: Survive — open room with wind pushing you around.
        -- Big floor, no pits. Enemies from both sides. The challenge is
        -- managing position in wind, not precision platforming.
        {
            id = "lust_05",
            title = "LUST",
            subtitle = "Prove it.",
            trial_rule = "SURVIVE WITHOUT THE CHAIN",
            mode = "campaign",
            room_type = "trial",
            circle_id = "lust",
            hard_cut = true,
            completion = "survive",
            trial_timer = 14,
            removed_ability = "grapple",
            fragments_required = 0,
            show_gate = false,
            bg = { 0.24, 0.1, 0.17 },
            solid_color = { 0.44, 0.2, 0.29 },
            accent_color = { 0.97, 0.44, 0.58 },
            abilities = { shoot = true, grapple = false, dash = true, crouch = true, roll = true },
            wind_areas = {
                {
                    x = 2 * 32, y = 1 * 32,
                    width = 9 * 32, height = 6 * 32,
                    force_x = 350,
                    label = "Tailwind",
                },
                {
                    x = 11 * 32, y = 1 * 32,
                    width = 9 * 32, height = 6 * 32,
                    force_x = -300,
                    label = "Crosswind",
                },
            },
            encounter_config = {
                trigger_x = 0,
                bounds = { left = 0, right = 22 * 32 },
                waves = {
                    {
                        spawns = {
                            { x = 17 * 32, y = 6 * 32, type = "walker" },
                            { x = 14 * 32, y = 2 * 32, type = "harpy" },
                        },
                        delay_after_clear = 0.5,
                    },
                    {
                        spawns = {
                            { x = 4 * 32, y = 6 * 32, type = "walker" },
                            { x = 17 * 32, y = 6 * 32, type = "walker" },
                            { x = 10 * 32, y = 2 * 32, type = "harpy" },
                        },
                    },
                },
            },
            map = {
                "1111111111111111111111",
                "1                    1",
                "1                    1",
                "1  P                 1",
                "1                    1",
                "1       111111       1",
                "1                    1",
                "1111111111111111111111",
            },
        },

        -- ============================================================
        -- GLUTTONY — Dash stripped. Sludge floors slow you down.
        -- No burst escape. You must be deliberate, not reactive.
        -- ============================================================

        -- GLUTTONY 1: Cross — sludge patches on the floor slow you.
        -- Must jump over them or accept the drag. Wide platforms.
        {
            id = "gluttony_01",
            title = "GLUTTONY",
            subtitle = "The burst is denied.",
            trial_rule = "NO DASH. MOVE THROUGH THE MIRE.",
            mode = "campaign",
            room_type = "trial",
            circle_id = "gluttony",
            hard_cut = true,
            completion = "reach_zone",
            reach_zone = { x = 22 * 32, y = 4 * 32, width = 64, height = 96 },
            trial_timer = 14,
            removed_ability = "dash",
            fragments_required = 0,
            show_gate = false,
            bg = { 0.12, 0.16, 0.08 },
            solid_color = { 0.28, 0.34, 0.22 },
            accent_color = { 0.4, 0.72, 0.3 },
            sludge_color = { 0.32, 0.48, 0.24 },
            abilities = { shoot = true, grapple = false, dash = false },
            map = {
                "1111111111111111111111111111",
                "1                          1",
                "1                          1",
                "1                          1",
                "1 P               111111   1",
                "1 1111                     1",
                "1       3333  1111         1",
                "1       3333               1",
                "1111111133331111111111111111",
            },
        },

        -- GLUTTONY 2: Climb — vertical room, sludge on some walls.
        -- Wall-jump upward but some walls are sludge (slow your slide).
        -- Must read which walls are safe to kick off.
        {
            id = "gluttony_02",
            title = "GLUTTONY",
            subtitle = "Read the walls.",
            trial_rule = "CLIMB WITHOUT THE BURST",
            mode = "campaign",
            room_type = "trial",
            circle_id = "gluttony",
            hard_cut = true,
            completion = "reach_zone",
            reach_zone = { x = 3 * 32, y = 1 * 32, width = 128, height = 48 },
            trial_timer = 14,
            removed_ability = "dash",
            fragments_required = 0,
            show_gate = false,
            bg = { 0.12, 0.16, 0.08 },
            solid_color = { 0.28, 0.34, 0.22 },
            accent_color = { 0.4, 0.72, 0.3 },
            abilities = { shoot = true, grapple = false, dash = false },
            map = {
                "1111111111111",
                "1           1",
                "1           1",
                "1           1",
                "1    111    1",
                "1           1",
                "1           1",
                "1   1111    1",
                "1           1",
                "1           1",
                "1    1111   1",
                "1    P      1",
                "1   111     1",
                "1111111111111",
            },
        },

        -- GLUTTONY 3: Fight — flat arena with sludge patches.
        -- Enemies walk through sludge fine, you don't. Must position
        -- carefully and shoot from safe ground.
        {
            id = "gluttony_03",
            title = "GLUTTONY",
            subtitle = "Hold your ground.",
            trial_rule = "FIGHT FROM SOLID GROUND",
            mode = "campaign",
            room_type = "trial",
            circle_id = "gluttony",
            hard_cut = true,
            completion = "kill_all",
            trial_timer = 14,
            removed_ability = "dash",
            fragments_required = 0,
            show_gate = false,
            bg = { 0.12, 0.16, 0.08 },
            solid_color = { 0.28, 0.34, 0.22 },
            accent_color = { 0.4, 0.72, 0.3 },
            abilities = { shoot = true, grapple = false, dash = false },
            encounter_config = {
                trigger_x = 0,
                bounds = { left = 0, right = 22 * 32 },
                waves = {
                    {
                        spawns = {
                            { x = 16 * 32, y = 6 * 32, type = "walker" },
                            { x = 18 * 32, y = 6 * 32, type = "walker" },
                            { x = 14 * 32, y = 2 * 32, type = "harpy" },
                        },
                    },
                },
            },
            map = {
                "1111111111111111111111",
                "1                    1",
                "1                    1",
                "1  P                 1",
                "1  1111              1",
                "1                    1",
                "1        111        1",
                "1111 333333333 111111",
                "1111111111111111111111",
            },
        },

        -- GLUTTONY 4: Patience — long sludge corridor with harpy above.
        -- Can't dash through. Must crouch-walk and time shots upward.
        -- Teaches: sometimes slow is the only way.
        {
            id = "gluttony_04",
            title = "GLUTTONY",
            subtitle = "Patience.",
            trial_rule = "YOU CANNOT RUSH THIS",
            mode = "campaign",
            room_type = "trial",
            circle_id = "gluttony",
            hard_cut = true,
            completion = "reach_zone",
            reach_zone = { x = 22 * 32, y = 4 * 32, width = 64, height = 96 },
            trial_timer = 16,
            removed_ability = "dash",
            fragments_required = 0,
            show_gate = false,
            bg = { 0.13, 0.17, 0.09 },
            solid_color = { 0.3, 0.36, 0.24 },
            accent_color = { 0.45, 0.75, 0.35 },
            abilities = { shoot = true, grapple = false, dash = false },
            map = {
                "1111111111111111111111111111",
                "1                          1",
                "1        H          H      1",
                "1                          1",
                "1 P                    11  1",
                "1 11                       1",
                "1    333333333333333333     1",
                "1111111111111111111111111111",
            },
        },

        -- GLUTTONY 5: Survive — sludge arena with enemies from all sides.
        -- Center island of solid ground. Sludge surrounds it.
        -- Must hold the island and shoot outward.
        {
            id = "gluttony_05",
            title = "GLUTTONY",
            subtitle = "Prove it.",
            trial_rule = "SURVIVE IN THE MIRE",
            mode = "campaign",
            room_type = "trial",
            circle_id = "gluttony",
            hard_cut = true,
            completion = "survive",
            trial_timer = 14,
            removed_ability = "dash",
            fragments_required = 0,
            show_gate = false,
            bg = { 0.14, 0.18, 0.1 },
            solid_color = { 0.32, 0.38, 0.26 },
            accent_color = { 0.5, 0.78, 0.38 },
            abilities = { shoot = true, grapple = false, dash = false, crouch = true },
            encounter_config = {
                trigger_x = 0,
                bounds = { left = 0, right = 22 * 32 },
                waves = {
                    {
                        spawns = {
                            { x = 4 * 32, y = 6 * 32, type = "walker" },
                            { x = 17 * 32, y = 2 * 32, type = "harpy" },
                        },
                        delay_after_clear = 0.5,
                    },
                    {
                        spawns = {
                            { x = 3 * 32, y = 6 * 32, type = "walker" },
                            { x = 18 * 32, y = 6 * 32, type = "walker" },
                            { x = 5 * 32, y = 2 * 32, type = "harpy" },
                            { x = 16 * 32, y = 2 * 32, type = "harpy" },
                        },
                    },
                },
            },
            map = {
                "1111111111111111111111",
                "1                    1",
                "1                    1",
                "1  P                 1",
                "1                    1",
                "1       111111       1",
                "1                    1",
                "1 33 333333333333 33 1",
                "1111111111111111111111",
            },
        },
    },
}

