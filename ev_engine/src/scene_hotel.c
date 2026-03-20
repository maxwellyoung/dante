// scene_hotel.c — STATE_HALLWAY, STATE_ROOM, STATE_BATHROOM
#include "game_ctx.h"
#include <math.h>
#include <string.h>

void set_exposure(float exp);
void show_text(const char *text);
void transition_to(GameState s);
void hard_cut_to(GameState s);
void load_state(GameState s);
InteractSoundType get_interact_sound_ext(const char *name);

void hallway_load(void) {
    build_hallway(&g.scene);
    init_player(&g.player, g.scene.spawn);
    StopAmbient(&g.audio);
    StartAmbient(&g.audio, DRONE_HALLWAY);
    StopClockAmbient(&g.audio);
    StopCityAmbient(&g.audio);
    StopWindAmbient(&g.audio);
    PlayMuffledPiano(&g.audio);
    PlayFootstepsAbove(&g.audio);
    SetSceneLighting(&g.lighting, LightingPreset_Hallway());
    set_exposure(-0.05f);
    SetPostFXGrain(&g.postfx, 0.5f);
}

void hallway_update(float dt) {
    update_player(&g.player, &g.scene, dt);
    UpdateEVAudio(&g.audio, g.player.moving, g.player.sprinting, g.scene.surface, dt);
    if (g.scene.has_exit) {
        float dist = Vector3Distance(g.player.camera.position, g.scene.exit_pos);
        if (dist < 1.5f) transition_to(STATE_ROOM);
    }
}

void room_load(void) {
    int saved_tasks = g.returning_to_room ? g.tasks_done : 0;
    bool saved_phone = g.returning_to_room ? g.phone_triggered : false;
    const char *saved_completed[MAX_OBJECTS] = {0};
    int saved_completed_count = 0;
    if (g.returning_to_room) {
        saved_completed_count = g.completed_count;
        for (int i = 0; i < g.completed_count && i < MAX_OBJECTS; i++)
            saved_completed[i] = g.completed_objects[i];
    }

    float saved_warmth = (float)saved_tasks / PARIS_TASK_COUNT;
    build_hotel_room(&g.scene);

    if (saved_tasks > 0) {
        init_player(&g.player, (Vector3){4.5f, 1.6f, -2.5f});
        g.tasks_done = saved_tasks;
        g.phone_triggered = saved_phone;
        SetPostFXWarmth(&g.postfx, saved_warmth);
        g.scene.fog_density = 0.004f - (saved_warmth * 0.003f);
        for (int i = 0; i < g.scene.object_count; i++) {
            for (int j = 0; j < saved_completed_count; j++) {
                if (saved_completed[j] && strcmp(g.scene.objects[i].name, saved_completed[j]) == 0) {
                    g.scene.objects[i].done = true;
                    g.scene.objects[i].step = g.scene.objects[i].max_steps;
                    break;
                }
            }
        }
        g.returning_to_room = false;
    } else {
        init_player(&g.player, g.scene.spawn);
        SetPostFXWarmth(&g.postfx, 0.0f);
    }

    StopAmbient(&g.audio);
    StartAmbient(&g.audio, DRONE_ROOM);
    PlayClockAmbient(&g.audio);
    StopCityAmbient(&g.audio);
    StopWindAmbient(&g.audio);
    PlayMuffledPiano(&g.audio);
    PlayFootstepsAbove(&g.audio);
    SetSceneLighting(&g.lighting, LightingPreset_Room());
    set_exposure(-0.05f);
    SetPostFXGrain(&g.postfx, 0.5f);
}

