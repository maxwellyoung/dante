// ENDEARING VOID — Custom engine on Raylib
// Maxwell Young
// A story about arriving somewhere with someone who isn't here.

#include "raylib.h"
#include "rlgl.h"
#include "raymath.h"
#include "ev_types.h"
#include "config.h"
#include "lighting.h"
#include "scene.h"
#include "player.h"
#include "audio.h"
#include "render.h"
#include "npc.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

float ev_mouse_sens = MOUSE_SENS_DEFAULT;

static GameState state = STATE_SPLASH;
static GameState previous_state = STATE_SPLASH;  // where we came from — disambiguates elevator, spawn positions
static float state_time = 0;
static float total_time = 0;  // cumulative playthrough timer (never resets)
static float fade_alpha = 1.0f;
static float fade_target = 0.0f;
static GameState next_state = STATE_SPLASH;
static bool transitioning = false;
static float fade_speed = FADE_SPEED_DEFAULT;

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

// ============================================================
// SCENE STATE — per-scene variables, grouped by system
// ============================================================

// ── Task progression ──
static int tasks_done = 0;
static float done_pause = 0;
static float paris_dream_timer = -1;   // -1 = not triggered, >0 = counting down
static bool returning_from_dream = false;
static bool returning_to_room = false;
static const char *completed_objects[MAX_OBJECTS] = {0};
static int completed_count = 0;

// ── Suite wrongness (Barton Fink layer) ──
static bool wrongness_photo = false;
static bool wrongness_door = false;
static bool wrongness_earth = false;
static int adjoining_door_wall_idx = -1;
static bool adjoining_door_opened = false;
static bool phone_triggered = false;
static int phone_wall_idx = -1;

// ── Suite photograph ──
static bool photograph_flipped = false;
static float photo_text_timer = 0;

// ── Elevator + transitions ──
static bool elevator_ding_played = false;
static bool elevator_to_corridor = false;
static bool gibbons_glanced = false;

// ── Balcony ──
static bool balcony_flash_triggered = false;
static float balcony_flash_timer = 0;
static bool eiffel_sparkle = false;
static float sparkle_timer = 0;

// ── Cigarette camera animation ──
static bool cigarette_anim = false;
static float cigarette_anim_timer = 0;
static Vector3 cigarette_cam_origin = {0};
static Vector3 cigarette_cam_target = {0};

// ── NPC ──
static NPC gibbons = {0};

// ── Corridor spatial audio ──
static Vector3 door_positions[3] = {{0}};
static float door_listen_timer = 0;
static bool door_1_silenced = false;

// ── Per-scene timers ──
static float bed_clock_rate = 1.0f;
static float agency_removal_timer = 0;
static int lobby_visit_count = 0;

// ── Cinematic FX (Godard / Blendo film grammar) ──
static CinematicFX cinema = {0};
static RenderTexture2D split_target;  // second viewport for split screen
static bool split_target_loaded = false;

// ── Rapid-cut montage (Thirty Flights ending) ──
static Montage montage = {0};

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
static Raindrop rain[MAX_RAIN];

static InteractSoundType get_interact_sound(const char *name) {
    if (strcmp(name, "lamp") == 0) return INTERACT_CLICK;
    if (strcmp(name, "drawers") == 0) return INTERACT_FABRIC;
    if (strcmp(name, "candles") == 0) return INTERACT_FLAME;
    if (strcmp(name, "ashtray") == 0) return INTERACT_CLICK;
    if (strcmp(name, "bed") == 0) return INTERACT_FABRIC;
    if (strcmp(name, "champagne") == 0) return INTERACT_CORK_POP;
    if (strcmp(name, "desk") == 0) return INTERACT_FABRIC;
    if (strcmp(name, "photograph") == 0) return INTERACT_FABRIC;
    if (strcmp(name, "bell") == 0) return INTERACT_CLICK;
    if (strcmp(name, "wineglass") == 0) return INTERACT_GLASS_CLINK;
    return INTERACT_CLICK;
}

static float transition_hold = TRANSITION_HOLD_DEFAULT;
static float hold_timer = 0;

// ── Extracted helpers (eliminate copy-paste) ──

// Reset PostFX to clean baseline — used by splash and title screens
static void reset_postfx_clean(EVPostFX *pfx) {
    SetPostFXCA(pfx, 0.0f);
    SetPostFXGrain(pfx, 0.0f);
    SetPostFXWarmth(pfx, 0.0f);
    SetPostFXSaturation(pfx, 1.0f);
    SetPostFXContrast(pfx, 0.0f);
    SetPostFXVignette(pfx, 0.0f);
}

// Stop all through-wall audio — terrestrial scenes
static void stop_through_wall_audio(EVAudio *a) {
    StopMuffledPiano(a);
    StopDistantVoices(a);
    StopFootstepsAbove(a);
}

// ── Interaction ritual timers (P3 — multi-second rituals) ──
static float interaction_timers[5] = {0};  // lamp, champagne, desk, bed, bath
static int interaction_phases[5] = {0};    // current animation phase

// ── Ambient fade (P4) ──
static float ambient_fade = 1.0f;

// ── Lobby ambient life — the hotel has other guests ──
static float lobby_ding_timer = 0;       // countdown to next distant ding
static float lobby_ding_interval = 8.0f; // 8-15s between dings

// ── Interaction micro-freeze + camera lean ──
static float interact_freeze = 0;   // >0 = game paused for attention beat
static float interact_lean = 0;     // FOV narrowing on interaction
static float interact_lean_vel = 0; // spring velocity

// ============================================================
// PAUSE MENU & SETTINGS
// ============================================================
typedef enum { MENU_NONE, MENU_PAUSE, MENU_SETTINGS } MenuMode;
static MenuMode menu_mode = MENU_NONE;
static int menu_cursor = 0;
static float menu_item_x[MENU_MAX_ITEMS];
static float menu_item_vx[MENU_MAX_ITEMS];
static float menu_overlay_a = 0.0f;
static float menu_overlay_va = 0.0f;
static float setting_master_vol = 1.0f;
static bool  setting_fullscreen = false;
static const char *pause_labels[] = {"RESUME", "SETTINGS", "QUIT"};
static const char *settings_labels[] = {"SENSITIVITY", "VOLUME", "STYLE", "FULLSCREEN", "BACK"};
static int menu_item_count(void) { return (menu_mode == MENU_SETTINGS) ? SETTINGS_ITEM_COUNT : PAUSE_ITEM_COUNT; }
static void menu_init_springs(void) { for (int i = 0; i < MENU_MAX_ITEMS; i++) { menu_item_x[i] = (float)(RENDER_W + i * 24); menu_item_vx[i] = 0; } }
static void menu_open(MenuMode mode) { menu_mode = mode; menu_cursor = 0; menu_init_springs(); menu_overlay_a = 0; menu_overlay_va = 0; EnableCursor(); }
static void menu_close(void) { menu_mode = MENU_NONE; menu_cursor = 0; menu_overlay_a = 0; menu_overlay_va = 0; if (state != STATE_SPLASH && state != STATE_TITLE) DisableCursor(); }

// Forward declaration
static void load_state(GameState s);

// Exposure helper — stores scene value and adds style bias
static void set_exposure(float exp) {
    scene_exposure = exp;
    SetPostFXExposure(&postfx, exp + visual_styles[current_style].exposure_bias);
}

