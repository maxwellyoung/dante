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
void add_model(Scene *s, float x, float y, float z, float sx, float sy, float sz,
               float rotation_deg, int model_index, MaterialType mat, Color c);
int find_model_asset(const char *name);
void add_object(Scene *s, float x, float y, float z, const char *name, Color c, int max_steps);

// Material assignment (set on most recently added wall)
void set_last_material(Scene *s, MaterialType mat);

// Rotation assignment (set on most recently added wall)
void set_last_rotation(Scene *s, float degrees);

// Mark as decal — polygon offset prevents z-fighting on overlay geometry
void set_last_decal(Scene *s);

// Nudge physics — mark most recent wall as pushable
void set_last_pushable(Scene *s, float mass, float damping);

// Breakable — shatters on hard impact
void set_last_breakable(Scene *s, float health);

// Hinge door — rotates around edge
void set_last_hinge(Scene *s, float closed_angle, float open_angle);

// Convenience: add_wall + auto-mark as decal (for floor overlays, light shafts, puddles)
void add_wall_decal(Scene *s, float x, float y, float z, float w, float h, float d, Color c);

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

// ── Shell system — Gehry-esque environments ──
// Visual: GLB mesh (organic curves, deconstructivist forms)
// Collision: invisible AABB walls defining walkable space
void add_collision_wall(Scene *s, float x, float y, float z, float w, float h, float d);
void add_shell(Scene *s, const char *model_name, float x, float y, float z,
               float sx, float sy, float sz, float rotation_deg, MaterialType mat, Color c);
void add_collision_floor(Scene *s, float x, float y, float z, float w, float d);
void add_collision_ceiling(Scene *s, float x, float y, float z, float w, float d);

void build_lobby(Scene *s);
void build_hallway(Scene *s);
void build_hotel_room(Scene *s);
void build_hotel_exterior(Scene *s);
void build_balcony(Scene *s);
void build_bathroom(Scene *s);
void build_elevator(Scene *s);
void build_elevator_space(Scene *s);
void build_taxi_ride(Scene *s);
void build_hyperspace(Scene *s);
void build_space_lobby(Scene *s);
void build_space_corridor(Scene *s);
void build_space_suite(Scene *s);
void build_return_taxi(Scene *s);
void build_space_suite_cleaned(Scene *s);
void build_glasshouse(Scene *s);

#endif
