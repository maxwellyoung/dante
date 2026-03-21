// scene_corridor.c — STATE_SPACE_CORRIDOR
#include "game_ctx.h"
#include <math.h>

void set_exposure(float exp);
void transition_to(GameState s);
void SetPostFXWarmth(EVPostFX *pfx, float warmth);

void corridor_load(void) {
    build_space_corridor(&g.scene);
    // Second playthrough: the corridor is shorter. Literally.
    // "Same corridor. Shorter this time." — Gibbons was right.
    if (g.backstory_count > 3) {
        float shrink = 0.65f;
        for (int i = 0; i < g.scene.wall_count; i++) {
            g.scene.walls[i].pos.z *= shrink;
            g.scene.walls[i].size.z *= shrink;
        }
        g.scene.spawn.z *= shrink;
        if (g.scene.has_exit) g.scene.exit_pos.z *= shrink;
    }
    init_player(&g.player, g.scene.spawn);
    StopAmbient(&g.audio);
    StopEarthPresence(&g.audio);
    StopClockAmbient(&g.audio);
    StopCityAmbient(&g.audio);
    StopWindAmbient(&g.audio);
    PlayAirlockHiss(&g.audio);
    PlayDoorThud(&g.audio);
    g.player.gravity_mult = 0.4f;
    StartAmbient(&g.audio, DRONE_SPACE_CORRIDOR);
    PlayCorridorMusic(&g.audio);
    PlayDistantVoices(&g.audio);
    // Second playthrough: Four stopped playing. Six checked out.
    // The corridor is quieter. The player notices what's missing.
    if (g.backstory_count <= 3) {
        PlayMuffledPiano(&g.audio);
        PlayRunningWater(&g.audio);
    }
    PlayTvMurmur(&g.audio);
    g.corridor_ghost_delay = 0.0f;
    g.corridor_ghost_was_moving = false;
    {
        float zs = g.backstory_count > 3 ? 0.65f : 1.0f;
        g.door_positions[0] = (Vector3){-3.5f, 1.6f, 4.0f * zs};
        g.door_positions[1] = (Vector3){3.5f, 1.6f, 8.0f * zs};
        g.door_positions[2] = (Vector3){-3.5f, 1.6f, 12.0f * zs};
    }
    SetSceneLighting(&g.lighting, LightingPreset_SpaceCorridor());
    set_exposure(0.16f);
    SetPostFXGrain(&g.postfx, 0.28f);
    SetPostFXWarmth(&g.postfx, 0.05f);
    // Room service tray outside Door 2 (Room Six) — two plates, one untouched
    // Second playthrough: "Six checked out. The trays stopped coming."
    if (g.backstory_count <= 3) {
        float tx = 3.5f, tz = 7.2f;
        add_wall(&g.scene, tx - 1.0f, 0.02f, tz, 0.5f, 0.02f, 0.3f, (Color){190, 185, 180, 220});
        set_last_material(&g.scene, MAT_BRASS);
        set_last_decal(&g.scene);
        add_cylinder(&g.scene, tx - 1.15f, 0.05f, tz - 0.05f, 0.1f, 0.015f, (Color){240, 238, 232, 230});
        add_cylinder(&g.scene, tx - 0.85f, 0.05f, tz + 0.05f, 0.1f, 0.015f, (Color){240, 238, 232, 230});
        add_sphere(&g.scene, tx - 0.85f, 0.12f, tz + 0.05f, 0.1f, (Color){190, 185, 180, 200});
        set_last_material(&g.scene, MAT_BRASS);
    }
    // Do Not Disturb sign on Door 3
    add_wall(&g.scene, -3.5f + 0.03f, 1.3f, 11.6f, 0.15f, 0.08f, 0.005f, (Color){240, 238, 232, 200});
    set_last_decal(&g.scene);
    // Gibbons
    {
        float zs = g.backstory_count > 3 ? 0.65f : 1.0f;
        Vector3 corr_wps[] = {
            {0, 1.6f, 2 * zs},
            {0, 1.6f, 8 * zs},
            {0, 1.6f, 14 * zs},
        };
        init_npc(&g.gibbons, g.scene.spawn, corr_wps, 3, 3.5f, 4.0f);
        static const char *corr_lines_first[] = {
            "Someone in Four plays that same nocturne every evening.",
            "The gentleman in Six orders for two. Every night. Sends half back.",
            "Yours is at the end. I left the curtains open.",
        };
        static const char *corr_lines_return[] = {
            "Four stopped playing last week. Nobody noticed.",
            "Six checked out. The trays stopped coming.",
            "Yours is as you left it.",
        };
        if (g.backstory_count > 3)
            npc_set_dialogue(&g.gibbons, corr_lines_return, 3, 4.0f);
        else
            npc_set_dialogue(&g.gibbons, corr_lines_first, 3, 4.0f);
    }
}

