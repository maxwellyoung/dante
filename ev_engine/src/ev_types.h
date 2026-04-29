// ev_types.h — Shared types for EV engine
#ifndef EV_TYPES_H
#define EV_TYPES_H

#include "raylib.h"
#include <stdbool.h>

#define RENDER_W 1920
#define RENDER_H 1200
#define UI_SCALE 2        // HUD/text scale factor (RENDER_W / 960)
#define MOUSE_SENS_DEFAULT 0.003f
extern float ev_mouse_sens;
#define MAX_OBJECTS 64
#define MAX_WALLS 2048

// ── Physics config ──────────────────────────────────────────────────
// All movement tuning lives here. Tweak freely.
typedef struct {
    // Movement speeds
    float walk_speed;           // 4.5
    float sprint_speed;         // 8.5
    float slide_speed_mult;     // 1.5x current speed

    // Ground acceleration (Quake/Source style)
    float ground_accel;         // how fast you reach target speed
    float ground_friction;      // how fast you stop
    float slide_friction;       // reduced friction while sliding

    // Air movement
    float air_accel;            // air acceleration (allows air strafing)
    float air_speed_cap;        // max speed gain per frame in air (Quake trick)
    float air_friction;         // minimal air drag

    // Gravity & jumping
    float gravity;              // downward acceleration
    float terminal_velocity;    // max downward speed (prevents floor tunneling)
    float jump_impulse;         // upward velocity on jump
    float coyote_time;          // seconds after leaving edge where jump still works
    float jump_buffer_time;     // seconds before landing where jump press is remembered

    // Collision
    float player_radius;        // horizontal collision half-width
    float step_height;          // max height player can step over
    float step_up_speed;        // how fast to interpolate up steps

    // Camera
    float eye_height;           // normal standing eye height
    float slide_eye_height;     // eye height while sliding
    float crouch_eye_height;    // eye height while crouching (standing still)
    float fov_walk;
    float fov_sprint;
    float fov_slide;
    float fov_lerp_speed;
    float tilt_walk;            // degrees of strafe lean
    float tilt_sprint;
    float tilt_slide;
    float tilt_lerp_speed;

    // Head bob
    float bob_walk_rate;
    float bob_sprint_rate;
    float bob_slide_rate;
    float bob_walk_amp;
    float bob_sprint_amp;
    float bob_slide_amp;

    // Landing
    float land_dip_strength;    // camera dip multiplier
    float land_dip_decay;       // how fast dip recovers
    float land_dip_max_vy;      // vy divisor for dip amount

    // Sprint stamina
    float stamina_drain;
    float stamina_regen;
    float stamina_min_to_sprint;

    // Slope
    float max_walkable_angle;   // degrees — steeper = slide
    float slope_slide_accel;    // how fast you slide down steep slopes

    // Bunny hop
    float bhop_friction_window; // seconds after landing where friction is skipped (if jumping)
    float bhop_speed_cap;       // max speed from bunny hopping (prevents infinite accel)

    // Wall running
    float wall_run_duration;    // max seconds on a wall
    float wall_run_speed;       // min speed to start wall running
    float wall_run_gravity;     // reduced gravity while wall running
    float wall_run_jump_out;    // horizontal impulse when jumping off wall
    float wall_run_jump_up;     // vertical impulse when jumping off wall

    // Ledge mantle
    float mantle_reach;         // how far above eye height you can grab a ledge
    float mantle_speed;         // how fast you pull up (units/sec)
    float mantle_forward;       // forward push after mantle completes

    // Crouch jump
    float crouch_jump_bonus;    // extra jump impulse when jumping from slide

    // Speed feedback
    float speed_fov_scale;      // extra FOV degrees per unit speed above sprint
    float speed_fov_max;        // cap on speed-reactive FOV
    float speed_shake_threshold; // speed above which camera shakes
    float speed_shake_intensity; // shake amplitude at max speed

    // Wall climb (mid-air jump near wall → pull up to ledge)
    float climb_reach;          // how far above eye you can grab while climbing
    float climb_min_vy;         // minimum downward vy to allow climb (negative = falling)
    float climb_wall_dist;      // max distance from wall to trigger climb

    // Dash
    float dash_speed;           // impulse speed of dash
    float dash_duration;        // how long the dash lasts
    float dash_cooldown;        // cooldown between dashes
} PhysicsConfig;

