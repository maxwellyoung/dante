// scene.h — Scene construction
#ifndef EV_SCENE_H
#define EV_SCENE_H

#include "ev_types.h"

void add_wall(Scene *s, float x, float y, float z, float w, float h, float d, Color c);
void add_cylinder(Scene *s, float x, float y, float z, float diameter, float height, Color c);
void add_light_panel(Scene *s, float x, float y, float z, float w, float h, float d, Color c);
void add_object(Scene *s, float x, float y, float z, const char *name, Color c, int max_steps);
void build_lobby(Scene *s);
void build_hallway(Scene *s);
void build_hotel_room(Scene *s);
void build_hotel_exterior(Scene *s);
void build_balcony(Scene *s);
void build_bathroom(Scene *s);
void build_stairwell(Scene *s);
void build_roof(Scene *s);
void build_elevator(Scene *s);
void build_taxi_ride(Scene *s);

#endif