void corridor_update(float dt) {
    update_player(&g.player, &g.scene, dt);
    UpdateEVAudio(&g.audio, g.player.moving, g.player.sprinting, g.scene.surface, dt);
    update_npc(&g.gibbons, g.player.camera.position, &g.scene, dt);
    // Gibbons behavior
    if (g.gibbons.waiting) {
        if (g.gibbons.current_waypoint == 1) {
            g.gibbons.behavior = NPC_GAZING;
            g.gibbons.yaw_target = 1.5708f;
        } else if (g.gibbons.current_waypoint == 2) {
            g.gibbons.behavior = NPC_GESTURING;
        }
    }
    // If the player bunny-hops far ahead, Gibbons stops and waits.
    // He doesn't hurry. He doesn't comment. Infinite patience.
    if (!g.gibbons.waiting && g.gibbons.active) {
        float ahead = g.player.camera.position.z - g.gibbons.pos.z;
        if (ahead > 6.0f) {
            // Player is way ahead — Gibbons slows to a stop
            g.gibbons.speed = 0.5f;
        } else if (ahead > 3.0f) {
            // Slightly ahead — Gibbons slows
            g.gibbons.speed = 2.0f;
        } else {
            // Normal — Gibbons walks his pace
            g.gibbons.speed = 3.5f;
        }
    }
    // ── PARALLEL FOOTSTEPS — someone behind the wall ──
    // Matches your pace. Stops when you stop. Starts a beat after you start.
    // First playthrough only — on replay, the corridor is emptier.
    if (g.backstory_count <= 3) {
        bool player_moving = g.player.moving;
        if (player_moving && !g.corridor_ghost_was_moving) {
            g.corridor_ghost_delay = 0.4f;  // beat of silence before they start
        }
        if (g.corridor_ghost_delay > 0) g.corridor_ghost_delay -= dt;
        bool ghost_moving = player_moving && g.corridor_ghost_delay <= 0;

        // Footstep volume — muffled through wall, positional
        float ghost_vol = ghost_moving ? 0.012f : 0;
        // Pan to the opposite wall from the player
        float ghost_pan = g.player.camera.position.x > 0 ? 0.2f : 0.8f;
        SetSoundVolume(g.audio.snd_footsteps_above, ghost_vol);
        SetSoundPan(g.audio.snd_footsteps_above, ghost_pan);
        if (ghost_moving) PlayFootstepsAbove(&g.audio);
        g.corridor_ghost_was_moving = player_moving;
    }

    // Speed modulation near windows
    {
        float px = fabsf(g.player.camera.position.x);
        if (px > 1.5f) {
            float wt = fminf(1.0f, (px - 1.5f) / 1.0f);
            float slow = 1.0f - wt * 0.35f;
            g.player.vel.x *= slow;
            g.player.vel.z *= slow;
        }
    }
    // Spatial compression
    if (g.scene.has_exit && g.player.vel.z > 0.1f) {
        g.player.vel.z *= 1.03f;
    }
    // Silence zones
    {
        float pz = g.player.camera.position.z;
        float silence = 1.0f;
        float d1 = fabsf(pz - 12.0f);
        if (d1 < 2.0f) {
            float t = 1.0f - (d1 / 2.0f);
            float s = 1.0f - t * 0.85f;
            if (s < silence) silence = s;
        }
        float d2 = fabsf(pz - 6.0f);
        if (d2 < 1.5f) {
            float t = 1.0f - (d2 / 1.5f);
            float s = 1.0f - t * 0.9f;
            if (s < silence) silence = s;
        }
        SetMasterVolume(silence);

        // Per-door spatial sounds
        for (int di = 0; di < 2; di++) {
            float ddx = g.player.camera.position.x - g.door_positions[di].x;
            float ddz = g.player.camera.position.z - g.door_positions[di].z;
            float ddist = sqrtf(ddx*ddx + ddz*ddz);
            float dvol = 1.0f / (1.0f + ddist * ddist);
            dvol *= silence * 0.03f;

            Vector3 fwd = Vector3Normalize(Vector3Subtract(g.player.camera.target, g.player.camera.position));
            Vector3 to_door = {-ddx, 0, -ddz};
            if (ddist > 0.01f) {
                to_door.x /= ddist; to_door.z /= ddist;
                float pan_raw = fwd.x * to_door.z - fwd.z * to_door.x;
                float pan = 0.5f + pan_raw * 0.4f;
                if (di == 0) SetSoundPan(g.audio.snd_muffled_piano, pan);
                else {
                    SetSoundPan(g.audio.snd_running_water, pan);
                    SetSoundPan(g.audio.snd_tv_murmur, pan);
                }
            }

            if (di == 0) {
                SetSoundVolume(g.audio.snd_muffled_piano, dvol);
                float light_fade = 1.0f;
                if (ddist < 4.0f) {
                    light_fade = ddist / 4.0f;
                    light_fade *= light_fade;
                }
                float breath = 0.85f + 0.15f * sinf(g.state_time * 1.5f);
                SetPointLightIdx(&g.lighting, 2,
                    g.door_positions[0].x, 0.05f, g.door_positions[0].z,
                    0.94f * light_fade * breath, 0.82f * light_fade * breath,
                    0.47f * light_fade * breath, 4.0f * light_fade);
            } else {
                // ── DOOR-LISTENING MECHANIC ──
                // TV murmur goes silent if you stand near the door for 6+ seconds.
                // They heard you. Or maybe they turned it off to sleep.
                // The silence IS the interaction.
                if (ddist < 2.5f) {
                    g.interaction_timers[1] += dt;
                } else {
                    // Reset slowly — they turn it back on after you leave
                    if (g.interaction_timers[1] > 0)
                        g.interaction_timers[1] -= dt * 0.3f;
                    if (g.interaction_timers[1] < 0) g.interaction_timers[1] = 0;
                }
                float listen_t = g.interaction_timers[1];
                float tv_fade = 1.0f;
                if (listen_t > 4.0f) {
                    // Fade out over 2 seconds (4s→6s)
                    tv_fade = 1.0f - fminf(1.0f, (listen_t - 4.0f) / 2.0f);
                }
                SetSoundVolume(g.audio.snd_running_water, dvol * 0.8f);
                SetSoundVolume(g.audio.snd_tv_murmur, dvol * tv_fade);
                float flk = 0.6f + 0.4f * sinf(g.state_time * 8.3f)
                           * sinf(g.state_time * 13.7f);
                // TV light also fades — they turned it off
                flk *= tv_fade;
                SetPointLightIdx(&g.lighting, 3,
                    g.door_positions[1].x, 0.05f, g.door_positions[1].z,
                    0.31f * flk, 0.47f * flk, 0.78f * flk, 3.0f * flk);
            }
        }

        // Dark door
        float d2x = g.player.camera.position.x - g.door_positions[2].x;
        float d2z = g.player.camera.position.z - g.door_positions[2].z;
        float dark_dist = sqrtf(d2x*d2x + d2z*d2z);
        if (dark_dist < 4.0f) {
            float dark_t = 1.0f - (dark_dist / 4.0f);
            dark_t *= dark_t;
            SetPostFXGrain(&g.postfx, 0.32f + dark_t * 0.20f);
            SetPostFXWarmth(&g.postfx, 0.05f - dark_t * 0.03f);
            set_exposure(0.12f - dark_t * 0.03f);
        }
    }
    if (g.scene.has_exit) {
        float dist = Vector3Distance(g.player.camera.position, g.scene.exit_pos);
        if (dist < 3.0f) {
            SetMasterVolume(1.0f);
            transition_to(STATE_SPACE_SUITE);
        }
    }
    if (!g.gibbons.active && g.gibbons.current_waypoint >= g.gibbons.waypoint_count) {
        g.done_pause += dt;
        if (g.done_pause > 1.5f) {
            SetMasterVolume(1.0f);
            transition_to(STATE_SPACE_SUITE);
        }
    }
}
