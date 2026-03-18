// scene_lobby.c — STATE_LOBBY
#include "game_ctx.h"
#include <math.h>
#include <string.h>

extern GameCtx g;

void set_exposure(float exp);
void show_text(const char *text);
void transition_to(GameState s);

void lobby_load(void) {
    EnableCursor(); DisableCursor();
    build_lobby(&g.scene);
    init_player(&g.player, g.scene.spawn);
    StopAmbient(&g.audio);
    StartAmbient(&g.audio, DRONE_LOBBY);
    StopClockAmbient(&g.audio);
    StopCityAmbient(&g.audio);
    StopWindAmbient(&g.audio);
    StopMuffledPiano(&g.audio); StopDistantVoices(&g.audio); StopFootstepsAbove(&g.audio);
    PlayDistantVoices(&g.audio);
    SetSceneLighting(&g.lighting, LightingPreset_Lobby());
    set_exposure(0.0f);
    SetPostFXGrain(&g.postfx, 0.4f);
    // Gibbons
    {
        Vector3 lobby_wps[] = {
            {-3, 1.6f, -6},
            {-1, 1.6f, -4},
            {0, 1.6f, 2},
        };
        init_npc(&g.gibbons, (Vector3){-3, 1.6f, -7}, lobby_wps, 3, 2.0f, 4.0f);
        static const char *lobby_lines[] = {
            "You made it. I had a feeling.",
            "I've been here... a while. The building settles at night.",
            "The elevator. It goes further than you'd think.",
        };
        npc_set_dialogue(&g.gibbons, lobby_lines, 3, 3.0f);
    }
}

void lobby_update(float dt) {
    update_player(&g.player, &g.scene, dt);
    UpdateEVAudio(&g.audio, g.player.moving, g.player.sprinting, g.scene.surface, dt);
    update_npc(&g.gibbons, g.player.camera.position, &g.scene, dt);
    // Chandelier sway
    {
        float sway = sinf(g.state_time * 0.7f) * 0.3f;
        SetPointLightIdx(&g.lighting, 0,
            sway, 6.5f, -3.0f,
            0.9f, 0.75f, 0.45f, 10.0f);
    }
    if (IsKeyPressed(KEY_E)) {
        for (int i = 0; i < g.scene.object_count; i++) {
            InteractObject *obj = &g.scene.objects[i];
            if (!obj->active || obj->done) continue;
            float dist = Vector3Distance(g.player.camera.position, obj->pos);
            if (dist < obj->radius) {
                Vector3 to = Vector3Normalize(Vector3Subtract(obj->pos, g.player.camera.position));
                Vector3 look = Vector3Normalize(Vector3Subtract(g.player.camera.target, g.player.camera.position));
                if (Vector3DotProduct(to, look) > 0.5f) {
                    if (strcmp(obj->name, "elevator") == 0) {
                        obj->step++; obj->done = true;
                        PlayInteract(&g.audio, INTERACT_CLICK);
                        transition_to(STATE_ELEVATOR);
                        break;
                    }
                    if (strcmp(obj->name, "newspaper") == 0) {
                        obj->step++; obj->done = true;
                        PlayInteract(&g.audio, INTERACT_FABRIC);
                        show_text("ORBITAL SUITE OPENS — THREE HOURS TO KILL");
                        break;
                    }
                    if (strcmp(obj->name, "bell") == 0) {
                        obj->step++; obj->done = true;
                        PlayElevatorDing(&g.audio);
                        kick_camera(&g.player, -0.01f, 0.005f);
                        break;
                    }
                }
            }
        }
    }
    // Gibbons dialogue
    {
        const char *line = npc_current_dialogue(&g.gibbons);
        if (line) show_text(line);
    }
}
