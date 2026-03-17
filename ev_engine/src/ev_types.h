// ev_types.h — Shared types for EV engine
#ifndef EV_TYPES_H
#define EV_TYPES_H

#include "raylib.h"
#include <stdbool.h>

#define RENDER_W 480
#define RENDER_H 300
#define WALK_SPEED 4.5f
#define SPRINT_SPEED 8.5f
#define MOUSE_SENS 0.003f
#define MAX_OBJECTS 64
#define MAX_WALLS 1024

typedef enum {
    STATE_TITLE,
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
    STATE_SPACE_LOBBY,
    STATE_SPACE_CORRIDOR,
    STATE_SPACE_SUITE,
} GameState;

typedef enum {
    SURFACE_MARBLE,
    SURFACE_CARPET,
    SURFACE_WOOD,
} SurfaceType;

typedef enum { SHAPE_CUBE, SHAPE_CYLINDER, SHAPE_SPHERE, SHAPE_CONE, SHAPE_SKYTOWER } ShapeType;

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
} MaterialType;

typedef struct {
    Vector3 pos;
    Vector3 size;
    Color color;
    bool active;
    ShapeType shape;
    MaterialType material;  // default 0 = MAT_CONCRETE (C zero-init)
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
    Vector3 velocity;
    float bob_timer;
    bool moving;
    bool sprinting;
    float sprint_stamina;
    float speed_current;
    float fov_current;
    float tilt_current;
    float kick_pitch;
    float kick_yaw;
    float kick_decay;
    // Jump
    float vy;           // vertical velocity
    bool grounded;
    float land_timer;   // camera dip on landing
    float ground_y;     // current ground height (for stairs/platforms)
    // Slide
    bool sliding;
    float slide_speed;
    // Debug
    bool noclip;
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

#endif