static void update_menu_springs(float dt) {
    float sk = SPRING_K, sd = SPRING_D, sm = SPRING_M;
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
        if (IsKeyPressed(KEY_ESCAPE) && state != STATE_SPLASH && state != STATE_TITLE && state != STATE_STARS) {
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
                case 0: snprintf(val, sizeof(val), "< %.1f >", (double)(ev_mouse_sens * 1000.0f)); break;
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
    fade_speed = FADE_SPEED_DEFAULT;
    transition_hold = TRANSITION_HOLD_DEFAULT;
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
    transition_hold = TRANSITION_HOLD_DEFAULT;
    cut_flash_timer = HARD_CUT_FLASH_DURATION;
    SetPostFXFlash(&postfx, 1.0f, HARD_CUT_FLASH_R, HARD_CUT_FLASH_G, HARD_CUT_FLASH_B);
    SetMasterVolume(1.0f);  // restore audio (hyperspace sets it to 0)
}

// ── Montage system — rapid-cut sequence engine ──
static void montage_start(Montage *m) {
    m->current_beat = 0;
    m->beat_timer = m->beats[0].duration;
    m->active = true;
    // Load first beat
    load_state(m->beats[0].scene);
    player.camera.position = m->beats[0].cam_pos;
    player.camera.target = m->beats[0].cam_target;
    player.control_mult = 0.0f;  // no player control during montage
    set_exposure(m->beats[0].exposure);
    SetPostFXWarmth(&postfx, m->beats[0].warmth);
    SetPostFXSaturation(&postfx, m->beats[0].saturation);
    SetPostFXGrain(&postfx, m->beats[0].grain);
    SetPostFXContrast(&postfx, m->beats[0].contrast);
    if (m->beats[0].text) show_text(m->beats[0].text);
    fade_alpha = 0; fade_target = 0; transitioning = false;
}

static void montage_advance(Montage *m) {
    m->current_beat++;
    if (m->current_beat >= m->beat_count) {
        m->active = false;
        return;
    }
    MontageBeat *b = &m->beats[m->current_beat];
    // Flash between cuts
    if (m->flash_between) {
        cut_flash_timer = 0.08f;
        SetPostFXFlash(&postfx, 1.0f, 1.0f, 0.95f, 0.85f);
        PlayHardCutPunch(&audio);
    }
    load_state(b->scene);
    // The cleaned room — the hotel moved on
    if (b->scene == STATE_SPACE_SUITE && b->text && strcmp(b->text, "__cleaned__") == 0) {
        build_space_suite_cleaned(&scene);
        b->text = NULL;  // don't display the sentinel
    }
    // In the montage, the photograph is always face-up
    if (b->scene == STATE_SPACE_SUITE && m->active) {
        for (int j = 0; j < scene.wall_count; j++) {
            if (fabsf(scene.walls[j].pos.x + 2.5f) < 0.2f &&
                fabsf(scene.walls[j].pos.z + 3.5f) < 0.2f &&
                scene.walls[j].pos.y > 0.6f && scene.walls[j].pos.y < 0.7f &&
                scene.walls[j].size.y < 0.05f) {
                scene.walls[j].color = (Color){230,218,195,255};
            }
        }
        // Red accent — her dress, the café, something warm
        add_wall(&scene, -2.48f, 0.625f, -3.48f, 0.04f, 0.003f, 0.03f, (Color){190,50,45,200});
        set_last_decal(&scene);
    }
    player.camera.position = b->cam_pos;
    player.camera.target = b->cam_target;
    player.control_mult = 0.0f;
    set_exposure(b->exposure);
    SetPostFXWarmth(&postfx, b->warmth);
    SetPostFXSaturation(&postfx, b->saturation);
    SetPostFXGrain(&postfx, b->grain);
    SetPostFXContrast(&postfx, b->contrast);
    if (b->text) show_text(b->text);
    else hide_text();
    m->beat_timer = b->duration;
    fade_alpha = 0; fade_target = 0; transitioning = false;
    // ── Gibbons reappearances — impossible locations ──
    // "Brief eye contact, then cut. He knew the whole time."
    if (b->scene == STATE_ELEVATOR) {
        // Gibbons alone in the elevator. Facing camera. Briefcase in hand.
        gibbons.active = true;
        gibbons.pos = (Vector3){0, 1.6f, -0.5f};
        gibbons.yaw = PI;  // facing the camera
        gibbons.behavior = NPC_WAITING;
        gibbons.waiting = true;
        gibbons.waypoint_count = 0;
    } else if (b->scene == STATE_BALCONY) {
        // Gibbons on the balcony — brief eye contact across the void
        gibbons.active = true;
        gibbons.pos = (Vector3){-1.5f, 1.6f, -4};
        gibbons.yaw = 0;  // facing player
        gibbons.behavior = NPC_WAITING;
        gibbons.waiting = true;
        gibbons.waypoint_count = 0;
    } else if (b->scene == STATE_RETURN_TAXI && m->current_beat == 9) {
        // Beat 10: Gibbons in the backseat. He was always leaving too.
        gibbons.active = true;
        gibbons.pos = (Vector3){0.4f, 1.2f, 1.5f};  // passenger seat, behind driver
        gibbons.yaw = 0;  // facing forward — not looking at you
        gibbons.behavior = NPC_GAZING;
        gibbons.waiting = true;
        gibbons.waypoint_count = 0;
    } else {
        gibbons.active = false;
    }
    // ── Montage audio — sparse, intentional ──
    // The void beat gets the held chord. Everything else: silence + hard cut transients.
    if (b->scene == STATE_STARS) {
        PlayHeldChord(&audio);  // stacked fifths — the final elegy
    }
    // The music returns — three notes, then silence. The ghost of the suite.
    // Plays during the final dawn taxi so it trails into the void.
    if (m->current_beat == m->beat_count - 2) {
        PlayMusicFragment(&audio);
    }
}

static void montage_update(Montage *m, float dt) {
    if (!m->active) return;
    m->beat_timer -= dt;
    if (m->beat_timer <= 0) {
        montage_advance(m);
    }
}

// ── The Ending — Thirty Flights of Loving ──
// Rapid cuts. Characters reappear with context. Time collapses.
// Each cut 2-5 seconds. Some cuts are single frames. Gibbons reappears.
static void compose_ending_montage(Montage *m) {
    memset(m, 0, sizeof(Montage));
    m->flash_between = true;
    int i = 0;

    // Beat 1: The taxi — where it started. Dawn light. The beginning again.
    m->beats[i++] = (MontageBeat){
        .scene = STATE_CAR,
        .duration = 3.0f,
        .cam_pos = {0, 1.2f, 0},
        .cam_target = {-0.45f, 0.6f, 0.1f},  // the second ticket on the seat
        .exposure = 0.1f, .warmth = 0.4f, .saturation = 0.9f,
        .grain = 0.4f, .contrast = 1.2f,
        .text = NULL,
    };

    // Beat 2: The lobby — empty. The observation window still glows.
    m->beats[i++] = (MontageBeat){
        .scene = STATE_SPACE_LOBBY,
        .duration = 1.5f,
        .cam_pos = {0, 1.6f, 8},
        .cam_target = {0, 1.6f, -5},
        .exposure = 0.0f, .warmth = 0.2f, .saturation = 0.8f,
        .grain = 0.5f, .contrast = 1.3f,
        .text = NULL,
    };

    // Beat 3: The hotel — the two chairs facing each other. Empty.
    m->beats[i++] = (MontageBeat){
        .scene = STATE_SPACE_HOTEL,
        .duration = 2.0f,
        .cam_pos = {-4, 1.6f, 4},
        .cam_target = {-6, 0.5f, 3},   // the two chairs facing each other. Empty.
        .exposure = -0.08f, .warmth = 0.3f, .saturation = 0.85f,
        .grain = 0.4f, .contrast = 1.2f,
        .text = NULL,
    };

    // Beat 4: Single-frame flash — corridor door. Someone's light is still on.
    m->beats[i++] = (MontageBeat){
        .scene = STATE_SPACE_CORRIDOR,
        .duration = 0.08f,  // ~5 frames at 60fps
        .cam_pos = {0, 1.6f, 6},
        .cam_target = {-2, 1.2f, 6},
        .exposure = 0.1f, .warmth = 0.0f, .saturation = 0.5f,
        .grain = 0.7f, .contrast = 1.5f,
        .text = NULL,
    };

    // Beat 5: The suite — "Three hours." The empty champagne glass. Untouched.
    m->beats[i++] = (MontageBeat){
        .scene = STATE_SPACE_SUITE,
        .duration = 2.5f,
        .cam_pos = {-3.8f, 1.0f, 3.2f},
        .cam_target = {-3.5f, 0.44f, 3.5f},   // the empty glass
        .exposure = -0.1f, .warmth = 0.6f, .saturation = 0.9f,
        .grain = 0.4f, .contrast = 1.1f,
        .text = NULL,   // the glasses speak for themselves
    };

    // Beat 5: The two robes — still hanging. Different sizes. She was expected.
    m->beats[i++] = (MontageBeat){
        .scene = STATE_SPACE_SUITE,
        .duration = 1.5f,
        .cam_pos = {6.5f, 1.6f, 2.3f},
        .cam_target = {7.0f, 1.7f, 2.3f},   // looking at the two robes on the wall
        .exposure = -0.05f, .warmth = 0.3f, .saturation = 0.8f,
        .grain = 0.5f, .contrast = 1.2f,
        .text = NULL,
    };

    // Beat 6: Paris dream fragment — B&W flash. Her coat, draped on the chair.
    m->beats[i++] = (MontageBeat){
        .scene = STATE_PARIS_DREAM,
        .duration = 0.15f,  // ~9 frames — subliminal
        .cam_pos = {-1.2f, 1.4f, 2.5f},
        .cam_target = {-1.5f, 0.8f, 2.0f},  // her coat, draped
        .exposure = 0.1f, .warmth = 0.0f, .saturation = 0.0f,
        .grain = 0.9f, .contrast = 2.0f,
        .text = NULL,
    };

    // Beat 6: Balcony — Earth, one last time. The void held you.
    m->beats[i++] = (MontageBeat){
        .scene = STATE_BALCONY,
        .duration = 3.0f,
        .cam_pos = {0, 1.6f, -1.3f},
        .cam_target = {0, 0.8f, -10},   // looking down into void, Earth below
        .exposure = 0.0f, .warmth = 0.3f, .saturation = 0.85f,
        .grain = 0.3f, .contrast = 1.0f,
        .text = NULL,
    };

    // Beat 7: KEY MOMENT — Gibbons in the elevator. Alone. Already leaving.
    // "Brief eye contact, then cut. He knew the whole time."
    // Camera: looking at where Gibbons stands. He faces you.
    m->beats[i++] = (MontageBeat){
        .scene = STATE_ELEVATOR,
        .duration = 2.0f,
        .cam_pos = {0, 1.6f, 1.5f},
        .cam_target = {0, 1.55f, -0.5f},  // looking at Gibbons
        .exposure = 0.05f, .warmth = 0.5f, .saturation = 0.7f,
        .grain = 0.5f, .contrast = 1.4f,
        .text = NULL,
    };

    // Beat 8: Single-frame — hyperspace tunnel. The threshold again.
    m->beats[i++] = (MontageBeat){
        .scene = STATE_HYPERSPACE,
        .duration = 0.05f,  // 3 frames
        .cam_pos = {0, 0, 0},
        .cam_target = {0, 0, -10},
        .exposure = 0.2f, .warmth = 0.0f, .saturation = 1.5f,
        .grain = 0.3f, .contrast = 1.0f,
        .text = NULL,
    };

    // Beat 9: KEY MOMENT — Gibbons in the taxi backseat. Impossible.
    // He's already on the way down. He was always leaving too.
    m->beats[i++] = (MontageBeat){
        .scene = STATE_RETURN_TAXI,
        .duration = 3.5f,
        .cam_pos = {0, 1.2f, 0},
        .cam_target = {0.3f, 1.1f, 1.5f},  // looking back at passenger seat
        .exposure = 0.05f, .warmth = 0.4f, .saturation = 1.0f,
        .grain = 0.3f, .contrast = 1.0f,
        .text = NULL,
    };

    // Beat 10: The suite — looking at the two robes on the bathroom door.
    m->beats[i++] = (MontageBeat){
        .scene = STATE_SPACE_SUITE,
        .duration = 1.5f,
        .cam_pos = {5, 1.6f, 1},
        .cam_target = {6.8f, 1.7f, 2.0f},   // two robes on the door
        .exposure = -0.15f, .warmth = 0.0f, .saturation = 0.6f,
        .grain = 0.6f, .contrast = 1.2f,
        .text = NULL,
    };

    // Beat 11: The photograph — face up now. She's laughing.
    // The game's only clear image of happiness. Hold.
    m->beats[i++] = (MontageBeat){
        .scene = STATE_SPACE_SUITE,
        .duration = 4.0f,
        .cam_pos = {-2.5f, 0.9f, -3.2f},
        .cam_target = {-2.5f, 0.62f, -3.5f},  // looking down at nightstand
        .exposure = 0.05f, .warmth = 0.4f, .saturation = 1.0f,
        .grain = 0.2f, .contrast = 1.0f,
        .text = NULL,  // silence. the image speaks.
    };

    // Beat 12: The cleaned room — one pillow, one robe, one glass.
    // The hotel moved on. Housekeeping erased her. The most violent cut.
    m->beats[i++] = (MontageBeat){
        .scene = STATE_SPACE_SUITE,
        .duration = 3.0f,
        .cam_pos = {5, 1.6f, 1},
        .cam_target = {6.8f, 1.7f, 2.6f},  // where her robe was. Gone.
        .exposure = -0.05f, .warmth = 0.0f, .saturation = 0.7f,
        .grain = 0.3f, .contrast = 1.2f,
        .text = "__cleaned__",
    };

    // Beat 13: Return taxi — dawn Auckland. The character was always leaving.
    m->beats[i++] = (MontageBeat){
        .scene = STATE_RETURN_TAXI,
        .duration = 4.0f,
        .cam_pos = {0, 1.2f, 0},
        .cam_target = {0, 0.9f, -2},     // looking forward. The Sky Tower shrinks.
        .exposure = 0.05f, .warmth = 0.35f, .saturation = 0.9f,
        .grain = 0.25f, .contrast = 1.0f,
        .text = NULL,
    };

    // The pillow indent — the last image before the void.
    // Hold. Longer than comfortable.
    m->beats[i++] = (MontageBeat){
        .scene = STATE_SPACE_SUITE,
        .duration = 4.0f,
        .cam_pos = {0.6f, 0.9f, -5.0f},
        .cam_target = {0.6f, 0.68f, -5.2f},   // looking down at her pillow. The indent.
        .exposure = -0.1f, .warmth = 0.2f, .saturation = 0.6f,
        .grain = 0.4f, .contrast = 1.1f,
        .text = NULL,
    };

    // The void — silence. The game ends.
    m->beats[i++] = (MontageBeat){
        .scene = STATE_STARS,
        .duration = 8.0f,
        .cam_pos = {0, 0, 0},
        .cam_target = {0, 0.5f, -1},
        .exposure = 0.0f, .warmth = 0.0f, .saturation = 0.3f,
        .grain = 0.2f, .contrast = 0.8f,
        .text = NULL,
    };

    m->beat_count = i;
}

// ============================================================
// CINEMATIC FX — update springs, drive shader uniforms
// Split screen, iris wipe, freeze frame, intertitles,
// aspect ratio shift, jump cuts (Breathless-style)
// ============================================================

static void cinema_spring(float *val, float *vel, float target, float dt) {
    float force = -280.0f * (*val - target) - 26.0f * (*vel);
    *vel += (force / 0.9f) * dt;
    *val += *vel * dt;
}

static void cinema_init(void) {
    memset(&cinema, 0, sizeof(CinematicFX));
    cinema.iris_radius = 2.0f;
    cinema.iris_target = 2.0f;
    cinema.iris_center_x = 0.5f;
    cinema.iris_center_y = 0.5f;
    cinema.split_pos = 0.5f;
    cinema.split_pos_target = 0.5f;
}

static void cinema_update(float dt) {
    // Split screen spring
    cinema_spring(&cinema.split_blend, &cinema.split_blend_vel,
                  cinema.split_blend_target, dt);
    cinema_spring(&cinema.split_pos, &cinema.split_pos_vel,
                  cinema.split_pos_target, dt);

    // Iris spring
    cinema_spring(&cinema.iris_radius, &cinema.iris_vel,
                  cinema.iris_target, dt);
    SetPostFXIris(&postfx, cinema.iris_radius,
                  cinema.iris_center_x, cinema.iris_center_y);

    // Aspect ratio spring
    cinema_spring(&cinema.aspect_letterbox, &cinema.aspect_vel,
                  cinema.aspect_target, dt);
    SetPostFXLetterbox(&postfx, cinema.aspect_letterbox);

    // Freeze frame countdown
    if (cinema.frozen) {
        cinema.freeze_timer -= dt;
        if (cinema.freeze_timer <= 0) cinema.frozen = false;
    }

    // Intertitle spring
    if (cinema.intertitle_active) {
        float target_a = 1.0f;
        if (cinema.intertitle_timer < 0.3f)
            target_a = cinema.intertitle_timer / 0.3f;
        cinema_spring(&cinema.intertitle_alpha, &cinema.intertitle_alpha_vel,
                      target_a, dt);
        cinema.intertitle_timer -= dt;
        if (cinema.intertitle_timer <= 0) {
            cinema.intertitle_active = false;
            cinema.intertitle_alpha = 0;
        }
    }

    // Jump cuts (Breathless-style time skip)
    if (cinema.jump_cut_active) {
        cinema.jump_cut_timer -= dt;
        if (cinema.jump_cut_timer <= 0 && cinema.jump_cut_count > 0) {
            cinema.jump_cut_count--;
            cinema.jump_cut_timer = cinema.jump_cut_interval;
            SetPostFXFlash(&postfx, 0.6f, 1.0f, 1.0f, 1.0f);
            cut_flash_timer = 0.04f;
            state_time += cinema.jump_cut_time_skip;
            if (cinema.jump_cut_count <= 0)
                cinema.jump_cut_active = false;
        }
    }
}

// ── Cinematic trigger helpers ──

__attribute__((unused))
static void cinema_split_start(SplitMode mode, GameState second_scene,
                               Vector3 cam_pos, Vector3 cam_target, float split_pos) {
    cinema.split_mode = mode;
    cinema.split_scene = second_scene;
    cinema.split_camera.position = cam_pos;
    cinema.split_camera.target = cam_target;
    cinema.split_camera.up = (Vector3){0, 1, 0};
    cinema.split_camera.fovy = 70.0f;
    cinema.split_camera.projection = CAMERA_PERSPECTIVE;
    cinema.split_pos_target = split_pos;
    cinema.split_blend_target = 1.0f;
}

__attribute__((unused))
static void cinema_split_end(void) {
    cinema.split_blend_target = 0.0f;
}

static void cinema_iris_close(float cx, float cy) {
    cinema.iris_center_x = cx;
    cinema.iris_center_y = cy;
    cinema.iris_target = 0.0f;
    cinema.iris_active = true;
}

static void cinema_iris_open(void) {
    cinema.iris_target = 2.0f;
    cinema.iris_active = false;
}

static void cinema_freeze(float duration) {
    cinema.frozen = true;
    cinema.freeze_timer = duration;
    cinema.freeze_duration = duration;
}

static void cinema_intertitle(const char *text, float duration, Color color, bool fullscreen) {
    cinema.intertitle_active = true;
    cinema.intertitle_text = text;
    cinema.intertitle_timer = duration;
    cinema.intertitle_duration = duration;
    cinema.intertitle_alpha = 0.0f;
    cinema.intertitle_alpha_vel = 0.0f;
    cinema.intertitle_color = color;
    cinema.intertitle_fullscreen = fullscreen;
}

static void cinema_aspect(float amount) {
    cinema.aspect_target = amount;
}

static void cinema_jump_cuts(int count, float interval, float time_skip) {
    cinema.jump_cut_active = true;
    cinema.jump_cut_count = count;
    cinema.jump_cut_interval = interval;
    cinema.jump_cut_timer = interval;
    cinema.jump_cut_time_skip = time_skip;
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
        "STATE_SPLASH", "STATE_TITLE", "STATE_CAR", "STATE_DRIVING",
        "STATE_HOTEL_EXT", "STATE_LOBBY", "STATE_ELEVATOR", "STATE_HALLWAY",
        "STATE_ROOM", "STATE_BATHROOM", "STATE_BALCONY", "STATE_BED",
        "STATE_STARS", "STATE_HYPERSPACE", "STATE_SPACE_LOBBY",
        "STATE_SPACE_HOTEL", "STATE_SPACE_CORRIDOR", "STATE_SPACE_SUITE",
        "STATE_PARIS_DREAM", "STATE_RETURN_TAXI"
    };
    FILE *f = fopen("/tmp/ev_state", "w");
    if (f) {
        int idx = (int)s;
        int name_count = (int)(sizeof(names) / sizeof(names[0]));
        if (idx >= 0 && idx < name_count) fprintf(f, "%s\n", names[idx]);
        else fprintf(f, "STATE_TITLE\n");

        fprintf(f, "%.2f %.2f %.2f\n",
                (double)player.camera.position.x, (double)player.camera.position.y,
                (double)player.camera.position.z);
        fclose(f);
    }
}