void room_update(float dt) {
    update_player(&g.player, &g.scene, dt);
    UpdateEVAudio(&g.audio, g.player.moving, g.player.sprinting, g.scene.surface, dt);
    if (IsKeyPressed(KEY_E)) {
        for (int i = 0; i < g.scene.object_count; i++) {
            InteractObject *obj = &g.scene.objects[i];
            if (!obj->active || obj->done) continue;
            float dist = Vector3Distance(g.player.camera.position, obj->pos);
            if (dist < obj->radius) {
                Vector3 to = Vector3Normalize(Vector3Subtract(obj->pos, g.player.camera.position));
                Vector3 look = Vector3Normalize(Vector3Subtract(g.player.camera.target, g.player.camera.position));
                if (Vector3DotProduct(to, look) > 0.5f) {
                    if (strcmp(obj->name, "bathroom") == 0) {
                        transition_to(STATE_BATHROOM);
                        break;
                    }
                    if (strcmp(obj->name, "wardrobe") == 0) {
                        obj->step++; obj->done = true;
                        PlayInteract(&g.audio, INTERACT_FABRIC);
                        kick_camera(&g.player, -0.03f, 0.01f);
                        StopClockAmbient(&g.audio);
                        load_state(STATE_SPACE_LOBBY);
                        g.player.camera.position = g.scene.exit_pos;
                        g.player.ground_y = 2.4f;
                        g.player.camera.target = (Vector3){
                            g.player.camera.position.x,
                            g.player.camera.position.y,
                            g.player.camera.position.z - 1
                        };
                        g.fade_alpha = 0.8f; g.fade_target = 0.0f;
                        break;
                    }
                    obj->step++;
                    PlayInteract(&g.audio, get_interact_sound_ext(obj->name));
                    kick_camera(&g.player, -0.01f, 0.005f);

                    // Per-step visual feedback
                    if (strcmp(obj->name, "lamp") == 0 && obj->step == 1) {
                        add_wall(&g.scene, -2.5f, 1.0f, -3.65f, 0.15f, 0.25f, 0.15f, (Color){220,210,185,180});
                    } else if (strcmp(obj->name, "candles") == 0 && obj->step == 1) {
                        add_wall(&g.scene, -4.0f, 0.55f, 3.0f, 0.06f, 0.12f, 0.06f, (Color){255,200,80,200});
                    } else if (strcmp(obj->name, "candles") == 0 && obj->step == 2) {
                        add_wall(&g.scene, -4.2f, 0.55f, 3.0f, 0.06f, 0.12f, 0.06f, (Color){255,200,80,200});
                    } else if (strcmp(obj->name, "bed") == 0 && obj->step == 1) {
                        add_wall(&g.scene, 0.0f, 0.54f, -3.3f, 2.8f, 0.02f, 1.4f, (Color){245,242,235,255});
                    } else if (strcmp(obj->name, "drawers") == 0 && obj->step == 1) {
                        add_wall(&g.scene, 2.0f, 0.4f, 3.4f, 0.5f, 0.04f, 0.35f, (Color){55,85,175,255});
                    } else if (strcmp(obj->name, "drawers") == 0 && obj->step == 2) {
                        add_wall(&g.scene, 2.5f, 0.4f, 3.3f, 0.35f, 0.04f, 0.4f, (Color){200,50,45,255});
                    }

                    if (obj->step >= obj->max_steps) {
                        obj->done = true;
                        obj->reward_timer = 1.5f;
                        g.tasks_done++;
                        if (g.completed_count < MAX_OBJECTS)
                            g.completed_objects[g.completed_count++] = obj->name;
                        PlayRewardSound(&g.audio);
                        float progress = (float)g.tasks_done / PARIS_TASK_COUNT;
                        SetPostFXWarmth(&g.postfx, progress);
                        g.scene.fog_density = 0.008f - (progress * 0.005f);
                        set_exposure(-0.05f + progress * 0.15f);

                        if (strcmp(obj->name, "lamp") == 0) {
                            add_light_panel(&g.scene, -2.5f, 1.2f, -3.8f, 0.5f, 0.6f, 0.5f, (Color){255,220,120,200});
                            SetPointLight(&g.lighting, -2.5f, 1.2f, -3.8f, 1.0f, 0.82f, 0.45f, 8.0f);
                        } else if (strcmp(obj->name, "candles") == 0) {
                            add_wall(&g.scene, -4.0f, 0.55f, 3.0f, 0.06f, 0.12f, 0.06f, (Color){255,180,60,220});
                            add_wall(&g.scene, -4.2f, 0.55f, 3.0f, 0.06f, 0.12f, 0.06f, (Color){255,190,70,200});
                            add_wall(&g.scene, -4.4f, 0.55f, 3.0f, 0.06f, 0.12f, 0.06f, (Color){255,170,50,210});
                            SetPointLight(&g.lighting, -4.2f, 0.55f, 3.0f, 1.0f, 0.7f, 0.25f, 5.0f);
                        } else if (strcmp(obj->name, "bed") == 0) {
                            add_wall(&g.scene, 0.0f, 0.68f, -3.2f, 0.18f, 0.04f, 0.12f, (Color){90,50,25,255});
                        } else if (strcmp(obj->name, "drawers") == 0) {
                            add_wall(&g.scene, 2.0f, 0.4f, 3.4f, 0.5f, 0.04f, 0.35f, (Color){55,85,175,255});
                            add_wall(&g.scene, 2.5f, 0.4f, 3.3f, 0.35f, 0.04f, 0.4f, (Color){200,50,45,255});
                            add_wall(&g.scene, 1.8f, 0.4f, 3.6f, 0.3f, 0.04f, 0.25f, (Color){240,238,230,255});
                        } else if (strcmp(obj->name, "ashtray") == 0) {
                            g.scene.fog_density = 0.001f;
                        }

                        if (g.tasks_done >= PARIS_TASK_COUNT) {
                            kick_camera(&g.player, -0.05f, 0.02f);
                            g.scene.fog_density = 0.001f;
                            SetPostFXWarmth(&g.postfx, 1.0f);
                            set_exposure(0.1f);
                        } else {
                            kick_camera(&g.player, -0.02f, 0.008f);
                        }
                    }
                    break;
                }
            }
        }
    }
    // Phone
    if (g.tasks_done >= 3 && !g.phone_triggered) {
        g.phone_triggered = true;
        add_wall(&g.scene, 5.2f, 0.87f, 0.15f, 0.2f, 0.01f, 0.1f, (Color){100,180,240,220});
        set_last_decal(&g.scene);
        g.phone_wall_idx = g.scene.wall_count - 1;
    }
    if (g.phone_triggered && g.phone_wall_idx >= 0) {
        float pulse = 0.6f + 0.4f * sinf((float)GetTime() * 3.0f);
        unsigned char pa = (unsigned char)(220 * pulse);
        g.scene.walls[g.phone_wall_idx].color.a = pa;
    }

    if (g.tasks_done >= PARIS_TASK_COUNT) {
        g.done_pause += dt;
        if (g.done_pause > 2.0f) {
            g.balcony_flash_triggered = false;
            g.balcony_flash_timer = 0;
            hard_cut_to(STATE_BALCONY);
        }
    }
}