// Default physics — the "feel" of the game
static inline PhysicsConfig physics_default(void) {
    return (PhysicsConfig){
        .walk_speed         = 4.5f,
        .sprint_speed       = 8.5f,
        .slide_speed_mult   = 1.5f,

        .ground_accel       = 10.0f,
        .ground_friction    = 6.0f,
        .slide_friction     = 1.5f,

        .air_accel          = 12.0f,
        .air_speed_cap      = 1.0f,     // Quake-style: low cap = big air strafe
        .air_friction       = 0.2f,

        .gravity            = 18.0f,
        .terminal_velocity  = -25.0f,  // max fall speed (prevents floor tunneling)
        .jump_impulse       = 5.5f,
        .coyote_time        = 0.12f,
        .jump_buffer_time   = 0.1f,

        .player_radius      = 0.45f,
        .step_height        = 0.5f,
        .step_up_speed      = 12.0f,

        .eye_height         = 1.6f,
        .slide_eye_height   = 1.2f,
        .crouch_eye_height  = 1.0f,
        .fov_walk           = 70.0f,
        .fov_sprint         = 82.0f,
        .fov_slide          = 88.0f,
        .fov_lerp_speed     = 8.0f,
        .tilt_walk          = 1.5f,
        .tilt_sprint        = 2.5f,
        .tilt_slide         = 3.5f,
        .tilt_lerp_speed    = 10.0f,

        .bob_walk_rate      = 9.0f,
        .bob_sprint_rate    = 14.0f,
        .bob_slide_rate     = 18.0f,
        .bob_walk_amp       = 0.04f,
        .bob_sprint_amp     = 0.08f,
        .bob_slide_amp      = 0.10f,

        .land_dip_strength  = 0.15f,
        .land_dip_decay     = 5.0f,
        .land_dip_max_vy    = 8.0f,

        .stamina_drain      = 0.25f,
        .stamina_regen      = 0.15f,
        .stamina_min_to_sprint = 0.05f,

        .max_walkable_angle = 46.0f,
        .slope_slide_accel  = 8.0f,

        .bhop_friction_window = 0.05f,  // 50ms grace period
        .bhop_speed_cap     = 16.0f,    // ~2x sprint speed max

        .wall_run_duration  = 0.8f,
        .wall_run_speed     = 3.5f,
        .wall_run_gravity   = 3.0f,     // ~1/6 normal gravity
        .wall_run_jump_out  = 5.0f,
        .wall_run_jump_up   = 5.0f,

        .mantle_reach       = 1.0f,     // can grab ledges 1m above eye
        .mantle_speed       = 6.0f,
        .mantle_forward     = 2.0f,

        .crouch_jump_bonus  = 1.5f,     // +1.5 to jump impulse from slide

        .speed_fov_scale    = 1.5f,     // 1.5° per unit above sprint
        .speed_fov_max      = 100.0f,
        .speed_shake_threshold = 12.0f,
        .speed_shake_intensity = 0.02f,

        .climb_reach    = 2.5f,     // can grab ledges 2.5m above eye (generous)
        .climb_min_vy   = -6.0f,   // can climb even when falling (not terminal)
        .climb_wall_dist = 0.8f,   // must be close to the wall

        .dash_speed     = 18.0f,
        .dash_duration  = 0.15f,
        .dash_cooldown  = 0.6f,
    };
}

// Legacy macros — scenes still reference these
#define WALK_SPEED  4.5f
#define SPRINT_SPEED 8.5f

// ─────────────────────────────────────────────────────────────────────