static void load_state(GameState s) {
    // Kill ALL looping audio — each scene starts fresh. No overlap.
    StopAllAudio(&audio);
    // Restore master volume — silence zones may have lowered it
    SetMasterVolume(setting_master_vol);

    // Save returning_to_room state before central reset
    int saved_tasks = (returning_to_room || returning_from_dream) ? tasks_done : 0;
    bool saved_phone = returning_to_room ? phone_triggered : false;
    bool saved_photo_flipped = (returning_to_room || returning_from_dream) ? photograph_flipped : false;
    // Snapshot completed object names before scene rebuild
    const char *saved_completed[MAX_OBJECTS] = {0};
    int saved_completed_count = 0;
    if (returning_to_room || returning_from_dream) {
        saved_completed_count = completed_count;
        for (int i = 0; i < completed_count && i < MAX_OBJECTS; i++)
            saved_completed[i] = completed_objects[i];
    }

    previous_state = state;
    state = s;
    state_time = 0;
    tasks_done = 0;
    done_pause = 0;
    if (!returning_to_room && !returning_from_dream) { completed_count = 0; memset(completed_objects, 0, sizeof(completed_objects)); }
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
        case STATE_SPLASH:
            DisableCursor();
            StopAmbient(&audio);
            StopSuiteMusic(&audio); StopBalconyMusic(&audio); StopCorridorMusic(&audio);
            reset_splash_state();
            reset_postfx_clean(&postfx);
            set_exposure(0.0f);
            break;

        case STATE_TITLE:
            DisableCursor();
            StopAmbient(&audio);
            StopSuiteMusic(&audio); StopBalconyMusic(&audio); StopCorridorMusic(&audio);
            reset_title_state();
            PlayTitleMusic(&audio);
            reset_postfx_clean(&postfx);
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
            PlayTaxiRadio(&audio);     // warm lullaby-jazz through car speakers
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
            DisableCursor();
            build_lobby(&scene);
            init_player(&player, scene.spawn);
            StartAmbient(&audio, DRONE_LOBBY);
            StopClockAmbient(&audio);
            StopCityAmbient(&audio);
            StopWindAmbient(&audio);
            stop_through_wall_audio(&audio);
            // Lobby has its own through-wall: distant voices from restaurant/bar area
            PlayDistantVoices(&audio);
            SetSceneLighting(&lighting, LightingPreset_Lobby());
            set_exposure(0.0f);   // grand lobby — let chandelier work
            SetPostFXGrain(&postfx, 0.4f);
            // Gibbons — first appearance. Behind the desk, waiting for you.
            {
                Vector3 lobby_wps[] = {
                    {-3, 1.6f, -6},    // behind desk (starting)
                    {-1, 1.6f, -4},    // steps out from desk
                    {0, 1.6f, 2},      // leads toward elevator
                };
                init_npc(&gibbons, (Vector3){-3, 1.6f, -7}, lobby_wps, 3, 2.0f, 4.0f);
                static const char *lobby_lines[] = {
                    "Your room is ready.",
                    "The elevator. It goes further than you'd think.",
                };
                npc_set_dialogue(&gibbons, lobby_lines, 2, 3.0f);
            }
            break;

        case STATE_ELEVATOR:
            if (elevator_to_corridor) {
                build_elevator_space(&scene);
            } else {
                build_elevator(&scene);
            }
            init_player(&player, scene.spawn);
            elevator_ding_played = false;
            StopAmbient(&audio);
            StopClockAmbient(&audio);
            StopCityAmbient(&audio);
            StopWindAmbient(&audio);
            stop_through_wall_audio(&audio);
            StopEarthPresence(&audio);
            PlayElevatorHum(&audio);
            if (elevator_to_corridor) {
                // Space ride — gentler, mechanical, no wind
                PlayDoorThud(&audio);  // doors closing behind you
                player.gravity_mult = 0.4f;  // orbital gravity — you're already in space
                SetSceneLighting(&lighting, LightingPreset_ElevatorSpace());
                set_exposure(-0.05f);  // slightly dimmer — between floors
                SetPostFXGrain(&postfx, 0.35f);
                // Gibbons rides with you — shared space, formal silence
                {
                    Vector3 elev_wp = {0.4f, 1.6f, -0.3f};
                    init_npc(&gibbons, (Vector3){0.4f, 1.6f, -0.3f}, &elev_wp, 1, 0.0f, 0.0f);
                    gibbons.active = true;
                    gibbons.behavior = NPC_WAITING;
                    gibbons.yaw = 0;  // facing forward, same direction as player
                }
            } else {
                PlayElevatorWhoosh(&audio);  // ascending wind — building toward hyperspace
                player.gravity_mult = 1.0f;  // Earth gravity — last time you'll feel this
                SetSceneLighting(&lighting, LightingPreset_Elevator());
                set_exposure(0.0f);
                SetPostFXGrain(&postfx, 0.3f);  // fluorescent — less grain
            }
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
            player.fov_current = 75.0f;  // wider than suite — the void opens
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

            // ---- DREAM DISTORTION ----
            // The room is narrower and taller than it should be.
            // Proportions are wrong but you can't quite say how.
            for (int i = 0; i < scene.wall_count; i++) {
                scene.walls[i].pos.x *= 0.75f;      // squeeze horizontally
                scene.walls[i].size.x *= 0.75f;     // squeeze wall widths too
                scene.walls[i].pos.y *= 1.15f;      // stretch vertically
                scene.walls[i].size.y *= 1.15f;     // taller walls
            }
            for (int i = 0; i < scene.object_count; i++) {
                scene.objects[i].pos.x *= 0.75f;
            }
            scene.spawn.x *= 0.75f;

            // Gaps in the dream — walls missing where they should be
            {
                int removed = 0;
                for (int i = scene.wall_count / 3; i < scene.wall_count && removed < 3; i += 7) {
                    if (scene.walls[i].size.y < 3.0f && scene.walls[i].size.y > 0.5f) {
                        scene.walls[i].active = false;
                        removed++;
                    }
                }
            }

            // A door in the ceiling — looking up reveals an exit that makes no sense
            add_wall(&scene, 0, 3.8f, -2.0f, 0.8f, 0.04f, 1.2f, (Color){45,40,35,200});
            add_wall(&scene, 0.3f, 3.6f, -2.0f, 0.03f, 0.2f, 0.03f, (Color){160,140,80,180});

            // Impossible recursive window — shows the room's interior from outside
            add_wall(&scene, 2.5f, 1.5f, 0.05f, 1.2f, 1.0f, 0.04f, (Color){20,25,35,120});
            set_last_material(&scene, MAT_GLASS);

            // The floor has a seam — two rooms spliced together
            add_wall(&scene, 0, 0.01f, 0, 0.02f, 0.02f, 8.0f, (Color){25,20,15,200});
            // ---- END DREAM DISTORTION ----

            // ---- HER PRESENCE — she was just here ----
            // The Paris dream is a memory. The last good trip.
            // She's not in the room but she was, moments ago.

            // Her coat on the chair — draped, not hung. She'll be right back.
            add_wall(&scene, 2.0f, 0.8f, -2.0f, 0.5f, 0.7f, 0.08f, (Color){55,45,40,220});
            set_last_material(&scene, MAT_FABRIC);

            // Coffee for two on the table — one cup half-drunk
            // Cup 1 (hers, half-drunk — dark coffee visible)
            add_cylinder(&scene, -1.5f, 0.48f, 0.5f, 0.05f, 0.08f, (Color){220,215,205,230});
            add_cylinder(&scene, -1.5f, 0.44f, 0.5f, 0.04f, 0.04f, (Color){45,30,20,200});
            // Cup 2 (yours, untouched — full)
            add_cylinder(&scene, -1.2f, 0.48f, 0.7f, 0.05f, 0.08f, (Color){220,215,205,230});
            add_cylinder(&scene, -1.2f, 0.45f, 0.7f, 0.04f, 0.06f, (Color){45,30,20,200});

            // Fresh pillow indent — she was lying here. Just got up.
            // Slight depression in the pillow shape (her side, left)
            add_wall(&scene, -0.4f, 0.56f, -3.6f, 0.45f, 0.06f, 0.3f, (Color){235,232,225,255});
            set_last_material(&scene, MAT_FABRIC);

            // Bathroom door ajar — steam curling out. She just showered.
            add_wall(&scene, -3.8f, 1.0f, -1.5f, 0.04f, 2.0f, 0.6f, (Color){55,48,42,200});
            set_last_material(&scene, MAT_WOOD);
            // Steam from the gap — translucent wisps
            add_wall(&scene, -3.6f, 1.5f, -1.4f, 0.15f, 0.4f, 0.15f, (Color){200,200,205,15});
            add_wall(&scene, -3.5f, 1.9f, -1.6f, 0.12f, 0.3f, 0.12f, (Color){200,200,205,10});

            // HER TOOTHBRUSH — in the bathroom glass. The most mundane detail.
            // On second playthrough, unbearable.
            add_cylinder(&scene, -4.2f, 0.56f, -1.8f, 0.04f, 0.1f, (Color){210,210,215,180}); // glass
            add_cylinder(&scene, -4.18f, 0.62f, -1.8f, 0.015f, 0.14f, (Color){80,160,140,220}); // toothbrush

            // THE RED OBJECT — one color survives the B&W.
            // A scarf on the bed. Godard red. Memory preserves color selectively.
            // You remember what mattered.
            add_wall(&scene, 0.3f, 0.62f, -3.2f, 0.6f, 0.02f, 0.15f, (Color){255,30,25,255});
            set_last_material(&scene, MAT_EMISSIVE);  // self-lit — bleeds through B&W

            // Red flower in a glass on the table — Godard red survives the dream
            add_cylinder(&scene, -1.35f, 0.48f, 0.6f, 0.025f, 0.08f, (Color){210,210,215,180}); // glass
            add_cylinder(&scene, -1.35f, 0.56f, 0.6f, 0.015f, 0.1f, (Color){255,30,25,255});    // flower
            set_last_material(&scene, MAT_EMISSIVE);  // self-lit — bleeds through B&W

            // Her book — same novel as the suite nightstand, further along.
            // She was reading it here first. The timeline is real.
            add_wall(&scene, -0.8f, 0.58f, -3.8f, 0.22f, 0.04f, 0.15f, (Color){140,45,50,255});
            set_last_material(&scene, MAT_LEATHER);
            // Pages open — she put it down mid-sentence
            add_wall(&scene, -0.8f, 0.605f, -3.8f, 0.2f, 0.005f, 0.14f, (Color){235,230,220,255});

            // ---- END HER PRESENCE ----

            // Dream-logic compression: ceiling drops, walls close in
            // The room remembers itself wrong. Proportions off by 10-15%.
            // Not enough to be obvious. Enough to feel wrong.
            for (int wi = 0; wi < scene.wall_count; wi++) {
                // Ceiling panels: lower by 15%
                if (scene.walls[wi].pos.y > 2.5f)
                    scene.walls[wi].pos.y *= 0.85f;
                // Walls: 10% closer to center
                if (fabsf(scene.walls[wi].pos.x) > 3.0f)
                    scene.walls[wi].pos.x *= 0.9f;
            }
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
            StartAmbient(&audio, DRONE_PARIS_DREAM);  // the room's ghost — detuned, muffled, uncanny
            PlayDreamRain(&audio);       // rain on the window — Paris at night
            PlayDreamTraffic(&audio);    // distant city — other lives, unreachable
            SetSceneLighting(&lighting, LightingPreset_ParisDream());
            // B&W GODARD — high contrast monochrome, not warm golden
            set_exposure(0.05f);                     // slightly lifted
            SetPostFXWarmth(&postfx, 0.0f);         // NO warmth — pure B&W
            SetPostFXGrain(&postfx, 0.8f);          // heavy film grain — Godard 16mm
            SetPostFXSaturation(&postfx, 0.06f);    // near-zero — only the most saturated reds bleed through
            SetPostFXCA(&postfx, 2.0f);             // subtle chromatic shift
            SetPostFXVignette(&postfx, 2.5f);       // heavy vignette — iris-like
            SetPostFXContrast(&postfx, 1.8f);       // crushed blacks, blown whites — Godard contrast
            scene.fog_color = (Color){10, 10, 12, 255};  // slightly blue-black — dream haze
            scene.fog_density = 0.012f;              // heavier fog — the dream closes in
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
            StopTaxiRadio(&audio);       // radio dies — reality breaks
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
            gibbons_glanced = false;
            StopAmbient(&audio);
            StopClockAmbient(&audio);
            StopCityAmbient(&audio);
            StopWindAmbient(&audio);
            stop_through_wall_audio(&audio);
            StopHyperspaceTone(&audio);
            StopHyperspaceRiser(&audio);
            PlayArrivalThump(&audio);    // deep bass impact — you have landed
            PlayGravitySettle(&audio);   // hull creak — the ship acknowledges your weight
            PlayAirlockHiss(&audio);     // pressurization — you're aboard
            PlayEarthPresence(&audio);   // 30Hz sub-bass — the planet's weight
            player.gravity_mult = 0.4f;  // orbital gravity — jumps soar, wall runs linger
            StartAmbient(&audio, DRONE_SPACE_LOBBY);
            // Ambient life — other guests exist somewhere in the station
            PlayCommsChatter(&audio);  // low murmur — someone on a call, far away
            SetSoundVolume(audio.snd_comms_chatter, 0.008f);  // whisper
            lobby_ding_timer = 4.0f + (float)(rand() % 40) / 10.0f;  // first ding in 4-8s
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
                    "Your room is ready.",
                    "The window. You should see it.",
                    "This way. I'll take you through.",
                };
                npc_set_dialogue(&gibbons, lobby_lines, 3, 3.0f);
            }
            break;

        case STATE_SPACE_HOTEL:
            build_space_hotel(&scene);
            init_player(&player, scene.spawn);
            StopAmbient(&audio);
            StopClockAmbient(&audio);
            StopCityAmbient(&audio);
            StopWindAmbient(&audio);
            stop_through_wall_audio(&audio);
            PlayArrivalThump(&audio);    // deep impact — the scale hits you
            PlayAirlockHiss(&audio);     // pressurization
            PlayEarthPresence(&audio);   // 30Hz sub-bass — Earth visible everywhere
            player.gravity_mult = 0.4f;  // orbital gravity
            StartAmbient(&audio, DRONE_SPACE_LOBBY);  // reuse lobby drone — vast reverberant space
            SetSceneLighting(&lighting, LightingPreset_SpaceHotel());
            set_exposure(-0.08f);          // darker than lobby — more dramatic
            SetPostFXGrain(&postfx, 0.35f);
            // Ambient life — same hotel, same elevators, same distant guests
            // Other people's relationships, muffled through glass and distance
            PlayCommsChatter(&audio);
            SetSoundVolume(audio.snd_comms_chatter, 0.005f);  // whisper — farther than lobby
            PlayDistantVoices(&audio);
            SetSoundVolume(audio.snd_distant_voices, 0.015f);  // couples' murmur — other lives
            lobby_ding_timer = 6.0f + (float)(rand() % 60) / 10.0f;  // first ding in 6-12s
            // Gibbons — walks the central axis, 4 waypoints through the atrium
            {
                Vector3 hotel_wps[] = {
                    {0, 1.6f, 0},       // near spawn
                    {0, 1.6f, 12},      // at the piano
                    {0, 1.6f, 24},      // past the sculpture
                    {0, 1.6f, 36},      // at the corridor door
                };
                init_npc(&gibbons, (Vector3){0, 1.6f, -1}, hotel_wps, 4, 2.5f, 4.0f);
                static const char *hotel_lines[] = {
                    "The hotel takes some getting used to.",
                    "Someone left this here. It doesn't play itself.",
                    "Your suite is through here.",
                };
                npc_set_dialogue(&gibbons, hotel_lines, 3, 3.5f);
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
            // Through bulkheads: space has its own acoustic character
            PlayCommsChatter(&audio);
            // Per-door spatial sounds — door 0: machinery+laughter, door 1: TV, door 2: silence
            PlayMuffledMachinery(&audio);
            PlayMuffledLaughter(&audio);  // couple behind the warm-light door
            PlaySound(audio.snd_running_water);
            PlaySound(audio.snd_tv_murmur);
            // Door positions for distance-based volume
            door_listen_timer = 0;
            door_1_silenced = false;
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
                    "Last door on the left. Your neighbour's quiet.",
                };
                npc_set_dialogue(&gibbons, corr_lines, 3, 3.5f);
            }
            break;

        case STATE_SPACE_SUITE:
            build_space_suite(&scene);
            init_player(&player, scene.spawn);
            wrongness_photo = false;
            wrongness_door = false;
            wrongness_earth = false;
            adjoining_door_wall_idx = -1;
            adjoining_door_opened = false;
            photograph_flipped = false;
            photo_text_timer = 0;
            // Photograph on nightstand — interactable. Optional, observational.
            add_object(&scene, -2.5f, 0.62f, -3.5f, "photograph", (Color){240,238,230,255}, 1);
            SetPostFXWarmth(&postfx, 0.0f);
            StopAmbient(&audio);
            StopCityAmbient(&audio);
            StopWindAmbient(&audio);
            StopMuffledPiano(&audio); StopFootstepsAbove(&audio);
            StopDistantVoices(&audio); StopMuffledLaughter(&audio);
            StopCorridorMusic(&audio);
            StopSound(audio.snd_running_water); StopSound(audio.snd_tv_murmur);
            PlayDoorThud(&audio);     // suite door closing — sealed in
            PlayAirlockHiss(&audio);  // pressurization equalize
            player.gravity_mult = 0.5f;  // suite has more gravity — domestic, grounded
            StartAmbient(&audio, DRONE_SPACE_SUITE);
            // The constant — insulated hotel hum (life support, air, the station breathing)
            // Commandment 4: this disappears on the balcony. The absence is the point.
            PlayMuffledMachinery(&audio);
            SetSoundVolume(audio.snd_muffled_machinery, 0.012f);  // barely there — you only notice when it's gone
            PlayClockAmbient(&audio);  // the room's heartbeat
            audio.clock_rate = 1.0f;   // reset clock rate
            SetSoundPitch(audio.snd_clock, 1.0f);
            agency_removal_timer = 0;
            SetSceneLighting(&lighting, LightingPreset_SpaceSuite());
            set_exposure(-0.1f);
            SetPostFXGrain(&postfx, 0.35f);
            // Gibbons — gestures you in, walks to sofa, sits
            // "He bows. He deactivates." (Master Plan)
            // After all tasks: stands, delivers final line, walks out
            {
                Vector3 suite_wps[] = {
                    {0, 1.6f, 4},       // threshold — pauses, gestures
                    {0, 1.6f, 2},       // into room
                    {-2.0f, 1.6f, 1},   // toward sofa
                };
                init_npc(&gibbons, (Vector3){0, 1.6f, 5.5f}, suite_wps, 3, 2.0f, 3.0f);
                static const char *suite_lines[] = {
                    "You should see the view.",
                    "Make yourself comfortable. It's yours.",
                    "Three hours. You'd be surprised what fits.",
                };
                npc_set_dialogue(&gibbons, suite_lines, 3, 3.5f);
            }
            if (returning_from_dream) {
                returning_from_dream = false;
                tasks_done = saved_tasks;
                // Restore completed object states
                for (int i = 0; i < scene.object_count; i++) {
                    for (int j = 0; j < saved_completed_count; j++) {
                        if (saved_completed[j] && strcmp(scene.objects[i].name, saved_completed[j]) == 0) {
                            scene.objects[i].done = true;
                            scene.objects[i].step = scene.objects[i].max_steps;
                        }
                    }
                }
                // Restore warmth — the dream didn't reset progress
                SetPostFXWarmth(&postfx, (float)tasks_done / SPACE_TASK_COUNT);
                paris_dream_timer = -1;  // don't re-trigger

                // The photograph is gone. It was here before the dream.
                // Don't explain it. Don't callback to it.
                for (int i = 0; i < scene.wall_count; i++) {
                    if (fabsf(scene.walls[i].pos.x - 3.0f) < 0.1f &&
                        fabsf(scene.walls[i].pos.z + 3.5f) < 0.1f &&
                        scene.walls[i].pos.y > 0.5f && scene.walls[i].pos.y < 0.6f) {
                        scene.walls[i].active = false;
                    }
                }
                wrongness_photo = true;  // photo was placed (now gone — wrongness changed)
                // The door seam persists. It was always there. Wasn't it?
                wrongness_door = true;  // re-place after rebuild
                // Restore photograph flip state if player turned it before the dream
                if (saved_photo_flipped) {
                    photograph_flipped = true;
                    for (int wi = 0; wi < scene.wall_count; wi++) {
                        if (fabsf(scene.walls[wi].pos.x + 2.5f) < 0.2f &&
                            fabsf(scene.walls[wi].pos.z + 3.5f) < 0.2f &&
                            scene.walls[wi].pos.y > 0.6f && scene.walls[wi].pos.y < 0.7f &&
                            scene.walls[wi].size.y < 0.05f) {
                            scene.walls[wi].color = (Color){230,218,195,255};
                        }
                    }
                    add_wall(&scene, -2.48f, 0.625f, -3.48f, 0.04f, 0.003f, 0.03f, (Color){190,50,45,200});
                    set_last_decal(&scene);
                    for (int wi = 0; wi < scene.object_count; wi++) {
                        if (strcmp(scene.objects[wi].name, "photograph") == 0) {
                            scene.objects[wi].done = true;
                            scene.objects[wi].step = scene.objects[wi].max_steps;
                        }
                    }
                }
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
           s, scene.wall_count, scene.object_count, (double)final_exp, current_style,
           lighting.shadowReady ? "ON" : "OFF", (double)scene.fog_density);
    printf("[DBG]   fog_color=(%d,%d,%d) cam=(%.1f,%.1f,%.1f)\n",
           scene.fog_color.r, scene.fog_color.g, scene.fog_color.b,
           (double)player.camera.position.x, (double)player.camera.position.y, (double)player.camera.position.z);

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
            case STATE_SPACE_HOTEL: sr = 40.0f; break;  // massive Gehry atrium
            case STATE_HOTEL_EXT:  sr = 30.0f; break;
            default: break;
        }
        UpdateShadowMatrix(&lighting,
            lighting.activePreset.keyDir,
            (Vector3){0, 2, 0}, sr);
    }

    // Reset interaction ritual timers
    for (int i = 0; i < 5; i++) { interaction_timers[i] = 0; interaction_phases[i] = 0; }
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
// QA TYPES (used by qa_render_shot and QA mode in main)
// ============================================================
#ifdef QA_MODE
typedef struct {
    Vector3 pos;
    Vector3 target;
} QACam;
typedef struct {
    GameState gs;
    const char *name;
    QACam hero;
    QACam spawn;
    bool dark_by_design;
    bool outdoor;
} QAEntry;

