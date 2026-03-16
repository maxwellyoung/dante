// ENDEARING VOID — Custom engine on Raylib
// Glitched Games — Maxwell Young & Adam Van der Voorn
// A story about arriving somewhere. Paris. A hotel. 2 AM.

#include "raylib.h"
#include "rlgl.h"
#include "raymath.h"
#include "ev_types.h"
#include "lighting.h"
#include "scene.h"
#include "player.h"
#include "audio.h"
#include "render.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

static GameState state = STATE_TITLE;
static float state_time = 0;
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
static RenderTexture2D render_target;
static RenderTexture2D postfx_target;
static float planet_angle = 0;

static int tasks_done = 0;
static const int total_tasks = 5;
static float done_pause = 0;

static bool eiffel_sparkle = false;
static float sparkle_timer = 0;

static bool show_debug = false;
static bool wireframe = false;

// Vignette text system
static const char *vig_text = NULL;
static float vig_text_alpha = 0;
static float vig_text_target = 0;

static void show_text(const char *text) {
    vig_text = text;
    vig_text_target = 1.0f;
}
static void hide_text(void) {
    vig_text_target = 0.0f;
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
    return INTERACT_CLICK;
}

static void transition_to(GameState s) {
    transitioning = true;
    next_state = s;
    fade_target = 1.0f;
    fade_speed = 2.0f;
    PlayDoorSound(&audio);
}

static void transition_to_slow(GameState s, float spd) {
    transitioning = true;
    next_state = s;
    fade_target = 1.0f;
    fade_speed = spd;
    PlayDoorSound(&audio);
}

static void load_state(GameState s) {
    state = s;
    state_time = 0;
    done_pause = 0;
    vig_text = NULL;
    vig_text_alpha = 0;
    vig_text_target = 0;

    switch (s) {
        case STATE_TITLE: DisableCursor(); StopAmbient(&audio); break;

        case STATE_CAR:
            // Init rain
            for (int i = 0; i < MAX_RAIN; i++) {
                rain[i].x = GetRandomValue(0, RENDER_W);
                rain[i].y = GetRandomValue(-RENDER_H, 0);
                rain[i].speed = 80 + GetRandomValue(0, 120);
                rain[i].len = 3 + GetRandomValue(0, 8);
            }
            break;

        case STATE_DRIVING:
            // Taxi arriving — auto scene
            break;

        case STATE_HOTEL_EXT:
            build_hotel_exterior(&scene);
            init_player(&player, scene.spawn);
            StopAmbient(&audio);
            break;

        case STATE_LOBBY:
            EnableCursor(); DisableCursor();
            build_lobby(&scene);
            init_player(&player, scene.spawn);
            StartAmbient(&audio, DRONE_LOBBY);
            break;

        case STATE_HALLWAY:
            build_hallway(&scene);
            init_player(&player, scene.spawn);
            StartAmbient(&audio, DRONE_HALLWAY);
            break;

        case STATE_ROOM:
            build_hotel_room(&scene);
            init_player(&player, scene.spawn);
            tasks_done = 0;
            StartAmbient(&audio, DRONE_ROOM);
            SetPostFXWarmth(&postfx, 0.0f);
            break;

        case STATE_BALCONY:
            build_balcony(&scene);
            init_player(&player, scene.spawn);
            eiffel_sparkle = false; sparkle_timer = 0;
            StopAmbient(&audio);
            SetPostFXWarmth(&postfx, 1.0f);
            break;

        case STATE_BED: StopAmbient(&audio); break;
        case STATE_STARS: StopAmbient(&audio); break;
    }
    fade_alpha = 1.0f;
    fade_target = 0.0f;
}

// ============================================================
// VIGNETTE DRAWING
// ============================================================

