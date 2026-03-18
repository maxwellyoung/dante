// scene_taxi.c — STATE_CAR, STATE_DRIVING, STATE_RETURN_TAXI
#include "game_ctx.h"
#include <math.h>

extern GameCtx g;

void set_exposure(float exp);
void show_text(const char *text);
void hide_text(void);
void transition_to(GameState s);
void hard_cut_to(GameState s);

void taxi_load(void) {
    build_taxi_ride(&g.scene);
    init_player(&g.player, g.scene.spawn);
    g.player.camera.target = (Vector3){0, 0.9f, -1.0f};
    for (int i = 0; i < MAX_RAIN; i++) {
        g.rain[i].x = (float)GetRandomValue(0, RENDER_W);
        g.rain[i].y = (float)GetRandomValue(-RENDER_H, 0);
        g.rain[i].speed = 80.0f + (float)GetRandomValue(0, 120);
        g.rain[i].len = 3.0f + (float)GetRandomValue(0, 8);
    }
    StopAmbient(&g.audio);
    StopTitleMusic(&g.audio);
    PlayCityAmbient(&g.audio);
    SetCityAmbientVolume(&g.audio, 0.04f);
    SetSceneLighting(&g.lighting, LightingPreset_Taxi());
    set_exposure(0.05f);
    SetPostFXGrain(&g.postfx, 0.45f);
    SetPostFXCA(&g.postfx, 1.5f);
    SetPostFXVignette(&g.postfx, 1.0f);
    SetPostFXWarmth(&g.postfx, 0.1f);
}

void taxi_update(float dt) {
    // Mouse look (yaw/pitch only, no position change)
    Vector2 md = GetMouseDelta();
    float car_yaw = -md.x * ev_mouse_sens;
    float car_pitch = -md.y * ev_mouse_sens;
    Vector3 car_fwd = Vector3Normalize(Vector3Subtract(g.player.camera.target, g.player.camera.position));
    Vector3 car_right = Vector3Normalize(Vector3CrossProduct(car_fwd, g.player.camera.up));
    Matrix car_ym = MatrixRotateY(car_yaw);
    car_fwd = Vector3Transform(car_fwd, car_ym);
    float car_cp = asinf(car_fwd.y);
    float car_np = car_cp + car_pitch;
    if (car_np > 0.8f) car_pitch = 0.8f - car_cp;
    if (car_np < -0.6f) car_pitch = -0.6f - car_cp;
    Matrix car_pm = MatrixRotate(car_right, car_pitch);
    car_fwd = Vector3Transform(car_fwd, car_pm);
    g.player.camera.target = Vector3Add(g.player.camera.position, car_fwd);

    // Auto-scroll city walls
    float taxi_speed = 8.0f;
    if (g.state_time > 10.0f) {
        float t = g.state_time - 10.0f;
        taxi_speed = 8.0f + t * t * 2.0f;
    }
    for (int i = g.scene.static_wall_count; i < g.scene.wall_count; i++) {
        g.scene.walls[i].pos.z += taxi_speed * dt;
        if (g.scene.walls[i].pos.z > 10.0f) {
            g.scene.walls[i].pos.z -= 220.0f;
        }
    }

    // Post-FX ramp
    if (g.state_time > 10.0f) {
        float ramp = (g.state_time - 10.0f) / 3.5f;
        if (ramp > 1.0f) ramp = 1.0f;
        SetPostFXCA(&g.postfx, 2.5f + ramp * 8.0f);
        SetPostFXGrain(&g.postfx, 0.5f + ramp * 0.3f);
        g.player.camera.fovy = 70.0f + ramp * 20.0f;
    }

    // Dialogue
    if (g.state_time > 1.5f && g.state_time < 2.0f) show_text("Auckland. 2:47 AM.");
    if (g.state_time > 4.0f && g.state_time < 4.5f) hide_text();
    if (g.state_time > 5.5f && g.state_time < 6.0f) show_text("You going up to the tower?");
    if (g.state_time > 8.0f && g.state_time < 8.5f) hide_text();
    if (g.state_time > 9.0f && g.state_time < 9.5f) show_text("They say there's a hotel up there now.");
    if (g.state_time > 12.0f && g.state_time < 12.5f) hide_text();
    if (g.state_time > 12.5f && g.state_time < 13.0f) show_text("Three hours to kill.");

    // Hard cut to hyperspace
    if (g.state_time > 13.5f) {
        g.player.camera.fovy = 70.0f;
        SetPostFXCA(&g.postfx, 2.5f);
        hard_cut_to(STATE_HYPERSPACE);
    }
    if (IsKeyPressed(KEY_ENTER) && g.state_time > 3) {
        g.player.camera.fovy = 70.0f;
        SetPostFXCA(&g.postfx, 2.5f);
        hard_cut_to(STATE_HYPERSPACE);
    }
}

