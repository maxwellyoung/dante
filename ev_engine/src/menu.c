// menu.c — Pause menu and settings overlay
#include "menu.h"
#include "render.h"
#include "ev_types.h"
#include "raylib.h"
#include <math.h>
#include <stdio.h>

static const char *pause_labels[]    = {"RESUME", "SETTINGS", "QUIT"};
static const char *settings_labels[] = {"SENSITIVITY", "VOLUME", "STYLE", "FULLSCREEN", "BACK"};

static int menu_item_count(GameCtx *g) { return (g->menu_mode == MENU_SETTINGS) ? SETTINGS_ITEM_COUNT : PAUSE_ITEM_COUNT; }
static void menu_init_springs(GameCtx *g) { for (int i = 0; i < MENU_MAX_ITEMS; i++) { g->menu_item_x[i] = (float)(RENDER_W + i * 24); g->menu_item_vx[i] = 0; } }

void menu_open(GameCtx *g, MenuMode mode) { g->menu_mode = mode; g->menu_cursor = 0; menu_init_springs(g); g->menu_overlay_a = 0; g->menu_overlay_va = 0; EnableCursor(); }
void menu_close(GameCtx *g) { g->menu_mode = MENU_NONE; g->menu_cursor = 0; g->menu_overlay_a = 0; g->menu_overlay_va = 0; if (g->state != STATE_TITLE) DisableCursor(); }

void update_menu_springs(GameCtx *g, float dt) {
    float sk = 280.0f, sd = 26.0f, sm = 0.9f;
    float ta = (g->menu_mode != MENU_NONE) ? 1.0f : 0.0f;
    float fa = -sk * (g->menu_overlay_a - ta) - sd * g->menu_overlay_va;
    g->menu_overlay_va += (fa / sm) * dt;
    g->menu_overlay_a += g->menu_overlay_va * dt;
    if (g->menu_overlay_a < 0) g->menu_overlay_a = 0;
    if (g->menu_overlay_a > 1) g->menu_overlay_a = 1;
    float tx = (g->menu_mode != MENU_NONE) ? 0.0f : (float)RENDER_W;
    for (int i = 0; i < MENU_MAX_ITEMS; i++) {
        float fx = -sk * (g->menu_item_x[i] - tx) - sd * g->menu_item_vx[i];
        g->menu_item_vx[i] += (fx / sm) * dt;
        g->menu_item_x[i] += g->menu_item_vx[i] * dt;
    }
}

bool update_pause_menu(GameCtx *g) {
    if (g->transitioning) return g->menu_mode != MENU_NONE;
    if (g->menu_mode == MENU_NONE) {
        if (IsKeyPressed(KEY_ESCAPE) && g->state != STATE_TITLE && g->state != STATE_STARS) {
            menu_open(g, MENU_PAUSE); return true;
        }
        return false;
    }
    int count = menu_item_count(g);
    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W))
        g->menu_cursor = (g->menu_cursor - 1 + count) % count;
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S))
        g->menu_cursor = (g->menu_cursor + 1) % count;
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
                { g->menu_cursor = i; break; }
        }
    }
    if (IsKeyPressed(KEY_ESCAPE)) {
        if (g->menu_mode == MENU_SETTINGS) menu_open(g, MENU_PAUSE);
        else menu_close(g);
        return true;
    }
    bool confirm  = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    bool go_left  = IsKeyPressed(KEY_LEFT)  || IsKeyPressed(KEY_A);
    bool go_right = IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D);
    if (g->menu_mode == MENU_PAUSE && confirm) {
        switch (g->menu_cursor) {
            case 0: menu_close(g); break;
            case 1: menu_open(g, MENU_SETTINGS); break;
            case 2: CloseWindow(); break;
        }
    } else if (g->menu_mode == MENU_SETTINGS) {
        switch (g->menu_cursor) {
            case 0:
                if (go_left)  ev_mouse_sens = fmaxf(0.001f, ev_mouse_sens - 0.0005f);
                if (go_right) ev_mouse_sens = fminf(0.008f, ev_mouse_sens + 0.0005f);
                break;
            case 1:
                if (go_left)  g->setting_master_vol = fmaxf(0.0f, g->setting_master_vol - 0.05f);
                if (go_right) g->setting_master_vol = fminf(1.0f, g->setting_master_vol + 0.05f);
                SetMasterVolume(g->setting_master_vol);
                break;
            case 2:
                if (go_left || go_right || confirm) {
                    if (go_left) g->current_style = (g->current_style - 1 + STYLE_COUNT) % STYLE_COUNT;
                    else         g->current_style = (g->current_style + 1) % STYLE_COUNT;
                    ApplyVisualStyle(&g->postfx, g->current_style);
                    SetPostFXExposure(&g->postfx, g->scene_exposure + visual_styles[g->current_style].exposure_bias);
                }
                break;
            case 3:
                if (confirm) { ToggleFullscreen(); g->setting_fullscreen = !g->setting_fullscreen; }
                break;
            case 4:
                if (confirm) menu_open(g, MENU_PAUSE);
                break;
        }
    }
    return true;
}