static void draw_taxi_ride(float t) {
    // Backseat of a taxi. Paris at 2 AM through a rain-streaked window.
    ClearBackground((Color){8, 10, 18, 255});

    // Window frame — you're looking out the right-side window
    // Dark interior surrounds a bright window
    Color interior = {15, 12, 10, 255};
    DrawRectangle(0, 0, 30, RENDER_H, interior);                    // left pillar
    DrawRectangle(RENDER_W - 25, 0, 25, RENDER_H, interior);       // right pillar
    DrawRectangle(0, 0, RENDER_W, 20, interior);                    // roof
    DrawRectangle(0, RENDER_H - 55, RENDER_W, 55, interior);       // door panel

    // Window area — deep blue Paris night
    int wx = 30, wy = 20, ww = RENDER_W - 55, wh = RENDER_H - 75;
    for (int y = 0; y < wh; y++) {
        float gt = (float)y / wh;
        unsigned char r = (unsigned char)(12 + gt * 15);
        unsigned char g = (unsigned char)(15 + gt * 12);
        unsigned char b = (unsigned char)(30 + gt * 15);
        DrawRectangle(wx, wy + y, ww, 1, (Color){r, g, b, 255});
    }

    // City lights streaking past — faster = longer streaks
    float speed = 1.0f;
    if (t > 10) speed = fmaxf(0, 1.0f - (t - 10) / 4.0f);  // slow down at end

    SetRandomSeed(42);
    for (int i = 0; i < 12; i++) {
        float bx_base = GetRandomValue(wx, wx + ww);
        float by = GetRandomValue(wy + 5, wy + wh - 20);
        float bspeed = (0.3f + GetRandomValue(0, 70) / 100.0f) * speed;

        // Streak position — scrolling
        float bx = fmodf(bx_base + t * bspeed * 200, ww) + wx;

        // Light streak
        int streak_len = (int)(3 + bspeed * 25 * speed);
        Color lc;
        int type = GetRandomValue(0, 3);
        if (type == 0) lc = (Color){240, 200, 100, 120};       // warm streetlight
        else if (type == 1) lc = (Color){255, 100, 80, 100};   // tail light red
        else if (type == 2) lc = (Color){100, 180, 255, 80};   // neon blue
        else lc = (Color){255, 255, 240, 60};                  // white

        DrawRectangle((int)bx, (int)by, streak_len, 2, lc);

        // Occasional window glow (building)
        if (i < 5) {
            float building_x = fmodf(i * 70 + t * bspeed * 100, ww) + wx;
            int bh = 20 + GetRandomValue(0, 40);
            DrawRectangle((int)building_x, wy + wh - bh, 12, bh,
                         (Color){18, 20, 30, 255});
            // Windows
            for (int wy2 = 0; wy2 < bh - 5; wy2 += 7) {
                if (GetRandomValue(0, 2) > 0)
                    DrawRectangle((int)building_x + 3, wy + wh - bh + wy2 + 2, 3, 3,
                                 (Color){220, 185, 100, (unsigned char)(40 + GetRandomValue(0, 60))});
            }
        }
    }

    // Rain on window
    for (int i = 0; i < MAX_RAIN; i++) {
        rain[i].y += rain[i].speed * GetFrameTime();
        if (rain[i].y > RENDER_H) {
            rain[i].y = -rain[i].len;
            rain[i].x = GetRandomValue(wx, wx + ww);
        }
        if (rain[i].x >= wx && rain[i].x < wx + ww &&
            rain[i].y >= wy && rain[i].y < wy + wh) {
            DrawLine((int)rain[i].x, (int)rain[i].y,
                    (int)rain[i].x, (int)(rain[i].y + rain[i].len),
                    (Color){180, 190, 210, 60});
        }
    }

    // Reflection on window — subtle
    DrawRectangle(wx, wy, ww, wh, (Color){100, 120, 150, 8});

    // Dashboard / taxi interior details
    // Taxi meter
    DrawRectangle(RENDER_W - 80, RENDER_H - 50, 55, 25, (Color){20, 18, 15, 255});
    DrawRectangle(RENDER_W - 78, RENDER_H - 48, 51, 21, (Color){40, 50, 35, 255});
    float fare = 12.40f + t * 0.15f;
    DrawText(TextFormat("%.2f", fare), RENDER_W - 74, RENDER_H - 45, 12,
             (Color){120, 230, 120, 200});

    // Headrest silhouette of driver
    DrawRectangle(55, RENDER_H - 80, 35, 30, (Color){20, 18, 15, 255});
    DrawCircle(72, RENDER_H - 82, 12, (Color){20, 18, 15, 255});
}