void driving_update(float dt) {
    (void)dt;
    // Arrival — taxi stopped, mouse look only
    Vector2 md2 = GetMouseDelta();
    float drv_yaw = -md2.x * ev_mouse_sens;
    float drv_pitch = -md2.y * ev_mouse_sens;
    Vector3 drv_fwd = Vector3Normalize(Vector3Subtract(g.player.camera.target, g.player.camera.position));
    Vector3 drv_right = Vector3Normalize(Vector3CrossProduct(drv_fwd, g.player.camera.up));
    Matrix drv_ym = MatrixRotateY(drv_yaw);
    drv_fwd = Vector3Transform(drv_fwd, drv_ym);
    float drv_cp = asinf(drv_fwd.y);
    float drv_np = drv_cp + drv_pitch;
    if (drv_np > 0.8f) drv_pitch = 0.8f - drv_cp;
    if (drv_np < -0.6f) drv_pitch = -0.6f - drv_cp;
    Matrix drv_pm = MatrixRotate(drv_right, drv_pitch);
    drv_fwd = Vector3Transform(drv_fwd, drv_pm);
    g.player.camera.target = Vector3Add(g.player.camera.position, drv_fwd);

    if (g.state_time > 7.0f) transition_to(STATE_HOTEL_EXT);
    if (IsKeyPressed(KEY_ENTER) && g.state_time > 2) transition_to(STATE_HOTEL_EXT);
}

void return_taxi_load(void) {
    build_return_taxi(&g.scene);
    init_player(&g.player, g.scene.spawn);
    g.player.camera.target = (Vector3){0, 0.9f, -1.0f};
    g.player.control_mult = 1.0f;
    StopAmbient(&g.audio);
    StopClockAmbient(&g.audio);
    StopWindAmbient(&g.audio);
    PlayCityAmbient(&g.audio);
    SetCityAmbientVolume(&g.audio, 0.02f);
    SetSceneLighting(&g.lighting, LightingPreset_ReturnTaxi());
    set_exposure(0.05f);
    SetPostFXWarmth(&g.postfx, 0.4f);
    SetPostFXGrain(&g.postfx, 0.3f);
    SetPostFXSaturation(&g.postfx, 1.05f);
    SetPostFXCA(&g.postfx, 1.5f);
    SetPostFXVignette(&g.postfx, 1.0f);
    SetPostFXContrast(&g.postfx, 1.0f);
}

void return_taxi_update(float dt) {
    // Mouse look only
    Vector2 md_r = GetMouseDelta();
    float ry = -md_r.x * ev_mouse_sens;
    float rp = -md_r.y * ev_mouse_sens;
    Vector3 rfwd = Vector3Normalize(Vector3Subtract(g.player.camera.target, g.player.camera.position));
    Vector3 rright = Vector3Normalize(Vector3CrossProduct(rfwd, g.player.camera.up));
    rfwd = Vector3Transform(rfwd, MatrixRotateY(ry));
    float rcp = asinf(rfwd.y);
    float rnp = rcp + rp;
    if (rnp > 0.8f) rp = 0.8f - rcp;
    if (rnp < -0.6f) rp = -0.6f - rcp;
    rfwd = Vector3Transform(rfwd, MatrixRotate(rright, rp));
    g.player.camera.target = Vector3Add(g.player.camera.position, rfwd);

    // City scrolls — DECELERATING
    float rtaxi_speed = 6.0f;
    if (g.state_time > 8.0f) {
        float t2 = g.state_time - 8.0f;
        rtaxi_speed = 6.0f - t2 * t2 * 0.3f;
        if (rtaxi_speed < 0.5f) rtaxi_speed = 0.5f;
    }
    for (int i = g.scene.static_wall_count; i < g.scene.wall_count; i++) {
        g.scene.walls[i].pos.z += rtaxi_speed * dt;
        if (g.scene.walls[i].pos.z > 10.0f)
            g.scene.walls[i].pos.z -= 220.0f;
    }

    // Dialogue
    if (g.state_time > 3.0f && g.state_time < 3.5f) show_text("Auckland. 5:52 AM.");
    if (g.state_time > 6.0f && g.state_time < 6.5f) hide_text();
    if (g.state_time > 8.0f && g.state_time < 8.5f) show_text("Good hotel?");
    if (g.state_time > 11.0f && g.state_time < 11.5f) hide_text();

    // Saturation drain
    {
        float drain = fminf(1.0f, g.state_time / 14.0f);
        SetPostFXSaturation(&g.postfx, 1.05f - drain * 0.25f);
    }

    // Fade to black
    if (g.state_time > 14.0f) {
        float fo = (g.state_time - 14.0f) / 3.0f;
        SetMasterVolume(fmaxf(0, 1.0f - fo));
        g.fade_alpha = fminf(1.0f, fo);
        g.fade_target = 1.0f;
        if (g.state_time > 17.0f) {
            CloseWindow();
        }
    }
}