void bathroom_load(void) {
    build_bathroom(&g.scene);
    init_player(&g.player, g.scene.spawn);
    StopAmbient(&g.audio);
    StartAmbient(&g.audio, DRONE_ROOM);
    StopClockAmbient(&g.audio);
    StopCityAmbient(&g.audio);
    StopWindAmbient(&g.audio);
    SetSceneLighting(&g.lighting, LightingPreset_Bathroom());
    set_exposure(0.1f);
    SetPostFXGrain(&g.postfx, 0.3f);
}

void bathroom_update(float dt) {
    update_player(&g.player, &g.scene, dt);
    UpdateEVAudio(&g.audio, g.player.moving, g.player.sprinting, g.scene.surface, dt);
    if (IsKeyPressed(KEY_E)) {
        for (int i = 0; i < g.scene.object_count; i++) {
            InteractObject *obj = &g.scene.objects[i];
            if (!obj->active || obj->done) continue;
            float dist = Vector3Distance(g.player.camera.position, obj->pos);
            if (dist < obj->radius) {
                Vector3 to = Vector3Normalize(Vector3Subtract(obj->pos, g.player.camera.position));
                Vector3 look = Vector3Normalize(Vector3Subtract(g.player.camera.target, g.player.camera.position));
                if (Vector3DotProduct(to, look) > 0.5f) {
                    if (strcmp(obj->name, "door") == 0) {
                        g.returning_to_room = true;
                        transition_to(STATE_ROOM);
                        break;
                    }
                    if (strcmp(obj->name, "tap") == 0) {
                        obj->step++; obj->done = true;
                        PlayInteract(&g.audio, INTERACT_CLICK);
                        kick_camera(&g.player, -0.01f, 0.005f);
                        add_wall(&g.scene, 2.0f, 0.78f, 0.5f, 0.06f, 0.14f, 0.04f, (Color){180,200,220,120});
                        break;
                    }
                    if (strcmp(obj->name, "mirror") == 0) {
                        obj->step++; obj->done = true;
                        PlayInteract(&g.audio, INTERACT_CLICK);
                        kick_camera(&g.player, -0.01f, 0.005f);
                        break;
                    }
                }
            }
        }
    }
}
