// scene_elevator.c — STATE_ELEVATOR
#include "game_ctx.h"
#include <math.h>

extern GameCtx g;

void set_exposure(float exp);
void hard_cut_to(GameState s);

void elevator_load(void) {
    if (g.elevator_to_corridor) {
        build_elevator_space(&g.scene);
    } else {
        build_elevator(&g.scene);
    }
    init_player(&g.player, g.scene.spawn);
    g.elevator_ding_played = false;
    StopAmbient(&g.audio);
    StopClockAmbient(&g.audio);
    StopCityAmbient(&g.audio);
    StopWindAmbient(&g.audio);
    StopMuffledPiano(&g.audio); StopDistantVoices(&g.audio); StopFootstepsAbove(&g.audio);
    StopEarthPresence(&g.audio);
    PlayElevatorHum(&g.audio);

    // Gibbons — Hotel Chevalier energy. Two people in a small space
    // with unspoken knowledge. He adjusts his tie. Professional silence.
    if (g.elevator_to_corridor) {
        Vector3 elev_wps[] = { {0, 1.6f, -0.5f} };
        init_npc(&g.gibbons, (Vector3){0.5f, 1.6f, -0.5f}, elev_wps, 1, 0.5f, 2.0f);
        g.gibbons.behavior = NPC_WAITING;
        static const char *elev_lines[] = { "Almost there." };
        npc_set_dialogue(&g.gibbons, elev_lines, 1, 3.0f);
    } else {
        Vector3 elev_wps[] = { {0.5f, 1.6f, 0} };
        init_npc(&g.gibbons, (Vector3){0.5f, 1.6f, 0}, elev_wps, 1, 0.5f, 2.0f);
        g.gibbons.behavior = NPC_WAITING;
        static const char *elev_lines[] = { "Going up." };
        npc_set_dialogue(&g.gibbons, elev_lines, 1, 2.5f);
    }

    if (g.elevator_to_corridor) {
        PlayDoorThud(&g.audio);
        g.player.gravity_mult = 0.4f;
        SetSceneLighting(&g.lighting, LightingPreset_ElevatorSpace());
        set_exposure(-0.05f);
        SetPostFXGrain(&g.postfx, 0.35f);
    } else {
        PlayElevatorWhoosh(&g.audio);
        g.player.gravity_mult = 1.0f;
        SetSceneLighting(&g.lighting, LightingPreset_Elevator());
        set_exposure(0.0f);
        SetPostFXGrain(&g.postfx, 0.3f);
    }
}

void elevator_update(float dt) {
    // Gibbons rides with you — professional silence, small space
    update_npc(&g.gibbons, g.player.camera.position, &g.scene, dt);
    // Gibbons rises with the elevator
    if (g.elevator_to_corridor) {
        g.gibbons.pos.y = g.player.camera.position.y - 0.0f;
    }

    if (g.elevator_to_corridor) {
        // SPACE ELEVATOR
        {
            float t = g.state_time;
            float speed = 0;
            if (t < 0.3f) {
                float jolt = t / 0.3f;
                speed = jolt * jolt * 2.0f;
                float shake = (1.0f - jolt) * 0.008f;
                g.player.camera.position.x += sinf(t * 60) * shake;
                g.player.camera.target.x += sinf(t * 60) * shake;
            } else if (t < 3.0f) {
                speed = 2.0f;
                float sway = sinf(t * 2.5f) * 0.003f;
                g.player.camera.position.x += sway;
                g.player.camera.target.x += sway;
            } else if (t < 3.8f) {
                float decel = 1.0f - (t - 3.0f) / 0.8f;
                speed = 2.0f * decel * decel;
            }
            g.player.camera.position.y += speed * dt;
            g.player.camera.target.y += speed * dt;
        }
        if (g.state_time > 3.5f && !g.elevator_ding_played) {
            g.elevator_ding_played = true;
            PlayElevatorDing(&g.audio);
        }
        if (g.state_time > 4.2f) {
            g.elevator_to_corridor = false;
            hard_cut_to(STATE_SPACE_CORRIDOR);
        }
    } else {
        // TERRESTRIAL ELEVATOR
        {
            float t = g.state_time;
            float accel = 0.3f + t * 0.4f;
            g.player.camera.position.y += accel * dt;
            g.player.camera.target.y += accel * dt;
            if (t < 3.0f) {
                float vib = (1.0f - t / 3.0f) * 0.004f;
                g.player.camera.position.x += sinf(t * 40) * vib;
            }
            if (t > 2.0f) {
                float dissolve = (t - 2.0f) / 3.0f;
                if (dissolve > 1.0f) dissolve = 1.0f;
                SetPostFXGrain(&g.postfx, 0.3f + dissolve * 0.5f);
                SetPostFXSaturation(&g.postfx, 1.0f - dissolve * 0.3f);
                SetPostFXWarmth(&g.postfx, dissolve * 0.15f);
            }
            if (t > 3.0f) {
                float narrow = (t - 3.0f) / 2.0f;
                if (narrow > 1.0f) narrow = 1.0f;
                g.player.camera.fovy = 70.0f - narrow * 15.0f;
            }
        }
        if (g.state_time > 2.0f && !g.elevator_ding_played) {
            g.elevator_ding_played = true;
            PlayElevatorDing(&g.audio);
        }
        if (g.state_time > 5.0f) {
            g.player.camera.fovy = 70.0f;
            hard_cut_to(STATE_HYPERSPACE);
        }
    }
}
