// scene_corridor.c — STATE_SPACE_CORRIDOR
#include "game_ctx.h"
#include <math.h>

extern GameCtx g;

void set_exposure(float exp);
void transition_to(GameState s);

void corridor_load(void) {
    build_space_corridor(&g.scene);
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
    PlayMuffledPiano(&g.audio);
    PlaySound(g.audio.snd_running_water);
    PlaySound(g.audio.snd_tv_murmur);
    g.door_positions[0] = (Vector3){-3.5f, 1.6f, 4.0f};
    g.door_positions[1] = (Vector3){3.5f, 1.6f, 8.0f};
    g.door_positions[2] = (Vector3){-3.5f, 1.6f, 12.0f};
    SetSceneLighting(&g.lighting, LightingPreset_SpaceCorridor());
    set_exposure(0.0f);
    SetPostFXGrain(&g.postfx, 0.4f);
    // Gibbons
    {
        Vector3 corr_wps[] = {
            {0, 1.6f, 2},
            {0, 1.6f, 8},
            {0, 1.6f, 14},
        };
        init_npc(&g.gibbons, g.scene.spawn, corr_wps, 3, 3.5f, 4.0f);
        static const char *corr_lines[] = {
            "The walls are thin. You hear things.",
            "I could tell you stories about this floor.",
            "Last door on the left. Your neighbor's quiet.",
        };
        npc_set_dialogue(&g.gibbons, corr_lines, 3, 3.5f);
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
                SetSoundVolume(g.audio.snd_running_water, dvol * 0.8f);
                SetSoundVolume(g.audio.snd_tv_murmur, dvol);
                float flk = 0.6f + 0.4f * sinf(g.state_time * 8.3f)
                           * sinf(g.state_time * 13.7f);
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
            SetPostFXGrain(&g.postfx, 0.4f + dark_t * 0.4f);
            set_exposure(0.0f - dark_t * 0.15f);
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
