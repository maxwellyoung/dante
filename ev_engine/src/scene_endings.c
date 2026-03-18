// scene_endings.c — STATE_BED, STATE_STARS, STATE_HYPERSPACE, STATE_PARIS_DREAM
#include "game_ctx.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

extern GameCtx g;

void set_exposure(float exp);
void show_text(const char *text);
void hide_text(void);
void hard_cut_to(GameState s);

void hyperspace_load(void) {
    build_hyperspace(&g.scene);
    init_player(&g.player, g.scene.spawn);
    g.player.camera.target = (Vector3){0, 0, -10};
    StopAmbient(&g.audio);
    StopClockAmbient(&g.audio);
    StopCityAmbient(&g.audio);
    StopWindAmbient(&g.audio);
    StopElevatorWhoosh(&g.audio);
    PlayHyperspaceTone(&g.audio);
    PlayHyperspaceRiser(&g.audio);
    SetSceneLighting(&g.lighting, LightingPreset_Hyperspace());
    set_exposure(0.2f);
    SetPostFXWarmth(&g.postfx, 0.3f);
    SetPostFXGrain(&g.postfx, 0.6f);
    SetPostFXCA(&g.postfx, 5.0f);
}

void hyperspace_update(float dt) {
    (void)dt;
    float t = g.state_time;

    if (t < 4.0f) {
        float speed = 8.0f + t * t * 3.0f;
        g.player.camera.position.z -= speed * GetFrameTime();
        g.player.camera.target.z -= speed * GetFrameTime();

        if (t < 2.0f) {
            g.player.camera.fovy = 70.0f + t * 7.5f;
        } else {
            float p2 = (t - 2.0f) / 2.0f;
            g.player.camera.fovy = 85.0f - p2 * 25.0f;
            SetMasterVolume(1.0f - p2 * 0.7f);
        }

        float roll = sinf(t * 1.5f) * 2.0f;
        g.player.camera.up = (Vector3){sinf(roll * DEG2RAD), cosf(roll * DEG2RAD), 0};

        float ca = 5.0f + t * 4.0f;
        SetPostFXCA(&g.postfx, ca > 20.0f ? 20.0f : ca);
        SetPostFXGrain(&g.postfx, 0.7f + t * 0.1f);
        set_exposure(0.1f + sinf(t * 3.0f) * 0.08f);
        float sat = 0.8f + sinf(t * 2.0f) * 0.3f;
        SetPostFXSaturation(&g.postfx, sat > 1.3f ? 1.3f : sat);
    } else if (t < 4.5f) {
        SetMasterVolume(0.0f);
        set_exposure(-1.0f);
    } else if (t < 6.0f) {
        float p4 = (t - 4.5f) / 1.5f;
        g.player.camera.fovy = 60.0f + p4 * 30.0f;
        SetMasterVolume(p4);
        set_exposure(-1.0f + p4 * 1.1f);
    }

    if (t > 6.0f) {
        StopHyperspaceTone(&g.audio);
        StopHyperspaceRiser(&g.audio);
        SetPostFXCA(&g.postfx, 2.5f);
        SetPostFXSaturation(&g.postfx, 0.92f);
        g.player.camera.fovy = 70.0f;
        g.player.camera.up = (Vector3){0, 1, 0};
        g.transition_hold = 1.5f;
        SetMasterVolume(0.0f);
        hard_cut_to(STATE_SPACE_LOBBY);
    }
}

void bed_load(void) {
    StopAmbient(&g.audio); StopCityAmbient(&g.audio); StopWindAmbient(&g.audio);
    StopSuiteMusic(&g.audio); StopBalconyMusic(&g.audio);
    PlayBedImpact(&g.audio);
    g.bed_clock_rate = 1.0f;
    PlayClockAmbient(&g.audio);
    // THE composed piece — plays once, never repeats. The emotional center.
    PlayBedRitual(&g.audio);
}

