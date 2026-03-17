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

// Rotation assignment (set on most recently added wall)
void set_last_rotation(Scene *s, float degrees);

// Mark as decal — polygon offset prevents z-fighting on overlay geometry
void set_last_decal(Scene *s);

// Composition helpers — rich furniture from simple primitives
void add_dining_table(Scene *s, float x, float y, float z, float w, float d, float angle, Color wood);
void add_chair(Scene *s, float x, float y, float z, float angle, Color wood, Color seat);
void add_chandelier(Scene *s, float x, float y, float z, int arms, float radius, Color metal, Color light);
void add_column_row(Scene *s, float x_start, float z, float spacing, int count, float radius, float height, Color c);
void add_wainscoting(Scene *s, float x, float y, float z, float length, float height, bool along_z, Color panel, Color trim);
void add_fireplace(Scene *s, float x, float y, float z, Color stone, Color glow);
void add_bar_counter(Scene *s, float x, float y, float z, float length, bool along_z, Color counter, Color front);
void add_rug(Scene *s, float x, float y, float z, float w, float d, Color primary, Color border);
void add_desk(Scene *s, float x, float y, float z, float angle, Color wood);
void add_sofa(Scene *s, float x, float y, float z, float angle, Color fabric);

// Architectural detail helpers
void add_door_frame(Scene *s, float x, float y, float z, float w, float h, float depth, Color frame_color);
void add_window(Scene *s, float x, float y, float z, float w, float h, Color frame_color, Color glass_color);
void add_baseboard(Scene *s, float x, float y, float z, float length, float depth_axis, Color c);
void add_crown_molding(Scene *s, float x, float y, float z, float length, float along_z, Color c);
void add_picture_frame(Scene *s, float x, float y, float z, float w, float h, Color frame_color, Color canvas_color);
void add_bookshelf(Scene *s, float x, float y, float z, float w, float h, int rows, Color shelf_color);
// P5: Enhanced geometry helpers
void add_arch_doorframe(Scene *s, float x, float y, float z, float w, float h,
                        float depth, Color frame_color);
void add_crown_molding_detailed(Scene *s, float x, float y, float z,
                                float length, bool along_z, Color color);
void add_corridor_door(Scene *s, float x, float y, float z,
                       float side, Color door_color, Color frame_color);

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
void build_hyperspace(Scene *s);
void build_space_lobby(Scene *s);
void build_space_corridor(Scene *s);
void build_space_suite(Scene *s);

#endif
