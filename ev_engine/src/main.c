// ENDEARING VOID — Custom engine on Raylib
// Maxwell Young
// A story about arriving somewhere. Auckland. A tower. 2 AM.

#include "raylib.h"
#include "rlgl.h"
#include "raymath.h"
#include "ev_types.h"
#include "lighting.h"
#include "scene.h"
#include "player.h"
#include "audio.h"
#include "render.h"
#include "npc.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

float ev_mouse_sens = MOUSE_SENS_DEFAULT;

static GameState state = STATE_TITLE;
static float state_time = 0;
static float total_time = 0;  // cumulative playthrough timer (never resets)
static float fade_alpha = 1.0f;
static float fade_target = 0.0f;
static GameState next_state = STATE_TITLE;
static bool transitioning = false;
static float fade_speed = 2.0f;

static Player player = {0};
static Scene scene = {0};

static EVLighting lighting = {0};
static EVPostFX postfx = {0};
static EVAudio audio = {0};
static Model cube_model = {0};
static bool cube_model_loaded = false;
static Model cyl_model = {0};
static bool cyl_model_loaded = false;
static Model sphere_model = {0};
static bool sphere_model_loaded = false;
static Model cone_model = {0};
static bool cone_model_loaded = false;
static Model skytower_model = {0};
static bool skytower_loaded = false;
static RenderTexture2D render_target;
static RenderTexture2D postfx_target;

static int tasks_done = 0;
#define PARIS_TASK_COUNT 5
#define SPACE_TASK_COUNT 4
static float done_pause = 0;

static bool phone_triggered = false;
static int phone_wall_idx = -1;


static bool eiffel_sparkle = false;
static float sparkle_timer = 0;

static bool elevator_ding_played = false;
static bool elevator_to_corridor = false;  // true = space lobby elevator, false = terrestrial

static bool returning_to_room = false;
// Track completed object names for room restore (player can complete in any order)
static const char *completed_objects[MAX_OBJECTS] = {0};
static int completed_count = 0;

// Rug pull — Gravity Bone flash on balcony
static bool balcony_flash_triggered = false;
static float balcony_flash_timer = 0;


// Cigarette camera animation (Phase 5)
static bool cigarette_anim = false;
static float cigarette_anim_timer = 0;
static Vector3 cigarette_cam_origin = {0};
static Vector3 cigarette_cam_target = {0};

static NPC gibbons = {0};

// Sprint 1: Clock deceleration state
static float bed_clock_rate = 1.0f;

// Sprint 2: Door positions for spatial audio (corridor)
static Vector3 door_positions[3] = {{0}};

// Sprint 3: Agency removal
static float agency_removal_timer = 0;

// Sprint 5C: Lobby memory palace
static int lobby_visit_count = 0;

static bool show_debug = false;
static bool wireframe = false;
static int current_style = 0;   // visual style preset index
static float scene_exposure = 0; // base exposure from load_state, before style bias

// ── Nudge mode (F5) — in-game wall position editor ──────────────
static bool nudge_mode = false;
static int nudge_selected = -1;  // wall index under crosshair

// Vignette text system — spring-based (Kowalski, Castilho)
static const char *vig_text = NULL;
static float text_y_offset = 20.0f;
static float text_y_velocity = 0.0f;
static float text_scale = 0.0f;
static float text_scale_vel = 0.0f;
static float text_scale_target = 0.0f;

static void show_text(const char *text) {
    vig_text = text;
    text_scale_target = 1.0f;
    text_y_offset = 20.0f;
    text_y_velocity = 0.0f;
}
static void hide_text(void) {
    text_scale_target = 0.0f;
}

// Rain drops for taxi scene
typedef struct { float x, y, speed, len; } Raindrop;
#define MAX_RAIN 60
static Raindrop rain[MAX_RAIN];

static InteractSoundType get_interact_sound(const char *name) {
    if (strcmp(name, "lamp") == 0) return INTERACT_CLICK;
    if (strcmp(name, "drawers") == 0) return INTERACT_FABRIC;
    if (strcmp(name, "candles") == 0) return INTERACT_FLAME;
    if (strcmp(name, "ashtray") == 0) return INTERACT_CLICK;
    if (strcmp(name, "bed") == 0) return INTERACT_FABRIC;
    if (strcmp(name, "champagne") == 0) return INTERACT_CORK_POP;
    if (strcmp(name, "desk") == 0) return INTERACT_FABRIC;
    if (strcmp(name, "bell") == 0) return INTERACT_CLICK;
    if (strcmp(name, "wineglass") == 0) return INTERACT_GLASS_CLINK;
    return INTERACT_CLICK;
}

static float transition_hold = 0.3f;
static float hold_timer = 0;

// ── Interaction ritual timers (P3 — multi-second rituals) ──
static float interaction_timers[4] = {0};  // lamp, champagne, desk, bed
static int interaction_phases[4] = {0};    // current animation phase

// ── Ambient fade (P4) ──
static float ambient_fade = 1.0f;

// ============================================================
// PAUSE MENU & SETTINGS
// ============================================================
typedef enum { MENU_NONE, MENU_PAUSE, MENU_SETTINGS } MenuMode;
static MenuMode menu_mode = MENU_NONE;
static int menu_cursor = 0;
#define MENU_MAX_ITEMS 5
static float menu_item_x[MENU_MAX_ITEMS];
static float menu_item_vx[MENU_MAX_ITEMS];
static float menu_overlay_a = 0.0f;
static float menu_overlay_va = 0.0f;
static float setting_master_vol = 1.0f;
static bool  setting_fullscreen = false;
#define PAUSE_ITEM_COUNT 3
#define SETTINGS_ITEM_COUNT 5
static const char *pause_labels[] = {"RESUME", "SETTINGS", "QUIT"};
static const char *settings_labels[] = {"SENSITIVITY", "VOLUME", "STYLE", "FULLSCREEN", "BACK"};
static int menu_item_count(void) { return (menu_mode == MENU_SETTINGS) ? SETTINGS_ITEM_COUNT : PAUSE_ITEM_COUNT; }
static void menu_init_springs(void) { for (int i = 0; i < MENU_MAX_ITEMS; i++) { menu_item_x[i] = (float)(RENDER_W + i * 24); menu_item_vx[i] = 0; } }
static void menu_open(MenuMode mode) { menu_mode = mode; menu_cursor = 0; menu_init_springs(); menu_overlay_a = 0; menu_overlay_va = 0; EnableCursor(); }
static void menu_close(void) { menu_mode = MENU_NONE; menu_cursor = 0; menu_overlay_a = 0; menu_overlay_va = 0; if (state != STATE_TITLE) DisableCursor(); }

// Forward declaration
static void load_state(GameState s);

// Exposure helper — stores scene value and adds style bias
static void set_exposure(float exp) {
    scene_exposure = exp;
    SetPostFXExposure(&postfx, exp + visual_styles[current_style].exposure_bias);
}

static void update_menu_springs(float dt) {
    float sk = 280.0f, sd = 26.0f, sm = 0.9f;
    float ta = (menu_mode != MENU_NONE) ? 1.0f : 0.0f;
    float fa = -sk * (menu_overlay_a - ta) - sd * menu_overlay_va;
    menu_overlay_va += (fa / sm) * dt;
    menu_overlay_a += menu_overlay_va * dt;
    if (menu_overlay_a < 0) menu_overlay_a = 0;
    if (menu_overlay_a > 1) menu_overlay_a = 1;
    float tx = (menu_mode != MENU_NONE) ? 0.0f : (float)RENDER_W;
    for (int i = 0; i < MENU_MAX_ITEMS; i++) {
        float fx = -sk * (menu_item_x[i] - tx) - sd * menu_item_vx[i];
        menu_item_vx[i] += (fx / sm) * dt;
        menu_item_x[i] += menu_item_vx[i] * dt;
    }
}

static bool update_pause_menu(void) {
    if (transitioning) return menu_mode != MENU_NONE;
    if (menu_mode == MENU_NONE) {
        if (IsKeyPressed(KEY_ESCAPE) && state != STATE_TITLE && state != STATE_STARS) {
            menu_open(MENU_PAUSE); return true;
        }
        return false;
    }
    int count = menu_item_count();
    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W))
        menu_cursor = (menu_cursor - 1 + count) % count;
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S))
        menu_cursor = (menu_cursor + 1) % count;
    // Mouse hover — window coords → render coords
    {
        int sw = GetScreenWidth(), sh = GetScreenHeight();
        float sc = fminf((float)sw / RENDER_W, (float)sh / RENDER_H);
        float ox = ((float)sw - RENDER_W * sc) * 0.5f;
        float oy = ((float)sh - RENDER_H * sc) * 0.5f;
        float mx = (GetMouseX() - ox) / sc;
        float my = (GetMouseY() - oy) / sc;
        int by = RENDER_H / 2 - (count * 18) / 2;
        for (int i = 0; i < count; i++) {
            int iy = by + i * 18;
            if (mx > 40 && mx < RENDER_W - 40 && my >= iy - 2 && my < iy + 14)
                { menu_cursor = i; break; }
        }
    }
    if (IsKeyPressed(KEY_ESCAPE)) {
        if (menu_mode == MENU_SETTINGS) menu_open(MENU_PAUSE);
        else menu_close();
        return true;
    }
    bool confirm  = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    bool go_left  = IsKeyPressed(KEY_LEFT)  || IsKeyPressed(KEY_A);
    bool go_right = IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D);
    if (menu_mode == MENU_PAUSE && confirm) {
        switch (menu_cursor) {
            case 0: menu_close(); break;
            case 1: menu_open(MENU_SETTINGS); break;
            case 2: CloseWindow(); break;
        }
    } else if (menu_mode == MENU_SETTINGS) {
        switch (menu_cursor) {
            case 0:
                if (go_left)  ev_mouse_sens = fmaxf(0.001f, ev_mouse_sens - 0.0005f);
                if (go_right) ev_mouse_sens = fminf(0.008f, ev_mouse_sens + 0.0005f);
                break;
            case 1:
                if (go_left)  setting_master_vol = fmaxf(0.0f, setting_master_vol - 0.05f);
                if (go_right) setting_master_vol = fminf(1.0f, setting_master_vol + 0.05f);
                SetMasterVolume(setting_master_vol);
                break;
            case 2:
                if (go_left || go_right || confirm) {
                    if (go_left) current_style = (current_style - 1 + STYLE_COUNT) % STYLE_COUNT;
                    else         current_style = (current_style + 1) % STYLE_COUNT;
                    ApplyVisualStyle(&postfx, current_style);
                    SetPostFXExposure(&postfx, scene_exposure + visual_styles[current_style].exposure_bias);
                }
                break;
            case 3:
                if (confirm) { ToggleFullscreen(); setting_fullscreen = !setting_fullscreen; }
                break;
            case 4:
                if (confirm) menu_open(MENU_PAUSE);
                break;
        }
    }
    return true;
}

static void draw_pause_menu(void) {
    if (menu_overlay_a < 0.01f) return;
    unsigned char oa = (unsigned char)(160.0f * menu_overlay_a);
    DrawRectangle(0, 0, RENDER_W, RENDER_H, (Color){8, 8, 10, oa});
    const char **labels = (menu_mode == MENU_SETTINGS) ? settings_labels : pause_labels;
    int count = (menu_mode == MENU_SETTINGS) ? SETTINGS_ITEM_COUNT : PAUSE_ITEM_COUNT;
    int fs = 10, lh = 18;
    int by = RENDER_H / 2 - (count * lh) / 2;
    for (int i = 0; i < count; i++) {
        int xo = (int)menu_item_x[i];
        int iy = by + i * lh;
        Color tc = (i == menu_cursor)
            ? (Color){245, 242, 238, 240} : (Color){165, 160, 152, 180};
        if (i == menu_cursor)
            DrawRectangle(RENDER_W / 2 - 80 + xo, iy + 3, 3, 7, (Color){200, 178, 130, 240});
        int lx = RENDER_W / 2 - 72 + xo;
        DrawText(labels[i], lx + 1, iy + 1, fs, (Color){0, 0, 0, 180});
        DrawText(labels[i], lx, iy, fs, tc);
        if (menu_mode == MENU_SETTINGS) {
            char val[48] = "";
            int vx = RENDER_W / 2 + 30 + xo;
            switch (i) {
                case 0: snprintf(val, sizeof(val), "< %.1f >", ev_mouse_sens * 1000.0f); break;
                case 1: snprintf(val, sizeof(val), "< %d%% >", (int)(setting_master_vol * 100.0f + 0.5f)); break;
                case 2: snprintf(val, sizeof(val), "< %s >", visual_styles[current_style].name); break;
                case 3: snprintf(val, sizeof(val), "[ %s ]", setting_fullscreen ? "ON" : "OFF"); break;
                default: break;
            }
            if (val[0]) {
                Color vc = (i == menu_cursor) ? (Color){200, 178, 130, 240} : (Color){165, 160, 152, 140};
                DrawText(val, vx + 1, iy + 1, fs, (Color){0, 0, 0, 180});
                DrawText(val, vx, iy, fs, vc);
            }
        }
    }
    const char *hint = (menu_mode == MENU_SETTINGS) ? "ARROWS TO ADJUST  /  ESC BACK" : "ESC TO RESUME";
    int hw = MeasureText(hint, 8);
    DrawText(hint, RENDER_W/2 - hw/2 + 1, RENDER_H - 23, 8, (Color){0,0,0,120});
    DrawText(hint, RENDER_W/2 - hw/2, RENDER_H - 24, 8, (Color){130,126,120,120});
}

static void transition_to(GameState s) {
    if (transitioning) return;  // already in flight — don't reset
    transitioning = true;
    next_state = s;
    fade_target = 1.0f;
    fade_speed = 2.0f;
    transition_hold = 0.3f;
    hold_timer = 0;
    PlayDoorSound(&audio);
}

static void transition_to_slow(GameState s, float spd) {
    if (transitioning) return;  // already in flight — don't reset
    transitioning = true;
    next_state = s;
    fade_target = 1.0f;
    fade_speed = spd;
    transition_hold = (s == STATE_BED) ? 1.0f : 0.3f;
    hold_timer = 0;
    PlayDoorSound(&audio);
}

// Hard cut — bypasses fade, instantly loads state (Blendo-style ellipsis)
// Flash kick: 2-3 frames of warm white, then instant scene
static float cut_flash_timer = 0;

static void hard_cut_to(GameState s) {
    StopClockAmbient(&audio);
    PlayHardCutPunch(&audio);  // micro-transient — makes the cut physical
    load_state(s);
    fade_alpha = 0.0f;  fade_target = 0.0f;
    transitioning = false;  hold_timer = 0;
    transition_hold = 0.3f;  // reset to default
    cut_flash_timer = 0.12f;
    SetPostFXFlash(&postfx, 1.0f, 1.0f, 0.95f, 0.85f);
    SetMasterVolume(1.0f);  // restore audio (hyperspace sets it to 0)
}

// ── Nudge mode: raycast to find wall under crosshair ────────────
// Simple AABB ray intersection — walls are axis-aligned boxes
static int raycast_wall(Camera3D cam, Scene *sc) {
    Vector3 origin = cam.position;
    Vector3 dir = Vector3Normalize(Vector3Subtract(cam.target, cam.position));
    float best_t = 1e9f;
    int best_i = -1;
    for (int i = 0; i < sc->wall_count; i++) {
        Wall *w = &sc->walls[i];
        if (!w->active) continue;
        Vector3 half = {w->size.x * 0.5f, w->size.y * 0.5f, w->size.z * 0.5f};
        Vector3 bmin = Vector3Subtract(w->pos, half);
        Vector3 bmax = Vector3Add(w->pos, half);
        float tmin = -1e9f, tmax = 1e9f;
        float *o = (float*)&origin, *d = (float*)&dir;
        float *mn = (float*)&bmin, *mx = (float*)&bmax;
        bool hit = true;
        for (int a = 0; a < 3; a++) {
            if (fabsf(d[a]) < 1e-8f) {
                if (o[a] < mn[a] || o[a] > mx[a]) { hit = false; break; }
            } else {
                float t1 = (mn[a] - o[a]) / d[a];
                float t2 = (mx[a] - o[a]) / d[a];
                if (t1 > t2) { float tmp = t1; t1 = t2; t2 = tmp; }
                if (t1 > tmin) tmin = t1;
                if (t2 < tmax) tmax = t2;
                if (tmin > tmax) { hit = false; break; }
            }
        }
        if (hit && tmin > 0 && tmin < best_t) {
            best_t = tmin;
            best_i = i;
        }
    }
    return best_i;
}

// Write current state to temp file for dev-watch to read on restart
static void write_state_file(GameState s) {
    const char *names[] = {
        "STATE_TITLE", "STATE_CAR", "STATE_DRIVING", "STATE_HOTEL_EXT",
        "STATE_LOBBY", "STATE_ELEVATOR", "STATE_HALLWAY", "STATE_ROOM",
        "STATE_BATHROOM", "STATE_BALCONY", "STATE_BED", "STATE_STARS",
        "STATE_HYPERSPACE", "STATE_SPACE_LOBBY", "STATE_SPACE_CORRIDOR",
        "STATE_SPACE_SUITE", "STATE_PARIS_DREAM", "STATE_RETURN_TAXI"
    };
    FILE *f = fopen("/tmp/ev_state", "w");
    if (f) {
        int idx = (int)s;
        if (idx >= 0 && idx < 18) fprintf(f, "%s\n", names[idx]);
        else fprintf(f, "STATE_TITLE\n");
        fprintf(f, "%.2f %.2f %.2f\n",
                player.camera.position.x, player.camera.position.y,
                player.camera.position.z);
        fclose(f);
    }
}