void bed_update(float dt) {
    (void)dt;
    // Clock deceleration
    if (g.state_time < 8.0f) {
        g.bed_clock_rate = 1.0f - (g.state_time / 8.0f);
        if (g.bed_clock_rate < 0.05f) g.bed_clock_rate = 0;
        SetClockRate(&g.audio, g.bed_clock_rate);
    } else if (g.bed_clock_rate > 0) {
        g.bed_clock_rate = 0;
        SetClockRate(&g.audio, 0);
    }
    if (g.state_time > 3.0f && !g.audio.bed_drone_playing) {
        PlayBedDrone(&g.audio);
    }
    // Camera breathing
    {
        float breath = sinf(g.state_time * 0.5f) * 0.002f;
        g.player.camera.target.y += breath;
    }
    // Progressive desaturation
    {
        float desat = fminf(1.0f, g.state_time / 16.0f);
        SetPostFXSaturation(&g.postfx, 0.92f - desat * 0.6f);
        SetPostFXContrast(&g.postfx, 1.0f - desat * 0.4f);
        SetPostFXGrain(&g.postfx, 0.3f + desat * 0.5f);
    }
    if (g.state_time > 20)
        hard_cut_to(STATE_STARS);
    // Breathing
    {
        float breath_rate = 0.4f;
        float breath_amp = 0.015f;
        float breath = sinf(g.state_time * breath_rate * 2 * 3.14159f) * breath_amp;
        g.player.camera.position.y += breath * GetFrameTime() * 2.0f;
    }
}

void stars_load(void) {
    StopAmbient(&g.audio); StopClockAmbient(&g.audio); StopCityAmbient(&g.audio); StopWindAmbient(&g.audio);
    StopBedDrone(&g.audio);
    StopBedRitual(&g.audio);
    PlayHeldChord(&g.audio);
    SetPostFXWarmth(&g.postfx, 0.0f);
    g.three_note_played = false;
    int rt_m = (int)g.total_time / 60, rt_s = (int)g.total_time % 60;
    printf("\n[PLAYTHROUGH] Total runtime: %d:%02d\n\n", rt_m, rt_s);
}

void stars_update(float dt) {
    (void)dt;
    if (g.state_time < 3.0f) {
        SetMasterVolume(g.state_time / 3.0f);
    }
    // Camera drift
    {
        float drift_yaw = g.state_time * 0.003f;
        float drift_pitch = sinf(g.state_time * 0.08f) * 0.002f;
        Vector3 fwd = Vector3Normalize(Vector3Subtract(
            g.player.camera.target, g.player.camera.position));
        Vector3 rt = Vector3Normalize(Vector3CrossProduct(fwd, g.player.camera.up));
        fwd = Vector3Transform(fwd, MatrixRotateY(drift_yaw * GetFrameTime()));
        fwd = Vector3Transform(fwd, MatrixRotate(rt, drift_pitch * GetFrameTime()));
        g.player.camera.target = Vector3Add(g.player.camera.position, fwd);
    }
    // PostFX recovery
    if (g.state_time < 1.0f) {
        SetPostFXSaturation(&g.postfx, 0.32f + g.state_time * 0.6f);
        SetPostFXContrast(&g.postfx, 0.6f + g.state_time * 0.4f);
        SetPostFXGrain(&g.postfx, 0.8f - g.state_time * 0.5f);
    }
    if (IsKeyPressed(KEY_ESCAPE)) { CloseWindow(); }
    // Three-note callback — the bed ritual returns as a fragment
    if (g.state_time > 10.0f && !g.three_note_played) {
        StopHeldChord(&g.audio);  // held chord fades, three notes take over
        PlayThreeNote(&g.audio);
        g.three_note_played = true;
    }
    // Waking up
    if (g.state_time > 13.0f) {
        float wake = (g.state_time - 13.0f) / 2.0f;
        SetPostFXWarmth(&g.postfx, wake * 0.5f);
        set_exposure(wake * 0.3f);
        SetMasterVolume(fmaxf(0, 1.0f - wake));
    }
    if (g.state_time > 15.0f || (g.state_time > 8.0f && IsKeyPressed(KEY_ENTER))) {
        SetMasterVolume(1.0f);
        SetPostFXWarmth(&g.postfx, 0.0f);
        hard_cut_to(STATE_RETURN_TAXI);
    }
}

