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
    StopRain(&g.audio);
    StopMuffledPiano(&g.audio); StopDistantVoices(&g.audio); StopFootstepsAbove(&g.audio);
    PlayDistantVoices(&g.audio);
    SetSceneLighting(&g.lighting, LightingPreset_Lobby());
    set_exposure(0.0f);
    SetPostFXGrain(&g.postfx, 0.4f);
    // Guest book on reception desk — found text
    add_object(&g.scene, 3.5f, 1.05f, -2.0f, "guestbook", (Color){85,65,40,255}, 1);
    // Coffee cups on the table for two — one drunk, one untouched
    add_object(&g.scene, -4.5f, 0.72f, 2.0f, "coffeetable", (Color){220,215,205,230}, 1);
    // Gibbons
    {
        Vector3 lobby_wps[] = {
            {-3, 1.6f, -6},
            {-1, 1.6f, -4},
            {0, 1.6f, 2},
        };
        init_npc(&g.gibbons, (Vector3){-3, 1.6f, -7}, lobby_wps, 3, 2.0f, 4.0f);
        // Second playthrough: Gibbons has seen you before. He knows.
        static const char *lobby_lines_first[] = {
            "Ah. We have your reservation.",
            "The building makes sounds at night. They're structural.",
            "The elevator knows the way.",
        };
        static const char *lobby_lines_return[] = {
            "Back again.",
            "Same room. I kept it.",
            "You know the way.",
        };
        if (g.backstory_count > 3) {
            // Second playthrough — prologue + taxi choices already made
            npc_set_dialogue(&g.gibbons, lobby_lines_return, 3, 3.5f);
        } else {
            npc_set_dialogue(&g.gibbons, lobby_lines_first, 3, 3.5f);
        }
    }
}

void lobby_update(float dt) {
    update_player(&g.player, &g.scene, dt);
    UpdateEVAudio(&g.audio, g.player.moving, g.player.sprinting, g.scene.surface, dt);
    update_npc(&g.gibbons, g.player.camera.position, &g.scene, dt);

    // ── THE GLANCE — Commandment 9 ──
    // Gibbons looks behind you. Just once. Then catches himself.
    // He was expecting two. He sees one.
    // Perfect composure from then on. Bolaño politeness.
    if (!g.gibbons_glanced && g.gibbons.waiting && g.gibbons.current_waypoint == 0) {
        float dist = Vector3Distance(g.player.camera.position, g.gibbons.pos);
        if (dist < 5.0f) {
            // Look past the player — behind them, where she would be
            Vector3 behind = Vector3Subtract(g.player.camera.position, g.gibbons.pos);
            behind = Vector3Normalize(behind);
            g.gibbons.yaw_target = atan2f(behind.x, behind.z);
            g.gibbons_glanced = true;
            g.done_pause = -1.5f; // timer: look behind for 1.5s, then snap back
        }
    }
    // After the glance: smoothly return to facing player
    if (g.gibbons_glanced && g.done_pause < 0) {
        g.done_pause += dt;
        if (g.done_pause >= 0) {
            // He caught himself. Back to professional.
            g.done_pause = 0;
        }
    }

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
                    if (strcmp(obj->name, "guestbook") == 0) {
                        obj->step++; obj->done = true;
                        PlayInteract(&g.audio, INTERACT_FABRIC);
                        // The last entry. Two names crossed out, one written back.
                        if (g.backstory_count > 3)
                            show_text("Your name. Again.");
                        else
                            show_text("Two names on the booking. One crossed out.");
                        break;
                    }
                    if (strcmp(obj->name, "coffeetable") == 0) {
                        obj->step++; obj->done = true;
                        PlayInteract(&g.audio, INTERACT_CLICK);
                        show_text("Two cups. One cold.");
                        break;
                    }
                }
            }
        }
    }
    // Gibbons dialogue — handled by main.c dialogue system
}
