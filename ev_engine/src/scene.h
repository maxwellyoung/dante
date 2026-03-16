// scene.h — Scene construction
#ifndef EV_SCENE_H
#define EV_SCENE_H

#include "ev_types.h"

void add_wall(Scene *s, float x, float y, float z, float w, float h, float d, Color c);
void add_object(Scene *s, float x, float y, float z, const char *name,
                const char *prompt, const char *done_text, Color c);
void build_lobby(Scene *s);
void build_hallway(Scene *s);
void build_hotel_room(Scene *s);

#endif
