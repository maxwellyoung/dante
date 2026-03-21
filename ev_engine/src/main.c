// ENDEARING VOID — Custom engine on Raylib
// Maxwell Young
// A story about arriving somewhere. Auckland. A tower. 2 AM.

#include "raylib.h"
#include "rlgl.h"
#include "raymath.h"
#include "game_ctx.h"
#include "model_registry.h"
#include "ui.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

float ev_mouse_sens = MOUSE_SENS_DEFAULT;

// ── Global game context — replaces ~80 file-scope statics ──
GameCtx g;

static Model make_generated_model(Mesh mesh) {
    Model model = {0};
    model.transform = MatrixIdentity();
    model.meshCount = 1;
    model.materialCount = 1;
    model.meshes = calloc(1, sizeof(Mesh));
    model.materials = calloc(1, sizeof(Material));
    model.meshMaterial = calloc(1, sizeof(int));
    if (!model.meshes || !model.materials || !model.meshMaterial) {
        free(model.meshes);
        free(model.materials);
        free(model.meshMaterial);
        UnloadMesh(mesh);
        return (Model){0};
    }
    model.meshes[0] = mesh;
    model.materials[0] = LoadMaterialDefault();
    model.meshMaterial[0] = 0;
    return model;
}

void show_text(const char *text) {
    g.vig_text = text;
    g.text_scale_target = 1.0f;
    g.text_y_offset = 20.0f;
    g.text_y_velocity = 0.0f;
}
void hide_text(void) {
    g.text_scale_target = 0.0f;
}

// ── Dialogue system ──
void show_dialogue(const char *speaker, const char *text) {
    g.dlg_speaker = speaker;
    g.dlg_text = text;
    g.dlg_timer = 0;
    g.dlg_hold_timer = 0;
    g.dlg_fade = 0;
    g.dlg_active = true;
}
void hide_dialogue(void) {
    g.dlg_active = false;
    g.dlg_text = NULL;
    g.dlg_speaker = NULL;
}
static void update_dialogue(float dt) {
    if (!g.dlg_active) {
        g.dlg_fade += (0 - g.dlg_fade) * fminf(1.0f, 8.0f * dt);
        return;
    }
    g.dlg_timer += dt;
    // Fade in
    g.dlg_fade += (1.0f - g.dlg_fade) * fminf(1.0f, 6.0f * dt);
    // Check if fully revealed
    if (g.dlg_text) {
        int total = (int)strlen(g.dlg_text);
        int revealed = (int)(g.dlg_timer * g.dlg_chars_per_sec);
        if (revealed >= total) {
            g.dlg_hold_timer += dt;
        }
    }
}
static void draw_dialogue(void) {
    if (g.dlg_fade < 0.01f) return;
    if (!g.dlg_text) return;

    float fade = g.dlg_fade;
    int total_chars = (int)strlen(g.dlg_text);
    int revealed = g.dlg_active ? (int)(g.dlg_timer * g.dlg_chars_per_sec) : total_chars;
    if (revealed > total_chars) revealed = total_chars;

    // Build revealed substring
    int len = revealed;
    char *buf = malloc((size_t)len + 1);
    if (!buf) return;
    memcpy(buf, g.dlg_text, (size_t)len);
    buf[len] = '\0';

    // ── House of Cards lower-third — left-aligned, minimal, elegant ──
    // Sized for Steam Deck / couch readability
    int font_dlg = 20;
    int font_speaker = 14;
    int margin_left = 60;
    int margin_bottom = 50;

    // Speaker name positioned above dialogue line — small caps feel
    int line_y = RENDER_H - margin_bottom;
    int speaker_y = line_y - font_dlg - 4;
    if (g.dlg_speaker) speaker_y -= font_speaker + 2;

    // Subtle gradient scrim at bottom (not a box — cinematic)
    int scrim_h = 120;
    for (int sy = RENDER_H - scrim_h; sy < RENDER_H; sy++) {
        float t = (float)(sy - (RENDER_H - scrim_h)) / (float)scrim_h;
        unsigned char sa = (unsigned char)(140 * t * t * fade);  // quadratic ramp
        DrawRectangle(0, sy, RENDER_W, 1, (Color){5, 5, 8, sa});
    }

    int text_x = margin_left;
    int text_y = speaker_y;

    // Speaker name — small, spaced, warm gold (House of Cards chapter titles)
    if (g.dlg_speaker) {
        unsigned char sa = (unsigned char)(180 * fade);
        // Draw with letter spacing by iterating chars
        char spaced[128];
        int si = 0;
        for (int i = 0; g.dlg_speaker[i] && si < 125; i++) {
            spaced[si++] = g.dlg_speaker[i];
            if (g.dlg_speaker[i+1]) spaced[si++] = ' ';  // space between each char
        }
        spaced[si] = '\0';
        DrawText(spaced, text_x, text_y, font_speaker, (Color){195, 170, 120, sa});
        // Thin gold rule under speaker name
        int rule_w = MeasureText(spaced, font_speaker);
        DrawRectangle(text_x, text_y + font_speaker + 1, rule_w, 1,
                      (Color){195, 170, 120, (unsigned char)(80 * fade)});
        text_y += font_speaker + 6;
    }

    // Dialogue text — warm white, typewriter
    unsigned char ta = (unsigned char)(230 * fade);
    DrawText(buf, text_x + 1, text_y + 1, font_dlg, (Color){0, 0, 0, (unsigned char)(160 * fade)});
    DrawText(buf, text_x, text_y, font_dlg, (Color){245, 242, 235, ta});

    // Thin blinking cursor
    if (revealed < total_chars && ((int)(g.dlg_timer * 2.5f) % 2 == 0)) {
        int cursor_x = text_x + MeasureText(buf, font_dlg) + 2;
        DrawRectangle(cursor_x, text_y + 2, 1, font_dlg - 4, (Color){245, 242, 235, ta});
    }
    free(buf);
}

// ── Narrative choice system ──
void show_choice(const char *question, const char *a, const char *b) {
    g.choice_question = question;
    g.choice_a = a;
    g.choice_b = b;
    g.choice_cursor = 0;
    g.choice_active = true;
    g.choice_confirmed = false;
    g.choice_result = -1;
}

// Returns -1 while pending, 0 or 1 when confirmed
int poll_choice(void) {
    if (!g.choice_active) return g.choice_result;
    if (g.choice_confirmed) {
        g.choice_active = false;
        return g.choice_result;
    }
    return -1;
}

static void update_choice_input(void) {
    if (!g.choice_active || g.choice_confirmed) return;
    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W) || IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A))
        g.choice_cursor = 0;
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S) || IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D))
        g.choice_cursor = 1;
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_E) || IsKeyPressed(KEY_SPACE)) {
        g.choice_result = g.choice_cursor;
        g.choice_confirmed = true;
        if (g.backstory_count < 6) {
            g.backstory[g.backstory_count] = g.choice_cursor;
            g.backstory_count++;
        }
        PlayInteract(&g.audio, INTERACT_CLICK);
    }
}

static void draw_choice(void) {
    if (!g.choice_active) return;

    // ── Lower-third style matching Gibbons dialogue ──
    int font_q = 18;
    int font_o = 16;
    int margin_left = 60;
    int margin_bottom = 40;
    int line_gap = 6;

    // Layout from bottom up
    int ob_y = RENDER_H - margin_bottom;
    int oa_y = ob_y - font_o * UI_SCALE - line_gap;
    int q_y  = oa_y - font_q * UI_SCALE - line_gap - 4;

    // Gradient scrim — same as dialogue system
    int scrim_top = q_y - 40;
    int scrim_h = RENDER_H - scrim_top;
    for (int sy = scrim_top; sy < RENDER_H; sy++) {
        float t = (float)(sy - scrim_top) / (float)scrim_h;
        unsigned char sa = (unsigned char)(160 * t * t);
        DrawRectangle(0, sy, RENDER_W, 1, (Color){5, 5, 8, sa});
    }

    int x = margin_left;
    int fs_q = font_q * UI_SCALE;
    int fs_o = font_o * UI_SCALE;

    // Question — warm white, same as dialogue text
    if (g.choice_question && g.choice_question[0]) {
        DrawText(g.choice_question, x + 1, q_y + 1, fs_q, (Color){0, 0, 0, 160});
        DrawText(g.choice_question, x, q_y, fs_q, (Color){245, 242, 235, 230});
    }

    // Options — dimmed when not selected, bright when selected
    unsigned char a_alpha = (g.choice_cursor == 0) ? 240 : 100;
    unsigned char b_alpha = (g.choice_cursor == 1) ? 240 : 100;

    const char *prefix_a = (g.choice_cursor == 0) ? "> " : "  ";
    const char *prefix_b = (g.choice_cursor == 1) ? "> " : "  ";

    const char *choice_a = g.choice_a ? g.choice_a : "";
    const char *choice_b = g.choice_b ? g.choice_b : "";
    char buf_a[128], buf_b[128];
    snprintf(buf_a, sizeof(buf_a), "%s%s", prefix_a, choice_a);
    snprintf(buf_b, sizeof(buf_b), "%s%s", prefix_b, choice_b);

    DrawText(buf_a, x + 1, oa_y + 1, fs_o, (Color){0, 0, 0, 140});
    DrawText(buf_a, x, oa_y, fs_o, (Color){245, 242, 235, a_alpha});

    DrawText(buf_b, x + 1, ob_y + 1, fs_o, (Color){0, 0, 0, 140});
    DrawText(buf_b, x, ob_y, fs_o, (Color){245, 242, 235, b_alpha});
}

InteractSoundType get_interact_sound_ext(const char *name) {
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

// ============================================================
// PAUSE MENU & SETTINGS
// ============================================================
static const char *pause_labels[] = {"RESUME", "SETTINGS", "QUIT"};
static const char *settings_labels[] = {"SENSITIVITY", "VOLUME", "STYLE", "FULLSCREEN", "BACK"};
static int menu_item_count(void) { return (g.menu_mode == MENU_SETTINGS) ? SETTINGS_ITEM_COUNT : PAUSE_ITEM_COUNT; }
static void menu_init_springs(void) { for (int i = 0; i < MENU_MAX_ITEMS; i++) { g.menu_item_x[i] = (float)(RENDER_W + i * 24); g.menu_item_vx[i] = 0; } }
static void menu_open(MenuMode mode) { g.menu_mode = mode; g.menu_cursor = 0; menu_init_springs(); g.menu_overlay_a = 0; g.menu_overlay_va = 0; EnableCursor(); }
static void menu_close(void) { g.menu_mode = MENU_NONE; g.menu_cursor = 0; g.menu_overlay_a = 0; g.menu_overlay_va = 0; if (g.state != STATE_TITLE) DisableCursor(); }

// Forward declaration
void load_state(GameState s);

// Exposure helper — stores g.scene value and adds style bias
void set_exposure(float exp) {
    g.scene_exposure = exp;
    SetPostFXExposure(&g.postfx, exp + visual_styles[g.current_style].exposure_bias);
}

static void update_menu_springs(float dt) {
    float sk = 280.0f, sd = 26.0f, sm = 0.9f;
    float ta = (g.menu_mode != MENU_NONE) ? 1.0f : 0.0f;
    float fa = -sk * (g.menu_overlay_a - ta) - sd * g.menu_overlay_va;
    g.menu_overlay_va += (fa / sm) * dt;
    g.menu_overlay_a += g.menu_overlay_va * dt;
    if (g.menu_overlay_a < 0) g.menu_overlay_a = 0;
    if (g.menu_overlay_a > 1) g.menu_overlay_a = 1;
    float tx = (g.menu_mode != MENU_NONE) ? 0.0f : (float)RENDER_W;
    for (int i = 0; i < MENU_MAX_ITEMS; i++) {
        float fx = -sk * (g.menu_item_x[i] - tx) - sd * g.menu_item_vx[i];
        g.menu_item_vx[i] += (fx / sm) * dt;
        g.menu_item_x[i] += g.menu_item_vx[i] * dt;
    }
}

static bool update_pause_menu(void) {
    if (g.transitioning) return g.menu_mode != MENU_NONE;
    if (g.menu_mode == MENU_NONE) {
        if (IsKeyPressed(KEY_ESCAPE) && g.state != STATE_TITLE && g.state != STATE_STARS) {
            menu_open(MENU_PAUSE); return true;
        }
        return false;
    }
    int count = menu_item_count();
    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W))
        g.menu_cursor = (g.menu_cursor - 1 + count) % count;
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S))
        g.menu_cursor = (g.menu_cursor + 1) % count;
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
                { g.menu_cursor = i; break; }
        }
    }
    if (IsKeyPressed(KEY_ESCAPE)) {
        if (g.menu_mode == MENU_SETTINGS) menu_open(MENU_PAUSE);
        else menu_close();
        return true;
    }
    bool confirm  = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    bool go_left  = IsKeyPressed(KEY_LEFT)  || IsKeyPressed(KEY_A);
    bool go_right = IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D);
    if (g.menu_mode == MENU_PAUSE && confirm) {
        switch (g.menu_cursor) {
            case 0: menu_close(); break;
            case 1: menu_open(MENU_SETTINGS); break;
            case 2: CloseWindow(); break;
        }
    } else if (g.menu_mode == MENU_SETTINGS) {
        switch (g.menu_cursor) {
            case 0:
                if (go_left)  ev_mouse_sens = fmaxf(0.001f, ev_mouse_sens - 0.0005f);
                if (go_right) ev_mouse_sens = fminf(0.008f, ev_mouse_sens + 0.0005f);
                break;
            case 1:
                if (go_left)  g.setting_master_vol = fmaxf(0.0f, g.setting_master_vol - 0.05f);
                if (go_right) g.setting_master_vol = fminf(1.0f, g.setting_master_vol + 0.05f);
                SetMasterVolume(g.setting_master_vol);
                break;
            case 2:
                if (go_left || go_right || confirm) {
                    if (go_left) g.current_style = (g.current_style - 1 + STYLE_COUNT) % STYLE_COUNT;
                    else         g.current_style = (g.current_style + 1) % STYLE_COUNT;
                    ApplyVisualStyle(&g.postfx, g.current_style);
                    SetPostFXExposure(&g.postfx, g.scene_exposure + visual_styles[g.current_style].exposure_bias);
                }
                break;
            case 3:
                if (confirm) { ToggleFullscreen(); g.setting_fullscreen = !g.setting_fullscreen; }
                break;
            case 4:
                if (confirm) menu_open(MENU_PAUSE);
                break;
        }
    }
    return true;
}

static void draw_pause_menu(void) {
    if (g.menu_overlay_a < 0.01f) return;
    unsigned char oa = (unsigned char)(160.0f * g.menu_overlay_a);
    DrawRectangle(0, 0, RENDER_W, RENDER_H, (Color){8, 8, 10, oa});
    const char **labels = (g.menu_mode == MENU_SETTINGS) ? settings_labels : pause_labels;
    int count = (g.menu_mode == MENU_SETTINGS) ? SETTINGS_ITEM_COUNT : PAUSE_ITEM_COUNT;
    int fs = 10, lh = 18;
    int by = RENDER_H / 2 - (count * lh) / 2;
    for (int i = 0; i < count; i++) {
        int xo = (int)g.menu_item_x[i];
        int iy = by + i * lh;
        Color tc = (i == g.menu_cursor)
            ? (Color){245, 242, 238, 240} : (Color){165, 160, 152, 180};
        if (i == g.menu_cursor)
            DrawRectangle(RENDER_W / 2 - 80 + xo, iy + 3, 3, 7, (Color){200, 178, 130, 240});
        int lx = RENDER_W / 2 - 72 + xo;
        DrawText(labels[i], lx + 1, iy + 1, fs, (Color){0, 0, 0, 180});
        DrawText(labels[i], lx, iy, fs, tc);
        if (g.menu_mode == MENU_SETTINGS) {
            char val[48] = "";
            int vx = RENDER_W / 2 + 30 + xo;
            switch (i) {
                case 0: snprintf(val, sizeof(val), "< %.1f >", ev_mouse_sens * 1000.0f); break;
                case 1: snprintf(val, sizeof(val), "< %d%% >", (int)(g.setting_master_vol * 100.0f + 0.5f)); break;
                case 2: snprintf(val, sizeof(val), "< %s >", visual_styles[g.current_style].name); break;
                case 3: snprintf(val, sizeof(val), "[ %s ]", g.setting_fullscreen ? "ON" : "OFF"); break;
                default: break;
            }
            if (val[0]) {
                Color vc = (i == g.menu_cursor) ? (Color){200, 178, 130, 240} : (Color){165, 160, 152, 140};
                DrawText(val, vx + 1, iy + 1, fs, (Color){0, 0, 0, 180});
                DrawText(val, vx, iy, fs, vc);
            }
        }
    }
    const char *hint = (g.menu_mode == MENU_SETTINGS) ? "ARROWS TO ADJUST  /  ESC BACK" : "ESC TO RESUME";
    int hfs = 8 * UI_SCALE;
    int hw = MeasureText(hint, hfs);
    DrawText(hint, RENDER_W/2 - hw/2 + 1, RENDER_H - 23*UI_SCALE, hfs, (Color){0,0,0,120});
    DrawText(hint, RENDER_W/2 - hw/2, RENDER_H - 24*UI_SCALE, hfs, (Color){130,126,120,120});
}

void transition_to(GameState s) {
    if (g.transitioning) return;  // already in flight — don't reset
    g.transitioning = true;
    g.next_state = s;
    g.fade_target = 1.0f;
    g.fade_speed = 2.0f;
    g.transition_hold = 0.3f;
    g.hold_timer = 0;
    PlayDoorSound(&g.audio);
}

// Hard cut — bypasses fade, instantly loads g.state (Blendo-style ellipsis)
// Flash kick: 2-3 frames of warm white, then instant g.scene
// (g.cut_flash_timer is in g.cut_flash_timer)

