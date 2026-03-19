// scene_space_lobby.c — STATE_SPACE_LOBBY
#include "game_ctx.h"
#include <math.h>
#include <string.h>

extern GameCtx g;

void set_exposure(float exp);
void show_text(const char *text);
void transition_to(GameState s);

void space_lobby_load(void) {
    build_space_lobby(&g.scene);
    init_player(&g.player, g.scene.spawn);
    g.elevator_ding_played = false;
    StopAmbient(&g.audio);
    StopClockAmbient(&g.audio);
    StopCityAmbient(&g.audio);
    StopWindAmbient(&g.audio);
    StopMuffledPiano(&g.audio); StopDistantVoices(&g.audio); StopFootstepsAbove(&g.audio);
    StopHyperspaceTone(&g.audio);
    StopHyperspaceRiser(&g.audio);
    PlayArrivalThump(&g.audio);
    PlayGravitySettle(&g.audio);
    PlayAirlockHiss(&g.audio);
    PlayEarthPresence(&g.audio);
    g.player.gravity_mult = 0.4f;
    StartAmbient(&g.audio, DRONE_SPACE_LOBBY);
    SetSceneLighting(&g.lighting, LightingPreset_SpaceLobby());
    set_exposure(-0.05f);
    SetPostFXGrain(&g.postfx, 0.4f);
    SetPostFXCA(&g.postfx, 2.5f);
    g.lobby_visit_count++;
    if (g.lobby_visit_count > 1) {
        float revisit_warmth = fminf(0.15f, (g.lobby_visit_count - 1) * 0.08f);
        SetPostFXWarmth(&g.postfx, revisit_warmth);
        set_exposure(-0.05f + revisit_warmth * 0.1f);
    }
    // Gibbons
    {
        Vector3 lobby_wps[] = {
            {3, 1.6f, 2},
            {6, 1.6f, -3},
            {0, 1.6f, -4},
            {0, 1.6f, 0},
        };
        init_npc(&g.gibbons, (Vector3){2, 1.6f, 4}, lobby_wps, 4, 2.5f, 3.5f);
        static const char *lobby_lines_first[] = {
            "The room's been ready for some time.",
            "The window. Most people need a moment.",
            "The corridor's longer than it looks.",
            "I'll be nearby.",
        };
        static const char *lobby_lines_return[] = {
            "I thought you might come back.",
            "The window hasn't moved.",
            "Same corridor. Shorter this time.",
            "You know where to find me.",
        };
        if (g.backstory_count > 3)
            npc_set_dialogue(&g.gibbons, lobby_lines_return, 4, 3.5f);
        else
            npc_set_dialogue(&g.gibbons, lobby_lines_first, 4, 3.5f);
    }
}

void space_lobby_update(float dt) {
    update_player(&g.player, &g.scene, dt);
    if (g.state_time > 1.5f && !g.elevator_ding_played) {
        g.elevator_ding_played = true;
        PlayElevatorDing(&g.audio);
    }
    // Speed manipulation near observation window
    {
        float pz = g.player.camera.position.z;
        if (pz < -2.0f) {
            float t = fminf(1.0f, (-2.0f - pz) / 5.5f);
            float slow = 1.0f - t * 0.55f;
            g.player.vel.x *= slow;
            g.player.vel.z *= slow;
            g.player.fov_current += (60.0f - g.player.fov_current) * t * 0.04f;
            set_exposure(-0.05f + t * 0.12f);
            SetPostFXGrain(&g.postfx, 0.4f - t * 0.2f);
        }
        // Earth glow pulse
        {
            float pulse = 0.8f + 0.2f * sinf(g.state_time * 0.5f);
            SetPointLightIdx(&g.lighting, 0,
                0.0f, 0.5f, -7.0f,
                0.24f * pulse, 0.51f * pulse, 0.78f * pulse,
                12.0f * pulse);
            float ch_flk = 0.92f + 0.08f * sinf(g.state_time * 3.1f) * sinf(g.state_time * 5.7f);
            SetPointLightIdx(&g.lighting, 3,
                0, 8.0f, -3.0f,
                0.7f * ch_flk, 0.55f * ch_flk, 0.35f * ch_flk, 10.0f);
            float window_dist = fabsf(pz + 7.0f);
            float sub_vol = (window_dist < 5.0f)
                ? 0.08f * (1.0f - window_dist / 5.0f) : 0.0f;
            SetEarthPresenceVolume(&g.audio, sub_vol * pulse);
        }
        // Gibbons head turn — he watches you see it
        {
            static bool gibbons_looked = false;
            if (pz < -3.0f && !gibbons_looked && g.gibbons.active) {
                g.gibbons.yaw = atan2f(-8.0f - g.gibbons.pos.z,
                                       0.0f - g.gibbons.pos.x);
                gibbons_looked = true;
            }
        }
        // Earth text — once, when you reach the glass
        {
            static bool earth_text_shown = false;
            if (pz < -5.0f && !earth_text_shown) {
                earth_text_shown = true;
                show_text("Oh.");
            }
            if (pz > -4.0f && earth_text_shown && g.vig_text != NULL) {
                // Walking away clears it
            }
        }
    }
    UpdateEVAudio(&g.audio, g.player.moving, g.player.sprinting, g.scene.surface, dt);
    update_npc(&g.gibbons, g.player.camera.position, &g.scene, dt);
    if (IsKeyPressed(KEY_E)) {
        for (int i = 0; i < g.scene.object_count; i++) {
            InteractObject *obj = &g.scene.objects[i];
            if (!obj->active || obj->done) continue;
            float dist = Vector3Distance(g.player.camera.position, obj->pos);
            if (dist < obj->radius) {
                Vector3 to = Vector3Normalize(Vector3Subtract(obj->pos, g.player.camera.position));
                Vector3 look = Vector3Normalize(Vector3Subtract(g.player.camera.target, g.player.camera.position));
                if (Vector3DotProduct(to, look) > 0.5f) {
                    obj->step++; obj->done = true;
                    PlayInteract(&g.audio, INTERACT_CLICK);
                    kick_camera(&g.player, -0.01f, 0.005f);
                    break;
                }
            }
        }
    }
    // Glass elevator
    {
        float elev_dist = sqrtf(
            g.player.camera.position.x * g.player.camera.position.x +
            g.player.camera.position.z * g.player.camera.position.z);
        if (elev_dist < 1.5f && !g.gibbons.active) {
            show_text("Going up.");
            g.elevator_to_corridor = true;
            transition_to(STATE_ELEVATOR);
        }
    }
}
