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
static Model cyl_model = {0};
static bool cyl_model_loaded = false;
static RenderTexture2D render_target;
static RenderTexture2D postfx_target;

static int tasks_done = 0;
static const int total_tasks = 5;
static float done_pause = 0;

static bool phone_triggered = false;
static float phone_text_timer = 0;

static float bathroom_text_timer = 0;
static bool bathroom_text_active = false;

static bool eiffel_sparkle = false;
static float sparkle_timer = 0;

static bool elevator_ding_played = false;

static bool returning_to_room = false;

static float lobby_text_timer = 0;
static bool lobby_text_active = false;

static float balcony_text_timer = 0;
static bool balcony_text_active = false;

static bool room_intro_shown = false;

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

static float transition_hold = 0.3f;
static float hold_timer = 0;

static void transition_to(GameState s) {
    transitioning = true;
    next_state = s;
    fade_target = 1.0f;
    fade_speed = 2.0f;
    transition_hold = 0.3f;
    hold_timer = 0;
    PlayDoorSound(&audio);
}

static void transition_to_slow(GameState s, float spd) {
    transitioning = true;
    next_state = s;
    fade_target = 1.0f;
    fade_speed = spd;
    transition_hold = (s == STATE_BED) ? 1.0f : 0.3f;
    hold_timer = 0;
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
            StopClockAmbient(&audio);
            StopStairwellAmbient(&audio);
            PlayCityAmbient(&audio);
            PlayWindAmbient(&audio);
            break;

        case STATE_LOBBY:
            EnableCursor(); DisableCursor();
            build_lobby(&scene);
            init_player(&player, scene.spawn);
            lobby_text_active = false;
            lobby_text_timer = 0;
            StartAmbient(&audio, DRONE_LOBBY);
            StopClockAmbient(&audio);
            StopCityAmbient(&audio);
            StopStairwellAmbient(&audio);
            StopWindAmbient(&audio);
            break;

        case STATE_ELEVATOR:
            build_elevator(&scene);
            init_player(&player, scene.spawn);
            elevator_ding_played = false;
            StopAmbient(&audio);
            StopClockAmbient(&audio);
            StopCityAmbient(&audio);
            StopWindAmbient(&audio);
            StopStairwellAmbient(&audio);
            PlayElevatorHum(&audio);
            break;

        case STATE_STAIRWELL:
            build_stairwell(&scene);
            init_player(&player, scene.spawn);
            StopAmbient(&audio);
            StopClockAmbient(&audio);
            StopCityAmbient(&audio);
            StopWindAmbient(&audio);
            PlayStairwellAmbient(&audio);
            break;

        case STATE_HALLWAY:
            build_hallway(&scene);
            init_player(&player, scene.spawn);
            StartAmbient(&audio, DRONE_HALLWAY);
            StopClockAmbient(&audio);
            StopCityAmbient(&audio);
            StopStairwellAmbient(&audio);
            StopWindAmbient(&audio);
            break;

        case STATE_ROOM:
            if (!returning_to_room) {
                build_hotel_room(&scene);
                init_player(&player, scene.spawn);
                tasks_done = 0;
                phone_triggered = false;
                phone_text_timer = 0;
                room_intro_shown = false;
                SetPostFXWarmth(&postfx, 0.0f);
            } else {
                // Returning from bathroom — preserve room state
                init_player(&player, (Vector3){5.0f, 1.6f, -3.0f});
                returning_to_room = false;
            }
            StartAmbient(&audio, DRONE_ROOM);
            PlayClockAmbient(&audio);
            StopCityAmbient(&audio);
            StopStairwellAmbient(&audio);
            StopWindAmbient(&audio);
            break;

        case STATE_BATHROOM:
            build_bathroom(&scene);
            init_player(&player, scene.spawn);
            bathroom_text_active = false;
            bathroom_text_timer = 0;
            StartAmbient(&audio, DRONE_ROOM);
            StopClockAmbient(&audio);
            StopCityAmbient(&audio);
            StopStairwellAmbient(&audio);
            StopWindAmbient(&audio);
            break;

        case STATE_BALCONY:
            build_balcony(&scene);
            init_player(&player, scene.spawn);
            eiffel_sparkle = false; sparkle_timer = 0;
            balcony_text_active = false; balcony_text_timer = 0;
            StopAmbient(&audio);
            StopClockAmbient(&audio);
            StopStairwellAmbient(&audio);
            PlayCityAmbient(&audio);
            PlayWindAmbient(&audio);
            SetPostFXWarmth(&postfx, 1.0f);
            break;

        case STATE_ROOF:
            build_roof(&scene);
            init_player(&player, scene.spawn);
            StopAmbient(&audio);
            StopClockAmbient(&audio);
            StopStairwellAmbient(&audio);
            PlayCityAmbient(&audio);
            PlayWindAmbient(&audio);
            SetPostFXWarmth(&postfx, 1.0f);
            break;

        case STATE_BED: StopAmbient(&audio); StopClockAmbient(&audio); StopCityAmbient(&audio); StopStairwellAmbient(&audio); StopWindAmbient(&audio); break;
        case STATE_STARS: StopAmbient(&audio); StopClockAmbient(&audio); StopCityAmbient(&audio); StopStairwellAmbient(&audio); StopWindAmbient(&audio); break;
    }
    fade_alpha = 1.0f;
    fade_target = 0.0f;
}