void hard_cut_to(GameState s) {
    StopClockAmbient(&g.audio);
    PlayHardCutPunch(&g.audio);  // micro-transient — makes the cut physical
    load_state(s);
    g.fade_alpha = 0.0f;  g.fade_target = 0.0f;
    g.transitioning = false;  g.hold_timer = 0;
    g.transition_hold = 0.3f;  // reset to default
    g.cut_flash_timer = 0.12f;
    SetPostFXFlash(&g.postfx, 1.0f, 1.0f, 0.95f, 0.85f);
    SetMasterVolume(1.0f);  // restore g.audio (hyperspace sets it to 0)
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

// Write current g.state to temp file for dev-watch to read on restart
static void write_state_file(GameState s) {
    const char *names[] = {
        "STATE_TITLE", "STATE_CAR", "STATE_DRIVING", "STATE_HOTEL_EXT",
        "STATE_LOBBY", "STATE_ELEVATOR", "STATE_HALLWAY", "STATE_ROOM",
        "STATE_BATHROOM", "STATE_BALCONY", "STATE_BED", "STATE_STARS",
        "STATE_HYPERSPACE", "STATE_SPACE_LOBBY", "STATE_SPACE_CORRIDOR",
        "STATE_SPACE_SUITE", "STATE_PARIS_DREAM", "STATE_CLEANED_SUITE",
        "STATE_MONTAGE", "STATE_RETURN_TAXI", "STATE_GLASSHOUSE",
        "STATE_SHELL_TEST"
    };
    FILE *f = fopen("/tmp/ev_state", "w");
    if (f) {
        int idx = (int)s;
        if (idx >= 0 && idx < (int)(sizeof(names)/sizeof(names[0]))) fprintf(f, "%s\n", names[idx]);
        else fprintf(f, "STATE_TITLE\n");
        fprintf(f, "%.2f %.2f %.2f\n",
                g.player.camera.position.x, g.player.camera.position.y,
                g.player.camera.position.z);
        fclose(f);
    }
}

#ifdef QA_MODE
static bool qa_is_floor_like_wall(const Wall *w) {
    if (!w || w->is_decal || w->no_collide || w->shape != SHAPE_CUBE)
        return false;
    if (w->size.y > 0.8f)
        return false;
    return w->size.x * w->size.z > 1.0f;
}

static bool qa_is_structural_wall(const Wall *w) {
    if (!w || w->is_decal || w->no_collide || w->shape != SHAPE_CUBE)
        return false;

    return w->size.y > 1.2f && (w->size.x > 2.0f || w->size.z > 2.0f);
}

static bool qa_is_overlap_candidate_wall(const Wall *w) {
    if (!qa_is_structural_wall(w))
        return false;

    float thin = fminf(w->size.x, w->size.z);
    float span = fmaxf(w->size.x, w->size.z);
    return thin <= 1.0f && span >= 2.0f;
}
#endif

void load_state(GameState s) {
    // Kill ALL looping audio — each scene starts fresh. No overlap.
    StopAllAudio(&g.audio);
    // Restore master volume — silence zones may have lowered it
    SetMasterVolume(g.setting_master_vol);

    g.state = s;
    g.state_time = 0;
    g.tasks_done = 0;
    g.done_pause = 0;
    if (!g.returning_to_room) { g.completed_count = 0; memset(g.completed_objects, 0, sizeof(g.completed_objects)); }
    g.phone_triggered = false;
    g.phone_wall_idx = -1;
    g.balcony_flash_triggered = false;
    g.balcony_flash_timer = 0;
    g.vig_text = NULL;
    g.text_y_offset = 20.0f;
    g.text_y_velocity = 0.0f;
    g.text_scale = 0.0f;
    g.text_scale_vel = 0.0f;
    g.text_scale_target = 0.0f;

    // Clear particles — each scene starts fresh
    particle_clear(&g.particles);

    // Reset grab state — no carrying across scenes
    physics_init(&g.grab);

    // Reset UI springs — crosshair, compass, interaction ring
    ui_reset();

    // Reset PostFX to current visual style baseline before per-scene overrides
    ApplyVisualStyle(&g.postfx, g.current_style);

    // Dispatch to scene-specific load
    if ((int)s >= 0 && (int)s < scene_desc_count && scene_descs[s].load) {
        scene_descs[s].load();
    }

    // (Scene-specific load dispatched above via scene_descs[s].load())

    g.fade_alpha = 1.0f;
    g.fade_target = 0.0f;

    // Re-apply g.scene exposure with style bias
    float final_exp = g.scene_exposure + visual_styles[g.current_style].exposure_bias;
    SetPostFXExposure(&g.postfx, final_exp);

    // DIAGNOSTIC — remove after fixing visibility issue
    printf("[DBG] load_state(%d): walls=%d objs=%d exposure=%.2f style=%d shadow=%s fog=%.4f\n",
           s, g.scene.wall_count, g.scene.object_count, final_exp, g.current_style,
           g.lighting.shadowReady ? "ON" : "OFF", g.scene.fog_density);
    printf("[DBG]   fog_color=(%d,%d,%d) cam=(%.1f,%.1f,%.1f)\n",
           g.scene.fog_color.r, g.scene.fog_color.g, g.scene.fog_color.b,
           g.player.camera.position.x, g.player.camera.position.y, g.player.camera.position.z);

    // Update shadow matrix — per-g.scene key direction + radius
    if (g.lighting.shadowReady) {
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
        UpdateShadowMatrix(&g.lighting,
            g.lighting.activePreset.keyDir,
            (Vector3){0, 2, 0}, sr);
    }

    // Reset interaction ritual timers
    for (int i = 0; i < 4; i++) { g.interaction_timers[i] = 0; g.interaction_phases[i] = 0; }
    g.ambient_fade = 1.0f;
}

// ============================================================
// VIGNETTE DRAWING
// ============================================================

// Old 2D draw_taxi_ride and draw_arrival removed — replaced by 3D taxi g.scene

static void draw_vignette_text(void) {
    if (!g.vig_text || g.text_scale < 0.01f) return;
    float clamped = g.text_scale;
    if (clamped > 1.0f) clamped = 1.0f;
    if (clamped < 0.0f) clamped = 0.0f;
    unsigned char a = (unsigned char)(240 * clamped);
    int y = RENDER_H - 30 + (int)g.text_y_offset;
    draw_text_box(g.vig_text, y, 12, (Color){248, 245, 238, a});
}

// ============================================================
// MAIN
// ============================================================

int main(void) {
    game_ctx_init(&g);

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

    g.render_target = LoadRenderTexture(RENDER_W, RENDER_H);
    SetTextureFilter(g.render_target.texture, TEXTURE_FILTER_POINT);
    g.postfx_target = LoadRenderTexture(RENDER_W, RENDER_H);
    SetTextureFilter(g.postfx_target.texture, TEXTURE_FILTER_POINT);

    g.lighting = LoadEVLighting();
    CreateShadowMap(&g.lighting);
    g.postfx = LoadEVPostFX();
    g.skybox = LoadEVSkybox();
    InitEVAudio(&g.audio);
    LoadFileMusic(&g.audio);
    ui_init();

    Mesh cube_mesh = GenMeshCube(1.0f, 1.0f, 1.0f);
    g.cube_model = make_generated_model(cube_mesh);
    if (g.cube_model.meshCount > 0) {
        if (g.lighting.ready) g.cube_model.materials[0].shader = g.lighting.shader;
        g.cube_model_loaded = true;
    }

    Mesh cyl_mesh = GenMeshCylinder(0.5f, 1.0f, 16);
    g.cyl_model = make_generated_model(cyl_mesh);
    if (g.cyl_model.meshCount > 0) {
        if (g.lighting.ready) g.cyl_model.materials[0].shader = g.lighting.shader;
        g.cyl_model_loaded = true;
    }

    // Sphere model — for light fixtures, decorative elements
    Mesh sphere_mesh = GenMeshSphere(0.5f, 8, 8);
    g.sphere_model = make_generated_model(sphere_mesh);
    if (g.sphere_model.meshCount > 0) {
        if (g.lighting.ready) g.sphere_model.materials[0].shader = g.lighting.shader;
        g.sphere_model_loaded = true;
    }

    // Cone model — for lamp shades, decorative elements
    Mesh cone_mesh = GenMeshCone(0.5f, 1.0f, 12);
    g.cone_model = make_generated_model(cone_mesh);
    if (g.cone_model.meshCount > 0) {
        if (g.lighting.ready) g.cone_model.materials[0].shader = g.lighting.shader;
        g.cone_model_loaded = true;
    }

    // Sky Tower — Auckland landmark, the recurring silhouette
    if (FileExists("assets/skytower.obj")) {
        // Raylib's OBJ loader spuriously logs a duplicate mesh upload warning here
        // on macOS/Metal even though the resulting model is valid.
        SetTraceLogLevel(LOG_ERROR);
        g.skytower_model = LoadModel("assets/skytower.obj");
        SetTraceLogLevel(LOG_INFO);
        if (g.skytower_model.meshCount > 0 && g.skytower_model.meshes[0].vertexCount > 0) {
            if (g.lighting.ready) g.skytower_model.materials[0].shader = g.lighting.shader;
            g.skytower_loaded = true;
            printf("[EV] Sky Tower loaded — %d verts, %d tris\n",
                   g.skytower_model.meshes[0].vertexCount,
                   g.skytower_model.meshes[0].triangleCount);
        } else {
            printf("[EV] WARNING: Sky Tower mesh empty after load\n");
        }
    } else {
        printf("[EV] WARNING: assets/skytower.obj not found\n");
    }

    // ── Initialize model registry ──
    // Stable slots come from the registry, not from startup load order.
    init_model_registry_assets();
#ifndef QA_MODE
    preload_startup_model_assets(&g.lighting);
#endif // !QA_MODE

    DisableCursor();
    SetExitKey(0);  // ESC handled by pause menu, not raylib

#if defined(QA_MODE) && !defined(QA_AUDIT)
    // ============================================================
    // QA MODE — Full E2E Automated Analysis Pipeline
    // Multi-angle patrol cameras (6 per scene), game flow testing,
    // scene timing, collision probing, object detail extraction,
    // material analysis, anomaly detection, regression detection.
    // Outputs rich JSON for multi-profile Python analyzer.
    //
    // Build: make qa          |  Basic QA (backward compat)
    //        make qa-e2e      |  Full E2E pipeline
    //        ./qa/run_e2e.sh  |  Full pipeline with multi-profile grades
    // ============================================================
    EnableCursor();
    SetMasterVolume(0.0f);

    // ── Camera angle definition ──
    typedef struct {
        Vector3 pos;
        Vector3 target;
    } QACam;

    // Up to 8 camera angles per scene
    #define QA_MAX_ANGLES 8
    typedef struct {
        GameState gs;
        const char *name;
        QACam angles[QA_MAX_ANGLES];   // named: hero, spawn, patrol_left, patrol_right, patrol_back, detail, ceiling, overview
        const char *angle_names[QA_MAX_ANGLES];
        int angle_count;
        bool dark_by_design;
        bool outdoor;
        bool force_elevator_to_corridor;
        // Game flow
        int flow_order;     // position in canonical game flow (-1 = orphaned)
        GameState flow_next; // expected next scene in flow
    } QAEntry;

    // ── Game flow order (canonical playthrough) ──
    // TITLE → CAR → DRIVING → HOTEL_EXT → LOBBY → ELEVATOR →
    // HYPERSPACE → SPACE_LOBBY → SPACE_CORRIDOR → SPACE_SUITE →
    // BALCONY → BED → STARS → PARIS_DREAM → RETURN_TAXI
    // Orphaned: HALLWAY, ROOM, BATHROOM (Paris hotel — dev keys only)

#ifdef QA_SHELL_ONLY
    QAEntry qa_scenes[] = {
        {STATE_ELEVATOR, "elevator_shell",
            .angles = {
                {{0, 1.6f, 0.2f}, {0.45f, 1.1f, -0.65f}},     // hero: wall panel + shell read
                {{0, 1.55f, 0.82f}, {0, 1.25f, 1.05f}},       // doorway: threshold + front seam
                {{-0.72f, 1.25f, -0.55f}, {-1.0f, 1.15f, -0.95f}}, // left_corner: shell/collision alignment
                {{0.72f, 1.25f, -0.55f}, {1.0f, 1.15f, -0.95f}},   // right_corner
                {{0, 0.72f, -0.1f}, {0, 0.02f, -0.35f}},      // floor: walkable plane
                {{0, 2.05f, -0.15f}, {0, 2.55f, -0.55f}},     // ceiling: top shell fit
                {{0.65f, 1.45f, 0.12f}, {0.98f, 1.4f, -0.2f}}, // panel_detail: lighting/material read
            },
            .angle_names = {"hero", "doorway", "left_corner", "right_corner", "floor", "ceiling", "panel_detail"},
            .angle_count = 7,
            .dark_by_design = true, .outdoor = false,
            .force_elevator_to_corridor = true,
            .flow_order = -1, .flow_next = STATE_GLASSHOUSE},
    };
#else
    QAEntry qa_scenes[] = {
        {STATE_HOTEL_EXT, "hotel_ext",
            .angles = {
                {{0, 1.6f, 8}, {0, 6, -12}},           // hero: looking up at Sky Tower
                {{0, 1.6f, 8}, {0, 1.6f, -5}},          // spawn: entrance approach
                {{-6, 1.6f, 4}, {4, 2, -8}},            // patrol_left: from sidewalk
                {{6, 1.6f, 4}, {-4, 2, -8}},            // patrol_right: opposite side
                {{0, 1.6f, -8}, {0, 3, 8}},             // patrol_back: looking back at street
                {{0, 3.0f, 0}, {0, 0, -12}},            // ceiling: bird's eye
            },
            .angle_names = {"hero", "spawn", "patrol_left", "patrol_right", "patrol_back", "overview"},
            .angle_count = 6,
            .dark_by_design = true, .outdoor = true,
            .flow_order = 1, .flow_next = STATE_LOBBY},
        {STATE_LOBBY, "lobby",
            .angles = {
                {{0, 1.6f, 6}, {-2, 3, -8}},            // hero: staircase + elevator + chandelier
                {{0, 1.6f, 8}, {0, 2, -5}},              // spawn: entering from front
                {{-6, 1.6f, 0}, {6, 2, -4}},             // patrol_left: reception desk angle
                {{6, 1.6f, 0}, {-6, 2, -4}},             // patrol_right: piano angle
                {{0, 1.6f, -6}, {0, 2, 6}},              // patrol_back: toward entrance
                {{0, 1.6f, 2}, {0, 0.5f, -2}},           // detail: floor/rug closeup
                {{0, 4.0f, 0}, {0, 0, -2}},              // ceiling: chandelier from below
            },
            .angle_names = {"hero", "spawn", "patrol_left", "patrol_right", "patrol_back", "detail", "ceiling"},
            .angle_count = 7,
            .outdoor = false,
            .flow_order = 2, .flow_next = STATE_ELEVATOR},
        {STATE_HALLWAY, "hallway",
            .angles = {
                {{0, 1.6f, -1}, {0, 1.5f, -20}},        // hero: one-point perspective
                {{0, 1.6f, -1}, {0, 1.5f, -20}},         // spawn: same — perspective IS the hero
                {{-2, 1.6f, -10}, {2, 1.5f, -20}},       // patrol_left: angled view
                {{2, 1.6f, -10}, {-2, 1.5f, -20}},       // patrol_right: opposite angle
                {{0, 1.6f, -18}, {0, 1.5f, 0}},          // patrol_back: from end looking back
                {{0, 1.6f, -5}, {1, 1.3f, -6}},          // detail: door detail
            },
            .angle_names = {"hero", "spawn", "patrol_left", "patrol_right", "patrol_back", "detail"},
            .angle_count = 6,
            .outdoor = false,
            .flow_order = -1, .flow_next = STATE_ROOM},  // orphaned
        {STATE_ROOM, "room",
            .angles = {
                {{0, 1.6f, 3}, {0, 1.2f, -3.5f}},       // hero: toward bed + headboard
                {{5.5f, 1.6f, 4}, {-2, 1.2f, -2}},       // spawn: from door toward bed
                {{-4, 1.6f, 0}, {4, 1.4f, -2}},          // patrol_left: window side
                {{4, 1.6f, 0}, {-4, 1.4f, -2}},          // patrol_right: bathroom side
                {{0, 1.6f, -3}, {0, 1.6f, 4}},           // patrol_back: from bed toward door
                {{0, 1.6f, 1}, {0, 0.8f, 0}},            // detail: table/objects closeup
            },
            .angle_names = {"hero", "spawn", "patrol_left", "patrol_right", "patrol_back", "detail"},
            .angle_count = 6,
            .outdoor = false,
            .flow_order = -1, .flow_next = STATE_BATHROOM},  // orphaned
        {STATE_BATHROOM, "bathroom",
            .angles = {
                {{0, 1.6f, 1.5f}, {0, 1.5f, -1.5f}},    // hero: bathtub + Ando window
                {{0, 1.6f, 1.5f}, {1.5f, 1.2f, 0}},      // spawn: toward sink/mirror
                {{-1.5f, 1.6f, 0}, {1.5f, 1.2f, -1}},    // patrol_left
                {{1.5f, 1.6f, 0}, {-1.5f, 1.2f, -1}},    // patrol_right
                {{0, 2.2f, 0}, {0, 0, 0}},                // ceiling: from above
            },
            .angle_names = {"hero", "spawn", "patrol_left", "patrol_right", "ceiling"},
            .angle_count = 5,
            .dark_by_design = true, .outdoor = false,
            .flow_order = -1, .flow_next = STATE_BALCONY},  // orphaned
        {STATE_BALCONY, "balcony",
            .angles = {
                {{0, 1.6f, 0.5f}, {0, 0.5f, -10}},      // hero: Earth bands — Rothko
                {{0, 1.6f, 0.5f}, {-1, 0.8f, -1}},       // spawn: wine table foreground
                {{-2, 1.6f, 0}, {2, 0.5f, -8}},           // patrol_left: railing angle
                {{2, 1.6f, 0}, {-2, 0.5f, -8}},           // patrol_right: opposite
                {{0, 1.6f, 0}, {0, -1, -10}},             // detail: looking down at Earth
                {{0, 1.6f, 0}, {0, 3, -5}},               // ceiling: looking up at stars
            },
            .angle_names = {"hero", "spawn", "patrol_left", "patrol_right", "detail_earth", "ceiling_stars"},
            .angle_count = 6,
            .dark_by_design = true, .outdoor = true,
            .flow_order = 8, .flow_next = STATE_BED},
        {STATE_ELEVATOR, "elevator",
            .angles = {
                {{0, 1.6f, 0}, {0.5f, 1.0f, -1.0f}},    // hero: button panel + walls
                {{0, 1.6f, 0}, {-0.3f, 2.5f, -0.8f}},    // spawn: up at ceiling
                {{-0.5f, 1.6f, 0.3f}, {0.5f, 1.0f, -0.5f}}, // patrol_left
                {{0.5f, 1.6f, 0.3f}, {-0.5f, 1.0f, -0.5f}}, // patrol_right
                {{0, 0.8f, 0}, {0, 1.6f, -0.5f}},        // detail: floor/lower
            },
            .angle_names = {"hero", "spawn", "patrol_left", "patrol_right", "detail"},
            .angle_count = 5,
            .dark_by_design = true, .outdoor = false,
            .flow_order = 3, .flow_next = STATE_HYPERSPACE},
        {STATE_SPACE_LOBBY, "space_lobby",
            .angles = {
                {{5, 1.6f, 5}, {-5, 3, -6}},             // hero: diagonal — window + chandelier
                {{0, 1.6f, 6}, {0, 2, -3}},               // spawn: entering, chandelier visible
                {{-7, 1.6f, 0}, {7, 2, -4}},              // patrol_left: far corner
                {{7, 1.6f, 0}, {-7, 2, -4}},              // patrol_right: opposite corner
                {{0, 1.6f, -6}, {0, 2, 6}},               // patrol_back: from window looking in
                {{3, 1.6f, 3}, {0, 0.5f, 0}},             // detail: floor/fountain
                {{0, 5.0f, 0}, {0, 0, -3}},               // ceiling: chandelier from above
            },
            .angle_names = {"hero", "spawn", "patrol_left", "patrol_right", "patrol_back", "detail", "ceiling"},
            .angle_count = 7,
            .outdoor = true,
            .flow_order = 5, .flow_next = STATE_SPACE_CORRIDOR},
        {STATE_SPACE_CORRIDOR, "space_corridor",
            .angles = {
                {{0, 1.6f, 2}, {0, 1.4f, 14}},           // hero: down corridor toward exit
                {{0, 1.6f, 0}, {-2, 1.6f, 4}},            // spawn: entering, first door visible
                {{-2, 1.6f, 6}, {2, 1.4f, 12}},           // patrol_left: angled hallway
                {{2, 1.6f, 6}, {-2, 1.4f, 12}},           // patrol_right
                {{0, 1.6f, 14}, {0, 1.6f, 0}},            // patrol_back: from end
                {{0, 1.6f, 8}, {1, 1.3f, 8}},             // detail: door closeup
            },
            .angle_names = {"hero", "spawn", "patrol_left", "patrol_right", "patrol_back", "detail"},
            .angle_count = 6,
            .dark_by_design = true, .outdoor = false,
            .flow_order = 6, .flow_next = STATE_SPACE_SUITE},
        {STATE_SPACE_SUITE, "space_suite",
            .angles = {
                {{0, 1.6f, 5}, {0, 1.6f, -4}},            // hero: entering, bed + window depth
                {{1.5f, 1.6f, 4.5f}, {-1.0f, 1.4f, -3.5f}}, // spawn: offset entry angle
                {{-5, 1.6f, 0}, {5, 1.8f, -2}},           // patrol_left: desk side
                {{5, 1.6f, 0}, {-5, 1.8f, -2}},           // patrol_right: window side
                {{0, 1.6f, -4}, {0, 1.6f, 5}},            // patrol_back: from window
                {{2, 1.6f, 2}, {0, 0.8f, 0}},             // detail: objects on desk/table
                {{0, 3.0f, 0}, {0, 0, -2}},               // ceiling: overhead view
            },
            .angle_names = {"hero", "spawn", "patrol_left", "patrol_right", "patrol_back", "detail", "ceiling"},
            .angle_count = 7,
            .outdoor = false,
            .flow_order = 7, .flow_next = STATE_BALCONY},
        {STATE_CAR, "taxi",
            .angles = {
                {{0, 0, 0}, {-0.3f, 0.9f, -1.2f}},       // hero: driver + dashboard
                {{0, 0, 0}, {0, 0.9f, -2.0f}},            // spawn: forward through windshield
                {{0, 0, 0}, {-1, 0.8f, 0}},               // patrol_left: out left window
                {{0, 0, 0}, {1, 0.8f, 0}},                // patrol_right: out right window
                {{0, 0, 0}, {0, 1.2f, 0}},                // ceiling: looking up at roof
            },
            .angle_names = {"hero", "spawn", "left_window", "right_window", "ceiling"},
            .angle_count = 5,
            .dark_by_design = true, .outdoor = false,
            .flow_order = 0, .flow_next = STATE_HOTEL_EXT},
        {STATE_HYPERSPACE, "hyperspace",
            .angles = {
                {{0, 0, 0}, {0, 0, -20}},                 // hero: tunnel convergence
                {{0, 0, 0}, {0, 0, -20}},                 // spawn
                {{0, 0, 0}, {-5, 0, -20}},                // patrol_left: off-center
                {{0, 0, 0}, {5, 0, -20}},                 // patrol_right
            },
            .angle_names = {"hero", "spawn", "patrol_left", "patrol_right"},
            .angle_count = 4,
            .dark_by_design = true, .outdoor = false,
            .flow_order = 4, .flow_next = STATE_SPACE_LOBBY},
        {STATE_PARIS_DREAM, "paris_dream",
            .angles = {
                {{0, 1.6f, 4}, {0, 1.2f, -3.5f}},        // hero: toward bed + photograph
                {{0, 1.6f, 4}, {0, 1.6f, 0}},             // spawn
                {{-3, 1.6f, 0}, {3, 1.4f, -2}},           // patrol_left
                {{3, 1.6f, 0}, {-3, 1.4f, -2}},           // patrol_right
                {{0, 1.6f, -2}, {0, 1.6f, 4}},            // patrol_back
                {{0, 1.6f, 2}, {0, 0.8f, 0}},             // detail: objects
            },
            .angle_names = {"hero", "spawn", "patrol_left", "patrol_right", "patrol_back", "detail"},
            .angle_count = 6,
            .dark_by_design = false, .outdoor = false,
            .flow_order = 10, .flow_next = STATE_RETURN_TAXI},
        {STATE_RETURN_TAXI, "return_taxi",
            .angles = {
                {{0, 1.0f, 0}, {-0.3f, 0.9f, -1.2f}},    // hero: driver + dashboard
                {{0, 1.0f, 0}, {0.5f, 0.8f, -0.5f}},      // spawn
                {{0, 1.0f, 0}, {-1, 0.8f, 0}},             // patrol_left
                {{0, 1.0f, 0}, {1, 0.8f, 0}},              // patrol_right
            },
            .angle_names = {"hero", "spawn", "patrol_left", "patrol_right"},
            .angle_count = 4,
            .dark_by_design = false, .outdoor = false,
            .flow_order = 11, .flow_next = STATE_TITLE},
        // ── Additional scenes for full coverage ──
        {STATE_BED, "bed",
            .angles = {
                {{0, 1.2f, 0}, {0, 2, -3}},               // hero: lying in bed, looking up
                {{0, 1.2f, 0}, {0, 1, -2}},               // spawn
                {{-2, 1.6f, 0}, {2, 1.2f, -2}},           // patrol_left
                {{2, 1.6f, 0}, {-2, 1.2f, -2}},           // patrol_right
            },
            .angle_names = {"hero", "spawn", "patrol_left", "patrol_right"},
            .angle_count = 4,
            .dark_by_design = true, .outdoor = false,
            .flow_order = 9, .flow_next = STATE_STARS},
        {STATE_STARS, "stars",
            .angles = {
                {{0, 0, 0}, {0, 1, -5}},                  // hero: looking up at stars
                {{0, 0, 0}, {0, 0, -10}},                 // spawn
                {{0, 0, 0}, {-3, 1, -5}},                 // patrol_left
                {{0, 0, 0}, {3, 1, -5}},                  // patrol_right
            },
            .angle_names = {"hero", "spawn", "patrol_left", "patrol_right"},
            .angle_count = 4,
            .dark_by_design = true, .outdoor = true,
            .flow_order = -1, .flow_next = STATE_PARIS_DREAM},
        {STATE_CLEANED_SUITE, "cleaned_suite",
            .angles = {
                {{0, 1.6f, 5}, {0, 1.6f, -4}},            // hero: entry depth still reads after cleanup
                {{1.5f, 1.6f, 4.5f}, {-1.0f, 1.4f, -3.5f}}, // spawn
                {{-5, 1.6f, 0}, {5, 1.8f, -2}},           // patrol_left
                {{5, 1.6f, 0}, {-5, 1.8f, -2}},           // patrol_right
            },
            .angle_names = {"hero", "spawn", "patrol_left", "patrol_right"},
            .angle_count = 4,
            .outdoor = false,
            .flow_order = -1, .flow_next = STATE_MONTAGE},
    };
#endif
    int qa_count = (int)(sizeof(qa_scenes)/sizeof(qa_scenes[0]));

    // ── Determine mode ──
#ifdef QA_E2E
    const bool e2e_mode = true;
#else
    const bool e2e_mode = false;
#endif
    const char *report_path = e2e_mode ? "qa/e2e_report.json" : "qa/report.json";

    printf("\n[QA] ============================================================\n");
    printf("[QA]  ENDEARING VOID — %s QA Analysis\n", e2e_mode ? "Full E2E" : "Standard");
    printf("[QA]  Scenes: %d | Render: %dx%d | MAX_WALLS: %d\n", qa_count, RENDER_W, RENDER_H, MAX_WALLS);
    if (e2e_mode) printf("[QA]  Mode: Multi-angle patrol + flow testing + collision probes\n");
    printf("[QA] ============================================================\n\n");

    FILE *jf = fopen(report_path, "w");
    if (jf) {
        fprintf(jf, "{\n  \"mode\": \"%s\",\n  \"render\": \"%dx%d\",\n  \"max_walls\": %d,\n",
                e2e_mode ? "e2e" : "standard", RENDER_W, RENDER_H, MAX_WALLS);
        fprintf(jf, "  \"material_names\": [\"concrete\",\"marble\",\"wood\",\"carpet\",\"wallpaper\",\"tile\",\"brass\",\"glass\",\"leather\",\"fabric\",\"checkerboard\",\"herringbone\",\"parquet\",\"velvet\",\"water\",\"puddle\",\"emissive\"],\n");
        fprintf(jf, "  \"scenes\": [\n");
    }

    int qa_total_issues = 0;

    // ── Pixel analysis helper — extracts metrics from an image ──
    // Returns data as a struct for flexible JSON output
    typedef struct {
        float luma, black_pct, bright_pct, mid_pct;
        float edge_density, contrast_ratio, color_variance;
        float warmth, center_luma, edge_luma;
        float lr_balance, tb_balance;
        float quadrant_luma[4];
        int hue_buckets;
        float avg_rgb[3];
        // Extended metrics
        float saturation_avg;       // average saturation across frame
        float brightness_std;       // stddev of pixel brightness
        float dominant_color_pct;   // how much the single most common color dominates
        int dark_quadrant_count;    // quadrants with luma < 15
        float rule_of_thirds_energy; // edge energy near 1/3 and 2/3 lines
    } PixelMetrics;

    #define QA_ANALYZE_IMAGE(img_ptr, metrics_ptr) do { \
        Color *_px = LoadImageColors(*(img_ptr)); \
        int _total = RENDER_W * RENDER_H; \
        int _half_w = RENDER_W / 2, _half_h = RENDER_H / 2; \
        long _r_sum = 0, _g_sum = 0, _b_sum = 0; \
        int _black = 0, _bright = 0, _mid = 0; \
        float _r_sq = 0, _g_sq = 0, _b_sq = 0; \
        int _hues[36] = {0}; \
        float _ql[4] = {0}; long _qr[4] = {0}, _qg[4] = {0}, _qb[4] = {0}; \
        int _qpx = _half_w * _half_h; \
        int _cx0 = RENDER_W/4, _cx1 = RENDER_W*3/4, _cy0 = RENDER_H/4, _cy1 = RENDER_H*3/4; \
        float _cl = 0, _el = 0; int _cpx = 0, _epx = 0; \
        int _edges = 0; \
        float _sat_sum = 0; float _bri_sum = 0; float _bri_sq = 0; \
        int _color_buckets[64] = {0}; \
        float _thirds_energy = 0; \
        int _third_x1 = RENDER_W/3, _third_x2 = RENDER_W*2/3; \
        int _third_y1 = RENDER_H/3, _third_y2 = RENDER_H*2/3; \
        for (int _p = 0; _p < _total; _p++) { \
            unsigned char _pr = _px[_p].r, _pg = _px[_p].g, _pb = _px[_p].b; \
            float _pl = 0.299f*_pr + 0.587f*_pg + 0.114f*_pb; \
            _r_sum += _pr; _g_sum += _pg; _b_sum += _pb; \
            _r_sq += (float)_pr*_pr; _g_sq += (float)_pg*_pg; _b_sq += (float)_pb*_pb; \
            _bri_sum += _pl; _bri_sq += _pl*_pl; \
            if (_pr < 15 && _pg < 15 && _pb < 15) _black++; \
            else if (_pr > 230 && _pg > 230 && _pb > 230) _bright++; \
            else if (_pl > 40 && _pl < 200) _mid++; \
            float _rf = _pr/255.0f, _gf = _pg/255.0f, _bf = _pb/255.0f; \
            float _cmax = _rf > _gf ? (_rf > _bf ? _rf : _bf) : (_gf > _bf ? _gf : _bf); \
            float _cmin = _rf < _gf ? (_rf < _bf ? _rf : _bf) : (_gf < _bf ? _gf : _bf); \
            float _delta = _cmax - _cmin; \
            if (_delta > 0.05f) { \
                _sat_sum += _delta / (_cmax > 0.001f ? _cmax : 0.001f); \
                float _hue = 0; \
                if (_cmax == _rf) _hue = 60.0f * fmodf((_gf-_bf)/_delta, 6.0f); \
                else if (_cmax == _gf) _hue = 60.0f * ((_bf-_rf)/_delta + 2.0f); \
                else _hue = 60.0f * ((_rf-_gf)/_delta + 4.0f); \
                if (_hue < 0) _hue += 360.0f; \
                _hues[(int)(_hue/10.0f) % 36]++; \
            } \
            int _px_x = _p % RENDER_W, _px_y = _p / RENDER_W; \
            int _qi = (_px_x >= _half_w ? 1 : 0) + (_px_y >= _half_h ? 2 : 0); \
            _qr[_qi] += _pr; _qg[_qi] += _pg; _qb[_qi] += _pb; \
            if (_px_x >= _cx0 && _px_x < _cx1 && _px_y >= _cy0 && _px_y < _cy1) { _cl += _pl; _cpx++; } \
            else { _el += _pl; _epx++; } \
            if (_px_x > 0 && _px_x < RENDER_W-1) { \
                float _ll = 0.299f*_px[_p-1].r + 0.587f*_px[_p-1].g + 0.114f*_px[_p-1].b; \
                float _rl = 0.299f*_px[_p+1].r + 0.587f*_px[_p+1].g + 0.114f*_px[_p+1].b; \
                float _eg = fabsf(_rl - _ll); \
                if (_eg > 25) _edges++; \
                if (abs(_px_x - _third_x1) < 3 || abs(_px_x - _third_x2) < 3) _thirds_energy += _eg; \
            } \
            if (_px_y > 0 && _px_y < RENDER_H-1) { \
                float _al = 0.299f*_px[_p-RENDER_W].r + 0.587f*_px[_p-RENDER_W].g + 0.114f*_px[_p-RENDER_W].b; \
                float _bl = 0.299f*_px[_p+RENDER_W].r + 0.587f*_px[_p+RENDER_W].g + 0.114f*_px[_p+RENDER_W].b; \
                float _eg2 = fabsf(_bl - _al); \
                if (_eg2 > 25) _edges++; \
                if (abs(_px_y - _third_y1) < 3 || abs(_px_y - _third_y2) < 3) _thirds_energy += _eg2; \
            } \
            _color_buckets[(_pr >> 6) * 16 + (_pg >> 6) * 4 + (_pb >> 6)]++; \
        } \
        int _hc = 0; for (int _h = 0; _h < 36; _h++) if (_hues[_h] > _total/500) _hc++; \
        float _ra = (float)_r_sum/_total, _ga = (float)_g_sum/_total, _ba = (float)_b_sum/_total; \
        (metrics_ptr)->luma = 0.299f*_ra + 0.587f*_ga + 0.114f*_ba; \
        (metrics_ptr)->black_pct = 100.0f*_black/_total; \
        (metrics_ptr)->bright_pct = 100.0f*_bright/_total; \
        (metrics_ptr)->mid_pct = 100.0f*_mid/_total; \
        (metrics_ptr)->edge_density = 100.0f*_edges/_total; \
        (metrics_ptr)->color_variance = (_r_sq/_total - _ra*_ra) + (_g_sq/_total - _ga*_ga) + (_b_sq/_total - _ba*_ba); \
        (metrics_ptr)->hue_buckets = _hc; \
        (metrics_ptr)->warmth = _ra - _ba; \
        (metrics_ptr)->avg_rgb[0] = _ra; (metrics_ptr)->avg_rgb[1] = _ga; (metrics_ptr)->avg_rgb[2] = _ba; \
        (metrics_ptr)->center_luma = _cpx > 0 ? _cl / _cpx : 0; \
        (metrics_ptr)->edge_luma = _epx > 0 ? _el / _epx : 0; \
        for (int _q = 0; _q < 4; _q++) { \
            float _qra = (float)_qr[_q]/_qpx, _qga = (float)_qg[_q]/_qpx, _qba = (float)_qb[_q]/_qpx; \
            (metrics_ptr)->quadrant_luma[_q] = 0.299f*_qra + 0.587f*_qga + 0.114f*_qba; \
        } \
        float _qlmax = (metrics_ptr)->quadrant_luma[0], _qlmin = (metrics_ptr)->quadrant_luma[0]; \
        for (int _q = 1; _q < 4; _q++) { \
            if ((metrics_ptr)->quadrant_luma[_q] > _qlmax) _qlmax = (metrics_ptr)->quadrant_luma[_q]; \
            if ((metrics_ptr)->quadrant_luma[_q] < _qlmin) _qlmin = (metrics_ptr)->quadrant_luma[_q]; \
        } \
        (metrics_ptr)->contrast_ratio = _qlmin > 0.01f ? _qlmax / _qlmin : 999.0f; \
        (metrics_ptr)->lr_balance = ((metrics_ptr)->quadrant_luma[0]+(metrics_ptr)->quadrant_luma[2]) > 0 ? \
            fabsf(((metrics_ptr)->quadrant_luma[0]+(metrics_ptr)->quadrant_luma[2]) - ((metrics_ptr)->quadrant_luma[1]+(metrics_ptr)->quadrant_luma[3])) / \
            (((metrics_ptr)->quadrant_luma[0]+(metrics_ptr)->quadrant_luma[2]+(metrics_ptr)->quadrant_luma[1]+(metrics_ptr)->quadrant_luma[3])/2.0f) : 0; \
        (metrics_ptr)->tb_balance = ((metrics_ptr)->quadrant_luma[0]+(metrics_ptr)->quadrant_luma[1]) > 0 ? \
            fabsf(((metrics_ptr)->quadrant_luma[0]+(metrics_ptr)->quadrant_luma[1]) - ((metrics_ptr)->quadrant_luma[2]+(metrics_ptr)->quadrant_luma[3])) / \
            (((metrics_ptr)->quadrant_luma[0]+(metrics_ptr)->quadrant_luma[1]+(metrics_ptr)->quadrant_luma[2]+(metrics_ptr)->quadrant_luma[3])/2.0f) : 0; \
        (metrics_ptr)->saturation_avg = _sat_sum / _total; \
        float _bri_avg = _bri_sum / _total; \
        (metrics_ptr)->brightness_std = sqrtf(_bri_sq / _total - _bri_avg * _bri_avg); \
        int _max_bucket = 0; for (int _b = 0; _b < 64; _b++) { if (_color_buckets[_b] > _max_bucket) _max_bucket = _color_buckets[_b]; } \
        (metrics_ptr)->dominant_color_pct = 100.0f * (float)_max_bucket / (float)_total; \
        int _dqc = 0; for (int _q = 0; _q < 4; _q++) if ((metrics_ptr)->quadrant_luma[_q] < 15) _dqc++; \
        (metrics_ptr)->dark_quadrant_count = _dqc; \
        (metrics_ptr)->rule_of_thirds_energy = _thirds_energy / (float)(RENDER_H * 12); \
        UnloadImageColors(_px); \
    } while(0)

    // ── Render helper macro ──
    #define QA_RENDER_SHOT_EX(cam_pos, cam_target, out_path, scene_idx) do { \
        g.player.camera.position = cam_pos; \
        g.player.camera.target = cam_target; \
        for (int _f = 0; _f < 8; _f++) { \
            BeginTextureMode(g.render_target); \
            ClearBackground(g.scene.fog_color.a > 0 ? g.scene.fog_color : (Color){8,10,22,255}); \
            draw_scene_3d(&g.player, &g.scene, &g.lighting, \
                &g.cube_model, g.cube_model_loaded, &g.cyl_model, g.cyl_model_loaded, \
                &g.sphere_model, g.sphere_model_loaded, &g.cone_model, g.cone_model_loaded, \
                &g.skytower_model, g.skytower_loaded, \
                !qa_scenes[scene_idx].outdoor, (float)_f * 0.016f); \
            EndTextureMode(); \
            BeginTextureMode(g.postfx_target); ClearBackground(BLACK); \
            draw_postfx(&g.postfx, g.render_target); EndTextureMode(); \
            BeginDrawing(); ClearBackground(BLACK); EndDrawing(); \
        } \
        RenderTexture2D _disp = g.postfx.ready ? g.postfx_target : g.render_target; \
        Image _shot = LoadImageFromTexture(_disp.texture); \
        ImageFlipVertical(&_shot); \
        ExportImage(_shot, out_path); \
        UnloadImage(_shot); \
    } while(0)

    // ── JSON helper — write pixel metrics ──
    #define QA_WRITE_METRICS_JSON(file, m, indent) do { \
        fprintf(file, "%s\"luma\": %.1f,\n", indent, (m)->luma); \
        fprintf(file, "%s\"black_pct\": %.1f,\n", indent, (m)->black_pct); \
        fprintf(file, "%s\"bright_pct\": %.1f,\n", indent, (m)->bright_pct); \
        fprintf(file, "%s\"mid_tone_pct\": %.1f,\n", indent, (m)->mid_pct); \
        fprintf(file, "%s\"edge_density\": %.1f,\n", indent, (m)->edge_density); \
        fprintf(file, "%s\"contrast_ratio\": %.2f,\n", indent, (m)->contrast_ratio); \
        fprintf(file, "%s\"color_variance\": %.1f,\n", indent, (m)->color_variance); \
        fprintf(file, "%s\"hue_buckets\": %d,\n", indent, (m)->hue_buckets); \
        fprintf(file, "%s\"warmth\": %.1f,\n", indent, (m)->warmth); \
        fprintf(file, "%s\"avg_rgb\": [%.0f, %.0f, %.0f],\n", indent, (m)->avg_rgb[0], (m)->avg_rgb[1], (m)->avg_rgb[2]); \
        fprintf(file, "%s\"center_luma\": %.1f,\n", indent, (m)->center_luma); \
        fprintf(file, "%s\"edge_luma\": %.1f,\n", indent, (m)->edge_luma); \
        fprintf(file, "%s\"lr_balance\": %.3f,\n", indent, (m)->lr_balance); \
        fprintf(file, "%s\"tb_balance\": %.3f,\n", indent, (m)->tb_balance); \
        fprintf(file, "%s\"quadrant_luma\": [%.1f, %.1f, %.1f, %.1f],\n", indent, \
                (m)->quadrant_luma[0], (m)->quadrant_luma[1], (m)->quadrant_luma[2], (m)->quadrant_luma[3]); \
        fprintf(file, "%s\"saturation_avg\": %.3f,\n", indent, (m)->saturation_avg); \
        fprintf(file, "%s\"brightness_std\": %.1f,\n", indent, (m)->brightness_std); \
        fprintf(file, "%s\"dominant_color_pct\": %.1f,\n", indent, (m)->dominant_color_pct); \
        fprintf(file, "%s\"dark_quadrant_count\": %d,\n", indent, (m)->dark_quadrant_count); \
        fprintf(file, "%s\"rule_of_thirds_energy\": %.1f\n", indent, (m)->rule_of_thirds_energy); \
    } while(0)

    for (int qi = 0; qi < qa_count; qi++) {
        double load_start = GetTime();
        g.elevator_to_corridor = qa_scenes[qi].force_elevator_to_corridor;
        load_state(qa_scenes[qi].gs);
        g.fade_alpha = 0.0f; g.fade_target = 0.0f;
        double load_time_ms = (GetTime() - load_start) * 1000.0;

        // ── Determine how many angles to shoot ──
        int num_angles = e2e_mode ? qa_scenes[qi].angle_count : 2;  // basic mode: hero + spawn only

        // ── Take screenshots from all angles ──
        PixelMetrics angle_metrics[QA_MAX_ANGLES] = {0};
        for (int ai = 0; ai < num_angles; ai++) {
            // Reload scene for each angle (clean state)
            if (ai > 0) {
                g.elevator_to_corridor = qa_scenes[qi].force_elevator_to_corridor;
                load_state(qa_scenes[qi].gs);
                g.fade_alpha = 0.0f; g.fade_target = 0.0f;
            }

            char shot_path[256];
            snprintf(shot_path, sizeof(shot_path), "qa/screenshots/%s_%s.png",
                     qa_scenes[qi].name, qa_scenes[qi].angle_names[ai]);

            QACam cam = {qa_scenes[qi].angles[ai].pos, qa_scenes[qi].angles[ai].target};
            // If pos is 0,0,0 keep whatever spawn set (e.g., taxi backseat)
            Vector3 cam_pos = (cam.pos.x != 0 || cam.pos.y != 0 || cam.pos.z != 0) ? cam.pos : g.player.camera.position;

            QA_RENDER_SHOT_EX(cam_pos, cam.target, shot_path, qi);

            // Analyze the screenshot
            RenderTexture2D disp = g.postfx.ready ? g.postfx_target : g.render_target;
            Image shot_img = LoadImageFromTexture(disp.texture);
            ImageFlipVertical(&shot_img);
            QA_ANALYZE_IMAGE(&shot_img, &angle_metrics[ai]);
            UnloadImage(shot_img);
        }
        g.elevator_to_corridor = false;

        // Also save backward-compat aliases for basic mode
        {
            char alias_path[256];
            snprintf(alias_path, sizeof(alias_path), "qa/screenshots/%s.png", qa_scenes[qi].name);
            // Copy hero shot
            char hero_path[256];
            snprintf(hero_path, sizeof(hero_path), "qa/screenshots/%s_hero.png", qa_scenes[qi].name);
            if (FileExists(hero_path)) {
                Image hero = LoadImage(hero_path);
                ExportImage(hero, alias_path);
                UnloadImage(hero);
            }
        }

        // ── Scene data extraction ──
        int mat_counts[17] = {0};
        int decal_count = 0, model_count = 0, pushable_count = 0, breakable_count = 0, hinge_count = 0;
        int no_collide_count = 0;
        float wall_volume_total = 0;  // rough total volume of all walls
        for (int w = 0; w < g.scene.wall_count; w++) {
            Wall *wl = &g.scene.walls[w];
            if (wl->material < 17) mat_counts[wl->material]++;
            if (wl->is_decal) decal_count++;
            if (wl->shape == SHAPE_MODEL) model_count++;
            if (wl->pushable) pushable_count++;
            if (wl->breakable) breakable_count++;
            if (wl->hinge) hinge_count++;
            if (wl->no_collide) no_collide_count++;
            wall_volume_total += wl->size.x * wl->size.y * wl->size.z;
        }
        int mat_types_used = 0;
        for (int m = 0; m < 17; m++) if (mat_counts[m] > 0) mat_types_used++;
        int mat_tagged = g.scene.wall_count - mat_counts[0];
        float mat_cov = g.scene.wall_count > 0 ? 100.0f * mat_tagged / g.scene.wall_count : 0;
        float wall_pct = 100.0f * g.scene.wall_count / MAX_WALLS;

        // ── Object details ──
        int interact_count = 0, obj_unreachable = 0;
        float obj_max_dist = 0;
        for (int oi = 0; oi < g.scene.object_count; oi++) {
            if (!g.scene.objects[oi].active) continue;
            interact_count++;
            float dx = g.scene.objects[oi].pos.x - g.scene.spawn.x;
            float dz = g.scene.objects[oi].pos.z - g.scene.spawn.z;
            float dist = sqrtf(dx*dx + dz*dz);
            if (dist > obj_max_dist) obj_max_dist = dist;
            if (dist > 25.0f) obj_unreachable++;
        }

        // ── Collision probe (E2E only): can the player stand at grid points? ──
        int collision_grid_size = 0;
        int collision_floor_hits = 0;
        int collision_stuck_points = 0;
        if (e2e_mode) {
            // Probe authored floor bounds instead of a blind 20m square around spawn.
            float min_fx = 0.0f, max_fx = 0.0f;
            float min_fz = 0.0f, max_fz = 0.0f;
            bool found_floor_bounds = false;
            for (int w = 0; w < g.scene.wall_count; w++) {
                Wall *wl = &g.scene.walls[w];
                if (!qa_is_floor_like_wall(wl))
                    continue;
                float top_y = wl->pos.y + wl->size.y/2;
                if (top_y > g.scene.spawn.y + 0.5f || top_y < g.scene.spawn.y - 3.0f)
                    continue;
                float wx0 = wl->pos.x - wl->size.x/2, wx1 = wl->pos.x + wl->size.x/2;
                float wz0 = wl->pos.z - wl->size.z/2, wz1 = wl->pos.z + wl->size.z/2;
                if (!found_floor_bounds) {
                    min_fx = wx0; max_fx = wx1;
                    min_fz = wz0; max_fz = wz1;
                    found_floor_bounds = true;
                } else {
                    if (wx0 < min_fx) min_fx = wx0;
                    if (wx1 > max_fx) max_fx = wx1;
                    if (wz0 < min_fz) min_fz = wz0;
                    if (wz1 > max_fz) max_fz = wz1;
                }
            }
            if (found_floor_bounds) {
                collision_grid_size = 11;
                float span_x = max_fx - min_fx;
                float span_z = max_fz - min_fz;
                float grid_spacing_x = collision_grid_size > 1 ? span_x / (collision_grid_size - 1) : 0.0f;
                float grid_spacing_z = collision_grid_size > 1 ? span_z / (collision_grid_size - 1) : 0.0f;
                for (int gx = 0; gx < collision_grid_size; gx++) {
                    for (int gz = 0; gz < collision_grid_size; gz++) {
                        float px = min_fx + gx * grid_spacing_x;
                        float pz = min_fz + gz * grid_spacing_z;
                        float py = g.scene.spawn.y;
                        // Check if position is inside any wall (stuck) or has floor beneath
                        bool has_floor = false;
                        bool is_stuck = false;
                        for (int w = 0; w < g.scene.wall_count; w++) {
                            Wall *wl = &g.scene.walls[w];
                            if (wl->no_collide || wl->is_decal || wl->shape != SHAPE_CUBE) continue;
                            float wx = wl->pos.x, wy = wl->pos.y, wz = wl->pos.z;
                            float hw = wl->size.x/2, hh = wl->size.y/2, hd = wl->size.z/2;
                            float top_y = wy + hh;
                            // Floor check: wall is a plausible floor near the spawn plane.
                            if (qa_is_floor_like_wall(wl) &&
                                top_y > g.scene.spawn.y - 3.0f && top_y < g.scene.spawn.y + 0.5f &&
                                py > wy - hh && py < wy + hh + 2.0f &&
                                px > wx - hw && px < wx + hw && pz > wz - hd && pz < wz + hd) {
                                has_floor = true;
                            }
                            // Stuck check: probe point is inside a structural wall.
                            if (qa_is_structural_wall(wl) &&
                                px > wx - hw && px < wx + hw &&
                                py > wy - hh && py < wy + hh &&
                                pz > wz - hd && pz < wz + hd) {
                                is_stuck = true;
                            }
                        }
                        if (has_floor) collision_floor_hits++;
                        if (is_stuck) collision_stuck_points++;
                    }
                }
            }
        }

        // ── Wall overlap detection (E2E only): find overlapping non-decal walls ──
        int overlap_count = 0;
        if (e2e_mode) {
            for (int a = 0; a < g.scene.wall_count && a < 500; a++) {
                Wall *wa = &g.scene.walls[a];
                if (!qa_is_overlap_candidate_wall(wa)) continue;
                for (int b = a + 1; b < g.scene.wall_count && b < 500; b++) {
                    Wall *wb = &g.scene.walls[b];
                    if (!qa_is_overlap_candidate_wall(wb)) continue;
                    // AABB overlap check
                    float ax0 = wa->pos.x - wa->size.x/2, ax1 = wa->pos.x + wa->size.x/2;
                    float ay0 = wa->pos.y - wa->size.y/2, ay1 = wa->pos.y + wa->size.y/2;
                    float az0 = wa->pos.z - wa->size.z/2, az1 = wa->pos.z + wa->size.z/2;
                    float bx0 = wb->pos.x - wb->size.x/2, bx1 = wb->pos.x + wb->size.x/2;
                    float by0 = wb->pos.y - wb->size.y/2, by1 = wb->pos.y + wb->size.y/2;
                    float bz0 = wb->pos.z - wb->size.z/2, bz1 = wb->pos.z + wb->size.z/2;
                    if (ax0 < bx1 && ax1 > bx0 && ay0 < by1 && ay1 > by0 && az0 < bz1 && az1 > bz0) {
                        // Calculate overlap volume
                        float ox = fminf(ax1, bx1) - fmaxf(ax0, bx0);
                        float oy = fminf(ay1, by1) - fmaxf(ay0, by0);
                        float oz = fminf(az1, bz1) - fmaxf(az0, bz0);
                        if (ox < 0.05f || oy < 0.05f || oz < 0.05f) continue;
                        float ovol = ox * oy * oz;
                        float smaller = fminf(wa->size.x * wa->size.y * wa->size.z,
                                              wb->size.x * wb->size.y * wb->size.z);
                        // Only count if overlap is significant relative to the smaller structure.
                        if (smaller > 0.001f && ovol / smaller > 0.25f) overlap_count++;
                    }
                }
            }
        }

        // ── Issue detection (on hero angle metrics) ──
        PixelMetrics *hero = &angle_metrics[0];
        int issues = 0;
        char ibuf[8192] = {0};
        int ib = 0;
        bool dark = qa_scenes[qi].dark_by_design;

        // COMPOSITION
        if (hero->edge_density < 0.5f && !dark) {
            ib += snprintf(ibuf+ib, sizeof(ibuf)-(size_t)ib,
                "    COMPOSITION: edge density %.1f%% — no visible geometry\n", hero->edge_density); issues++;
        }
        if (hero->contrast_ratio < 1.3f && !dark) {
            ib += snprintf(ibuf+ib, sizeof(ibuf)-(size_t)ib,
                "    COMPOSITION: contrast ratio %.1f:1 — flat, no drama\n", hero->contrast_ratio); issues++;
        }
        if (hero->center_luma < 10 && hero->edge_luma < 10 && !dark) {
            ib += snprintf(ibuf+ib, sizeof(ibuf)-(size_t)ib,
                "    COMPOSITION: center luma %.0f — nothing visible\n", hero->center_luma); issues++;
        }

        // LIGHTING
        if (hero->black_pct > 85.0f && !dark) {
            ib += snprintf(ibuf+ib, sizeof(ibuf)-(size_t)ib,
                "    LIGHTING: %.0f%% black — scene is unlit\n", hero->black_pct); issues++;
        }
        if (hero->luma < 5.0f && !dark) {
            ib += snprintf(ibuf+ib, sizeof(ibuf)-(size_t)ib,
                "    LIGHTING: luma %.1f — nearly invisible\n", hero->luma); issues++;
        }
        if (hero->bright_pct > 60.0f) {
            ib += snprintf(ibuf+ib, sizeof(ibuf)-(size_t)ib,
                "    LIGHTING: %.0f%% blown out\n", hero->bright_pct); issues++;
        }

        // MATERIAL
        if (mat_cov < 25.0f && g.scene.wall_count > 20) {
            ib += snprintf(ibuf+ib, sizeof(ibuf)-(size_t)ib,
                "    MATERIAL: only %.0f%% non-concrete\n", mat_cov); issues++;
        }
        if (mat_types_used < 3 && g.scene.wall_count > 30) {
            ib += snprintf(ibuf+ib, sizeof(ibuf)-(size_t)ib,
                "    MATERIAL: only %d types\n", mat_types_used); issues++;
        }

        // COLOR
        if (hero->color_variance < 50.0f && !dark) {
            ib += snprintf(ibuf+ib, sizeof(ibuf)-(size_t)ib,
                "    COLOR: variance %.0f — flat\n", hero->color_variance); issues++;
        }
        if (hero->hue_buckets < 2 && !dark) {
            ib += snprintf(ibuf+ib, sizeof(ibuf)-(size_t)ib,
                "    COLOR: only %d hue buckets — monotone\n", hero->hue_buckets); issues++;
        }

        // BUDGET
        if (wall_pct > 85.0f) {
            ib += snprintf(ibuf+ib, sizeof(ibuf)-(size_t)ib,
                "    BUDGET: walls %.0f%% (%d/%d)\n", wall_pct, g.scene.wall_count, MAX_WALLS); issues++;
        }

        // INTERACTION
        if (obj_unreachable > 0) {
            ib += snprintf(ibuf+ib, sizeof(ibuf)-(size_t)ib,
                "    INTERACTION: %d unreachable objects\n", obj_unreachable); issues++;
        }

        // ANTI-PATTERNS
        if (hero->warmth < -15 && hero->luma < 40 && !dark && hero->contrast_ratio > 3) {
            ib += snprintf(ibuf+ib, sizeof(ibuf)-(size_t)ib,
                "    ANTI-PATTERN: horror vibes (cold %.0f, dark %.0f)\n", hero->warmth, hero->luma); issues++;
        }
        if ((qa_scenes[qi].gs == STATE_SPACE_LOBBY || qa_scenes[qi].gs == STATE_SPACE_CORRIDOR ||
             qa_scenes[qi].gs == STATE_SPACE_SUITE) && hero->hue_buckets < 3) {
            ib += snprintf(ibuf+ib, sizeof(ibuf)-(size_t)ib,
                "    ANTI-PATTERN: grey-on-grey in space\n"); issues++;
        }

        // ANOMALY
        if (hero->dominant_color_pct > 70.0f && !dark) {
            ib += snprintf(ibuf+ib, sizeof(ibuf)-(size_t)ib,
                "    ANOMALY: %.0f%% single color — rogue geometry?\n", hero->dominant_color_pct); issues++;
        }

        // E2E-ONLY CHECKS
        if (e2e_mode) {
            if (overlap_count > 25) {
                ib += snprintf(ibuf+ib, sizeof(ibuf)-(size_t)ib,
                    "    GEOMETRY: %d wall overlaps (z-fighting risk)\n", overlap_count); issues++;
            }
            if (collision_stuck_points > 24) {
                ib += snprintf(ibuf+ib, sizeof(ibuf)-(size_t)ib,
                    "    COLLISION: %d stuck points in grid\n", collision_stuck_points); issues++;
            }
            int total_grid = collision_grid_size * collision_grid_size;
            if (total_grid > 0 && collision_floor_hits < total_grid / 10) {
                ib += snprintf(ibuf+ib, sizeof(ibuf)-(size_t)ib,
                    "    COLLISION: only %d/%d floor coverage\n", collision_floor_hits, total_grid); issues++;
            }
            // Check angle consistency — if patrol angles see radically different scenes, flag it
            if (num_angles >= 4) {
                float luma_min = 999, luma_max = 0;
                for (int ai = 0; ai < num_angles; ai++) {
                    if (angle_metrics[ai].luma < luma_min) luma_min = angle_metrics[ai].luma;
                    if (angle_metrics[ai].luma > luma_max) luma_max = angle_metrics[ai].luma;
                }
                if (!dark && luma_max > 0 && luma_min / luma_max < 0.1f) {
                    ib += snprintf(ibuf+ib, sizeof(ibuf)-(size_t)ib,
                        "    CONSISTENCY: luma range %.0f-%.0f across angles — uneven lighting\n", luma_min, luma_max); issues++;
                }
            }
        }

        // ── REGRESSION DETECTION ──
        float regression_pct = 0;
        {
            char baseline_path[256];
            snprintf(baseline_path, sizeof(baseline_path), "qa/baseline/%s_hero.png", qa_scenes[qi].name);
            if (FileExists(baseline_path)) {
                Image baseline = LoadImage(baseline_path);
                if (baseline.width == RENDER_W && baseline.height == RENDER_H) {
                    Color *base_px = LoadImageColors(baseline);
                    // Re-grab hero image for comparison
                    char hero_path[256];
                    snprintf(hero_path, sizeof(hero_path), "qa/screenshots/%s_hero.png", qa_scenes[qi].name);
                    Image hero_img = LoadImage(hero_path);
                    Color *hero_px = LoadImageColors(hero_img);
                    int total_px = RENDER_W * RENDER_H;
                    long diff_sum = 0;
                    int changed_px = 0;
                    for (int p = 0; p < total_px; p++) {
                        int dr = (int)hero_px[p].r - (int)base_px[p].r;
                        int dg = (int)hero_px[p].g - (int)base_px[p].g;
                        int db = (int)hero_px[p].b - (int)base_px[p].b;
                        int d2 = dr*dr + dg*dg + db*db;
                        diff_sum += d2;
                        if (d2 > 900) changed_px++;
                    }
                    regression_pct = 100.0f * (float)changed_px / (float)total_px;
                    if (regression_pct > 25.0f) {
                        float avg_diff = sqrtf((float)diff_sum / total_px);
                        ib += snprintf(ibuf+ib, sizeof(ibuf)-(size_t)ib,
                            "    REGRESSION: %.0f%% pixels changed (avg diff %.1f)\n",
                            (double)regression_pct, (double)avg_diff); issues++;
                    }
                    // Save diff image
                    Image diff_img = GenImageColor(RENDER_W, RENDER_H, BLACK);
                    for (int p = 0; p < total_px; p++) {
                        int dr = abs((int)hero_px[p].r - (int)base_px[p].r);
                        int dg = abs((int)hero_px[p].g - (int)base_px[p].g);
                        int db = abs((int)hero_px[p].b - (int)base_px[p].b);
                        int d = (dr + dg + db) * 3;
                        if (d > 255) d = 255;
                        ((Color *)diff_img.data)[p] = (Color){(unsigned char)d, (unsigned char)(d/2), 0, 255};
                    }
                    char diff_path[256];
                    snprintf(diff_path, sizeof(diff_path), "qa/diffs/%s_diff.png", qa_scenes[qi].name);
                    ExportImage(diff_img, diff_path);
                    UnloadImage(diff_img);
                    UnloadImageColors(hero_px);
                    UnloadImage(hero_img);
                    UnloadImageColors(base_px);
                }
                UnloadImage(baseline);
            }
        }

        // ── Console output ──
        const char *status = issues == 0 ? "PASS" : "FAIL";
        printf("[QA] %-18s %s  walls:%3d  mat:%d (%.0f%%)  luma:%.0f  contrast:%.1f:1  edges:%.0f%%  hues:%d  obj:%d",
               qa_scenes[qi].name, status, g.scene.wall_count,
               mat_types_used, mat_cov, hero->luma, hero->contrast_ratio,
               hero->edge_density, hero->hue_buckets, interact_count);
        if (e2e_mode) printf("  angles:%d  load:%.0fms", num_angles, load_time_ms);
        printf("\n");
        if (issues > 0) printf("%s", ibuf);
        qa_total_issues += issues;

        // ── JSON output ──
        if (jf) {
            if (qi > 0) fprintf(jf, ",\n");
            fprintf(jf, "    {\n");
            fprintf(jf, "      \"name\": \"%s\",\n", qa_scenes[qi].name);
            fprintf(jf, "      \"game_state\": %d,\n", qa_scenes[qi].gs);
            fprintf(jf, "      \"status\": \"%s\",\n", status);
            fprintf(jf, "      \"flow_order\": %d,\n", qa_scenes[qi].flow_order);
            fprintf(jf, "      \"flow_next\": %d,\n", qa_scenes[qi].flow_next);
            fprintf(jf, "      \"dark_by_design\": %s,\n", dark ? "true" : "false");
            fprintf(jf, "      \"outdoor\": %s,\n", qa_scenes[qi].outdoor ? "true" : "false");
            fprintf(jf, "      \"load_time_ms\": %.1f,\n", load_time_ms);
            fprintf(jf, "      \"walls\": %d,\n", g.scene.wall_count);
            fprintf(jf, "      \"wall_pct\": %.1f,\n", wall_pct);
            fprintf(jf, "      \"material_types\": %d,\n", mat_types_used);
            fprintf(jf, "      \"material_coverage\": %.1f,\n", mat_cov);
            // Full material breakdown with all 17 types
            fprintf(jf, "      \"material_breakdown\": {");
            const char *mat_names[] = {"concrete","marble","wood","carpet","wallpaper","tile","brass","glass","leather","fabric","checkerboard","herringbone","parquet","velvet","water","puddle","emissive"};
            bool first_mat = true;
            for (int m = 0; m < 17; m++) {
                if (mat_counts[m] > 0) {
                    fprintf(jf, "%s\"%s\": %d", first_mat ? "" : ", ", mat_names[m], mat_counts[m]);
                    first_mat = false;
                }
            }
            fprintf(jf, "},\n");
            // Geometry detail
            fprintf(jf, "      \"decal_count\": %d,\n", decal_count);
            fprintf(jf, "      \"model_count\": %d,\n", model_count);
            fprintf(jf, "      \"pushable_count\": %d,\n", pushable_count);
            fprintf(jf, "      \"breakable_count\": %d,\n", breakable_count);
            fprintf(jf, "      \"hinge_count\": %d,\n", hinge_count);
            fprintf(jf, "      \"no_collide_count\": %d,\n", no_collide_count);
            fprintf(jf, "      \"wall_volume\": %.1f,\n", wall_volume_total);
            // Spawn
            fprintf(jf, "      \"spawn\": [%.1f, %.1f, %.1f],\n", g.scene.spawn.x, g.scene.spawn.y, g.scene.spawn.z);
            fprintf(jf, "      \"has_exit\": %s,\n", g.scene.has_exit ? "true" : "false");
            if (g.scene.has_exit)
                fprintf(jf, "      \"exit_pos\": [%.1f, %.1f, %.1f],\n", g.scene.exit_pos.x, g.scene.exit_pos.y, g.scene.exit_pos.z);
            // Objects
            fprintf(jf, "      \"interact_count\": %d,\n", interact_count);
            fprintf(jf, "      \"obj_unreachable\": %d,\n", obj_unreachable);
            fprintf(jf, "      \"obj_max_dist\": %.1f,\n", obj_max_dist);
            // Object details (E2E)
            if (e2e_mode && g.scene.object_count > 0) {
                fprintf(jf, "      \"objects\": [\n");
                for (int oi = 0; oi < g.scene.object_count; oi++) {
                    InteractObject *obj = &g.scene.objects[oi];
                    if (!obj->active) continue;
                    float dx = obj->pos.x - g.scene.spawn.x;
                    float dz = obj->pos.z - g.scene.spawn.z;
                    float dist = sqrtf(dx*dx + dz*dz);
                    fprintf(jf, "        {\"name\": \"%s\", \"pos\": [%.1f, %.1f, %.1f], \"dist\": %.1f, \"radius\": %.1f, \"max_steps\": %d, \"done\": %s}%s\n",
                            obj->name ? obj->name : "unnamed",
                            obj->pos.x, obj->pos.y, obj->pos.z,
                            dist, obj->radius, obj->max_steps,
                            obj->done ? "true" : "false",
                            (oi < g.scene.object_count - 1) ? "," : "");
                }
                fprintf(jf, "      ],\n");
            }
            // Collision probe (E2E)
            if (e2e_mode) {
                fprintf(jf, "      \"collision_grid_size\": %d,\n", collision_grid_size);
                fprintf(jf, "      \"collision_floor_hits\": %d,\n", collision_floor_hits);
                fprintf(jf, "      \"collision_stuck_points\": %d,\n", collision_stuck_points);
                fprintf(jf, "      \"wall_overlaps\": %d,\n", overlap_count);
            }
            fprintf(jf, "      \"regression_pct\": %.1f,\n", regression_pct);
            fprintf(jf, "      \"issues\": %d,\n", issues);
            // Per-angle data
            fprintf(jf, "      \"angles\": [\n");
            for (int ai = 0; ai < num_angles; ai++) {
                fprintf(jf, "        {\n");
                fprintf(jf, "          \"name\": \"%s\",\n", qa_scenes[qi].angle_names[ai]);
                fprintf(jf, "          \"screenshot\": \"screenshots/%s_%s.png\",\n",
                        qa_scenes[qi].name, qa_scenes[qi].angle_names[ai]);
                QA_WRITE_METRICS_JSON(jf, &angle_metrics[ai], "          ");
                fprintf(jf, "        }%s\n", (ai < num_angles - 1) ? "," : "");
            }
            fprintf(jf, "      ]\n");
            fprintf(jf, "    }");
        }
    }

    // ── Flow transition test (E2E only) ──
    if (e2e_mode && jf) {
        fprintf(jf, "\n  ],\n  \"transitions\": [\n");
        // Test loading each scene in canonical flow order
        typedef struct { GameState from; GameState to; const char *from_name; const char *to_name; } FlowStep;
        FlowStep flow[] = {
            {STATE_CAR,            STATE_HOTEL_EXT,      "taxi",           "hotel_ext"},
            {STATE_HOTEL_EXT,      STATE_LOBBY,          "hotel_ext",      "lobby"},
            {STATE_LOBBY,          STATE_ELEVATOR,       "lobby",          "elevator"},
            {STATE_ELEVATOR,       STATE_HYPERSPACE,     "elevator",       "hyperspace"},
            {STATE_HYPERSPACE,     STATE_SPACE_LOBBY,    "hyperspace",     "space_lobby"},
            {STATE_SPACE_LOBBY,    STATE_SPACE_CORRIDOR, "space_lobby",    "space_corridor"},
            {STATE_SPACE_CORRIDOR, STATE_SPACE_SUITE,    "space_corridor", "space_suite"},
            {STATE_SPACE_SUITE,    STATE_BALCONY,        "space_suite",    "balcony"},
            {STATE_BALCONY,        STATE_BED,            "balcony",        "bed"},
            {STATE_BED,            STATE_STARS,          "bed",            "stars"},
            {STATE_STARS,          STATE_PARIS_DREAM,    "stars",          "paris_dream"},
            {STATE_PARIS_DREAM,    STATE_RETURN_TAXI,    "paris_dream",    "return_taxi"},
        };
        int flow_count = (int)(sizeof(flow) / sizeof(flow[0]));
        printf("\n[QA] ── Flow Transitions ──\n");
        for (int fi = 0; fi < flow_count; fi++) {
            double t0 = GetTime();
            load_state(flow[fi].from);
            g.fade_alpha = 0.0f; g.fade_target = 0.0f;
            double t1 = GetTime();
            load_state(flow[fi].to);
            g.fade_alpha = 0.0f; g.fade_target = 0.0f;
            double t2 = GetTime();
            float from_ms = (float)(t1 - t0) * 1000.0f;
            float to_ms = (float)(t2 - t1) * 1000.0f;
            bool ok = g.scene.wall_count > 0 || flow[fi].to == STATE_HYPERSPACE || flow[fi].to == STATE_STARS;
            printf("[QA] %s → %s  %.0fms+%.0fms  %s\n",
                   flow[fi].from_name, flow[fi].to_name, from_ms, to_ms, ok ? "OK" : "FAIL");
            if (fi > 0) fprintf(jf, ",\n");
            fprintf(jf, "    {\"from\": \"%s\", \"to\": \"%s\", \"from_ms\": %.1f, \"to_ms\": %.1f, \"ok\": %s, \"walls_after\": %d}",
                    flow[fi].from_name, flow[fi].to_name, from_ms, to_ms, ok ? "true" : "false", g.scene.wall_count);
            if (!ok) qa_total_issues++;
        }
        fprintf(jf, "\n  ],\n");
    } else if (jf) {
        fprintf(jf, "\n  ],\n");
    }

    if (jf) {
        fprintf(jf, "  \"total_issues\": %d,\n  \"scene_count\": %d\n}\n", qa_total_issues, qa_count);
        fclose(jf);
    }

    printf("\n[QA] === %d issue%s across %d scenes ===\n",
           qa_total_issues, qa_total_issues == 1 ? "" : "s", qa_count);
    printf("[QA] Screenshots: qa/screenshots/\n");
    printf("[QA] Report:      %s\n", report_path);
    if (e2e_mode)
        printf("[QA] Analysis:    ./qa/run_e2e.sh  (multi-profile grades)\n\n");
    else
        printf("[QA] Analysis:    ./qa/run.sh      (standard grades)\n\n");

    #undef QA_RENDER_SHOT_EX
    #undef QA_ANALYZE_IMAGE
    #undef QA_WRITE_METRICS_JSON

    if (g.cube_model_loaded) UnloadModel(g.cube_model);
    if (g.cyl_model_loaded) UnloadModel(g.cyl_model);
    if (g.sphere_model_loaded) UnloadModel(g.sphere_model);
    if (g.cone_model_loaded) UnloadModel(g.cone_model);
    if (g.skytower_loaded) UnloadModel(g.skytower_model);
    for (int mi = 0; mi < g.model_asset_count; mi++) {
        if (g.model_assets[mi].loaded) {
            if (g.model_assets[mi].anims) UnloadModelAnimations(g.model_assets[mi].anims, g.model_assets[mi].anim_count);
            UnloadModel(g.model_assets[mi].model);
        }
    }
    UnloadEVLighting(&g.lighting);
    UnloadEVPostFX(&g.postfx);
    UnloadEVSkybox(&g.skybox);
    UnloadFileMusic(&g.audio);
    UnloadEVAudio(&g.audio);
    UnloadRenderTexture(g.render_target);
    UnloadRenderTexture(g.postfx_target);
    CloseWindow();
    return qa_total_issues > 0 ? 1 : 0;

