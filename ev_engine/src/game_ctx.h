// game_ctx.h — Single struct replacing all file-scope statics in main.c
#ifndef EV_GAME_CTX_H
#define EV_GAME_CTX_H

#include "raylib.h"
#include "raymath.h"
#include "ev_types.h"
#include "lighting.h"
#include "render.h"
#include "audio.h"
#include "npc.h"
#include "player.h"
#include "scene.h"
#include "particle.h"
#include "skybox.h"
#include "physics.h"
#include <string.h>

// Rain drops for taxi scene
typedef struct { float x, y, speed, len; } Raindrop;
#define MAX_RAIN 60

// Pause menu
typedef enum { MENU_NONE, MENU_PAUSE, MENU_SETTINGS } MenuMode;
#define MENU_MAX_ITEMS 5
#define PAUSE_ITEM_COUNT 3
#define SETTINGS_ITEM_COUNT 5

#define PARIS_TASK_COUNT 5
#define SPACE_TASK_COUNT 4

// Scene description — function pointer table entry
typedef struct {
    void (*load)(void);
    void (*update)(float dt);
    bool indoor;
} SceneDesc;

typedef struct {
    // ── Core state machine ──
    GameState state;
    float state_time;
    float total_time;

    // ── Transition ──
    float fade_alpha;
    float fade_target;
    GameState next_state;
    bool transitioning;
    float fade_speed;
    float transition_hold;
    float hold_timer;

    // ── Player + Scene ──
    Player player;
    Scene scene;

    // ── Subsystems ──
    EVLighting lighting;
    EVPostFX postfx;
    EVAudio audio;

    // ── Models ──
    Model cube_model;
    bool cube_model_loaded;
    Model cyl_model;
    bool cyl_model_loaded;
    Model sphere_model;
    bool sphere_model_loaded;
    Model cone_model;
    bool cone_model_loaded;
    Model skytower_model;
    bool skytower_loaded;

    // ── Model assets (loaded from assets/*.glb, assets/*.obj) ──
    ModelAsset model_assets[MAX_MODEL_ASSETS];
    int model_asset_count;

    // ── Render targets ──
    RenderTexture2D render_target;
    RenderTexture2D postfx_target;

    // ── Task progress ──
    int tasks_done;
    float done_pause;

    // ── Phone ──
    bool phone_triggered;
    int phone_wall_idx;

    // ── Eiffel sparkle ──
    bool eiffel_sparkle;
    float sparkle_timer;

    // ── Elevator ──
    bool elevator_ding_played;
    bool elevator_to_corridor;

    // ── Room restore ──
    bool returning_to_room;
    const char *completed_objects[MAX_OBJECTS];
    int completed_count;

    // ── Balcony flash ──
    bool balcony_flash_triggered;
    float balcony_flash_timer;

    // ── Cigarette camera animation ──
    bool cigarette_anim;
    float cigarette_anim_timer;
    Vector3 cigarette_cam_origin;
    Vector3 cigarette_cam_target;

    // ── NPC ──
    NPC gibbons;
    bool gibbons_glanced;   // lobby: Gibbons looks behind you once (Commandment 9)
    bool photograph_flipped; // suite: photograph turned face-up once
    bool three_note_played;  // ending: 3-note callback plays once in stars

    // ── Paris dream window morphing ──
    int dream_tower_walls[3];    // indices of Eiffel/Sky Tower silhouette walls
    int dream_window_phase;      // 0=Eiffel, 1=Sky Tower, 2=neither

    // ── Clock deceleration ──
    float bed_clock_rate;

    // ── Door positions for spatial audio ──
    Vector3 door_positions[3];

    // ── Agency removal ──
    float agency_removal_timer;

    // ── Lobby memory palace ──
    int lobby_visit_count;

    // ── Skybox ──
    EVSkybox skybox;

    // ── Physics / Grab ──
    GrabSystem grab;

    // ── Particles ──
    ParticleSystem particles;

    // ── Debug / visual ──
    bool show_debug;
    bool wireframe;
    int current_style;
    float scene_exposure;

    // ── Nudge mode ──
    bool nudge_mode;
    int nudge_selected;

    // ── Vignette text (legacy — system messages) ──
    const char *vig_text;
    float text_y_offset;
    float text_y_velocity;
    float text_scale;
    float text_scale_vel;
    float text_scale_target;

    // ── Dialogue system ──
    const char *dlg_speaker;      // speaker name (NULL = narration)
    const char *dlg_text;         // full line text
    float dlg_timer;              // time since line started (drives typewriter)
    float dlg_chars_per_sec;      // typewriter speed (default 30)
    float dlg_hold_timer;         // time to hold after fully revealed
    float dlg_fade;               // 0..1 fade in/out
    bool dlg_active;              // dialogue visible

    // ── Rain ──
    Raindrop rain[MAX_RAIN];

    // ── Hard cut flash ──
    float cut_flash_timer;

    // ── Montage ──
    int montage_shot;         // current shot index (0-12)
    float montage_shot_time;  // time in current shot
    bool montage_flash;       // white flash between cuts

    // ── Interaction ritual timers ──
    float interaction_timers[4];
    int interaction_phases[4];

    // ── Ambient fade ──
    float ambient_fade;

    // ── Interaction micro-freeze + camera lean ──
    float interact_freeze;
    float interact_lean;
    float interact_lean_vel;

    // ── Narrative choices (Firewatch-style backstory) ──
    // Option A: pre-title black screen choices
    // Option B: taxi ride internal choices
    int choice_cursor;            // 0 or 1 (which option highlighted)
    bool choice_active;           // choice UI visible
    bool choice_confirmed;        // player pressed enter
    const char *choice_question;  // question text (centered top)
    const char *choice_a;         // option 0
    const char *choice_b;         // option 1
    int choice_result;            // what they picked (0 or 1), -1 if not yet
    // Backstory storage — persists across scenes, colors the experience
    int backstory[6];             // results of up to 6 choices (-1 = unanswered)
    int backstory_count;          // how many choices completed
    int backstory_phase;          // which choice we're on (pre-title or taxi)
    float beat_timer;             // time within current dialogue beat

    // ── Pause menu ──
    MenuMode menu_mode;
    int menu_cursor;
    float menu_item_x[MENU_MAX_ITEMS];
    float menu_item_vx[MENU_MAX_ITEMS];
    float menu_overlay_a;
    float menu_overlay_va;
    float setting_master_vol;
    bool setting_fullscreen;
} GameCtx;

// Initialize all fields to sane defaults
static inline void game_ctx_init(GameCtx *g) {
    // Zero everything first
    memset(g, 0, sizeof(GameCtx));

    // Non-zero defaults
    g->state = STATE_TITLE;
    g->fade_alpha = 1.0f;
    g->next_state = STATE_TITLE;
    g->fade_speed = 2.0f;
    g->transition_hold = 0.3f;
    g->phone_wall_idx = -1;
    g->bed_clock_rate = 1.0f;
    g->nudge_selected = -1;
    g->text_y_offset = 20.0f;
    g->dlg_chars_per_sec = 28.0f;
    g->ambient_fade = 1.0f;
    g->setting_master_vol = 1.0f;
    for (int i = 0; i < 6; i++) g->backstory[i] = -1;
}

// Scene registry — extern, defined in scene_registry.c
extern const SceneDesc scene_descs[];
extern const int scene_desc_count;

#endif
