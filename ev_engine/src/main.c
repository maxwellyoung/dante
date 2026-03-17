// ENDEARING VOID — Custom engine on Raylib
// Maxwell Young
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
static Model sphere_model = {0};
static bool sphere_model_loaded = false;
static Model cone_model = {0};
static bool cone_model_loaded = false;
static Model skytower_model = {0};
static bool skytower_loaded = false;
static RenderTexture2D render_target;
static RenderTexture2D postfx_target;

static int tasks_done = 0;
static const int total_tasks = 5;
static float done_pause = 0;

static bool phone_triggered = false;


static bool eiffel_sparkle = false;
static float sparkle_timer = 0;

static bool elevator_ding_played = false;

static bool returning_to_room = false;

// Rug pull — Gravity Bone flash on balcony
static bool balcony_flash_triggered = false;
static float balcony_flash_timer = 0;


// Cigarette camera animation (Phase 5)
static bool cigarette_anim = false;
static float cigarette_anim_timer = 0;
static Vector3 cigarette_cam_origin = {0};
static Vector3 cigarette_cam_target = {0};

static bool show_debug = false;
static bool wireframe = false;

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
    return INTERACT_CLICK;
}

static float transition_hold = 0.3f;
static float hold_timer = 0;

// Forward declaration
static void load_state(GameState s);

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
    load_state(s);
    fade_alpha = 0.0f;  fade_target = 0.0f;
    transitioning = false;  hold_timer = 0;
    cut_flash_timer = 0.12f;  // ~3 frames at 60fps
    SetPostFXFlash(&postfx, 1.0f, 1.0f, 0.95f, 0.85f);  // warm white flash
}