typedef enum {
    STATE_TITLE,
    STATE_PROTO_LAB,
    STATE_PROTO_MOVEMENT,
    STATE_PROTO_SHOOTER,
    STATE_PROTO_PUZZLE,
    STATE_CAR,
    STATE_DRIVING,
    STATE_HOTEL_EXT,
    STATE_LOBBY,
    STATE_ELEVATOR,
    STATE_HALLWAY,
    STATE_ROOM,
    STATE_BATHROOM,
    STATE_BALCONY,
    STATE_BED,
    STATE_STARS,
    STATE_HYPERSPACE,
    STATE_SPACE_LOBBY,
    STATE_SPACE_CORRIDOR,
    STATE_SPACE_SUITE,
    STATE_PARIS_DREAM,
    STATE_CLEANED_SUITE,
    STATE_MONTAGE,
    STATE_RETURN_TAXI,
    STATE_GLASSHOUSE,   // observation lounge — the first big space
    STATE_SHELL_TEST,   // dev-only: shell system validation
} GameState;

typedef enum {
    PROTOTYPE_NONE = 0,
    PROTOTYPE_MOVEMENT,
    PROTOTYPE_SHOOTER,
    PROTOTYPE_PUZZLE,
    PROTOTYPE_COUNT,
} PrototypeId;

typedef enum {
    PROTO_LAB_PLAY = 0,
    PROTO_LAB_REVIEW,
    PROTO_LAB_COMPARE,
    PROTO_LAB_RESET,
    PROTO_LAB_ACTION_COUNT,
} PrototypeLabAction;

typedef struct {
    bool completed;
    float completion_time;
    int resets;
    float distance;
    int jumps;
    int dashes;
    int shots_fired;
    int shots_hit;
    int shot_ricochets;
    int direct_hits;
    int bank_shots_attempted;
    int bank_shot_hits;
    int breach_uses;
    int breach_kills;
    int grapples_fired;
    int grapples_latched;
    int anchor_assisted_clears;
    int armored_kills;
    int exposure_hits;
    int recharge_pickups;
    int puzzle_actions;
    int puzzle_misreads;
    int route_nodes_triggered;
    int shortcut_uses;
    int recovery_uses;
    int finish_cleanliness;
    int relay_interactions;
    int invalid_states;
    int puzzle_stage_clears;
    float hintless_solve_time;
    float first_meaningful_action_time;
} PrototypeRunStats;

typedef struct {
    int would_replay;
    int readability;
    int mechanical_depth;
    int ship_confidence;
    int best_moment;
    int main_friction;
    bool submitted;
} PrototypeEval;

typedef struct {
    const char *display_name;
    const char *core_question;
    const char *allowed_verbs;
    const char *success_condition;
    const char *score_fields;
    int recommended_session_length;
    int max_resets;
    float max_completion_time;
    float min_distance;
    int min_jumps;
    int min_dashes;
    int min_shots_hit;
    int min_puzzle_actions;
    int max_puzzle_misreads;
} PrototypeQAExpectation;

typedef enum {
    SURFACE_MARBLE,
    SURFACE_CARPET,
    SURFACE_WOOD,
} SurfaceType;

typedef enum { SHAPE_CUBE, SHAPE_CYLINDER, SHAPE_SPHERE, SHAPE_CONE, SHAPE_SKYTOWER, SHAPE_TORUS, SHAPE_MODEL } ShapeType;

// ── Model asset registry ────────────────────────────────────────────
// Loaded 3D models from the engine-owned registry.
// Walls with SHAPE_MODEL use model_index to reference these.
typedef enum {
    MODEL_KIND_PROP,
    MODEL_KIND_SHELL,
} ModelKind;

typedef enum {
    MODEL_STATUS_DORMANT,
    MODEL_STATUS_ACTIVE,
} ModelStatus;