static void load_state(GameState s) {
    // Kill ALL looping audio — each scene starts fresh. No overlap.
    StopAllAudio(&audio);
    // Restore master volume — silence zones may have lowered it
    SetMasterVolume(setting_master_vol);

    // Save returning_to_room state before central reset
    int saved_tasks = returning_to_room ? tasks_done : 0;
    bool saved_phone = returning_to_room ? phone_triggered : false;
    // Snapshot completed object names before scene rebuild
    const char *saved_completed[MAX_OBJECTS] = {0};
    int saved_completed_count = 0;
    if (returning_to_room) {
        saved_completed_count = completed_count;
        for (int i = 0; i < completed_count && i < MAX_OBJECTS; i++)
            saved_completed[i] = completed_objects[i];
    }

    state = s;
    state_time = 0;
    tasks_done = 0;
    done_pause = 0;
    if (!returning_to_room) { completed_count = 0; memset(completed_objects, 0, sizeof(completed_objects)); }
    phone_triggered = false;
    phone_wall_idx = -1;
    balcony_flash_triggered = false;
    balcony_flash_timer = 0;
    vig_text = NULL;
    text_y_offset = 20.0f;
    text_y_velocity = 0.0f;
    text_scale = 0.0f;
    text_scale_vel = 0.0f;
    text_scale_target = 0.0f;

    // Reset PostFX to current visual style baseline before per-scene overrides
    // This ensures CA, contrast, vignette, saturation are always restored
    ApplyVisualStyle(&postfx, current_style);

    switch (s) {
        case STATE_TITLE:
            DisableCursor();
            StopAmbient(&audio);
            StopSuiteMusic(&audio); StopBalconyMusic(&audio); StopCorridorMusic(&audio);
            reset_title_state();
            PlayTitleMusic(&audio);  // ambient atmosphere
            // Clean PostFX — no CA, no grain, no warmth on title
            SetPostFXCA(&postfx, 0.0f);
            SetPostFXGrain(&postfx, 0.0f);
            SetPostFXWarmth(&postfx, 0.0f);
            SetPostFXSaturation(&postfx, 1.0f);
            SetPostFXContrast(&postfx, 0.0f);
            SetPostFXVignette(&postfx, 0.0f);
            set_exposure(0.0f);
            break;

        case STATE_CAR:
            build_taxi_ride(&scene);
            init_player(&player, scene.spawn);
            // Look forward (-Z)
            player.camera.target = (Vector3){0, 0.9f, -1.0f};
            // Init rain (2D overlay)
            for (int i = 0; i < MAX_RAIN; i++) {
                rain[i].x = (float)GetRandomValue(0, RENDER_W);
                rain[i].y = (float)GetRandomValue(-RENDER_H, 0);
                rain[i].speed = 80.0f + (float)GetRandomValue(0, 120);
                rain[i].len = 3.0f + (float)GetRandomValue(0, 8);
            }
            StopAmbient(&audio);
            StopTitleMusic(&audio);  // fade out title atmosphere
            PlayCityAmbient(&audio);
            SetCityAmbientVolume(&audio, 0.04f);
            SetSceneLighting(&lighting, LightingPreset_Taxi());
            set_exposure(0.05f);  // slight lift — see the interior
            SetPostFXGrain(&postfx, 0.45f);      // grainy night footage
            SetPostFXCA(&postfx, 1.5f);           // subtle CA — night feeling
            SetPostFXVignette(&postfx, 1.0f);     // intimate interior
            SetPostFXWarmth(&postfx, 0.1f);       // sodium warmth
            break;

        case STATE_DRIVING:
            // Taxi arriving — same scene, just stopped
            // Don't rebuild — keep the scene from STATE_CAR
            break;

        case STATE_HOTEL_EXT:
            build_hotel_exterior(&scene);
            init_player(&player, scene.spawn);
            StopAmbient(&audio);
            StopClockAmbient(&audio);

            PlayCityAmbient(&audio);
            PlayWindAmbient(&audio);
            SetSceneLighting(&lighting, LightingPreset_Exterior());
            set_exposure(0.0f);   // let moonlight work
            SetPostFXGrain(&postfx, 0.6f);
            break;

        case STATE_LOBBY:
            EnableCursor(); DisableCursor();
            build_lobby(&scene);
            init_player(&player, scene.spawn);
            StartAmbient(&audio, DRONE_LOBBY);
            StopClockAmbient(&audio);
            StopCityAmbient(&audio);
            StopWindAmbient(&audio);
            StopMuffledPiano(&audio); StopDistantVoices(&audio); StopFootstepsAbove(&audio);
            // Lobby has its own through-wall: distant voices from restaurant/bar area
            PlayDistantVoices(&audio);
            SetSceneLighting(&lighting, LightingPreset_Lobby());
            set_exposure(0.0f);   // grand lobby — let chandelier work
            SetPostFXGrain(&postfx, 0.4f);
            break;

        case STATE_ELEVATOR:
            build_elevator(&scene);
            init_player(&player, scene.spawn);
            elevator_ding_played = false;
            StopAmbient(&audio);
            StopClockAmbient(&audio);
            StopCityAmbient(&audio);
            StopWindAmbient(&audio);
            StopMuffledPiano(&audio); StopDistantVoices(&audio); StopFootstepsAbove(&audio);
            PlayElevatorHum(&audio);
            PlayElevatorWhoosh(&audio);  // ascending wind — building toward hyperspace
            player.gravity_mult = 1.0f;  // Earth gravity — last time you'll feel this
            SetSceneLighting(&lighting, LightingPreset_Elevator());
            set_exposure(0.0f);
            SetPostFXGrain(&postfx, 0.3f);  // fluorescent — less grain
            break;

        case STATE_HALLWAY:
            build_hallway(&scene);
            init_player(&player, scene.spawn);
            StartAmbient(&audio, DRONE_HALLWAY);
            StopClockAmbient(&audio);
            StopCityAmbient(&audio);
            StopWindAmbient(&audio);
            // Through-wall sounds — other lives happening in parallel
            PlayMuffledPiano(&audio);    // someone else's room
            PlayFootstepsAbove(&audio);  // walking upstairs
            SetSceneLighting(&lighting, LightingPreset_Hallway());
            set_exposure(-0.05f);  // dim corridor — slight
            SetPostFXGrain(&postfx, 0.5f);
            break;

        case STATE_ROOM: {
            float saved_warmth = (float)saved_tasks / PARIS_TASK_COUNT;

            build_hotel_room(&scene);

            if (saved_tasks > 0) {
                // Returning from bathroom — restore task progress
                init_player(&player, (Vector3){4.5f, 1.6f, -2.5f});
                tasks_done = saved_tasks;
                phone_triggered = saved_phone;
                SetPostFXWarmth(&postfx, saved_warmth);
                scene.fog_density = 0.004f - (saved_warmth * 0.003f);
                // Mark completed objects as done — match by name, not index
                for (int i = 0; i < scene.object_count; i++) {
                    for (int j = 0; j < saved_completed_count; j++) {
                        if (saved_completed[j] && strcmp(scene.objects[i].name, saved_completed[j]) == 0) {
                            scene.objects[i].done = true;
                            scene.objects[i].step = scene.objects[i].max_steps;
                            break;
                        }
                    }
                }
                returning_to_room = false;
            } else {
                init_player(&player, scene.spawn);
                SetPostFXWarmth(&postfx, 0.0f);
            }
            StartAmbient(&audio, DRONE_ROOM);
            PlayClockAmbient(&audio);
            StopCityAmbient(&audio);
            StopWindAmbient(&audio);
            // Through-wall: the hotel is populated by absence
            PlayMuffledPiano(&audio);     // music from the next room
            PlayFootstepsAbove(&audio);   // someone pacing upstairs
            SetSceneLighting(&lighting, LightingPreset_Room());
            set_exposure(-0.05f);  // slight darkness — 2AM
            SetPostFXGrain(&postfx, 0.5f);
            break;
        }

        case STATE_BATHROOM:
            build_bathroom(&scene);
            init_player(&player, scene.spawn);
            StartAmbient(&audio, DRONE_ROOM);
            StopClockAmbient(&audio);
            StopCityAmbient(&audio);

            StopWindAmbient(&audio);
            SetSceneLighting(&lighting, LightingPreset_Bathroom());
            set_exposure(0.1f);   // bright bathroom fluorescent
            SetPostFXGrain(&postfx, 0.3f);      // clinical — less grain
            break;

        case STATE_BALCONY:
            build_balcony(&scene);
            init_player(&player, scene.spawn);
            eiffel_sparkle = false; sparkle_timer = 0;
            cigarette_anim = false; cigarette_anim_timer = 0;
            StopAmbient(&audio);
            StopClockAmbient(&audio);
            StopCityAmbient(&audio);
            StopSuiteMusic(&audio);  // lighthouse ends
            PlayBalconyMusic(&audio);  // ambient4 — the void outside
            PlayWindAmbient(&audio);  // void wind — no city sounds in orbit
            PlayBalconyGust(&audio);  // one-shot rush — the void entering
            player.gravity_mult = 0.3f;  // exposed to void — lightest gravity
            SetPostFXWarmth(&postfx, 1.0f);
            SetSceneLighting(&lighting, LightingPreset_Balcony());
            set_exposure(0.05f);  // slight lift — Earth glow
            SetPostFXGrain(&postfx, 0.6f);      // contemplative grain
            break;

        case STATE_BED:
            StopAmbient(&audio); StopCityAmbient(&audio); StopWindAmbient(&audio);
            StopSuiteMusic(&audio); StopBalconyMusic(&audio);
            PlayBedImpact(&audio);  // soft muffled thud — surrender
            // Clock keeps playing — will decelerate
            bed_clock_rate = 1.0f;
            PlayClockAmbient(&audio);
            break;
        case STATE_STARS: {
            StopAmbient(&audio); StopClockAmbient(&audio); StopCityAmbient(&audio); StopWindAmbient(&audio);
            StopBedDrone(&audio);
            PlayHeldChord(&audio);  // stacked fifths — credits as elegy
            int rt_m = (int)total_time / 60, rt_s = (int)total_time % 60;
            printf("\n[PLAYTHROUGH] Total runtime: %d:%02d\n\n", rt_m, rt_s);
            break;
        }

        case STATE_PARIS_DREAM:
            build_hotel_room(&scene);
            init_player(&player, scene.spawn);
            player.control_mult = 0.5f;  // dream movement — slow, deliberate
            player.gravity_mult = 0.7f;  // dream gravity — slightly floaty
            // The photograph is now interactable
            add_object(&scene, -2.5f, 0.62f, -3.5f, "photograph", (Color){240,238,230,255}, 1);
            // The balcony door — look through it to see Paris
            add_object(&scene, -5.5f, 1.5f, -1.0f, "window", (Color){120,150,220,100}, 1);
            // Eiffel Tower silhouette visible through balcony glass
            add_wall(&scene, -6.5f, 2.5f, -1.0f, 0.06f, 2.0f, 0.06f, (Color){40,35,30,60});
            add_wall(&scene, -6.5f, 3.5f, -1.0f, 0.6f, 0.04f, 0.04f, (Color){40,35,30,40});
            add_wall(&scene, -6.5f, 3.0f, -1.0f, 0.35f, 0.04f, 0.04f, (Color){40,35,30,45});
            StopAmbient(&audio);
            StopClockAmbient(&audio);  // NO clock in the dream
            StopWindAmbient(&audio);
            StartAmbient(&audio, DRONE_ROOM);
            SetSceneLighting(&lighting, LightingPreset_ParisDream());
            set_exposure(0.1f);                     // bright golden
            SetPostFXWarmth(&postfx, 0.7f);         // warm but not total wash
            SetPostFXGrain(&postfx, 0.7f);          // dreamy grain
            SetPostFXSaturation(&postfx, 1.2f);     // oversaturated
            SetPostFXCA(&postfx, 3.5f);             // dream shimmer
            SetPostFXVignette(&postfx, 2.0f);       // heavy vignette — darkens edges for contrast
            SetPostFXContrast(&postfx, 1.5f);       // actually punchy — drives shadows darker
            scene.fog_color = (Color){30, 22, 10, 255};  // darker golden fog
            scene.fog_density = 0.005f;              // thinner — let shadows breathe
            break;

        case STATE_RETURN_TAXI:
            build_return_taxi(&scene);
            init_player(&player, scene.spawn);
            player.camera.target = (Vector3){0, 0.9f, -1.0f};
            player.control_mult = 1.0f;
            StopAmbient(&audio);
            StopClockAmbient(&audio);
            StopWindAmbient(&audio);
            PlayCityAmbient(&audio);
            SetCityAmbientVolume(&audio, 0.02f);  // quiet dawn
            SetSceneLighting(&lighting, LightingPreset_ReturnTaxi());
            set_exposure(0.05f);                    // slight lift
            SetPostFXWarmth(&postfx, 0.4f);         // warm but gentle
            SetPostFXGrain(&postfx, 0.3f);          // clean dawn
            SetPostFXSaturation(&postfx, 1.05f);    // natural
            SetPostFXCA(&postfx, 1.5f);             // minimal
            SetPostFXVignette(&postfx, 1.0f);       // default
            SetPostFXContrast(&postfx, 1.0f);       // default
            break;

        case STATE_HYPERSPACE:
            build_hyperspace(&scene);
            init_player(&player, scene.spawn);
            // Camera looks straight down the tunnel
            player.camera.target = (Vector3){0, 0, -10};
            StopAmbient(&audio);
            StopClockAmbient(&audio);
            StopCityAmbient(&audio);
            StopWindAmbient(&audio);
            StopElevatorWhoosh(&audio);
            PlayHyperspaceTone(&audio);  // rising 80Hz→400Hz glissando
            PlayHyperspaceRiser(&audio); // fat layered riser — noise+harmonics+sub
            SetSceneLighting(&lighting, LightingPreset_Hyperspace());
            set_exposure(0.1f);
            SetPostFXGrain(&postfx, 0.7f);
            SetPostFXCA(&postfx, 5.0f);  // start with elevated CA
            break;

        case STATE_SPACE_LOBBY:
            build_space_lobby(&scene);
            init_player(&player, scene.spawn);
            elevator_ding_played = false;  // reset for arrival ding
            StopAmbient(&audio);
            StopClockAmbient(&audio);
            StopCityAmbient(&audio);
            StopWindAmbient(&audio);
            StopMuffledPiano(&audio); StopDistantVoices(&audio); StopFootstepsAbove(&audio);
            StopHyperspaceTone(&audio);
            StopHyperspaceRiser(&audio);
            PlayArrivalThump(&audio);    // deep bass impact — you have landed
            PlayGravitySettle(&audio);   // hull creak — the ship acknowledges your weight
            PlayAirlockHiss(&audio);     // pressurization — you're aboard
            PlayEarthPresence(&audio);   // 30Hz sub-bass — the planet's weight
            player.gravity_mult = 0.4f;  // orbital gravity — jumps soar, wall runs linger
            StartAmbient(&audio, DRONE_SPACE_LOBBY);
            SetSceneLighting(&lighting, LightingPreset_SpaceLobby());
            set_exposure(-0.05f);
            SetPostFXGrain(&postfx, 0.4f);
            SetPostFXCA(&postfx, 2.5f);  // reset CA from hyperspace
            // Sprint 5C: Lobby memory palace — warmer on revisit
            lobby_visit_count++;
            if (lobby_visit_count > 1) {
                float revisit_warmth = fminf(0.15f, (lobby_visit_count - 1) * 0.08f);
                SetPostFXWarmth(&postfx, revisit_warmth);
                set_exposure(-0.05f + revisit_warmth * 0.1f);
            }
            // Gibbons — leads past observation window to glass elevator
            {
                Vector3 lobby_wps[] = {
                    {3, 1.6f, 2},       // meets you near spawn
                    {6, 1.6f, -3},      // past observation window
                    {0, 1.6f, -4},      // toward elevator shaft
                    {0, 1.6f, 0},       // AT the glass elevator — get in
                };
                init_npc(&gibbons, (Vector3){2, 1.6f, 4}, lobby_wps, 4, 2.5f, 3.5f);
                static const char *lobby_lines[] = {
                    "There you are. I was beginning to worry.",
                    "The window. You should see it before...",
                    "This way. I'll take you up.",
                    "After you. I insist.",
                };
                npc_set_dialogue(&gibbons, lobby_lines, 4, 3.0f);
            }
            break;

        case STATE_SPACE_CORRIDOR:
            build_space_corridor(&scene);
            init_player(&player, scene.spawn);
            StopAmbient(&audio);
            StopEarthPresence(&audio);
            StopClockAmbient(&audio);
            StopCityAmbient(&audio);
            StopWindAmbient(&audio);
            PlayAirlockHiss(&audio);  // pressurization — entering sealed corridor
            PlayDoorThud(&audio);     // bulkhead sealing behind you
            player.gravity_mult = 0.4f;  // orbital gravity persists
            StartAmbient(&audio, DRONE_SPACE_CORRIDOR);
            PlayCorridorMusic(&audio);  // ambient1 — underneath the walk
            // Through bulkheads: distant voices, other passengers
            PlayDistantVoices(&audio);
            // Per-door spatial sounds — door 0: piano, door 1: TV, door 2: silence
            PlayMuffledPiano(&audio);
            PlaySound(audio.snd_running_water);
            PlaySound(audio.snd_tv_murmur);
            // Door positions for distance-based volume
            door_positions[0] = (Vector3){-3.5f, 1.6f, 4.0f};   // warm light door
            door_positions[1] = (Vector3){3.5f, 1.6f, 8.0f};    // blue light door
            door_positions[2] = (Vector3){-3.5f, 1.6f, 12.0f};  // dark door — silence
            SetSceneLighting(&lighting, LightingPreset_SpaceCorridor());
            set_exposure(0.0f);
            SetPostFXGrain(&postfx, 0.4f);
            // Gibbons — leads through corridor to suite
            {
                Vector3 corr_wps[] = {
                    {0, 1.6f, 2},       // ahead in corridor
                    {0, 1.6f, 8},       // mid corridor
                    {0, 1.6f, 14},      // near exit
                };
                init_npc(&gibbons, scene.spawn, corr_wps, 3, 3.5f, 4.0f);
                static const char *corr_lines[] = {
                    "The walls are thin. You hear things.",
                    "I could tell you stories about this floor.",
                    "Last door on the left. Your neighbor's quiet.",
                };
                npc_set_dialogue(&gibbons, corr_lines, 3, 3.5f);
            }
            break;

        case STATE_SPACE_SUITE:
            build_space_suite(&scene);
            init_player(&player, scene.spawn);
            SetPostFXWarmth(&postfx, 0.0f);
            StopAmbient(&audio);
            StopCityAmbient(&audio);
            StopWindAmbient(&audio);
            StopMuffledPiano(&audio); StopFootstepsAbove(&audio);
            StopDistantVoices(&audio);
            StopCorridorMusic(&audio);
            StopSound(audio.snd_running_water); StopSound(audio.snd_tv_murmur);
            PlayDoorThud(&audio);     // suite door closing — sealed in
            PlayAirlockHiss(&audio);  // pressurization equalize
            player.gravity_mult = 0.5f;  // suite has more gravity — domestic, grounded
            StartAmbient(&audio, DRONE_SPACE_SUITE);
            PlayClockAmbient(&audio);  // the room's heartbeat
            audio.clock_rate = 1.0f;   // reset clock rate
            SetSoundPitch(audio.snd_clock, 1.0f);
            agency_removal_timer = 0;
            SetSceneLighting(&lighting, LightingPreset_SpaceSuite());
            set_exposure(-0.1f);
            SetPostFXGrain(&postfx, 0.35f);
            // Gibbons — walks to sofa, sits, waits for player to finish tasks
            // After all tasks: stands, delivers final line, walks out
            {
                Vector3 suite_wps[] = {
                    {0, 1.6f, 2},       // into room
                    {-2.0f, 1.6f, 1},   // toward sofa
                };
                init_npc(&gibbons, (Vector3){0, 1.6f, 5}, suite_wps, 2, 2.5f, 3.0f);
                static const char *suite_lines[] = {
                    "Make yourself comfortable. It's yours.",
                    "Three hours. You'd be surprised what fits.",
                };
                npc_set_dialogue(&gibbons, suite_lines, 2, 3.5f);
            }
            break;
    }
    fade_alpha = 1.0f;
    fade_target = 0.0f;

    // Re-apply scene exposure with style bias
    float final_exp = scene_exposure + visual_styles[current_style].exposure_bias;
    SetPostFXExposure(&postfx, final_exp);

    // DIAGNOSTIC — remove after fixing visibility issue
    printf("[DBG] load_state(%d): walls=%d objs=%d exposure=%.2f style=%d shadow=%s fog=%.4f\n",
           s, scene.wall_count, scene.object_count, final_exp, current_style,
           lighting.shadowReady ? "ON" : "OFF", scene.fog_density);
    printf("[DBG]   fog_color=(%d,%d,%d) cam=(%.1f,%.1f,%.1f)\n",
           scene.fog_color.r, scene.fog_color.g, scene.fog_color.b,
           player.camera.position.x, player.camera.position.y, player.camera.position.z);

    // Update shadow matrix — per-scene key direction + radius
    if (lighting.shadowReady) {
        float sr = 25.0f;  // default shadow radius
        switch (s) {
            case STATE_ELEVATOR:   sr = 5.0f;  break;  // tiny box
            case STATE_BATHROOM:   sr = 8.0f;  break;  // small room
            case STATE_ROOM:       sr = 12.0f; break;
            case STATE_HALLWAY:    sr = 15.0f; break;
            case STATE_SPACE_SUITE: sr = 15.0f; break;
            case STATE_LOBBY:      sr = 20.0f; break;
            case STATE_SPACE_CORRIDOR: sr = 20.0f; break;
            case STATE_SPACE_LOBBY: sr = 30.0f; break;  // cavernous
            case STATE_HOTEL_EXT:  sr = 30.0f; break;
            default: break;
        }
        UpdateShadowMatrix(&lighting,
            lighting.activePreset.keyDir,
            (Vector3){0, 2, 0}, sr);
    }

    // Reset interaction ritual timers
    for (int i = 0; i < 4; i++) { interaction_timers[i] = 0; interaction_phases[i] = 0; }
    ambient_fade = 1.0f;
}

// ============================================================
// VIGNETTE DRAWING
// ============================================================

// Old 2D draw_taxi_ride and draw_arrival removed — replaced by 3D taxi scene

