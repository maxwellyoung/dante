// game_ctx.h — Central game context (all file-scope state)
#ifndef GAME_CTX_H
#define GAME_CTX_H

#include "ev_types.h"
#include "lighting.h"
#include "render.h"
#include "audio.h"
#include "npc.h"

// ── Raindrop (taxi scene) ─────────────────────────────────────────
typedef struct { float x, y, speed, len; } Raindrop;
#define MAX_RAIN 60

// ── Menu system ────────────────────────────────────────────────────
typedef enum { MENU_NONE, MENU_PAUSE, MENU_SETTINGS } MenuMode;
#define MENU_MAX_ITEMS 5
#define PAUSE_ITEM_COUNT 3
#define SETTINGS_ITEM_COUNT 5

// ── GameCtx — all mutable game state ─────────────────────────────
typedef struct {
    // Core state machine
    GameState state;
    float state_time;
    float total_time;
    float fade_alpha;
    float fade_target;
    float fade_speed;
    GameState next_state;
    bool transitioning;
    float transition_hold;
    float hold_timer;
    float cut_flash_timer;

    // World
    Player player;
    Scene scene;
    EVLighting lighting;
    EVPostFX postfx;
    EVAudio audio;

    // Geometry models
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

    // Render targets
    RenderTexture2D render_target;
    RenderTexture2D postfx_target;

    // Task / progress
    int tasks_done;
    float done_pause;
    bool phone_triggered;
    int phone_wall_idx;
    bool eiffel_sparkle;
    float sparkle_timer;
    bool elevator_ding_played;
    bool elevator_to_corridor;
    bool returning_to_room;
    const char *completed_objects[MAX_OBJECTS];
    int completed_count;

    // Scene-specific state
    bool balcony_flash_triggered;
    float balcony_flash_timer;
    bool cigarette_anim;
    float cigarette_anim_timer;
    Vector3 cigarette_cam_origin;
    Vector3 cigarette_cam_target;
    NPC gibbons;
    float bed_clock_rate;
    Vector3 door_positions[3];
    float agency_removal_timer;
    int lobby_visit_count;

    // Debug / display
    bool show_debug;
    bool wireframe;
    int current_style;
    float scene_exposure;
    bool nudge_mode;
    int nudge_selected;

    // Vignette text spring
    const char *vig_text;
    float text_y_offset;
    float text_y_velocity;
    float text_scale;
    float text_scale_vel;
    float text_scale_target;

    // Rain (taxi)
    Raindrop rain[MAX_RAIN];

    // Interaction rituals
    float interaction_timers[4];
    int interaction_phases[4];
    float ambient_fade;
    float interact_freeze;
    float interact_lean;
    float interact_lean_vel;

    // Menu / settings
    MenuMode menu_mode;
    int menu_cursor;
    float menu_item_x[MENU_MAX_ITEMS];
    float menu_item_vx[MENU_MAX_ITEMS];
    float menu_overlay_a;
    float menu_overlay_va;
    float setting_master_vol;
    bool setting_fullscreen;
} GameCtx;

// Initialize non-zero defaults
static inline void game_ctx_init(GameCtx *g) {
    g->state = STATE_TITLE;
    g->next_state = STATE_TITLE;
    g->fade_alpha = 1.0f;
    g->fade_speed = 2.0f;
    g->transition_hold = 0.3f;
    g->phone_wall_idx = -1;
    g->bed_clock_rate = 1.0f;
    g->ambient_fade = 1.0f;
    g->setting_master_vol = 1.0f;
    g->text_y_offset = 20.0f;
    g->nudge_selected = -1;
}

#endif // GAME_CTX_H
