// player.h — Player movement and collision
#ifndef EV_PLAYER_H
#define EV_PLAYER_H

#include "ev_types.h"

void init_player(Player *p, Vector3 pos);
void update_player(Player *p, Scene *scene, float dt);

#endif