void draw_pause_menu(GameCtx *g) {
    if (g->menu_overlay_a < 0.01f) return;
    unsigned char oa = (unsigned char)(160.0f * g->menu_overlay_a);
    DrawRectangle(0, 0, RENDER_W, RENDER_H, (Color){8, 8, 10, oa});
    const char **labels = (g->menu_mode == MENU_SETTINGS) ? settings_labels : pause_labels;
    int count = (g->menu_mode == MENU_SETTINGS) ? SETTINGS_ITEM_COUNT : PAUSE_ITEM_COUNT;
    int fs = 10, lh = 18;
    int by = RENDER_H / 2 - (count * lh) / 2;
    for (int i = 0; i < count; i++) {
        int xo = (int)g->menu_item_x[i];
        int iy = by + i * lh;
        Color tc = (i == g->menu_cursor)
            ? (Color){245, 242, 238, 240} : (Color){165, 160, 152, 180};
        if (i == g->menu_cursor)
            DrawRectangle(RENDER_W / 2 - 80 + xo, iy + 3, 3, 7, (Color){200, 178, 130, 240});
        int lx = RENDER_W / 2 - 72 + xo;
        DrawText(labels[i], lx + 1, iy + 1, fs, (Color){0, 0, 0, 180});
        DrawText(labels[i], lx, iy, fs, tc);
        if (g->menu_mode == MENU_SETTINGS) {
            char val[48] = "";
            int vx = RENDER_W / 2 + 30 + xo;
            switch (i) {
                case 0: snprintf(val, sizeof(val), "< %.1f >", ev_mouse_sens * 1000.0f); break;
                case 1: snprintf(val, sizeof(val), "< %d%% >", (int)(g->setting_master_vol * 100.0f + 0.5f)); break;
                case 2: snprintf(val, sizeof(val), "< %s >", visual_styles[g->current_style].name); break;
                case 3: snprintf(val, sizeof(val), "[ %s ]", g->setting_fullscreen ? "ON" : "OFF"); break;
                default: break;
            }
            if (val[0]) {
                Color vc = (i == g->menu_cursor) ? (Color){200, 178, 130, 240} : (Color){165, 160, 152, 140};
                DrawText(val, vx + 1, iy + 1, fs, (Color){0, 0, 0, 180});
                DrawText(val, vx, iy, fs, vc);
            }
        }
    }
    const char *hint = (g->menu_mode == MENU_SETTINGS) ? "ARROWS TO ADJUST  /  ESC BACK" : "ESC TO RESUME";
    int hw = MeasureText(hint, 8);
    DrawText(hint, RENDER_W/2 - hw/2 + 1, RENDER_H - 23, 8, (Color){0,0,0,120});
    DrawText(hint, RENDER_W/2 - hw/2, RENDER_H - 24, 8, (Color){130,126,120,120});
}