static void draw_vignette_text(void) {
    if (!vig_text || text_scale < 0.01f) return;
    float clamped = text_scale;
    if (clamped > 1.0f) clamped = 1.0f;
    if (clamped < 0.0f) clamped = 0.0f;
    unsigned char a = (unsigned char)(240 * clamped);
    int y = RENDER_H - 30 + (int)text_y_offset;
    draw_text_box(vig_text, y, 12, (Color){248, 245, 238, a});
}

// ============================================================
// MAIN
// ============================================================

int main(void) {
#ifdef QA_MODE
    // Unfocused + always-run: no focus steal, GL stays active even if occluded
    // ALWAYS_RUN keeps GL rendering when window isn't focused
    SetConfigFlags(FLAG_WINDOW_ALWAYS_RUN);
    InitWindow(960, 600, "EV QA");
#else
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(960, 600, "Endearing Void");
#endif
    SetTargetFPS(60);

    // Tighten depth buffer — default 0.01/1000 wastes precision
    // Our scenes are max ~100m deep; 0.05/300 gives ~20x better z-resolution
    rlSetClipPlanes(0.05, 300.0);

    render_target = LoadRenderTexture(RENDER_W, RENDER_H);
    SetTextureFilter(render_target.texture, TEXTURE_FILTER_POINT);
    postfx_target = LoadRenderTexture(RENDER_W, RENDER_H);
    SetTextureFilter(postfx_target.texture, TEXTURE_FILTER_POINT);

    lighting = LoadEVLighting();
    CreateShadowMap(&lighting);
    postfx = LoadEVPostFX();
    InitEVAudio(&audio);
    LoadFileMusic(&audio);

    Mesh cube_mesh = GenMeshCube(1.0f, 1.0f, 1.0f);
    cube_model = LoadModelFromMesh(cube_mesh);
    if (lighting.ready) cube_model.materials[0].shader = lighting.shader;
    cube_model_loaded = true;

    Mesh cyl_mesh = GenMeshCylinder(0.5f, 1.0f, 16);
    cyl_model = LoadModelFromMesh(cyl_mesh);
    if (lighting.ready) cyl_model.materials[0].shader = lighting.shader;
    cyl_model_loaded = true;

    // Sphere model — for light fixtures, decorative elements
    Mesh sphere_mesh = GenMeshSphere(0.5f, 8, 8);
    sphere_model = LoadModelFromMesh(sphere_mesh);
    if (lighting.ready) sphere_model.materials[0].shader = lighting.shader;
    sphere_model_loaded = true;

    // Cone model — for lamp shades, decorative elements
    Mesh cone_mesh = GenMeshCone(0.5f, 1.0f, 12);
    cone_model = LoadModelFromMesh(cone_mesh);
    if (lighting.ready) cone_model.materials[0].shader = lighting.shader;
    cone_model_loaded = true;

    // Sky Tower — Auckland landmark, the recurring silhouette
    if (FileExists("assets/skytower.obj")) {
        skytower_model = LoadModel("assets/skytower.obj");
        if (skytower_model.meshCount > 0 && skytower_model.meshes[0].vertexCount > 0) {
            if (lighting.ready) skytower_model.materials[0].shader = lighting.shader;
            skytower_loaded = true;
            printf("[EV] Sky Tower loaded — %d verts, %d tris\n",
                   skytower_model.meshes[0].vertexCount,
                   skytower_model.meshes[0].triangleCount);
        } else {
            printf("[EV] WARNING: Sky Tower mesh empty after load\n");
        }
    } else {
        printf("[EV] WARNING: assets/skytower.obj not found\n");
    }

    DisableCursor();
    SetExitKey(0);  // ESC handled by pause menu, not raylib

#ifdef QA_MODE
    // ============================================================
    // QA MODE — Comprehensive automated analysis pipeline
    // Multi-angle screenshots, quadrant composition, material breakdown,
    // object reachability, contrast ratios, anti-pattern detection.
    // Build: make qa   |   Full pipeline: ./qa/run.sh
    // ============================================================
    EnableCursor();
    SetMasterVolume(0.0f);

    // ── Scene registry with per-scene camera angles ──
    // Each scene gets a "hero" shot (best composition) and "spawn" shot (first impression)
    typedef struct {
        Vector3 pos;        // camera position (0,0,0 = use scene spawn)
        Vector3 target;     // look-at target
    } QACam;
    typedef struct {
        GameState gs;
        const char *name;
        QACam hero;         // the money shot — shows scene at its best
        QACam spawn;        // what the player actually sees first
        bool dark_by_design;
        bool outdoor;
    } QAEntry;

    // Camera angles tuned per-scene based on geometry and hero elements
    QAEntry qa_scenes[] = {
        {STATE_HOTEL_EXT, "hotel_ext",
            .hero  = {{0, 1.6f, 8}, {0, 6, -12}},        // looking up at Sky Tower
            .spawn = {{0, 1.6f, 8}, {0, 1.6f, -5}},      // entrance approach
            .dark_by_design = true, .outdoor = true},
        {STATE_LOBBY, "lobby",
            .hero  = {{0, 1.6f, 6}, {-2, 3, -8}},        // staircase + elevator + chandelier
            .spawn = {{0, 1.6f, 8}, {0, 2, -5}},          // entering from front
            .outdoor = false},
        {STATE_HALLWAY, "hallway",
            .hero  = {{0, 1.6f, -1}, {0, 1.5f, -20}},     // one-point perspective from spawn
            .spawn = {{0, 1.6f, -1}, {0, 1.5f, -20}},      // same — perspective IS the hero
            .outdoor = false},
        {STATE_ROOM, "room",
            .hero  = {{0, 1.6f, 3}, {0, 1.2f, -3.5f}},   // toward bed + headboard
            .spawn = {{5.5f, 1.6f, 4}, {-2, 1.2f, -2}},   // from door toward bed
            .outdoor = false},
        {STATE_BATHROOM, "bathroom",
            .hero  = {{0, 1.6f, 1.5f}, {0, 1.5f, -1.5f}}, // bathtub + Ando window
            .spawn = {{0, 1.6f, 1.5f}, {1.5f, 1.2f, 0}},  // toward sink/mirror
            .dark_by_design = true,  // clinical flat-lit by design — exempt from contrast check
            .outdoor = false},
        {STATE_BALCONY, "balcony",
            .hero  = {{0, 1.6f, 0.5f}, {0, 0.5f, -10}},   // Earth bands — Rothko
            .spawn = {{0, 1.6f, 0.5f}, {-1, 0.8f, -1}},   // wine table foreground
            .dark_by_design = true, .outdoor = true},
        {STATE_ELEVATOR, "elevator",
            .hero  = {{0, 1.6f, 0}, {0.5f, 1.0f, -1.0f}},   // button panel + walls visible
            .spawn = {{0, 1.6f, 0}, {-0.3f, 2.5f, -0.8f}},  // up at ceiling + mirror
            .outdoor = false},
        {STATE_SPACE_LOBBY, "space_lobby",
            .hero  = {{0, 1.6f, 4}, {0, 3, -7}},          // observation window + Earth
            .spawn = {{0, 1.6f, 6}, {0, 2, -3}},           // entering, chandelier visible
            .outdoor = true},
        {STATE_SPACE_CORRIDOR, "space_corridor",
            .hero  = {{0, 0, 0}, {0, 1.6f, -12}},          // from spawn looking down corridor
            .spawn = {{0, 0, 0}, {-1, 1.6f, 0}},            // toward first porthole
            .outdoor = false},
        {STATE_SPACE_SUITE, "space_suite",
            .hero  = {{3, 1.6f, 0}, {-6, 2, -2}},         // from right looking at Earth window
            .spawn = {{0, 1.6f, 5}, {0, 1.6f, -4}},        // entering, bed visible
            .outdoor = false},
        {STATE_CAR, "taxi",
            .hero  = {{0, 0, 0}, {-0.3f, 0.9f, -1.2f}},   // driver + dashboard
            .spawn = {{0, 0, 0}, {0.5f, 0.8f, -0.5f}},     // windshield + city
            .dark_by_design = true, .outdoor = false},
        {STATE_HYPERSPACE, "hyperspace",
            .hero  = {{0, 0, 0}, {0, 0, -20}},             // tunnel convergence
            .spawn = {{0, 0, 0}, {0, 0, -20}},
            .dark_by_design = true, .outdoor = false},
        {STATE_PARIS_DREAM, "paris_dream",
            .hero  = {{0, 1.6f, 4}, {0, 1.2f, -3.5f}},      // toward bed + photograph
            .spawn = {{0, 1.6f, 4}, {0, 1.6f, 0}},
            .dark_by_design = false, .outdoor = false},
        {STATE_RETURN_TAXI, "return_taxi",
            .hero  = {{0, 1.0f, 0}, {-0.3f, 0.9f, -1.2f}},  // driver + dashboard
            .spawn = {{0, 1.0f, 0}, {0.5f, 0.8f, -0.5f}},
            .dark_by_design = false, .outdoor = false},
    };
    int qa_count = (int)(sizeof(qa_scenes)/sizeof(qa_scenes[0]));

    printf("\n[QA] ============================================================\n");
    printf("[QA]  ENDEARING VOID — Comprehensive QA Analysis\n");
    printf("[QA]  Scenes: %d | Render: %dx%d | MAX_WALLS: %d\n", qa_count, RENDER_W, RENDER_H, MAX_WALLS);
    printf("[QA] ============================================================\n\n");

    FILE *jf = fopen("qa/report.json", "w");
    if (jf) fprintf(jf, "{\n  \"render\": \"%dx%d\",\n  \"max_walls\": %d,\n  \"scenes\": [\n", RENDER_W, RENDER_H, MAX_WALLS);

    int qa_total_issues = 0;

    for (int qi = 0; qi < qa_count; qi++) {
        load_state(qa_scenes[qi].gs);
        fade_alpha = 0.0f; fade_target = 0.0f;

        // ── Render helper: take a screenshot from a given angle ──
        #define QA_RENDER_SHOT(cam_setup, suffix) do { \
            if (cam_setup.pos.x != 0 || cam_setup.pos.y != 0 || cam_setup.pos.z != 0) \
                player.camera.position = cam_setup.pos; \
            player.camera.target = cam_setup.target; \
            for (int _f = 0; _f < 8; _f++) { \
                BeginTextureMode(render_target); \
                ClearBackground(scene.fog_color.a > 0 ? scene.fog_color : (Color){8,10,22,255}); \
                draw_scene_3d(&player, &scene, &lighting, \
                    &cube_model, cube_model_loaded, &cyl_model, cyl_model_loaded, \
                    &sphere_model, sphere_model_loaded, &cone_model, cone_model_loaded, \
                    &skytower_model, skytower_loaded, \
                    !qa_scenes[qi].outdoor, (float)_f * 0.016f); \
                EndTextureMode(); \
                BeginTextureMode(postfx_target); ClearBackground(BLACK); \
                draw_postfx(&postfx, render_target); EndTextureMode(); \
                BeginDrawing(); ClearBackground(BLACK); EndDrawing(); \
            } \
            RenderTexture2D _disp = postfx.ready ? postfx_target : render_target; \
            Image _img = LoadImageFromTexture(_disp.texture); \
            ImageFlipVertical(&_img); \
            char _path[256]; \
            snprintf(_path, sizeof(_path), "qa/screenshots/%s_%s.png", qa_scenes[qi].name, suffix); \
            ExportImage(_img, _path); \
            UnloadImage(_img); \
        } while(0)

        // Take hero shot — this is also the image we analyze
        QA_RENDER_SHOT(qa_scenes[qi].hero, "hero");

        // Capture hero image for analysis (re-render from same angle)
        // The macro already rendered; just grab the framebuffer
        RenderTexture2D display = postfx.ready ? postfx_target : render_target;
        Image img = LoadImageFromTexture(display.texture);
        ImageFlipVertical(&img);
        char primary_path[256];
        snprintf(primary_path, sizeof(primary_path), "qa/screenshots/%s.png", qa_scenes[qi].name);
        ExportImage(img, primary_path);

        // Take spawn shot
        load_state(qa_scenes[qi].gs);
        fade_alpha = 0.0f; fade_target = 0.0f;
        QA_RENDER_SHOT(qa_scenes[qi].spawn, "spawn");

        Color *pixels = LoadImageColors(img);
        int total_px = RENDER_W * RENDER_H;
        int half_w = RENDER_W / 2, half_h = RENDER_H / 2;

        // ── Full-frame analysis ──
        long r_sum = 0, g_sum = 0, b_sum = 0;
        int black_px = 0, bright_px = 0, mid_px = 0;
        float r_sq = 0, g_sq = 0, b_sq = 0;
        int unique_hues[36] = {0};

        // ── Quadrant analysis (TL, TR, BL, BR) ──
        float q_luma[4] = {0};
        long q_r[4] = {0}, q_g[4] = {0}, q_b[4] = {0};
        int q_black[4] = {0};
        int q_px = half_w * half_h;

        // ── Center region (inner 50%) ──
        int cx0 = RENDER_W / 4, cx1 = RENDER_W * 3 / 4;
        int cy0 = RENDER_H / 4, cy1 = RENDER_H * 3 / 4;
        float center_luma = 0;
        int center_px = 0;
        float edge_luma = 0;
        int edge_px = 0;

        // ── Edge detection (Sobel-lite: horizontal luminance gradient) ──
        int strong_edges = 0;

        for (int p = 0; p < total_px; p++) {
            unsigned char pr = pixels[p].r, pg = pixels[p].g, pb = pixels[p].b;
            float pl = 0.299f*pr + 0.587f*pg + 0.114f*pb;
            r_sum += pr; g_sum += pg; b_sum += pb;
            r_sq += (float)pr*pr; g_sq += (float)pg*pg; b_sq += (float)pb*pb;
            if (pr < 15 && pg < 15 && pb < 15) black_px++;
            else if (pr > 230 && pg > 230 && pb > 230) bright_px++;
            else if (pl > 40 && pl < 200) mid_px++;

            // Hue bucket
            float rf = pr/255.0f, gf = pg/255.0f, bf = pb/255.0f;
            float cmax = rf > gf ? (rf > bf ? rf : bf) : (gf > bf ? gf : bf);
            float cmin = rf < gf ? (rf < bf ? rf : bf) : (gf < bf ? gf : bf);
            float delta = cmax - cmin;
            if (delta > 0.05f) {
                float hue = 0;
                if (cmax == rf) hue = 60.0f * fmodf((gf-bf)/delta, 6.0f);
                else if (cmax == gf) hue = 60.0f * ((bf-rf)/delta + 2.0f);
                else hue = 60.0f * ((rf-gf)/delta + 4.0f);
                if (hue < 0) hue += 360.0f;
                unique_hues[(int)(hue/10.0f) % 36]++;
            }

            // Quadrant
            int px_x = p % RENDER_W, px_y = p / RENDER_W;
            int qi_quad = (px_x >= half_w ? 1 : 0) + (px_y >= half_h ? 2 : 0);
            q_r[qi_quad] += pr; q_g[qi_quad] += pg; q_b[qi_quad] += pb;
            if (pr < 15 && pg < 15 && pb < 15) q_black[qi_quad]++;

            // Center vs edge
            if (px_x >= cx0 && px_x < cx1 && px_y >= cy0 && px_y < cy1) {
                center_luma += pl; center_px++;
            } else {
                edge_luma += pl; edge_px++;
            }

            // Edge detection (horizontal + vertical gradients)
            if (px_x > 0 && px_x < RENDER_W - 1) {
                Color left = pixels[p - 1], right = pixels[p + 1];
                float ll = 0.299f*left.r + 0.587f*left.g + 0.114f*left.b;
                float rl = 0.299f*right.r + 0.587f*right.g + 0.114f*right.b;
                if (fabsf(rl - ll) > 25) strong_edges++;
            }
            if (px_y > 0 && px_y < RENDER_H - 1) {
                Color above = pixels[p - RENDER_W], below = pixels[p + RENDER_W];
                float al = 0.299f*above.r + 0.587f*above.g + 0.114f*above.b;
                float bl2 = 0.299f*below.r + 0.587f*below.g + 0.114f*below.b;
                if (fabsf(bl2 - al) > 25) strong_edges++;
            }
        }

        int hue_count = 0;
        for (int h = 0; h < 36; h++) if (unique_hues[h] > total_px/500) hue_count++;

        float r_avg = (float)r_sum/total_px, g_avg = (float)g_sum/total_px, b_avg = (float)b_sum/total_px;
        float luma = 0.299f*r_avg + 0.587f*g_avg + 0.114f*b_avg;
        float cvar = (r_sq/total_px - r_avg*r_avg) + (g_sq/total_px - g_avg*g_avg) + (b_sq/total_px - b_avg*b_avg);
        float black_pct = 100.0f*black_px/total_px;
        float bright_pct = 100.0f*bright_px/total_px;
        float mid_pct = 100.0f*mid_px/total_px;
        float edge_density = 100.0f*strong_edges/total_px;

        // Quadrant lumas
        for (int q = 0; q < 4; q++) {
            float qr = (float)q_r[q]/q_px, qg = (float)q_g[q]/q_px, qb = (float)q_b[q]/q_px;
            q_luma[q] = 0.299f*qr + 0.587f*qg + 0.114f*qb;
        }
        float q_luma_max = q_luma[0], q_luma_min = q_luma[0];
        for (int q = 1; q < 4; q++) {
            if (q_luma[q] > q_luma_max) q_luma_max = q_luma[q];
            if (q_luma[q] < q_luma_min) q_luma_min = q_luma[q];
        }
        float contrast_ratio = q_luma_min > 0.01f ? q_luma_max / q_luma_min : 999.0f;
        float lr_balance = (q_luma[0] + q_luma[2]) > 0 ?
            fabsf((q_luma[0]+q_luma[2]) - (q_luma[1]+q_luma[3])) / ((q_luma[0]+q_luma[2]+q_luma[1]+q_luma[3])/2.0f) : 0;
        float tb_balance = (q_luma[0] + q_luma[1]) > 0 ?
            fabsf((q_luma[0]+q_luma[1]) - (q_luma[2]+q_luma[3])) / ((q_luma[0]+q_luma[1]+q_luma[2]+q_luma[3])/2.0f) : 0;
        float center_avg = center_px > 0 ? center_luma / center_px : 0;
        float edge_avg = edge_px > 0 ? edge_luma / edge_px : 0;

        // ── Color temperature: warm (red-shifted) vs cool (blue-shifted) ──
        float warmth = r_avg - b_avg;  // positive = warm, negative = cool

        // ── Material breakdown ──
        int mat_counts[13] = {0};  // one per MaterialType
        for (int w = 0; w < scene.wall_count; w++)
            mat_counts[scene.walls[w].material]++;
        int mat_types_used = 0;
        for (int m = 0; m < 13; m++) if (mat_counts[m] > 0) mat_types_used++;
        int mat_tagged = scene.wall_count - mat_counts[0];  // non-concrete
        float mat_cov = scene.wall_count > 0 ? 100.0f*mat_tagged/scene.wall_count : 0;
        float wall_pct = 100.0f*scene.wall_count/MAX_WALLS;

        // ── Object reachability from spawn ──
        int interact_count = scene.object_count;
        float obj_min_dist = 999, obj_max_dist = 0;
        int obj_unreachable = 0;  // farther than 3.0 from spawn
        for (int oi = 0; oi < scene.object_count; oi++) {
            if (!scene.objects[oi].active) continue;
            float dx = scene.objects[oi].pos.x - scene.spawn.x;
            float dz = scene.objects[oi].pos.z - scene.spawn.z;
            float dist = sqrtf(dx*dx + dz*dz);
            if (dist < obj_min_dist) obj_min_dist = dist;
            if (dist > obj_max_dist) obj_max_dist = dist;
            // Check if within reasonable walk distance (corridor is ~32m)
            if (dist > 25.0f) obj_unreachable++;
        }

        // ── Print summary line ──
        int issues = 0;
        char ibuf[4096] = {0};
        int ib = 0;
        bool dark = qa_scenes[qi].dark_by_design;

        // COMPOSITION checks
        if (edge_density < 0.5f && !dark) {
            ib += snprintf(ibuf+ib, sizeof(ibuf)-(size_t)ib,
                "    COMPOSITION: edge density %.1f%% — no visible geometry/detail\n", edge_density); issues++;
        }
        if (contrast_ratio < 1.3f && !dark) {
            ib += snprintf(ibuf+ib, sizeof(ibuf)-(size_t)ib,
                "    COMPOSITION: contrast ratio %.1f:1 — flat, no drama\n", contrast_ratio); issues++;
        }
        if (center_avg < 10 && edge_avg < 10 && !dark) {
            ib += snprintf(ibuf+ib, sizeof(ibuf)-(size_t)ib,
                "    COMPOSITION: center luma %.0f — nothing visible in frame center\n", center_avg); issues++;
        }

        // LIGHTING checks
        if (black_pct > 85.0f && !dark) {
            ib += snprintf(ibuf+ib, sizeof(ibuf)-(size_t)ib,
                "    LIGHTING: %.0f%% black — scene is unlit\n", black_pct); issues++;
        }
        if (luma < 5.0f && !dark) {
            ib += snprintf(ibuf+ib, sizeof(ibuf)-(size_t)ib,
                "    LIGHTING: luma %.1f — nearly invisible\n", luma); issues++;
        }
        if (bright_pct > 60.0f) {
            ib += snprintf(ibuf+ib, sizeof(ibuf)-(size_t)ib,
                "    LIGHTING: %.0f%% blown out — over-exposed\n", bright_pct); issues++;
        }

        // MATERIAL checks
        if (mat_cov < 25.0f && scene.wall_count > 20) {
            ib += snprintf(ibuf+ib, sizeof(ibuf)-(size_t)ib,
                "    MATERIAL: only %.0f%% non-concrete — surface monotony\n", mat_cov); issues++;
        }
        if (mat_types_used < 3 && scene.wall_count > 30) {
            ib += snprintf(ibuf+ib, sizeof(ibuf)-(size_t)ib,
                "    MATERIAL: only %d types — needs variety\n", mat_types_used); issues++;
        }

        // COLOR checks
        if (cvar < 50.0f && !dark) {
            ib += snprintf(ibuf+ib, sizeof(ibuf)-(size_t)ib,
                "    COLOR: variance %.0f — flat, no accent colors\n", cvar); issues++;
        }
        if (hue_count < 2 && !dark) {
            ib += snprintf(ibuf+ib, sizeof(ibuf)-(size_t)ib,
                "    COLOR: only %d hue buckets — monotone\n", hue_count); issues++;
        }

        // BUDGET checks
        if (wall_pct > 85.0f) {
            ib += snprintf(ibuf+ib, sizeof(ibuf)-(size_t)ib,
                "    BUDGET: wall budget %.0f%% (%d/%d) — near overflow\n", wall_pct, scene.wall_count, MAX_WALLS); issues++;
        }

        // INTERACTION checks
        if (obj_unreachable > 0) {
            ib += snprintf(ibuf+ib, sizeof(ibuf)-(size_t)ib,
                "    INTERACTION: %d object(s) too far from spawn\n", obj_unreachable); issues++;
        }

        // ANTI-PATTERN checks
        // Horror: blue-green dominant + low luma + high contrast
        if (warmth < -15 && luma < 40 && !dark && contrast_ratio > 3) {
            ib += snprintf(ibuf+ib, sizeof(ibuf)-(size_t)ib,
                "    ANTI-PATTERN: reads as horror (cold %.0f, dark luma %.0f)\n", warmth, luma); issues++;
        }
        // Grey-on-grey in space (space scenes should have color)
        if ((qa_scenes[qi].gs == STATE_SPACE_LOBBY || qa_scenes[qi].gs == STATE_SPACE_CORRIDOR ||
             qa_scenes[qi].gs == STATE_SPACE_SUITE) && hue_count < 3) {
            ib += snprintf(ibuf+ib, sizeof(ibuf)-(size_t)ib,
                "    ANTI-PATTERN: grey-on-grey in space — hull must pop against void\n"); issues++;
        }

        const char *qs = issues == 0 ? "PASS" : "FAIL";
        printf("[QA] %-18s %s  walls:%3d  mat:%d types (%.0f%%)  luma:%.0f  contrast:%.1f:1  edges:%.0f%%  hues:%d  objects:%d\n",
               qa_scenes[qi].name, qs, scene.wall_count,
               mat_types_used, mat_cov, luma, contrast_ratio, edge_density, hue_count, interact_count);
        if (issues > 0) printf("%s", ibuf);
        qa_total_issues += issues;

        // ── JSON entry ──
        if (jf) {
            if (qi > 0) fprintf(jf, ",\n");
            fprintf(jf, "    {\n");
            fprintf(jf, "      \"name\": \"%s\",\n", qa_scenes[qi].name);
            fprintf(jf, "      \"status\": \"%s\",\n", qs);
            fprintf(jf, "      \"walls\": %d,\n", scene.wall_count);
            fprintf(jf, "      \"wall_pct\": %.1f,\n", wall_pct);
            fprintf(jf, "      \"material_types\": %d,\n", mat_types_used);
            fprintf(jf, "      \"material_coverage\": %.1f,\n", mat_cov);
            fprintf(jf, "      \"material_breakdown\": {");
            const char *mat_names[] = {"concrete","marble","wood","carpet","wallpaper","tile","brass","glass","leather","fabric","checkerboard","herringbone","parquet"};
            bool first_mat = true;
            for (int m = 0; m < 13; m++) {
                if (mat_counts[m] > 0) {
                    fprintf(jf, "%s\"%s\": %d", first_mat ? "" : ", ", mat_names[m], mat_counts[m]);
                    first_mat = false;
                }
            }
            fprintf(jf, "},\n");
            fprintf(jf, "      \"interact_count\": %d,\n", interact_count);
            fprintf(jf, "      \"obj_unreachable\": %d,\n", obj_unreachable);
            fprintf(jf, "      \"obj_max_dist\": %.1f,\n", obj_max_dist);
            fprintf(jf, "      \"luma\": %.1f,\n", luma);
            fprintf(jf, "      \"black_pct\": %.1f,\n", black_pct);
            fprintf(jf, "      \"bright_pct\": %.1f,\n", bright_pct);
            fprintf(jf, "      \"mid_tone_pct\": %.1f,\n", mid_pct);
            fprintf(jf, "      \"color_variance\": %.1f,\n", cvar);
            fprintf(jf, "      \"hue_buckets\": %d,\n", hue_count);
            fprintf(jf, "      \"warmth\": %.1f,\n", warmth);
            fprintf(jf, "      \"avg_rgb\": [%.0f, %.0f, %.0f],\n", r_avg, g_avg, b_avg);
            fprintf(jf, "      \"contrast_ratio\": %.2f,\n", contrast_ratio);
            fprintf(jf, "      \"edge_density\": %.1f,\n", edge_density);
            fprintf(jf, "      \"center_luma\": %.1f,\n", center_avg);
            fprintf(jf, "      \"edge_luma\": %.1f,\n", edge_avg);
            fprintf(jf, "      \"lr_balance\": %.3f,\n", lr_balance);
            fprintf(jf, "      \"tb_balance\": %.3f,\n", tb_balance);
            fprintf(jf, "      \"quadrant_luma\": [%.1f, %.1f, %.1f, %.1f],\n", q_luma[0], q_luma[1], q_luma[2], q_luma[3]);
            fprintf(jf, "      \"dark_by_design\": %s,\n", dark ? "true" : "false");
            fprintf(jf, "      \"outdoor\": %s,\n", qa_scenes[qi].outdoor ? "true" : "false");
            fprintf(jf, "      \"issues\": %d,\n", issues);
            fprintf(jf, "      \"screenshots\": {\"hero\": \"screenshots/%s_hero.png\", \"spawn\": \"screenshots/%s_spawn.png\"}\n",
                    qa_scenes[qi].name, qa_scenes[qi].name);
            fprintf(jf, "    }");
        }

        UnloadImageColors(pixels);
        UnloadImage(img);
    }
    #undef QA_RENDER_SHOT

    if (jf) {
        fprintf(jf, "\n  ],\n  \"total_issues\": %d,\n  \"scene_count\": %d\n}\n", qa_total_issues, qa_count);
        fclose(jf);
    }

    printf("\n[QA] === %d issue%s across %d scenes ===\n",
           qa_total_issues, qa_total_issues == 1 ? "" : "s", qa_count);
    printf("[QA] Screenshots: qa/screenshots/ (hero + spawn per scene)\n");
    printf("[QA] Report:      qa/report.json\n");
    printf("[QA] Analysis:    ./qa/run.sh   (full pipeline with grades)\n\n");

    if (cube_model_loaded) UnloadModel(cube_model);
    if (cyl_model_loaded) UnloadModel(cyl_model);
    if (sphere_model_loaded) UnloadModel(sphere_model);
    if (cone_model_loaded) UnloadModel(cone_model);
    if (skytower_loaded) UnloadModel(skytower_model);
    UnloadEVLighting(&lighting);
    UnloadEVPostFX(&postfx);
    UnloadFileMusic(&audio);
    UnloadEVAudio(&audio);
    UnloadRenderTexture(render_target);
    UnloadRenderTexture(postfx_target);
    CloseWindow();
    return qa_total_issues > 0 ? 1 : 0;

