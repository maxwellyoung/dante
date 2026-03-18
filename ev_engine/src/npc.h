// npc.h — Gibbons: geometric cube-person NPC
// A Bolaño character — too aware, too polite, slightly broken.
// Carries a briefcase he never opens. Adjusts a tie no one asked about.
#ifndef EV_NPC_H
#define EV_NPC_H

#include "ev_types.h"
#include "lighting.h"

void init_npc(NPC *npc, Vector3 start, Vector3 *waypoints, int count,
              float speed, float wait_radius);
void init_npc_grounded(NPC *npc, Vector3 start, Vector3 *waypoints, int count,
                       float speed, float wait_radius);

// Set dialogue lines — spoken at waypoint arrivals
void npc_set_dialogue(NPC *npc, const char **lines, int count, float duration);

// Update — pass scene for physics collision, NULL for ghost mode
void update_npc(NPC *npc, Vector3 player_pos, Scene *scene, float dt);

// Returns current line to display (or NULL if silent)
const char *npc_current_dialogue(NPC *npc);

void draw_npc(NPC *npc, Model *cube_model, Model *cyl_model, EVLighting *lighting);

// Interest points — Gibbons notices sounds in the room
// A brief yaw adjustment. Professional awareness. Not investigation.
void npc_post_interest(NPC *npc, Vector3 pos, float lifetime);

#endif