static void draw_arrival(float t) {
    // Taxi has stopped. Through the window: the hotel facade, warm and golden.
    ClearBackground((Color){8, 10, 18, 255});

    Color interior = {15, 12, 10, 255};
    DrawRectangle(0, 0, 20, RENDER_H, interior);
    DrawRectangle(RENDER_W - 20, 0, 20, RENDER_H, interior);
    DrawRectangle(0, 0, RENDER_W, 15, interior);
    DrawRectangle(0, RENDER_H - 45, RENDER_W, 45, interior);

    int wx = 20, wy = 15, ww = RENDER_W - 40, wh = RENDER_H - 60;

    // Night sky through window
    for (int y = 0; y < wh; y++) {
        float gt = (float)y / wh;
        DrawRectangle(wx, wy + y, ww, 1,
            (Color){(unsigned char)(10+gt*12), (unsigned char)(12+gt*10), (unsigned char)(25+gt*12), 255});
    }

    // Hotel facade — centered, golden, warm
    int hx = wx + ww/2 - 80, hw = 160, hh_start = wy + 15;
    DrawRectangle(hx, hh_start, hw, wh - 20, (Color){190, 160, 95, 255});

    // Windows — warm golden glow
    for (int floor = 0; floor < 4; floor++) {
        for (int col = 0; col < 5; col++) {
            int fwx = hx + 10 + col * 30;
            int fwy = hh_start + 8 + floor * (wh - 30) / 4;
            DrawRectangle(fwx, fwy, 18, 22, (Color){240, 210, 130, 180});
            // Warm glow spill
            DrawRectangle(fwx - 2, fwy + 22, 22, 3, (Color){240, 210, 130, 40});
        }
    }

    // Entrance — bright warm light pouring out
    DrawRectangle(hx + hw/2 - 20, hh_start + wh - 45, 40, 35,
                 (Color){250, 235, 180, 220});

    // Red awning
    DrawRectangle(hx + hw/2 - 30, hh_start + wh - 50, 60, 6,
                 (Color){210, 55, 50, 255});

    // "HOTEL CHEVALIER" text on facade
    float text_alpha = fminf(1, t / 2.0f);
    const char *name = "HOTEL CHEVALIER";
    int tw = MeasureText(name, 8);
    DrawText(name, wx + ww/2 - tw/2, hh_start + 2, 8,
             (Color){240, 210, 120, (unsigned char)(230 * text_alpha)});

    // Lamppost glow outside
    DrawCircle(hx - 15, hh_start + wh - 30, 8, (Color){240, 210, 120, 50});
    DrawCircle(hx + hw + 15, hh_start + wh - 30, 8, (Color){240, 210, 120, 50});

    // Rain still visible but lighter
    for (int i = 0; i < MAX_RAIN / 3; i++) {
        rain[i].y += rain[i].speed * GetFrameTime() * 0.3f;
        if (rain[i].y > RENDER_H) rain[i].y = -rain[i].len;
        if (rain[i].x >= wx && rain[i].x < wx + ww)
            DrawLine((int)rain[i].x, (int)rain[i].y,
                    (int)rain[i].x, (int)(rain[i].y + rain[i].len * 0.5f),
                    (Color){180, 190, 210, 35});
    }
}

static void draw_vignette_text(void) {
    if (!vig_text || vig_text_alpha < 0.01f) return;
    int tw = MeasureText(vig_text, 14);
    unsigned char a = (unsigned char)(220 * vig_text_alpha);
    // Bottom of screen, clean
    DrawRectangle(0, RENDER_H - 36, RENDER_W, 32, (Color){0, 0, 0, (unsigned char)(120 * vig_text_alpha)});
    DrawText(vig_text, RENDER_W/2 - tw/2, RENDER_H - 29, 14, (Color){235, 228, 210, a});
}

// ============================================================
// MAIN
// ============================================================