#else  // normal game

#ifdef DEV_START
    load_state(DEV_START);
#else
    load_state(STATE_TITLE);
#endif

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        UpdateFileMusic(&audio);  // stream music buffers (even when paused)
        update_menu_springs(dt);
        bool menu_active = update_pause_menu();
        if (!menu_active) {
            state_time += dt;
            total_time += dt;
        }

        // Decay scene-cut flash
        if (cut_flash_timer > 0) {
            cut_flash_timer -= dt;
            if (cut_flash_timer <= 0) {
                cut_flash_timer = 0;
                SetPostFXFlash(&postfx, 0.0f, 1.0f, 1.0f, 1.0f);
            } else {
                // Rapid exponential decay — bright to gone in ~0.12s
                float t = cut_flash_timer / 0.12f;
                SetPostFXFlash(&postfx, t * t, 1.0f, 0.95f, 0.85f);
            }
        }

        if (!menu_active) {  // ── begin game logic gate (paused = frozen) ──

#ifndef PLAYTEST
        if (IsKeyPressed(KEY_F1)) wireframe = !wireframe;
        if (IsKeyPressed(KEY_F3)) show_debug = !show_debug;
        if (IsKeyPressed(KEY_F4)) player.noclip = !player.noclip;

        // F5: Nudge mode — select and reposition walls in-game
        if (IsKeyPressed(KEY_F5)) {
            nudge_mode = !nudge_mode;
            if (nudge_mode) {
                nudge_selected = raycast_wall(player.camera, &scene);
                show_text("NUDGE MODE");
            } else {
                nudge_selected = -1;
                hide_text();
            }
        }

        // Nudge controls — arrow keys move selected wall, prints new pos to stdout
        if (nudge_mode) {
            // Re-select on click
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                nudge_selected = raycast_wall(player.camera, &scene);
            }
            if (nudge_selected >= 0 && nudge_selected < scene.wall_count) {
                Wall *w = &scene.walls[nudge_selected];
                bool fine = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
                bool vert = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
                float step = fine ? 0.01f : 0.1f;
                bool moved = false;

                // Arrow keys: X/Z movement (or Y with Ctrl)
                // Hold key = continuous nudge via IsKeyDown at 10Hz
                static float nudge_repeat = 0;
                nudge_repeat -= dt;
                bool repeat_ok = nudge_repeat <= 0;

                #define NUDGE_KEY(key) (IsKeyPressed(key) || (IsKeyDown(key) && repeat_ok))
                bool any_held = IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_LEFT) ||
                                IsKeyDown(KEY_UP) || IsKeyDown(KEY_DOWN) ||
                                IsKeyDown(KEY_RIGHT_BRACKET) || IsKeyDown(KEY_LEFT_BRACKET);

                if (NUDGE_KEY(KEY_RIGHT)) { w->pos.x += step; moved = true; }
                if (NUDGE_KEY(KEY_LEFT))  { w->pos.x -= step; moved = true; }
                if (!vert) {
                    if (NUDGE_KEY(KEY_UP))   { w->pos.z -= step; moved = true; }
                    if (NUDGE_KEY(KEY_DOWN)) { w->pos.z += step; moved = true; }
                } else {
                    if (NUDGE_KEY(KEY_UP))   { w->pos.y += step; moved = true; }
                    if (NUDGE_KEY(KEY_DOWN)) { w->pos.y -= step; moved = true; }
                }
                // Resize with [ and ]
                if (NUDGE_KEY(KEY_RIGHT_BRACKET)) { w->size.x += step; w->size.z += step; moved = true; }
                if (NUDGE_KEY(KEY_LEFT_BRACKET))  { w->size.x -= step; w->size.z -= step; moved = true; }
                #undef NUDGE_KEY

                if (moved && any_held) nudge_repeat = 0.1f;  // 10Hz repeat

                if (moved) {
                    printf("[NUDGE] wall[%d] pos=(%.2ff, %.2ff, %.2ff) size=(%.2ff, %.2ff, %.2ff)\n",
                           nudge_selected, w->pos.x, w->pos.y, w->pos.z,
                           w->size.x, w->size.y, w->size.z);
                }
            }
        }

        // Write current state for dev-watch
        write_state_file(state);

        // Visual style presets — Shift+number (1-9)
        if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) {
            int new_style = -1;
            if (IsKeyPressed(KEY_ONE))   new_style = 0;
            if (IsKeyPressed(KEY_TWO))   new_style = 1;
            if (IsKeyPressed(KEY_THREE)) new_style = 2;
            if (IsKeyPressed(KEY_FOUR))  new_style = 3;
            if (IsKeyPressed(KEY_FIVE))  new_style = 4;
            if (IsKeyPressed(KEY_SIX))   new_style = 5;
            if (IsKeyPressed(KEY_SEVEN)) new_style = 6;
            if (IsKeyPressed(KEY_EIGHT)) new_style = 7;
            if (IsKeyPressed(KEY_NINE))  new_style = 8;
            if (new_style >= 0 && new_style < STYLE_COUNT) {
                current_style = new_style;
                ApplyVisualStyle(&postfx, current_style);
                // Reapply exposure with new style bias
                SetPostFXExposure(&postfx, scene_exposure + visual_styles[current_style].exposure_bias);
                show_text(visual_styles[current_style].name);
            }
        } else {
        // Dev scene jumps — number keys for instant room testing (no shift held)
        if (IsKeyPressed(KEY_ONE))   load_state(STATE_HOTEL_EXT);
        if (IsKeyPressed(KEY_TWO))   load_state(STATE_LOBBY);
        if (IsKeyPressed(KEY_THREE)) load_state(STATE_HALLWAY);
        if (IsKeyPressed(KEY_FOUR))  load_state(STATE_ROOM);
        if (IsKeyPressed(KEY_FIVE))  load_state(STATE_BALCONY);
        if (IsKeyPressed(KEY_SIX))   load_state(STATE_BATHROOM);
        if (IsKeyPressed(KEY_SEVEN)) load_state(STATE_SPACE_LOBBY);
        if (IsKeyPressed(KEY_EIGHT)) load_state(STATE_SPACE_CORRIDOR);
        if (IsKeyPressed(KEY_NINE))  load_state(STATE_SPACE_SUITE);
        if (IsKeyPressed(KEY_ZERO))  load_state(STATE_CAR);
        }