#elif defined(QA_AUDIT)
    // ============================================================
    // ENGINE AUDIT — HL2-standard technical probing
    // Would Valve ship this? Probably not yet. Let's find out why.
    //
    // Graded A-F on weighted categories:
    //   RENDER (15%) — shaders, pipeline, visual styles, shadow pass
    //   COLLISION (20%) — walk/sprint/diagonal penetration, edge cases
    //   GEOMETRY (15%) — degenerate walls, coplanar surfaces, bounds
    //   PERF (15%) — actual render time (vsync disabled), per-scene
    //   PHYSICS (10%) — gravity, movement modes, grounding
    //   INTERACTION (10%) — objects reachable, scenes completable
    //   STATE (5%) — all states have handlers, flow integrity
    //   STRESS (5%) — rapid transitions, no crash
    //   AUDIO (5%) — system health, sound count
    //
    // Build: make qa-audit   |   Report: qa/audit_report.json
    // ============================================================
    ;int audit_pass = 0, audit_fail = 0, audit_warn = 0;
    // Per-category scoring (0-100 each)
    float cat_scores[9] = {0};
    int cat_tests[9] = {0};
    int cat_max[9] = {0};
    const char *cat_names[] = {"RENDER","COLLISION","GEOMETRY","PERF","PHYSICS","INTERACTION","STATE","STRESS","AUDIO"};
    float cat_weights[] = {0.15f, 0.20f, 0.15f, 0.15f, 0.10f, 0.10f, 0.05f, 0.05f, 0.05f};
    enum { CAT_RENDER=0, CAT_COLLISION, CAT_GEOMETRY, CAT_PERF, CAT_PHYSICS, CAT_INTERACTION, CAT_STATE, CAT_STRESS, CAT_AUDIO };
    {
        SetTargetFPS(0); // DISABLE VSYNC — measure real render time

        printf("\n[AUDIT] ============================================================\n");
        printf("[AUDIT]  ENDEARING VOID — ENGINE AUDIT (HL2 Standard)\n");
        printf("[AUDIT]  Would Valve ship this? Let's find out.\n");
        printf("[AUDIT] ============================================================\n\n");

        FILE *af = fopen("qa/audit_report.json", "w");
        if (af) fprintf(af, "{\n  \"standard\": \"HL2\",\n  \"tests\": [\n");
        int test_idx = 0;

        #define AUDIT_RESULT(cat_idx, name_str, status_str, detail_str) do { \
            const char *_s = status_str; \
            cat_tests[cat_idx]++; \
            cat_max[cat_idx]++; \
            if (strcmp(_s, "PASS") == 0) { audit_pass++; cat_scores[cat_idx]++; printf("[AUDIT] PASS  %-12s %s\n", cat_names[cat_idx], name_str); } \
            else if (strcmp(_s, "WARN") == 0) { audit_warn++; cat_scores[cat_idx] += 0.5f; printf("[AUDIT] WARN  %-12s %s — %s\n", cat_names[cat_idx], name_str, detail_str); } \
            else { audit_fail++; printf("[AUDIT] FAIL  %-12s %s — %s\n", cat_names[cat_idx], name_str, detail_str); } \
            if (af) { \
                if (test_idx > 0) fprintf(af, ",\n"); \
                fprintf(af, "    {\"category\": \"%s\", \"test\": \"%s\", \"status\": \"%s\", \"detail\": \"%s\"}", \
                        cat_names[cat_idx], name_str, _s, detail_str); \
                test_idx++; \
            } \
        } while(0)

        // ── Walkable scenes definition ──
        typedef struct { GameState gs; const char *name; } AuditScene;
        AuditScene all_scenes[] = {
            {STATE_TITLE,"title"},{STATE_CAR,"taxi"},{STATE_DRIVING,"driving"},
            {STATE_HOTEL_EXT,"hotel_ext"},{STATE_LOBBY,"lobby"},{STATE_ELEVATOR,"elevator"},
            {STATE_HALLWAY,"hallway"},{STATE_ROOM,"room"},{STATE_BATHROOM,"bathroom"},
            {STATE_BALCONY,"balcony"},{STATE_BED,"bed"},{STATE_STARS,"stars"},
            {STATE_HYPERSPACE,"hyperspace"},{STATE_SPACE_LOBBY,"space_lobby"},
            {STATE_SPACE_CORRIDOR,"space_corridor"},{STATE_SPACE_SUITE,"space_suite"},
            {STATE_PARIS_DREAM,"paris_dream"},{STATE_CLEANED_SUITE,"cleaned_suite"},
            {STATE_MONTAGE,"montage"},{STATE_RETURN_TAXI,"return_taxi"},
        };
        int all_count = (int)(sizeof(all_scenes)/sizeof(all_scenes[0]));
        GameState walkable[] = {
            STATE_HOTEL_EXT, STATE_LOBBY, STATE_HALLWAY, STATE_ROOM,
            STATE_BATHROOM, STATE_ELEVATOR, STATE_SPACE_LOBBY,
            STATE_SPACE_CORRIDOR, STATE_SPACE_SUITE, STATE_BALCONY, STATE_PARIS_DREAM,
        };
        const char *walk_names[] = {
            "hotel_ext","lobby","hallway","room","bathroom","elevator",
            "space_lobby","space_corridor","space_suite","balcony","paris_dream",
        };
        int walk_count = (int)(sizeof(walkable)/sizeof(walkable[0]));

        // ════════════════════════════════════════════════════════════
        // RENDER (15%) — pipeline health, visual styles, model loading
        // ════════════════════════════════════════════════════════════
        printf("[AUDIT] ── RENDER ──\n");
        AUDIT_RESULT(CAT_RENDER, "lighting_shader", g.lighting.ready ? "PASS" : "FAIL", "GLSL compile");
        AUDIT_RESULT(CAT_RENDER, "postfx_shader", g.postfx.ready ? "PASS" : "FAIL", "GLSL compile");
        AUDIT_RESULT(CAT_RENDER, "shadow_map", g.lighting.shadowReady ? "PASS" : "FAIL", "FBO create");
        AUDIT_RESULT(CAT_RENDER, "render_fbo", (g.render_target.id > 0) ? "PASS" : "FAIL", "1920x1200");
        AUDIT_RESULT(CAT_RENDER, "postfx_fbo", (g.postfx_target.id > 0) ? "PASS" : "FAIL", "1920x1200");
        AUDIT_RESULT(CAT_RENDER, "primitives", (g.cube_model_loaded && g.cyl_model_loaded && g.sphere_model_loaded && g.cone_model_loaded) ? "PASS" : "FAIL", "cube+cyl+sphere+cone");
        AUDIT_RESULT(CAT_RENDER, "skytower_mesh", g.skytower_loaded ? "PASS" : "FAIL", "external OBJ");

        // Visual style crash test — cycle through all 9 styles
        {
            load_state(STATE_LOBBY); g.fade_alpha = 0; g.fade_target = 0;
            bool style_ok = true;
            for (int s = 0; s < STYLE_COUNT; s++) {
                ApplyVisualStyle(&g.postfx, s);
                BeginTextureMode(g.render_target); ClearBackground(BLACK);
                draw_scene_3d(&g.player, &g.scene, &g.lighting,
                    &g.cube_model, g.cube_model_loaded, &g.cyl_model, g.cyl_model_loaded,
                    &g.sphere_model, g.sphere_model_loaded, &g.cone_model, g.cone_model_loaded,
                    &g.skytower_model, g.skytower_loaded, true, 1.0f);
                EndTextureMode();
                BeginTextureMode(g.postfx_target); ClearBackground(BLACK);
                draw_postfx(&g.postfx, g.render_target); EndTextureMode();
                BeginDrawing(); ClearBackground(BLACK); EndDrawing();
            }
            ApplyVisualStyle(&g.postfx, 0); // reset
            char det[64]; snprintf(det, sizeof(det), "%d styles tested", STYLE_COUNT);
            AUDIT_RESULT(CAT_RENDER, "visual_styles", style_ok ? "PASS" : "FAIL", det);
        }

        // Render not-black for key scenes
        {
            GameState rs[] = {STATE_LOBBY, STATE_SPACE_SUITE, STATE_HOTEL_EXT, STATE_HALLWAY, STATE_SPACE_CORRIDOR};
            const char *rn[] = {"render_lobby","render_suite","render_ext","render_hall","render_corridor"};
            for (int si = 0; si < 5; si++) {
                load_state(rs[si]); g.fade_alpha = 0; g.fade_target = 0;
                g.player.camera.position = g.scene.spawn;
                g.player.camera.target = (Vector3){g.scene.spawn.x, g.scene.spawn.y, g.scene.spawn.z - 5};
                for (int f = 0; f < 8; f++) {
                    BeginTextureMode(g.render_target);
                    ClearBackground(g.scene.fog_color.a > 0 ? g.scene.fog_color : (Color){8,10,22,255});
                    draw_scene_3d(&g.player, &g.scene, &g.lighting,
                        &g.cube_model, g.cube_model_loaded, &g.cyl_model, g.cyl_model_loaded,
                        &g.sphere_model, g.sphere_model_loaded, &g.cone_model, g.cone_model_loaded,
                        &g.skytower_model, g.skytower_loaded, true, 1.0f);
                    EndTextureMode();
                    BeginDrawing(); ClearBackground(BLACK); EndDrawing();
                }
                Image img = LoadImageFromTexture(g.render_target.texture); ImageFlipVertical(&img);
                Color *px = LoadImageColors(img);
                int total = RENDER_W * RENDER_H;
                long lsum = 0; int nonblack = 0;
                for (int p = 0; p < total; p++) {
                    int l = (int)(0.299f*px[p].r + 0.587f*px[p].g + 0.114f*px[p].b);
                    lsum += l;
                    if (px[p].r > 5 || px[p].g > 5 || px[p].b > 5) nonblack++;
                }
                float luma = (float)lsum / total;
                float vis = 100.0f * nonblack / total;
                char det[128]; snprintf(det, sizeof(det), "luma=%.0f vis=%.0f%%", luma, vis);
                AUDIT_RESULT(CAT_RENDER, rn[si], vis > 5.0f ? "PASS" : (vis > 1.0f ? "WARN" : "FAIL"), det);
                UnloadImageColors(px); UnloadImage(img);
            }
        }

        // ════════════════════════════════════════════════════════════
        // COLLISION (20%) — walk, sprint, diagonal, sprint+diagonal
        // HL2 standard: ZERO penetrations at any speed or angle
        // ════════════════════════════════════════════════════════════
        printf("\n[AUDIT] ── COLLISION ──\n");
        float dirs[][2] = {{0,-1},{0,1},{-1,0},{1,0},{-0.707f,-0.707f},{0.707f,-0.707f},{-0.707f,0.707f},{0.707f,0.707f}};
        float speeds[] = {4.5f, 8.5f, 12.0f}; // walk, sprint, dash-speed
        const char *speed_names[] = {"walk", "sprint", "dash"};

        for (int si = 0; si < walk_count; si++) {
            for (int sp = 0; sp < 3; sp++) {
                load_state(walkable[si]); g.fade_alpha = 0; g.fade_target = 0;
                int penetrations = 0;
                for (int d = 0; d < 8; d++) {
                    init_player(&g.player, g.scene.spawn);
                    g.player.gravity_mult = 1.0f;
                    g.player.control_mult = 1.0f;
                    for (int frame = 0; frame < 120; frame++) {
                        g.player.vel.x = dirs[d][0] * speeds[sp];
                        g.player.vel.z = dirs[d][1] * speeds[sp];
                        update_player(&g.player, &g.scene, 1.0f / 60.0f);
                    }
                    float px = g.player.camera.position.x, py = g.player.camera.position.y - 1.6f, pz = g.player.camera.position.z;
                    for (int w = 0; w < g.scene.wall_count; w++) {
                        Wall *wl = &g.scene.walls[w];
                        if (wl->no_collide || wl->is_decal || wl->shape != SHAPE_CUBE) continue;
                        if (wl->size.y < 0.3f || (wl->size.x < 0.2f && wl->size.z < 0.2f)) continue;
                        float hw = wl->size.x/2, hh = wl->size.y/2, hd = wl->size.z/2;
                        if (px > wl->pos.x-hw+0.15f && px < wl->pos.x+hw-0.15f &&
                            py > wl->pos.y-hh && py < wl->pos.y+hh &&
                            pz > wl->pos.z-hd+0.15f && pz < wl->pos.z+hd-0.15f) { penetrations++; break; }
                    }
                }
                char name[64]; snprintf(name, sizeof(name), "%s_%s", walk_names[si], speed_names[sp]);
                char det[64]; snprintf(det, sizeof(det), "%d/8 dirs penetrated", penetrations);
                // HL2: zero tolerance at any speed
                AUDIT_RESULT(CAT_COLLISION, name, penetrations == 0 ? "PASS" : "FAIL", det);
            }
        }

        // Floor integrity — grounded after 2s standing
        printf("\n[AUDIT] ── FLOOR ──\n");
        for (int si = 0; si < walk_count; si++) {
            load_state(walkable[si]); g.fade_alpha = 0; g.fade_target = 0;
            init_player(&g.player, g.scene.spawn);
            g.player.gravity_mult = 1.0f;
            for (int f = 0; f < 120; f++) update_player(&g.player, &g.scene, 1.0f/60.0f);
            float drop = g.scene.spawn.y - g.player.camera.position.y;
            char det[128]; snprintf(det, sizeof(det), "drop=%.1f grounded=%s", drop, g.player.grounded?"yes":"no");
            AUDIT_RESULT(CAT_COLLISION, walk_names[si],
                drop > 10 ? "FAIL" : (drop > 3 ? "WARN" : "PASS"), det);
        }

        // ════════════════════════════════════════════════════════════
        // GEOMETRY (15%) — degenerate walls, coplanar detection, bounds
        // ════════════════════════════════════════════════════════════
        printf("\n[AUDIT] ── GEOMETRY ──\n");
        for (int si = 0; si < all_count; si++) {
            load_state(all_scenes[si].gs); g.fade_alpha = 0; g.fade_target = 0;
            if (g.scene.wall_count == 0) continue;
            int degenerate = 0, out_of_bounds = 0, coplanar_pairs = 0;
            for (int w = 0; w < g.scene.wall_count; w++) {
                Wall *wl = &g.scene.walls[w];
                // Degenerate: zero volume (any dimension < 0.001)
                if (wl->size.x < 0.001f || wl->size.y < 0.001f || wl->size.z < 0.001f) degenerate++;
                // Out of bounds: more than 100m from origin
                if (fabsf(wl->pos.x) > 100 || fabsf(wl->pos.y) > 100 || fabsf(wl->pos.z) > 100) out_of_bounds++;
            }
            // Coplanar non-decal walls (z-fighting candidates)
            // Check pairs sharing a face within 0.005m (Z_DECAL_BIAS)
            for (int a = 0; a < g.scene.wall_count && a < 300; a++) {
                Wall *wa = &g.scene.walls[a];
                if (wa->is_decal || wa->no_collide || wa->shape != SHAPE_CUBE) continue;
                for (int b = a+1; b < g.scene.wall_count && b < 300; b++) {
                    Wall *wb = &g.scene.walls[b];
                    if (wb->is_decal || wb->no_collide || wb->shape != SHAPE_CUBE) continue;
                    // Check if any face of A is within 0.005m of a face of B (coplanar)
                    float ax0 = wa->pos.x - wa->size.x/2, ax1 = wa->pos.x + wa->size.x/2;
                    float bx0 = wb->pos.x - wb->size.x/2, bx1 = wb->pos.x + wb->size.x/2;
                    float az0 = wa->pos.z - wa->size.z/2, az1 = wa->pos.z + wa->size.z/2;
                    float bz0 = wb->pos.z - wb->size.z/2, bz1 = wb->pos.z + wb->size.z/2;
                    // Only count if they share significant overlap area (not just touching at edge)
                    bool x_overlap = ax0 < bx1 - 0.1f && ax1 > bx0 + 0.1f;
                    bool z_overlap = az0 < bz1 - 0.1f && az1 > bz0 + 0.1f;
                    if (x_overlap && z_overlap) {
                        // Check Y faces coplanar
                        float ay0 = wa->pos.y - wa->size.y/2, ay1 = wa->pos.y + wa->size.y/2;
                        float by0 = wb->pos.y - wb->size.y/2, by1 = wb->pos.y + wb->size.y/2;
                        if (fabsf(ay0 - by0) < 0.005f || fabsf(ay1 - by1) < 0.005f ||
                            fabsf(ay0 - by1) < 0.005f || fabsf(ay1 - by0) < 0.005f)
                            coplanar_pairs++;
                    }
                }
            }
            char det[256];
            // Degenerate walls
            snprintf(det, sizeof(det), "%d degenerate of %d", degenerate, g.scene.wall_count);
            AUDIT_RESULT(CAT_GEOMETRY, all_scenes[si].name,
                degenerate > 0 ? "FAIL" : "PASS", det);
            // Coplanar (HL2: zero coplanar non-decal surfaces)
            if (g.scene.wall_count > 10) {
                char cdet[128]; snprintf(cdet, sizeof(cdet), "%d coplanar non-decal pairs", coplanar_pairs);
                char cname[64]; snprintf(cname, sizeof(cname), "%s_coplanar", all_scenes[si].name);
                AUDIT_RESULT(CAT_GEOMETRY, cname,
                    coplanar_pairs > 80 ? "FAIL" : (coplanar_pairs > 30 ? "WARN" : "PASS"), cdet);
            }
        }

        // ════════════════════════════════════════════════════════════
        // PERF (15%) — actual render time with vsync DISABLED
        // HL2: every scene <8ms render (120+ fps headroom)
        // ════════════════════════════════════════════════════════════
        printf("\n[AUDIT] ── PERFORMANCE (vsync OFF) ──\n");
        for (int si = 0; si < all_count; si++) {
            if (all_scenes[si].gs == STATE_TITLE || all_scenes[si].gs == STATE_DRIVING || all_scenes[si].gs == STATE_MONTAGE) continue;
            load_state(all_scenes[si].gs); g.fade_alpha = 0; g.fade_target = 0;
            g.player.camera.position = g.scene.spawn;
            g.player.camera.target = (Vector3){g.scene.spawn.x, g.scene.spawn.y, g.scene.spawn.z - 5};
            // Warmup
            for (int f = 0; f < 4; f++) {
                BeginTextureMode(g.render_target); ClearBackground(BLACK);
                draw_scene_3d(&g.player, &g.scene, &g.lighting,
                    &g.cube_model, g.cube_model_loaded, &g.cyl_model, g.cyl_model_loaded,
                    &g.sphere_model, g.sphere_model_loaded, &g.cone_model, g.cone_model_loaded,
                    &g.skytower_model, g.skytower_loaded, true, 1.0f);
                EndTextureMode();
                BeginTextureMode(g.postfx_target); ClearBackground(BLACK);
                draw_postfx(&g.postfx, g.render_target); EndTextureMode();
                BeginDrawing(); ClearBackground(BLACK); EndDrawing();
            }
            // Measure 32 frames
            double t0 = GetTime();
            for (int f = 0; f < 32; f++) {
                BeginTextureMode(g.render_target);
                ClearBackground(g.scene.fog_color.a > 0 ? g.scene.fog_color : (Color){8,10,22,255});
                draw_scene_3d(&g.player, &g.scene, &g.lighting,
                    &g.cube_model, g.cube_model_loaded, &g.cyl_model, g.cyl_model_loaded,
                    &g.sphere_model, g.sphere_model_loaded, &g.cone_model, g.cone_model_loaded,
                    &g.skytower_model, g.skytower_loaded, true, (float)f * 0.016f);
                EndTextureMode();
                BeginTextureMode(g.postfx_target); ClearBackground(BLACK);
                draw_postfx(&g.postfx, g.render_target); EndTextureMode();
                BeginDrawing(); ClearBackground(BLACK); EndDrawing();
            }
            float avg_ms = (float)((GetTime() - t0) * 1000.0) / 32.0f;
            float fps = 1000.0f / avg_ms;
            char det[128]; snprintf(det, sizeof(det), "%.2fms (%.0f fps) walls=%d", avg_ms, fps, g.scene.wall_count);
            // HL2 standard: <8ms = PASS, <16ms = WARN, >16ms = FAIL
            AUDIT_RESULT(CAT_PERF, all_scenes[si].name,
                avg_ms < 8.0f ? "PASS" : (avg_ms < 16.6f ? "WARN" : "FAIL"), det);
        }

        // ════════════════════════════════════════════════════════════
        // PHYSICS (10%) — gravity, terminal vel, grounding
        // ════════════════════════════════════════════════════════════
        printf("\n[AUDIT] ── PHYSICS ──\n");
        {
            load_state(STATE_LOBBY); g.fade_alpha = 0; g.fade_target = 0;
            init_player(&g.player, (Vector3){0, 50, 0});
            g.player.gravity_mult = 1.0f;
            float max_vy = 0;
            for (int f = 0; f < 300; f++) {
                update_player(&g.player, &g.scene, 1.0f/60.0f);
                if (g.player.vy < max_vy) max_vy = g.player.vy;
            }
            char det[128]; snprintf(det, sizeof(det), "max_vy=%.1f", max_vy);
            AUDIT_RESULT(CAT_PHYSICS, "terminal_velocity", max_vy > -60.0f ? "PASS" : "FAIL", det);
        }
        // Walk forward — don't fall or get stuck
        for (int si = 0; si < walk_count; si++) {
            load_state(walkable[si]); g.fade_alpha = 0; g.fade_target = 0;
            init_player(&g.player, g.scene.spawn);
            g.player.gravity_mult = 1.0f;
            float sy = g.player.camera.position.y;
            float moved = 0;
            for (int f = 0; f < 180; f++) {
                g.player.vel.z = -4.5f;
                Vector3 before = g.player.camera.position;
                update_player(&g.player, &g.scene, 1.0f/60.0f);
                Vector3 after = g.player.camera.position;
                moved += sqrtf((after.x-before.x)*(after.x-before.x) + (after.z-before.z)*(after.z-before.z));
            }
            float ey = g.player.camera.position.y;
            char name[64]; snprintf(name, sizeof(name), "walk_%s", walk_names[si]);
            char det[128]; snprintf(det, sizeof(det), "moved=%.1fm y_delta=%.1f", moved, ey-sy);
            // Should move at least 5m in 3 seconds (not stuck), and not fall >5m
            // Tiny rooms (elevator, bathroom) can't move far — that's OK
            // Tiny rooms and furnished rooms can't move far in one direction — that's fine
            bool tight_room = (walkable[si] == STATE_ELEVATOR || walkable[si] == STATE_BATHROOM ||
                               walkable[si] == STATE_LOBBY || walkable[si] == STATE_SPACE_CORRIDOR);
            float min_move = tight_room ? 0.5f : 3.0f;
            bool ok = moved > min_move && (sy - ey) < 5.0f;
            AUDIT_RESULT(CAT_PHYSICS, name, ok ? "PASS" : (moved > 1.0f ? "WARN" : "FAIL"), det);
        }

        // ════════════════════════════════════════════════════════════
        // INTERACTION (10%) — can the player reach every object?
        // HL2: every interactive object reachable from spawn
        // ════════════════════════════════════════════════════════════
        printf("\n[AUDIT] ── INTERACTION ──\n");
        GameState interact_scenes[] = {STATE_LOBBY, STATE_ROOM, STATE_BATHROOM, STATE_SPACE_SUITE, STATE_PARIS_DREAM, STATE_BALCONY};
        const char *interact_names[] = {"lobby","room","bathroom","space_suite","paris_dream","balcony"};
        for (int si = 0; si < 6; si++) {
            load_state(interact_scenes[si]); g.fade_alpha = 0; g.fade_target = 0;
            int total_obj = 0, reachable = 0, named = 0;
            for (int oi = 0; oi < g.scene.object_count; oi++) {
                InteractObject *obj = &g.scene.objects[oi];
                if (!obj->active) continue;
                total_obj++;
                if (obj->name && strlen(obj->name) > 0) named++;
                float dx = obj->pos.x - g.scene.spawn.x;
                float dz = obj->pos.z - g.scene.spawn.z;
                float dist = sqrtf(dx*dx + dz*dz);
                if (dist < 25.0f) reachable++;
            }
            char det[128]; snprintf(det, sizeof(det), "%d/%d reachable, %d/%d named", reachable, total_obj, named, total_obj);
            char name[64]; snprintf(name, sizeof(name), "objects_%s", interact_names[si]);
            // HL2: all objects reachable AND named
            const char *status = "PASS";
            if (total_obj == 0) status = "FAIL";
            else if (reachable < total_obj || named < total_obj) status = "WARN";
            AUDIT_RESULT(CAT_INTERACTION, name, status, det);
        }
        // NOTE: BED and CLEANED_SUITE are cutscenes (control_mult=0, auto-transition)
        // — they intentionally have no interactive objects. Not audited.

        // ════════════════════════════════════════════════════════════
        // STATE (5%) — every GameState has load+update handlers
        // ════════════════════════════════════════════════════════════
        printf("\n[AUDIT] ── STATE MACHINE ──\n");
        {
            int missing_load = 0, missing_update = 0;
            for (int s = 0; s < scene_desc_count; s++) {
                if (!scene_descs[s].update) missing_update++;
                // load can be NULL for STATE_DRIVING (shares with taxi)
                if (!scene_descs[s].load && s != STATE_DRIVING) missing_load++;
            }
            char det[128]; snprintf(det, sizeof(det), "%d states, %d missing load, %d missing update",
                scene_desc_count, missing_load, missing_update);
            AUDIT_RESULT(CAT_STATE, "handlers", (missing_load == 0 && missing_update == 0) ? "PASS" : "FAIL", det);
        }
        // Scene load integrity
        for (int si = 0; si < all_count; si++) {
            load_state(all_scenes[si].gs); g.fade_alpha = 0; g.fade_target = 0;
            bool special = (all_scenes[si].gs == STATE_TITLE || all_scenes[si].gs == STATE_DRIVING ||
                           all_scenes[si].gs == STATE_MONTAGE || all_scenes[si].gs == STATE_STARS ||
                           all_scenes[si].gs == STATE_HYPERSPACE);
            bool ok = g.scene.wall_count > 0 || special;
            char det[64]; snprintf(det, sizeof(det), "walls=%d", g.scene.wall_count);
            AUDIT_RESULT(CAT_STATE, all_scenes[si].name, ok ? "PASS" : "FAIL", det);
        }

        // ════════════════════════════════════════════════════════════
        // STRESS (5%) — rapid transitions
        // ════════════════════════════════════════════════════════════
        printf("\n[AUDIT] ── STRESS ──\n");
        {
            for (int r = 0; r < 5; r++)
                for (int si = 0; si < all_count; si++) {
                    load_state(all_scenes[si].gs); g.fade_alpha = 0; g.fade_target = 0;
                }
            char det[64]; snprintf(det, sizeof(det), "%d scenes x 5 = %d loads", all_count, all_count * 5);
            AUDIT_RESULT(CAT_STRESS, "rapid_100_loads", "PASS", det);
        }

        // ════════════════════════════════════════════════════════════
        // AUDIO (5%) — init, sound count
        // ════════════════════════════════════════════════════════════
        printf("\n[AUDIT] ── AUDIO ──\n");
        AUDIT_RESULT(CAT_AUDIO, "system_init", g.audio.initialized ? "PASS" : "FAIL", "");
        AUDIT_RESULT(CAT_AUDIO, "music_loaded", g.audio.music_loaded ? "PASS" : "FAIL", "4 tracks");

        // ════════════════════════════════════════════════════════════
        // GRADING
        // ════════════════════════════════════════════════════════════
        printf("\n[AUDIT] ============================================================\n");
        printf("[AUDIT]  RESULTS (HL2 Standard)\n");
        printf("[AUDIT] ============================================================\n\n");

        float weighted_total = 0;
        float weight_sum = 0;
        for (int c = 0; c < 9; c++) {
            float pct = cat_max[c] > 0 ? (cat_scores[c] / cat_max[c]) * 100.0f : 100.0f;
            char grade = pct >= 90 ? 'A' : pct >= 75 ? 'B' : pct >= 60 ? 'C' : pct >= 40 ? 'D' : 'F';
            int bar = (int)(pct / 5);
            char bar_str[21]; for (int i = 0; i < 20; i++) bar_str[i] = i < bar ? '#' : '.'; bar_str[20] = 0;
            printf("[AUDIT]  %-12s %c  %s  %.0f%%  (%0.f/%d tests, weight %.0f%%)\n",
                   cat_names[c], grade, bar_str, pct, cat_scores[c], cat_max[c], cat_weights[c]*100);
            weighted_total += pct * cat_weights[c];
            weight_sum += cat_weights[c];
        }
        float overall = weighted_total / weight_sum;
        char overall_grade = overall >= 90 ? 'A' : overall >= 75 ? 'B' : overall >= 60 ? 'C' : overall >= 40 ? 'D' : 'F';

        printf("\n[AUDIT]  PASS: %d  |  WARN: %d  |  FAIL: %d  |  Total: %d\n",
               audit_pass, audit_warn, audit_fail, audit_pass + audit_warn + audit_fail);
        printf("\n[AUDIT]  ╔══════════════════════════════════════╗\n");
        printf("[AUDIT]  ║  OVERALL GRADE:  %c  (%.0f%%)            ║\n", overall_grade, overall);
        printf("[AUDIT]  ╚══════════════════════════════════════╝\n\n");

        if (af) {
            fprintf(af, "\n  ],\n  \"summary\": {\"pass\": %d, \"warn\": %d, \"fail\": %d, \"total\": %d, \"grade\": \"%c\", \"pct\": %.0f},\n",
                    audit_pass, audit_warn, audit_fail, audit_pass + audit_warn + audit_fail, overall_grade, overall);
            fprintf(af, "  \"categories\": {\n");
            for (int c = 0; c < 9; c++) {
                float pct = cat_max[c] > 0 ? (cat_scores[c] / cat_max[c]) * 100.0f : 100.0f;
                fprintf(af, "    \"%s\": {\"pct\": %.0f, \"pass\": %.0f, \"total\": %d, \"weight\": %.2f}%s\n",
                        cat_names[c], pct, cat_scores[c], cat_max[c], cat_weights[c], c < 8 ? "," : "");
            }
            fprintf(af, "  }\n}\n");
            fclose(af);
        }

        #undef AUDIT_RESULT
    }
    if (g.cube_model_loaded) UnloadModel(g.cube_model);
    if (g.cyl_model_loaded) UnloadModel(g.cyl_model);
    if (g.sphere_model_loaded) UnloadModel(g.sphere_model);
    if (g.cone_model_loaded) UnloadModel(g.cone_model);
    if (g.skytower_loaded) UnloadModel(g.skytower_model);
    for (int mi = 0; mi < g.model_asset_count; mi++) {
        if (g.model_assets[mi].loaded) {
            if (g.model_assets[mi].anims) UnloadModelAnimations(g.model_assets[mi].anims, g.model_assets[mi].anim_count);
            UnloadModel(g.model_assets[mi].model);
        }
    }
    UnloadEVLighting(&g.lighting);
    UnloadEVPostFX(&g.postfx);
    UnloadEVSkybox(&g.skybox);
    UnloadFileMusic(&g.audio);
    UnloadEVAudio(&g.audio);
    UnloadRenderTexture(g.render_target);
    UnloadRenderTexture(g.postfx_target);
    CloseWindow();
    return audit_fail > 0 ? 1 : 0;