typedef struct {
    const char *name;              // filename without extension (e.g. "gibbons")
    const char *path;              // asset path relative to engine root
    ModelKind kind;
    bool startup_load;
    int estimated_vao_cost;
    bool preserve_blender_colors;
    ModelStatus status;
} ModelRegistryEntry;

#define MAX_MODEL_ASSETS 32
typedef struct {
    char name[64];              // filename without extension (e.g. "taxi", "gibbons")
    const char *path;           // registry-owned asset path
    ModelKind kind;
    bool startup_load;
    int estimated_vao_cost;
    bool preserve_blender_colors;
    ModelStatus status;
    Model model;
    bool loaded;
    // Animation (GLB only)
    ModelAnimation *anims;
    int anim_count;
    int current_anim;
    int current_frame;
} ModelAsset;

// Procedural material types — shader generates surface detail from world position
typedef enum {
    MAT_CONCRETE,   // 0 — raw brutalist, subtle pitting
    MAT_MARBLE,     // 1 — veined, polished
    MAT_WOOD,       // 2 — grain, warm
    MAT_CARPET,     // 3 — dense fuzzy noise
    MAT_WALLPAPER,  // 4 — damask repeat pattern
    MAT_TILE,       // 5 — grid with grout lines
    MAT_BRASS,      // 6 — metallic, high specular
    MAT_GLASS,      // 7 — slight transparency, reflective
    MAT_LEATHER,    // 8 — subtle grain
    MAT_FABRIC,     // 9 — soft weave
    MAT_CHECKERBOARD, // 10 — two-tone checkerboard from baseColor
    MAT_HERRINGBONE,  // 11 — interlocking plank pattern
    MAT_PARQUET,      // 12 — alternating wood grain direction
    MAT_VELVET,       // 13 — directional sheen, view-dependent nap
    MAT_WATER,        // 14 — dark reflective surface, animated ripple
    MAT_PUDDLE,       // 15 — thin film on ground, picks up nearby light color
    MAT_EMISSIVE,     // 16 — unlit, outputs baseColor directly (Earth, glow panels)
} MaterialType;

typedef struct {
    Vector3 pos;
    Vector3 size;
    Color color;
    bool active;
    ShapeType shape;
    MaterialType material;  // default 0 = MAT_CONCRETE (C zero-init)
    float rotation_y;
    float rotation_x;       // SHAPE_MODEL: X-axis pre-rotation (axis-correcting bad exports)
    bool is_decal;          // overlay geometry — polygon offset prevents z-fighting
    bool no_collide;        // decorative — skip in collision (cigarettes, floating objects, decals)
    int model_index;        // SHAPE_MODEL only: index into model_assets[]
    // Nudge physics — authored responses with spring-back
    bool pushable;
    float push_mass;        // resistance (0 = immovable, 0.3 = wine glass, 1.0 = book)
    Vector3 push_vel;       // current velocity
    Vector3 push_origin;    // home position (springs back)
    float push_damping;     // velocity decay rate
    // Extended physics — freed objects, breakables, doors
    bool freed;             // no spring-back (thrown/knocked loose)
    float push_vy;          // vertical velocity for arcs
    bool breakable;         // shatters on hard impact
    float health;           // 1.0=pristine, 0=shattered
    bool hinge;             // door: rotates around edge
    float hinge_angle;      // current rotation
    float hinge_target;     // target angle (open/closed)
    float hinge_vel;        // angular velocity
} Wall;

typedef struct {
    Vector3 pos;
    Color color;
    const char *name;
    bool done;
    bool active;
    float radius;
    float reward_timer;
    // Multi-step
    int step;
    int max_steps;
} InteractObject;