// ============================================================
// VIGNETTE DRAWING
// ============================================================

static void draw_taxi_ride(float t) {
    // Jeremy Blake — abstract luminous color fields seen through wet glass
    // NOT literal: the FEELING of Paris at 2 AM from a moving taxi
    ClearBackground((Color){4, 4, 8, 255});

    // Speed factor — shapes slow as taxi approaches destination
    float speed = 1.0f;
    if (t > 10) speed = fmaxf(0.05f, 1.0f - (t - 10) / 4.0f);

    // 4 large overlapping color fields drifting leftward at different speeds (parallax)
    // Colors: warm amber, deep blue, soft rose, white
    typedef struct { float base_x; float drift_speed; int w; int h; int y_off;
                     unsigned char r, g, b, a; } BlakeShape;
    BlakeShape shapes[] = {
        {  0, 55, 180, RENDER_H,      0,   240, 180,  80,  55},  // warm amber, full height
        {120, 35, 140, RENDER_H*2/3, 40,    40,  60, 140,  50},  // deep blue, 2/3 height
        {300, 45, 120, RENDER_H,      0,   200, 120, 130,  45},  // soft rose, full height
        {200, 20, 160, RENDER_H*3/4, 20,   245, 240, 235,  40},  // white, 3/4 height
    };
    int shape_count = 4;

    for (int i = 0; i < shape_count; i++) {
        BlakeShape *s = &shapes[i];
        // Drift leftward, wrapping
        float x = fmodf(s->base_x - t * s->drift_speed * speed + RENDER_W * 3, (float)(RENDER_W + s->w)) - s->w;
        // Alpha modulated slightly by time for breathing effect
        float breath = 0.85f + 0.15f * sinf(t * 0.7f + i * 1.5f);
        unsigned char a = (unsigned char)(s->a * breath);
        DrawRectangle((int)x, s->y_off, s->w, s->h, (Color){s->r, s->g, s->b, a});
        // Softer edge glow
        DrawRectangle((int)x - 10, s->y_off, 10, s->h, (Color){s->r, s->g, s->b, (unsigned char)(a / 3)});
        DrawRectangle((int)x + s->w, s->y_off, 10, s->h, (Color){s->r, s->g, s->b, (unsigned char)(a / 3)});
    }

    // Occasional bright horizontal streak — headlight passing
    // Fast, thin, white, random timing
    SetRandomSeed(42);
    for (int i = 0; i < 3; i++) {
        float streak_time = 2.5f + i * 4.3f;
        float streak_dur = 0.15f;
        if (t > streak_time && t < streak_time + streak_dur) {
            float streak_t = (t - streak_time) / streak_dur;
            int sy = 40 + GetRandomValue(0, RENDER_H - 80);
            unsigned char sa = (unsigned char)(180 * (1.0f - fabsf(streak_t * 2.0f - 1.0f)));
            DrawRectangle(0, sy, RENDER_W, 2, (Color){255, 252, 245, sa});
        }
    }

    // Rain — small, fast, light drizzle (not defined drops)
    for (int i = 0; i < MAX_RAIN; i++) {
        rain[i].y += rain[i].speed * 1.5f * GetFrameTime();  // faster
        if (rain[i].y > RENDER_H) {
            rain[i].y = -(rain[i].len * 0.5f);
            rain[i].x = GetRandomValue(0, RENDER_W);
        }
        // Smaller drops — 1px dots instead of lines
        DrawPixel((int)rain[i].x, (int)rain[i].y, (Color){200, 210, 225, 35});
    }

    // Taxi meter in the corner — concrete anchor
    DrawRectangle(RENDER_W - 80, RENDER_H - 50, 55, 25, (Color){20, 18, 15, 255});
    DrawRectangle(RENDER_W - 78, RENDER_H - 48, 51, 21, (Color){40, 50, 35, 255});
    float fare = 12.40f + t * 0.15f;
    DrawText(TextFormat("%.2f", fare), RENDER_W - 74, RENDER_H - 45, 12,
             (Color){120, 230, 120, 200});

    // Driver silhouette — anchoring detail
    DrawRectangle(55, RENDER_H - 80, 35, 30, (Color){10, 8, 6, 255});
    DrawCircle(72, RENDER_H - 82, 12, (Color){10, 8, 6, 255});
}