// Render scene from a given camera angle and save screenshot
static void qa_render_shot(QACam cam, const char *suffix,
                           const char *scene_name, bool outdoor) {
    if (cam.pos.x != 0 || cam.pos.y != 0 || cam.pos.z != 0)
        player.camera.position = cam.pos;
    player.camera.target = cam.target;
    for (int f = 0; f < 8; f++) {
        BeginTextureMode(render_target);
        ClearBackground(scene.fog_color.a > 0 ? scene.fog_color : (Color){8,10,22,255});
        draw_scene_3d(&player, &scene, &lighting,
            &cube_model, cube_model_loaded, &cyl_model, cyl_model_loaded,
            &sphere_model, sphere_model_loaded, &cone_model, cone_model_loaded,
            &skytower_model, skytower_loaded,
            !outdoor, (float)f * 0.016f);
        EndTextureMode();
        process_bloom(&postfx, render_target);
        BeginTextureMode(postfx_target); ClearBackground(BLACK);
        draw_postfx(&postfx, render_target); EndTextureMode();
        BeginDrawing(); ClearBackground(BLACK); EndDrawing();
    }
    RenderTexture2D disp = postfx.ready ? postfx_target : render_target;
    Image img = LoadImageFromTexture(disp.texture);
    ImageFlipVertical(&img);
    char path[256];
    snprintf(path, sizeof(path), "qa/screenshots/%s_%s.png", scene_name, suffix);
    ExportImage(img, path);
    UnloadImage(img);
}
#endif

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
    // Split screen second viewport
    split_target = LoadRenderTexture(RENDER_W, RENDER_H);
    SetTextureFilter(split_target.texture, TEXTURE_FILTER_POINT);
    split_target_loaded = true;
    cinema_init();

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
    // Types defined above main() for qa_render_shot function

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
            .hero  = {{-14, 1.6f, 3}, {5, 1.4f, 1}},        // near spawn, looking down curved corridor (doors + windows visible)
            .spawn = {{-14, 1.6f, 3}, {-8, 1.6f, 2}},       // toward first porthole
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

        // Take hero shot — this is also the image we analyze
        qa_render_shot(qa_scenes[qi].hero, "hero",
                       qa_scenes[qi].name, qa_scenes[qi].outdoor);

        // Capture hero framebuffer for pixel analysis
        RenderTexture2D display = postfx.ready ? postfx_target : render_target;
        Image img = LoadImageFromTexture(display.texture);
        ImageFlipVertical(&img);
        char primary_path[256];
        snprintf(primary_path, sizeof(primary_path), "qa/screenshots/%s.png", qa_scenes[qi].name);
        ExportImage(img, primary_path);

        // Take spawn shot
        load_state(qa_scenes[qi].gs);
        fade_alpha = 0.0f; fade_target = 0.0f;
        qa_render_shot(qa_scenes[qi].spawn, "spawn",
                       qa_scenes[qi].name, qa_scenes[qi].outdoor);

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
                "    COMPOSITION: center luma %.0f — nothing visible in frame center\n", (double)center_avg); issues++;
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
                "    COLOR: variance %.0f — flat, no accent colors\n", (double)cvar); issues++;
        }
        if (hue_count < 2 && !dark) {
            ib += snprintf(ibuf+ib, sizeof(ibuf)-(size_t)ib,
                "    COLOR: only %d hue buckets — monotone\n", hue_count); issues++;
        }

        // BUDGET checks
        if (wall_pct > 85.0f) {
            ib += snprintf(ibuf+ib, sizeof(ibuf)-(size_t)ib,
                "    BUDGET: wall budget %.0f%% (%d/%d) — near overflow\n", (double)wall_pct, scene.wall_count, MAX_WALLS); issues++;
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
                "    ANTI-PATTERN: reads as horror (cold %.0f, dark luma %.0f)\n", (double)warmth, (double)luma); issues++;
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
               mat_types_used, (double)mat_cov, (double)luma, (double)contrast_ratio, (double)edge_density, hue_count, interact_count);
        if (issues > 0) printf("%s", ibuf);
        qa_total_issues += issues;

        // ── JSON entry ──
        if (jf) {
            if (qi > 0) fprintf(jf, ",\n");
            fprintf(jf, "    {\n");
            fprintf(jf, "      \"name\": \"%s\",\n", qa_scenes[qi].name);
            fprintf(jf, "      \"status\": \"%s\",\n", qs);
            fprintf(jf, "      \"walls\": %d,\n", scene.wall_count);
            fprintf(jf, "      \"wall_pct\": %.1f,\n", (double)wall_pct);
            fprintf(jf, "      \"material_types\": %d,\n", mat_types_used);
            fprintf(jf, "      \"material_coverage\": %.1f,\n", (double)mat_cov);
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
            fprintf(jf, "      \"obj_max_dist\": %.1f,\n", (double)obj_max_dist);
            fprintf(jf, "      \"luma\": %.1f,\n", (double)luma);
            fprintf(jf, "      \"black_pct\": %.1f,\n", (double)black_pct);
            fprintf(jf, "      \"bright_pct\": %.1f,\n", (double)bright_pct);
            fprintf(jf, "      \"mid_tone_pct\": %.1f,\n", (double)mid_pct);
            fprintf(jf, "      \"color_variance\": %.1f,\n", (double)cvar);
            fprintf(jf, "      \"hue_buckets\": %d,\n", hue_count);
            fprintf(jf, "      \"warmth\": %.1f,\n", (double)warmth);
            fprintf(jf, "      \"avg_rgb\": [%.0f, %.0f, %.0f],\n", (double)r_avg, (double)g_avg, (double)b_avg);
            fprintf(jf, "      \"contrast_ratio\": %.2f,\n", (double)contrast_ratio);
            fprintf(jf, "      \"edge_density\": %.1f,\n", (double)edge_density);
            fprintf(jf, "      \"center_luma\": %.1f,\n", (double)center_avg);
            fprintf(jf, "      \"edge_luma\": %.1f,\n", (double)edge_avg);
            fprintf(jf, "      \"lr_balance\": %.3f,\n", (double)lr_balance);
            fprintf(jf, "      \"tb_balance\": %.3f,\n", (double)tb_balance);
            fprintf(jf, "      \"quadrant_luma\": [%.1f, %.1f, %.1f, %.1f],\n", (double)q_luma[0], (double)q_luma[1], (double)q_luma[2], (double)q_luma[3]);
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
    if (split_target_loaded) UnloadRenderTexture(split_target);
    CloseWindow();
    return qa_total_issues > 0 ? 1 : 0;

#else  // normal game

#ifdef DEV_START
    load_state(DEV_START);
#else
    load_state(STATE_SPLASH);
#endif

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        UpdateFileMusic(&audio);

        // ── Interaction micro-freeze — a moment of attention ──
        // 50ms pause after pressing E on an object. The world holds its breath.
        if (interact_freeze > 0) {
            interact_freeze -= dt;
            if (interact_freeze > 0) {
                // Skip the rest of the frame — freeze
                BeginDrawing();
                EndDrawing();
                continue;
            }
        }
        // Interaction lean spring — FOV snaps narrow then recovers
        if (interact_lean > 0.01f || fabsf(interact_lean_vel) > 0.01f) {
            float spring_k = 200.0f, spring_d = 18.0f;
            float force = -spring_k * interact_lean - spring_d * interact_lean_vel;
            interact_lean_vel += force * dt;
            interact_lean += interact_lean_vel * dt;
            if (interact_lean < 0) { interact_lean = 0; interact_lean_vel = 0; }
        }

        update_menu_springs(dt);
        cinema_update(dt);
        // ── Freeze frame — Truffaut's 400 Blows. A moment held past comfort. ──
        if (cinema.frozen && menu_mode == MENU_NONE) {
            // Still render the frozen frame, but skip all game logic
            goto render_frame;
        }
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
                           nudge_selected,
                           (double)w->pos.x, (double)w->pos.y, (double)w->pos.z,
                           (double)w->size.x, (double)w->size.y, (double)w->size.z);
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

        // ── Cinema debug keys (F6-F9) — test film grammar tools ──
        // F6: Iris wipe toggle (close on crosshair / open)
        if (IsKeyPressed(KEY_F6)) {
            if (cinema.iris_active) {
                cinema_iris_open();
                show_text("IRIS: OPEN");
            } else {
                cinema_iris_close(0.5f, 0.5f);
                show_text("IRIS: CLOSE");
            }
        }
        // F7: Letterbox toggle (cinematic aspect ratio)
        if (IsKeyPressed(KEY_F7)) {
            if (cinema.aspect_target > 0.3f) {
                cinema_aspect(0.0f);
                show_text("ASPECT: 16:10");
            } else {
                cinema_aspect(0.7f);
                show_text("ASPECT: 2.39:1");
            }
        }
        // F8: Freeze frame (2 second hold)
        if (IsKeyPressed(KEY_F8)) {
            cinema_freeze(2.0f);
            show_text("FREEZE");
        }
        // F9: Intertitle card
        if (IsKeyPressed(KEY_F9)) {
            cinema_intertitle("CHAMBRE 714", 2.5f,
                              (Color){240, 232, 210, 255}, true);
        }
        // F10: Jump cuts (3 cuts, 0.8s apart, skip 900s each)
        if (IsKeyPressed(KEY_F10)) {
            cinema_jump_cuts(3, 0.8f, 900.0f);
            show_text("BREATHLESS");
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

        // ---- MONTAGE (overrides normal update when active) ----
        if (montage.active) {
            montage_update(&montage, dt);
            if (!montage.active) {
                // Montage finished — the void. Silence. The game ends.
                // Kill all audio. The silence IS the ending.
                StopAllAudio(&audio);
                SetMasterVolume(0.0f);
                // Slow fade to black over 3 seconds
                fade_alpha = 0; fade_target = 1.0f; fade_speed = 0.33f;
                transitioning = false;
            }
        }
        // Post-montage: fade complete → close gracefully
        if (!montage.active && montage.beat_count > 0 && fade_alpha >= 0.99f) {
            // Print runtime and exit
            int rt_m = (int)total_time / 60, rt_s = (int)total_time % 60;
            printf("\n[PLAYTHROUGH] Total runtime: %d:%02d\n\n", rt_m, rt_s);
            CloseWindow();
            return 0;
        }

        // ---- UPDATE ----
        switch (state) {
            case STATE_SPLASH: {
                // Auto-advance to title after splash animation (5.5s)
                if (state_time > 4.5f) {
                    transition_to(STATE_TITLE);
                }
                // Skip splash on any key
                if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ESCAPE)) {
                    transition_to(STATE_TITLE);
                }
                break;
            }

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
                // Driver speaks — two lines, the frame story
                {
                    static bool driver_line1 = false, driver_line2 = false;
                    if (state_time > 1.5f && !driver_line1) {
                        show_text("Where to?");
                        driver_line1 = true;
                    }
                    if (state_time > 4.0f && !driver_line2) {
                        show_text("Sky Tower. Three hours.");
                        driver_line2 = true;
                    }
                }
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
                if (state_time > 4.5f && state_time < 6.0f) show_text("You going up to the tower?");
                if (state_time > 8.0f && state_time < 8.5f) hide_text();
                if (state_time > 9.0f && state_time < 9.5f) show_text("They say there's a hotel up there now.");
                if (state_time > 12.0f && state_time < 12.5f) hide_text();
                if (state_time > 12.5f && state_time < 13.0f) show_text("Three hours to kill.");
                // Taxi drops you at the Sky Tower — terrestrial hotel first
                if (state_time > 13.5f) {
                    transition_to(STATE_HOTEL_EXT);
                }
                if (IsKeyPressed(KEY_ENTER) && state_time > 3) {
                    transition_to(STATE_HOTEL_EXT);
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
                UpdateEVAudio(&audio, player.moving, player.sprinting, scene.surface, dt);
                // Sky Tower beacon — red pulse every 2 seconds
                {
                    float beacon = sinf(state_time * PI) * 0.5f + 0.5f;
                    beacon = beacon * beacon;  // sharper pulse
                    SetPointLightIdx(&lighting, 2,
                        0, 52, 8,
                        beacon * 0.8f, beacon * 0.05f, beacon * 0.05f,
                        beacon * 15.0f);
                }
                // Looking up at the tower — FOV widens with awe
                if (player.camera.target.y > player.camera.position.y + 0.3f) {
                    float look_up = fminf(1.0f,
                        (player.camera.target.y - player.camera.position.y - 0.3f) / 0.5f);
                    player.fov_current += (78.0f - player.fov_current) * look_up * 0.05f;
                }
                if (scene.has_exit) {
                    float dist = Vector3Distance(player.camera.position, scene.exit_pos);
                    // Threshold crossing — slow the player as they approach the door
                    if (dist < 3.0f) {
                        float threshold_t = 1.0f - (dist / 3.0f);
                        float slow = 1.0f - threshold_t * 0.5f;
                        player.vel.x *= slow;
                        player.vel.z *= slow;
                        // FOV narrows — contraction into the building
                        player.fov_current += (65.0f - player.fov_current) * threshold_t * 0.1f;
                    }
                    if (dist < 1.5f) transition_to(STATE_LOBBY);
                }
                break;

            case STATE_LOBBY:
                update_player(&player, &scene, dt);
                UpdateEVAudio(&audio, player.moving, player.sprinting, scene.surface, dt);
                update_npc(&gibbons, player.camera.position, &scene, dt);
                // Chandelier sway — grand lobby breathes
                {
                    float sway = sinf(state_time * 0.7f) * 0.3f;
                    SetPointLightIdx(&lighting, 0,
                        sway, 6.5f, -3.0f,
                        0.9f, 0.75f, 0.45f, 10.0f);
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
                                if (strcmp(obj->name, "elevator") == 0) {
                                    obj->step++; obj->done = true;
                                    PlayInteract(&audio, INTERACT_CLICK);
                                    transition_to(STATE_ELEVATOR);
                                    break;
                                }
                                if (strcmp(obj->name, "newspaper") == 0) {
                                    obj->step++; obj->done = true;
                                    PlayInteract(&audio, INTERACT_FABRIC);
                                    show_text("ORBITAL SUITE OPENS — THREE HOURS TO KILL");
                                    break;
                                }
                                if (strcmp(obj->name, "bell") == 0) {
                                    obj->step++; obj->done = true;
                                    PlayElevatorDing(&audio);  // same ding — foreshadowing
                                    kick_camera(&player, -0.01f, 0.005f);
                                    break;
                                }
                            }
                        }
                    }
                }
                // Gibbons dialogue → vignette text (same system as space scenes)
                {
                    const char *line = npc_current_dialogue(&gibbons);
                    if (line) show_text(line);
                }
                // Elevator is the only way up — no hallway exit
                break;

            case STATE_ELEVATOR:
                if (elevator_to_corridor) {
                    // SPACE ELEVATOR — lobby to corridor
                    // 4 phases: jolt (0-0.3s), cruise (0.3-3s), decel (3-3.8s), arrive (3.8-4.2s)
                    {
                        float t = state_time;
                        float speed = 0;

                        if (t < 0.3f) {
                            // Jolt — mechanical start, slight camera drop then rise
                            float jolt = t / 0.3f;
                            speed = jolt * jolt * 2.0f;  // quadratic ramp
                            // Subtle camera shake — you feel the mechanism engage
                            float shake = (1.0f - jolt) * 0.008f;
                            player.camera.position.x += sinf(t * 60) * shake;
                            player.camera.target.x += sinf(t * 60) * shake;
                        } else if (t < 3.0f) {
                            // Cruise — steady rise, gentle sway
                            speed = 2.0f;
                            // Very subtle sway — the shaft isn't perfectly straight
                            float sway = sinf(t * 2.5f) * 0.003f;
                            player.camera.position.x += sway;
                            player.camera.target.x += sway;
                        } else if (t < 3.8f) {
                            // Deceleration — slowing, arriving
                            float decel = 1.0f - (t - 3.0f) / 0.8f;
                            speed = 2.0f * decel * decel;
                        }
                        // else: stopped, waiting for cut

                        player.camera.position.y += speed * dt;
                        player.camera.target.y += speed * dt;
                    }
                    // Gibbons shares the ride — update him so he breathes
                    update_npc(&gibbons, player.camera.position, &scene, dt);
                    // Shared silence — Gibbons adjusts his tie at 1.5s (idle quirk)
                    if (state_time > 1.5f && state_time < 1.6f && gibbons.active) {
                        gibbons.idle_timer = 5.0f;  // trigger tie-adjust in npc_update
                    }
                    // Warmth builds as you ascend — the hotel welcomes you
                    {
                        float ascent_t = fminf(1.0f, state_time / 4.0f);
                        SetPostFXWarmth(&postfx, ascent_t * 0.3f);
                    }
                    // Ding at 3.5s — arrival ding, warm tone not sharp ping
                    if (state_time > 3.5f && !elevator_ding_played) {
                        elevator_ding_played = true;
                        SetSoundVolume(audio.elevator_ding, 0.3f);  // gentle, not sharp
                        SetSoundPan(audio.elevator_ding, 0.5f);     // centered
                        PlayElevatorDing(&audio);
                    }
                    // Cut after ding has rung and doors "open"
                    if (state_time > 4.2f) {
                        elevator_to_corridor = false;
                        hard_cut_to(STATE_SPACE_HOTEL);
                    }
                } else {
                    // TERRESTRIAL ELEVATOR — Auckland lobby up the Sky Tower
                    // Simple ascent to hotel floor. The real journey comes later.
                    {
                        float t = state_time;
                        float accel = 0.3f + t * 0.3f;
                        player.camera.position.y += accel * dt;
                        player.camera.target.y += accel * dt;

                        // Mechanical vibration — you feel the tower
                        if (t < 2.0f) {
                            float vib = (1.0f - t / 2.0f) * 0.004f;
                            player.camera.position.x += sinf(t * 40) * vib;
                        }
                    }
                    // Ding at 2 seconds
                    if (state_time > 2.0f && !elevator_ding_played) {
                        elevator_ding_played = true;
                        PlayElevatorDing(&audio);
                    }
                    // Arrive at hallway floor
                    if (state_time > 3.5f) {
                        hard_cut_to(STATE_HALLWAY);
                    }
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
                    // WARDROBE LAUNCH — room complete, blast to space
                    // FOV widens + CA ramps during pause — something is building
                    if (done_pause > 0.5f) {
                        float ramp = fminf(1.0f, (done_pause - 0.5f) / 1.5f);
                        player.camera.fovy = 70.0f + ramp * 15.0f;
                        SetPostFXCA(&postfx, 2.5f + ramp * 5.0f);
                    }
                    if (done_pause > 2.0f) {
                        player.camera.fovy = 70.0f;
                        hard_cut_to(STATE_HYPERSPACE);
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
                                    // Fog clears — wipe the mirror. Not a reflection, just clarity.
                                    for (int m = 0; m < scene.wall_count; m++) {
                                        Color mc = scene.walls[m].color;
                                        if (mc.r == 210 && mc.g == 215 && mc.b == 220) {
                                            scene.walls[m].color = (Color){230, 232, 238, 220};
                                        }
                                    }
                                    // Warm point light behind mirror — reflection glow
                                    SetPointLightIdx(&lighting, 3,
                                        2.38f, 1.7f, 0.5f,
                                        0.6f, 0.45f, 0.3f, 2.0f);
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
                        // Speed slows — you approach the infinite gradually
                        float slow = 1.0f - t * 0.5f;  // down to 50% at railing
                        player.vel.x *= slow;
                        player.vel.z *= slow;
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
                        // Smoke trail — drifts in zero-g, three wisps dispersing
                        add_sphere(&scene, player.camera.position.x + 0.3f,
                                   player.camera.position.y - 0.2f,
                                   player.camera.position.z - 0.5f,
                                   0.08f, (Color){180,180,185,40});
                        add_sphere(&scene, player.camera.position.x + 0.5f,
                                   player.camera.position.y,
                                   player.camera.position.z - 0.7f,
                                   0.06f, (Color){180,180,185,25});
                        add_sphere(&scene, player.camera.position.x + 0.2f,
                                   player.camera.position.y + 0.3f,
                                   player.camera.position.z - 0.4f,
                                   0.04f, (Color){180,180,185,15});
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
                // The balcony holds you. Then lets you go.
                if (state_time > 14.0f)
                    hard_cut_to(STATE_BED);
                if (state_time > 10 && IsKeyPressed(KEY_ENTER))
                    hard_cut_to(STATE_BED);
                break;

            case STATE_PARIS_DREAM:
                // The dream — Paris hotel room, black and white
                // Godard: Vivre sa vie, Breathless, Alphaville
                update_player(&player, &scene, dt);
                // Dream grain escalation — the image degrades like old film stock
                {
                    float dream_t = fminf(1.0f, state_time / 5.0f);
                    (void)dream_t;
                    // Contrast pulses — like a projector struggling
                    float pulse = 1.8f + 0.2f * sinf(state_time * 1.2f);
                    SetPostFXContrast(&postfx, pulse);
                    // Near-zero saturation — only emissive reds bleed through
                    SetPostFXSaturation(&postfx, 0.06f);
                    // Agency slowly draining — you're losing the dream
                    if (state_time > 20.0f) {
                        float dream_fade = fminf(1.0f, (state_time - 20.0f) / 40.0f);
                        player.control_mult = 0.5f - dream_fade * 0.3f;
                    }
                    // Grain increases — the film is burning
                    SetPostFXGrain(&postfx, 0.8f + state_time * 0.005f);
                    // Memory window — the silhouette flickers
                    // Eiffel → Sky Tower → neither. Memory edits itself.
                    {
                        float flk = sinf(state_time * 0.7f) * sinf(state_time * 1.3f);
                        for (int wi = 0; wi < scene.wall_count; wi++) {
                            if (scene.walls[wi].pos.z > -1.2f && scene.walls[wi].pos.z < -0.8f &&
                                scene.walls[wi].pos.x < -4.0f && scene.walls[wi].size.x < 0.1f &&
                                scene.walls[wi].size.y > 0.5f) {
                                scene.walls[wi].color.a = (unsigned char)(60.0f * (0.5f + 0.5f * flk));
                            }
                        }
                    }
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
                                    show_text("Paris. The two of you.");
                                    done_pause = -3.0f;
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
                        // Reset post-FX before hard cut to suite
                        SetPostFXWarmth(&postfx, 0.0f);
                        SetPostFXSaturation(&postfx, 0.92f);
                        SetPostFXCA(&postfx, 2.5f);
                        SetPostFXVignette(&postfx, 1.0f);
                        SetPostFXContrast(&postfx, 1.0f);
                        returning_from_dream = true;
                        hard_cut_to(STATE_SPACE_SUITE);
                    }
                }
                // Auto-exit after 20 seconds — the dream is brief, genuinely strange
                if (state_time > 20.0f) {
                    SetPostFXWarmth(&postfx, 0.0f);
                    SetPostFXSaturation(&postfx, 0.92f);
                    SetPostFXCA(&postfx, 2.5f);
                    SetPostFXVignette(&postfx, 1.0f);
                    SetPostFXContrast(&postfx, 1.0f);
                    returning_from_dream = true;
                    hard_cut_to(STATE_SPACE_SUITE);
                }
                break;

            case STATE_BED:
                // Clock deceleration — the emotional peak
                // Clock stopping = commandment #4 ("remove the constant")
                if (state_time < 8.0f) {
                    bed_clock_rate = 1.0f - (state_time / 8.0f);
                    if (bed_clock_rate < 0.05f) bed_clock_rate = 0;
                    SetClockRate(&audio, bed_clock_rate);
                } else if (bed_clock_rate > 0) {
                    bed_clock_rate = 0;
                    SetClockRate(&audio, 0);
                }
                // Bed drone fades in (3s+)
                if (state_time > 3.0f && !audio.bed_drone_playing) {
                    PlayBedDrone(&audio);
                }
                // Camera breathing — looking up at ceiling
                {
                    float breath = sinf(state_time * 0.5f) * 0.002f;
                    player.camera.target.y += breath;
                }
                // Progressive desaturation — color drains as consciousness fades
                {
                    float desat = fminf(1.0f, state_time / 16.0f);
                    SetPostFXSaturation(&postfx, 0.92f - desat * 0.6f);  // 0.92 → 0.32
                    // Contrast drops — edges soften
                    SetPostFXContrast(&postfx, 1.0f - desat * 0.4f);
                    // Grain increases — the image dissolves
                    SetPostFXGrain(&postfx, 0.3f + desat * 0.5f);
                }
                // Transition
                if (state_time > 20)
                    hard_cut_to(STATE_STARS);
                
                // Breathing — camera rises and falls like lungs
                {
                    float breath_rate = 0.4f;  // slow, surrendered breathing
                    float breath_amp = 0.015f;
                    float breath = sinf(state_time * breath_rate * 2 * PI) * breath_amp;
                    player.camera.position.y += breath * dt * 2.0f;
                }
                break;

            case STATE_STARS:
                // The held chord swells like a wave, then recedes
                if (state_time < 8.0f) {
                    float swell = state_time / 8.0f;
                    swell = swell * swell;  // quadratic — slow start, faster build
                    SetMasterVolume(swell);
                } else if (state_time < 13.0f) {
                    // Hold at peak briefly, then begin decay
                    float decay = 1.0f - (state_time - 10.0f) / 3.0f;
                    if (decay > 1.0f) decay = 1.0f;
                    if (decay < 0.0f) decay = 0.0f;
                    SetMasterVolume(decay * decay);  // quadratic decay — gentle
                }
                // Total silence before montage
                if (state_time > 14.0f) {
                    SetMasterVolume(0.0f);
                }
                // Camera drift — the void pulls you in, accelerating
                // You're floating. The stars turn. Faster, imperceptibly.
                {
                    float drift_rate = 0.003f + state_time * 0.0005f;  // accelerating
                    float drift_yaw = drift_rate;
                    float drift_pitch = sinf(state_time * 0.08f) * 0.002f;
                    Vector3 fwd = Vector3Normalize(Vector3Subtract(
                        player.camera.target, player.camera.position));
                    Vector3 rt = Vector3Normalize(Vector3CrossProduct(fwd, player.camera.up));
                    fwd = Vector3Transform(fwd, MatrixRotateY(drift_yaw * dt));
                    fwd = Vector3Transform(fwd, MatrixRotate(rt, drift_pitch * dt));
                    player.camera.target = Vector3Add(player.camera.position, fwd);
                }
                // Reset post-FX from BED desaturation
                if (state_time < 1.0f) {
                    SetPostFXSaturation(&postfx, 0.32f + state_time * 0.6f);
                    SetPostFXContrast(&postfx, 0.6f + state_time * 0.4f);
                    SetPostFXGrain(&postfx, 0.8f - state_time * 0.5f);
                }
                // The cleanest image in the game — no grain, no CA, pure
                if (state_time > 1.0f) {
                    SetPostFXGrain(&postfx, 0.0f);     // zero grain — purity
                    SetPostFXCA(&postfx, 0.0f);        // zero CA — perfect clarity
                    SetPostFXVignette(&postfx, 0.5f);  // minimal vignette — open
                    SetPostFXContrast(&postfx, 0.9f);  // slightly soft — surrendered
                }
                if (IsKeyPressed(KEY_ESCAPE)) { CloseWindow(); return 0; }
                // Waking up — warmth spikes, exposure blooms
                if (state_time > 13.0f) {
                    float wake = (state_time - 13.0f) / 2.0f;  // 0→1 over 2s
                    SetPostFXWarmth(&postfx, wake * 0.5f);
                    set_exposure(wake * 0.3f);
                }
                if (state_time > 15.0f || (state_time > 8.0f && IsKeyPressed(KEY_ENTER))) {
                    SetMasterVolume(1.0f);
                    SetPostFXWarmth(&postfx, 0.0f);
                    // ── THE ENDING — Thirty Flights of Loving ──
                    // Rapid cuts. Time collapses. Characters reappear with context.
                    compose_ending_montage(&montage);
                    montage_start(&montage);
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
                    SetSoundVolume(audio.elevator_ding, 0.5f);
                    SetSoundPan(audio.elevator_ding, 0.5f);
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
                    // Very close to window: the image sharpens — clarity in Earth's presence
                    if (pz < -5.0f) {
                        float close_t = fminf(1.0f, (-5.0f - pz) / 2.5f);
                        // CA drops — the image becomes razor sharp
                        SetPostFXCA(&postfx, 2.5f - close_t * 1.5f);
                        // Vignette softens — peripheral vision opens
                        SetPostFXVignette(&postfx, 1.2f - close_t * 0.4f);
                        // FOV narrows to 55 — intimate, focused on Earth
                        player.fov_current += (55.0f - player.fov_current) * close_t * 0.03f;
                        // Bloom lifts — Earth's light fills your eyes
                        set_exposure(-0.05f + close_t * 0.15f);
                    }
                    // Earth glow pulse — the planet breathes light into the room
                    {
                        float pulse = 0.8f + 0.2f * sinf(state_time * 0.5f);
                        SetPointLightIdx(&lighting, 0,
                            0.0f, 0.5f, -7.0f,
                            0.24f * pulse, 0.51f * pulse, 0.78f * pulse,
                            12.0f * pulse);
                        // Chandelier micro-flicker — warm light breathes independently
                        float ch_flk = 0.92f + 0.08f * sinf(state_time * 3.1f) * sinf(state_time * 5.7f);
                        SetPointLightIdx(&lighting, 3,
                            0, 8.0f, -3.0f,
                            0.7f * ch_flk, 0.55f * ch_flk, 0.35f * ch_flk, 10.0f);
                        // Earth sub-bass: louder near window (z < -4), silent far away
                        float window_dist = fabsf(pz + 7.0f);  // window at z=-7
                        float sub_vol = (window_dist < 5.0f)
                            ? 0.08f * (1.0f - window_dist / 5.0f) : 0.0f;
                        SetEarthPresenceVolume(&audio, sub_vol * pulse);
                    }
                    // Gibbons's glance — he looks behind you. Once. Then never again.
                    // He was expecting two. He sees one.
                    if (!gibbons_glanced && state_time > 1.0f && state_time < 1.5f && gibbons.active) {
                        // Look behind the player — toward the elevator where the second person would be
                        float behind_yaw = atan2f(
                            player.camera.position.z + 5.0f - gibbons.pos.z,  // behind the player
                            player.camera.position.x - gibbons.pos.x);
                        gibbons.yaw = behind_yaw;
                        gibbons.yaw_target = behind_yaw;
                    }
                    if (!gibbons_glanced && state_time > 1.5f) {
                        gibbons_glanced = true;
                        // Snap back to facing the player — composure restored
                        // From here on, he never looks back again
                    }
                    // Looks at window when player approaches
                    {
                        static bool gibbons_looked_window = false;
                        if (pz < -3.0f && !gibbons_looked_window && gibbons.active) {
                            gibbons.yaw = atan2f(-8.0f - gibbons.pos.z,
                                                  0.0f - gibbons.pos.x);
                            gibbons_looked_window = true;
                        }
                    }
                }
                // First arrival — one line of wonder, then silence
                {
                    static bool first_lobby_text_shown = false;
                    static bool first_lobby_text_hidden = false;
                    if (lobby_visit_count == 1 && !first_lobby_text_shown && state_time > 2.0f) {
                        show_text("Your room is ready.");
                        first_lobby_text_shown = true;
                    }
                    if (first_lobby_text_shown && !first_lobby_text_hidden && state_time > 5.0f) {
                        hide_text();
                        first_lobby_text_hidden = true;
                    }
                }
                // ── Ambient life: distant elevator dings ──
                // Other floors. Other guests. The hotel is populated by absence.
                {
                    lobby_ding_timer -= dt;
                    if (lobby_ding_timer <= 0) {
                        // Play ding at low volume with random pan (other elevators, other floors)
                        float vol = 0.04f + (float)(rand() % 30) / 1000.0f;  // 0.04-0.07
                        float pan = 0.2f + (float)(rand() % 60) / 100.0f;    // 0.2-0.8
                        SetSoundVolume(audio.elevator_ding, vol);
                        SetSoundPan(audio.elevator_ding, pan);
                        PlaySound(audio.elevator_ding);
                        // Next ding in 8-15 seconds
                        lobby_ding_interval = 8.0f + (float)(rand() % 70) / 10.0f;
                        lobby_ding_timer = lobby_ding_interval;
                    }
                    // Comms chatter pan drifts slowly — the caller is walking
                    float chatter_pan = 0.5f + 0.2f * sinf(state_time * 0.15f);
                    SetSoundPan(audio.snd_comms_chatter, chatter_pan);
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
                                if (strcmp(obj->name, "newspaper") == 0) {
                                    PlayInteract(&audio, INTERACT_FABRIC);
                                    show_text("SKY TOWER DEPARTURES RESUME AFTER 3-HOUR DELAY");
                                } else if (strcmp(obj->name, "bell") == 0) {
                                    PlayElevatorDing(&audio);
                                    kick_camera(&player, -0.01f, 0.005f);
                                } else {
                                    PlayInteract(&audio, INTERACT_CLICK);
                                    kick_camera(&player, -0.01f, 0.005f);
                                }
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

            case STATE_SPACE_HOTEL:
                update_player(&player, &scene, dt);
                UpdateEVAudio(&audio, player.moving, player.sprinting, scene.surface, dt);
                update_npc(&gibbons, player.camera.position, &scene, dt);
                // Gibbons behavior at waypoints
                if (gibbons.waiting) {
                    if (gibbons.current_waypoint == 1) {
                        // At the piano — he looks at it
                        gibbons.behavior = NPC_GAZING;
                        gibbons.yaw_target = -1.5708f;  // face the piano
                    } else if (gibbons.current_waypoint == 3) {
                        // At corridor door — gesture through
                        gibbons.behavior = NPC_GESTURING;
                    }
                }
                // Earth glow pulse — the planet breathes through the glass walls
                {
                    float pulse = 0.8f + 0.2f * sinf(state_time * 0.4f);
                    SetPointLightIdx(&lighting, 2,
                        0.0f, 0.5f, 18.0f,
                        0.3f * pulse, 0.55f * pulse, 0.85f * pulse,
                        25.0f * pulse);
                    // Earth sub-bass volume — louder near glass walls
                    float wall_dist = fminf(
                        fabsf(player.camera.position.x + 20.0f),
                        fabsf(player.camera.position.x - 20.0f));
                    float sub_vol = (wall_dist < 8.0f)
                        ? 0.06f * (1.0f - wall_dist / 8.0f) : 0.0f;
                    SetEarthPresenceVolume(&audio, sub_vol * pulse);
                    // Chandelier micro-flicker
                    float ch_flk = 0.92f + 0.08f * sinf(state_time * 3.1f) * sinf(state_time * 5.7f);
                    SetPointLightIdx(&lighting, 0,
                        -8, 18, 10,
                        1.2f * ch_flk, 0.90f * ch_flk, 0.55f * ch_flk, 20.0f);
                    SetPointLightIdx(&lighting, 1,
                        8, 18, 24,
                        1.1f * ch_flk, 0.85f * ch_flk, 0.50f * ch_flk, 20.0f);
                }
                // ── Ambient life: distant elevator dings from other floors ──
                {
                    lobby_ding_timer -= dt;
                    if (lobby_ding_timer <= 0) {
                        float vol = 0.03f + (float)(rand() % 20) / 1000.0f;
                        float pan = 0.15f + (float)(rand() % 70) / 100.0f;
                        SetSoundVolume(audio.elevator_ding, vol);
                        SetSoundPan(audio.elevator_ding, pan);
                        PlaySound(audio.elevator_ding);
                        lobby_ding_timer = 10.0f + (float)(rand() % 100) / 10.0f;  // 10-20s
                    }
                }
                // ── Speed modulation — awe near glass, cathedral sprint in center ──
                {
                    float px = player.camera.position.x;
                    float edge_dist = fminf(fabsf(px + 20.0f), fabsf(px - 20.0f));
                    if (edge_dist < 5.0f) {
                        // Near glass walls: slow, awe, void exposure
                        float t = 1.0f - edge_dist / 5.0f;
                        float slow = 1.0f - t * 0.4f;
                        player.vel.x *= slow;
                        player.vel.z *= slow;
                        player.fov_current += (62.0f - player.fov_current) * t * 0.03f;
                        SetPostFXGrain(&postfx, 0.35f + t * 0.3f);
                    } else if (player.sprinting && edge_dist > 10.0f) {
                        // Cathedral sprint — the atrium center rewards running.
                        // Gravity Bone energy: the space is enormous, you're tiny,
                        // sprinting through it makes the scale hit harder.
                        float center_t = fminf(1.0f, (edge_dist - 10.0f) / 5.0f);
                        player.fov_current += (88.0f - player.fov_current) * center_t * 0.05f;
                        SetPostFXGrain(&postfx, fmaxf(0.15f, 0.35f - center_t * 0.2f));
                    }
                }
                // ── Piano warm pool proximity ──
                {
                    float pdx = player.camera.position.x + 2.0f;
                    float pdz = player.camera.position.z - 12.0f;
                    float piano_dist = sqrtf(pdx*pdx + pdz*pdz);
                    if (piano_dist < 6.0f) {
                        float t = 1.0f - piano_dist / 6.0f;
                        SetPostFXWarmth(&postfx, t * 0.3f);
                        // Piano spotlight intensifies
                        SetPointLightIdx(&lighting, 3,
                            -2, 4, 12,
                            (1.0f + t * 0.5f) * 0.75f, (1.0f + t * 0.3f) * 0.55f, 0.40f * (1.0f - t * 0.3f),
                            8.0f + t * 4.0f);
                    } else {
                        SetPostFXWarmth(&postfx, 0.0f);
                    }
                }
                // ── Comms chatter — someone on a phone, drifting through the atrium ──
                if (!IsSoundPlaying(audio.snd_comms_chatter)) {
                    PlayCommsChatter(&audio);
                    SetSoundVolume(audio.snd_comms_chatter, 0.005f);  // whisper
                }
                {
                // Piano interaction — one note. It resonates through the entire atrium.
                // "Someone left this here. It doesn't play itself." (Gibbons)
                if (IsKeyPressed(KEY_E)) {
                    for (int i = 0; i < scene.object_count; i++) {
                        InteractObject *obj = &scene.objects[i];
                        if (!obj->active || obj->done) continue;
                        float dist = Vector3Distance(player.camera.position, obj->pos);
                        if (dist < obj->radius) {
                            Vector3 to = Vector3Normalize(Vector3Subtract(obj->pos, player.camera.position));
                            Vector3 look = Vector3Normalize(Vector3Subtract(player.camera.target, player.camera.position));
                            if (Vector3DotProduct(to, look) > 0.5f) {
                                if (strcmp(obj->name, "piano") == 0) {
                                    obj->step++; obj->done = true;
                                    PlayMusicFragment(&audio);
                                    kick_camera(&player, -0.01f, 0.005f);
                                    add_wall(&scene, -2.2f, 0.88f, 11.18f, 0.12f, 0.04f, 0.18f,
                                             (Color){235, 232, 225, 255});
                                    break;
                                }
                            }
                        }
                    }
                }
                    float chatter_pan = 0.5f + 0.3f * sinf(state_time * 0.1f);
                    SetSoundPan(audio.snd_comms_chatter, chatter_pan);
                }
                // ── Couples' murmur — louder near the seating clusters ──
                // The hotel is full of people living the trip you were supposed to have.
                {
                    float pz_h = player.camera.position.z;
                    float px_h = player.camera.position.x;
                    // Cluster 1: chairs at (-6, 0, 3), Cluster 2: sofa at (-8, 0, 14)
                    float d1 = sqrtf((px_h+6)*(px_h+6) + (pz_h-3)*(pz_h-3));
                    float d2 = sqrtf((px_h+8)*(px_h+8) + (pz_h-14)*(pz_h-14));
                    float near = fminf(d1, d2);
                    float voices_vol = (near < 8.0f)
                        ? 0.015f + 0.025f * (1.0f - near / 8.0f)
                        : 0.015f;
                    SetSoundVolume(audio.snd_distant_voices, voices_vol);
                }
                // Exit — corridor door at far end
                if (scene.has_exit) {
                    float dist = Vector3Distance(player.camera.position, scene.exit_pos);
                    if (dist < 2.5f) {
                        transition_to(STATE_SPACE_CORRIDOR);
                    }
                }
                break;

            case STATE_SPACE_CORRIDOR:
                update_player(&player, &scene, dt);
                UpdateEVAudio(&audio, player.moving, player.sprinting, scene.surface, dt);
                update_npc(&gibbons, player.camera.position, &scene, dt);
                // Gibbons behavior — context at specific waypoints
                if (gibbons.waiting) {
                    if (gibbons.current_waypoint == 1) {
                        // Mid-corridor: gazes out the window at Earth
                        gibbons.behavior = NPC_GAZING;
                        gibbons.yaw_target = 1.5708f;  // face the window (90°)
                    } else if (gibbons.current_waypoint == 2) {
                        // Last waypoint: gesture toward suite door
                        gibbons.behavior = NPC_GESTURING;
                    }
                }
                // Per-position speed modulation — slow near windows (Earth views)
                {
                    float px = fabsf(player.camera.position.x);
                    if (px > 1.5f) {
                        float wt = fminf(1.0f, (px - 1.5f) / 1.0f);
                        float slow = 1.0f - wt * 0.35f;
                        player.vel.x *= slow;
                        player.vel.z *= slow;
                    }
                }
                // ── Spatial impossibility — Commandment 7 ──
                // The corridor is longer going FORWARD, shorter coming BACK.
                // Progression escalates in ambiguity. You can't trust distance.
                {
                    Vector3 fwd = Vector3Normalize(Vector3Subtract(player.camera.target, player.camera.position));
                    float forward_component = fwd.z;
                    if (forward_component > 0.3f) {
                        // Walking toward suite: 10% slower — the corridor stretches
                        player.vel.z *= 0.9f;
                    } else if (forward_component < -0.3f) {
                        // Walking back: 5% faster — it lets you go
                        player.vel.z *= 1.05f;
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
                            if (di == 0) {
                                SetSoundPan(audio.snd_muffled_machinery, pan);
                                SetSoundPan(audio.snd_running_water, pan);
                            } else {
                                SetSoundPan(audio.snd_tv_murmur, pan);
                            }
                        }

                        if (di == 0) {
                            SetSoundVolume(audio.snd_muffled_machinery, dvol);
                            // Muffled laughter — couple behind this door
                            // Louder than machinery, warmer — these are people, not systems
                            float laugh_vol = (ddist < 5.0f) ? 0.04f * (1.0f - ddist / 5.0f) : 0.0f;
                            laugh_vol *= silence;
                            SetSoundVolume(audio.snd_muffled_laughter, laugh_vol);
                            SetSoundPan(audio.snd_muffled_laughter, (di == 0 && ddist > 0.01f) ?
                                0.5f + (fwd.x * to_door.z - fwd.z * to_door.x) * 0.4f : 0.5f);
                            // Running water — bathroom behind this door
                            // Fades in as you approach, someone's home
                            float water_vol = (ddist < 4.0f) ? 0.04f * (1.0f - ddist / 4.0f) : 0.0f;
                            SetSoundVolume(audio.snd_running_water, water_vol);
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
                            // Warm light leaks under the door — someone's home
                            float warm_t = (ddist < 3.0f) ? (1.0f - ddist / 3.0f) : 0.0f;
                            SetPointLightIdx(&lighting, 2,
                                door_positions[0].x, 0.05f, door_positions[0].z,
                                (0.94f * light_fade + 0.6f * warm_t) * breath,
                                (0.82f * light_fade + 0.45f * warm_t) * breath,
                                (0.47f * light_fade + 0.2f * warm_t) * breath,
                                4.0f * light_fade + 3.0f * warm_t);
                        } else {
                            // Through-wall silence moment: listen at door 1 for 6s → TV goes quiet
                            if (!door_1_silenced) {
                                Vector3 fwd2 = Vector3Normalize(Vector3Subtract(player.camera.target, player.camera.position));
                                Vector3 to_d1 = Vector3Normalize((Vector3){-ddx, 0, -ddz});
                                float facing = Vector3DotProduct(fwd2, to_d1);
                                if (ddist < 2.0f && facing > 0.6f) {
                                    door_listen_timer += dt;
                                    if (door_listen_timer > 6.0f) door_1_silenced = true;
                                } else {
                                    door_listen_timer *= 0.95f;  // slow decay when you look away
                                }
                                SetSoundVolume(audio.snd_tv_murmur, dvol);
                            } else {
                                // Total silence. The blue light is off. They know you're there.
                                // Commandment 6: inaccessible spaces communicate through absence
                                SetSoundVolume(audio.snd_tv_murmur, 0);
                                SetPointLightIdx(&lighting, 3,
                                    door_positions[1].x, 0.05f, door_positions[1].z,
                                    0, 0, 0, 0);
                            }
                            // TV flicker — blue light stutters behind door 1
                            // Only active when NOT silenced — once they notice, everything cuts
                            if (!door_1_silenced) {
                                float flk = 0.6f + 0.4f * sinf(state_time * 8.3f)
                                           * sinf(state_time * 13.7f);
                                SetPointLightIdx(&lighting, 3,
                                    door_positions[1].x, 0.05f, door_positions[1].z,
                                    0.31f * flk, 0.47f * flk, 0.78f * flk, 3.0f * flk);
                            }
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
                        // The void seeps through — ambient pitch drops near the dark door
                        // The absence has weight. Something is MISSING behind this door.
                        float pitch_drop = 1.0f - dark_t * 0.15f;
                        SetSoundPitch(audio.drone_space_corridor, pitch_drop);
                    } else {
                        // Restore normal pitch when away from the dark door
                        SetSoundPitch(audio.drone_space_corridor, 1.0f);
                    }
                }
                // Floating newspaper — zero-g headline about the station
                if (IsKeyPressed(KEY_E)) {
                    for (int i = 0; i < scene.object_count; i++) {
                        InteractObject *obj = &scene.objects[i];
                        if (!obj->active || obj->done) continue;
                        float edist = Vector3Distance(player.camera.position, obj->pos);
                        if (edist < obj->radius) {
                            Vector3 eto = Vector3Normalize(Vector3Subtract(obj->pos, player.camera.position));
                            Vector3 elook = Vector3Normalize(Vector3Subtract(player.camera.target, player.camera.position));
                            if (Vector3DotProduct(eto, elook) > 0.5f) {
                                if (strcmp(obj->name, "newspaper") == 0) {
                                    obj->step++; obj->done = true;
                                    PlayInteract(&audio, INTERACT_FABRIC);
                                    show_text("CHECKOUT: 5 AM. NO EXCEPTIONS.");
                                    break;
                                }
                            }
                        }
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
                // Paris dream countdown — triggered by bed ritual
                if (paris_dream_timer > 0) {
                    paris_dream_timer -= dt;
                    // Grain and CA spike as the crack opens
                    float crack_t = 1.0f - (paris_dream_timer / 5.0f);
                    SetPostFXGrain(&postfx, 0.35f + crack_t * 0.5f);
                    SetPostFXCA(&postfx, 2.5f + crack_t * 4.0f);
                    if (paris_dream_timer <= 0) {
                        paris_dream_timer = -1;
                        hard_cut_to(STATE_PARIS_DREAM);
                    }
                }
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
                    // Near bed (z < -3): slow to 70%, agency softens
                    if (pz < -3.0f) {
                        float t = fminf(1.0f, (-3.0f - pz) / 2.5f);
                        float bed_mult = 1.0f - t * 0.3f;
                        if (bed_mult < speed_mult) speed_mult = bed_mult;
                        // Agency surrender — mouse sensitivity fades near bed.
                        // The room teaches you to be still.
                        player.control_mult = 1.0f - t * 0.4f;  // 1.0 at z=-3, 0.6 at bed
                    } else {
                        player.control_mult = 1.0f;
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
                // Photograph text auto-hide — "Paris." fades after 2 seconds
                if (photo_text_timer > 0) {
                    photo_text_timer -= dt;
                    if (photo_text_timer <= 0) {
                        hide_text();
                    }
                }
                UpdateEVAudio(&audio, player.moving, player.sprinting, scene.surface, dt);
                update_npc(&gibbons, player.camera.position, &scene, dt);
                // Gibbons vanishes after delivering his last line
                // "He's already gone when you turn around"
                if (!gibbons.active && gibbons.current_line >= gibbons.line_count) {
                    // Silently deactivated — he was never really there
                    gibbons.pos.y = -100;  // move off-screen so shadow doesn't linger
                }
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
                    if (interaction_timers[3] <= 0) {
                        interaction_phases[3] = 2;
                        // ── Phosphorescent ceiling stars ──
                        // Glow-in-the-dark dots on the ceiling above the bed.
                        // Someone put them there. A previous guest. A child. You.
                        // They appear when you lie down because now you're looking up.
                        Color star_glow = {140, 220, 180, 200};  // phosphorescent green-white
                        // Big dipper — recognizable even at 960x600
                        add_wall(&scene, -1.2f, 4.92f, -5.0f, 0.08f, 0.01f, 0.08f, star_glow);
                        add_wall(&scene, -0.7f, 4.92f, -4.7f, 0.06f, 0.01f, 0.06f, star_glow);
                        add_wall(&scene,  0.0f, 4.92f, -4.6f, 0.07f, 0.01f, 0.07f, star_glow);
                        add_wall(&scene,  0.6f, 4.92f, -4.8f, 0.06f, 0.01f, 0.06f, star_glow);
                        // Handle
                        add_wall(&scene,  1.0f, 4.92f, -5.2f, 0.05f, 0.01f, 0.05f, star_glow);
                        add_wall(&scene,  1.4f, 4.92f, -5.5f, 0.06f, 0.01f, 0.06f, star_glow);
                        add_wall(&scene,  1.1f, 4.92f, -5.8f, 0.05f, 0.01f, 0.05f, star_glow);
                        // Scattered others — asymmetric, placed by hand
                        add_wall(&scene, -1.8f, 4.92f, -3.8f, 0.04f, 0.01f, 0.04f, star_glow);
                        add_wall(&scene,  2.0f, 4.92f, -4.2f, 0.05f, 0.01f, 0.05f, star_glow);
                        add_wall(&scene, -0.3f, 4.92f, -3.5f, 0.04f, 0.01f, 0.04f, star_glow);
                        add_wall(&scene,  1.8f, 4.92f, -5.6f, 0.03f, 0.01f, 0.03f, star_glow);
                        add_wall(&scene, -2.2f, 4.92f, -4.5f, 0.03f, 0.01f, 0.03f, star_glow);
                    }
                }
                // ── Ceiling stars fade-in + camera drift ──
                // Lying down: controls go soft, camera drifts, stars glow
                if (interaction_phases[3] == 2) {
                    interaction_timers[3] += dt;  // count UP from 0 — time since lying down
                    // Phosphorescent glow — fades in over 3 seconds
                    float star_t = fminf(1.0f, interaction_timers[3] / 3.0f);
                    // Green-white glow point light on ceiling — the stars emit
                    float pulse = 0.7f + 0.3f * sinf(state_time * 0.8f);
                    SetPointLightIdx(&lighting, 3,
                        0, 4.9f, -4.8f,
                        star_t * 0.15f * pulse, star_t * 0.25f * pulse, star_t * 0.18f * pulse,
                        star_t * 6.0f);
                    // Camera drift — gentle, like falling asleep
                    float drift_x = sinf(state_time * 0.3f) * 0.002f;
                    float drift_z = cosf(state_time * 0.4f) * 0.001f;
                    player.camera.position.x += drift_x;
                    player.camera.position.z += drift_z;
                    // Agency softens — you're letting go
                    float soft = fmaxf(0.1f, 1.0f - star_t * 0.7f);
                    player.control_mult = soft;
                }
                // ── Micro-animations: completed objects stay alive ──
                // Lamp flicker — barely perceptible, the bulb breathes
                if (interaction_phases[0] == 2) {
                    float flk = 0.95f + 0.05f * sinf(state_time * 7.3f) * sinf(state_time * 11.1f);
                    SetPointLightIdx(&lighting, 0, -2.5f, 1.2f, -4.8f,
                                     flk, flk*0.82f, flk*0.45f, 8.0f);
                }
                // Champagne glow — liquid catches shifting light
                if (interaction_phases[1] == 2) {
                    float glow = 0.5f + 0.1f * sinf(state_time * 1.2f);
                    SetPointLightIdx(&lighting, 2, -3.1f, 0.42f, 3.5f,
                                     glow*0.6f, glow*0.5f, glow*0.15f, 3.0f);
                }
                // P3: Bath ritual — water fills over 1.8s (visual via exposure shift)
                if (interaction_phases[4] == 1) {
                    interaction_timers[4] -= dt;
                    float bth = fminf(1.0f, 1.0f - interaction_timers[4] / 1.8f);
                    // Subtle exposure lift as steam begins to fill — the room softens
                    set_exposure(bth * 0.04f);
                    if (interaction_timers[4] <= 0) {
                        interaction_phases[4] = 2;
                        // Water surface — bright, filled
                        add_wall(&scene, -5.0f, 0.30f, -1.5f,
                                 1.5f, 0.01f, 0.4f, (Color){200,220,240,120});
                        // Steam — translucent cubes rising from the water
                        add_wall(&scene, -4.8f, 0.8f, -1.4f, 0.3f, 0.4f, 0.3f, (Color){240,240,245,30});
                        add_wall(&scene, -5.2f, 1.0f, -1.6f, 0.25f, 0.35f, 0.25f, (Color){240,240,245,20});
                        add_wall(&scene, -5.0f, 1.3f, -1.5f, 0.2f, 0.3f, 0.2f, (Color){240,240,245,15});
                        // Window steams up — glass near the tub gets fogged
                        add_wall(&scene, -6.8f, 1.5f, -1.5f, 1.5f, 2.0f, 0.01f, (Color){220,225,230,40});
                    }
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
                                // Micro-freeze + lean — a moment of attention
                                interact_freeze = 0.05f;
                                interact_lean = 0.5f;
                                interact_lean_vel = 0;

                                // Per-step visual feedback — world accumulates changes
                                if (strcmp(obj->name, "lamp") == 0 && obj->step == 1) {
                                    // P3: Start lamp glow ritual — filament visible
                                    add_wall(&scene, -2.5f, 1.0f, -4.65f, 0.15f, 0.25f, 0.15f,
                                             (Color){220,210,185,180});
                                    interaction_phases[0] = 1;
                                    interaction_timers[0] = 1.5f;
                                } else if (strcmp(obj->name, "desk") == 0 && obj->step == 1) {
                                    // Open the textbook — studying for a conversation
                                    // that never happened. Bolaño mundane-unhinged.
                                    add_wall(&scene, 5.3f, 0.45f, -1.6f, 0.4f, 0.04f, 0.3f,
                                             (Color){235,230,220,255});
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
                                } else if (strcmp(obj->name, "bath") == 0 && obj->step == 1) {
                                    // Step 1: Turn the taps — water sound begins
                                    // Running water glow on tub surface
                                    add_wall(&scene, -5.0f, 0.32f, -1.5f, 1.5f, 0.01f, 0.4f,
                                             (Color){200,220,240,80});
                                    // Start bath fill ritual — 1.8s
                                    interaction_phases[4] = 1;
                                    interaction_timers[4] = 1.8f;
                                } else if (strcmp(obj->name, "photograph") == 0 && obj->step == 1) {
                                    // Turn the photograph face-up. The two of you. Paris. A café.
                                    // She's laughing. You look like someone who doesn't know yet.
                                    kick_camera(&player, -0.03f, 0.01f);
                                    // Replace face-down photo with face-up — warm cream (the image side)
                                    for (int j = 0; j < scene.wall_count; j++) {
                                        if (fabsf(scene.walls[j].pos.x + 2.5f) < 0.2f &&
                                            fabsf(scene.walls[j].pos.z + 3.5f) < 0.2f &&
                                            scene.walls[j].pos.y > 0.6f && scene.walls[j].pos.y < 0.7f &&
                                            scene.walls[j].size.y < 0.05f) {
                                            scene.walls[j].color = (Color){230,218,195,255};
                                        }
                                    }
                                    // Tiny red accent — her dress, the café awning, something warm
                                    add_wall(&scene, -2.48f, 0.625f, -3.48f, 0.04f, 0.003f, 0.03f, (Color){190,50,45,200});
                                    set_last_decal(&scene);
                                    photograph_flipped = true;
                                    show_text("Paris.");
                                    photo_text_timer = 2.0f;
                                    // Done — but NOT a counted task. Observational.
                                    obj->done = true;
                                    break;
                                } else if (strcmp(obj->name, "thermostat") == 0 && obj->step == 1) {
                                    // Change the temperature. The compromise is over.
                                    PlayInteract(&audio, INTERACT_CLICK);
                                    kick_camera(&player, -0.005f, 0.003f);
                                    // Emissive display shifts from warm green to cool blue
                                    for (int j = 0; j < scene.wall_count; j++) {
                                        if (fabsf(scene.walls[j].pos.x - 5.5f) < 0.1f &&
                                            fabsf(scene.walls[j].pos.z - 4.48f) < 0.1f &&
                                            scene.walls[j].material == MAT_EMISSIVE) {
                                            scene.walls[j].color = (Color){80,140,220,200};
                                        }
                                    }
                                    obj->done = true;
                                    break;
                                } else if (strcmp(obj->name, "adjoining_door") == 0 && obj->step == 1 && !adjoining_door_opened) {
                                    // ── THE ADJOINING DOOR ──
                                    // A second room. Also booked. Also empty. Also prepared.
                                    // The hotel doesn't know the trip changed.
                                    adjoining_door_opened = true;
                                    PlayDoorSound(&audio);
                                    kick_camera(&player, -0.02f, 0.008f);
                                    // Remove the door panel
                                    if (adjoining_door_wall_idx >= 0 && adjoining_door_wall_idx < scene.wall_count)
                                        scene.walls[adjoining_door_wall_idx].active = false;
                                    {
                                        // ── THE SECOND SUITE — glimpsed, not entered ──
                                        float adx = 3.8f;
                                        float adz = -6.5f;
                                        // Floor
                                        add_wall(&scene, adx, -0.05f, adz-3, 8, 0.1f, 6, (Color){105,78,48,255});
                                        set_last_material(&scene, MAT_HERRINGBONE);
                                        // Back wall + cream panel
                                        add_wall(&scene, adx, 2.5f, adz-6, 8, 5, 0.3f, (Color){50,52,60,255});
                                        set_last_material(&scene, MAT_CONCRETE);
                                        add_wall(&scene, adx, 2.0f, adz-5.8f, 6, 4, 0.04f, (Color){242,238,230,255});
                                        set_last_material(&scene, MAT_WALLPAPER);
                                        // Side walls + ceiling
                                        add_wall(&scene, adx-4, 2.5f, adz-3, 0.3f, 5, 6, (Color){50,52,60,255});
                                        set_last_material(&scene, MAT_CONCRETE);
                                        add_wall(&scene, adx+4, 2.5f, adz-3, 0.3f, 5, 6, (Color){50,52,60,255});
                                        set_last_material(&scene, MAT_CONCRETE);
                                        add_wall(&scene, adx, 5.0f, adz-3, 8, 0.2f, 6, (Color){50,52,60,255});
                                        set_last_material(&scene, MAT_CONCRETE);
                                        // Bed — identical, untouched
                                        add_wall(&scene, adx, 0.2f, adz-4.5f, 3.4f, 0.4f, 2, (Color){105,78,48,255});
                                        set_last_material(&scene, MAT_WOOD);
                                        add_wall(&scene, adx, 0.5f, adz-4.5f, 3.2f, 0.25f, 1.8f, (Color){245,242,235,255});
                                        set_last_material(&scene, MAT_FABRIC);
                                        // Two pillows — smoothed, unwrinkled, waiting
                                        add_wall(&scene, adx-0.6f, 0.68f, adz-5.2f, 0.7f, 0.2f, 0.4f, (Color){245,242,235,255});
                                        set_last_material(&scene, MAT_FABRIC);
                                        add_wall(&scene, adx+0.6f, 0.68f, adz-5.2f, 0.7f, 0.2f, 0.4f, (Color){245,242,235,255});
                                        set_last_material(&scene, MAT_FABRIC);
                                        // Headboard — navy
                                        add_wall(&scene, adx, 1.8f, adz-5.5f, 3.6f, 2.8f, 0.12f, (Color){35,45,75,255});
                                        set_last_material(&scene, MAT_FABRIC);
                                        // Two robes — wrapped, on hooks. Nobody coming.
                                        add_wall(&scene, adx+3.5f, 1.8f, adz-1, 0.04f, 0.8f, 0.3f, (Color){245,242,235,255});
                                        set_last_material(&scene, MAT_FABRIC);
                                        add_wall(&scene, adx+3.5f, 1.7f, adz-0.4f, 0.04f, 0.7f, 0.25f, (Color){245,242,235,255});
                                        set_last_material(&scene, MAT_FABRIC);
                                        // Champagne on table — corked, two glasses, untouched
                                        add_wall(&scene, adx-2, 0.35f, adz-1.5f, 0.6f, 0.03f, 0.6f, (Color){180,155,90,255});
                                        set_last_material(&scene, MAT_BRASS);
                                        add_cylinder(&scene, adx-2, 0.5f, adz-1.5f, 0.06f, 0.25f, (Color){40,60,30,255});
                                        add_cone(&scene, adx-1.8f, 0.42f, adz-1.3f, 0.06f, 0.08f, (Color){210,210,215,200});
                                        add_cone(&scene, adx-2.2f, 0.42f, adz-1.7f, 0.06f, 0.08f, (Color){210,210,215,200});
                                        // Nightstands
                                        add_wall(&scene, adx-2.5f, 0.32f, adz-4.8f, 0.6f, 0.6f, 0.6f, (Color){105,78,48,255});
                                        set_last_material(&scene, MAT_WOOD);
                                        add_wall(&scene, adx+2.5f, 0.32f, adz-4.8f, 0.6f, 0.6f, 0.6f, (Color){105,78,48,255});
                                        set_last_material(&scene, MAT_WOOD);
                                        // Warm light — the room was turned down for arrival
                                        add_light_panel(&scene, adx, 0.8f, adz-3, 3, 0.5f, 3, (Color){255,235,180,40});
                                    }
                                    show_text("Also booked.");
                                    obj->done = true;
                                    break;
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

                                    // The composed piece — TBD (placeholder: lighthouse.wav)
                                    // Triggers ONLY on bed ritual completion.
                                    // The dream fires from the bed. You're lying down.
                                    // The music plays. The room cracks. Paris.
                                    if (strcmp(obj->name, "bed") == 0) {
                                        PlaySuiteMusic(&audio);
                                        paris_dream_timer = 5.0f;  // 5s of grain/CA spike, then hard cut to Paris
                                    }

                                    // Sprint 2B: Clock heartbeat deceleration
                                    // Each task: clock rate drops by ~0.2
                                    float new_rate = 1.0f - (float)tasks_done * 0.2f;
                                    if (new_rate < 0.1f) new_rate = 0.1f;
                                    SetClockRate(&audio, new_rate);

                                    // ── ACCUMULATING WRONGNESS (Barton Fink layer) ──
                                    // The room shifts imperceptibly after each ritual.
                                    // Not horror — ambiguity. You're not supposed to be here.
                                    if (tasks_done == 1 && !wrongness_photo) {
                                        wrongness_photo = true;
                                        // First wrongness: bathroom door drifts open 8cm
                                        // A crack of warm light where there shouldn't be
                                        add_wall(&scene, 6.88f, 1.3f, -3.0f, 0.04f, 2.5f, 0.9f,
                                                 (Color){210,207,202,255});
                                        set_last_material(&scene, MAT_WOOD);
                                        // Warm light leaks through the gap
                                        SetPointLightIdx(&lighting, 1, 6.95f, 1.0f, -3.0f,
                                                         0.4f, 0.32f, 0.15f, 2.5f);
                                        // The photograph appears — it wasn't there before. Face down.
                                        // A previous guest. Or a future one.
                                        add_wall(&scene, 3.0f, 0.52f, -3.5f, 0.15f, 0.005f, 0.1f,
                                                 (Color){240,235,225,255});
                                        // Face-down: slightly dark rectangle on top (the back of the photo)
                                        add_wall(&scene, 3.0f, 0.526f, -3.5f, 0.14f, 0.002f, 0.09f,
                                                 (Color){60,55,50,255});
                                    }
                                    if (tasks_done >= 2 && !wrongness_door) {
                                        wrongness_door = true;
                                        // Second wrongness: the adjoining room door appears.
                                        // A door that shouldn't be there — or was it always?
                                        // It leads to the second room. Also booked. Also empty.
                                        // The hotel doesn't know the trip changed.
                                        add_wall(&scene, 3.8f, 1.2f, -5.95f, 0.8f, 2.4f, 0.04f,
                                                 (Color){75,60,45,255});
                                        set_last_material(&scene, MAT_WOOD);
                                        adjoining_door_wall_idx = scene.wall_count - 1;
                                        // Door frame — brass, matching the hotel's style
                                        add_wall(&scene, 3.4f, 1.2f, -5.94f, 0.04f, 2.5f, 0.03f,
                                                 (Color){180,155,90,255});
                                        set_last_material(&scene, MAT_BRASS);
                                        add_wall(&scene, 4.2f, 1.2f, -5.94f, 0.04f, 2.5f, 0.03f,
                                                 (Color){180,155,90,255});
                                        set_last_material(&scene, MAT_BRASS);
                                        // Door handle — brass, at hand height
                                        add_wall(&scene, 3.55f, 1.0f, -5.92f, 0.06f, 0.06f, 0.03f,
                                                 (Color){200,170,100,255});
                                        set_last_material(&scene, MAT_BRASS);
                                        // Thin light seam under the door — the room beyond is lit.
                                        // Prepared. Waiting for someone who won't arrive.
                                        add_wall(&scene, 3.8f, 0.01f, -5.93f, 0.7f, 0.02f, 0.02f,
                                                 (Color){255,235,180,80});
                                        set_last_decal(&scene);
                                        // Interactive — optional. "It's worse if you do."
                                        add_object(&scene, 3.8f, 1.2f, -5.9f, "adjoining_door",
                                                   (Color){200,170,100,255}, 1);
                                    }
                                    if (tasks_done == 3 && !wrongness_earth) {
                                        wrongness_earth = true;
                                        // Third wrongness: Earth is in the wrong position
                                        // Through the suite window, it should be below-right
                                        // Now it's shifted left. Time passed while you weren't looking.
                                        // (Visual: cold blue light from wrong angle)
                                        SetPointLightIdx(&lighting, 1, -7.5f, 0.5f, 0.0f,
                                                         0.15f, 0.25f, 0.45f, 6.0f);
                                        // Room gets subtly darker — the light source changed angle
                                        set_exposure(-0.15f);
                                    }

                                    // Visible consequences — completion rewards
                                    if (strcmp(obj->name, "lamp") == 0) {
                                        // Lamp is on — warm golden glow
                                        add_light_panel(&scene, -2.5f, 1.2f, -4.8f,
                                                        0.5f, 0.6f, 0.5f, (Color){255,220,120,200});
                                        // Lamp casts warm shadow on wall behind
                                        add_wall(&scene, -2.5f, 1.8f, -5.35f,
                                                 1.2f, 1.5f, 0.01f, (Color){60,45,20,40});
                                    } else if (strcmp(obj->name, "desk") == 0) {
                                        // Geometry textbook spread on desk — Bolaño washing-line moment
                                        // Someone was studying orbital mechanics for a conversation
                                        // that never happened. Amalfitano's geometry book on the line.
                                        add_wall(&scene, 5.5f, 0.85f, -2, 1.8f, 0.01f, 0.8f,
                                                 (Color){245,240,230,255});
                                        set_last_material(&scene, MAT_FABRIC);
                                        // Diagrams — orbital trajectories, ellipses
                                        add_wall(&scene, 5.2f, 0.86f, -2.1f, 0.06f, 0.005f, 0.06f,
                                                 (Color){40,45,60,120});
                                        add_wall(&scene, 5.7f, 0.86f, -1.8f, 0.04f, 0.005f, 0.04f,
                                                 (Color){40,45,60,100});
                                        add_wall(&scene, 5.4f, 0.86f, -1.6f, 0.05f, 0.005f, 0.05f,
                                                 (Color){40,45,60,90});
                                        add_wall(&scene, 6.0f, 0.86f, -2.3f, 0.03f, 0.005f, 0.03f,
                                                 (Color){40,45,60,110});
                                        // Pencil underline — she was marking passages
                                        add_wall(&scene, 5.45f, 0.862f, -1.95f, 0.8f, 0.003f, 0.02f,
                                                 (Color){180,60,50,100});
                                        // Margin note — her handwriting, small red circle
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
                                    } else if (strcmp(obj->name, "bath") == 0) {
                                        // Bath full — steam rises, window fogs
                                        // Steam cubes drifting in zero-g above the water
                                        add_wall(&scene, -4.7f, 0.9f, -1.3f, 0.35f, 0.45f, 0.35f, (Color){240,240,245,25});
                                        add_wall(&scene, -5.3f, 1.2f, -1.7f, 0.3f, 0.4f, 0.3f, (Color){240,240,245,18});
                                        add_wall(&scene, -4.9f, 1.6f, -1.5f, 0.25f, 0.35f, 0.25f, (Color){240,240,245,12});
                                        add_wall(&scene, -5.1f, 2.0f, -1.4f, 0.2f, 0.3f, 0.2f, (Color){240,240,245,8});
                                        // Window steams up — the glass wall near the tub gets fogged
                                        add_wall(&scene, -6.8f, 1.5f, -1.5f, 1.5f, 2.0f, 0.01f, (Color){220,225,230,50});
                                        // Water glow — blue-green light from the filled tub
                                        add_light_panel(&scene, -5.0f, 0.35f, -1.5f,
                                                        1.4f, 0.1f, 0.5f, (Color){160,200,230,60});
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
                // Gibbons gestures at suite threshold (waypoint 0) — the bow
                if (gibbons.active && gibbons.waiting && gibbons.current_waypoint == 0) {
                    gibbons.behavior = NPC_GESTURING;
                    gibbons.yaw_target = PI;  // face the player
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
                    // ── THE REMOVAL (Commandment 4) ──
                    // The constant: muffled machinery / life support hum.
                    // You didn't notice it until it's gone. Now you notice.
                    // Silence after sound is devastating. (Wreden, Beginner's Guide)
                    if (done_pause > 1.5f && done_pause < 1.7f) {
                        StopMuffledMachinery(&audio);
                    }
                    // Ambient drone fades over 2 seconds — the room empties
                    if (done_pause > 2.0f && done_pause < 4.5f) {
                        float drain = (done_pause - 2.0f) / 2.5f;
                        float amb_vol = fmaxf(0.0f, 1.0f - drain);
                        SetSoundVolume(audio.drone_space_suite, amb_vol * 0.03f);
                    }
                    if (done_pause > 4.5f && done_pause < 4.7f) {
                        StopAmbient(&audio);
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
        if (state == STATE_SPACE_LOBBY || state == STATE_SPACE_HOTEL
            || state == STATE_SPACE_CORRIDOR || state == STATE_SPACE_SUITE) {
            const char *line = npc_current_dialogue(&gibbons);
            if (line && vig_text != line) {
                show_text(line);
            } else if (!line && vig_text && !gibbons.line_showing
                       && gibbons.current_line >= gibbons.line_count) {
                // Only hide text when ALL dialogue is exhausted, not between lines
                hide_text();
            }
        }

        }  // ── end game logic gate ──

        // ---- SHADOW PASS ----
        lighting.shadowPassRan = false;
        if (lighting.shadowReady && state != STATE_SPLASH && state != STATE_TITLE && state != STATE_BED && state != STATE_STARS) {
            draw_shadow_pass(&scene, &lighting,
                            &cube_model, &cyl_model, &sphere_model, &cone_model);
        }

        // ---- RENDER ----
        render_frame:  // goto target for freeze frame
        BeginTextureMode(render_target);
        ClearBackground(scene.fog_color.a > 0 ? scene.fog_color : (Color){8, 10, 22, 255});
        if (wireframe) rlEnableWireMode();

        // Bind shadow map ONLY if shadow pass actually wrote depth this frame
        if (lighting.shadowPassRan) BindShadowMap(&lighting);

        switch (state) {
            case STATE_SPLASH:
                draw_splash(state_time);
                break;

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
            case STATE_SPACE_HOTEL:
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
                           || state == STATE_SPACE_HOTEL || state == STATE_SPACE_CORRIDOR
                           || state == STATE_SPACE_SUITE) {
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
                        else if (state == STATE_SPACE_HOTEL) earth_pos = (Vector3){-15, -25, -35};  // visible through both glass arcs
                        else if (state == STATE_SPACE_CORRIDOR) earth_pos = (Vector3){-20, -30, -30};
                        else if (state == STATE_SPACE_SUITE) {
                            float prog = (float)tasks_done / SPACE_TASK_COUNT;
                            // After 3 tasks: Earth has moved. Time is wrong.
                            // The player may not notice. But the light is different.
                            float drift = wrongness_earth ? 8.0f : 0.0f;
                            earth_pos = (Vector3){-35+prog*5 - drift, -20+prog*3 + drift*0.3f, -10+prog*3};
                        }
                        else if (state == STATE_BALCONY) earth_pos = (Vector3){0, -30, -50};
                        // Commandment 7: Earth rotates faster when you're not looking
                        // The planet changes while your back is turned. Imperceptible.
                        {
                            Vector3 to_earth = Vector3Normalize(Vector3Subtract(earth_pos, player.camera.position));
                            Vector3 look = Vector3Normalize(Vector3Subtract(player.camera.target, player.camera.position));
                            float facing = Vector3DotProduct(to_earth, look);
                            // When facing Earth: normal time. When looking away: 2x rotation
                            float earth_time = state_time * (1.0f + (1.0f - fmaxf(0, facing)) * 1.0f);
                            draw_earth(player.camera, earth_time, &sphere_model, &lighting, earth_pos);
                        }
                    }
                } else {
                    ClearBackground(scene.fog_color);
                }
                {
                    bool indoor = !(state == STATE_HOTEL_EXT || state == STATE_BALCONY
                                    || state == STATE_SPACE_LOBBY || state == STATE_SPACE_HOTEL
                                    || state == STATE_ELEVATOR || state == STATE_HYPERSPACE);
                    draw_scene_3d(&player, &scene, &lighting, &cube_model, cube_model_loaded,
                                  &cyl_model, cyl_model_loaded,
                                  &sphere_model, sphere_model_loaded,
                                  &cone_model, cone_model_loaded,
                                  &skytower_model, skytower_loaded,
                                  indoor, state_time);
                }
                // Gibbons NPC — draw wherever he's active
                // Normal: lobby, hotel, corridor, suite, elevator (shared ride).
                // Montage: elevator, balcony, taxi (impossible).
                if (gibbons.active && (
                    state == STATE_LOBBY || state == STATE_SPACE_LOBBY
                    || state == STATE_SPACE_HOTEL
                    || state == STATE_SPACE_CORRIDOR || state == STATE_SPACE_SUITE
                    || (state == STATE_ELEVATOR && elevator_to_corridor)
                    || (montage.active && (state == STATE_ELEVATOR
                        || state == STATE_BALCONY
                        || state == STATE_RETURN_TAXI || state == STATE_CAR))
                )) {
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
            state == STATE_SPACE_LOBBY || state == STATE_SPACE_HOTEL ||
            state == STATE_SPACE_CORRIDOR || state == STATE_SPACE_SUITE ||
            state == STATE_HALLWAY) {
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

        // Godard intertitle — black card between scenes
        if (cinema.intertitle_active) {
            draw_intertitle(cinema.intertitle_text, cinema.intertitle_alpha,
                            cinema.intertitle_color, cinema.intertitle_fullscreen);
        }

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
                       state, rs/n, gs/n, bs/n, (double)fade_alpha);
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

        // Apply interaction lean to FOV (spring-based narrowing)
        if (interact_lean > 0.01f) {
            player.camera.fovy -= interact_lean * 8.0f;  // max ~4° narrowing
        }

        // Post-FX — feed player speed to shader for speed lines
        SetPostFXSpeed(&postfx, player_speed_normalized(&player));

        // Bloom downsample chain — wide soft light bleed from practicals
        process_bloom(&postfx, render_target);

        // Volumetric fog — light shafts through doorways and chandeliers
        // Indoor scenes get volumetric fog scaled by scene fog density
        {
            float vol_density = 0.0f;
            if (state == STATE_LOBBY || state == STATE_SPACE_LOBBY)
                vol_density = 0.8f;   // dramatic chandelier shafts
            else if (state == STATE_HALLWAY || state == STATE_SPACE_CORRIDOR)
                vol_density = 0.5f;   // corridor light pools
            else if (state == STATE_ROOM || state == STATE_SPACE_SUITE || state == STATE_PARIS_DREAM)
                vol_density = 0.6f;   // intimate lamp shafts
            else if (state == STATE_ELEVATOR)
                vol_density = 0.3f;   // subtle brass cage glow
            else if (state == STATE_BALCONY)
                vol_density = 0.4f;   // Earth glow through glass
            update_volumetric_fog(&postfx, &lighting, player.camera, vol_density);
        }

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
                "SPLASH", "TITLE", "CAR", "DRIVING", "EXTERIOR", "LOBBY",
                "ELEVATOR", "HALLWAY", "ROOM", "BATHROOM", "BALCONY",
                "BED", "STARS", "HYPERSPACE", "SP_LOBBY", "SP_HOTEL",
                "SP_CORRIDOR", "SP_SUITE", "PARIS_DREAM", "RETURN_TAXI"
            };
            const char *sn = (state >= 0 && state < 20) ? state_names[state] : "???";
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
    if (split_target_loaded) UnloadRenderTexture(split_target);
    CloseWindow();
    return 0;
#endif  // QA_MODE
}