#endif // PLAYTEST

        // Fade with hold-in-black for doorway transitions
        if (hold_timer > 0) {
            hold_timer -= dt;
            if (hold_timer <= 0) {
                hold_timer = 0;
                transitioning = false;
                load_state(next_state);
            }
        } else if (fade_alpha != fade_target) {
            float dir = (fade_target > fade_alpha) ? 1 : -1;
            fade_alpha += dir * fade_speed * dt;
            if ((dir > 0 && fade_alpha >= fade_target) || (dir < 0 && fade_alpha <= fade_target)) {
                fade_alpha = fade_target;
                if (transitioning) {
                    hold_timer = transition_hold;
                }
            }
        }

        // Vignette text spring physics (silk preset: mass 0.9, stiffness 280, damping 26)
        {
            float spring_k = 280.0f, spring_d = 26.0f, spring_m = 0.9f;

            // Y offset spring toward 0
            float force_y = -spring_k * text_y_offset - spring_d * text_y_velocity;
            text_y_velocity += (force_y / spring_m) * dt;
            text_y_offset += text_y_velocity * dt;

            // Scale spring toward target
            float force_s = -spring_k * (text_scale - text_scale_target) - spring_d * text_scale_vel;
            text_scale_vel += (force_s / spring_m) * dt;
            text_scale += text_scale_vel * dt;

            // When fully hidden, clear text
            if (text_scale_target < 0.5f && text_scale < 0.01f && fabsf(text_scale_vel) < 0.01f) {
                text_scale = 0.0f;
                text_scale_vel = 0.0f;
                vig_text = NULL;
            }
        }

        // Reward timers
        for (int i = 0; i < scene.object_count; i++)
            if (scene.objects[i].reward_timer > 0) {
                scene.objects[i].reward_timer -= dt * 0.5f;
                if (scene.objects[i].reward_timer < 0) scene.objects[i].reward_timer = 0;
            }

        // ---- UPDATE ----
        switch (state) {
            case STATE_TITLE: {
                static bool title_breath_played = false;
                if (state_time > 1.0f && !title_breath_played) {
                    PlayTitleBreath(&audio);  // subliminal inhale before text
                    title_breath_played = true;
                }
                if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
                    title_breath_played = false;
                    transition_to(STATE_CAR);
                }
                break;
            }

            case STATE_CAR: {
                // 3D taxi ride — mouse look, no movement, city scrolls past
                // Mouse look (yaw/pitch only, no position change)
                Vector2 md = GetMouseDelta();
                float car_yaw = -md.x * ev_mouse_sens;
                float car_pitch = -md.y * ev_mouse_sens;
                Vector3 car_fwd = Vector3Normalize(Vector3Subtract(player.camera.target, player.camera.position));
                Vector3 car_right = Vector3Normalize(Vector3CrossProduct(car_fwd, player.camera.up));
                Matrix car_ym = MatrixRotateY(car_yaw);
                car_fwd = Vector3Transform(car_fwd, car_ym);
                float car_cp = asinf(car_fwd.y);
                float car_np = car_cp + car_pitch;
                if (car_np > 0.8f) car_pitch = 0.8f - car_cp;
                if (car_np < -0.6f) car_pitch = -0.6f - car_cp;
                Matrix car_pm = MatrixRotate(car_right, car_pitch);
                car_fwd = Vector3Transform(car_fwd, car_pm);
                player.camera.target = Vector3Add(player.camera.position, car_fwd);

                // Auto-scroll city walls — accelerating toward Sky Tower
                float taxi_speed = 8.0f;
                if (state_time > 10.0f) {
                    // ACCELERATE — rushing toward the tower, not stopping
                    float t = state_time - 10.0f;
                    taxi_speed = 8.0f + t * t * 2.0f;
                }
                for (int i = scene.static_wall_count; i < scene.wall_count; i++) {
                    scene.walls[i].pos.z += taxi_speed * dt;
                    if (scene.walls[i].pos.z > 10.0f) {
                        scene.walls[i].pos.z -= 220.0f;
                    }
                }

                // Post-FX ramp — the closer to the tower, the more intense
                if (state_time > 10.0f) {
                    float ramp = (state_time - 10.0f) / 3.5f;  // 0→1 over last 3.5s
                    if (ramp > 1.0f) ramp = 1.0f;
                    SetPostFXCA(&postfx, 2.5f + ramp * 8.0f);
                    SetPostFXGrain(&postfx, 0.5f + ramp * 0.3f);
                    // FOV widens with speed
                    player.camera.fovy = 70.0f + ramp * 20.0f;
                }

                // Taxi driver speaks — Bolaño: matter-of-fact, slightly too much
                if (state_time > 1.5f && state_time < 2.0f) show_text("Auckland. 2:47 AM.");
                if (state_time > 4.0f && state_time < 4.5f) hide_text();
                if (state_time > 5.5f && state_time < 6.0f) show_text("You going up to the tower?");
                if (state_time > 8.0f && state_time < 8.5f) hide_text();
                if (state_time > 9.0f && state_time < 9.5f) show_text("They say there's a hotel up there now.");
                if (state_time > 12.0f && state_time < 12.5f) hide_text();
                if (state_time > 12.5f && state_time < 13.0f) show_text("Three hours to kill.");
                // Hard cut straight into the wormhole — taxi → hyperspace → space
                if (state_time > 13.5f) {
                    player.camera.fovy = 70.0f;
                    SetPostFXCA(&postfx, 2.5f);
                    hard_cut_to(STATE_HYPERSPACE);
                }
                if (IsKeyPressed(KEY_ENTER) && state_time > 3) {
                    player.camera.fovy = 70.0f;
                    SetPostFXCA(&postfx, 2.5f);
                    hard_cut_to(STATE_HYPERSPACE);
                }
                break;
            }

            case STATE_DRIVING: {
                // Arrival — taxi stopped, same scene, mouse look only
                Vector2 md2 = GetMouseDelta();
                float drv_yaw = -md2.x * ev_mouse_sens;
                float drv_pitch = -md2.y * ev_mouse_sens;
                Vector3 drv_fwd = Vector3Normalize(Vector3Subtract(player.camera.target, player.camera.position));
                Vector3 drv_right = Vector3Normalize(Vector3CrossProduct(drv_fwd, player.camera.up));
                Matrix drv_ym = MatrixRotateY(drv_yaw);
                drv_fwd = Vector3Transform(drv_fwd, drv_ym);
                float drv_cp = asinf(drv_fwd.y);
                float drv_np = drv_cp + drv_pitch;
                if (drv_np > 0.8f) drv_pitch = 0.8f - drv_cp;
                if (drv_np < -0.6f) drv_pitch = -0.6f - drv_cp;
                Matrix drv_pm = MatrixRotate(drv_right, drv_pitch);
                drv_fwd = Vector3Transform(drv_fwd, drv_pm);
                player.camera.target = Vector3Add(player.camera.position, drv_fwd);

                // No text — the taxi stopping IS the communication
                if (state_time > 7.0f) transition_to(STATE_HOTEL_EXT);
                if (IsKeyPressed(KEY_ENTER) && state_time > 2) transition_to(STATE_HOTEL_EXT);
                break;
            }

            case STATE_HOTEL_EXT:
                update_player(&player, &scene, dt);
                if (scene.has_exit) {
                    float dist = Vector3Distance(player.camera.position, scene.exit_pos);
                    // Threshold crossing — slow the player as they approach the door
                    // "Getting out and grabbing it is a core part of the experience"
                    // Every transition is deliberate, not automatic
                    if (dist < 3.0f) {
                        float threshold_t = 1.0f - (dist / 3.0f);  // 0 at 3m, 1 at door
                        float slow = 1.0f - threshold_t * 0.5f;    // 50% slowdown at door
                        player.vel.x *= slow;
                        player.vel.z *= slow;
                        // FOV narrows — contraction
                        player.fov_current += (65.0f - player.fov_current) * threshold_t * 0.1f;
                    }
                    if (dist < 1.5f) transition_to(STATE_LOBBY);
                }
                break;

            case STATE_LOBBY:
                update_player(&player, &scene, dt);
                UpdateEVAudio(&audio, player.moving, player.sprinting, scene.surface, dt);
                if (IsKeyPressed(KEY_E)) {
                    for (int i = 0; i < scene.object_count; i++) {
                        InteractObject *obj = &scene.objects[i];
                        if (!obj->active || obj->done) continue;
                        float dist = Vector3Distance(player.camera.position, obj->pos);
                        if (dist < obj->radius) {
                            Vector3 to = Vector3Normalize(Vector3Subtract(obj->pos, player.camera.position));
                            Vector3 look = Vector3Normalize(Vector3Subtract(player.camera.target, player.camera.position));
                            if (Vector3DotProduct(to, look) > 0.5f) {
                                if (strcmp(obj->name, "elevator") == 0) {
                                    obj->step++; obj->done = true;
                                    PlayInteract(&audio, INTERACT_CLICK);
                                    transition_to(STATE_ELEVATOR);
                                    break;
                                }
                                if (strcmp(obj->name, "newspaper") == 0) {
                                    obj->step++; obj->done = true;
                                    PlayInteract(&audio, INTERACT_CLICK);
                                    break;
                                }
                            }
                        }
                    }
                }
                // Elevator is the only way up — no hallway exit
                break;

            case STATE_ELEVATOR:
                // The Glass Elevator — brass box ascending to orbit
                // Accelerating upward drift, faster than before
                {
                    float accel = 0.3f + state_time * 0.4f;  // accelerates over time
                    player.camera.position.y += accel * dt;
                    player.camera.target.y += accel * dt;
                }
                // Ding at 2 seconds — the sound is enough
                if (state_time > 2.0f && !elevator_ding_played) {
                    elevator_ding_played = true;
                    PlayElevatorDing(&audio);
                }
                // Doors open
                if (elevator_to_corridor) {
                    // Space lobby → corridor: short ride, ding, arrive
                    if (state_time > 3.0f) {
                        elevator_to_corridor = false;
                        hard_cut_to(STATE_SPACE_CORRIDOR);
                    }
                } else {
                    // Terrestrial → hyperspace: longer ride into the wormhole
                    if (state_time > 5.0f) hard_cut_to(STATE_HYPERSPACE);
                }
                break;

            case STATE_HYPERSPACE:
                // Sprint 3A: 6-phase hyperspace transformation
                // Phase 1 (0-2s): gentle acceleration
                // Phase 2 (2-4s): tunnel narrows, FOV compresses 85→60, volume drops
                // Phase 3 (4-4.5s): silence + black
                // Phase 4 (4.5-6s): release, FOV snaps to 90, hard cut
                {
                    float t = state_time;

                    if (t < 4.0f) {
                        // Phase 1-2: acceleration with FOV manipulation
                        float speed = 8.0f + t * t * 3.0f;
                        player.camera.position.z -= speed * dt;
                        player.camera.target.z -= speed * dt;

                        if (t < 2.0f) {
                            // Phase 1: gentle accel, FOV widens
                            player.camera.fovy = 70.0f + t * 7.5f;  // 70→85
                        } else {
                            // Phase 2: tunnel narrows, FOV compresses
                            float p2 = (t - 2.0f) / 2.0f;  // 0→1 over 2s
                            player.camera.fovy = 85.0f - p2 * 25.0f;  // 85→60
                            // Volume drops — world receding
                            SetMasterVolume(1.0f - p2 * 0.7f);
                        }

                        // Subtle roll — disorienting
                        float roll = sinf(t * 1.5f) * 2.0f;
                        player.camera.up = (Vector3){sinf(roll * DEG2RAD), cosf(roll * DEG2RAD), 0};

                        // Post-FX ramp
                        float ca = 5.0f + t * 4.0f;
                        SetPostFXCA(&postfx, ca > 20.0f ? 20.0f : ca);
                        SetPostFXGrain(&postfx, 0.7f + t * 0.1f);
                        set_exposure(0.1f + sinf(t * 3.0f) * 0.08f);
                        float sat = 0.8f + sinf(t * 2.0f) * 0.3f;
                        SetPostFXSaturation(&postfx, sat > 1.3f ? 1.3f : sat);
                    } else if (t < 4.5f) {
                        // Phase 3: silence + black — the void between
                        SetMasterVolume(0.0f);
                        set_exposure(-1.0f);  // pure black
                    } else if (t < 6.0f) {
                        // Phase 4: release — FOV snaps wide
                        float p4 = (t - 4.5f) / 1.5f;
                        player.camera.fovy = 60.0f + p4 * 30.0f;  // 60→90
                        SetMasterVolume(p4);  // volume returns
                        set_exposure(-1.0f + p4 * 1.1f);
                    }
                }
                // P6: Hyperspace → Space Lobby — 1.5s BLACK + silence before arrival
                if (state_time > 6.0f) {
                    StopHyperspaceTone(&audio);
                    StopHyperspaceRiser(&audio);
                    SetPostFXCA(&postfx, 2.5f);
                    SetPostFXSaturation(&postfx, 0.92f);
                    player.camera.fovy = 70.0f;
                    player.camera.up = (Vector3){0, 1, 0};
                    // Extended hold in black — true silence before the lobby
                    transition_hold = 1.5f;
                    SetMasterVolume(0.0f);
                    hard_cut_to(STATE_SPACE_LOBBY);
                }
                break;

            case STATE_HALLWAY:
                update_player(&player, &scene, dt);
                UpdateEVAudio(&audio, player.moving, player.sprinting, scene.surface, dt);
                if (scene.has_exit) {
                    float dist = Vector3Distance(player.camera.position, scene.exit_pos);
                    if (dist < 1.5f) transition_to(STATE_ROOM);
                }
                break;

            case STATE_ROOM:
                update_player(&player, &scene, dt);
                UpdateEVAudio(&audio, player.moving, player.sprinting, scene.surface, dt);
                // Boarding pass on desk tells the story — no text needed
                if (IsKeyPressed(KEY_E)) {
                    for (int i = 0; i < scene.object_count; i++) {
                        InteractObject *obj = &scene.objects[i];
                        if (!obj->active || obj->done) continue;
                        float dist = Vector3Distance(player.camera.position, obj->pos);
                        if (dist < obj->radius) {
                            Vector3 to = Vector3Normalize(Vector3Subtract(obj->pos, player.camera.position));
                            Vector3 look = Vector3Normalize(Vector3Subtract(player.camera.target, player.camera.position));
                            if (Vector3DotProduct(to, look) > 0.5f) {
                                // Bathroom door — transition, not a task
                                if (strcmp(obj->name, "bathroom") == 0) {
                                    transition_to(STATE_BATHROOM);
                                    break;
                                }
                                // Wardrobe — secret passage to lobby mezzanine
                                if (strcmp(obj->name, "wardrobe") == 0) {
                                    obj->step++; obj->done = true;
                                    PlayInteract(&audio, INTERACT_FABRIC);
                                    kick_camera(&player, -0.03f, 0.01f);
                                    // Hard cut to space lobby — you emerge on the mezzanine
                                    // Disorienting, Blendo-style, but rewarding
                                    StopClockAmbient(&audio);
                                    load_state(STATE_SPACE_LOBBY);
                                    // Override spawn to mezzanine position
                                    player.camera.position = scene.exit_pos;
                                    player.ground_y = 2.4f;  // mezzanine height
                                    player.camera.target = (Vector3){
                                        player.camera.position.x,
                                        player.camera.position.y,
                                        player.camera.position.z - 1
                                    };
                                    fade_alpha = 0.8f; fade_target = 0.0f;
                                    break;
                                }
                                obj->step++;
                                PlayInteract(&audio, get_interact_sound(obj->name));
                                kick_camera(&player, -0.01f, 0.005f);

                                // Per-step visual feedback — world accumulates changes
                                if (strcmp(obj->name, "lamp") == 0 && obj->step == 1) {
                                    // Lamp shade tilting — warm rectangle
                                    add_wall(&scene, -2.5f, 1.0f, -3.65f, 0.15f, 0.25f, 0.15f, (Color){220,210,185,180});
                                } else if (strcmp(obj->name, "candles") == 0 && obj->step == 1) {
                                    // First flame
                                    add_wall(&scene, -4.0f, 0.55f, 3.0f, 0.06f, 0.12f, 0.06f, (Color){255,200,80,200});
                                } else if (strcmp(obj->name, "candles") == 0 && obj->step == 2) {
                                    // Second flame, offset
                                    add_wall(&scene, -4.2f, 0.55f, 3.0f, 0.06f, 0.12f, 0.06f, (Color){255,200,80,200});
                                } else if (strcmp(obj->name, "bed") == 0 && obj->step == 1) {
                                    // Sheets smoothed — brighter rectangle on bed
                                    add_wall(&scene, 0.0f, 0.54f, -3.3f, 2.8f, 0.02f, 1.4f, (Color){245,242,235,255});
                                } else if (strcmp(obj->name, "drawers") == 0 && obj->step == 1) {
                                    // First item unpacked — blue shirt
                                    add_wall(&scene, 2.0f, 0.4f, 3.4f, 0.5f, 0.04f, 0.35f, (Color){55,85,175,255});
                                } else if (strcmp(obj->name, "drawers") == 0 && obj->step == 2) {
                                    // Second item — red scarf
                                    add_wall(&scene, 2.5f, 0.4f, 3.3f, 0.35f, 0.04f, 0.4f, (Color){200,50,45,255});
                                }

                                if (obj->step >= obj->max_steps) {
                                    obj->done = true;
                                    obj->reward_timer = 1.5f;
                                    tasks_done++;
                                    if (completed_count < MAX_OBJECTS)
                                        completed_objects[completed_count++] = obj->name;
                                    PlayRewardSound(&audio);
                                    float progress = (float)tasks_done / PARIS_TASK_COUNT;
                                    SetPostFXWarmth(&postfx, progress);
                                    scene.fog_density = 0.008f - (progress * 0.005f);
                                    // Exposure ramp — room brightens as you settle in
                                    set_exposure(-0.05f + progress * 0.15f);

                                    // Visible consequences per task (Chung, Vlambeer)
                                    if (strcmp(obj->name, "lamp") == 0) {
                                        // Lamp on — geometry glow + move shader point light
                                        add_light_panel(&scene, -2.5f, 1.2f, -3.8f, 0.5f, 0.6f, 0.5f, (Color){255,220,120,200});
                                        SetPointLight(&lighting, -2.5f, 1.2f, -3.8f, 1.0f, 0.82f, 0.45f, 8.0f);
                                    } else if (strcmp(obj->name, "candles") == 0) {
                                        // 3 small flames + move shader point light to candles
                                        add_wall(&scene, -4.0f, 0.55f, 3.0f, 0.06f, 0.12f, 0.06f, (Color){255,180,60,220});
                                        add_wall(&scene, -4.2f, 0.55f, 3.0f, 0.06f, 0.12f, 0.06f, (Color){255,190,70,200});
                                        add_wall(&scene, -4.4f, 0.55f, 3.0f, 0.06f, 0.12f, 0.06f, (Color){255,170,50,210});
                                        SetPointLight(&lighting, -4.2f, 0.55f, 3.0f, 1.0f, 0.7f, 0.25f, 5.0f);
                                    } else if (strcmp(obj->name, "bed") == 0) {
                                        // Small chocolate on the bed
                                        add_wall(&scene, 0.0f, 0.68f, -3.2f, 0.18f, 0.04f, 0.12f, (Color){90,50,25,255});
                                    } else if (strcmp(obj->name, "drawers") == 0) {
                                        // Clothes laid out near suitcase
                                        add_wall(&scene, 2.0f, 0.4f, 3.4f, 0.5f, 0.04f, 0.35f, (Color){55,85,175,255});
                                        add_wall(&scene, 2.5f, 0.4f, 3.3f, 0.35f, 0.04f, 0.4f, (Color){200,50,45,255});
                                        add_wall(&scene, 1.8f, 0.4f, 3.6f, 0.3f, 0.04f, 0.25f, (Color){240,238,230,255});
                                    } else if (strcmp(obj->name, "ashtray") == 0) {
                                        // Clean air — reduce fog further
                                        scene.fog_density = 0.001f;
                                    }

                                    // Camera kick — bigger for final task
                                    if (tasks_done >= PARIS_TASK_COUNT) {
                                        kick_camera(&player, -0.05f, 0.02f);
                                        scene.fog_density = 0.001f;
                                        SetPostFXWarmth(&postfx, 1.0f);
                                        set_exposure(0.1f);  // fully settled — bright
                                    } else {
                                        kick_camera(&player, -0.02f, 0.008f);
                                    }
                                }
                                break;
                            }
                        }
                    }
                }
                // Phone lights up at 3 tasks — screen glow is enough
                if (tasks_done >= 3 && !phone_triggered) {
                    phone_triggered = true;
                    // Bright screen on phone — bigger, brighter, the glow tells the story
                    add_wall(&scene, 5.2f, 0.87f, 0.15f, 0.2f, 0.01f, 0.1f, (Color){100,180,240,220});
                    phone_wall_idx = scene.wall_count - 1;
                }
                // Phone glow pulse — the screen breathes
                if (phone_triggered && phone_wall_idx >= 0) {
                    float pulse = 0.6f + 0.4f * sinf((float)GetTime() * 3.0f);
                    unsigned char pa = (unsigned char)(220 * pulse);
                    scene.walls[phone_wall_idx].color.a = pa;
                }

                if (tasks_done >= PARIS_TASK_COUNT) {
                    done_pause += dt;
                    // HARD CUT to balcony — Blendo-style, no gentle fade
                    if (done_pause > 2.0f) {
                        balcony_flash_triggered = false;
                        balcony_flash_timer = 0;
                        hard_cut_to(STATE_BALCONY);
                    }
                }
                break;

            case STATE_BATHROOM:
                update_player(&player, &scene, dt);
                UpdateEVAudio(&audio, player.moving, player.sprinting, scene.surface, dt);
                if (IsKeyPressed(KEY_E)) {
                    for (int i = 0; i < scene.object_count; i++) {
                        InteractObject *obj = &scene.objects[i];
                        if (!obj->active || obj->done) continue;
                        float dist = Vector3Distance(player.camera.position, obj->pos);
                        if (dist < obj->radius) {
                            Vector3 to = Vector3Normalize(Vector3Subtract(obj->pos, player.camera.position));
                            Vector3 look = Vector3Normalize(Vector3Subtract(player.camera.target, player.camera.position));
                            if (Vector3DotProduct(to, look) > 0.5f) {
                                if (strcmp(obj->name, "door") == 0) {
                                    returning_to_room = true;
                                    transition_to(STATE_ROOM);
                                    break;
                                }
                                if (strcmp(obj->name, "tap") == 0) {
                                    obj->step++;
                                    obj->done = true;
                                    PlayInteract(&audio, INTERACT_CLICK);
                                    kick_camera(&player, -0.01f, 0.005f);
                                    // Running water under the tap
                                    add_wall(&scene, 2.0f, 0.78f, 0.5f, 0.06f, 0.14f, 0.04f, (Color){180,200,220,120});
                                    break;
                                }
                                if (strcmp(obj->name, "mirror") == 0) {
                                    obj->step++;
                                    obj->done = true;
                                    PlayInteract(&audio, INTERACT_CLICK);
                                    kick_camera(&player, -0.01f, 0.005f);
                                    // No text — the mirror reflects enough
                                    break;
                                }
                            }
                        }
                    }
                }
                break;

            case STATE_BALCONY:
                // Sprint 3B: Balcony starts at 0.3 control (can look, barely walk)
                // Before BED transition: 0.0
                {
                    float balc_ctrl = 0.3f;
                    if (state_time > 10.0f) {
                        float fade = fminf(1.0f, (state_time - 10.0f) / 4.0f);
                        balc_ctrl = 0.3f * (1.0f - fade);
                    }
                    player.control_mult = balc_ctrl;
                }
                if (!cigarette_anim) update_player(&player, &scene, dt);
                // Railing approach — the void opens as you lean toward it
                {
                    float railing_z = -1.5f;
                    float dist_to_rail = player.camera.position.z - railing_z;
                    if (dist_to_rail < 1.5f && dist_to_rail > 0) {
                        float t = 1.0f - (dist_to_rail / 1.5f);
                        // FOV widens — the view expands
                        player.fov_current += (80.0f - player.fov_current) * t * 0.06f;
                        // Exposure lifts — Earth brightens
                        set_exposure(0.05f + t * 0.08f);
                    }
                }
                UpdateEVAudio(&audio, player.moving && !cigarette_anim, player.sprinting, scene.surface, dt);
                if (IsKeyPressed(KEY_E)) {
                    for (int i = 0; i < scene.object_count; i++) {
                        InteractObject *obj = &scene.objects[i];
                        if (!obj->active || obj->done) continue;
                        float dist = Vector3Distance(player.camera.position, obj->pos);
                        if (dist < obj->radius) {
                            Vector3 to = Vector3Normalize(Vector3Subtract(obj->pos, player.camera.position));
                            Vector3 look = Vector3Normalize(Vector3Subtract(player.camera.target, player.camera.position));
                            if (Vector3DotProduct(to, look) > 0.5f) {
                                if (strcmp(obj->name, "cigarette") == 0) {
                                    obj->step++; obj->done = true;
                                    PlayInteract(&audio, INTERACT_CLICK);
                                    // Camera animation handles this — Phase 5
                                    cigarette_anim = true;
                                    cigarette_anim_timer = 0;
                                    cigarette_cam_origin = player.camera.position;
                                    // Target: lean toward the cigarette
                                    Vector3 to_cig = Vector3Normalize(Vector3Subtract(obj->pos, player.camera.position));
                                    cigarette_cam_target = Vector3Add(player.camera.position, Vector3Scale(to_cig, 0.5f));
                                    break;
                                }
                            }
                        }
                    }
                }
                // Cigarette camera animation — camera tells the story
                if (cigarette_anim) {
                    cigarette_anim_timer += dt;
                    float t = cigarette_anim_timer;
                    if (t < 0.8f) {
                        // Lean toward cigarette (smoothstep)
                        float s = t / 0.8f;
                        s = s * s * (3.0f - 2.0f * s); // smoothstep
                        player.camera.position = Vector3Lerp(cigarette_cam_origin, cigarette_cam_target, s);
                    } else if (t < 1.3f) {
                        // Hold — Walter considers
                        player.camera.position = cigarette_cam_target;
                    } else if (t < 2.1f) {
                        // Pull back
                        float s = (t - 1.3f) / 0.8f;
                        s = s * s * (3.0f - 2.0f * s);
                        player.camera.position = Vector3Lerp(cigarette_cam_target, cigarette_cam_origin, s);
                    } else {
                        // Done
                        player.camera.position = cigarette_cam_origin;
                        cigarette_anim = false;
                    }
                    // Disable player movement during animation
                    player.camera.target = Vector3Add(player.camera.position, Vector3Normalize(Vector3Subtract(cigarette_cam_target, cigarette_cam_origin)));
                }
                if (!eiffel_sparkle && state_time > 0.5f) {
                    eiffel_sparkle = true; sparkle_timer = 0;
                    PlaySparkleSound(&audio);
                }
                if (eiffel_sparkle) sparkle_timer += dt;
                // Rug pull — at 8 seconds, a flash intrudes. Like a memory.
                if (state_time > 8.0f && !balcony_flash_triggered) {
                    balcony_flash_triggered = true;
                    balcony_flash_timer = 0;
                }
                if (balcony_flash_triggered) balcony_flash_timer += dt;
                // Silence — then the dream (Paris, golden, a memory)
                if (state_time > 14.0f)
                    hard_cut_to(STATE_PARIS_DREAM);
                if (state_time > 10 && IsKeyPressed(KEY_ENTER))
                    hard_cut_to(STATE_PARIS_DREAM);
                break;

            case STATE_PARIS_DREAM:
                // The dream — Paris hotel room, golden light
                // Hotel Chevalier: warm, intimate, already fading
                update_player(&player, &scene, dt);
                // Dream shimmer — warmth oscillates like a memory fading
                {
                    float dream_t = fminf(1.0f, state_time / 5.0f);
                    float shimmer = 0.7f + 0.1f * sinf(state_time * 0.8f);
                    SetPostFXWarmth(&postfx, shimmer * dream_t);
                    // Agency slowly draining — you're losing the dream
                    if (state_time > 30.0f) {
                        float dream_fade = fminf(1.0f, (state_time - 30.0f) / 60.0f);
                        player.control_mult = 1.0f - dream_fade * 0.5f;
                    }
                    // Grain increases over time — the dream is degrading
                    SetPostFXGrain(&postfx, 0.7f + state_time * 0.003f);
                }
                // Interactions: photograph + window
                if (IsKeyPressed(KEY_E)) {
                    for (int i = 0; i < scene.object_count; i++) {
                        InteractObject *obj = &scene.objects[i];
                        if (!obj->active || obj->done) continue;
                        float dist = Vector3Distance(player.camera.position, obj->pos);
                        if (dist < obj->radius) {
                            Vector3 to = Vector3Normalize(Vector3Subtract(obj->pos, player.camera.position));
                            Vector3 look = Vector3Normalize(Vector3Subtract(player.camera.target, player.camera.position));
                            if (Vector3DotProduct(to, look) > 0.4f) {
                                if (strcmp(obj->name, "photograph") == 0) {
                                    obj->step++; obj->done = true;
                                    kick_camera(&player, -0.04f, 0.02f);
                                    show_text("It's blank.");
                                    done_pause = -2.0f;
                                } else if (strcmp(obj->name, "window") == 0) {
                                    obj->step++; obj->done = true;
                                    kick_camera(&player, -0.02f, 0.01f);
                                    show_text("Paris. Before all this.");
                                }
                            }
                            break;
                        }
                    }
                }
                // Dream exit — after photo flip or timeout
                if (done_pause < 0) {
                    done_pause += dt;
                    if (done_pause >= 0) {
                        hide_text();
                        // Reset post-FX before hard cut to BED
                        SetPostFXWarmth(&postfx, 0.0f);
                        SetPostFXSaturation(&postfx, 0.92f);
                        SetPostFXCA(&postfx, 2.5f);
                        SetPostFXVignette(&postfx, 1.0f);
                        SetPostFXContrast(&postfx, 1.0f);
                        hard_cut_to(STATE_BED);
                    }
                }
                // Auto-exit after 90 seconds if no interaction
                if (state_time > 90.0f) {
                    SetPostFXWarmth(&postfx, 0.0f);
                    SetPostFXSaturation(&postfx, 0.92f);
                    SetPostFXCA(&postfx, 2.5f);
                    SetPostFXVignette(&postfx, 1.0f);
                    SetPostFXContrast(&postfx, 1.0f);
                    hard_cut_to(STATE_BED);
                }
                break;

            case STATE_BED:
                // Sprint 1A: Clock deceleration — the emotional peak
                // Don't stop clock on load. Ramp from 1.0→0.0 over 8 seconds.
                // Clock stopping = commandment #4 ("remove the constant")
                if (state_time < 8.0f) {
                    bed_clock_rate = 1.0f - (state_time / 8.0f);
                    if (bed_clock_rate < 0.05f) bed_clock_rate = 0;
                    SetClockRate(&audio, bed_clock_rate);
                } else if (bed_clock_rate > 0) {
                    bed_clock_rate = 0;
                    SetClockRate(&audio, 0);  // silence — the peak
                }
                // Bed drone fades in with ceiling (3s+)
                if (state_time > 3.0f && !audio.bed_drone_playing) {
                    PlayBedDrone(&audio);
                }
                // Camera breathing — looking up at ceiling
                {
                    float breath = sinf(state_time * 0.5f) * 0.002f;
                    player.camera.target.y += breath;
                }
                // Transition — no early skip until 30s (let the player sit in it)
                if (state_time > 20)
                    hard_cut_to(STATE_STARS);
                break;

            case STATE_STARS:
                // The held chord plays. The void holds you.
                // Volume: held chord fades in over 3s, then sustains
                if (state_time < 3.0f) {
                    SetMasterVolume(state_time / 3.0f);
                }
                if (IsKeyPressed(KEY_ESCAPE)) { CloseWindow(); return 0; }
                // Waking up — warmth spikes, exposure blooms, then hard cut to dawn
                if (state_time > 13.0f) {
                    float wake = (state_time - 13.0f) / 2.0f;  // 0→1 over 2s
                    SetPostFXWarmth(&postfx, wake * 0.5f);
                    set_exposure(wake * 0.3f);
                    SetMasterVolume(fmaxf(0, 1.0f - wake));
                }
                if (state_time > 15.0f || (state_time > 8.0f && IsKeyPressed(KEY_ENTER))) {
                    SetMasterVolume(1.0f);
                    SetPostFXWarmth(&postfx, 0.0f);
                    hard_cut_to(STATE_RETURN_TAXI);
                }
                break;

            case STATE_RETURN_TAXI: {
                // Dawn Auckland — the same ride, inverted.
                // Mouse look only (same as STATE_CAR)
                Vector2 md_r = GetMouseDelta();
                float ry = -md_r.x * ev_mouse_sens;
                float rp = -md_r.y * ev_mouse_sens;
                Vector3 rfwd = Vector3Normalize(Vector3Subtract(player.camera.target, player.camera.position));
                Vector3 rright = Vector3Normalize(Vector3CrossProduct(rfwd, player.camera.up));
                rfwd = Vector3Transform(rfwd, MatrixRotateY(ry));
                float rcp = asinf(rfwd.y);
                float rnp = rcp + rp;
                if (rnp > 0.8f) rp = 0.8f - rcp;
                if (rnp < -0.6f) rp = -0.6f - rcp;
                rfwd = Vector3Transform(rfwd, MatrixRotate(rright, rp));
                player.camera.target = Vector3Add(player.camera.position, rfwd);

                // City scrolls — DECELERATING (opposite of opening)
                float rtaxi_speed = 6.0f;
                if (state_time > 8.0f) {
                    float t2 = state_time - 8.0f;
                    rtaxi_speed = 6.0f - t2 * t2 * 0.3f;
                    if (rtaxi_speed < 0.5f) rtaxi_speed = 0.5f;
                }
                for (int i = scene.static_wall_count; i < scene.wall_count; i++) {
                    scene.walls[i].pos.z += rtaxi_speed * dt;
                    if (scene.walls[i].pos.z > 10.0f)
                        scene.walls[i].pos.z -= 220.0f;
                }

                // Dialogue — sparse, dawn. The inverse of the opening.
                if (state_time > 3.0f && state_time < 3.5f) show_text("Auckland. 5:52 AM.");
                if (state_time > 6.0f && state_time < 6.5f) hide_text();
                if (state_time > 8.0f && state_time < 8.5f) show_text("Good hotel?");
                if (state_time > 11.0f && state_time < 11.5f) hide_text();
                // Saturation slowly drains — reality is less vivid than the hotel
                {
                    float drain = fminf(1.0f, state_time / 14.0f);
                    SetPostFXSaturation(&postfx, 1.05f - drain * 0.25f);
                }

                // Gentle fade to black — the game ends
                if (state_time > 14.0f) {
                    float fo = (state_time - 14.0f) / 3.0f;
                    SetMasterVolume(fmaxf(0, 1.0f - fo));
                    fade_alpha = fminf(1.0f, fo);
                    fade_target = 1.0f;
                    if (state_time > 17.0f) {
                        CloseWindow(); return 0;
                    }
                }
                break;
            }

            case STATE_SPACE_LOBBY:
                update_player(&player, &scene, dt);
                // Arrival ding — you've reached your floor
                if (state_time > 1.5f && !elevator_ding_played) {
                    elevator_ding_played = true;
                    PlayElevatorDing(&audio);
                }
                // SPEED MANIPULATION — awe near the observation window
                // Beginner's Guide technique: agency as a dial, not a switch
                {
                    float pz = player.camera.position.z;
                    // Approaching the window: graduated slowdown starting at z < -2
                    if (pz < -2.0f) {
                        float t = fminf(1.0f, (-2.0f - pz) / 5.5f);
                        float slow = 1.0f - t * 0.55f;
                        player.vel.x *= slow;
                        player.vel.z *= slow;
                        // FOV narrows — tunnel vision on the window
                        player.fov_current += (60.0f - player.fov_current) * t * 0.04f;
                        // Exposure lifts — Earth fills your vision with light
                        set_exposure(-0.05f + t * 0.12f);
                        // Grain drops — the closer you get, the cleaner the image
                        SetPostFXGrain(&postfx, 0.4f - t * 0.2f);
                    }
                    // Earth glow pulse — the planet breathes light into the room
                    {
                        float pulse = 0.8f + 0.2f * sinf(state_time * 0.5f);
                        SetPointLightIdx(&lighting, 0,
                            0.0f, 0.5f, -7.0f,
                            0.24f * pulse, 0.51f * pulse, 0.78f * pulse,
                            12.0f * pulse);
                        // Earth sub-bass: louder near window (z < -4), silent far away
                        float window_dist = fabsf(pz + 7.0f);  // window at z=-7
                        float sub_vol = (window_dist < 5.0f)
                            ? 0.08f * (1.0f - window_dist / 5.0f) : 0.0f;
                        SetEarthPresenceVolume(&audio, sub_vol * pulse);
                    }
                    // Gibbons head turn — one-time: he looks at window when you approach
                    {
                        static bool gibbons_looked = false;
                        if (pz < -3.0f && !gibbons_looked && gibbons.active) {
                            gibbons.yaw = atan2f(-8.0f - gibbons.pos.z,
                                                  0.0f - gibbons.pos.x);
                            gibbons_looked = true;
                        }
                    }
                }
                UpdateEVAudio(&audio, player.moving, player.sprinting, scene.surface, dt);
                update_npc(&gibbons, player.camera.position, &scene, dt);
                if (IsKeyPressed(KEY_E)) {
                    for (int i = 0; i < scene.object_count; i++) {
                        InteractObject *obj = &scene.objects[i];
                        if (!obj->active || obj->done) continue;
                        float dist = Vector3Distance(player.camera.position, obj->pos);
                        if (dist < obj->radius) {
                            Vector3 to = Vector3Normalize(Vector3Subtract(obj->pos, player.camera.position));
                            Vector3 look = Vector3Normalize(Vector3Subtract(player.camera.target, player.camera.position));
                            if (Vector3DotProduct(to, look) > 0.5f) {
                                obj->step++; obj->done = true;
                                PlayInteract(&audio, INTERACT_CLICK);
                                kick_camera(&player, -0.01f, 0.005f);
                                break;
                            }
                        }
                    }
                }
                // Glass elevator — Gibbons leads you here, step in to ascend
                // Elevator shaft is at (0, y, 0) — center of lobby
                {
                    float elev_dist = sqrtf(
                        player.camera.position.x * player.camera.position.x +
                        player.camera.position.z * player.camera.position.z);
                    if (elev_dist < 1.5f && !gibbons.active) {
                        // Gibbons finished and player is at elevator — ride up
                        show_text("Going up.");
                        elevator_to_corridor = true;
                        transition_to(STATE_ELEVATOR);
                    }
                }
                break;

            case STATE_SPACE_CORRIDOR:
                update_player(&player, &scene, dt);
                UpdateEVAudio(&audio, player.moving, player.sprinting, scene.surface, dt);
                update_npc(&gibbons, player.camera.position, &scene, dt);
                // Per-position speed modulation — slow near windows (Earth views)
                // Windows are on alternating segments; player slows when near the wall
                {
                    float px = fabsf(player.camera.position.x);
                    if (px > 1.5f) {
                        // Near a window wall — slow to 65%
                        float wt = fminf(1.0f, (px - 1.5f) / 1.0f);
                        float slow = 1.0f - wt * 0.35f;
                        player.vel.x *= slow;
                        player.vel.z *= slow;
                    }
                }
                // ============================================================
                // SILENCE ZONES — the Beginner's Guide's most powerful technique
                // Specific spots where ambient sound drops to near-zero, then returns.
                // The absence of the constant is more powerful than any new sound.
                // ============================================================
                {
                    float pz = player.camera.position.z;
                    // Two silence pockets — near the impossible door and the empty section
                    float silence = 1.0f;
                    // Zone 1: near z=12 (impossible door area)
                    float d1 = fabsf(pz - 12.0f);
                    if (d1 < 2.0f) {
                        float t = 1.0f - (d1 / 2.0f);
                        float s = 1.0f - t * 0.85f;  // drops to 15% volume
                        if (s < silence) silence = s;
                    }
                    // Zone 2: near z=6 (empty hull section)
                    float d2 = fabsf(pz - 6.0f);
                    if (d2 < 1.5f) {
                        float t = 1.0f - (d2 / 1.5f);
                        float s = 1.0f - t * 0.9f;   // drops to 10% — almost nothing
                        if (s < silence) silence = s;
                    }
                    SetMasterVolume(silence);

                    // Sprint 2A: Per-door sound sources — distance-based volume
                    // Door 0 (warm light): muffled piano
                    // Door 1 (blue light): TV/voices (running water behind bathroom)
                    // Door 2 (dark): silence — the absence IS the point
                    for (int di = 0; di < 2; di++) {
                        float ddx = player.camera.position.x - door_positions[di].x;
                        float ddz = player.camera.position.z - door_positions[di].z;
                        float ddist = sqrtf(ddx*ddx + ddz*ddz);
                        float dvol = 1.0f / (1.0f + ddist * ddist);
                        dvol *= silence * 0.03f;

                        // Spatial panning — left/right based on player facing
                        // Cross product of forward direction × door direction gives left/right
                        Vector3 fwd = Vector3Normalize(Vector3Subtract(player.camera.target, player.camera.position));
                        Vector3 to_door = {-ddx, 0, -ddz};
                        if (ddist > 0.01f) {
                            to_door.x /= ddist; to_door.z /= ddist;
                            // Cross product Y component: fwd.x*to.z - fwd.z*to.x
                            // Positive = door is to the right, negative = left
                            float pan_raw = fwd.x * to_door.z - fwd.z * to_door.x;
                            float pan = 0.5f + pan_raw * 0.4f;  // 0.1 (full left) to 0.9 (full right)
                            if (di == 0) SetSoundPan(audio.snd_muffled_piano, pan);
                            else {
                                SetSoundPan(audio.snd_running_water, pan);
                                SetSoundPan(audio.snd_tv_murmur, pan);
                            }
                        }

                        if (di == 0) {
                            SetSoundVolume(audio.snd_muffled_piano, dvol);
                            // Door 0 light dims as you approach — someone turned it off
                            // The light under the door fades when you're within 4m
                            // Commandment 6: inaccessible spaces communicate through absence
                            float light_fade = 1.0f;
                            if (ddist < 4.0f) {
                                light_fade = ddist / 4.0f;
                                light_fade *= light_fade;  // quadratic — snaps off close up
                            }
                            // Reading lamp breathes — someone's in there
                            float breath = 0.85f + 0.15f * sinf(state_time * 1.5f);
                            SetPointLightIdx(&lighting, 2,
                                door_positions[0].x, 0.05f, door_positions[0].z,
                                0.94f * light_fade * breath, 0.82f * light_fade * breath,
                                0.47f * light_fade * breath, 4.0f * light_fade);
                        } else {
                            SetSoundVolume(audio.snd_running_water, dvol * 0.8f);
                            SetSoundVolume(audio.snd_tv_murmur, dvol);
                            // TV flicker — blue light stutters behind door 1
                            float flk = 0.6f + 0.4f * sinf(state_time * 8.3f)
                                       * sinf(state_time * 13.7f);
                            SetPointLightIdx(&lighting, 3,
                                door_positions[1].x, 0.05f, door_positions[1].z,
                                0.31f * flk, 0.47f * flk, 0.78f * flk, 3.0f * flk);
                        }
                    }

                    // Door 2 (dark): the void seeps through
                    // Near the dark door: grain spikes, exposure dips, cold tint
                    // The absence communicates more than presence ever could
                    float d2x = player.camera.position.x - door_positions[2].x;
                    float d2z = player.camera.position.z - door_positions[2].z;
                    float dark_dist = sqrtf(d2x*d2x + d2z*d2z);
                    if (dark_dist < 4.0f) {
                        float dark_t = 1.0f - (dark_dist / 4.0f);
                        dark_t *= dark_t;  // quadratic — snaps near the door
                        SetPostFXGrain(&postfx, 0.4f + dark_t * 0.4f);
                        set_exposure(0.0f - dark_t * 0.15f);
                    }
                }
                if (scene.has_exit) {
                    float dist = Vector3Distance(player.camera.position, scene.exit_pos);
                    if (dist < 3.0f) {
                        SetMasterVolume(1.0f);  // restore before transition
                        transition_to(STATE_SPACE_SUITE);
                    }
                }
                // Gibbons finished his path — auto-transition after pause
                if (!gibbons.active && gibbons.current_waypoint >= gibbons.waypoint_count) {
                    done_pause += dt;
                    if (done_pause > 1.5f) {
                        SetMasterVolume(1.0f);
                        transition_to(STATE_SPACE_SUITE);
                    }
                }
                break;

            case STATE_SPACE_SUITE:
                update_player(&player, &scene, dt);
                // Per-position speed modulation — agency as a dial
                // Near window (left wall): slow, contemplative
                // Near bed (back wall): soft, surrendering
                {
                    float px = player.camera.position.x;
                    float pz = player.camera.position.z;
                    float speed_mult = 1.0f;
                    // Near window (x < -4): slow to 60%
                    if (px < -4.0f) {
                        float t = fminf(1.0f, (-4.0f - px) / 3.0f);
                        speed_mult = 1.0f - t * 0.4f;
                    }
                    // Near bed (z < -3): slow to 70%
                    if (pz < -3.0f) {
                        float t = fminf(1.0f, (-3.0f - pz) / 2.5f);
                        float bed_mult = 1.0f - t * 0.3f;
                        if (bed_mult < speed_mult) speed_mult = bed_mult;
                    }
                    if (speed_mult < 1.0f) {
                        player.vel.x *= speed_mult;
                        player.vel.z *= speed_mult;
                    }

                    // Spatial temperature contrast — "safety needs danger nearby to register"
                    // Near window/void: cold tint, grain increases (infinite void outside)
                    // Deep in room/bed: warm tint, grain decreases (insulated, safe)
                    float cold_t = 0;
                    if (px < -3.0f) cold_t = fminf(1.0f, (-3.0f - px) / 4.0f);
                    float warm_t = 0;
                    if (pz < -2.0f) warm_t = fminf(1.0f, (-2.0f - pz) / 4.0f);
                    // Blend: cold overrides warm (void is more powerful)
                    float temp = warm_t * 0.3f - cold_t * 0.2f;
                    float base_warmth = (float)tasks_done / SPACE_TASK_COUNT;
                    // Sprint 5A: Time-based warmth — suite warms over time, not just on tasks
                    float time_warmth = fminf(0.15f, state_time * 0.001f);
                    SetPostFXWarmth(&postfx, base_warmth + temp + time_warmth);
                    // Grain: more near void (raw exposure), less in bed (cocoon)
                    float spatial_grain = 0.35f + cold_t * 0.3f - warm_t * 0.1f;
                    SetPostFXGrain(&postfx, spatial_grain);

                    // Sprint 2B: Clock positional volume — louder near center, softer at edges
                    // The clock is the room's heartbeat. You hear it most when you're still.
                    if (audio.clock_playing) {
                        float center_dist = sqrtf(px * px + pz * pz);
                        float clock_vol = 0.035f / (1.0f + center_dist * 0.15f);
                        // Boost when looking up (near ceiling where clock would be)
                        if (player.camera.position.y > 1.8f) clock_vol *= 1.3f;
                        SetSoundVolume(audio.snd_clock, clock_vol);
                    }
                }
                UpdateEVAudio(&audio, player.moving, player.sprinting, scene.surface, dt);
                update_npc(&gibbons, player.camera.position, &scene, dt);
                // P3: Lamp ritual — filament warms over 1.5s
                if (interaction_phases[0] == 1) {
                    interaction_timers[0] -= dt;
                    float gt = fminf(1.0f, 1.0f - interaction_timers[0] / 1.5f);
                    SetPointLightIdx(&lighting, 0, -2.5f, 1.2f, -4.8f, gt, gt*0.82f, gt*0.45f, gt*8.0f);
                    if (interaction_timers[0] <= 0) interaction_phases[0] = 2;
                }
                // P3: Champagne pour ritual — liquid appears over 2s
                if (interaction_phases[1] == 1) {
                    interaction_timers[1] -= dt;
                    float pt = fminf(1.0f, 1.0f - interaction_timers[1] / 2.0f);
                    // Grow a gold cylinder in the glass (liquid level rising)
                    // We use a point light to simulate the warm golden glow of champagne
                    SetPointLightIdx(&lighting, 2, -3.1f, 0.42f, 3.5f,
                                     pt * 0.6f, pt * 0.5f, pt * 0.15f, pt * 3.0f);
                    if (interaction_timers[1] <= 0) {
                        interaction_phases[1] = 2;
                        // Pour complete — add liquid and zero-g bubbles
                        add_wall(&scene, -3.1f, 0.41f, 3.5f, 0.05f, 0.06f, 0.05f,
                                 (Color){240,210,100,200});
                        add_sphere(&scene, -2.8f, 0.7f, 3.3f, 0.1f,
                                   (Color){240,210,100,180});
                        add_sphere(&scene, -3.2f, 0.9f, 3.6f, 0.08f,
                                   (Color){240,210,100,160});
                        add_sphere(&scene, -3.0f, 1.2f, 3.4f, 0.06f,
                                   (Color){240,210,100,140});
                    }
                }
                // P3: Desk ritual — drawer slides open over 1.2s, reading light warms
                if (interaction_phases[2] == 1) {
                    interaction_timers[2] -= dt;
                    float dt2 = fminf(1.0f, 1.0f - interaction_timers[2] / 1.2f);
                    // Reading light slowly brightens — someone is about to study the sky
                    SetPointLightIdx(&lighting, 3, 5.5f, 1.4f, -2.0f,
                                     dt2 * 0.7f, dt2 * 0.6f, dt2 * 0.3f, dt2 * 5.0f);
                    if (interaction_timers[2] <= 0) interaction_phases[2] = 2;
                }
                // P3: Bed ritual — camera descends over 2s
                if (interaction_phases[3] == 1) {
                    interaction_timers[3] -= dt;
                    float bt = fminf(1.0f, 1.0f - interaction_timers[3] / 2.0f);
                    player.camera.position.y += ((1.6f - bt*0.8f) - player.camera.position.y) * dt * 2.0f;
                    if (interaction_timers[3] <= 0) interaction_phases[3] = 2;
                }
                if (IsKeyPressed(KEY_E)) {
                    for (int i = 0; i < scene.object_count; i++) {
                        InteractObject *obj = &scene.objects[i];
                        if (!obj->active || obj->done) continue;
                        float dist = Vector3Distance(player.camera.position, obj->pos);
                        if (dist < obj->radius) {
                            Vector3 to = Vector3Normalize(Vector3Subtract(obj->pos, player.camera.position));
                            Vector3 look = Vector3Normalize(Vector3Subtract(player.camera.target, player.camera.position));
                            if (Vector3DotProduct(to, look) > 0.5f) {
                                obj->step++;
                                PlayInteract(&audio, get_interact_sound(obj->name));
                                kick_camera(&player, -0.01f, 0.005f);

                                // Per-step visual feedback — world accumulates changes
                                if (strcmp(obj->name, "lamp") == 0 && obj->step == 1) {
                                    // P3: Start lamp glow ritual — filament visible
                                    add_wall(&scene, -2.5f, 1.0f, -4.65f, 0.15f, 0.25f, 0.15f,
                                             (Color){220,210,185,180});
                                    interaction_phases[0] = 1;
                                    interaction_timers[0] = 1.5f;
                                } else if (strcmp(obj->name, "desk") == 0 && obj->step == 1) {
                                    // Open a drawer — blue folder visible
                                    add_wall(&scene, 5.3f, 0.45f, -1.6f, 0.4f, 0.04f, 0.3f,
                                             (Color){55,85,175,255});
                                    // Start desk ritual — reading light warms over 1.2s
                                    interaction_phases[2] = 1;
                                    interaction_timers[2] = 1.2f;
                                } else if (strcmp(obj->name, "bed") == 0 && obj->step == 1) {
                                    // Smooth sheets — bright rectangle
                                    add_wall(&scene, 0, 0.54f, -4.3f, 2.8f, 0.02f, 1.4f,
                                             (Color){245,242,235,255});
                                } else if (strcmp(obj->name, "champagne") == 0 && obj->step == 1) {
                                    // Step 1: Catch the floating glass — it settles on the table
                                    add_cone(&scene, -3.1f, 0.39f, 3.5f, 0.06f, 0.08f,
                                             (Color){210,210,215,200});
                                    add_cylinder(&scene, -3.1f, 0.44f, 3.5f, 0.02f, 0.08f,
                                                 (Color){210,210,215,200});
                                }

                                // Step 2: champagne pour — start 2s ritual
                                if (strcmp(obj->name, "champagne") == 0 && obj->step == 2) {
                                    PlayInteract(&audio, INTERACT_GLASS_CLINK);
                                    interaction_phases[1] = 1;
                                    interaction_timers[1] = 2.0f;
                                }

                                if (obj->step >= obj->max_steps) {
                                    obj->done = true;
                                    obj->reward_timer = 1.5f;
                                    tasks_done++;
                                    PlayRewardSound(&audio);
                                    SetPostFXWarmth(&postfx, (float)tasks_done / SPACE_TASK_COUNT);
                                    scene.fog_density = 0.001f - ((float)tasks_done / SPACE_TASK_COUNT * 0.0005f);

                                    // Lighthouse — the one composed piece
                                    // Triggers on second task. Plays once. Never repeats.
                                    if (tasks_done == 2) PlaySuiteMusic(&audio);

                                    // Sprint 2B: Clock heartbeat deceleration
                                    // Each task: clock rate drops by ~0.2
                                    float new_rate = 1.0f - (float)tasks_done * 0.2f;
                                    if (new_rate < 0.1f) new_rate = 0.1f;
                                    SetClockRate(&audio, new_rate);

                                    // Visible consequences — completion rewards
                                    if (strcmp(obj->name, "lamp") == 0) {
                                        // Lamp is on — warm golden glow
                                        add_light_panel(&scene, -2.5f, 1.2f, -4.8f,
                                                        0.5f, 0.6f, 0.5f, (Color){255,220,120,200});
                                        // Lamp casts warm shadow on wall behind
                                        add_wall(&scene, -2.5f, 1.8f, -5.35f,
                                                 1.2f, 1.5f, 0.01f, (Color){60,45,20,40});
                                    } else if (strcmp(obj->name, "desk") == 0) {
                                        // Star chart spread on desk
                                        add_wall(&scene, 5.5f, 0.85f, -2, 1.8f, 0.01f, 0.8f,
                                                 (Color){20,25,45,255});
                                        // Star dots — constellations
                                        add_wall(&scene, 5.2f, 0.86f, -2.1f, 0.06f, 0.005f, 0.06f,
                                                 (Color){240,235,220,200});
                                        add_wall(&scene, 5.7f, 0.86f, -1.8f, 0.04f, 0.005f, 0.04f,
                                                 (Color){240,235,220,180});
                                        add_wall(&scene, 5.4f, 0.86f, -1.6f, 0.05f, 0.005f, 0.05f,
                                                 (Color){240,235,220,160});
                                        add_wall(&scene, 6.0f, 0.86f, -2.3f, 0.03f, 0.005f, 0.03f,
                                                 (Color){240,235,220,170});
                                        // Route line — pencil mark between stars
                                        add_wall(&scene, 5.45f, 0.862f, -1.95f, 0.8f, 0.003f, 0.02f,
                                                 (Color){180,60,50,100});
                                        // Annotation circle — red pencil around destination
                                        add_cylinder(&scene, 5.7f, 0.863f, -1.8f, 0.12f, 0.003f,
                                                     (Color){180,60,50,80});
                                    } else if (strcmp(obj->name, "bed") == 0) {
                                        // P3: Start bed descent ritual
                                        add_wall(&scene, 0, 0.68f, -5.0f, 0.18f, 0.04f, 0.12f,
                                                 (Color){90,50,25,255});
                                        interaction_phases[3] = 1;
                                        interaction_timers[3] = 2.0f;
                                    } else if (strcmp(obj->name, "champagne") == 0) {
                                        // Pour complete — golden liquid in glass + bubbles in zero-g
                                        add_wall(&scene, -3.1f, 0.41f, 3.5f, 0.05f, 0.06f, 0.05f,
                                                 (Color){240,210,100,200});
                                        // Bubbles caught in zero-g — golden spheres drifting up
                                        add_sphere(&scene, -2.8f, 0.7f, 3.3f, 0.1f,
                                                   (Color){240,210,100,180});
                                        add_sphere(&scene, -3.2f, 0.9f, 3.6f, 0.08f,
                                                   (Color){240,210,100,160});
                                        add_sphere(&scene, -3.0f, 1.2f, 3.4f, 0.06f,
                                                   (Color){240,210,100,140});
                                        // Escapee — one bubble drifts toward ceiling, catching Earth glow
                                        add_sphere(&scene, -2.9f, 2.0f, 3.5f, 0.04f,
                                                   (Color){180,200,240,100});
                                        // Second escapee — smaller, higher, almost gone
                                        add_sphere(&scene, -3.1f, 2.8f, 3.35f, 0.025f,
                                                   (Color){160,190,230,60});
                                    }

                                    if (tasks_done >= SPACE_TASK_COUNT) {
                                        kick_camera(&player, -0.05f, 0.02f);
                                        SetPostFXWarmth(&postfx, 1.0f);
                                    } else {
                                        kick_camera(&player, -0.02f, 0.008f);
                                    }
                                }
                                break;
                            }
                        }
                    }
                }
                // Sprint 4A: Gibbons sits after dialogue, stands on task completion
                if (!gibbons.active && tasks_done < SPACE_TASK_COUNT) {
                    // Gibbons finished dialogue — sits on sofa, waiting
                    gibbons.behavior = NPC_SITTING;
                    // Keep him visible but stationary
                    gibbons.active = true;
                    gibbons.waiting = true;
                }
                // All tasks done — Gibbons stands, delivers final line, then balcony
                if (tasks_done >= SPACE_TASK_COUNT) {
                    done_pause += dt;

                    // Gibbons stands and walks out with final line
                    if (done_pause < 0.1f && gibbons.behavior == NPC_SITTING) {
                        gibbons.behavior = NPC_WALKING;
                        gibbons.waiting = false;
                        // Add exit waypoint — walks to door
                        gibbons.waypoints[0] = (Vector3){0, 1.6f, 5};
                        gibbons.waypoint_count = 1;
                        gibbons.current_waypoint = 0;
                        gibbons.target_pos = gibbons.waypoints[0];
                        // Final line
                        static const char *exit_line[] = {"Time's up."};
                        npc_set_dialogue(&gibbons, exit_line, 1, 2.0f);
                    }

                    // Sprint 2B: After final task, stop the clock — silence = completion
                    if (done_pause > 0.5f) {
                        StopClockAmbient(&audio);
                    }
                    // Sprint 3B: Progressive agency removal over 4s before balcony cut
                    if (done_pause < 4.0f) {
                        float removal = done_pause / 4.0f;  // 0→1
                        player.control_mult = 1.0f - removal;
                    } else {
                        player.control_mult = 0.0f;
                    }
                    if (done_pause > 5.0f) {
                        player.control_mult = 1.0f;  // restore for balcony
                        balcony_flash_triggered = false;
                        balcony_flash_timer = 0;
                        hard_cut_to(STATE_BALCONY);
                    }
                }
                break;

        }

        // Gibbons dialogue → vignette text
        if (state == STATE_SPACE_LOBBY || state == STATE_SPACE_CORRIDOR
            || state == STATE_SPACE_SUITE) {
            const char *line = npc_current_dialogue(&gibbons);
            if (line && vig_text != line) {
                show_text(line);
            } else if (!line && vig_text && gibbons.active) {
                hide_text();
            }
        }

        }  // ── end game logic gate ──

        // ---- SHADOW PASS ----
        lighting.shadowPassRan = false;
        if (lighting.shadowReady && state != STATE_TITLE && state != STATE_BED && state != STATE_STARS) {
            draw_shadow_pass(&scene, &lighting,
                            &cube_model, &cyl_model, &sphere_model, &cone_model);
        }

        // ---- RENDER ----
        BeginTextureMode(render_target);
        ClearBackground(scene.fog_color.a > 0 ? scene.fog_color : (Color){8, 10, 22, 255});
        if (wireframe) rlEnableWireMode();

        // Bind shadow map ONLY if shadow pass actually wrote depth this frame
        if (lighting.shadowPassRan) BindShadowMap(&lighting);

        switch (state) {
            case STATE_TITLE:
                draw_title();
                break;

            case STATE_CAR:
            case STATE_DRIVING:
                // 3D taxi scene with night sky
                ClearBackground((Color){8, 12, 28, 255});
                draw_night_sky(state_time);
                draw_scene_3d(&player, &scene, &lighting, &cube_model, cube_model_loaded,
                              &cyl_model, cyl_model_loaded,
                              &sphere_model, sphere_model_loaded,
                              &cone_model, cone_model_loaded,
                              &skytower_model, skytower_loaded,
                              false, state_time);
                // Rain overlay on windshield — 2D after 3D
                for (int i = 0; i < MAX_RAIN; i++) {
                    rain[i].y += rain[i].speed * 1.5f * dt;
                    if (rain[i].y > RENDER_H) {
                        rain[i].y = -(rain[i].len * 0.5f);
                        rain[i].x = (float)GetRandomValue(0, RENDER_W);
                    }
                    DrawLine((int)rain[i].x, (int)rain[i].y,
                             (int)(rain[i].x - 0.5f), (int)(rain[i].y + rain[i].len),
                             (Color){180, 195, 210, 30});
                }
                break;

            case STATE_HOTEL_EXT:
            case STATE_LOBBY:
            case STATE_ELEVATOR:
            case STATE_HALLWAY:
            case STATE_ROOM:
            case STATE_BATHROOM:
            case STATE_BALCONY:
            case STATE_SPACE_LOBBY:
            case STATE_SPACE_CORRIDOR:
            case STATE_SPACE_SUITE:
            case STATE_HYPERSPACE:
            case STATE_PARIS_DREAM:
            case STATE_RETURN_TAXI:
                if (state == STATE_RETURN_TAXI) {
                    // Dawn sky behind the taxi
                    ClearBackground((Color){45, 38, 32, 255});
                    draw_dawn_sky(state_time);
                } else if (state == STATE_PARIS_DREAM) {
                    // Golden fog — no sky, just warm haze
                    ClearBackground((Color){35, 28, 15, 255});
                } else if (state == STATE_HOTEL_EXT) {
                    ClearBackground((Color){8, 12, 28, 255});
                    draw_night_sky(state_time);
                } else if (state == STATE_ELEVATOR) {
                    // Elevator transition: Auckland sky fades to void as you ascend
                    float ascent = fminf(1.0f, state_time / 4.0f);
                    unsigned char br = (unsigned char)(8 * (1.0f - ascent) + 4 * ascent);
                    unsigned char bg = (unsigned char)(12 * (1.0f - ascent) + 5 * ascent);
                    unsigned char bb = (unsigned char)(28 * (1.0f - ascent) + 12 * ascent);
                    ClearBackground((Color){br, bg, bb, 255});
                    if (ascent < 0.8f) draw_night_sky(state_time);
                } else if (state == STATE_HYPERSPACE) {
                    ClearBackground((Color){2, 2, 6, 255});  // deep void tunnel
                } else if (state == STATE_BALCONY || state == STATE_SPACE_LOBBY
                           || state == STATE_SPACE_CORRIDOR || state == STATE_SPACE_SUITE) {
                    ClearBackground((Color){4, 5, 12, 255});
                    // Void gradient — not flat black. Space has depth.
                    // Subtle blue-purple wash from bottom (Earth-lit) to black (deep space)
                    for (int gy = 0; gy < RENDER_H; gy++) {
                        float gt = (float)gy / RENDER_H;
                        // Bottom: faint Earth blue. Top: pure void.
                        unsigned char gr = (unsigned char)(4 + gt * 3);
                        unsigned char gg = (unsigned char)(5 + gt * 4);
                        unsigned char gb = (unsigned char)(12 + gt * 8);
                        if (gr > 4 || gg > 5 || gb > 12)
                            DrawRectangle(0, RENDER_H - gy, RENDER_W, 1, (Color){gr, gg, gb, 255});
                    }
                    // Deep space star field — 2D pixel stars
                    {
                        SetRandomSeed(42);
                        int star_count = (state == STATE_BALCONY) ? 140 : 70;
                        for (int si = 0; si < star_count; si++) {
                            int sx = GetRandomValue(0, RENDER_W);
                            int sy = GetRandomValue(0, RENDER_H);
                            float bri = 0.12f + (GetRandomValue(0, 88) / 100.0f);
                            float twk = 0.6f + 0.4f * sinf(state_time * (0.3f + GetRandomValue(0, 10) / 10.0f) + si * 2.7f);
                            unsigned char sr = (si % 5 == 0) ? 255 : (si % 7 == 0) ? 200 : 230;
                            unsigned char sg = 225;
                            unsigned char sb = (si % 5 == 0) ? 200 : (si % 7 == 0) ? 255 : 230;
                            DrawPixel(sx, sy, (Color){sr, sg, sb, (unsigned char)(255 * bri * twk)});
                            // Bright stars get a 3x3 glow halo
                            if (bri > 0.7f) {
                                unsigned char ha = (unsigned char)(30 * bri * twk);
                                DrawPixel(sx-1, sy, (Color){sr,sg,sb,ha});
                                DrawPixel(sx+1, sy, (Color){sr,sg,sb,ha});
                                DrawPixel(sx, sy-1, (Color){sr,sg,sb,ha});
                                DrawPixel(sx, sy+1, (Color){sr,sg,sb,ha});
                            }
                        }
                    }
                    // Procedural Earth — positioned per scene so it's visible through windows
                    if (sphere_model_loaded) {
                        Vector3 earth_pos = {0, -40, -60};  // default
                        if (state == STATE_SPACE_LOBBY) earth_pos = (Vector3){3, -20, -40};
                        else if (state == STATE_SPACE_CORRIDOR) earth_pos = (Vector3){-20, -30, -30};
                        else if (state == STATE_SPACE_SUITE) earth_pos = (Vector3){-35, -20, -10};
                        else if (state == STATE_BALCONY) earth_pos = (Vector3){0, -30, -50};
                        draw_earth(player.camera, state_time, &sphere_model, &lighting, earth_pos);
                    }
                } else {
                    ClearBackground(scene.fog_color);
                }
                {
                    bool indoor = !(state == STATE_HOTEL_EXT || state == STATE_BALCONY
                                    || state == STATE_SPACE_LOBBY || state == STATE_ELEVATOR
                                    || state == STATE_HYPERSPACE);
                    draw_scene_3d(&player, &scene, &lighting, &cube_model, cube_model_loaded,
                                  &cyl_model, cyl_model_loaded,
                                  &sphere_model, sphere_model_loaded,
                                  &cone_model, cone_model_loaded,
                                  &skytower_model, skytower_loaded,
                                  indoor, state_time);
                }
                // Gibbons NPC — draw in space scenes
                if ((state == STATE_SPACE_LOBBY || state == STATE_SPACE_CORRIDOR
                     || state == STATE_SPACE_SUITE) && gibbons.active) {
                    BeginMode3D(player.camera);
                    draw_npc(&gibbons, &cube_model, &cyl_model, &lighting);
                    EndMode3D();
                }
                // Sprint 5B: Zero-G sparkles — larger drift, occasional bright flashes
                if (state == STATE_SPACE_CORRIDOR || state == STATE_SPACE_SUITE
                    || state == STATE_SPACE_LOBBY) {
                    BeginMode3D(player.camera);
                    draw_zero_g_sparkles(player.camera, state_time);
                    EndMode3D();
                }
                // Earthshine — blue-white shimmer on the lower screen half
                // Light from the planet's atmosphere casting up into the deck
                if (state == STATE_BALCONY && eiffel_sparkle && sparkle_timer < 12.0f) {
                    SetRandomSeed((unsigned int)(sparkle_timer * 8));
                    int sc = 3 + (int)(sparkle_timer * 1.5f); if (sc > 15) sc = 15;
                    for (int i = 0; i < sc; i++) {
                        int sx = GetRandomValue(10, RENDER_W - 10);
                        int sy = GetRandomValue(RENDER_H/2, RENDER_H - 5);
                        float fl = 0.5f + 0.5f * sinf(GetTime()*6 + i*4.1f);
                        if (fl > 0.65f) {
                            unsigned char a = (unsigned char)(80*(fl-0.65f)/0.35f);
                            DrawPixel(sx, sy, (Color){100, 160, 220, a});
                        }
                    }
                }
                // Rug pull flash — Dadaist Rothko rectangles. Meaningless. Absurd.
                // Like the Gravity Bone death photos. The void between what
                // you saw and what it means. The game never explains.
                if (state == STATE_BALCONY && balcony_flash_triggered &&
                    balcony_flash_timer > 0 && balcony_flash_timer < 0.3f) {
                    // Warm background
                    DrawRectangle(0, 0, RENDER_W, RENDER_H, (Color){245, 235, 215, 255});
                    // Large red rectangle — center, Rothko
                    DrawRectangle(RENDER_W/2 - 120, RENDER_H/2 - 60, 240, 100, (Color){200, 45, 40, 255});
                    // Blue rectangle — overlapping slightly, offset
                    DrawRectangle(RENDER_W/2 - 90, RENDER_H/2 - 30, 180, 80, (Color){40, 60, 160, 220});
                }
                break;

            case STATE_BED: {
                // Phase 1 (0-3s): black screen, just breathing
                // Phase 2 (3-8s): ceiling materializes — dark warm brown
                // Phase 3 (8+): glow-in-the-dark stars emerge one by one
                ClearBackground((Color){0, 0, 0, 255});

                // Ceiling materializes between 3-8s
                if (state_time > 3.0f) {
                    float ceil_t = fminf(1.0f, (state_time - 3.0f) / 5.0f);
                    unsigned char cr = (unsigned char)(25 * ceil_t);
                    unsigned char cg = (unsigned char)(20 * ceil_t);
                    unsigned char cb = (unsigned char)(15 * ceil_t);
                    ClearBackground((Color){cr, cg, cb, 255});
                }

                // Stars emerge one by one after 8s
                // Sprint 1A: Color progression — first 5 warm gold, next 5 cool white, final 10 pale blue
                if (state_time > 8.0f) {
                    int max_stars = 20;
                    int sc = (int)fminf(max_stars, (state_time - 8.0f) * 2.5f);
                    SetRandomSeed(99);
                    for (int i = 0; i < sc; i++) {
                        int sx = GetRandomValue(RENDER_W/8, RENDER_W*7/8);
                        int sy = GetRandomValue(RENDER_H/8, RENDER_H*5/8);
                        float appear = fmaxf(0, state_time - 8.0f - i * 0.4f);
                        float bri = fminf(1.0f, appear / 1.5f);
                        float gl = 0.6f + 0.3f * sinf(GetTime()*(0.3f+i*0.07f) + i*2);
                        // Color progression: warm gold → cool white → pale blue
                        Color star_core, star_glow;
                        if (i < 5) {
                            // Warm gold
                            star_core = (Color){240, 200, 100, (unsigned char)(bri*gl*160)};
                            star_glow = (Color){220, 180, 80, (unsigned char)(bri*gl*35)};
                        } else if (i < 10) {
                            // Cool white
                            star_core = (Color){230, 235, 245, (unsigned char)(bri*gl*150)};
                            star_glow = (Color){210, 220, 240, (unsigned char)(bri*gl*30)};
                        } else {
                            // Pale blue — the void reaching in
                            star_core = (Color){180, 200, 255, (unsigned char)(bri*gl*140)};
                            star_glow = (Color){150, 180, 240, (unsigned char)(bri*gl*25)};
                        }
                        DrawCircle(sx, sy, 1.5f, star_core);
                        DrawCircle(sx, sy, 5, star_glow);
                    }
                }
                break;
            }

            case STATE_STARS: {
                // Background warms slowly — the void isn't cold, it's tender
                {
                    float bgw = fminf(1.0f, state_time / 12.0f);
                    ClearBackground((Color){
                        (unsigned char)(4 + bgw * 8),
                        (unsigned char)(5 + bgw * 4),
                        (unsigned char)(12 + bgw * 2), 255});
                }
                float zoom = fminf(1, state_time / 4.0f);

                // Sprint 1B: BED stars persist — same seed (99), continuous transition
                // They expand from ceiling positions into full-field
                SetRandomSeed(99);
                for (int i = 0; i < 20; i++) {
                    int bx = GetRandomValue(RENDER_W/8, RENDER_W*7/8);
                    int by = GetRandomValue(RENDER_H/8, RENDER_H*5/8);
                    // Expand outward over first 4s
                    float expand = fminf(1.0f, state_time / 4.0f);
                    int sx = (int)(bx + (bx - RENDER_W/2) * expand * 0.5f);
                    int sy = (int)(by + (by - RENDER_H/2) * expand * 0.5f);
                    float gl = 0.6f + 0.3f * sinf(GetTime()*(0.3f+i*0.07f) + i*2);
                    // Keep color progression from BED
                    unsigned char r, g, b;
                    if (i < 5) { r = 240; g = 200; b = 100; }
                    else if (i < 10) { r = 230; g = 235; b = 245; }
                    else { r = 180; g = 200; b = 255; }
                    DrawCircle(sx, sy, 1.5f, (Color){r, g, b, (unsigned char)(gl*120)});
                    DrawCircle(sx, sy, 4, (Color){r, g, b, (unsigned char)(gl*25)});
                }

                // Background star field expands
                SetRandomSeed(77);
                int count = 60 + (int)(zoom * 140);
                for (int i = 0; i < count; i++) {
                    int sx = GetRandomValue(0, RENDER_W), sy = GetRandomValue(0, RENDER_H);
                    float bri = (0.2f + (GetRandomValue(0,80)/100.0f)) * zoom;
                    float twk = 0.7f + 0.3f * sinf(GetTime()*(0.5f+GetRandomValue(0,30)/10.0f) + i);
                    DrawPixel(sx, sy, (Color){255,250,230,(unsigned char)(255*bri*twk)});
                }
                // Title
                if (state_time > 3) {
                    float a = fminf(1, (state_time - 3) / 3.0f);
                    draw_text_box("E N D E A R I N G   V O I D", RENDER_H/2-8, 14,
                                  (Color){240,232,210,(unsigned char)(230*a)});
                }
                // Credits — fade in like end of a film
                if (state_time > 5) {
                    float ca = fminf(1, (state_time - 5) / 2.0f);
                    draw_text_box("A game by Maxwell Young", RENDER_H - 40, 10,
                                  (Color){180,175,165,(unsigned char)(180*ca)});
                }
                if (state_time > 7) {
                    float na = fminf(1, (state_time - 7) / 2.0f);
                    draw_text_box("Maxwell Young", RENDER_H - 28, 10,
                                  (Color){160,155,148,(unsigned char)(160*na)});
                }
                break;
            }
        }

        if (wireframe) rlDisableWireMode();

        // HUD
        if (state == STATE_ROOM || state == STATE_BATHROOM ||
            state == STATE_LOBBY || state == STATE_ELEVATOR || state == STATE_BALCONY ||
            state == STATE_SPACE_LOBBY || state == STATE_SPACE_CORRIDOR ||
            state == STATE_SPACE_SUITE || state == STATE_HALLWAY) {
            draw_hud(&player, &scene);
            // No progress dots. No step counter. The world changes ARE the feedback.
            // "Removing the map marker was the best decision I ever made." — Sam C
        }

        // Crosshair — drawn by draw_hud for interactive scenes;
        // simple dot for non-interactive 3D scenes
        if (state == STATE_HOTEL_EXT || state == STATE_CAR || state == STATE_DRIVING) {
            DrawPixel(RENDER_W/2, RENDER_H/2, (Color){220, 215, 205, 60});
        }

        // Vignette text overlay
        draw_vignette_text();

        // Pause menu overlay — drawn into 480x300, gets film grain treatment
        if (menu_mode != MENU_NONE) draw_pause_menu();

        // Fade
        if (fade_alpha > 0.01f)
            DrawRectangle(0, 0, RENDER_W, RENDER_H, (Color){0,0,0,(unsigned char)(255*fade_alpha)});

        if (lighting.shadowPassRan) UnbindShadowMap();
        EndTextureMode();

        // DIAGNOSTIC — log per-state on frame 5 of each new state
        {
            static GameState dbg_prev = -1;
            static int dbg_sf = 0;
            if ((int)state != (int)dbg_prev) { dbg_prev = state; dbg_sf = 0; }
            dbg_sf++;
            if (dbg_sf == 5) {
                Image di = LoadImageFromTexture(render_target.texture);
                Color *px = LoadImageColors(di);
                long rs = 0, gs = 0, bs = 0;
                for (int i = 0; i < RENDER_W*RENDER_H; i++) {
                    rs += px[i].r; gs += px[i].g; bs += px[i].b;
                }
                int n = RENDER_W*RENDER_H;
                printf("[DBG] state=%d f5 pre-pfx R=%ld G=%ld B=%ld fade=%.2f\n",
                       state, rs/n, gs/n, bs/n, fade_alpha);
                UnloadImageColors(px);
                UnloadImage(di);
            }
        }

        // ── Scene breathing — exposure subtly pulses in contemplative spaces ──
        // The scene is alive. Not static. Like a held breath.
        if (state == STATE_SPACE_SUITE || state == STATE_BALCONY ||
            state == STATE_SPACE_LOBBY || state == STATE_ROOM) {
            float breath = sinf(state_time * 0.3f) * 0.02f;  // ±0.02 EV, very slow
            // Only when player is still or slow — movement kills the effect
            float still = 1.0f - fminf(1.0f, player.speed_current / 2.0f);
            SetPostFXExposure(&postfx, scene_exposure +
                visual_styles[current_style].exposure_bias + breath * still);
        }

        // Post-FX — feed player speed to shader for speed lines
        SetPostFXSpeed(&postfx, player_speed_normalized(&player));
        BeginTextureMode(postfx_target);
        ClearBackground(BLACK);
        draw_postfx(&postfx, render_target);
        EndTextureMode();

        // Screen
        BeginDrawing();
        ClearBackground(BLACK);
        float scale = fminf((float)GetScreenWidth()/RENDER_W, (float)GetScreenHeight()/RENDER_H);
        float ox = (GetScreenWidth() - RENDER_W*scale) / 2;
        float oy = (GetScreenHeight() - RENDER_H*scale) / 2;
        RenderTexture2D display = postfx.ready ? postfx_target : render_target;
        DrawTexturePro(display.texture,
            (Rectangle){0, 0, RENDER_W, -(float)RENDER_H},
            (Rectangle){ox, oy, RENDER_W*scale, RENDER_H*scale},
            (Vector2){0, 0}, 0, WHITE);