void paris_dream_load(void) {
    build_hotel_room(&g.scene);
    init_player(&g.player, g.scene.spawn);
    g.player.control_mult = 0.5f;
    g.player.gravity_mult = 0.7f;

    // ── HER PRESENCE — she was just here ──
    // The Paris dream is a memory. The last good trip.

    // Her coat on the chair — draped, not hung. She'll be right back.
    add_wall(&g.scene, 2.0f, 0.8f, -2.0f, 0.5f, 0.7f, 0.08f, (Color){55,45,40,220});
    set_last_material(&g.scene, MAT_FABRIC);

    // Coffee for two — one cup half-drunk, one untouched
    add_cylinder(&g.scene, -1.5f, 0.48f, 0.5f, 0.05f, 0.08f, (Color){220,215,205,230});
    add_cylinder(&g.scene, -1.5f, 0.44f, 0.5f, 0.04f, 0.04f, (Color){45,30,20,200});
    add_cylinder(&g.scene, -1.2f, 0.48f, 0.7f, 0.05f, 0.08f, (Color){220,215,205,230});
    add_cylinder(&g.scene, -1.2f, 0.45f, 0.7f, 0.04f, 0.06f, (Color){45,30,20,200});

    // Fresh pillow indent — her side
    add_wall(&g.scene, -0.4f, 0.56f, -3.6f, 0.45f, 0.06f, 0.3f, (Color){235,232,225,255});
    set_last_material(&g.scene, MAT_FABRIC);

    // THE RED OBJECT — one color survives the B&W.
    // A scarf on the bed. Godard red. Memory preserves color selectively.
    add_wall(&g.scene, 0.3f, 0.62f, -3.2f, 0.6f, 0.02f, 0.15f, (Color){200,50,45,255});
    set_last_material(&g.scene, MAT_FABRIC);
    set_last_decal(&g.scene);

    // HER TOOTHBRUSH — in the bathroom glass by the sink.
    // The most mundane detail. The most devastating on replay.
    // A cylinder (glass) + thin cylinder (toothbrush) on the sink counter.
    add_cylinder(&g.scene, 2.0f, 0.95f, 0.8f, 0.03f, 0.1f, (Color){220,225,230,160}); // glass
    add_cylinder(&g.scene, 2.02f, 1.08f, 0.8f, 0.01f, 0.12f, (Color){60,140,180,255}); // toothbrush — blue
    set_last_material(&g.scene, MAT_CONCRETE);

    // BOARDING PASS BOOKMARK — in her book on the bed.
    // The same book as the suite nightstand, but here it's open, further along.
    // The bookmark IS the boarding pass. Her name on it. The timeline connects.
    // Book (open, spine up) — Godard red, same as suite but further read
    add_wall(&g.scene, 0.8f, 0.65f, -3.8f, 0.4f, 0.08f, 0.3f, (Color){200,50,45,255});
    set_last_material(&g.scene, MAT_FABRIC);
    // Pages visible (open book, spine cracked further)
    add_wall(&g.scene, 0.8f, 0.69f, -3.8f, 0.38f, 0.01f, 0.28f, (Color){240,235,225,255});
    set_last_decal(&g.scene);
    // Boarding pass bookmark — white rectangle with blue airline stripe, peeking out
    add_wall(&g.scene, 0.95f, 0.70f, -3.65f, 0.15f, 0.005f, 0.08f, (Color){245,242,235,255});
    set_last_decal(&g.scene);
    add_wall(&g.scene, 0.95f, 0.705f, -3.68f, 0.15f, 0.003f, 0.02f, (Color){50,80,180,255});
    set_last_decal(&g.scene);

    // Eiffel Tower silhouette through window
    add_object(&g.scene, -2.5f, 0.62f, -3.5f, "photograph", (Color){240,238,230,255}, 1);
    add_object(&g.scene, -5.5f, 1.5f, -1.0f, "window", (Color){120,150,220,100}, 1);
    add_wall(&g.scene, -6.5f, 2.5f, -1.0f, 0.06f, 2.0f, 0.06f, (Color){40,35,30,60});
    add_wall(&g.scene, -6.5f, 3.5f, -1.0f, 0.6f, 0.04f, 0.04f, (Color){40,35,30,40});
    add_wall(&g.scene, -6.5f, 3.0f, -1.0f, 0.35f, 0.04f, 0.04f, (Color){40,35,30,45});

    // Dream geometry compression — proportions wrong, memory distorts
    for (int i = 0; i < g.scene.wall_count; i++) {
        g.scene.walls[i].pos.x *= 0.85f;
        g.scene.walls[i].size.x *= 0.85f;
    }

    StopAmbient(&g.audio);
    StopClockAmbient(&g.audio);
    StopWindAmbient(&g.audio);
    StartAmbient(&g.audio, DRONE_ROOM);
    SetSceneLighting(&g.lighting, LightingPreset_ParisDream());
    set_exposure(0.1f);
    SetPostFXWarmth(&g.postfx, 0.7f);
    SetPostFXGrain(&g.postfx, 0.7f);
    SetPostFXSaturation(&g.postfx, 1.2f);
    SetPostFXCA(&g.postfx, 3.5f);
    SetPostFXVignette(&g.postfx, 2.0f);
    SetPostFXContrast(&g.postfx, 1.5f);
    g.scene.fog_color = (Color){30, 22, 10, 255};
    g.scene.fog_density = 0.005f;
}