static void load_state(GameState s) {
    state = s;
    state_time = 0;
    done_pause = 0;
    vig_text = NULL;
    text_y_offset = 20.0f;
    text_y_velocity = 0.0f;
    text_scale = 0.0f;
    text_scale_vel = 0.0f;
    text_scale_target = 0.0f;

    switch (s) {
        case STATE_TITLE: DisableCursor(); StopAmbient(&audio); break;

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
            PlayCityAmbient(&audio);
            SetCityAmbientVolume(&audio, 0.04f);
            SetSceneLighting(&lighting, LightingPreset_Taxi());
            SetPostFXExposure(&postfx, -0.3f);  // dark taxi interior
            SetPostFXGrain(&postfx, 0.7f);       // grainy night footage
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
            SetPostFXExposure(&postfx, -0.4f);  // 2AM Paris streets
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
            SetSceneLighting(&lighting, LightingPreset_Lobby());
            SetPostFXExposure(&postfx, -0.1f);  // grand lobby, slightly dim
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

            PlayElevatorHum(&audio);
            SetSceneLighting(&lighting, LightingPreset_Elevator());
            SetPostFXExposure(&postfx, 0.0f);
            SetPostFXGrain(&postfx, 0.3f);  // fluorescent — less grain
            break;

        case STATE_HALLWAY:
            build_hallway(&scene);
            init_player(&player, scene.spawn);
            StartAmbient(&audio, DRONE_HALLWAY);
            StopClockAmbient(&audio);
            StopCityAmbient(&audio);

            StopWindAmbient(&audio);
            SetSceneLighting(&lighting, LightingPreset_Hallway());
            SetPostFXExposure(&postfx, -0.15f);  // dim corridor
            SetPostFXGrain(&postfx, 0.5f);
            break;

        case STATE_ROOM: {
            int saved_tasks = returning_to_room ? tasks_done : 0;
            bool saved_phone = returning_to_room ? phone_triggered : false;
            float saved_warmth = returning_to_room ? (float)tasks_done / total_tasks : 0.0f;

            build_hotel_room(&scene);

            if (returning_to_room) {
                // Spawn near bathroom door, restore task progress
                init_player(&player, (Vector3){4.5f, 1.6f, -2.5f});
                tasks_done = saved_tasks;
                phone_triggered = saved_phone;
                SetPostFXWarmth(&postfx, saved_warmth);
                scene.fog_density = 0.004f - (saved_warmth * 0.003f);
                // Mark completed objects as done
                for (int i = 0; i < scene.object_count && i < saved_tasks; i++) {
                    scene.objects[i].done = true;
                    scene.objects[i].step = scene.objects[i].max_steps;
                }
                returning_to_room = false;
            } else {
                init_player(&player, scene.spawn);
                tasks_done = 0;
                phone_triggered = false;
                SetPostFXWarmth(&postfx, 0.0f);
            }
            StartAmbient(&audio, DRONE_ROOM);
            PlayClockAmbient(&audio);
            StopCityAmbient(&audio);

            StopWindAmbient(&audio);
            SetSceneLighting(&lighting, LightingPreset_Room());
            SetPostFXExposure(&postfx, -0.05f);  // slight darkness — 2AM
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
            SetPostFXExposure(&postfx, 0.1f);   // bright bathroom fluorescent
            SetPostFXGrain(&postfx, 0.3f);      // clinical — less grain
            break;

        case STATE_BALCONY:
            build_balcony(&scene);
            init_player(&player, scene.spawn);
            eiffel_sparkle = false; sparkle_timer = 0;
            cigarette_anim = false; cigarette_anim_timer = 0;
            StopAmbient(&audio);
            StopClockAmbient(&audio);

            PlayCityAmbient(&audio);
            SetCityAmbientVolume(&audio, 0.015f);  // pre-dawn quiet — half volume
            PlayWindAmbient(&audio);
            SetPostFXWarmth(&postfx, 1.0f);
            SetSceneLighting(&lighting, LightingPreset_Balcony());
            SetPostFXExposure(&postfx, -0.25f);  // pre-dawn darkness
            SetPostFXGrain(&postfx, 0.6f);       // contemplative night grain
            break;

        case STATE_BED: StopAmbient(&audio); StopClockAmbient(&audio); StopCityAmbient(&audio); StopWindAmbient(&audio); break;
        case STATE_STARS: StopAmbient(&audio); StopClockAmbient(&audio); StopCityAmbient(&audio); StopWindAmbient(&audio); break;

        case STATE_SPACE_LOBBY:
            build_space_lobby(&scene);
            init_player(&player, scene.spawn);
            StopAmbient(&audio);
            StopClockAmbient(&audio);
            StopCityAmbient(&audio);
            StopWindAmbient(&audio);
            StartAmbient(&audio, DRONE_LOBBY);
            SetSceneLighting(&lighting, LightingPreset_Space());
            SetPostFXExposure(&postfx, -0.2f);
            SetPostFXGrain(&postfx, 0.4f);
            break;

        case STATE_SPACE_CORRIDOR:
            build_space_corridor(&scene);
            init_player(&player, scene.spawn);
            StopAmbient(&audio);
            StopClockAmbient(&audio);
            StopCityAmbient(&audio);
            StopWindAmbient(&audio);
            StartAmbient(&audio, DRONE_HALLWAY);
            SetSceneLighting(&lighting, LightingPreset_Space());
            SetPostFXExposure(&postfx, -0.3f);
            SetPostFXGrain(&postfx, 0.4f);
            break;

        case STATE_SPACE_SUITE:
            build_space_suite(&scene);
            init_player(&player, scene.spawn);
            StopAmbient(&audio);
            StopClockAmbient(&audio);
            StopCityAmbient(&audio);
            StopWindAmbient(&audio);
            StartAmbient(&audio, DRONE_ROOM);
            SetSceneLighting(&lighting, LightingPreset_Space());
            SetPostFXExposure(&postfx, -0.15f);
            SetPostFXGrain(&postfx, 0.4f);
            break;
    }
    fade_alpha = 1.0f;
    fade_target = 0.0f;
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
        if (lighting.ready) skytower_model.materials[0].shader = lighting.shader;
        skytower_loaded = true;
        printf("[EV] Sky Tower model loaded — %d verts\n",
               skytower_model.meshes[0].vertexCount);
    } else {
        printf("[EV] WARNING: assets/skytower.obj not found\n");
    }

    DisableCursor();
#ifdef DEV_START
    load_state(DEV_START);
#else
    load_state(STATE_TITLE);
#endif

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        state_time += dt;

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

        if (IsKeyPressed(KEY_F1)) wireframe = !wireframe;
        if (IsKeyPressed(KEY_F3)) show_debug = !show_debug;
        if (IsKeyPressed(KEY_F4)) player.noclip = !player.noclip;
        // Dev scene jumps — number keys for instant room testing
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
            case STATE_TITLE:
                if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) transition_to(STATE_CAR);
                break;

            case STATE_CAR: {
                // 3D taxi ride — mouse look, no movement, city scrolls past
                // Mouse look (yaw/pitch only, no position change)
                Vector2 md = GetMouseDelta();
                float car_yaw = -md.x * MOUSE_SENS;
                float car_pitch = -md.y * MOUSE_SENS;
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

                // Auto-scroll city walls — taxi speed decreasing toward end
                float taxi_speed = 8.0f;
                if (state_time > 10.0f) taxi_speed = fmaxf(0.5f, 8.0f - (state_time - 10.0f) * 2.5f);
                for (int i = scene.static_wall_count; i < scene.wall_count; i++) {
                    scene.walls[i].pos.z += taxi_speed * dt;
                    // Wrap city walls that pass behind us
                    if (scene.walls[i].pos.z > 10.0f) {
                        scene.walls[i].pos.z -= 220.0f;
                    }
                }

                // Vignette text
                if (state_time > 1.5f && state_time < 2.0f) show_text("Paris. 2:47 AM.");
                if (state_time > 5.0f && state_time < 5.5f) hide_text();
                // No text — the taxi meter shows 2:47, the player connects the dots
                if (state_time > 13.5f) hard_cut_to(STATE_DRIVING);
                if (IsKeyPressed(KEY_ENTER) && state_time > 3) hard_cut_to(STATE_DRIVING);
                break;
            }

            case STATE_DRIVING: {
                // Arrival — taxi stopped, same scene, mouse look only
                Vector2 md2 = GetMouseDelta();
                float drv_yaw = -md2.x * MOUSE_SENS;
                float drv_pitch = -md2.y * MOUSE_SENS;
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
                                if (strcmp(obj->name, "newspaper") == 0) {
                                    obj->step++; obj->done = true;
                                    PlayInteract(&audio, INTERACT_CLICK);
                                    break;
                                }
                            }
                        }
                    }
                }
                if (scene.has_exit) {
                    float dist = Vector3Distance(player.camera.position, scene.exit_pos);
                    if (dist < 1.5f) transition_to(STATE_HALLWAY);
                }
                break;

            case STATE_ELEVATOR:
                // Tiny elevator ride — auto-transition after 4 seconds
                // Subtle upward drift to simulate going up
                player.camera.position.y += 0.3f * dt;
                player.camera.target.y += 0.3f * dt;
                // Ding at 2 seconds — the sound is enough
                if (state_time > 2.0f && !elevator_ding_played) {
                    elevator_ding_played = true;
                    PlayElevatorDing(&audio);
                }
                // Auto-transition to hallway at 4 seconds
                if (state_time > 4.0f) transition_to(STATE_HALLWAY);
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
                                    // Hard cut to lobby — you emerge on the mezzanine
                                    // Disorienting, Blendo-style, but rewarding
                                    load_state(STATE_LOBBY);
                                    // Override spawn to mezzanine position
                                    player.camera.position = scene.exit_pos;
                                    player.ground_y = 2.4f;  // mezzanine height
                                    Vector3 fwd = {0, player.camera.position.y, player.camera.position.z - 1};
                                    player.camera.target = fwd;
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
                // Phone lights up at 3 tasks — screen glow is enough
                if (tasks_done >= 3 && !phone_triggered) {
                    phone_triggered = true;
                    // Bright screen on phone — bigger, brighter, the glow tells the story
                    add_wall(&scene, 5.2f, 0.87f, 0.15f, 0.2f, 0.01f, 0.1f, (Color){100,180,240,220});
                }
                // Phone glow pulse — the screen breathes
                if (phone_triggered) {
                    float pulse = 0.6f + 0.4f * sinf((float)GetTime() * 3.0f);
                    unsigned char pa = (unsigned char)(220 * pulse);
                    // Find the phone glow wall (last added wall matching phone position)
                    for (int i = scene.wall_count - 1; i >= 0; i--) {
                        Wall *w = &scene.walls[i];
                        if (w->active && fabsf(w->pos.x - 5.2f) < 0.01f &&
                            fabsf(w->pos.y - 0.87f) < 0.01f &&
                            fabsf(w->size.x - 0.2f) < 0.01f) {
                            w->color.a = pa;
                            break;
                        }
                    }
                }

                if (tasks_done >= total_tasks) {
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
                if (!cigarette_anim) update_player(&player, &scene, dt);
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
                // Silence — then bed (after the flash settles)
                if (state_time > 14.0f)
                    transition_to_slow(STATE_BED, 0.7f);
                if (state_time > 10 && IsKeyPressed(KEY_ENTER))
                    transition_to_slow(STATE_BED, 0.7f);
                break;

            case STATE_BED:
                // Ceiling materializes, stars emerge, then hard cut to void
                if (state_time > 16 || (state_time > 10 && IsKeyPressed(KEY_ENTER)))
                    hard_cut_to(STATE_STARS);
                break;

            case STATE_STARS:
                if (IsKeyPressed(KEY_ESCAPE) || (state_time > 10 && IsKeyPressed(KEY_ENTER))) {
                    CloseWindow(); return 0;
                }
                break;

            case STATE_SPACE_LOBBY:
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
                                obj->step++; obj->done = true;
                                PlayInteract(&audio, INTERACT_CLICK);
                                kick_camera(&player, -0.01f, 0.005f);
                                break;
                            }
                        }
                    }
                }
                if (scene.has_exit) {
                    float dist = Vector3Distance(player.camera.position, scene.exit_pos);
                    if (dist < 1.5f) transition_to(STATE_SPACE_CORRIDOR);
                }
                break;

            case STATE_SPACE_CORRIDOR:
                update_player(&player, &scene, dt);
                UpdateEVAudio(&audio, player.moving, player.sprinting, scene.surface, dt);
                if (scene.has_exit) {
                    float dist = Vector3Distance(player.camera.position, scene.exit_pos);
                    if (dist < 1.5f) transition_to(STATE_SPACE_SUITE);
                }
                break;

            case STATE_SPACE_SUITE:
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
                                obj->step++;
                                PlayInteract(&audio, get_interact_sound(obj->name));
                                kick_camera(&player, -0.01f, 0.005f);
                                if (obj->step >= obj->max_steps) {
                                    obj->done = true;
                                    obj->reward_timer = 1.5f;
                                    PlayRewardSound(&audio);
                                }
                                break;
                            }
                        }
                    }
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
            case STATE_DRIVING:
                // 3D taxi scene with night sky
                ClearBackground((Color){8, 12, 28, 255});
                draw_night_sky(state_time);
                draw_scene_3d(&player, &scene, &lighting, &cube_model, cube_model_loaded,
                              &cyl_model, cyl_model_loaded,
                              &sphere_model, sphere_model_loaded,
                              &cone_model, cone_model_loaded,
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
                if (state == STATE_HOTEL_EXT || state == STATE_BALCONY) {
                    ClearBackground((Color){8, 12, 28, 255});
                    draw_night_sky(state_time);
                } else if (state == STATE_SPACE_LOBBY || state == STATE_SPACE_CORRIDOR
                           || state == STATE_SPACE_SUITE) {
                    ClearBackground((Color){4, 5, 12, 255});
                } else {
                    ClearBackground(scene.fog_color);
                }
                {
                    bool indoor = !(state == STATE_HOTEL_EXT || state == STATE_BALCONY);
                    draw_scene_3d(&player, &scene, &lighting, &cube_model, cube_model_loaded,
                                  &cyl_model, cyl_model_loaded,
                                  &sphere_model, sphere_model_loaded,
                                  &cone_model, cone_model_loaded,
                                  indoor, state_time);
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
            state == STATE_HALLWAY ||
            state == STATE_SPACE_LOBBY || state == STATE_SPACE_CORRIDOR ||
            state == STATE_SPACE_SUITE) {
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

        // Crosshair — drawn by draw_hud for interactive scenes;
        // simple dot for non-interactive 3D scenes
        if (state == STATE_HOTEL_EXT || state == STATE_CAR || state == STATE_DRIVING ||
            state == STATE_SPACE_LOBBY || state == STATE_SPACE_CORRIDOR ||
            state == STATE_SPACE_SUITE) {
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
            const char *state_names[] = {
                "TITLE", "CAR", "DRIVING", "EXTERIOR", "LOBBY",
                "ELEVATOR", "HALLWAY", "ROOM", "BATHROOM", "BALCONY",
                "BED", "STARS", "SP_LOBBY", "SP_CORRIDOR", "SP_SUITE"
            };
            const char *sn = (state >= 0 && state < 15) ? state_names[state] : "???";
            DrawText(TextFormat("FPS: %d", GetFPS()), 10, 10, 20, GREEN);
            DrawText(TextFormat("Walls: %d  %s", scene.wall_count, sn), 10, 35, 20, GREEN);
            DrawText(TextFormat("Pos: %.1f %.1f %.1f",
                player.camera.position.x, player.camera.position.y, player.camera.position.z),
                10, 60, 20, GREEN);
            DrawText("1-9: rooms  F4: noclip", 10, 85, 20, (Color){100,200,100,180});
            if (player.noclip) DrawText("NOCLIP", 10, 105, 20, YELLOW);
            if (wireframe) DrawText("WIREFRAME", 10, 125, 20, YELLOW);
        }
        EndDrawing();
    }

    if (cube_model_loaded) UnloadModel(cube_model);
    if (cyl_model_loaded) UnloadModel(cyl_model);
    if (sphere_model_loaded) UnloadModel(sphere_model);
    if (cone_model_loaded) UnloadModel(cone_model);
    UnloadEVLighting(&lighting);
    UnloadEVPostFX(&postfx);
    UnloadEVAudio(&audio);
    UnloadRenderTexture(render_target);
    UnloadRenderTexture(postfx_target);
    CloseWindow();
    return 0;
}