static void draw_arrival(float t) {
    // Jeremy Blake — the abstract color fields SLOW and SETTLE
    // Motion resolves into warmth. A large golden rectangle centers itself.
    // The hotel emerging from abstraction.
    ClearBackground((Color){4, 4, 8, 255});

    // Shapes settle: drift speed decays to near zero over 5 seconds
    float settle = fminf(1.0f, t / 5.0f);  // 0 = moving, 1 = still
    float drift_mul = 1.0f - settle * 0.95f;  // shapes nearly stop

    // Same 4 Blake shapes but converging to center, warm tones dominating
    // As settle increases, shapes drift toward center and alpha increases for warm tones
    typedef struct { float start_x; float end_x; int w; int h; int y_off;
                     unsigned char r, g, b; float start_a, end_a; } ArrivalShape;
    ArrivalShape shapes[] = {
        // Warm amber — grows dominant, centers
        { 50, RENDER_W/2 - 100, 180, RENDER_H, 0,   240, 195, 100, 55, 80},
        // Deep blue — fades and moves off left
        {200, -60, 120, RENDER_H*2/3, 40,    40,  60, 140, 40, 15},
        // Soft rose — settles right of center
        {350, RENDER_W/2 + 30, 130, RENDER_H, 0,   210, 140, 130, 45, 55},
        // White glow — centers and brightens (the hotel entrance light)
        {180, RENDER_W/2 - 80, 160, RENDER_H*3/4, 20,   250, 245, 235, 30, 60},
    };
    int shape_count = 4;

    for (int i = 0; i < shape_count; i++) {
        ArrivalShape *s = &shapes[i];
        // Interpolate position from start to settled end
        float x = s->start_x + (s->end_x - s->start_x) * settle;
        // Small residual drift
        x += sinf(t * 0.3f + i * 2.0f) * 5.0f * drift_mul;
        // Alpha interpolates too
        float a = s->start_a + (s->end_a - s->start_a) * settle;
        float breath = 0.9f + 0.1f * sinf(t * 0.5f + i * 1.3f);
        unsigned char alpha = (unsigned char)(a * breath);
        DrawRectangle((int)x, s->y_off, s->w, s->h, (Color){s->r, s->g, s->b, alpha});
        // Soft edges
        DrawRectangle((int)x - 8, s->y_off, 8, s->h, (Color){s->r, s->g, s->b, (unsigned char)(alpha / 3)});
        DrawRectangle((int)x + s->w, s->y_off, 8, s->h, (Color){s->r, s->g, s->b, (unsigned char)(alpha / 3)});
    }

    // "HOTEL CHEVALIER" text fades in over the settled composition
    float text_alpha = fmaxf(0, fminf(1.0f, (t - 2.5f) / 2.0f));
    if (text_alpha > 0) {
        const char *name = "HOTEL CHEVALIER";
        int tw = MeasureText(name, 10);
        DrawText(name, RENDER_W/2 - tw/2, RENDER_H/2 - 20, 10,
                 (Color){240, 220, 160, (unsigned char)(230 * text_alpha)});
    }

    // Subtle rain — lighter, slower, dots
    for (int i = 0; i < MAX_RAIN / 3; i++) {
        rain[i].y += rain[i].speed * GetFrameTime() * 0.2f;
        if (rain[i].y > RENDER_H) rain[i].y = -(rain[i].len * 0.3f);
        DrawPixel((int)rain[i].x, (int)rain[i].y, (Color){200, 210, 225, 25});
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

    Mesh cyl_mesh = GenMeshCylinder(0.5f, 1.0f, 16);
    cyl_model = LoadModelFromMesh(cyl_mesh);
    if (lighting.ready) cyl_model.materials[0].shader = lighting.shader;
    cyl_model_loaded = true;

    DisableCursor();
    load_state(STATE_TITLE);

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        state_time += dt;

        if (IsKeyPressed(KEY_F1)) wireframe = !wireframe;
        if (IsKeyPressed(KEY_F3)) show_debug = !show_debug;
        if (IsKeyPressed(KEY_F4)) player.noclip = !player.noclip;

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
                                    transition_to(STATE_ELEVATOR);
                                    break;
                                }
                                if (strcmp(obj->name, "newspaper") == 0) {
                                    obj->step++; obj->done = true;
                                    PlayInteract(&audio, INTERACT_CLICK);
                                    show_text("Yesterday's Le Monde.");
                                    lobby_text_active = true;
                                    lobby_text_timer = 0;
                                    break;
                                }
                            }
                        }
                    }
                }
                // Auto-hide storytelling text after 3s
                if (lobby_text_active) {
                    lobby_text_timer += dt;
                    if (lobby_text_timer > 3.0f) {
                        hide_text();
                        lobby_text_active = false;
                    }
                }
                if (scene.has_exit) {
                    float dist = Vector3Distance(player.camera.position, scene.exit_pos);
                    if (dist < 1.5f) transition_to(STATE_STAIRWELL);
                }
                break;

            case STATE_ELEVATOR:
                // Tiny elevator ride — auto-transition after 4 seconds
                // Subtle upward drift to simulate going up
                player.camera.position.y += 0.3f * dt;
                player.camera.target.y += 0.3f * dt;
                // Ding at 2 seconds
                if (state_time > 2.0f && !elevator_ding_played) {
                    elevator_ding_played = true;
                    PlayElevatorDing(&audio);
                    show_text("2");
                }
                if (state_time > 3.0f) hide_text();
                // Auto-transition to hallway at 4 seconds
                if (state_time > 4.0f) transition_to(STATE_HALLWAY);
                break;

            case STATE_STAIRWELL:
                update_player(&player, &scene, dt);
                UpdateEVAudio(&audio, player.moving, player.sprinting, scene.surface, dt);
                if (scene.has_exit) {
                    float dist = Vector3Distance(player.camera.position, scene.exit_pos);
                    if (dist < 1.5f) {
                        // Upper landing (y > 5) goes to roof, lower goes to hallway
                        if (player.camera.position.y > 5.0f)
                            transition_to(STATE_ROOF);
                        else
                            transition_to(STATE_HALLWAY);
                    }
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
                if (!room_intro_shown && state_time > 3.0f) {
                    room_intro_shown = true;
                    show_text("Three hours to kill.");
                }
                if (room_intro_shown && state_time > 7.0f && vig_text != NULL) {
                    hide_text();
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
                                // Bathroom door — transition, not a task
                                if (strcmp(obj->name, "bathroom") == 0) {
                                    transition_to(STATE_BATHROOM);
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
                // Phone lights up at 3 tasks — "Missed call."
                if (tasks_done >= 3 && !phone_triggered) {
                    phone_triggered = true;
                    phone_text_timer = 0;
                    // Bright screen on phone
                    add_wall(&scene, 5.2f, 0.87f, 0.15f, 0.12f, 0.01f, 0.06f, (Color){120,180,230,180});
                    show_text("Missed call.");
                }
                if (phone_triggered && phone_text_timer < 10.0f) {
                    phone_text_timer += dt;
                    if (phone_text_timer > 4.0f && phone_text_timer < 4.5f) hide_text();
                }

                if (tasks_done >= total_tasks) {
                    done_pause += dt;
                    if (done_pause > 0.5f && done_pause < 1.0f) {
                        // Force-clear any lingering phone text
                        phone_text_timer = 10.0f;
                        show_text("She'll be here soon.");
                    }
                    if (done_pause > 4.5f && done_pause < 5.0f) hide_text();
                    if (done_pause > 5.5f) transition_to(STATE_BALCONY);
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
                                    show_text("You look tired.");
                                    bathroom_text_active = true;
                                    bathroom_text_timer = 0;
                                    break;
                                }
                            }
                        }
                    }
                }
                // Bathroom text auto-hide after 3 seconds
                if (bathroom_text_active) {
                    bathroom_text_timer += dt;
                    if (bathroom_text_timer > 3.0f) {
                        hide_text();
                        bathroom_text_active = false;
                    }
                }
                break;

            case STATE_ROOF:
                update_player(&player, &scene, dt);
                UpdateEVAudio(&audio, player.moving, player.sprinting, scene.surface, dt);
                if (scene.has_exit) {
                    float dist = Vector3Distance(player.camera.position, scene.exit_pos);
                    if (dist < 2.0f) transition_to(STATE_BALCONY);
                }
                break;

            case STATE_BALCONY:
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
                                if (strcmp(obj->name, "cigarette") == 0) {
                                    obj->step++; obj->done = true;
                                    PlayInteract(&audio, INTERACT_CLICK);
                                    show_text("You quit last year.");
                                    balcony_text_active = true;
                                    balcony_text_timer = 0;
                                    break;
                                }
                            }
                        }
                    }
                }
                // Balcony text auto-hide after 3 seconds
                if (balcony_text_active) {
                    balcony_text_timer += dt;
                    if (balcony_text_timer > 3.0f) {
                        hide_text();
                        balcony_text_active = false;
                    }
                }
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
                draw_title();
                break;

            case STATE_CAR:
                draw_taxi_ride(state_time);
                break;

            case STATE_DRIVING:
                draw_arrival(state_time);
                break;

            case STATE_HOTEL_EXT:
            case STATE_LOBBY:
            case STATE_ELEVATOR:
            case STATE_STAIRWELL:
            case STATE_HALLWAY:
            case STATE_ROOM:
            case STATE_BATHROOM:
            case STATE_BALCONY:
            case STATE_ROOF:
                if (state == STATE_HOTEL_EXT || state == STATE_BALCONY || state == STATE_ROOF) {
                    ClearBackground((Color){8, 12, 28, 255});
                    draw_night_sky(state_time);
                } else {
                    ClearBackground(scene.fog_color);
                }
                {
                    bool indoor = !(state == STATE_HOTEL_EXT || state == STATE_BALCONY || state == STATE_ROOF);
                    draw_scene_3d(&player, &scene, &lighting, &cube_model, cube_model_loaded,
                                  &cyl_model, cyl_model_loaded, indoor, state_time);
                }
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
        if (state == STATE_ROOM || state == STATE_BATHROOM ||
            state == STATE_LOBBY || state == STATE_ELEVATOR || state == STATE_BALCONY ||
            state == STATE_HALLWAY || state == STATE_STAIRWELL || state == STATE_ROOF) {
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

        // Crosshair — subtle dot, always visible in 3D scenes
        if (state == STATE_HOTEL_EXT || state == STATE_LOBBY || state == STATE_ELEVATOR ||
            state == STATE_STAIRWELL || state == STATE_HALLWAY || state == STATE_ROOM ||
            state == STATE_BATHROOM || state == STATE_BALCONY || state == STATE_ROOF) {
            DrawPixel(RENDER_W/2, RENDER_H/2, (Color){220, 215, 205, 60});
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
    if (cyl_model_loaded) UnloadModel(cyl_model);
    UnloadEVLighting(&lighting);
    UnloadEVPostFX(&postfx);
    UnloadEVAudio(&audio);
    UnloadRenderTexture(render_target);
    UnloadRenderTexture(postfx_target);
    CloseWindow();
    return 0;
}
