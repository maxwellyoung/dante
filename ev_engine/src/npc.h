// npc.h — Gibbons: geometric cube-person NPC (Gravity Bone style)
#ifndef EV_NPC_H
#define EV_NPC_H

#include "ev_types.h"
#include "lighting.h"

void init_npc(NPC *npc, Vector3 start, Vector3 *waypoints, int count,
              float speed, float wait_radius);
void update_npc(NPC *npc, Vector3 player_pos, float dt);
void draw_npc(NPC *npc, Model *cube_model, Model *cyl_model, EVLighting *lighting);

#endif
