// ev_types.h — Shared types for EV engine
#ifndef EV_TYPES_H
#define EV_TYPES_H

#include "raylib.h"

#define RENDER_W 480
#define RENDER_H 300
#define MOVE_SPEED 4.0f
#define SPRINT_SPEED 7.0f
#define MOUSE_SENS 0.003f
#define MAX_OBJECTS 64
#define MAX_WALLS 512

typedef enum {
    STATE_TITLE,
    STATE_CAR,
    STATE_DRIVING,
    STATE_HOTEL_EXT,
    STATE_LOBBY,
    STATE_HALLWAY,
    STATE_ROOM,
    STATE_BED,
    STATE_STARS,
} GameState;

typedef struct {
    Vector3 pos;
    Vector3 size;
    Color color;
    bool active;
} Wall;

typedef struct {
    Vector3 pos;
    Color color;
    const char *name;
    const char *prompt;
    const char *done_text;
    bool done;
    bool active;
    float radius;
} InteractObject;

typedef struct {
    Camera3D camera;
    Vector3 velocity;
    bool sprinting;
    float sprint_stamina;
    float bob_timer;
    bool moving;
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
} Scene;

#endif
