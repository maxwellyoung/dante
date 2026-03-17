// scene.h — Scene construction
#ifndef EV_SCENE_H
#define EV_SCENE_H

#include "ev_types.h"

void add_wall(Scene *s, float x, float y, float z, float w, float h, float d, Color c);
void add_cylinder(Scene *s, float x, float y, float z, float diameter, float height, Color c);
void add_sphere(Scene *s, float x, float y, float z, float diameter, Color c);
void add_cone(Scene *s, float x, float y, float z, float diameter, float height, Color c);
void add_light_panel(Scene *s, float x, float y, float z, float w, float h, float d, Color c);
void add_skytower(Scene *s, float x, float y, float z, float scale, Color c);
void add_object(Scene *s, float x, float y, float z, const char *name, Color c, int max_steps);

// Material assignment (set on most recently added wall)
void set_last_material(Scene *s, MaterialType mat);

// Architectural detail helpers
void add_door_frame(Scene *s, float x, float y, float z, float w, float h, float depth, Color frame_color);
void add_window(Scene *s, float x, float y, float z, float w, float h, Color frame_color, Color glass_color);
void add_baseboard(Scene *s, float x, float y, float z, float length, float depth_axis, Color c);
void add_crown_molding(Scene *s, float x, float y, float z, float length, float along_z, Color c);
void add_picture_frame(Scene *s, float x, float y, float z, float w, float h, Color frame_color, Color canvas_color);
void add_bookshelf(Scene *s, float x, float y, float z, float w, float h, int rows, Color shelf_color);
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
void build_space_lobby(Scene *s);
void build_space_corridor(Scene *s);
void build_space_suite(Scene *s);

#endif