void paris_dream_update(float dt) {
    update_player(&g.player, &g.scene, dt);
    // Dream shimmer
    {
        float dream_t = fminf(1.0f, g.state_time / 5.0f);
        float shimmer = 0.7f + 0.1f * sinf(g.state_time * 0.8f);
        SetPostFXWarmth(&g.postfx, shimmer * dream_t);
        if (g.state_time > 30.0f) {
            float dream_fade = fminf(1.0f, (g.state_time - 30.0f) / 60.0f);
            g.player.control_mult = 1.0f - dream_fade * 0.5f;
        }
        SetPostFXGrain(&g.postfx, 0.7f + g.state_time * 0.003f);
    }
    // Interactions
    if (IsKeyPressed(KEY_E)) {
        for (int i = 0; i < g.scene.object_count; i++) {
            InteractObject *obj = &g.scene.objects[i];
            if (!obj->active || obj->done) continue;
            float dist = Vector3Distance(g.player.camera.position, obj->pos);
            if (dist < obj->radius) {
                Vector3 to = Vector3Normalize(Vector3Subtract(obj->pos, g.player.camera.position));
                Vector3 look = Vector3Normalize(Vector3Subtract(g.player.camera.target, g.player.camera.position));
                if (Vector3DotProduct(to, look) > 0.4f) {
                    if (strcmp(obj->name, "photograph") == 0) {
                        obj->step++; obj->done = true;
                        kick_camera(&g.player, -0.04f, 0.02f);
                        show_text("It's blank.");
                        g.done_pause = -2.0f;
                    } else if (strcmp(obj->name, "window") == 0) {
                        obj->step++; obj->done = true;
                        kick_camera(&g.player, -0.02f, 0.01f);
                        show_text("Paris. Before all this.");
                    }
                }
                break;
            }
        }
    }
    // Dream exit
    if (g.done_pause < 0) {
        g.done_pause += dt;
        if (g.done_pause >= 0) {
            hide_text();
            SetPostFXWarmth(&g.postfx, 0.0f);
            SetPostFXSaturation(&g.postfx, 0.92f);
            SetPostFXCA(&g.postfx, 2.5f);
            SetPostFXVignette(&g.postfx, 1.0f);
            SetPostFXContrast(&g.postfx, 1.0f);
            hard_cut_to(STATE_CLEANED_SUITE);
        }
    }
    if (g.state_time > 90.0f) {
        SetPostFXWarmth(&g.postfx, 0.0f);
        SetPostFXSaturation(&g.postfx, 0.92f);
        SetPostFXCA(&g.postfx, 2.5f);
        SetPostFXVignette(&g.postfx, 1.0f);
        SetPostFXContrast(&g.postfx, 1.0f);
        hard_cut_to(STATE_CLEANED_SUITE);
    }
}
