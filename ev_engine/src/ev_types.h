// ev_types.h — Shared types for EV engine
#ifndef EV_TYPES_H
#define EV_TYPES_H
#include "raylib.h"
#include <stdbool.h>
#define RENDER_W 960
#define RENDER_H 600
#define MOUSE_SENS_DEFAULT 0.003f
extern float ev_mouse_sens;
#define MAX_OBJECTS 64
#define MAX_WALLS 2048
typedef struct { float walk_speed, sprint_speed, slide_speed_mult, ground_accel, ground_friction, slide_friction, air_accel, air_speed_cap, air_friction, gravity, jump_impulse, coyote_time, jump_buffer_time, player_radius, step_height, step_up_speed, eye_height, slide_eye_height, fov_walk, fov_sprint, fov_slide, fov_lerp_speed, tilt_walk, tilt_sprint, tilt_slide, tilt_lerp_speed, bob_walk_rate, bob_sprint_rate, bob_slide_rate, bob_walk_amp, bob_sprint_amp, bob_slide_amp, land_dip_strength, land_dip_decay, land_dip_max_vy, stamina_drain, stamina_regen, stamina_min_to_sprint, max_walkable_angle, slope_slide_accel, bhop_friction_window, bhop_speed_cap, wall_run_duration, wall_run_speed, wall_run_gravity, wall_run_jump_out, wall_run_jump_up, mantle_reach, mantle_speed, mantle_forward, crouch_jump_bonus, speed_fov_scale, speed_fov_max, speed_shake_threshold, speed_shake_intensity, dash_speed, dash_duration, dash_cooldown; } PhysicsConfig;
static inline PhysicsConfig physics_default(void) { return (PhysicsConfig){.walk_speed=4.5f,.sprint_speed=8.5f,.slide_speed_mult=1.5f,.ground_accel=10.0f,.ground_friction=6.0f,.slide_friction=1.5f,.air_accel=12.0f,.air_speed_cap=1.0f,.air_friction=0.2f,.gravity=18.0f,.jump_impulse=5.5f,.coyote_time=0.12f,.jump_buffer_time=0.1f,.player_radius=0.45f,.step_height=0.5f,.step_up_speed=12.0f,.eye_height=1.6f,.slide_eye_height=1.2f,.fov_walk=70.0f,.fov_sprint=82.0f,.fov_slide=88.0f,.fov_lerp_speed=8.0f,.tilt_walk=1.5f,.tilt_sprint=2.5f,.tilt_slide=3.5f,.tilt_lerp_speed=10.0f,.bob_walk_rate=9.0f,.bob_sprint_rate=14.0f,.bob_slide_rate=18.0f,.bob_walk_amp=0.04f,.bob_sprint_amp=0.08f,.bob_slide_amp=0.10f,.land_dip_strength=0.15f,.land_dip_decay=5.0f,.land_dip_max_vy=8.0f,.stamina_drain=0.25f,.stamina_regen=0.15f,.stamina_min_to_sprint=0.05f,.max_walkable_angle=46.0f,.slope_slide_accel=8.0f,.bhop_friction_window=0.05f,.bhop_speed_cap=16.0f,.wall_run_duration=0.8f,.wall_run_speed=3.5f,.wall_run_gravity=3.0f,.wall_run_jump_out=5.0f,.wall_run_jump_up=5.0f,.mantle_reach=1.0f,.mantle_speed=6.0f,.mantle_forward=2.0f,.crouch_jump_bonus=1.5f,.speed_fov_scale=1.5f,.speed_fov_max=100.0f,.speed_shake_threshold=12.0f,.speed_shake_intensity=0.02f,.dash_speed=18.0f,.dash_duration=0.15f,.dash_cooldown=0.6f}; }
#define WALK_SPEED 4.5f
#define SPRINT_SPEED 8.5f
typedef enum { STATE_SPLASH, STATE_TITLE, STATE_CAR, STATE_DRIVING, STATE_HOTEL_EXT, STATE_LOBBY, STATE_ELEVATOR, STATE_HALLWAY, STATE_ROOM, STATE_BATHROOM, STATE_BALCONY, STATE_BED, STATE_STARS, STATE_HYPERSPACE, STATE_SPACE_LOBBY, STATE_SPACE_HOTEL, STATE_SPACE_CORRIDOR, STATE_SPACE_SUITE, STATE_PARIS_DREAM, STATE_RETURN_TAXI } GameState;
typedef enum { SURFACE_MARBLE, SURFACE_CARPET, SURFACE_WOOD } SurfaceType;
typedef enum { SHAPE_CUBE, SHAPE_CYLINDER, SHAPE_SPHERE, SHAPE_CONE, SHAPE_SKYTOWER, SHAPE_TORUS } ShapeType;
typedef enum { MAT_CONCRETE, MAT_MARBLE, MAT_WOOD, MAT_CARPET, MAT_WALLPAPER, MAT_TILE, MAT_BRASS, MAT_GLASS, MAT_LEATHER, MAT_FABRIC, MAT_CHECKERBOARD, MAT_HERRINGBONE, MAT_PARQUET, MAT_VELVET, MAT_EMISSIVE } MaterialType;
typedef struct { Vector3 pos, size; Color color; bool active; ShapeType shape; MaterialType material; float rotation_y; bool is_decal, no_collide; } Wall;
typedef struct { Vector3 pos; Color color; const char *name; bool done, active; float radius, reward_timer; int step, max_steps; } InteractObject;
typedef struct { Camera3D camera; Vector3 vel; float vy; bool moving, sprinting; float sprint_stamina, speed_current, fov_current, tilt_current, bob_timer, kick_pitch, kick_yaw, kick_decay; bool grounded; float land_timer, ground_y, ground_normal_y, coyote_timer, jump_buffer; bool sliding; float slide_speed; Vector3 slide_dir; bool wall_running; float wall_run_timer; Vector3 wall_normal; float wall_run_side; bool mantling; float mantle_target_y, mantle_timer, bhop_timer; bool dashing; float dash_timer, dash_cooldown_timer; Vector3 dash_dir; float peak_speed, control_mult, gravity_mult, idle_time; bool noclip; Vector3 velocity; } Player;
typedef struct { Wall walls[MAX_WALLS]; int wall_count; InteractObject objects[MAX_OBJECTS]; int object_count; Vector3 spawn, exit_pos; bool has_exit; Color floor_color, ceiling_color, fog_color; float fog_density; SurfaceType surface; int static_wall_count, driver_wall_start, driver_wall_end; } Scene;
typedef enum { NPC_WALKING, NPC_WAITING, NPC_READING, NPC_SITTING, NPC_GAZING, NPC_GESTURING } NPCBehavior;
#define MAX_NPC_WAYPOINTS 32
typedef struct { Vector3 pos, target_pos; float yaw, speed, bob_timer; bool active, waiting; float wait_radius; Vector3 waypoints[MAX_NPC_WAYPOINTS]; int waypoint_count, current_waypoint; bool use_physics; float vy, ground_y; bool grounded; float collision_radius, idle_timer, yaw_target; NPCBehavior behavior; Color body_color, head_color, cap_color, leg_color, tie_color, briefcase_color; const char **lines; int line_count, current_line; float line_timer, line_duration; bool line_showing; } NPC;
#define MAX_MONTAGE_BEATS 32
typedef struct { GameState scene; float duration; Vector3 cam_pos, cam_target; float exposure, warmth, saturation, grain, contrast; const char *text; } MontageBeat;
typedef struct { MontageBeat beats[MAX_MONTAGE_BEATS]; int beat_count, current_beat; float beat_timer; bool active, flash_between; } Montage;
typedef enum { SPLIT_NONE, SPLIT_VERTICAL, SPLIT_HORIZONTAL, SPLIT_PIP } SplitMode;
typedef struct { SplitMode split_mode; float split_pos, split_pos_target, split_pos_vel; GameState split_scene; Camera3D split_camera; float split_blend, split_blend_target, split_blend_vel, iris_radius, iris_target, iris_vel, iris_center_x, iris_center_y; bool iris_active, frozen; float freeze_timer, freeze_duration; bool intertitle_active; const char *intertitle_text; float intertitle_timer, intertitle_duration, intertitle_alpha, intertitle_alpha_vel; Color intertitle_color; bool intertitle_fullscreen; float aspect_letterbox, aspect_target, aspect_vel; bool jump_cut_active; float jump_cut_timer, jump_cut_interval; int jump_cut_count; float jump_cut_time_skip; } CinematicFX;
#endif
