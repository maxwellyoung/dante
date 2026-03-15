-- campaign.lua
-- Micro-trial campaign: short sharp rooms, hard cuts, one rule per room.
-- Inspired by WarioWare brevity + Blendo Games smash cuts + Dante's progressive loss.

return {
    id = "infernal_ascent",
    title = "INFERNAL ASCENT",
    subtitle = "Each circle strips a verb. Each trial proves the new rule.",
    rooms = {
        -- ============================================================
        -- LIMBO — Full kit. Learn the verbs. 5 micro-trials.
        -- ============================================================

        -- LIMBO 1: Cross — staggered islands descending then rising over a hazard pit.
        -- Forces: jump commitment, air control, platform reading.
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
            reach_zone = { x = 27 * 32, y = 3 * 32, width = 32, height = 64 },
            trial_timer = 8,
            fragments_required = 0,
            show_gate = false,
            bg = { 0.1, 0.11, 0.14 },
            solid_color = { 0.28, 0.31, 0.36 },
            accent_color = { 0.88, 0.58, 0.24 },
            abilities = { shoot = true, grapple = true, dash = true },
            map = {
                "111111111111111111111111111111",
                "1                            1",
                "1                            1",
                "1                       11   1",
                "1                            1",
                "1                  11        1",
                "1 P                          1",
                "1 11     11                  1",
                "1              11            1",
                "1                            1",
                "1    1              1        1",
                "1 22222222222222222222222222 1",
                "111111111111111111111111111111",
            },
        },

        -- LIMBO 2: Climb — narrow zigzag chimney. Alternating ledges force
        -- wall jumps with direction changes. Tight, vertical, rhythmic.
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
            reach_zone = { x = 3 * 32, y = 1 * 32, width = 96, height = 32 },
            trial_timer = 10,
            fragments_required = 0,
            show_gate = false,
            bg = { 0.1, 0.11, 0.14 },
            solid_color = { 0.28, 0.31, 0.36 },
            accent_color = { 0.88, 0.58, 0.24 },
            abilities = { shoot = true, grapple = true, dash = true },
            -- Open chimney with small kick-off nubs. Clear sightline to top.
            -- Player can always see the goal. Walls guide, never block.
            map = {
                "11111111111",
                "1         1",
                "1         1",
                "11        1",
                "1        11",
                "1         1",
                "11        1",
                "1        11",
                "1         1",
                "11        1",
                "1        11",
                "1         1",
                "11        1",
                "1    P   11",
                "1   111   1",
                "11111111111",
            },
        },

        -- LIMBO 3: Shoot — multi-level arena with central pillar.
        -- Enemies on different heights. Player must move vertically to get angles.
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
            trial_timer = 12,
            fragments_required = 0,
            show_gate = false,
            bg = { 0.1, 0.11, 0.14 },
            solid_color = { 0.28, 0.31, 0.36 },
            accent_color = { 0.88, 0.58, 0.24 },
            abilities = { shoot = true, grapple = true, dash = true },
            encounter_config = {
                trigger_x = 0,
                bounds = { left = 0, right = 22 * 32 },
                waves = {
                    {
                        spawns = {
                            { x = 16 * 32, y = 8 * 32, type = "walker" },
                            { x = 5 * 32, y = 5 * 32, type = "walker" },
                            { x = 18 * 32, y = 2 * 32, type = "harpy" },
                        },
                    },
                },
            },
            map = {
                "1111111111111111111111",
                "1                    1",
                "1                    1",
                "1 P          111     1",
                "1 11    11           1",
                "1              1    1",
                "1  111               1",
                "1          11       1",
                "1      1        111 1",
                "1                    1",
                "1  111      111  111 1",
                "1111111111111111111111",
            },
        },

        -- LIMBO 4: Chain — grapple across a diagonal chasm. Anchors placed
        -- so you must chain 3 swings. Hazard floor. No straight path.
        {
            id = "limbo_04",
            title = "LIMBO",
            subtitle = "Chain.",
            trial_rule = "GRAPPLE ACROSS THE PIT",
            mode = "campaign",
            room_type = "trial",
            circle_id = "limbo",
            hard_cut = true,
            completion = "reach_zone",
            reach_zone = { x = 24 * 32, y = 1 * 32, width = 32, height = 64 },
            trial_timer = 10,
            fragments_required = 0,
            show_gate = false,
            bg = { 0.1, 0.11, 0.14 },
            solid_color = { 0.28, 0.31, 0.36 },
            accent_color = { 0.88, 0.58, 0.24 },
            abilities = { shoot = true, grapple = true, dash = true },
            map = {
                "11111111111111111111111111111",
                "1                           1",
                "1 P           A          11 1",
                "1 11                        1",
                "1        A          A       1",
                "1                           1",
                "1              A            1",
                "1   A                       1",
                "1                           1",
                "1 2222222222222222222222222  1",
                "11111111111111111111111111111",
            },
        },

        -- LIMBO 5: Survive — compact arena with 3 pillars for cover.
        -- Two waves, tight space, enemies from both sides.
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
            trial_timer = 10,
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
                            { x = 3 * 32, y = 3 * 32, type = "harpy" },
                        },
                        delay_after_clear = 0.3,
                    },
                    {
                        spawns = {
                            { x = 4 * 32, y = 7 * 32, type = "walker" },
                            { x = 16 * 32, y = 7 * 32, type = "walker" },
                            { x = 18 * 32, y = 2 * 32, type = "harpy" },
                        },
                    },
                },
            },
            map = {
                "1111111111111111111111",
                "1                    1",
                "1                    1",
                "1        1     1     1",
                "1   P    1     1     1",
                "1  11    1     1     1",
                "1                    1",
                "1    111   111   111 1",
                "1                    1",
                "1  11    11    11    1",
                "1111111111111111111111",
            },
        },

        -- ============================================================
        -- LUST — Grapple stripped. Wind replaces recovery. 5 micro-trials.
        -- ============================================================

        -- LUST 1: Cross without grapple — same-ish chasm layout but no anchors.
        -- Must wall-jump off narrow pillars and dash to reach far side.
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
            reach_zone = { x = 26 * 32, y = 2 * 32, width = 32, height = 64 },
            trial_timer = 12,
            removed_ability = "grapple",
            fragments_required = 0,
            show_gate = false,
            bg = { 0.2, 0.1, 0.16 },
            solid_color = { 0.4, 0.2, 0.28 },
            accent_color = { 0.95, 0.46, 0.56 },
            abilities = { shoot = true, grapple = false, dash = true },
            map = {
                "1111111111111111111111111111111",
                "1                             1",
                "1 P                       11  1",
                "1 11                          1",
                "1              1              1",
                "1              1         1    1",
                "1      1       1              1",
                "1      1                      1",
                "1                             1",
                "1 22222222222222222222222222   1",
                "1111111111111111111111111111111",
            },
        },

        -- LUST 2: Wind — ascending staircase of platforms with tailwind.
        -- Wind pushes you right and up. Must hop platform-to-platform
        -- while the wind wants to throw you past them.
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
            reach_zone = { x = 28 * 32, y = 1 * 32, width = 32, height = 64 },
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
                    x = 3 * 32, y = 1 * 32,
                    width = 24 * 32, height = 10 * 32,
                    force_x = 480,
                    label = "Tailwind",
                },
            },
            map = {
                "111111111111111111111111111111111",
                "1                               1",
                "1 P                         11  1",
                "1 11                             1",
                "1                                1",
                "1          11                    1",
                "1                    11          1",
                "1     11                         1",
                "1                11              1",
                "1                                1",
                "1 222222222222222222222222222222  1",
                "111111111111111111111111111111111",
            },
        },

        -- LUST 3: Brace — headwind corridor with low ceiling sections.
        -- Must crouch to resist wind in narrow passages, then sprint
        -- through gaps between ceiling blocks.
        {
            id = "lust_03",
            title = "LUST",
            subtitle = "Brace.",
            trial_rule = "CROUCH THROUGH THE GUST",
            mode = "campaign",
            room_type = "trial",
            circle_id = "lust",
            hard_cut = true,
            completion = "reach_zone",
            reach_zone = { x = 28 * 32, y = 6 * 32, width = 32, height = 64 },
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
                    x = 3 * 32, y = 4 * 32,
                    width = 24 * 32, height = 5 * 32,
                    force_x = -550,
                    label = "Headwind",
                },
            },
            map = {
                "111111111111111111111111111111111",
                "1                               1",
                "1                               1",
                "1 P                             1",
                "1 11  1111    1111    1111       1",
                "1                               1",
                "1     1111    1111    1111   11  1",
                "1                               1",
                "1111111111111111111111111111 1111",
                "111111111111111111111111111111111",
            },
        },

        -- LUST 4: Bank — enemies behind L-shaped walls. Direct shots
        -- are blocked. Must use ricochet off ceiling/floor to hit them.
        -- Crosswind bends your aim.
        {
            id = "lust_04",
            title = "LUST",
            subtitle = "Bank.",
            trial_rule = "RICOCHET KILL IN CROSSWIND",
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
                    x = 8 * 32, y = 1 * 32,
                    width = 10 * 32, height = 10 * 32,
                    force_x = -300,
                    label = "Crosswind",
                },
            },
            encounter_config = {
                trigger_x = 0,
                bounds = { left = 0, right = 24 * 32 },
                waves = {
                    {
                        spawns = {
                            { x = 19 * 32, y = 7 * 32, type = "walker" },
                            { x = 20 * 32, y = 3 * 32, type = "harpy" },
                        },
                    },
                },
            },
            map = {
                "1111111111111111111111111",
                "1                       1",
                "1                  1    1",
                "1 P                1    1",
                "1 11        1     1    1",
                "1           1          1",
                "1    11     1          1",
                "1           1     111  1",
                "1      11              1",
                "1           111    11  1",
                "1111111111111111111111111",
            },
        },

        -- LUST 5: Survive without the chain — compact arena, wind pushing
        -- player around, hazard floor sections, pillars for cover.
        -- Two waves of mixed enemies. The final exam.
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
                    width = 10 * 32, height = 5 * 32,
                    force_x = 380,
                    label = "Tailwind",
                },
                {
                    x = 12 * 32, y = 5 * 32,
                    width = 10 * 32, height = 5 * 32,
                    force_x = -320,
                    label = "Crosswind",
                },
            },
            encounter_config = {
                trigger_x = 0,
                bounds = { left = 0, right = 24 * 32 },
                waves = {
                    {
                        spawns = {
                            { x = 18 * 32, y = 8 * 32, type = "walker" },
                            { x = 4 * 32, y = 2 * 32, type = "harpy" },
                        },
                        delay_after_clear = 0.5,
                    },
                    {
                        spawns = {
                            { x = 5 * 32, y = 8 * 32, type = "walker" },
                            { x = 17 * 32, y = 8 * 32, type = "walker" },
                            { x = 10 * 32, y = 2 * 32, type = "harpy" },
                            { x = 19 * 32, y = 2 * 32, type = "harpy" },
                        },
                    },
                },
            },
            map = {
                "1111111111111111111111111",
                "1                       1",
                "1        1       1      1",
                "1   P    1       1      1",
                "1  11    1       1      1",
                "1                       1",
                "1    11      1      11  1",
                "1            1          1",
                "1   22   111 1 111  22  1",
                "1  1111            1111 1",
                "1111111111111111111111111",
            },
        },
    },
}
