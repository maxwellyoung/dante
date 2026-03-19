// scene_exterior.c — STATE_HOTEL_EXT
#include "game_ctx.h"
#include <math.h>

extern GameCtx g;

void set_exposure(float exp);
void transition_to(GameState s);

void show_text(const char *text);
void hide_text(void);

void exterior_load(void) {
    build_hotel_exterior(&g.scene);
    init_player(&g.player, g.scene.spawn);
    StopAmbient(&g.audio);
    StopClockAmbient(&g.audio);
    PlayCityAmbient(&g.audio);
    PlayWindAmbient(&g.audio);
    PlayRain(&g.audio);
    SetSceneLighting(&g.lighting, LightingPreset_Exterior());
    set_exposure(0.0f);
    SetPostFXWarmth(&g.postfx, 0.05f);
    SetPostFXGrain(&g.postfx, 0.6f);
}

void exterior_update(float dt) {
    update_player(&g.player, &g.scene, dt);
    UpdateEVAudio(&g.audio, g.player.moving, g.player.sprinting, g.scene.surface, dt);
    // Sky Tower beacon
    {
        float beacon = sinf(g.state_time * 3.14159f) * 0.5f + 0.5f;
        beacon = beacon * beacon;
        SetPointLightIdx(&g.lighting, 2,
            0, 52, 8,
            beacon * 0.8f, beacon * 0.05f, beacon * 0.05f,
            beacon * 15.0f);
    }
    // Looking up awe — the tower, first time you see it on foot
    if (g.player.camera.target.y > g.player.camera.position.y + 0.3f) {
        float look_up = fminf(1.0f,
            (g.player.camera.target.y - g.player.camera.position.y - 0.3f) / 0.5f);
        g.player.fov_current += (78.0f - g.player.fov_current) * look_up * 0.05f;
    }
    // Auckland at 2 AM — timed text beats
    {
        static bool ext_text_1 = false;
        static bool ext_text_2 = false;
        // First beat — after the player has a second to look around
        if (g.state_time > 2.0f && !ext_text_1) {
            ext_text_1 = true;
            show_text("The rain doesn't know it's 2 AM.");
        }
        if (g.state_time > 5.5f && ext_text_1 && !ext_text_2) {
            hide_text();
            ext_text_2 = true;
        }
    }
    if (g.scene.has_exit) {
        float dist = Vector3Distance(g.player.camera.position, g.scene.exit_pos);
        if (dist < 3.0f) {
            float threshold_t = 1.0f - (dist / 3.0f);
            float slow = 1.0f - threshold_t * 0.5f;
            g.player.vel.x *= slow;
            g.player.vel.z *= slow;
            g.player.fov_current += (65.0f - g.player.fov_current) * threshold_t * 0.1f;
        }
        if (dist < 1.5f) transition_to(STATE_LOBBY);
    }
}
