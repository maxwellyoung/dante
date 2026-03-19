// scene_balcony.c — STATE_BALCONY
#include "game_ctx.h"
#include <math.h>
#include <string.h>

extern GameCtx g;

void set_exposure(float exp);
void hard_cut_to(GameState s);
void show_text(const char *text);
void hide_text(void);

void balcony_load(void) {
    build_balcony(&g.scene);
    init_player(&g.player, g.scene.spawn);
    g.eiffel_sparkle = false; g.sparkle_timer = 0;
    g.cigarette_anim = false; g.cigarette_anim_timer = 0;
    StopAmbient(&g.audio);
    StopClockAmbient(&g.audio);
    StopCityAmbient(&g.audio);
    StopSuiteMusic(&g.audio);
    PlayBalconyMusic(&g.audio);
    PlayWindAmbient(&g.audio);
    PlayBalconyGust(&g.audio);
    g.player.gravity_mult = 0.3f;
    SetPostFXWarmth(&g.postfx, 1.0f);
    SetSceneLighting(&g.lighting, LightingPreset_Balcony());
    set_exposure(0.12f);
    SetPostFXGrain(&g.postfx, 0.5f);
}

void balcony_update(float dt) {
    // Agency control
    {
        float balc_ctrl = 0.3f;
        if (g.state_time > 10.0f) {
            float fade = fminf(1.0f, (g.state_time - 10.0f) / 4.0f);
            balc_ctrl = 0.3f * (1.0f - fade);
        }
        g.player.control_mult = balc_ctrl;
    }
    if (!g.cigarette_anim) update_player(&g.player, &g.scene, dt);
    // Railing approach
    {
        float railing_z = -1.5f;
        float dist_to_rail = g.player.camera.position.z - railing_z;
        if (dist_to_rail < 1.5f && dist_to_rail > 0) {
            float t = 1.0f - (dist_to_rail / 1.5f);
            g.player.fov_current += (80.0f - g.player.fov_current) * t * 0.06f;
            set_exposure(0.05f + t * 0.08f);
        }
    }
    UpdateEVAudio(&g.audio, g.player.moving && !g.cigarette_anim, g.player.sprinting, g.scene.surface, dt);
    if (IsKeyPressed(KEY_E)) {
        for (int i = 0; i < g.scene.object_count; i++) {
            InteractObject *obj = &g.scene.objects[i];
            if (!obj->active || obj->done) continue;
            float dist = Vector3Distance(g.player.camera.position, obj->pos);
            if (dist < obj->radius) {
                Vector3 to = Vector3Normalize(Vector3Subtract(obj->pos, g.player.camera.position));
                Vector3 look = Vector3Normalize(Vector3Subtract(g.player.camera.target, g.player.camera.position));
                if (Vector3DotProduct(to, look) > 0.5f) {
                    if (strcmp(obj->name, "cigarette") == 0) {
                        obj->step++; obj->done = true;
                        PlayInteract(&g.audio, INTERACT_CLICK);
                        g.cigarette_anim = true;
                        g.cigarette_anim_timer = 0;
                        g.cigarette_cam_origin = g.player.camera.position;
                        Vector3 to_cig = Vector3Normalize(Vector3Subtract(obj->pos, g.player.camera.position));
                        g.cigarette_cam_target = Vector3Add(g.player.camera.position, Vector3Scale(to_cig, 0.5f));
                        break;
                    }
                }
            }
        }
    }
    // Cigarette camera animation
    if (g.cigarette_anim) {
        g.cigarette_anim_timer += dt;
        float t = g.cigarette_anim_timer;
        if (t < 0.8f) {
            float s = t / 0.8f;
            s = s * s * (3.0f - 2.0f * s);
            g.player.camera.position = Vector3Lerp(g.cigarette_cam_origin, g.cigarette_cam_target, s);
        } else if (t < 1.3f) {
            g.player.camera.position = g.cigarette_cam_target;
        } else if (t < 2.1f) {
            float s = (t - 1.3f) / 0.8f;
            s = s * s * (3.0f - 2.0f * s);
            g.player.camera.position = Vector3Lerp(g.cigarette_cam_target, g.cigarette_cam_origin, s);
        } else {
            g.player.camera.position = g.cigarette_cam_origin;
            g.cigarette_anim = false;
        }
        g.player.camera.target = Vector3Add(g.player.camera.position, Vector3Normalize(Vector3Subtract(g.cigarette_cam_target, g.cigarette_cam_origin)));
    }
    if (!g.eiffel_sparkle && g.state_time > 0.5f) {
        g.eiffel_sparkle = true; g.sparkle_timer = 0;
        PlaySparkleSound(&g.audio);
    }
    if (g.eiffel_sparkle) g.sparkle_timer += dt;
    // Two chairs — the text appears when you're near the railing, looking out
    {
        static bool chairs_text_shown = false;
        float pz = g.player.camera.position.z;
        if (pz < -1.0f && !chairs_text_shown) {
            chairs_text_shown = true;
            show_text("Two chairs.");
        }
        // After cigarette animation — a beat of silence, then the thought
        if (g.cigarette_anim == false && g.cigarette_anim_timer > 2.0f) {
            static bool post_cig = false;
            if (!post_cig) {
                post_cig = true;
                hide_text();
                show_text("She would have loved this.");
            }
        }
    }
    // Rug pull flash
    if (g.state_time > 8.0f && !g.balcony_flash_triggered) {
        g.balcony_flash_triggered = true;
        g.balcony_flash_timer = 0;
    }
    if (g.balcony_flash_triggered) g.balcony_flash_timer += dt;
    if (g.state_time > 14.0f)
        hard_cut_to(STATE_PARIS_DREAM);
    if (g.state_time > 10 && IsKeyPressed(KEY_ENTER))
        hard_cut_to(STATE_PARIS_DREAM);
}