typedef struct {
    Camera3D camera;
    // Velocity — proper vector, not scalar
    Vector3 vel;            // horizontal velocity (x/z plane)
    float vy;               // vertical velocity (separate for clarity)
    // Movement state
    bool moving;
    bool sprinting;
    float sprint_stamina;
    float speed_current;    // computed magnitude of vel (for bob/fov)
    // Camera feel
    float fov_current;
    float tilt_current;
    float bob_timer;
    float kick_pitch;
    float kick_yaw;
    float kick_decay;
    // Ground
    bool grounded;
    float land_timer;       // camera dip on landing
    float ground_y;         // current ground height (for stairs/platforms)
    float ground_normal_y;  // slope: Y component of surface normal (1.0 = flat)
    float coyote_timer;     // time since last grounded (for coyote jump)
    float jump_buffer;      // time since last jump press (for buffered jump)
    // Crouch / Slide
    bool crouching;         // standing crouch (Ctrl while still/slow)
    bool sliding;
    float slide_speed;
    Vector3 slide_dir;      // direction when slide started (momentum)
    // Wall running
    bool wall_running;
    float wall_run_timer;   // time remaining on wall
    Vector3 wall_normal;    // normal of wall being run on
    float wall_run_side;    // -1 = left wall, +1 = right wall
    // Mantle
    bool mantling;
    float mantle_target_y;  // Y position we're pulling up to
    float mantle_timer;     // progress through mantle
    // Bunny hop
    float bhop_timer;       // time since landing (for friction skip)
    // Dash
    bool dashing;
    float dash_timer;       // time remaining in dash
    float dash_cooldown_timer;
    Vector3 dash_dir;       // locked direction during dash
    // Speed tracking
    float peak_speed;       // highest speed reached (for feedback scaling)
    // Agency dial — scales wish_speed and mouse sens (1.0 = full, 0.0 = frozen)
    float control_mult;
    // Gravity multiplier — 1.0 = Earth, 0.4 = orbital hotel, 0.0 = zero-g
    float gravity_mult;
    // Idle breathing — accumulates when nearly still
    float idle_time;
    // Debug
    bool noclip;
    // Deprecated — kept for compatibility
    Vector3 velocity;       // unused, see vel/vy
} Player;

typedef struct {
    Wall walls[MAX_WALLS];
    int wall_count;
    InteractObject objects[MAX_OBJECTS];
    int object_count;
    Vector3 spawn;
    Vector3 exit_pos;
    bool has_exit;
    Color floor_color;
    Color ceiling_color;
    Color fog_color;
    float fog_density;
    SurfaceType surface;
    int static_wall_count;  // walls below this index move with camera (taxi interior)
} Scene;

// NPC behavior states — what Gibbons is doing right now
typedef enum {
    NPC_WALKING,
    NPC_WAITING,
    NPC_READING,
    NPC_SITTING,
    NPC_GAZING,     // looking at something (window, Earth) — not the player
    NPC_GESTURING,  // arm extended, inviting player through a door
} NPCBehavior;

// NPC — geometric cube-person (Gravity Bone / Thirty Flights style)
#define MAX_NPC_WAYPOINTS 32
typedef struct {
    Vector3 pos;
    Vector3 target_pos;     // current waypoint target
    float yaw;              // facing direction (radians)
    float speed;
    float bob_timer;        // walk cycle
    bool active;
    bool waiting;           // paused at waypoint, waiting for player
    float wait_radius;      // how close player must be to trigger advance
    // Waypoint path
    Vector3 waypoints[MAX_NPC_WAYPOINTS];
    int waypoint_count;
    int current_waypoint;
    // Physics (toggle: ghost vs grounded)
    bool use_physics;
    float vy;
    float ground_y;
    bool grounded;
    float collision_radius;
    float idle_timer;       // time spent waiting (drives quirks)
    float yaw_target;       // smoothed facing
    NPCBehavior behavior;   // current behavior state
    // Appearance
    Color body_color;
    Color head_color;
    Color cap_color;
    Color leg_color;
    Color tie_color;
    Color briefcase_color;
    // Dialogue — lines spoken at waypoints
    const char **lines;     // array of string pointers (NULL-terminated or count-bounded)
    int line_count;
    int current_line;
    float line_timer;       // time current line has been showing
    float line_duration;    // seconds per line (default 3.0)
    bool line_showing;
} NPC;

#endif
