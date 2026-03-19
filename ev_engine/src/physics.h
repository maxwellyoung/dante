// physics.h — Object physics, grab/carry/throw, breakables, doors
// HL2-style world interaction. Everything responds.
#ifndef EV_PHYSICS_H
#define EV_PHYSICS_H

#include "ev_types.h"
#include <stdbool.h>

typedef enum {
    GRAB_NONE,
    GRAB_CARRYING,
    GRAB_THROWING,
} GrabState;

typedef struct {
    GrabState state;
    int wall_index;         // index into scene.walls[]
    float grab_dist;        // distance at which object was grabbed
    float throw_cooldown;   // cooldown after throw
    Vector3 carry_offset;   // offset from camera target position
} GrabSystem;

void physics_init(GrabSystem *grab);
void physics_update(Scene *scene, Player *player, GrabSystem *grab, float dt);
void physics_grab_input(Scene *scene, Player *player, GrabSystem *grab, float dt);

#endif
