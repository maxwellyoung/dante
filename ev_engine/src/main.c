// ENDEARING VOID — Custom engine on Raylib
// Maxwell Young
// A story about arriving somewhere. Auckland. A tower. 2 AM.

#include "raylib.h"
#include "rlgl.h"
#include "raymath.h"
#include "game_ctx.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

float ev_mouse_sens = MOUSE_SENS_DEFAULT;

// ── Global game context — replaces ~80 file-scope statics ──
GameCtx g;

void show_text(const char *text) {
    g.vig_text = text;
    g.text_scale_target = 1.0f;
    g.text_y_offset = 20.0f;
    g.text_y_velocity = 0.0f;
}
void hide_text(void) {
    g.text_scale_target = 0.0f;
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
    int hw = MeasureText(hint, 8);
    DrawText(hint, RENDER_W/2 - hw/2 + 1, RENDER_H - 23, 8, (Color){0,0,0,120});
    DrawText(hint, RENDER_W/2 - hw/2, RENDER_H - 24, 8, (Color){130,126,120,120});
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
        "STATE_SPACE_SUITE", "STATE_PARIS_DREAM", "STATE_RETURN_TAXI"
    };
    FILE *f = fopen("/tmp/ev_state", "w");
    if (f) {
        int idx = (int)s;
        if (idx >= 0 && idx < 18) fprintf(f, "%s\n", names[idx]);
        else fprintf(f, "STATE_TITLE\n");
        fprintf(f, "%.2f %.2f %.2f\n",
                g.player.camera.position.x, g.player.camera.position.y,
                g.player.camera.position.z);
        fclose(f);
    }
}

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

    // Reset PostFX to current visual style baseline before per-scene overrides
    ApplyVisualStyle(&g.postfx, g.current_style);

    // Dispatch to scene-specific load
    if ((int)s < scene_desc_count && scene_descs[s].load) {
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
    InitEVAudio(&g.audio);
    LoadFileMusic(&g.audio);

    Mesh cube_mesh = GenMeshCube(1.0f, 1.0f, 1.0f);
    g.cube_model = LoadModelFromMesh(cube_mesh);
    if (g.lighting.ready) g.cube_model.materials[0].shader = g.lighting.shader;
    g.cube_model_loaded = true;

    Mesh cyl_mesh = GenMeshCylinder(0.5f, 1.0f, 16);
    g.cyl_model = LoadModelFromMesh(cyl_mesh);
    if (g.lighting.ready) g.cyl_model.materials[0].shader = g.lighting.shader;
    g.cyl_model_loaded = true;

    // Sphere model — for light fixtures, decorative elements
    Mesh sphere_mesh = GenMeshSphere(0.5f, 8, 8);
    g.sphere_model = LoadModelFromMesh(sphere_mesh);
    if (g.lighting.ready) g.sphere_model.materials[0].shader = g.lighting.shader;
    g.sphere_model_loaded = true;

    // Cone model — for lamp shades, decorative elements
    Mesh cone_mesh = GenMeshCone(0.5f, 1.0f, 12);
    g.cone_model = LoadModelFromMesh(cone_mesh);
    if (g.lighting.ready) g.cone_model.materials[0].shader = g.lighting.shader;
    g.cone_model_loaded = true;

    // Sky Tower — Auckland landmark, the recurring silhouette
    if (FileExists("assets/skytower.obj")) {
        g.skytower_model = LoadModel("assets/skytower.obj");
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

    // ── Scene registry with per-g.scene camera angles ──
    // Each g.scene gets a "hero" shot (best composition) and "spawn" shot (first impression)
    typedef struct {
        Vector3 pos;        // camera position (0,0,0 = use g.scene spawn)
        Vector3 target;     // look-at target
    } QACam;
    typedef struct {
        GameState gs;
        const char *name;
        QACam hero;         // the money shot — shows g.scene at its best
        QACam spawn;        // what the g.player actually sees first
        bool dark_by_design;
        bool outdoor;
    } QAEntry;

    // Camera angles tuned per-g.scene based on geometry and hero elements
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
            .hero  = {{0, 1.6f, 2}, {0, 1.4f, 14}},         // down corridor toward exit
            .spawn = {{0, 1.6f, 0}, {-2, 1.6f, 4}},         // entering, first door visible
            .dark_by_design = true,
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
        g.fade_alpha = 0.0f; g.fade_target = 0.0f;

        // ── Render helper: take a screenshot from a given angle ──
        #define QA_RENDER_SHOT(cam_setup, suffix) do { \
            if (cam_setup.pos.x != 0 || cam_setup.pos.y != 0 || cam_setup.pos.z != 0) \
                g.player.camera.position = cam_setup.pos; \
            g.player.camera.target = cam_setup.target; \
            for (int _f = 0; _f < 8; _f++) { \
                BeginTextureMode(g.render_target); \
                ClearBackground(g.scene.fog_color.a > 0 ? g.scene.fog_color : (Color){8,10,22,255}); \
                draw_scene_3d(&g.player, &g.scene, &g.lighting, \
                    &g.cube_model, g.cube_model_loaded, &g.cyl_model, g.cyl_model_loaded, \
                    &g.sphere_model, g.sphere_model_loaded, &g.cone_model, g.cone_model_loaded, \
                    &g.skytower_model, g.skytower_loaded, \
                    !qa_scenes[qi].outdoor, (float)_f * 0.016f); \
                EndTextureMode(); \
                BeginTextureMode(g.postfx_target); ClearBackground(BLACK); \
                draw_postfx(&g.postfx, g.render_target); EndTextureMode(); \
                BeginDrawing(); ClearBackground(BLACK); EndDrawing(); \
            } \
            RenderTexture2D _disp = g.postfx.ready ? g.postfx_target : g.render_target; \
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
        RenderTexture2D display = g.postfx.ready ? g.postfx_target : g.render_target;
        Image img = LoadImageFromTexture(display.texture);
        ImageFlipVertical(&img);
        char primary_path[256];
        snprintf(primary_path, sizeof(primary_path), "qa/screenshots/%s.png", qa_scenes[qi].name);
        ExportImage(img, primary_path);

        // Take spawn shot
        load_state(qa_scenes[qi].gs);
        g.fade_alpha = 0.0f; g.fade_target = 0.0f;
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
        for (int w = 0; w < g.scene.wall_count; w++)
            mat_counts[g.scene.walls[w].material]++;
        int mat_types_used = 0;
        for (int m = 0; m < 13; m++) if (mat_counts[m] > 0) mat_types_used++;
        int mat_tagged = g.scene.wall_count - mat_counts[0];  // non-concrete
        float mat_cov = g.scene.wall_count > 0 ? 100.0f*mat_tagged/g.scene.wall_count : 0;
        float wall_pct = 100.0f*g.scene.wall_count/MAX_WALLS;

        // ── Object reachability from spawn ──
        int interact_count = g.scene.object_count;
        float obj_min_dist = 999, obj_max_dist = 0;
        int obj_unreachable = 0;  // farther than 3.0 from spawn
        for (int oi = 0; oi < g.scene.object_count; oi++) {
            if (!g.scene.objects[oi].active) continue;
            float dx = g.scene.objects[oi].pos.x - g.scene.spawn.x;
            float dz = g.scene.objects[oi].pos.z - g.scene.spawn.z;
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
                "    LIGHTING: %.0f%% black — g.scene is unlit\n", black_pct); issues++;
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
        if (mat_cov < 25.0f && g.scene.wall_count > 20) {
            ib += snprintf(ibuf+ib, sizeof(ibuf)-(size_t)ib,
                "    MATERIAL: only %.0f%% non-concrete — surface monotony\n", mat_cov); issues++;
        }
        if (mat_types_used < 3 && g.scene.wall_count > 30) {
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
                "    BUDGET: wall budget %.0f%% (%d/%d) — near overflow\n", wall_pct, g.scene.wall_count, MAX_WALLS); issues++;
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
               qa_scenes[qi].name, qs, g.scene.wall_count,
               mat_types_used, mat_cov, luma, contrast_ratio, edge_density, hue_count, interact_count);
        if (issues > 0) printf("%s", ibuf);
        qa_total_issues += issues;

        // ── JSON entry ──
        if (jf) {
            if (qi > 0) fprintf(jf, ",\n");
            fprintf(jf, "    {\n");
            fprintf(jf, "      \"name\": \"%s\",\n", qa_scenes[qi].name);
            fprintf(jf, "      \"status\": \"%s\",\n", qs);
            fprintf(jf, "      \"walls\": %d,\n", g.scene.wall_count);
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
    printf("[QA] Screenshots: qa/screenshots/ (hero + spawn per g.scene)\n");
    printf("[QA] Report:      qa/report.json\n");
    printf("[QA] Analysis:    ./qa/run.sh   (full pipeline with grades)\n\n");

    if (g.cube_model_loaded) UnloadModel(g.cube_model);
    if (g.cyl_model_loaded) UnloadModel(g.cyl_model);
    if (g.sphere_model_loaded) UnloadModel(g.sphere_model);
    if (g.cone_model_loaded) UnloadModel(g.cone_model);
    if (g.skytower_loaded) UnloadModel(g.skytower_model);
    UnloadEVLighting(&g.lighting);
    UnloadEVPostFX(&g.postfx);
    UnloadFileMusic(&g.audio);
    UnloadEVAudio(&g.audio);
    UnloadRenderTexture(g.render_target);
    UnloadRenderTexture(g.postfx_target);
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

        update_menu_springs(dt);
        bool menu_active = update_pause_menu();
        if (!menu_active) {
            g.state_time += dt;
            g.total_time += dt;
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
                show_text("NUDGE MODE");
            } else {
                g.nudge_selected = -1;
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
        if ((int)g.state < scene_desc_count && scene_descs[g.state].update) {
            scene_descs[g.state].update(dt);
        }

        // Gibbons dialogue → vignette text (cross-scene concern)
        if (g.state == STATE_SPACE_LOBBY || g.state == STATE_SPACE_CORRIDOR
            || g.state == STATE_SPACE_SUITE) {
            const char *line = npc_current_dialogue(&g.gibbons);
            if (line && g.vig_text != line) {
                show_text(line);
            } else if (!line && g.vig_text && !g.gibbons.line_showing
                       && g.gibbons.current_line >= g.gibbons.line_count) {
                hide_text();
            }
        }

        }  // ── end game logic gate ──

        // ---- SHADOW PASS ----
        g.lighting.shadowPassRan = false;
        if (g.lighting.shadowReady && g.state != STATE_TITLE && g.state != STATE_BED && g.state != STATE_STARS) {
            draw_shadow_pass(&g.scene, &g.lighting,
                            &g.cube_model, &g.cyl_model, &g.sphere_model, &g.cone_model);
        }

        // ---- RENDER ----
        BeginTextureMode(g.render_target);
        ClearBackground(g.scene.fog_color.a > 0 ? g.scene.fog_color : (Color){8, 10, 22, 255});
        if (g.wireframe) rlEnableWireMode();

        // Bind shadow map ONLY if shadow pass actually wrote depth this frame
        if (g.lighting.shadowPassRan) BindShadowMap(&g.lighting);

        switch (g.state) {
            case STATE_TITLE:
                draw_title();
                break;

            case STATE_CAR:
            case STATE_DRIVING:
                // 3D taxi g.scene with night sky
                ClearBackground((Color){8, 12, 28, 255});
                draw_night_sky(g.state_time);
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
            case STATE_RETURN_TAXI:
                if (g.state == STATE_RETURN_TAXI) {
                    // Dawn sky behind the taxi
                    ClearBackground((Color){45, 38, 32, 255});
                    draw_dawn_sky(g.state_time);
                } else if (g.state == STATE_PARIS_DREAM) {
                    // Golden fog — no sky, just warm haze
                    ClearBackground((Color){35, 28, 15, 255});
                } else if (g.state == STATE_HOTEL_EXT) {
                    ClearBackground((Color){8, 12, 28, 255});
                    draw_night_sky(g.state_time);
                } else if (g.state == STATE_ELEVATOR) {
                    // Elevator transition: Auckland sky fades to void as you ascend
                    float ascent = fminf(1.0f, g.state_time / 4.0f);
                    unsigned char br = (unsigned char)(8 * (1.0f - ascent) + 4 * ascent);
                    unsigned char bg = (unsigned char)(12 * (1.0f - ascent) + 5 * ascent);
                    unsigned char bb = (unsigned char)(28 * (1.0f - ascent) + 12 * ascent);
                    ClearBackground((Color){br, bg, bb, 255});
                    if (ascent < 0.8f) draw_night_sky(g.state_time);
                } else if (g.state == STATE_HYPERSPACE) {
                    ClearBackground((Color){2, 2, 6, 255});  // deep void tunnel
                } else if (g.state == STATE_BALCONY || g.state == STATE_SPACE_LOBBY
                           || g.state == STATE_SPACE_CORRIDOR || g.state == STATE_SPACE_SUITE) {
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
                        int star_count = (g.state == STATE_BALCONY) ? 140 : 70;
                        for (int si = 0; si < star_count; si++) {
                            int sx = GetRandomValue(0, RENDER_W);
                            int sy = GetRandomValue(0, RENDER_H);
                            float bri = 0.12f + (GetRandomValue(0, 88) / 100.0f);
                            float twk = 0.6f + 0.4f * sinf(g.state_time * (0.3f + GetRandomValue(0, 10) / 10.0f) + si * 2.7f);
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
                    // Procedural Earth — positioned per g.scene so it's visible through windows
                    if (g.sphere_model_loaded) {
                        Vector3 earth_pos = {0, -40, -60};  // default
                        if (g.state == STATE_SPACE_LOBBY) earth_pos = (Vector3){3, -20, -40};
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
                        }
                    }
                } else {
                    ClearBackground(g.scene.fog_color);
                }
                {
                    bool indoor = !(g.state == STATE_HOTEL_EXT || g.state == STATE_BALCONY
                                    || g.state == STATE_SPACE_LOBBY || g.state == STATE_ELEVATOR
                                    || g.state == STATE_HYPERSPACE);
                    draw_scene_3d(&g.player, &g.scene, &g.lighting, &g.cube_model, g.cube_model_loaded,
                                  &g.cyl_model, g.cyl_model_loaded,
                                  &g.sphere_model, g.sphere_model_loaded,
                                  &g.cone_model, g.cone_model_loaded,
                                  &g.skytower_model, g.skytower_loaded,
                                  indoor, g.state_time);
                }
                // Gibbons NPC — draw in space scenes
                if ((g.state == STATE_LOBBY || g.state == STATE_SPACE_LOBBY
                     || g.state == STATE_SPACE_CORRIDOR || g.state == STATE_SPACE_SUITE)
                    && g.gibbons.active) {
                    BeginMode3D(g.player.camera);
                    draw_npc(&g.gibbons, &g.cube_model, &g.cyl_model, &g.lighting);
                    EndMode3D();
                }
                // Sprint 5B: Zero-G sparkles — larger drift, occasional bright flashes
                if (g.state == STATE_SPACE_CORRIDOR || g.state == STATE_SPACE_SUITE
                    || g.state == STATE_SPACE_LOBBY) {
                    BeginMode3D(g.player.camera);
                    draw_zero_g_sparkles(g.player.camera, g.state_time);
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

        // HUD
        if (g.state == STATE_ROOM || g.state == STATE_BATHROOM ||
            g.state == STATE_LOBBY || g.state == STATE_ELEVATOR || g.state == STATE_BALCONY ||
            g.state == STATE_SPACE_LOBBY || g.state == STATE_SPACE_CORRIDOR ||
            g.state == STATE_SPACE_SUITE || g.state == STATE_HALLWAY) {
            draw_hud(&g.player, &g.scene);
            // No progress dots. No step counter. The world changes ARE the feedback.
            // "Removing the map marker was the best decision I ever made." — Sam C
        }

        // Crosshair — drawn by draw_hud for interactive scenes;
        // simple dot for non-interactive 3D scenes
        if (g.state == STATE_HOTEL_EXT || g.state == STATE_CAR || g.state == STATE_DRIVING) {
            DrawPixel(RENDER_W/2, RENDER_H/2, (Color){220, 215, 205, 60});
        }

        // Vignette text overlay
        draw_vignette_text();

        // Pause menu overlay — drawn into 480x300, gets film grain treatment
        if (g.menu_mode != MENU_NONE) draw_pause_menu();

        // Fade
        if (g.fade_alpha > 0.01f)
            DrawRectangle(0, 0, RENDER_W, RENDER_H, (Color){0,0,0,(unsigned char)(255*g.fade_alpha)});

        if (g.lighting.shadowPassRan) UnbindShadowMap();
        EndTextureMode();

        // DIAGNOSTIC — log per-g.state on frame 5 of each new g.state
        {
            static GameState dbg_prev = -1;
            static int dbg_sf = 0;
            if ((int)g.state != (int)dbg_prev) { dbg_prev = g.state; dbg_sf = 0; }
            dbg_sf++;
            if (dbg_sf == 5) {
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
            DrawText(TextFormat("FPS: %d", GetFPS()), 10, 10, 20, GREEN);
            DrawText(TextFormat("Walls: %d  %s", g.scene.wall_count, sn), 10, 35, 20, GREEN);
            DrawText(TextFormat("Pos: %.1f %.1f %.1f",
                g.player.camera.position.x, g.player.camera.position.y, g.player.camera.position.z),
                10, 60, 20, GREEN);

            // ── Physics debug ──
            float hspd = g.player.speed_current;
            float spd_norm = player_speed_normalized(&g.player);
            // Speedometer bar
            int bar_w = 120, bar_h = 8;
            int bar_x = 10, bar_y = 88;
            DrawRectangle(bar_x, bar_y, bar_w, bar_h, (Color){40,40,40,180});
            int fill = (int)(spd_norm * bar_w);
            Color bar_col = spd_norm > 0.7f ? (Color){255,80,80,220} :
                           spd_norm > 0.3f ? (Color){255,200,60,220} : (Color){80,220,80,220};
            DrawRectangle(bar_x, bar_y, fill, bar_h, bar_col);
            DrawText(TextFormat("%.1f u/s", hspd), bar_x + bar_w + 5, bar_y - 2, 20, GREEN);

            // Movement g.state flags
            int fy = 102;
            const char *mstate = g.player.dashing ? "DASH" :
                                g.player.wall_running ? "WALLRUN" :
                                g.player.mantling ? "MANTLE" :
                                g.player.sliding ? "SLIDE" :
                                !g.player.grounded ? "AIR" :
                                g.player.sprinting ? "SPRINT" :
                                g.player.moving ? "WALK" : "IDLE";
            Color mc = g.player.dashing ? (Color){255,100,255,255} :
                      g.player.wall_running ? (Color){100,200,255,255} :
                      g.player.sliding ? (Color){255,200,60,255} :
                      !g.player.grounded ? (Color){200,200,255,255} : GREEN;
            DrawText(mstate, 10, fy, 20, mc);

            // Stamina / cooldowns
            DrawText(TextFormat("stam:%.0f%%", g.player.sprint_stamina * 100), 100, fy, 20,
                g.player.sprint_stamina < 0.2f ? (Color){255,80,80,200} : (Color){100,200,100,140});
            if (g.player.dash_cooldown_timer > 0)
                DrawText(TextFormat("dash:%.1f", g.player.dash_cooldown_timer), 210, fy, 20, (Color){200,150,255,180});

            int tm = (int)g.total_time / 60, ts = (int)g.total_time % 60;
            DrawText(TextFormat("Runtime: %d:%02d  Scene: %.1fs", tm, ts, g.state_time),
                10, fy + 20, 20, (Color){255,200,60,220});
            DrawText(TextFormat("Style: %s (Shift+1-9)", visual_styles[g.current_style].name),
                10, fy + 40, 20, (Color){100,200,100,180});
            DrawText("1-9: rooms  F4: noclip  F5: nudge  Q: dash", 10, fy + 60, 20, (Color){100,200,100,140});
            if (g.player.noclip) DrawText("NOCLIP", 10, fy + 60, 20, YELLOW);
            if (g.wireframe) DrawText("WIREFRAME", 10, fy + 80, 20, YELLOW);
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
    DestroyShadowMap(&g.lighting);
    UnloadEVLighting(&g.lighting);
    UnloadEVPostFX(&g.postfx);
    UnloadFileMusic(&g.audio);
    UnloadEVAudio(&g.audio);
    UnloadRenderTexture(g.render_target);
    UnloadRenderTexture(g.postfx_target);
    CloseWindow();
    return 0;
#endif  // QA_MODE
}