#ifndef PLAYTEST
        if (show_debug) {
            const char *state_names[] = {
                "TITLE", "CAR", "DRIVING", "EXTERIOR", "LOBBY",
                "ELEVATOR", "HALLWAY", "ROOM", "BATHROOM", "BALCONY",
                "BED", "STARS", "HYPERSPACE", "SP_LOBBY", "SP_CORRIDOR", "SP_SUITE",
                "PARIS_DREAM", "RETURN_TAXI"
            };
            const char *sn = (state >= 0 && state < 18) ? state_names[state] : "???";
            DrawText(TextFormat("FPS: %d", GetFPS()), 10, 10, 20, GREEN);
            DrawText(TextFormat("Walls: %d  %s", scene.wall_count, sn), 10, 35, 20, GREEN);
            DrawText(TextFormat("Pos: %.1f %.1f %.1f",
                player.camera.position.x, player.camera.position.y, player.camera.position.z),
                10, 60, 20, GREEN);

            // ── Physics debug ──
            float hspd = player.speed_current;
            float spd_norm = player_speed_normalized(&player);
            // Speedometer bar
            int bar_w = 120, bar_h = 8;
            int bar_x = 10, bar_y = 88;
            DrawRectangle(bar_x, bar_y, bar_w, bar_h, (Color){40,40,40,180});
            int fill = (int)(spd_norm * bar_w);
            Color bar_col = spd_norm > 0.7f ? (Color){255,80,80,220} :
                           spd_norm > 0.3f ? (Color){255,200,60,220} : (Color){80,220,80,220};
            DrawRectangle(bar_x, bar_y, fill, bar_h, bar_col);
            DrawText(TextFormat("%.1f u/s", hspd), bar_x + bar_w + 5, bar_y - 2, 20, GREEN);

            // Movement state flags
            int fy = 102;
            const char *mstate = player.dashing ? "DASH" :
                                player.wall_running ? "WALLRUN" :
                                player.mantling ? "MANTLE" :
                                player.sliding ? "SLIDE" :
                                !player.grounded ? "AIR" :
                                player.sprinting ? "SPRINT" :
                                player.moving ? "WALK" : "IDLE";
            Color mc = player.dashing ? (Color){255,100,255,255} :
                      player.wall_running ? (Color){100,200,255,255} :
                      player.sliding ? (Color){255,200,60,255} :
                      !player.grounded ? (Color){200,200,255,255} : GREEN;
            DrawText(mstate, 10, fy, 20, mc);

            // Stamina / cooldowns
            DrawText(TextFormat("stam:%.0f%%", player.sprint_stamina * 100), 100, fy, 20,
                player.sprint_stamina < 0.2f ? (Color){255,80,80,200} : (Color){100,200,100,140});
            if (player.dash_cooldown_timer > 0)
                DrawText(TextFormat("dash:%.1f", player.dash_cooldown_timer), 210, fy, 20, (Color){200,150,255,180});

            int tm = (int)total_time / 60, ts = (int)total_time % 60;
            DrawText(TextFormat("Runtime: %d:%02d  Scene: %.1fs", tm, ts, state_time),
                10, fy + 20, 20, (Color){255,200,60,220});
            DrawText(TextFormat("Style: %s (Shift+1-9)", visual_styles[current_style].name),
                10, fy + 40, 20, (Color){100,200,100,180});
            DrawText("1-9: rooms  F4: noclip  F5: nudge  Q: dash", 10, fy + 60, 20, (Color){100,200,100,140});
            if (player.noclip) DrawText("NOCLIP", 10, fy + 60, 20, YELLOW);
            if (wireframe) DrawText("WIREFRAME", 10, fy + 80, 20, YELLOW);
        }

        // Nudge mode overlay — drawn on top of everything
        if (nudge_mode) {
            int ny = GetScreenHeight() - 100;
            DrawText("NUDGE MODE", 10, ny, 20, (Color){255,200,60,255});
            if (nudge_selected >= 0 && nudge_selected < scene.wall_count) {
                Wall *w = &scene.walls[nudge_selected];
                DrawText(TextFormat("wall[%d] pos=(%.2f, %.2f, %.2f) size=(%.2f, %.2f, %.2f)",
                    nudge_selected, w->pos.x, w->pos.y, w->pos.z,
                    w->size.x, w->size.y, w->size.z),
                    10, ny + 22, 16, (Color){255,255,200,220});
                DrawText("Arrows: move X/Z  Ctrl+Arrows: move Y  [/]: resize  Shift: fine (0.01)",
                    10, ny + 42, 16, (Color){200,200,180,180});
                DrawText("Click: re-select  F5: exit nudge",
                    10, ny + 60, 16, (Color){200,200,180,180});
            } else {
                DrawText("No wall selected — look at a wall and click", 10, ny + 22, 16, (Color){255,100,100,200});
            }
        }
#endif // PLAYTEST
        EndDrawing();
    }

    if (cube_model_loaded) UnloadModel(cube_model);
    if (cyl_model_loaded) UnloadModel(cyl_model);
    if (sphere_model_loaded) UnloadModel(sphere_model);
    if (cone_model_loaded) UnloadModel(cone_model);
    if (skytower_loaded) UnloadModel(skytower_model);
    DestroyShadowMap(&lighting);
    UnloadEVLighting(&lighting);
    UnloadEVPostFX(&postfx);
    UnloadFileMusic(&audio);
    UnloadEVAudio(&audio);
    UnloadRenderTexture(render_target);
    UnloadRenderTexture(postfx_target);
    CloseWindow();
    return 0;
#endif  // QA_MODE
}