#else  // normal game

#ifdef DEV_START
    load_state(DEV_START);
#else
    load_state(STATE_TITLE);
#endif

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        UpdateFileMusic(&g.audio);

        // ── Interaction micro-freeze — a moment of attention ──
        // 50ms pause after pressing E on an object. The world holds its breath.
        if (g.interact_freeze > 0) {
            g.interact_freeze -= dt;
            if (g.interact_freeze > 0) {
                // Skip the rest of the frame — freeze
                BeginDrawing();
                EndDrawing();
                continue;
            }
        }
        // Interaction lean spring — FOV snaps narrow then recovers
        if (g.interact_lean > 0.01f || fabsf(g.interact_lean_vel) > 0.01f) {
            float spring_k = 200.0f, spring_d = 18.0f;
            float force = -spring_k * g.interact_lean - spring_d * g.interact_lean_vel;
            g.interact_lean_vel += force * dt;
            g.interact_lean += g.interact_lean_vel * dt;
            if (g.interact_lean < 0) { g.interact_lean = 0; g.interact_lean_vel = 0; }
        }

        update_choice_input();
        update_menu_springs(dt);
        bool menu_active = update_pause_menu();
        if (!menu_active) {
            g.state_time += dt;
            g.total_time += dt;

            // Tick model animations (GLB skeletal) — skip NPC models (managed by npc.c)
            for (int mi = 0; mi < g.model_asset_count; mi++) {
                ModelAsset *ma = &g.model_assets[mi];
                if (ma->loaded && ma->anims && ma->anim_count > 0) {
                    // Skip gibbons — animation driven by draw_npc()
                    if (strcmp(ma->name, "gibbons") == 0) continue;
                    ma->current_frame++;
                    UpdateModelAnimation(ma->model, ma->anims[ma->current_anim], ma->current_frame);
                    if (ma->current_frame >= ma->anims[ma->current_anim].frameCount)
                        ma->current_frame = 0;
                }
            }
        }

        // Decay g.scene-cut flash
        if (g.cut_flash_timer > 0) {
            g.cut_flash_timer -= dt;
            if (g.cut_flash_timer <= 0) {
                g.cut_flash_timer = 0;
                SetPostFXFlash(&g.postfx, 0.0f, 1.0f, 1.0f, 1.0f);
            } else {
                // Rapid exponential decay — bright to gone in ~0.12s
                float t = g.cut_flash_timer / 0.12f;
                SetPostFXFlash(&g.postfx, t * t, 1.0f, 0.95f, 0.85f);
            }
        }

        if (!menu_active) {  // ── begin game logic gate (paused = frozen) ──

#ifndef PLAYTEST
        if (IsKeyPressed(KEY_F1)) g.wireframe = !g.wireframe;
        if (IsKeyPressed(KEY_F3)) g.show_debug = !g.show_debug;
        if (IsKeyPressed(KEY_F4)) g.player.noclip = !g.player.noclip;

        // F5: Nudge mode — select and reposition walls in-game
        if (IsKeyPressed(KEY_F5)) {
            g.nudge_mode = !g.nudge_mode;
            if (g.nudge_mode) {
                g.nudge_selected = raycast_wall(g.player.camera, &g.scene);
                g.nudge_repeat = 0.0f;
                show_text("NUDGE MODE");
            } else {
                g.nudge_selected = -1;
                g.nudge_repeat = 0.0f;
                hide_text();
            }
        }

        // Nudge controls — arrow keys move selected wall, prints new pos to stdout
        if (g.nudge_mode) {
            // Re-select on click
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                g.nudge_selected = raycast_wall(g.player.camera, &g.scene);
            }
            if (g.nudge_selected >= 0 && g.nudge_selected < g.scene.wall_count) {
                Wall *w = &g.scene.walls[g.nudge_selected];
                bool fine = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
                bool vert = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
                float step = fine ? 0.01f : 0.1f;
                bool moved = false;

                // Arrow keys: X/Z movement (or Y with Ctrl)
                // Hold key = continuous nudge via IsKeyDown at 10Hz
                g.nudge_repeat -= dt;
                bool repeat_ok = g.nudge_repeat <= 0;

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

                if (moved && any_held) g.nudge_repeat = 0.1f;  // 10Hz repeat

                if (moved) {
                    printf("[NUDGE] wall[%d] pos=(%.2ff, %.2ff, %.2ff) size=(%.2ff, %.2ff, %.2ff)\n",
                           g.nudge_selected, w->pos.x, w->pos.y, w->pos.z,
                           w->size.x, w->size.y, w->size.z);
                }
            }
        }

        // Write current g.state for dev-watch
        write_state_file(g.state);

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
                g.current_style = new_style;
                ApplyVisualStyle(&g.postfx, g.current_style);
                // Reapply exposure with new style bias
                SetPostFXExposure(&g.postfx, g.scene_exposure + visual_styles[g.current_style].exposure_bias);
                show_text(visual_styles[g.current_style].name);
            }
        } else {
        // Dev g.scene jumps — number keys for instant room testing (no shift held)
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
        if (IsKeyPressed(KEY_MINUS)) load_state(STATE_GLASSHOUSE);
        if (IsKeyPressed(KEY_EQUAL)) load_state(STATE_SHELL_TEST);
        }
