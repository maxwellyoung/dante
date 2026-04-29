-- campaign.lua
-- Micro-trial campaign: short sharp rooms, hard cuts, one rule per room.
--
-- DESIGN PRINCIPLES (Miyamoto / Vanaman / Fish):
-- - Player should ALWAYS see where they need to go
-- - Obstacles create choices, not dead ends
-- - Wide platforms, generous margins — controls aren't Celeste-tight
-- - Each room teaches ONE thing through play, not text
-- - If death feels unfair, the room is wrong, not the player

local Palette = require("infernal_ascent_palette")
local limbo = Palette.world.limbo
local lust = Palette.world.lust
local gluttony = Palette.world.gluttony

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
            dramatic_question = "Can the full kit read in one glance?",
            transition_beat = "The first room should feel generous, fast, and honest about the baseline.",
            trial_rule = "REACH THE OTHER SIDE",
            objective_text = "Cross the room cleanly and enter the EXIT zone.",
            hud_objective_text = "Cross clean. Exit right.",
            mode = "campaign",
            room_type = "traversal",
            circle_id = "limbo",
            hard_cut = true,
            completion = "reach_zone",
            reach_zone = { x = 22 * 32, y = 5 * 32, width = 64, height = 96 },
            trial_timer = 10,
            fragments_required = 0,
            show_gate = false,
            bg = limbo.bg,
            solid_color = limbo.solid,
            accent_color = limbo.accent,
            abilities = { shoot = true, grapple = true, dash = true },
            qa_expectations = {
                max_deaths = 0,
            },
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
            dramatic_question = "Does vertical commitment feel clean without hidden tech?",
            transition_beat = "Two walls, one rhythm, no debris between the decision and the movement.",
            trial_rule = "WALL JUMP TO THE TOP",
            objective_text = "Wall-jump up the shaft and hit the EXIT zone at the top.",
            hud_objective_text = "Climb the shaft. Exit up top.",
            mode = "campaign",
            room_type = "traversal",
            circle_id = "limbo",
            hard_cut = true,
            completion = "reach_zone",
            reach_zone = { x = 3 * 32, y = 1 * 32, width = 128, height = 48 },
            trial_timer = 12,
            fragments_required = 0,
            show_gate = false,
            bg = limbo.bg,
            solid_color = limbo.solid,
            accent_color = limbo.accent,
            abilities = { shoot = true, grapple = true, dash = true },
            qa_expectations = {
                max_deaths = 0,
            },
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
            dramatic_question = "Can combat stay in motion instead of becoming a stop-and-pop mode?",
            transition_beat = "Pressure should arrive in lanes, not as noise.",
            trial_rule = "KILL EVERYTHING",
            objective_text = "Clear every enemy in the room, then move on.",
            hud_objective_text = "Kill everything.",
            mode = "campaign",
            room_type = "combat",
            circle_id = "limbo",
            hard_cut = true,
            completion = "kill_all",
            trial_timer = 14,
            fragments_required = 0,
            show_gate = false,
            bg = limbo.bg,
            solid_color = limbo.solid,
            accent_color = limbo.accent,
            abilities = { shoot = true, grapple = true, dash = true },
            qa_expectations = {
                requires_encounter_clear = true,
            },
            encounter_config = {
                trigger_x = 0,
                bounds = { left = 0, right = 24 * 32 },
                intro_title = "LIMBO",
                intro_subtitle = "Hold the lane. Kill cleanly.",
                outro_subtitle = "Room clear. Keep the rhythm.",
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
            dramatic_question = "Does the grapple compress a route instead of rescuing a mistake?",
            transition_beat = "The anchor should feel like a commitment, not a bailout.",
            trial_rule = "GRAPPLE ACROSS",
            objective_text = "Latch the anchor route and cross the room cleanly.",
            hud_objective_text = "Latch. Cross. Exit right.",
            mode = "campaign",
            room_type = "grapple",
            circle_id = "limbo",
            hard_cut = true,
            completion = "reach_zone",
            reach_zone = { x = 20 * 32, y = 4 * 32, width = 64, height = 96 },
            trial_timer = 10,
            fragments_required = 0,
            show_gate = false,
            bg = limbo.bg,
            solid_color = limbo.solid,
            accent_color = limbo.accent,
            abilities = { shoot = true, grapple = true, dash = true },
            qa_expectations = {
                min_grapple_latches = 1,
            },
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
            dramatic_question = "Can the full kit stay readable once pressure arrives from every side?",
            transition_beat = "The room should test composure before the first subtraction lands.",
            trial_rule = "STAY ALIVE",
            objective_text = "Survive the encounter, then take the route forward.",
            hud_objective_text = "Survive. Move on.",
            mode = "campaign",
            room_type = "pressure",
            circle_id = "limbo",
            hard_cut = true,
            completion = "survive",
            trial_timer = 12,
            fragments_required = 0,
            show_gate = false,
            bg = limbo.bg,
            solid_color = limbo.solid,
            accent_color = limbo.accent,
            abilities = { shoot = true, grapple = true, dash = true },
            qa_expectations = {
                requires_encounter_clear = true,
            },
            encounter_config = {
                trigger_x = 0,
                bounds = { left = 0, right = 22 * 32 },
                intro_title = "LIMBO GATE",
                intro_subtitle = "Keep the full kit readable under pressure.",
                outro_subtitle = "The gate yields. Something is about to be taken.",
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
            id = "circle1_01",
            title = "LUST",
            subtitle = "The chain is taken.",
            dramatic_question = "When the chain is gone, does the room immediately teach a different kind of commitment?",
            transition_beat = "Circle 1 should feel like loss first, then replacement through movement.",
            trial_rule = "NO GRAPPLE. CROSS.",
            objective_text = "Cross without the chain and let the wind carry the final commitment.",
            hud_objective_text = "No grapple. Cross anyway.",
            mode = "campaign",
            room_type = "traversal",
            circle_id = "lust",
            hard_cut = true,
            completion = "reach_zone",
            reach_zone = { x = 22 * 32, y = 4 * 32, width = 64, height = 96 },
            trial_timer = 12,
            removed_ability = "grapple",
            environment_hook = "Wind lanes replace the chain.",
            fragments_required = 0,
            show_gate = false,
            bg = lust.bg,
            solid_color = lust.solid,
            accent_color = lust.accent,
            abilities = { shoot = true, grapple = false, dash = true },
            beat_config = {
                title = "CIRCLE 1: LUST",
                subtitle = "The chain is gone. Read the wind instead.",
                trigger_x = 6 * 32,
                duration = 2.2,
            },
            wind_areas = {
                {
                    x = 12 * 32, y = 2 * 32,
                    width = 10 * 32, height = 5 * 32,
                    force_x = 220,
                    label = "Tailwind",
                },
            },
            qa_expectations = {
                requires_removed_ability_notice = true,
                requires_environment_label = true,
            },
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
            id = "circle1_02",
            title = "LUST",
            subtitle = "Wind.",
            dramatic_question = "Can posture and restraint make the wind feel authored instead of arbitrary?",
            trial_rule = "RIDE THE TAILWIND",
            objective_text = "Read the gust, keep low when needed, and ride it across the room.",
            hud_objective_text = "Read the gust. Ride it across.",
            mode = "campaign",
            room_type = "traversal",
            circle_id = "lust",
            hard_cut = true,
            completion = "reach_zone",
            reach_zone = { x = 24 * 32, y = 4 * 32, width = 64, height = 96 },
            trial_timer = 10,
            removed_ability = "grapple",
            environment_hook = "The gust decides the route now.",
            fragments_required = 0,
            show_gate = false,
            bg = lust.bg,
            solid_color = lust.solid,
            accent_color = lust.accent,
            abilities = { shoot = true, grapple = false, dash = true, crouch = true, roll = true },
            npc_guidance = {
                {
                    speaker = "Virgil",
                    line = "Do not fight the gust. Enter it with intent.",
                    trigger_radius = 72,
                },
            },
            qa_expectations = {
                requires_removed_ability_notice = true,
                requires_environment_label = true,
            },
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
            id = "circle1_03",
            title = "LUST",
            subtitle = "Brace.",
            dramatic_question = "Will the player understand that low posture is now a routing tool?",
            trial_rule = "CROUCH INTO THE WIND",
            objective_text = "Brace into the headwind, stay low through the lane, and take the EXIT gate.",
            hud_objective_text = "Brace low. Stay low. Exit right.",
            mode = "campaign",
            room_type = "posture",
            circle_id = "lust",
            hard_cut = true,
            completion = "reach_zone",
            reach_zone = { x = 12 * 32, y = 5 * 32, width = 96, height = 96 },
            trial_timer = 12,
            removed_ability = "grapple",
            environment_hook = "Stay low and the headwind stops owning the lane.",
            fragments_required = 0,
            show_gate = true,
            bg = lust.bg,
            solid_color = lust.solid,
            accent_color = lust.accent,
            abilities = { shoot = true, grapple = false, dash = true, crouch = true },
            npc_guidance = {
                {
                    speaker = "Virgil",
                    line = "Stay low. Let the gust slide over you, then take the gate.",
                    trigger_radius = 72,
                },
            },
            qa_expectations = {
                requires_removed_ability_notice = true,
                requires_environment_label = true,
                min_crouch_time = 1,
            },
            wind_areas = {
                {
                    x = 4 * 32, y = 2 * 32,
                    width = 10 * 32, height = 5 * 32,
                    force_x = -480,
                    label = "Headwind",
                },
            },
            map = {
                "1111111111111111111111111111",
                "1                          1",
                "1                          1",
                "1 P                        1",
                "1           111111         1",
                "1                          1",
                "1            G             1",
                "1111111111111111111111111111",
            },
        },

        -- LUST 4: Bank — one walker on a platform you can't reach directly.
        -- Wall behind it to bounce shots off. Crosswind makes direct shots
        -- drift. Teaches: ricochet is a tool, not a trick.
        {
            id = "circle1_04",
            title = "LUST",
            subtitle = "Bank.",
            dramatic_question = "When direct movement is denied, will the player turn the room itself into the weapon?",
            trial_rule = "USE THE WALLS",
            objective_text = "Use the wall, bank the shot, and clear the room from the drift lane.",
            hud_objective_text = "Bank the shot. Clear the room.",
            mode = "campaign",
            room_type = "ricochet",
            circle_id = "lust",
            hard_cut = true,
            completion = "kill_all",
            trial_timer = 14,
            removed_ability = "grapple",
            environment_hook = "Crosswind bends the obvious shot off-course.",
            fragments_required = 0,
            show_gate = false,
            bg = lust.bg,
            solid_color = lust.solid,
            accent_color = lust.accent,
            abilities = { shoot = true, grapple = false, dash = true },
            npc_guidance = {
                {
                    speaker = "Virgil",
                    line = "Take the wall first. The straight line is the trap.",
                    trigger_radius = 72,
                },
            },
            qa_expectations = {
                requires_removed_ability_notice = true,
                requires_environment_label = true,
                requires_encounter_clear = true,
            },
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
                intro_title = "BANK SHOT",
                intro_subtitle = "The wall is part of the route now.",
                outro_subtitle = "Bank confirmed. Keep moving.",
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
            id = "circle1_05",
            title = "LUST",
            subtitle = "Prove it.",
            dramatic_question = "Can the player survive the first true subtraction and explain the new rule afterward?",
            transition_beat = "The gate room should confirm that Circle 1 is different, not merely harder.",
            trial_rule = "SURVIVE WITHOUT THE CHAIN",
            objective_text = "Survive the wind lane, hold the room, then take the gate.",
            hud_objective_text = "Survive the lane. Hold. Exit.",
            mode = "campaign",
            room_type = "pressure",
            circle_id = "lust",
            hard_cut = true,
            completion = "survive",
            trial_timer = 14,
            removed_ability = "grapple",
            environment_hook = "Wind and posture replace the chain.",
            fragments_required = 0,
            show_gate = false,
            bg = lust.bg,
            solid_color = lust.solid,
            accent_color = lust.accent,
            abilities = { shoot = true, grapple = false, dash = true, crouch = true, roll = true },
            qa_expectations = {
                requires_removed_ability_notice = true,
                requires_environment_label = true,
                requires_encounter_clear = true,
                min_rolls = 1,
                min_crouch_time = 1,
            },
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
                intro_title = "LUST GATE",
                intro_subtitle = "No chain. Read the wind. Keep your footing.",
                outro_subtitle = "The first subtraction holds. Advance.",
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
            bg = gluttony.bg,
            solid_color = gluttony.solid,
            accent_color = gluttony.accent,
            sludge_color = gluttony.sludge,
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
            bg = gluttony.bg,
            solid_color = gluttony.solid,
            accent_color = gluttony.accent,
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
            bg = gluttony.bg,
            solid_color = gluttony.solid,
            accent_color = gluttony.accent,
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
            bg = gluttony.bg,
            solid_color = gluttony.solid,
            accent_color = gluttony.accent,
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
            bg = gluttony.bg,
            solid_color = gluttony.solid,
            accent_color = gluttony.accent,
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