int main(void) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(960, 600, "Endearing Void");
    SetTargetFPS(60);

    render_target = LoadRenderTexture(RENDER_W, RENDER_H);
    SetTextureFilter(render_target.texture, TEXTURE_FILTER_POINT);
    postfx_target = LoadRenderTexture(RENDER_W, RENDER_H);
    SetTextureFilter(postfx_target.texture, TEXTURE_FILTER_POINT);

    lighting = LoadEVLighting();
    postfx = LoadEVPostFX();
    InitEVAudio(&audio);

    Mesh cube_mesh = GenMeshCube(1.0f, 1.0f, 1.0f);
    cube_model = LoadModelFromMesh(cube_mesh);
    if (lighting.ready) cube_model.materials[0].shader = lighting.shader;
    cube_model_loaded = true;

    DisableCursor();
    load_state(STATE_TITLE);

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        state_time += dt;

        if (IsKeyPressed(KEY_F1)) wireframe = !wireframe;
        if (IsKeyPressed(KEY_F3)) show_debug = !show_debug;
        if (IsKeyPressed(KEY_F4)) player.noclip = !player.noclip;

        // Fade
        if (fade_alpha != fade_target) {
            float dir = (fade_target > fade_alpha) ? 1 : -1;
            fade_alpha += dir * fade_speed * dt;
            if ((dir > 0 && fade_alpha >= fade_target) || (dir < 0 && fade_alpha <= fade_target)) {
                fade_alpha = fade_target;
                if (transitioning) { transitioning = false; load_state(next_state); }
            }
        }

        // Vignette text fade
        float text_speed = 3.0f;
        if (vig_text_alpha < vig_text_target) {
            vig_text_alpha += text_speed * dt;
            if (vig_text_alpha > vig_text_target) vig_text_alpha = vig_text_target;
        } else if (vig_text_alpha > vig_text_target) {
            vig_text_alpha -= text_speed * dt;
            if (vig_text_alpha < 0) { vig_text_alpha = 0; vig_text = NULL; }
        }

        // Reward timers
        for (int i = 0; i < scene.object_count; i++)
            if (scene.objects[i].reward_timer > 0) {
                scene.objects[i].reward_timer -= dt * 0.5f;
                if (scene.objects[i].reward_timer < 0) scene.objects[i].reward_timer = 0;
            }

        // ---- UPDATE ----
        switch (state) {
            case STATE_TITLE:
                planet_angle += dt * 0.2f;
                if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) transition_to(STATE_CAR);
                break;

            case STATE_CAR:
                // Taxi ride — auto-playing cinematic
                if (state_time > 1.5f && state_time < 2.0f) show_text("Paris. 2:47 AM.");
                if (state_time > 5.0f) hide_text();
                if (state_time > 7.0f && state_time < 7.5f) show_text("You arrived three hours early.");
                if (state_time > 10.0f) hide_text();
                if (state_time > 13.5f) transition_to(STATE_DRIVING);
                if (IsKeyPressed(KEY_ENTER) && state_time > 3) transition_to(STATE_DRIVING);
                break;

            case STATE_DRIVING:
                // Arrival — taxi stopped, looking at hotel through window
                if (state_time > 1.5f && state_time < 2.0f) show_text("Nous sommes arrives, monsieur.");
                if (state_time > 5.5f) hide_text();
                if (state_time > 7.0f) transition_to(STATE_HOTEL_EXT);
                if (IsKeyPressed(KEY_ENTER) && state_time > 2) transition_to(STATE_HOTEL_EXT);
                break;

            case STATE_HOTEL_EXT:
                update_player(&player, &scene, dt);
                if (scene.has_exit) {
                    float dist = Vector3Distance(player.camera.position, scene.exit_pos);
                    if (dist < 1.5f) transition_to(STATE_LOBBY);
                }
                break;

            case STATE_LOBBY:
                update_player(&player, &scene, dt);
                UpdateEVAudio(&audio, player.moving, player.sprinting, scene.surface, dt);
                if (scene.has_exit) {
                    float dist = Vector3Distance(player.camera.position, scene.exit_pos);
                    if (dist < 1.5f) transition_to(STATE_HALLWAY);
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
                // "Three hours to kill." — connects taxi to room (Vanaman, Wreden)
                if (state_time > 3.0f && state_time < 3.5f) show_text("Three hours to kill.");
                if (state_time > 7.0f && state_time < 7.5f) hide_text();
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
                                if (obj->step >= obj->max_steps) {
                                    obj->done = true;
                                    obj->reward_timer = 1.5f;
                                    tasks_done++;
                                    PlayRewardSound(&audio);
                                    SetPostFXWarmth(&postfx, (float)tasks_done / total_tasks);
                                    scene.fog_density = 0.008f - ((float)tasks_done / total_tasks * 0.005f);

                                    // Visible consequences per task (Chung, Vlambeer)
                                    if (strcmp(obj->name, "lamp") == 0) {
                                        // Lamp is now on — warm golden glow
                                        add_light_panel(&scene, -2.5f, 1.2f, -3.8f, 0.5f, 0.6f, 0.5f, (Color){255,220,120,200});
                                    } else if (strcmp(obj->name, "candles") == 0) {
                                        // 3 small flames — warm orange rectangles
                                        add_wall(&scene, -4.0f, 0.55f, 3.0f, 0.06f, 0.12f, 0.06f, (Color){255,180,60,220});
                                        add_wall(&scene, -4.2f, 0.55f, 3.0f, 0.06f, 0.12f, 0.06f, (Color){255,190,70,200});
                                        add_wall(&scene, -4.4f, 0.55f, 3.0f, 0.06f, 0.12f, 0.06f, (Color){255,170,50,210});
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

                                    // Camera kick — bigger for final task (Change 6)
                                    if (tasks_done >= total_tasks) {
                                        kick_camera(&player, -0.05f, 0.02f);
                                        scene.fog_density = 0.001f;
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
                if (tasks_done >= total_tasks) {
                    done_pause += dt;
                    if (done_pause > 3.0f) transition_to(STATE_BALCONY);
                }
                break;

            case STATE_BALCONY:
                update_player(&player, &scene, dt);
                if (!eiffel_sparkle && state_time > 6) {
                    eiffel_sparkle = true; sparkle_timer = 0;
                    PlaySparkleSound(&audio);
                }
                if (eiffel_sparkle) sparkle_timer += dt;
                // Resignation text — "..." (Wreden)
                if (state_time > 25.0f && state_time < 25.5f) show_text("...");
                if (state_time > 30.0f && state_time < 30.5f) {
                    hide_text();
                    transition_to_slow(STATE_BED, 0.7f);
                }
                if (state_time > 12 && IsKeyPressed(KEY_ENTER))
                    transition_to_slow(STATE_BED, 0.7f);
                break;

            case STATE_BED:
                // "..." fades in during black, then ceiling, then stars
                if (state_time > 0.5f && state_time < 1.0f) show_text("...");
                if (state_time > 3.0f) hide_text();
                if (state_time > 18 || (state_time > 8 && IsKeyPressed(KEY_ENTER)))
                    transition_to_slow(STATE_STARS, 0.7f);
                break;

            case STATE_STARS:
                if (IsKeyPressed(KEY_ESCAPE) || (state_time > 10 && IsKeyPressed(KEY_ENTER))) {
                    CloseWindow(); return 0;
                }
                break;
        }

        // ---- RENDER ----
        BeginTextureMode(render_target);
        ClearBackground(scene.fog_color.a > 0 ? scene.fog_color : (Color){8, 10, 22, 255});
        if (wireframe) rlEnableWireMode();

        switch (state) {
            case STATE_TITLE:
                draw_title(planet_angle);
                break;

            case STATE_CAR:
                draw_taxi_ride(state_time);
                break;

            case STATE_DRIVING:
                draw_arrival(state_time);
                break;

            case STATE_HOTEL_EXT:
            case STATE_LOBBY:
            case STATE_HALLWAY:
            case STATE_ROOM:
            case STATE_BALCONY:
                draw_scene_3d(&player, &scene, &lighting, &cube_model, cube_model_loaded);
                if (state == STATE_BALCONY && eiffel_sparkle && sparkle_timer < 8.0f) {
                    SetRandomSeed((unsigned int)(sparkle_timer * 10));
                    int sc = 6 + (int)(sparkle_timer * 3); if (sc > 30) sc = 30;
                    for (int i = 0; i < sc; i++) {
                        int sx = GetRandomValue(RENDER_W/4, RENDER_W*3/4);
                        int sy = GetRandomValue(10, RENDER_H/2);
                        float fl = 0.5f + 0.5f * sinf(GetTime()*20 + i*7.3f);
                        if (fl > 0.7f)
                            DrawPixel(sx, sy, (Color){255,240,180,(unsigned char)(200*(fl-0.7f)/0.3f)});
                    }
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

                // Stars emerge one by one after 8s — warm amber/gold
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
                        // Warm amber/gold — not green
                        DrawCircle(sx, sy, 1.5f,
                            (Color){240, 200, 100, (unsigned char)(bri*gl*160)});
                        DrawCircle(sx, sy, 5,
                            (Color){220, 180, 80, (unsigned char)(bri*gl*35)});
                    }
                }
                break;
            }

            case STATE_STARS: {
                ClearBackground((Color){4, 5, 12, 255});
                float zoom = fminf(1, state_time / 4.0f);
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
                    const char *t = "E N D E A R I N G   V O I D";
                    int tw = MeasureText(t, 14);
                    DrawText(t, RENDER_W/2-tw/2, RENDER_H/2-8, 14,
                             (Color){240,232,210,(unsigned char)(230*a)});
                }
                // Credits — fade in like end of a film
                if (state_time > 5) {
                    float ca = fminf(1, (state_time - 5) / 2.0f);
                    const char *by = "A game by Glitched Games";
                    int bw = MeasureText(by, 8);
                    DrawText(by, RENDER_W/2-bw/2, RENDER_H - 40, 8,
                             (Color){180,175,165,(unsigned char)(180*ca)});
                }
                if (state_time > 7) {
                    float na = fminf(1, (state_time - 7) / 2.0f);
                    const char *names = "Maxwell Young & Adam Van der Voorn";
                    int nw = MeasureText(names, 8);
                    DrawText(names, RENDER_W/2-nw/2, RENDER_H - 28, 8,
                             (Color){160,155,148,(unsigned char)(160*na)});
                }
                break;
            }
        }

        if (wireframe) rlDisableWireMode();

        // HUD
        if (state == STATE_ROOM) {
            draw_hud(&player, &scene);
            for (int i = 0; i < scene.object_count; i++) {
                InteractObject *obj = &scene.objects[i];
                if (!obj->active || obj->done || obj->step == 0) continue;
                float dist = Vector3Distance(player.camera.position, obj->pos);
                if (dist < obj->radius) {
                    Vector3 to = Vector3Normalize(Vector3Subtract(obj->pos, player.camera.position));
                    Vector3 look = Vector3Normalize(Vector3Subtract(player.camera.target, player.camera.position));
                    if (Vector3DotProduct(to, look) > 0.5f) {
                        for (int s = 0; s < obj->max_steps; s++) {
                            int dx = RENDER_W/2 - (obj->max_steps-1)*4 + s*8;
                            Color dc = s < obj->step ? (Color){240,210,100,200} : (Color){100,95,85,120};
                            DrawCircle(dx, RENDER_H/2 + 12, 2, dc);
                        }
                    }
                }
            }
        }

        // Vignette text overlay
        draw_vignette_text();

        // Fade
        if (fade_alpha > 0.01f)
            DrawRectangle(0, 0, RENDER_W, RENDER_H, (Color){0,0,0,(unsigned char)(255*fade_alpha)});

        EndTextureMode();

        // Post-FX
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

        if (show_debug) {
            DrawText(TextFormat("FPS: %d", GetFPS()), 10, 10, 20, GREEN);
            DrawText(TextFormat("Walls: %d  State: %d", scene.wall_count, state), 10, 35, 20, GREEN);
            DrawText(TextFormat("Tasks: %d/%d  Pos: %.1f %.1f %.1f",
                tasks_done, total_tasks,
                player.camera.position.x, player.camera.position.y, player.camera.position.z),
                10, 60, 20, GREEN);
            if (player.noclip) DrawText("NOCLIP", 10, 85, 20, YELLOW);
            if (wireframe) DrawText("WIREFRAME", 10, 105, 20, YELLOW);
        }
        EndDrawing();
    }

    if (cube_model_loaded) UnloadModel(cube_model);
    UnloadEVLighting(&lighting);
    UnloadEVPostFX(&postfx);
    UnloadEVAudio(&audio);
    UnloadRenderTexture(render_target);
    UnloadRenderTexture(postfx_target);
    CloseWindow();
    return 0;
}