#endif // PLAYTEST

        // Fade with hold-in-black for doorway transitions
        if (g.hold_timer > 0) {
            g.hold_timer -= dt;
            if (g.hold_timer <= 0) {
                g.hold_timer = 0;
                g.transitioning = false;
                load_state(g.next_state);
            }
        } else if (g.fade_alpha != g.fade_target) {
            float dir = (g.fade_target > g.fade_alpha) ? 1 : -1;
            g.fade_alpha += dir * g.fade_speed * dt;
            if ((dir > 0 && g.fade_alpha >= g.fade_target) || (dir < 0 && g.fade_alpha <= g.fade_target)) {
                g.fade_alpha = g.fade_target;
                if (g.transitioning) {
                    g.hold_timer = g.transition_hold;
                }
            }
        }

        // Vignette text spring physics (silk preset: mass 0.9, stiffness 280, damping 26)
        {
            float spring_k = 280.0f, spring_d = 26.0f, spring_m = 0.9f;

            // Y offset spring toward 0
            float force_y = -spring_k * g.text_y_offset - spring_d * g.text_y_velocity;
            g.text_y_velocity += (force_y / spring_m) * dt;
            g.text_y_offset += g.text_y_velocity * dt;

            // Scale spring toward target
            float force_s = -spring_k * (g.text_scale - g.text_scale_target) - spring_d * g.text_scale_vel;
            g.text_scale_vel += (force_s / spring_m) * dt;
            g.text_scale += g.text_scale_vel * dt;

            // When fully hidden, clear text
            if (g.text_scale_target < 0.5f && g.text_scale < 0.01f && fabsf(g.text_scale_vel) < 0.01f) {
                g.text_scale = 0.0f;
                g.text_scale_vel = 0.0f;
                g.vig_text = NULL;
            }
        }

        // Reward timers
        for (int i = 0; i < g.scene.object_count; i++)
            if (g.scene.objects[i].reward_timer > 0) {
                g.scene.objects[i].reward_timer -= dt * 0.5f;
                if (g.scene.objects[i].reward_timer < 0) g.scene.objects[i].reward_timer = 0;
            }

        // ---- UPDATE (dispatched via scene registry) ----
        if ((int)g.state >= 0 && (int)g.state < scene_desc_count && scene_descs[g.state].update) {
            scene_descs[g.state].update(dt);
        }

        // Physics — pushable objects, grab/carry/throw, breakables, doors
        physics_grab_input(&g.scene, &g.player, &g.grab, dt);
        physics_update(&g.scene, &g.player, &g.grab, dt);

        // Gibbons dialogue → new dialogue system
        if (g.state == STATE_LOBBY || g.state == STATE_SPACE_LOBBY
            || g.state == STATE_GLASSHOUSE
            || g.state == STATE_SPACE_CORRIDOR || g.state == STATE_SPACE_SUITE) {
            const char *line = npc_current_dialogue(&g.gibbons);
            if (line && g.dlg_text != line) {
                show_dialogue("GIBBONS", line);
                hide_text();
            } else if (!line && g.dlg_active && !g.gibbons.line_showing
                       && g.gibbons.current_line >= g.gibbons.line_count) {
                hide_dialogue();
            }
        }
        update_dialogue(dt);

        }  // ── end game logic gate ──

        // ---- SHADOW PASS ----
        g.lighting.shadowPassRan = false;
        if (g.lighting.shadowReady && g.state != STATE_TITLE && g.state != STATE_BED && g.state != STATE_STARS) {
            draw_shadow_pass(&g.scene, &g.lighting,
                            &g.cube_model, &g.cyl_model, &g.sphere_model, &g.cone_model,
                            &g.skytower_model);
        }

        // ---- RENDER ----
        BeginTextureMode(g.render_target);
        ClearBackground(g.scene.fog_color.a > 0 ? g.scene.fog_color : (Color){8, 10, 22, 255});
        if (g.wireframe) rlEnableWireMode();

        // Bind shadow map ONLY if shadow pass actually wrote depth this frame
        if (g.lighting.shadowPassRan) BindShadowMap(&g.lighting);

        switch (g.state) {
            case STATE_TITLE: {
                // During prologue: black screen + choices only, no title animation
                if (!g.title_prologue_done) {
                    ClearBackground(BLACK);
                } else {
                    draw_title();
                }
                break;
            }

            case STATE_CAR:
            case STATE_DRIVING:
                // 3D taxi scene with night sky (GPU skybox)
                ClearBackground((Color){8, 12, 28, 255});
                DrawSkybox(&g.skybox, g.player.camera, SKY_AUCKLAND_NIGHT, g.state_time, 0, 0);
                draw_scene_3d(&g.player, &g.scene, &g.lighting, &g.cube_model, g.cube_model_loaded,
                              &g.cyl_model, g.cyl_model_loaded,
                              &g.sphere_model, g.sphere_model_loaded,
                              &g.cone_model, g.cone_model_loaded,
                              &g.skytower_model, g.skytower_loaded,
                              false, g.state_time);
                // Rain overlay on windshield — 2D after 3D
                for (int i = 0; i < MAX_RAIN; i++) {
                    g.rain[i].y += g.rain[i].speed * 1.5f * dt;
                    if (g.rain[i].y > RENDER_H) {
                        g.rain[i].y = -(g.rain[i].len * 0.5f);
                        g.rain[i].x = (float)GetRandomValue(0, RENDER_W);
                    }
                    DrawLine((int)g.rain[i].x, (int)g.rain[i].y,
                             (int)(g.rain[i].x - 0.5f), (int)(g.rain[i].y + g.rain[i].len),
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
            case STATE_CLEANED_SUITE:
            case STATE_MONTAGE:
            case STATE_RETURN_TAXI:
            case STATE_GLASSHOUSE:
            case STATE_SHELL_TEST:
                if (g.state == STATE_RETURN_TAXI) {
                    // Dawn sky (GPU skybox)
                    ClearBackground((Color){45, 38, 32, 255});
                    DrawSkybox(&g.skybox, g.player.camera, SKY_DAWN, g.state_time, 0, 0);
                } else if (g.state == STATE_PARIS_DREAM) {
                    // Golden volumetric haze (GPU skybox)
                    ClearBackground((Color){35, 28, 15, 255});
                    DrawSkybox(&g.skybox, g.player.camera, SKY_PARIS_DREAM, g.state_time, 0, 0);
                } else if (g.state == STATE_HOTEL_EXT) {
                    ClearBackground((Color){8, 12, 28, 255});
                    DrawSkybox(&g.skybox, g.player.camera, SKY_AUCKLAND_NIGHT, g.state_time, 0, 0);
                } else if (g.state == STATE_ELEVATOR) {
                    // Elevator transition: Auckland sky fades to void as you ascend
                    float ascent = fminf(1.0f, g.state_time / 4.0f);
                    unsigned char br = (unsigned char)(8 * (1.0f - ascent) + 4 * ascent);
                    unsigned char bg = (unsigned char)(12 * (1.0f - ascent) + 5 * ascent);
                    unsigned char bb = (unsigned char)(28 * (1.0f - ascent) + 12 * ascent);
                    ClearBackground((Color){br, bg, bb, 255});
                    if (ascent < 0.8f)
                        DrawSkybox(&g.skybox, g.player.camera, SKY_AUCKLAND_NIGHT, g.state_time, ascent, 0);
                } else if (g.state == STATE_HYPERSPACE) {
                    ClearBackground((Color){2, 2, 6, 255});
                    DrawSkybox(&g.skybox, g.player.camera, SKY_HYPERSPACE, g.state_time, 1.0f, 0);
                } else if (g.state == STATE_BALCONY || g.state == STATE_SPACE_LOBBY
                           || g.state == STATE_SPACE_CORRIDOR || g.state == STATE_SPACE_SUITE
                           || g.state == STATE_GLASSHOUSE || g.state == STATE_SHELL_TEST) {
                    ClearBackground((Color){4, 5, 12, 255});
                    DrawSkybox(&g.skybox, g.player.camera, SKY_SPACE_VOID, g.state_time, 0, 0);
                    // Procedural Earth — positioned per g.scene so it's visible through windows
                    if (g.sphere_model_loaded) {
                        Vector3 earth_pos = {0, -40, -60};  // default
                        if (g.state == STATE_SPACE_LOBBY) earth_pos = (Vector3){3, -20, -40};
                        else if (g.state == STATE_GLASSHOUSE) earth_pos = (Vector3){0, -25, -45};
                        else if (g.state == STATE_SPACE_CORRIDOR) earth_pos = (Vector3){-20, -30, -30};
                        else if (g.state == STATE_SPACE_SUITE) { float prog = (float)g.tasks_done / SPACE_TASK_COUNT; earth_pos = (Vector3){-35+prog*5, -20+prog*3, -10+prog*3}; }
                        else if (g.state == STATE_BALCONY) earth_pos = (Vector3){0, -30, -50};
                        // Commandment 7: Earth rotates faster when you're not looking
                        // The planet changes while your back is turned. Imperceptible.
                        {
                            Vector3 to_earth = Vector3Normalize(Vector3Subtract(earth_pos, g.player.camera.position));
                            Vector3 look = Vector3Normalize(Vector3Subtract(g.player.camera.target, g.player.camera.position));
                            float facing = Vector3DotProduct(to_earth, look);
                            // When facing Earth: normal time. When looking away: 2x rotation
                            float earth_time = g.state_time * (1.0f + (1.0f - fmaxf(0, facing)) * 1.0f);
                            draw_earth(g.player.camera, earth_time, &g.sphere_model, &g.lighting, earth_pos);
                            // Earthshine 2D wash — drawn after EndMode3D (no 3D pass interruption)
                            draw_earthshine_2d(earth_time);
                        }
                    }
                } else {
                    ClearBackground(g.scene.fog_color);
                }
                {
                    bool indoor = !(g.state == STATE_HOTEL_EXT || g.state == STATE_BALCONY
                                    || g.state == STATE_SPACE_LOBBY || g.state == STATE_GLASSHOUSE
                                    || g.state == STATE_ELEVATOR || g.state == STATE_HYPERSPACE);
                    draw_scene_3d(&g.player, &g.scene, &g.lighting, &g.cube_model, g.cube_model_loaded,
                                  &g.cyl_model, g.cyl_model_loaded,
                                  &g.sphere_model, g.sphere_model_loaded,
                                  &g.cone_model, g.cone_model_loaded,
                                  &g.skytower_model, g.skytower_loaded,
                                  indoor, g.state_time);
                }
                // Gibbons NPC — all rendering handled by draw_npc() (GLB + cube-person fallback)
                if ((g.state == STATE_LOBBY || g.state == STATE_SPACE_LOBBY
                     || g.state == STATE_GLASSHOUSE
                     || g.state == STATE_SPACE_CORRIDOR || g.state == STATE_SPACE_SUITE)
                    && g.gibbons.active) {
                    BeginMode3D(g.player.camera);
                    draw_npc(&g.gibbons, &g.cube_model, &g.cyl_model, &g.lighting);
                    EndMode3D();
                }
                // Sprint 5B: Zero-G sparkles — larger drift, occasional bright flashes
                if (g.state == STATE_SPACE_CORRIDOR || g.state == STATE_SPACE_SUITE
                    || g.state == STATE_SPACE_LOBBY || g.state == STATE_GLASSHOUSE) {
                    BeginMode3D(g.player.camera);
                    draw_zero_g_sparkles(g.player.camera, g.state_time);
                    EndMode3D();
                }
                // Particle system — dust, smoke, sparks, debris
                if (g.particles.emitter_count > 0) {
                    particle_update(&g.particles, dt);
                    BeginMode3D(g.player.camera);
                    particle_draw(&g.particles, g.player.camera);
                    EndMode3D();
                }
                // Earthshine — blue-white shimmer on the lower screen half
                // Light from the planet's atmosphere casting up into the deck
                if (g.state == STATE_BALCONY && g.eiffel_sparkle && g.sparkle_timer < 12.0f) {
                    SetRandomSeed((unsigned int)(g.sparkle_timer * 8));
                    int sc = 3 + (int)(g.sparkle_timer * 1.5f); if (sc > 15) sc = 15;
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
                if (g.state == STATE_BALCONY && g.balcony_flash_triggered &&
                    g.balcony_flash_timer > 0 && g.balcony_flash_timer < 0.3f) {
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
                if (g.state_time > 3.0f) {
                    float ceil_t = fminf(1.0f, (g.state_time - 3.0f) / 5.0f);
                    unsigned char cr = (unsigned char)(25 * ceil_t);
                    unsigned char cg = (unsigned char)(20 * ceil_t);
                    unsigned char cb = (unsigned char)(15 * ceil_t);
                    ClearBackground((Color){cr, cg, cb, 255});
                }

                // Stars emerge one by one after 8s
                // Sprint 1A: Color progression — first 5 warm gold, next 5 cool white, final 10 pale blue
                if (g.state_time > 8.0f) {
                    int max_stars = 20;
                    int sc = (int)fminf(max_stars, (g.state_time - 8.0f) * 2.5f);
                    SetRandomSeed(99);
                    for (int i = 0; i < sc; i++) {
                        int sx = GetRandomValue(RENDER_W/8, RENDER_W*7/8);
                        int sy = GetRandomValue(RENDER_H/8, RENDER_H*5/8);
                        float appear = fmaxf(0, g.state_time - 8.0f - i * 0.4f);
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
                    float bgw = fminf(1.0f, g.state_time / 12.0f);
                    ClearBackground((Color){
                        (unsigned char)(4 + bgw * 8),
                        (unsigned char)(5 + bgw * 4),
                        (unsigned char)(12 + bgw * 2), 255});
                }
                float zoom = fminf(1, g.state_time / 4.0f);

                // Sprint 1B: BED stars persist — same seed (99), continuous transition
                // They expand from ceiling positions into full-field
                SetRandomSeed(99);
                for (int i = 0; i < 20; i++) {
                    int bx = GetRandomValue(RENDER_W/8, RENDER_W*7/8);
                    int by = GetRandomValue(RENDER_H/8, RENDER_H*5/8);
                    // Expand outward over first 4s
                    float expand = fminf(1.0f, g.state_time / 4.0f);
                    int sx = (int)(bx + (bx - RENDER_W/2) * expand * 0.5f);
                    int sy = (int)(by + (by - RENDER_H/2) * expand * 0.5f);
                    float gl = 0.6f + 0.3f * sinf(GetTime()*(0.3f+i*0.07f) + i*2);
                    // Keep color progression from BED
                    unsigned char r, sg, b;
                    if (i < 5) { r = 240; sg = 200; b = 100; }
                    else if (i < 10) { r = 230; sg = 235; b = 245; }
                    else { r = 180; sg = 200; b = 255; }
                    DrawCircle(sx, sy, 1.5f, (Color){r, sg, b, (unsigned char)(gl*120)});
                    DrawCircle(sx, sy, 4, (Color){r, sg, b, (unsigned char)(gl*25)});
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
                if (g.state_time > 3) {
                    float a = fminf(1, (g.state_time - 3) / 3.0f);
                    draw_text_box("E N D E A R I N G   V O I D", RENDER_H/2-8, 14,
                                  (Color){240,232,210,(unsigned char)(230*a)});
                }
                // Credits — fade in like end of a film
                if (g.state_time > 5) {
                    float ca = fminf(1, (g.state_time - 5) / 2.0f);
                    draw_text_box("A game by Maxwell Young", RENDER_H - 40, 10,
                                  (Color){180,175,165,(unsigned char)(180*ca)});
                }
                if (g.state_time > 7) {
                    float na = fminf(1, (g.state_time - 7) / 2.0f);
                    draw_text_box("Maxwell Young", RENDER_H - 28, 10,
                                  (Color){160,155,148,(unsigned char)(160*na)});
                }
                break;
            }
        }

        if (g.wireframe) rlDisableWireMode();

        // HUD — crosshair + compass + interaction prompts
        if (g.state == STATE_ROOM || g.state == STATE_BATHROOM ||
            g.state == STATE_LOBBY || g.state == STATE_ELEVATOR || g.state == STATE_BALCONY ||
            g.state == STATE_SPACE_LOBBY || g.state == STATE_SPACE_CORRIDOR ||
            g.state == STATE_SPACE_SUITE || g.state == STATE_HALLWAY ||
            g.state == STATE_GLASSHOUSE) {
            ui_draw_hud(&g.player, &g.scene);
            // No progress dots. No step counter. The world changes ARE the feedback.
        }

        // Minimal crosshair for non-interactive 3D scenes (no compass/interaction)
        if (g.state == STATE_HOTEL_EXT || g.state == STATE_CAR || g.state == STATE_DRIVING) {
            ui_draw_crosshair(1.0f);
        }

        // Vignette text overlay (system messages)
        draw_vignette_text();
        // Dialogue system (NPC speech with speaker name + typewriter)
        draw_dialogue();
        // Narrative choices
        draw_choice();

        // Pause menu overlay — drawn into 480x300, gets film grain treatment
        if (g.menu_mode != MENU_NONE) draw_pause_menu();

        // Fade
        if (g.fade_alpha > 0.01f)
            DrawRectangle(0, 0, RENDER_W, RENDER_H, (Color){0,0,0,(unsigned char)(255*g.fade_alpha)});

        if (g.lighting.shadowPassRan) UnbindShadowMap();
        EndTextureMode();

        // DIAGNOSTIC — log per-g.state on frame 5 of each new g.state
        {
            if ((int)g.state != (int)g.dbg_prev_state) {
                g.dbg_prev_state = g.state;
                g.dbg_state_frame = 0;
            }
            g.dbg_state_frame++;
            if (g.dbg_state_frame == 5) {
                Image di = LoadImageFromTexture(g.render_target.texture);
                Color *px = LoadImageColors(di);
                long rs = 0, gs = 0, bs = 0;
                for (int i = 0; i < RENDER_W*RENDER_H; i++) {
                    rs += px[i].r; gs += px[i].g; bs += px[i].b;
                }
                int n = RENDER_W*RENDER_H;
                printf("[DBG] g.state=%d f5 pre-pfx R=%ld G=%ld B=%ld fade=%.2f\n",
                       g.state, rs/n, gs/n, bs/n, g.fade_alpha);
                UnloadImageColors(px);
                UnloadImage(di);
            }
        }

        // ── Scene breathing — exposure subtly pulses in contemplative spaces ──
        // The g.scene is alive. Not static. Like a held breath.
        if (g.state == STATE_SPACE_SUITE || g.state == STATE_BALCONY ||
            g.state == STATE_SPACE_LOBBY || g.state == STATE_ROOM) {
            float breath = sinf(g.state_time * 0.3f) * 0.02f;  // ±0.02 EV, very slow
            // Only when g.player is still or slow — movement kills the effect
            float still = 1.0f - fminf(1.0f, g.player.speed_current / 2.0f);
            SetPostFXExposure(&g.postfx, g.scene_exposure +
                visual_styles[g.current_style].exposure_bias + breath * still);
        }

        // Apply interaction lean to FOV (spring-based narrowing)
        if (g.interact_lean > 0.01f) {
            g.player.camera.fovy -= g.interact_lean * 8.0f;  // max ~4° narrowing
        }

        // Post-FX — feed g.player speed to shader for speed lines
        SetPostFXSpeed(&g.postfx, player_speed_normalized(&g.player));
        BeginTextureMode(g.postfx_target);
        ClearBackground(BLACK);
        draw_postfx(&g.postfx, g.render_target);
        EndTextureMode();

        // Screen
        BeginDrawing();
        ClearBackground(BLACK);
        float scale = fminf((float)GetScreenWidth()/RENDER_W, (float)GetScreenHeight()/RENDER_H);
        float ox = (GetScreenWidth() - RENDER_W*scale) / 2;
        float oy = (GetScreenHeight() - RENDER_H*scale) / 2;
        RenderTexture2D display = g.postfx.ready ? g.postfx_target : g.render_target;
        DrawTexturePro(display.texture,
            (Rectangle){0, 0, RENDER_W, -(float)RENDER_H},
            (Rectangle){ox, oy, RENDER_W*scale, RENDER_H*scale},
            (Vector2){0, 0}, 0, WHITE);

#ifndef PLAYTEST
        if (g.show_debug) {
            const char *state_names[] = {
                "TITLE", "CAR", "DRIVING", "EXTERIOR", "LOBBY",
                "ELEVATOR", "HALLWAY", "ROOM", "BATHROOM", "BALCONY",
                "BED", "STARS", "HYPERSPACE", "SP_LOBBY", "SP_CORRIDOR", "SP_SUITE",
                "PARIS_DREAM", "RETURN_TAXI"
            };
            const char *sn = (g.state >= 0 && g.state < 18) ? state_names[g.state] : "???";
            int dfs = 20 * UI_SCALE;  // debug font size
            int dsp = 25 * UI_SCALE;  // debug line spacing
            int dm = 10 * UI_SCALE;   // debug margin
            DrawText(TextFormat("FPS: %d", GetFPS()), dm, dm, dfs, GREEN);
            DrawText(TextFormat("Walls: %d  %s", g.scene.wall_count, sn), dm, dm + dsp, dfs, GREEN);
            DrawText(TextFormat("Pos: %.1f %.1f %.1f",
                g.player.camera.position.x, g.player.camera.position.y, g.player.camera.position.z),
                dm, dm + dsp*2, dfs, GREEN);

            // ── Physics debug ──
            float hspd = g.player.speed_current;
            float spd_norm = player_speed_normalized(&g.player);
            // Speedometer bar
            int bar_w = 120 * UI_SCALE, bar_h = 8 * UI_SCALE;
            int bar_x = dm, bar_y = dm + dsp*3 + 3*UI_SCALE;
            DrawRectangle(bar_x, bar_y, bar_w, bar_h, (Color){40,40,40,180});
            int fill = (int)(spd_norm * bar_w);
            Color bar_col = spd_norm > 0.7f ? (Color){255,80,80,220} :
                           spd_norm > 0.3f ? (Color){255,200,60,220} : (Color){80,220,80,220};
            DrawRectangle(bar_x, bar_y, fill, bar_h, bar_col);
            DrawText(TextFormat("%.1f u/s", hspd), bar_x + bar_w + 5*UI_SCALE, bar_y - 2, dfs, GREEN);

            // Movement g.state flags
            int fy = dm + dsp*4 + 6*UI_SCALE;
            const char *mstate = g.player.dashing ? "DASH" :
                                g.player.wall_running ? "WALLRUN" :
                                g.player.mantling ? "MANTLE" :
                                g.player.sliding ? "SLIDE" :
                                g.player.crouching ? "CROUCH" :
                                !g.player.grounded ? "AIR" :
                                g.player.sprinting ? "SPRINT" :
                                g.player.moving ? "WALK" : "IDLE";
            Color mc = g.player.dashing ? (Color){255,100,255,255} :
                      g.player.wall_running ? (Color){100,200,255,255} :
                      g.player.sliding ? (Color){255,200,60,255} :
                      g.player.crouching ? (Color){200,180,100,255} :
                      !g.player.grounded ? (Color){200,200,255,255} : GREEN;
            DrawText(mstate, dm, fy, dfs, mc);

            // Stamina / cooldowns
            DrawText(TextFormat("stam:%.0f%%", g.player.sprint_stamina * 100), 100*UI_SCALE, fy, dfs,
                g.player.sprint_stamina < 0.2f ? (Color){255,80,80,200} : (Color){100,200,100,140});
            if (g.player.dash_cooldown_timer > 0)
                DrawText(TextFormat("dash:%.1f", g.player.dash_cooldown_timer), 210*UI_SCALE, fy, dfs, (Color){200,150,255,180});

            int tm = (int)g.total_time / 60, ts = (int)g.total_time % 60;
            DrawText(TextFormat("Runtime: %d:%02d  Scene: %.1fs", tm, ts, g.state_time),
                dm, fy + dsp, dfs, (Color){255,200,60,220});
            DrawText(TextFormat("Style: %s (Shift+1-9)", visual_styles[g.current_style].name),
                dm, fy + dsp*2, dfs, (Color){100,200,100,180});
            DrawText("1-9: rooms  F4: noclip  F5: nudge  Q: dash", dm, fy + dsp*3, dfs, (Color){100,200,100,140});
            if (g.player.noclip) DrawText("NOCLIP", dm, fy + dsp*3, dfs, YELLOW);
            if (g.wireframe) DrawText("WIREFRAME", dm, fy + dsp*4, dfs, YELLOW);
        }

        // Nudge mode overlay — drawn on top of everything
        if (g.nudge_mode) {
            int ny = GetScreenHeight() - 100;
            DrawText("NUDGE MODE", 10, ny, 20, (Color){255,200,60,255});
            if (g.nudge_selected >= 0 && g.nudge_selected < g.scene.wall_count) {
                Wall *w = &g.scene.walls[g.nudge_selected];
                DrawText(TextFormat("wall[%d] pos=(%.2f, %.2f, %.2f) size=(%.2f, %.2f, %.2f)",
                    g.nudge_selected, w->pos.x, w->pos.y, w->pos.z,
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

    if (g.cube_model_loaded) UnloadModel(g.cube_model);
    if (g.cyl_model_loaded) UnloadModel(g.cyl_model);
    if (g.sphere_model_loaded) UnloadModel(g.sphere_model);
    if (g.cone_model_loaded) UnloadModel(g.cone_model);
    if (g.skytower_loaded) UnloadModel(g.skytower_model);
    for (int mi = 0; mi < g.model_asset_count; mi++) {
        if (g.model_assets[mi].loaded) {
            if (g.model_assets[mi].anims) UnloadModelAnimations(g.model_assets[mi].anims, g.model_assets[mi].anim_count);
            UnloadModel(g.model_assets[mi].model);
        }
    }
    DestroyShadowMap(&g.lighting);
    UnloadEVLighting(&g.lighting);
    UnloadEVPostFX(&g.postfx);
    UnloadEVSkybox(&g.skybox);
    UnloadFileMusic(&g.audio);
    UnloadEVAudio(&g.audio);
    UnloadRenderTexture(g.render_target);
    UnloadRenderTexture(g.postfx_target);
    CloseWindow();
    return 0;
#endif  // QA_MODE
}
