// scene.c — Scene construction
// Material palette. Clean volumes. Dramatic light.
#include "scene.h"
#include "config.h"
#include "scale.h"
#include "palette.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

void add_wall(Scene *s, float x, float y, float z, float w, float h, float d, Color c) {
    if (s->wall_count >= MAX_WALLS) {
        fprintf(stderr, "[EV] WARNING: wall overflow (%d/%d) — wall at (%.1f,%.1f,%.1f) dropped\n",
                s->wall_count, MAX_WALLS, x, y, z);
        return;
    }
    s->walls[s->wall_count++] = (Wall){
        .pos = {x, y, z}, .size = {w, h, d}, .color = c, .active = true,
        .shape = SHAPE_CUBE,
    };
}

void add_cylinder(Scene *s, float x, float y, float z, float diameter, float height, Color c) {
    if (s->wall_count >= MAX_WALLS) {
        fprintf(stderr, "[EV] WARNING: wall overflow (%d/%d) — cylinder at (%.1f,%.1f,%.1f) dropped\n",
                s->wall_count, MAX_WALLS, x, y, z);
        return;
    }
    s->walls[s->wall_count++] = (Wall){
        .pos = {x, y, z}, .size = {diameter, height, diameter}, .color = c,
        .active = true, .shape = SHAPE_CYLINDER,
    };
}

void add_sphere(Scene *s, float x, float y, float z, float diameter, Color c) {
    if (s->wall_count >= MAX_WALLS) {
        fprintf(stderr, "[EV] WARNING: wall overflow (%d/%d) — sphere at (%.1f,%.1f,%.1f) dropped\n",
                s->wall_count, MAX_WALLS, x, y, z);
        return;
    }
    s->walls[s->wall_count++] = (Wall){
        .pos = {x, y, z}, .size = {diameter, diameter, diameter}, .color = c,
        .active = true, .shape = SHAPE_SPHERE,
    };
}

void add_cone(Scene *s, float x, float y, float z, float diameter, float height, Color c) {
    if (s->wall_count >= MAX_WALLS) {
        fprintf(stderr, "[EV] WARNING: wall overflow (%d/%d) — cone at (%.1f,%.1f,%.1f) dropped\n",
                s->wall_count, MAX_WALLS, x, y, z);
        return;
    }
    s->walls[s->wall_count++] = (Wall){
        .pos = {x, y, z}, .size = {diameter, height, diameter}, .color = c,
        .active = true, .shape = SHAPE_CONE,
    };
}

void add_object(Scene *s, float x, float y, float z, const char *name, Color c, int max_steps) {
    if (s->object_count >= MAX_OBJECTS) {
        fprintf(stderr, "[EV] WARNING: object overflow (%d/%d) — '%s' at (%.1f,%.1f,%.1f) dropped\n",
                s->object_count, MAX_OBJECTS, name, x, y, z);
        return;
    }
    s->objects[s->object_count++] = (InteractObject){
        .pos = {x, y, z}, .color = c, .name = name,
        .done = false, .active = true, .radius = 2.0f,
        .reward_timer = 0, .step = 0, .max_steps = max_steps,
    };
}

static void add_column(Scene *s, float x, float z, float r, float h, Color c) {
    add_cylinder(s, x, h/2, z, r*2, h, c);
}

void add_skytower(Scene *s, float x, float y, float z, float scale, Color c) {
    if (s->wall_count >= MAX_WALLS) {
        fprintf(stderr, "[EV] WARNING: wall overflow (%d/%d) — skytower at (%.1f,%.1f,%.1f) dropped\n",
                s->wall_count, MAX_WALLS, x, y, z);
        return;
    }
    s->walls[s->wall_count++] = (Wall){
        .pos = {x, y, z}, .size = {scale, scale, scale}, .color = c,
        .active = true, .shape = SHAPE_SKYTOWER,
    };
}

// Set material on the most recently added wall — chainable, non-breaking
void set_last_material(Scene *s, MaterialType mat) {
    if (s->wall_count > 0) s->walls[s->wall_count - 1].material = mat;
}

void set_last_rotation(Scene *s, float degrees) {
    if (s->wall_count > 0) s->walls[s->wall_count - 1].rotation_y = degrees;
}

// Mark most recent wall as decal — rendered with polygon offset to prevent z-fighting
void set_last_decal(Scene *s) {
    if (s->wall_count > 0) s->walls[s->wall_count - 1].is_decal = true;
}

// ── P5: Enhanced geometry helpers ──

// Arched doorframe — rectangular frame + semicircle of boxes at top
void add_arch_doorframe(Scene *s, float x, float y, float z, float w, float h,
                        float depth, Color frame_color) {
    // Side posts
    add_wall(s, x - w/2, y + h/2, z, 0.08f, h, depth, frame_color);
    add_wall(s, x + w/2, y + h/2, z, 0.08f, h, depth, frame_color);
    // Top lintel
    add_wall(s, x, y + h, z, w + 0.16f, 0.08f, depth, frame_color);
    // Arch segments — 8 small boxes arranged in a semicircle above the lintel
    for (int i = 0; i < 8; i++) {
        float angle = PI * (float)i / 7.0f;
        float ax = x + cosf(angle) * w * 0.5f;
        float ay = y + h + sinf(angle) * w * 0.25f;
        add_wall(s, ax, ay, z, 0.1f, 0.06f, depth * 0.8f, frame_color);
    }
}

// Crown molding with stepped profile — more dimensional than flat strip
void add_crown_molding_detailed(Scene *s, float x, float y, float z,
                                float length, bool along_z, Color color) {
    float lx = along_z ? 0.04f : length;
    float lz = along_z ? length : 0.04f;
    // Base strip
    add_wall(s, x, y, z, lx, 0.06f, lz, color);
    // Angled strip (slight offset down and out)
    float ox = along_z ? 0.03f : 0;
    float oz = along_z ? 0 : 0.03f;
    add_wall(s, x + ox, y - 0.04f, z + oz, lx * 0.9f, 0.04f, lz * 0.9f, color);
    // Top lip
    add_wall(s, x - ox * 0.5f, y + 0.04f, z - oz * 0.5f,
             lx * 0.8f, 0.025f, lz * 0.8f, color);
}

// Corridor door — recessed frame, door panel, handle
void add_corridor_door(Scene *s, float x, float y, float z,
                       float side, Color door_color, Color frame_color) {
    float dx = side > 0 ? 0.12f : -0.12f;
    // Recessed frame (3 boxes)
    add_wall(s, x + dx, y + 1.3f, z, 0.12f, 2.6f, 0.05f, frame_color);  // top
    add_wall(s, x + dx, y + 1.3f, z - 0.55f, 0.12f, 2.6f, 0.05f, frame_color);
    add_wall(s, x + dx, y + 1.3f, z + 0.55f, 0.12f, 2.6f, 0.05f, frame_color);
    // Door panel
    add_wall(s, x + dx * 0.8f, y + 1.3f, z, 0.1f, 2.5f, 0.95f, door_color);
    // Handle — small sphere
    add_sphere(s, x + dx * 0.6f, y + 1.1f, z + 0.35f * (side > 0 ? -1 : 1),
               0.06f, frame_color);
    set_last_material(s, MAT_BRASS);
}

// Auto-assign materials by color matching after scene is built
static void tag_materials_by_color(Scene *s) {
    for (int i = 0; i < s->wall_count; i++) {
        Wall *w = &s->walls[i];
        if (w->material != MAT_CONCRETE) continue;  // already tagged
        unsigned char r = w->color.r, g = w->color.g, b = w->color.b;
        // Brass range: warm gold-brown, r>g>b, r>150
        if (r > 150 && g > 100 && g < 200 && b < g && r > g)
            w->material = MAT_BRASS;
        // Marble: light grey, all channels close, >170
        else if (r > 170 && g > 170 && b > 165 && abs(r-g) < 20 && abs(g-b) < 20)
            w->material = MAT_MARBLE;
        // Wood: brown, r>g>b, r<160
        else if (r > 60 && r < 160 && g > 40 && g < 120 && b < g && r > g)
            w->material = MAT_WOOD;
        // Glass/void: very dark, all < 20
        else if (r < 20 && g < 20 && b < 20)
            w->material = MAT_GLASS;
        // White/cream: all > 220
        else if (r > 220 && g > 220 && b > 210)
            w->material = MAT_WALLPAPER;
    }
}

// === Architectural detail helpers ===
// These compose primitives into hotel vocabulary.

void add_door_frame(Scene *s, float x, float y, float z,
                    float w, float h, float depth, Color frame_color) {
    float t = 0.08f;  // frame thickness
    // Top
    add_wall(s, x, y + h/2 - t/2, z, w + t*2, t, depth, frame_color);
    set_last_material(s, MAT_WOOD);
    // Left
    add_wall(s, x - w/2 - t/2, y, z, t, h, depth, frame_color);
    set_last_material(s, MAT_WOOD);
    // Right
    add_wall(s, x + w/2 + t/2, y, z, t, h, depth, frame_color);
    set_last_material(s, MAT_WOOD);
}

void add_window(Scene *s, float x, float y, float z,
                float w, float h, Color frame_color, Color glass_color) {
    float t = 0.06f;
    // Glass pane
    add_wall(s, x, y, z, w, h, 0.02f, glass_color);
    set_last_material(s, MAT_GLASS);
    // Frame — top, bottom, left, right
    add_wall(s, x, y + h/2 + t/2, z, w + t*2, t, 0.04f, frame_color);
    set_last_material(s, MAT_WOOD);
    add_wall(s, x, y - h/2 - t/2, z, w + t*2, t, 0.04f, frame_color);
    set_last_material(s, MAT_WOOD);
    add_wall(s, x - w/2 - t/2, y, z, t, h, 0.04f, frame_color);
    set_last_material(s, MAT_WOOD);
    add_wall(s, x + w/2 + t/2, y, z, t, h, 0.04f, frame_color);
    set_last_material(s, MAT_WOOD);
    // Sill — thin shelf below
    add_wall(s, x, y - h/2 - t, z + 0.04f, w + t*4, 0.03f, 0.08f, frame_color);
    set_last_material(s, MAT_WOOD);
}

void add_baseboard(Scene *s, float x, float y, float z,
                   float length, float depth_axis, Color c) {
    // depth_axis: 'x' length along X, 'z' length along Z
    // Thin strip at floor level
    float h = 0.08f;
    if (depth_axis > 0.01f) {
        // Along Z axis
        add_wall(s, x, y + h/2, z, 0.03f, h, length, c);
    } else {
        // Along X axis
        add_wall(s, x, y + h/2, z, length, h, 0.03f, c);
    }
    set_last_material(s, MAT_WOOD);
}

void add_crown_molding(Scene *s, float x, float y, float z,
                       float length, float along_z, Color c) {
    float h = 0.06f;
    if (along_z > 0.01f) {
        add_wall(s, x, y - h/2, z, 0.04f, h, length, c);
    } else {
        add_wall(s, x, y - h/2, z, length, h, 0.04f, c);
    }
    set_last_material(s, MAT_WOOD);
}

void add_picture_frame(Scene *s, float x, float y, float z,
                       float w, float h, Color frame_color, Color canvas_color) {
    float t = 0.05f;
    // Canvas
    add_wall(s, x, y, z, w, h, 0.01f, canvas_color);
    set_last_material(s, MAT_FABRIC);
    // Frame strips
    add_wall(s, x, y + h/2 + t/2, z, w + t*2, t, 0.03f, frame_color);
    set_last_material(s, MAT_WOOD);
    add_wall(s, x, y - h/2 - t/2, z, w + t*2, t, 0.03f, frame_color);
    set_last_material(s, MAT_WOOD);
    add_wall(s, x - w/2 - t/2, y, z, t, h, 0.03f, frame_color);
    set_last_material(s, MAT_WOOD);
    add_wall(s, x + w/2 + t/2, y, z, t, h, 0.03f, frame_color);
    set_last_material(s, MAT_WOOD);
}

void add_bookshelf(Scene *s, float x, float y, float z,
                   float w, float h, int rows, Color shelf_color) {
    float row_h = h / (float)rows;
    // Back panel
    add_wall(s, x, y, z, w, h, 0.02f, shelf_color);
    set_last_material(s, MAT_WOOD);
    // Shelves
    for (int i = 0; i <= rows; i++) {
        float sy = y - h/2 + row_h * i;
        add_wall(s, x, sy, z + 0.08f, w, 0.03f, 0.16f, shelf_color);
        set_last_material(s, MAT_WOOD);
    }
    // Books on each shelf (colored rectangles)
    for (int i = 0; i < rows; i++) {
        float sy = y - h/2 + row_h * i + row_h * 0.5f;
        int books = 3 + (i * 7) % 4;
        float bw = w / (float)(books + 1);
        for (int b = 0; b < books; b++) {
            float bx = x - w/2 + bw * (b + 0.5f) + bw * 0.25f;
            // Deterministic book colors
            unsigned char cr = (unsigned char)(80 + ((b * 73 + i * 37) % 120));
            unsigned char cg = (unsigned char)(40 + ((b * 51 + i * 89) % 80));
            unsigned char cb = (unsigned char)(30 + ((b * 97 + i * 23) % 100));
            add_wall(s, bx, sy, z + 0.1f, bw * 0.6f, row_h * 0.7f, 0.1f,
                     (Color){cr, cg, cb, 255});
            set_last_material(s, MAT_LEATHER);
        }
    }
}

void add_light_panel(Scene *s, float x, float y, float z,
                     float w, float h, float d, Color c) {
    add_wall(s, x, y, z, w, h, d, (Color){c.r, c.g, c.b, 160});
    add_wall(s, x, y, z + (d > 0.2f ? 0 : 0.05f), w + 0.2f, h + 0.2f, d + 0.08f,
             (Color){c.r, c.g, c.b, 50});
}

// Recessed panel — creates depth illusion with inset + frame
// Main wall already exists; this adds a darker inset and thin frame border
static void add_recessed_panel(Scene *s, float x, float y, float z,
                               float w, float h, float inset_depth,
                               Color wall_color) {
    // Darken the wall color for the recessed area
    unsigned char dr = (unsigned char)(wall_color.r * 0.82f);
    unsigned char dg = (unsigned char)(wall_color.g * 0.82f);
    unsigned char db = (unsigned char)(wall_color.b * 0.82f);
    Color dark = {dr, dg, db, 255};
    // Slightly lighter for the frame
    unsigned char fr = (unsigned char)fminf(255, wall_color.r * 0.9f);
    unsigned char fg = (unsigned char)fminf(255, wall_color.g * 0.88f);
    unsigned char fb = (unsigned char)fminf(255, wall_color.b * 0.85f);
    Color frame = {fr, fg, fb, 255};
    float frame_w = 0.06f;

    // Inset panel — slightly recessed
    add_wall(s, x, y, z - inset_depth, w, h, 0.02f, dark);
    // Frame — four thin strips around the panel
    add_wall(s, x, y + h/2 + frame_w/2, z - inset_depth*0.5f,
             w + frame_w*2, frame_w, 0.03f, frame);  // top
    add_wall(s, x, y - h/2 - frame_w/2, z - inset_depth*0.5f,
             w + frame_w*2, frame_w, 0.03f, frame);  // bottom
    add_wall(s, x - w/2 - frame_w/2, y, z - inset_depth*0.5f,
             frame_w, h, 0.03f, frame);              // left
    add_wall(s, x + w/2 + frame_w/2, y, z - inset_depth*0.5f,
             frame_w, h, 0.03f, frame);              // right
}

// Dropped ceiling section — coffered ceiling effect
static void add_dropped_ceiling(Scene *s, float x, float y, float z,
                                float w, float d, float drop,
                                Color ceil_color, Color light_color) {
    // Lowered ceiling panel
    unsigned char dr = (unsigned char)(ceil_color.r * 0.88f);
    unsigned char dg = (unsigned char)(ceil_color.g * 0.88f);
    unsigned char db = (unsigned char)(ceil_color.b * 0.88f);
    Color lowered = {dr, dg, db, 255};
    add_wall(s, x, y - drop, z, w, 0.1f, d, lowered);
    // Edge trim — four sides hanging down
    float trim = 0.08f;
    add_wall(s, x, y - drop/2, z + d/2, w, drop, trim, ceil_color);  // front
    add_wall(s, x, y - drop/2, z - d/2, w, drop, trim, ceil_color);  // back
    add_wall(s, x - w/2, y - drop/2, z, trim, drop, d, ceil_color);  // left
    add_wall(s, x + w/2, y - drop/2, z, trim, drop, d, ceil_color);  // right
    // Light panels underneath
    add_light_panel(s, x - w/4, y - drop - 0.02f, z,
                    w/3, 0.03f, d*0.6f, light_color);
    add_light_panel(s, x + w/4, y - drop - 0.02f, z,
                    w/3, 0.03f, d*0.6f, light_color);
}

// ── THE MOTIF: Red book (Commandment 9) ──
// "An object that appears in every scene. Seemingly decorative.
// Accumulating weight. The player should notice it on the second playthrough."
// A small red leather-bound book. Passport-sized. Always present, always peripheral.
// It was always there. You never picked it up.
static void add_red_book(Scene *s, float x, float y, float z, float rot_y) {
    Color book_red = {175, 35, 30, 255};
    Color pages    = {240, 235, 220, 255};
    // Cover
    add_wall(s, x, y, z, 0.12f, 0.025f, 0.16f, book_red);
    set_last_material(s, MAT_LEATHER);
    if (rot_y != 0) set_last_rotation(s, rot_y);
    // Pages edge — cream stripe
    add_wall(s, x, y, z + 0.07f, 0.10f, 0.018f, 0.01f, pages);
    if (rot_y != 0) set_last_rotation(s, rot_y);
    // No collision — decorative, don't block movement
    s->walls[s->wall_count - 1].no_collide = true;
    s->walls[s->wall_count - 2].no_collide = true;
}

// ============================================================
// SCENES
// ============================================================

void build_hotel_exterior(Scene *s) {
    *s = (Scene){0};
    // SKY TOWER BASE — Auckland CBD, 2 AM
    // The tower IS the building. You walk to its entrance.
    s->surface = SURFACE_MARBLE;

    Color gold = PAL_BRASS;
    Color window_lit = {240, 210, 130, 180};
    Color sidewalk = {120, 115, 110, 255};
    Color road = {50, 50, 55, 255};
    Color godard_red = PAL_RED;
    Color tower_base = {140, 135, 145, 255};   // concrete base structure
    Color tower_steel = {100, 105, 115, 255};   // tower shaft

    s->fog_color = PAL_FOG_NIGHT;
    s->fog_density = 0.002f;  // open air — don't choke the view

    // Sidewalk — stone tile, not poured concrete
    add_wall(s, 0, -0.1f, 0, 24, 0.2f, 10, sidewalk);
    set_last_material(s, MAT_TILE);
    // Road — dark asphalt
    add_wall(s, 0, -0.15f, -10, 35, 0.15f, 12, road);
    set_last_material(s, MAT_TILE);

    // Sky Tower — the main event, rising from the ground
    // Placed right behind the entrance so it dominates the view
    add_skytower(s, 0, 0, 8, 6.0f, tower_steel);

    // Base structure — concrete podium building at the tower's feet
    add_wall(s, 0, 3, 5, 12, 6, 8, tower_base);
    set_last_material(s, MAT_CONCRETE);
    // Darker inset panels on the base — marble cladding
    add_wall(s, -4, 3, 0.9f, 3, 4, 0.1f, (Color){110,108,115,255});
    set_last_material(s, MAT_MARBLE);
    add_wall(s, 4, 3, 0.9f, 3, 4, 0.1f, (Color){110,108,115,255});
    set_last_material(s, MAT_MARBLE);

    // Ground floor — lit entrance (warm against cold concrete)
    add_wall(s, 0, 1.8f, 0.95f, 4, 3.6f, 0.1f, (Color){250, 230, 170, 200});
    // Awning — Godard red, same as Paris
    add_wall(s, 0, 3.8f, 0, 5, 0.1f, 2, godard_red);
    add_wall(s, -2.4f, 2.5f, 0, 0.06f, 2.5f, 0.06f, gold);
    set_last_material(s, MAT_BRASS);
    add_wall(s, 2.4f, 2.5f, 0, 0.06f, 2.5f, 0.06f, gold);
    set_last_material(s, MAT_BRASS);

    // Entrance sign — gold strip
    add_wall(s, 0, 4.2f, 0.92f, 6, 0.4f, 0.04f, gold);
    set_last_material(s, MAT_BRASS);

    // Windows on the base structure
    for (int floor = 0; floor < 2; floor++) {
        for (int col = 0; col < 4; col++) {
            float wx = -4.5f + col * 3;
            float wy = 2.5f + floor * 2.5f;
            add_wall(s, wx, wy, 0.88f, 1.5f, 1.8f, 0.05f, window_lit);
            set_last_material(s, MAT_GLASS);
        }
    }

    // Lamppost left — brass pole
    add_wall(s, -7, 1.8f, -2, 0.15f, 3.6f, 0.15f, (Color){70, 65, 55, 255});
    set_last_material(s, MAT_BRASS);
    add_light_panel(s, -7, 3.8f, -2, 0.6f, 0.6f, 0.6f, (Color){240, 210, 120, 200});
    // Lamppost right — brass pole
    add_wall(s, 7, 1.8f, -2, 0.15f, 3.6f, 0.15f, (Color){70, 65, 55, 255});
    set_last_material(s, MAT_BRASS);
    add_light_panel(s, 7, 3.8f, -2, 0.6f, 0.6f, 0.6f, (Color){240, 210, 120, 200});

    // Auckland CBD silhouettes — varied materials for visual depth
    add_wall(s, -18, 8, 18, 6, 16, 3, (Color){20, 22, 38, 255});
    set_last_material(s, MAT_CONCRETE);
    add_wall(s, 16, 6, 20, 5, 12, 3, (Color){18, 20, 35, 255});
    set_last_material(s, MAT_TILE);  // glass-fronted office tower
    add_wall(s, -10, 5, 22, 8, 10, 3, (Color){22, 24, 40, 255});
    set_last_material(s, MAT_MARBLE);  // older stone building
    add_wall(s, 22, 7, 16, 4, 14, 3, (Color){16, 18, 32, 255});
    set_last_material(s, MAT_GLASS);

    // Wet pavement reflection — rain-slicked streets catch lamplight
    add_wall(s, -5, 0.01f, -3, 4, 0.01f, 3, (Color){240, 210, 120, 20});
    set_last_decal(s);
    add_wall(s, 5, 0.01f, -2, 3, 0.01f, 2, (Color){240, 210, 120, 15});
    set_last_decal(s);
    // Entrance glow pool on sidewalk
    add_wall(s, 0, 0.01f, -1, 4, 0.01f, 2, (Color){250, 230, 170, 25});
    set_last_decal(s);

    // Parked taxi — you just got out of this
    add_wall(s, -5, 0.7f, -5, 2.2f, 1.2f, 4.0f, (Color){220, 200, 50, 255}); // body
    set_last_material(s, MAT_TILE);  // painted metal reads as tile sheen
    add_wall(s, -5, 1.4f, -5.2f, 1.8f, 0.5f, 2.2f, (Color){220, 200, 50, 255}); // roof
    set_last_material(s, MAT_TILE);
    add_wall(s, -5, 0.9f, -5, 2.0f, 0.7f, 3.6f, (Color){30, 35, 45, 200}); // windows
    set_last_material(s, MAT_GLASS);
    // Taillights — Godard red accents
    add_wall(s, -4.1f, 0.6f, -7.0f, 0.15f, 0.1f, 0.04f, PAL_RED);
    add_wall(s, -5.9f, 0.6f, -7.0f, 0.15f, 0.1f, 0.04f, PAL_RED);

    // Street furniture
    // Bollard — brass-topped
    add_cylinder(s, -3, 0.4f, -3, 0.15f, 0.8f, (Color){80, 78, 75, 255});
    set_last_material(s, MAT_BRASS);
    add_cylinder(s, 3, 0.4f, -3, 0.15f, 0.8f, (Color){80, 78, 75, 255});
    set_last_material(s, MAT_BRASS);
    // Trash bin
    add_wall(s, 8, 0.5f, -3, 0.5f, 1.0f, 0.5f, (Color){55, 55, 55, 255});
    set_last_material(s, MAT_TILE);

    // More city buildings — deeper, taller, creating a canyon feel
    add_wall(s, -25, 12, 25, 8, 24, 4, (Color){15, 17, 30, 255});
    set_last_material(s, MAT_TILE);
    add_wall(s, 28, 10, 22, 6, 20, 4, (Color){14, 16, 28, 255});
    set_last_material(s, MAT_CONCRETE);
    // Lit windows on distant buildings
    for (int b = 0; b < 5; b++) {
        float bx = -22 + b * 4.0f + ((b*7)%3);
        float by = 4 + (b*3)%8;
        float bz = 18 + (b*5)%6;
        add_wall(s, bx, by, bz, 0.8f, 0.6f, 0.05f, (Color){240, 210, 130, (unsigned char)(60+(b*31)%80)});
    }

    // Interactive entrance door — diegetic. Walk up, press E, enter.
    add_object(s, 0, 1.8f, 0.5f, "door", (Color){230, 200, 110, 255}, 1);

    // ============================================================
    // AUCKLAND AT 2 AM — the city is mostly asleep
    // Rain-slicked streets, neon haze, harbor in the distance.
    // Michael Mann's Collateral: beauty in empty streets.
    // ============================================================

    // RAIN PUDDLES — irregular pools reflecting the tower and lampposts
    // Large puddle near entrance — reflects the warm entrance glow
    add_wall(s, -2, 0.005f, -2, 3.0f, 0.005f, 2.5f, (Color){250, 230, 170, 18});
    set_last_decal(s);
    // Puddle catching lamppost glow — left
    add_wall(s, -8, 0.005f, -3, 2.5f, 0.005f, 2.0f, (Color){240, 210, 120, 22});
    set_last_decal(s);
    // Puddle near taxi — red taillight reflection
    add_wall(s, -5, 0.005f, -6, 2.0f, 0.005f, 1.5f, (Color){180, 40, 40, 15});
    set_last_decal(s);
    // Narrow puddle along curb — continuous
    add_wall(s, 0, 0.003f, -7, 12, 0.005f, 0.6f, (Color){120, 140, 170, 10});
    set_last_decal(s);

    // NEON SIGNAGE — Auckland CBD nocturnal texture
    // Bar sign on left building — Godard red neon rectangle
    add_wall(s, -16, 4.5f, 17.5f, 2.5f, 0.8f, 0.06f, (Color){220, 45, 40, 160});
    // Neon glow pool on sidewalk below
    add_wall(s, -16, 0.01f, 15, 2.0f, 0.005f, 2.0f, (Color){180, 35, 30, 12});
    set_last_decal(s);
    // Pharmacy cross — blue-green, right side
    add_wall(s, 14, 5.0f, 18, 0.3f, 1.2f, 0.06f, (Color){40, 200, 180, 140});
    add_wall(s, 14, 5.0f, 18, 1.2f, 0.3f, 0.06f, (Color){40, 200, 180, 140});
    // Hotel name — brass letters on the awning (warm glow)
    add_light_panel(s, 0, 4.0f, 0.3f, 4.0f, 0.3f, 0.1f, (Color){240, 210, 120, 60});

    // NIGHT SKY — stars visible between buildings, above the tower
    for (int i = 0; i < 25; i++) {
        float sx = -30.0f + (float)((i*37)%60);
        float sy = 20.0f + (float)((i*17)%30);
        float sz = 30.0f + (float)((i*13)%20);
        float size = 0.2f + (float)((i*7)%3) * 0.1f;
        add_wall(s, sx, sy, sz, size, size, size,
                 (Color){220,218,210,(unsigned char)(60+(i*23)%80)});
    }

    // SKY TOWER TOP — observation deck glow and beacon
    // The tower model is at (0, 0, 8), scale 6.0
    // Red beacon at the very top — aircraft warning light
    add_sphere(s, 0, 52, 8, 0.5f, (Color){220, 30, 30, 180});
    // Observation deck glow — amber ring of light high up
    add_cylinder(s, 0, 38, 8, 4.0f, 0.3f, (Color){240, 210, 120, 40});
    // Restaurant level glow — warm amber band
    add_wall(s, 0, 35, 7.5f, 3.5f, 1.2f, 0.1f, (Color){240, 200, 110, 60});

    // HARBOR — Auckland's Waitemata Harbour in the distance
    // Water surface — dark, faintly reflective, far behind the tower
    add_wall(s, 0, -0.5f, 35, 80, 0.05f, 25, (Color){8, 14, 28, 255});
    // Harbor reflections — scattered city light on water
    for (int i = 0; i < 8; i++) {
        float hx = -20.0f + (float)((i*31)%40);
        float hz = 30.0f + (float)((i*13)%20);
        add_wall(s, hx, -0.45f, hz, 1.5f, 0.02f, 0.3f,
                 (Color){240, 200, 110, (unsigned char)(15+(i*11)%20)});
        set_last_decal(s);
    }
    // Harbor Bridge — distant silhouette, arc shape approximated
    add_wall(s, -15, 3, 45, 35, 0.3f, 0.5f, (Color){20, 22, 30, 255});
    // Bridge lights — dotted line of amber
    for (int i = 0; i < 12; i++) {
        float bx = -28.0f + i * 4.5f;
        float by = 3.2f + sinf(i * 0.5f) * 0.8f;  // gentle arc
        add_wall(s, bx, by, 45, 0.15f, 0.15f, 0.15f,
                 (Color){240, 200, 100, (unsigned char)(80+(i*17)%60)});
    }

    // STREET LIFE — 2 AM detritus
    // Takeaway container on sidewalk — someone ate here
    add_wall(s, 4, 0.02f, -5, 0.25f, 0.08f, 0.2f, (Color){235, 230, 220, 200});
    // Newspaper page caught against bollard — wind-pressed
    add_wall(s, 3.1f, 0.3f, -3, 0.4f, 0.5f, 0.02f, (Color){220, 215, 205, 150});
    // Shopping bag — plastic, ghostly
    add_wall(s, -9, 0.15f, -1, 0.2f, 0.3f, 0.15f, (Color){200, 200, 210, 60});
    // Traffic light — red (intersection empty, but the light doesn't know)
    add_wall(s, 10, 3.5f, -8, 0.12f, 0.35f, 0.12f, (Color){30, 30, 30, 255});
    add_wall(s, 10, 3.65f, -8.08f, 0.08f, 0.08f, 0.02f, (Color){220, 30, 30, 180});
    add_wall(s, 10, 0.01f, -8, 0.8f, 0.005f, 0.8f, (Color){180, 30, 30, 10});
    set_last_decal(s);

    // DRAIN GRATE — brass, mundane, real
    add_wall(s, -1, 0.005f, -6, 0.5f, 0.01f, 0.5f, (Color){100, 90, 70, 200});
    set_last_material(s, MAT_BRASS);

    // SECOND AWNING — closed shop next door, dark
    add_wall(s, -8, 3.0f, 0.5f, 3, 0.08f, 1.5f, (Color){25, 28, 35, 255});
    // Rolled-down security shutter
    add_wall(s, -8, 1.5f, 0.95f, 3, 3, 0.06f, (Color){45, 48, 55, 255});
    set_last_material(s, MAT_CONCRETE);

    s->spawn = (Vector3){0, 1.6f, -4};
    s->exit_pos = (Vector3){0, 1.6f, 0.5f};
    s->has_exit = true;

    // ── THE MOTIF: cigarette ──
    // Someone just stepped inside. Still glowing on the wet pavement.
    add_cylinder(s, 1.2f, 0.01f, -4.0f, 0.025f, 0.05f, (Color){230,225,215,200});
    add_wall(s, 1.22f, 0.015f, -4.0f, 0.025f, 0.015f, 0.025f, (Color){200,80,40,180});

    // ── THE MOTIF: red book ──
    // On the bench near the entrance. Rain-spotted but intact.
    add_red_book(s, 3.5f, 0.55f, -2.0f, 7.0f);

}

void build_lobby(Scene *s) {
    *s = (Scene){0};
    // BOUNDS: 30m x 20m, partially enclosed (front wall has gaps for entrance)
    s->surface = SURFACE_MARBLE;

    Color cream = PAL_CREAM;
    Color gold = PAL_BRASS;
    Color emerald = PAL_GREEN;
    Color concrete_a = PAL_MARBLE_A;
    Color concrete_b = PAL_MARBLE_B;
    Color marble_a = {60, 58, 55, 255};       // dark marble floor
    Color plant = {60, 130, 65, 255};
    Color terracotta = PAL_TERRACOTTA;

    s->fog_color = PAL_FOG_GREEN;
    s->fog_density = 0.004f;  // lighter fog — let chandelier light reach

    // Floor — single plane, checkerboard material does the pattern
    add_wall(s, 0, -0.05f, 0, 30, 0.1f, 20, marble_a);
    set_last_material(s, MAT_CHECKERBOARD);

    add_wall(s, 0, 7, 0, 30, 0.2f, 20, cream);
    set_last_material(s, MAT_WALLPAPER);
    // Hanging sphere light fixtures
    add_sphere(s, -4, 6.4f, 0, 0.5f, PAL_LIGHT_WARM);
    add_sphere(s, -2, 6.4f, -2, 0.45f, PAL_LIGHT_WARM);
    add_sphere(s, 4, 6.4f, -3, 0.5f, PAL_LIGHT_WARM);
    add_sphere(s, 2, 6.4f, 1, 0.45f, PAL_LIGHT_WARM);

    add_wall(s, 0, 3.5f, -10, 30, 7, 0.3f, emerald);   // back wall — entirely deep green
    set_last_material(s, MAT_WALLPAPER);
    add_wall(s, -12, 3.5f, 10, 10, 7, 0.3f, cream);
    set_last_material(s, MAT_WALLPAPER);
    add_wall(s, 10, 3.5f, 10, 8, 7, 0.3f, cream);
    set_last_material(s, MAT_WALLPAPER);
    add_wall(s, -2, 5.5f, 10, 6, 3, 0.3f, cream);
    set_last_material(s, MAT_WALLPAPER);
    add_wall(s, -15, 3.5f, 0, 0.3f, 7, 20, cream);     // side walls cream
    set_last_material(s, MAT_WALLPAPER);
    add_wall(s, 15, 3.5f, -5, 0.3f, 7, 10, cream);
    set_last_material(s, MAT_WALLPAPER);
    add_wall(s, 15, 3.5f, 6, 0.3f, 7, 8, cream);
    set_last_material(s, MAT_WALLPAPER);
    // Green wainscoting on side walls — lower third
    add_wall(s, -14.8f, 1.0f, 0, 0.1f, 2.0f, 20, emerald);
    set_last_material(s, MAT_WALLPAPER);
    add_wall(s, 14.8f, 1.0f, 0, 0.1f, 2.0f, 20, emerald);
    set_last_material(s, MAT_WALLPAPER);

    add_wall(s, 0, 1.0f, -9.85f, 30, 0.06f, 0.1f, gold);
    set_last_material(s, MAT_BRASS);
    add_wall(s, -14.85f, 1.0f, 0, 0.1f, 0.06f, 20, gold);
    set_last_material(s, MAT_BRASS);
    add_wall(s, 14.85f, 1.0f, 0, 0.1f, 0.06f, 20, gold);
    set_last_material(s, MAT_BRASS);

    // Concrete reveal panel — architectural, not pop art
    add_wall(s, 0, 3.5f, -9.8f, 8, 3.5f, 0.08f, concrete_a);
    set_last_material(s, MAT_CONCRETE);  // intentional raw concrete
    add_wall(s, 0, 3.5f, -9.75f, 7.8f, 3.3f, 0.04f, concrete_b);
    set_last_material(s, MAT_MARBLE);

    add_column(s, -8, -4, 0.5f, 7, cream);
    add_column(s, -3, 1, 0.45f, 7, cream);
    add_column(s, 4, -5, 0.5f, 7, cream);
    add_column(s, 7, 2, 0.4f, 7, cream);

    add_wall(s, -11, 1.5f, -8, 2, 3, 0.4f, plant);
    set_last_material(s, MAT_FABRIC);
    add_wall(s, 10, 1, -8, 1.5f, 2, 0.4f, plant);
    set_last_material(s, MAT_FABRIC);

    add_wall(s, 0, 0.5f, -7, 6, 1.0f, 1.5f, gold);
    set_last_material(s, MAT_WOOD);
    add_wall(s, 0, 1.02f, -7, 6.1f, 0.04f, 1.55f, (Color){230, 200, 110, 255});
    set_last_material(s, MAT_BRASS);

    // Desk lamp — cylinder base + shade + glow
    add_cylinder(s, 2.2f, 1.08f, -7.0f, 0.06f, 0.04f, gold);          // base
    add_cylinder(s, 2.2f, 1.18f, -7.0f, 0.03f, 0.15f, gold);          // shaft
    add_cylinder(s, 2.25f, 1.28f, -7.0f, 0.12f, 0.08f, (Color){235,228,215,255}); // shade
    add_light_panel(s, 2.25f, 1.22f, -7.0f, 0.18f, 0.2f, 0.18f, (Color){240,220,160,160}); // glow

    // Computer screen on desk — dark with faint glow
    add_wall(s, -1.5f, 1.22f, -7.2f, 0.5f, 0.35f, 0.04f, (Color){30,32,35,255});   // screen
    add_wall(s, -1.5f, 1.22f, -7.18f, 0.46f, 0.31f, 0.02f, (Color){80,120,160,60}); // glow

    // Hotel bell on desk
    add_cylinder(s, 0.8f, 1.07f, -6.5f, 0.06f, 0.03f, (Color){200,195,180,255});

    // Guest book — flat rectangle
    add_wall(s, -0.3f, 1.05f, -6.5f, 0.5f, 0.03f, 0.35f, (Color){180,170,150,255});

    add_wall(s, -7, 0.3f, 5, 2.5f, 0.6f, 0.7f, (Color){165, 130, 85, 255});
    set_last_material(s, MAT_LEATHER);
    add_wall(s, -7, 0.3f, -1, 2.5f, 0.6f, 0.7f, (Color){165, 130, 85, 255});
    set_last_material(s, MAT_LEATHER);

    // Terracotta bench — ONE accent
    add_wall(s, 5, 0.25f, -8, 2.5f, 0.5f, 0.7f, terracotta);
    set_last_material(s, MAT_FABRIC);

    // Newspaper on bench — environmental storytelling
    add_wall(s, 5.1f, 0.52f, -8, 0.6f, 0.02f, 0.4f, (Color){235,232,228,255});
    add_object(s, 5.1f, 0.6f, -8, "newspaper", (Color){235,232,228,255}, 1);

    add_light_panel(s, 14.8f, 1.5f, 1, 0.1f, 3, 2.5f, gold);

    // Dropped ceiling — coffered effect in lobby center
    add_dropped_ceiling(s, 0, 7, -2, 8, 6, 0.3f, cream, gold);

    // LIGHT SHAFT — light pouring in from entrance onto lobby floor
    add_wall(s, 0, 0.02f, 8, 3.0f, 0.02f, 4.0f, (Color){235,230,218,100});
    // Subtle light band on back wall from ceiling panels
    add_wall(s, 0, 4.5f, -9.78f, 8.0f, 0.8f, 0.02f, (Color){245,240,230,80});

    // Elevator doors on back wall — two tall brass rectangles with dark gap
    add_wall(s, -3.0f, 1.5f, -9.78f, 1.0f, 3.0f, 0.1f, gold);   // left door
    add_wall(s, -1.0f, 1.5f, -9.78f, 1.0f, 3.0f, 0.1f, gold);   // right door
    add_wall(s, -2.0f, 1.5f, -9.82f, 0.08f, 3.0f, 0.04f, (Color){30,28,25,255}); // gap
    // Frame
    add_wall(s, -2.0f, 3.05f, -9.76f, 2.2f, 0.1f, 0.08f, gold);  // top frame
    add_wall(s, -3.55f, 1.5f, -9.76f, 0.1f, 3.0f, 0.08f, gold);  // left frame
    add_wall(s, -0.45f, 1.5f, -9.76f, 0.1f, 3.0f, 0.08f, gold);  // right frame

    // Elevator doors — interactive, the Glass Elevator to space
    add_object(s, -2.0f, 1.5f, -9.5f, "elevator", gold, 1);

    // Door frame around elevator doors
    add_door_frame(s, -2.0f, 1.5f, -9.76f, 2.2f, 3.0f, 0.1f, gold);

    // Hotel bell on reception desk — ring it (foreshadows elevator ding)
    add_sphere(s, -4.5f, 0.82f, -7.0f, 0.08f, gold);
    add_cylinder(s, -4.5f, 0.78f, -7.0f, 0.1f, 0.02f, gold);
    add_object(s, -4.5f, 0.85f, -7.0f, "bell", gold, 1);

    // ── THE MOTIF: cigarette ──
    // Left on a lobby bench. Someone was here before you.
    add_cylinder(s, 5.3f, 0.53f, -7.8f, 0.025f, 0.05f, (Color){230,225,215,200});

    // ============================================================
    // EVIDENCE OF DEPARTURE — someone was just here
    // The lobby is populated by absence. Gravity Bone: objects tell stories.
    // ============================================================
    // Forgotten coat on column hook — dark wool, draped
    add_wall(s, -7.8f, 1.4f, -4, 0.04f, 0.8f, 0.3f, (Color){35,32,28,255});
    set_last_material(s, MAT_FABRIC);
    // Hook on column
    add_wall(s, -7.85f, 1.85f, -4, 0.06f, 0.06f, 0.04f, gold);
    set_last_material(s, MAT_BRASS);
    // Abandoned suitcase — near entrance, someone left without it
    add_wall(s, 3.5f, 0.15f, 6, 0.6f, 0.3f, 0.4f, (Color){95,60,30,255});
    set_last_material(s, MAT_LEATHER);
    add_wall(s, 3.5f, 0.32f, 6, 0.62f, 0.02f, 0.42f, gold);
    set_last_material(s, MAT_BRASS);
    set_last_decal(s);
    // Key left on desk — small brass rectangle
    add_wall(s, 1.2f, 1.05f, -6.8f, 0.08f, 0.02f, 0.04f, gold);
    set_last_material(s, MAT_BRASS);
    // Spilled coffee — dark ring stain near the bench
    add_wall(s, 5.5f, 0.01f, -7.5f, 0.35f, 0.01f, 0.35f, (Color){60,45,30,60});
    set_last_decal(s);

    // Picture frames flanking concrete reveal panel on back wall
    add_picture_frame(s, -6.0f, 3.5f, -9.72f, 1.0f, 0.7f, gold, (Color){195,45,40,255});
    add_picture_frame(s, 6.0f, 3.5f, -9.72f, 1.0f, 0.7f, gold, (Color){40,65,160,255});

    // Baseboards — all 4 walls at floor level
    add_baseboard(s, 0, 0, -9.85f, 30, 0, (Color){105,78,48,255});
    add_baseboard(s, 0, 0, 9.85f, 30, 0, (Color){105,78,48,255});
    add_baseboard(s, -14.85f, 0, 0, 20, 1.0f, (Color){105,78,48,255});
    add_baseboard(s, 14.85f, 0, 0, 20, 1.0f, (Color){105,78,48,255});

    // BLOCKING WALL — seal the front entrance gap to prevent escape into void
    // The entrance area (z=10) has gaps between wall segments; block at z=10
    add_wall(s, -1, 2, 10, 12, 4, 0.3f, cream);  // lower fill across entrance

    // ============================================================
    // GRAND STAIRCASE — wide stairs up to mezzanine at y=2.4
    // Against the back wall, 8 steps, each 0.3m high, 0.6m deep, 4m wide
    // ============================================================
    float stair_w = 4.0f;
    float step_h = 0.3f;
    float step_d = 0.6f;
    int num_steps = 8;
    float stair_base_z = -8.0f;  // starts near back wall

    for (int i = 0; i < num_steps; i++) {
        float sy = (i + 1) * step_h - step_h / 2;  // center of step
        float sz = stair_base_z + i * step_d;
        add_wall(s, 0, sy, sz, stair_w, step_h, step_d, concrete_a);
        set_last_material(s, MAT_MARBLE);
    }

    // Brass banister railings along staircase sides
    for (int i = 0; i < num_steps; i++) {
        float sy = (i + 1) * step_h + 0.45f;  // railing height above step
        float sz = stair_base_z + i * step_d;
        add_wall(s, -stair_w / 2 - 0.05f, sy, sz, 0.06f, 0.9f, 0.06f, gold);
        set_last_material(s, MAT_BRASS);
        add_wall(s, stair_w / 2 + 0.05f, sy, sz, 0.06f, 0.9f, 0.06f, gold);
        set_last_material(s, MAT_BRASS);
    }
    // Railing top rail — angled, approximated with one long bar
    float rail_y_bot = step_h + 0.9f;
    float rail_y_top = num_steps * step_h + 0.9f;
    float rail_y_mid = (rail_y_bot + rail_y_top) / 2;
    float rail_z_mid = stair_base_z + (num_steps - 1) * step_d / 2;
    float rail_len = num_steps * step_d;
    add_wall(s, -stair_w / 2 - 0.05f, rail_y_mid, rail_z_mid, 0.04f, 0.04f, rail_len, gold);
    set_last_material(s, MAT_BRASS);
    add_wall(s, stair_w / 2 + 0.05f, rail_y_mid, rail_z_mid, 0.04f, 0.04f, rail_len, gold);
    set_last_material(s, MAT_BRASS);

    // MEZZANINE PLATFORM — 4m x 6m at the top of the stairs
    float mezz_y = num_steps * step_h;  // 2.4m
    float mezz_z = stair_base_z + num_steps * step_d + 3.0f;  // extends back from top step
    // Platform floor
    add_wall(s, 0, mezz_y - 0.05f, mezz_z - 3.0f + step_d, stair_w + 2, 0.1f, 6.0f, concrete_b);
    set_last_material(s, MAT_MARBLE);

    // Mezzanine railing — overlooking the lobby floor
    float railing_h = 0.9f;
    // Front railing (facing lobby)
    add_wall(s, 0, mezz_y + railing_h / 2, mezz_z - 6.0f + step_d, stair_w + 2, railing_h, 0.06f, gold);
    set_last_material(s, MAT_BRASS);
    // Side railings
    add_wall(s, -(stair_w / 2 + 1), mezz_y + railing_h / 2, mezz_z - 3.0f + step_d,
             0.06f, railing_h, 6.0f, gold);
    set_last_material(s, MAT_BRASS);
    add_wall(s, (stair_w / 2 + 1), mezz_y + railing_h / 2, mezz_z - 3.0f + step_d,
             0.06f, railing_h, 6.0f, gold);
    set_last_material(s, MAT_BRASS);

    // Mezzanine light panel — warm glow above
    add_light_panel(s, 0, 6.8f, mezz_z - 2.0f, 3.0f, 0.05f, 2.0f, gold);

    // ── THE MOTIF: cigarette ──
    // In the ashtray by the entrance. Brass tray, crushed butt.
    // The lobby is non-smoking. Someone didn't care.
    add_cylinder(s, 3.5f, 0.32f, 7.5f, 0.025f, 0.05f, (Color){230,225,215,200});
    add_wall(s, 3.52f, 0.325f, 7.5f, 0.01f, 0.005f, 0.01f, (Color){200,80,40,160});

    s->spawn = (Vector3){0, 1.6f, 8};
    // Exit is now on the mezzanine — you go UP to leave the lobby
    s->exit_pos = (Vector3){0, mezz_y + 1.6f, mezz_z - 1.0f};
    s->has_exit = true;
}

void build_hallway(Scene *s) {
    *s = (Scene){0};
    // BOUNDS: 4.5m x 40m, fully enclosed (4 walls + floor + ceiling)
    s->surface = SURFACE_CARPET;

    Color cream = PAL_CREAM;
    Color gold = PAL_BRASS;
    Color godard_red = PAL_RED;
    Color godard_blue = PAL_BLUE;
    Color carpet_a = {160, 55, 45, 255};      // DOMINANT — warm terracotta/red carpet
    Color warm_amber = PAL_GLOW_AMBER;
    Color ceiling_dark = PAL_CHARCOAL;

    s->fog_color = PAL_FOG_RED;
    s->fog_density = 0.010f;
    float L = 20, W = 4.5f, H = 4;

    // Floor — single plane, checkerboard carpet
    add_wall(s, 0, -0.05f, -L/2, W, 0.1f, L, carpet_a);
    set_last_material(s, MAT_CHECKERBOARD);

    add_wall(s, 0, H, -L/2, W, 0.2f, L, ceiling_dark);
    for (int i = 0; i < 4; i++)
        add_light_panel(s, 0, H-0.1f, -3 - i*5, 1.5f, 0.05f, 0.5f, warm_amber);

    add_wall(s, -W/2, H/2, -L/2, 0.3f, H, L, cream);
    set_last_material(s, MAT_WALLPAPER);
    add_wall(s, W/2, H/2, -L/2, 0.3f, H, L, cream);
    set_last_material(s, MAT_WALLPAPER);
    add_wall(s, 0, H/2, 0, W, H, 0.3f, cream);
    set_last_material(s, MAT_WALLPAPER);
    add_wall(s, 0, H/2, -L, W, H, 0.3f, cream);
    set_last_material(s, MAT_WALLPAPER);

    add_wall(s, -W/2+0.05f, 1.0f, -L/2, 0.08f, 0.04f, L, gold);
    set_last_material(s, MAT_BRASS);
    add_wall(s, W/2-0.05f, 1.0f, -L/2, 0.08f, 0.04f, L, gold);
    set_last_material(s, MAT_BRASS);

    for (int i = 0; i < 4; i++) {
        float z = -3.5f - i*4.5f;
        Color door_c = (i%2==0) ? godard_red : godard_blue;
        float side = (i%2==0) ? -(W/2-0.1f) : (W/2-0.1f);
        add_wall(s, side, 1.3f, z, 0.12f, 2.6f, 1.3f, door_c);
        add_wall(s, side, 2.75f, z, 0.12f, 0.15f, 1.4f, gold);
        add_wall(s, side, 1.3f, z-0.7f, 0.12f, 2.6f, 0.06f, gold);
        add_wall(s, side, 1.3f, z+0.7f, 0.12f, 2.6f, 0.06f, gold);
        add_wall(s, side*0.95f, 2, z-0.9f, 0.06f, 0.25f, 0.18f, gold);
        add_light_panel(s, side, 0.03f, z, 0.06f, 0.05f, 1.1f, warm_amber);
    }

    add_wall(s, 0, 0.01f, -L/2, 1.6f, 0.02f, L, (Color){170, 65, 50, 255});
    add_light_panel(s, 0, 2.2f, -L+0.2f, 2.5f, 1.8f, 0.05f, (Color){230, 200, 120, 180});

    // Flowers — near the end window
    add_wall(s, W/2-0.12f, 0.8f, -L+4, 0.35f, 0.04f, 0.22f, gold);
    add_wall(s, W/2-0.12f, 0.92f, -L+4, 0.07f, 0.18f, 0.07f, (Color){200, 200, 210, 200});
    add_wall(s, W/2-0.14f, 1.1f, -L+4, 0.09f, 0.06f, 0.09f, godard_red);

    // LIGHT SHAFT — light from end window, trapezoid approximation on floor
    add_wall(s, 0, 0.02f, -L+2, 1.5f, 0.02f, 2.0f, (Color){230,225,215,90});
    add_wall(s, 0, 0.02f, -L+4, 2.0f, 0.02f, 2.0f, (Color){230,225,215,70});
    add_wall(s, 0, 0.02f, -L+6, 2.5f, 0.02f, 2.0f, (Color){230,225,215,50});

    // "Do Not Disturb" sign on door 2 (left side)
    add_wall(s, -(W/2-0.1f)-0.02f, 1.1f, -3.5f - 1*4.5f, 0.3f, 0.15f, 0.04f, (Color){200,50,40,255});

    // Door frames around all 4 doors
    for (int i = 0; i < 4; i++) {
        float z = -3.5f - i*4.5f;
        float side = (i%2==0) ? -(W/2-0.1f) : (W/2-0.1f);
        add_door_frame(s, side, 1.3f, z, 1.3f, 2.6f, 0.14f, gold);
    }

    // Picture frames between doors on alternate walls
    add_picture_frame(s, -(W/2-0.08f), 2.2f, -6.0f, 0.6f, 0.45f, gold, (Color){195,45,40,255});
    add_picture_frame(s, (W/2-0.08f), 2.2f, -15.0f, 0.6f, 0.45f, gold, (Color){40,65,160,255});

    // Baseboards — both long walls
    add_baseboard(s, -W/2+0.08f, 0, -L/2, L, 1.0f, (Color){105,78,48,255});
    add_baseboard(s, W/2-0.08f, 0, -L/2, L, 1.0f, (Color){105,78,48,255});
    // Baseboards — end walls
    add_baseboard(s, 0, 0, -0.08f, W, 0, (Color){105,78,48,255});
    add_baseboard(s, 0, 0, -L+0.08f, W, 0, (Color){105,78,48,255});

    // Crown moldings — both long walls at ceiling
    add_crown_molding(s, -W/2+0.08f, H, -L/2, L, 1.0f, gold);
    add_crown_molding(s, W/2-0.08f, H, -L/2, L, 1.0f, gold);

    // Room service tray outside door 3 (right side)
    add_wall(s, (W/2-0.1f)*0.95f, 0.03f, -3.5f - 2*4.5f, 0.5f, 0.04f, 0.35f, (Color){200,198,195,255});
    add_cylinder(s, (W/2-0.1f)*0.95f - 0.1f, 0.1f, -3.5f - 2*4.5f, 0.08f, 0.1f, (Color){240,238,234,255});
    add_wall(s, (W/2-0.1f)*0.95f + 0.1f, 0.06f, -3.5f - 2*4.5f, 0.2f, 0.02f, 0.2f, (Color){240,238,234,255});

    s->spawn = (Vector3){0, 1.6f, -1};
    s->exit_pos = (Vector3){0, 1.6f, -L+1};
    s->has_exit = true;
}

void build_hotel_room(Scene *s) {
    *s = (Scene){0};
    // BOUNDS: 12m x 10m, fully enclosed (4 walls + floor + ceiling + corner blocks)
    s->surface = SURFACE_WOOD;

    Color wall_gold = {200, 188, 155, 255};  // warm champagne — Hotel Chevalier, not Minecraft
    Color gold = PAL_BRASS;
    Color cream = PAL_CREAM;
    Color godard_red = PAL_RED;
    Color wood = PAL_WOOD_DARK;
    Color dark_wood = {105, 78, 48, 255};
    Color white = PAL_WHITE;
    Color warm_gray = PAL_NAVY;
    Color warm_light = PAL_LIGHT_WARM;

    s->fog_color = PAL_FOG_GOLD;
    s->fog_density = 0.006f;
    float rw = 12, rd = 10, rh = 3.8f;

    // Floor — single plane, parquet wood
    add_wall(s, 0, -0.05f, 0, rw, 0.1f, rd, wood);
    set_last_material(s, MAT_PARQUET);

    add_wall(s, 0, rh, 0, rw, 0.2f, rd, (Color){165, 158, 145, 255});  // warm gray ceiling — NOT bright cream
    set_last_material(s, MAT_CONCRETE);
    add_wall(s, 0, rh-0.08f, -rd/2+0.08f, rw, 0.12f, 0.06f, gold);
    set_last_material(s, MAT_BRASS);
    add_wall(s, 0, rh-0.08f, rd/2-0.08f, rw, 0.12f, 0.06f, gold);
    set_last_material(s, MAT_BRASS);
    add_wall(s, -rw/2+0.08f, rh-0.08f, 0, 0.06f, 0.12f, rd, gold);
    set_last_material(s, MAT_BRASS);
    add_wall(s, rw/2-0.08f, rh-0.08f, 0, 0.06f, 0.12f, rd, gold);
    set_last_material(s, MAT_BRASS);

    add_wall(s, 0, rh/2, -rd/2, rw, rh, 0.3f, wall_gold);
    set_last_material(s, MAT_WALLPAPER);
    add_wall(s, 0, rh/2, rd/2, rw, rh, 0.3f, wall_gold);
    set_last_material(s, MAT_WALLPAPER);
    add_wall(s, -rw/2, rh/2, 0, 0.3f, rh, rd, wall_gold);
    set_last_material(s, MAT_WALLPAPER);
    add_wall(s, rw/2, rh/2, 0, 0.3f, rh, rd, wall_gold);
    set_last_material(s, MAT_WALLPAPER);

    // Baseboards — dark wood
    add_wall(s, 0, 0.45f, -rd/2+0.08f, rw, 0.9f, 0.06f, dark_wood);
    add_wall(s, 0, 0.45f, rd/2-0.08f, rw, 0.9f, 0.06f, dark_wood);
    add_wall(s, -rw/2+0.08f, 0.45f, 0, 0.06f, 0.9f, rd, dark_wood);
    add_wall(s, rw/2-0.08f, 0.45f, 0, 0.06f, 0.9f, rd, dark_wood);

    // Recessed wall panels — slightly darker, create visual rhythm + depth
    {
        Color panel = {180, 170, 142, 255};  // darker champagne — shadow reveal
        // Back wall: 3 evenly spaced panels
        for (int p = 0; p < 3; p++) {
            float px = -3.0f + p * 3.0f;
            add_wall(s, px, rh*0.5f, -rd/2+0.05f, 2.2f, rh*0.6f, 0.02f, panel);
            set_last_material(s, MAT_WALLPAPER);
        }
        // Side walls: 2 panels each
        for (int p = 0; p < 2; p++) {
            float pz = -2.0f + p * 4.0f;
            add_wall(s, -rw/2+0.05f, rh*0.5f, pz, 0.02f, rh*0.6f, 2.8f, panel);
            set_last_material(s, MAT_WALLPAPER);
            add_wall(s, rw/2-0.05f, rh*0.5f, pz, 0.02f, rh*0.6f, 2.8f, panel);
            set_last_material(s, MAT_WALLPAPER);
        }
    }

    // Bed
    add_wall(s, 0, 0.2f, -3.5f, 3.4f, 0.4f, 2.0f, dark_wood);
    set_last_material(s, MAT_WOOD);
    add_wall(s, 0, 0.5f, -3.5f, 3.2f, 0.25f, 1.8f, white);
    set_last_material(s, MAT_FABRIC);
    add_wall(s, -0.6f, 0.68f, -4.2f, 0.7f, 0.2f, 0.4f, white);
    set_last_material(s, MAT_FABRIC);
    add_wall(s, 0.6f, 0.68f, -4.2f, 0.7f, 0.2f, 0.4f, white);
    set_last_material(s, MAT_FABRIC);
    add_wall(s, 0, 1.2f, -4.5f, 3.6f, 1.6f, 0.12f, godard_red);
    set_last_material(s, MAT_FABRIC);
    add_wall(s, 0, 2.05f, -4.48f, 3.7f, 0.05f, 0.06f, gold);
    set_last_material(s, MAT_BRASS);

    add_wall(s, -2.5f, 0.3f, -3.8f, 0.6f, 0.6f, 0.6f, wood);
    add_wall(s, 2.5f, 0.3f, -3.8f, 0.6f, 0.6f, 0.6f, wood);
    // Bedside lamps — cylinder base + shaft + cone shade + glow
    Color brass_lamp = PAL_BRASS;
    Color cream_shade = {235,228,215,255};
    // Left lamp
    add_cylinder(s, -2.5f, 0.62f, -3.8f, 0.08f, 0.04f, brass_lamp);  // base
    add_cylinder(s, -2.5f, 0.74f, -3.8f, 0.03f, 0.2f, brass_lamp);   // shaft
    add_cone(s, -2.5f, 0.90f, -3.8f, 0.18f, 0.14f, cream_shade);     // cone shade
    add_light_panel(s, -2.5f, 0.85f, -3.8f, 0.2f, 0.35f, 0.2f, warm_light);
    // Right lamp
    add_cylinder(s, 2.5f, 0.62f, -3.8f, 0.08f, 0.04f, brass_lamp);
    add_cylinder(s, 2.5f, 0.74f, -3.8f, 0.03f, 0.2f, brass_lamp);
    add_cone(s, 2.5f, 0.90f, -3.8f, 0.18f, 0.14f, cream_shade);      // cone shade
    add_light_panel(s, 2.5f, 0.85f, -3.8f, 0.2f, 0.35f, 0.2f, warm_light);

    // Desk
    add_wall(s, 5.0f, 0.4f, 0, 2.5f, 0.8f, 0.9f, wood);
    set_last_material(s, MAT_WOOD);
    add_wall(s, 5.0f, 0.82f, 0, 2.6f, 0.03f, 0.95f, gold);
    set_last_material(s, MAT_BRASS);
    // Desk chair — seat with 4 cylinder legs + back
    Color chair_c = {160,90,50,255};
    add_wall(s, 3.8f, 0.35f, 0, 0.5f, 0.04f, 0.5f, chair_c);          // seat (thin)
    set_last_material(s, MAT_LEATHER);
    add_cylinder(s, 3.57f, 0.165f, -0.22f, 0.025f, 0.3f, chair_c);     // leg FL
    add_cylinder(s, 4.03f, 0.165f, -0.22f, 0.025f, 0.3f, chair_c);     // leg FR
    add_cylinder(s, 3.57f, 0.165f, 0.22f, 0.025f, 0.3f, chair_c);      // leg BL
    add_cylinder(s, 4.03f, 0.165f, 0.22f, 0.025f, 0.3f, chair_c);      // leg BR
    add_wall(s, 3.8f, 0.75f, -0.22f, 0.5f, 0.55f, 0.08f, chair_c);    // back (angled via offset)
    set_last_material(s, MAT_LEATHER);
    add_wall(s, 5.75f, 1.8f, 0, 0.04f, 1.1f, 1.3f, (Color){200,205,210,180});
    add_wall(s, 5.78f, 1.8f, 0, 0.03f, 1.2f, 0.05f, gold);

    // Sofa — warm gray
    add_wall(s, -4.2f, 0.25f, 1.5f, 2.4f, 0.5f, 0.9f, warm_gray);
    set_last_material(s, MAT_LEATHER);
    add_wall(s, -4.2f, 0.6f, 1.95f, 2.4f, 0.6f, 0.15f, warm_gray);
    set_last_material(s, MAT_LEATHER);
    add_wall(s, -5.4f, 0.4f, 1.5f, 0.08f, 0.45f, 0.9f, warm_gray);
    add_wall(s, -3.0f, 0.4f, 1.5f, 0.08f, 0.45f, 0.9f, warm_gray);

    // Coffee table — flat top with 4 cylinder legs
    add_wall(s, -4.2f, 0.38f, 3.0f, 1.45f, 0.03f, 0.85f, gold);       // top surface
    set_last_material(s, MAT_BRASS);
    add_cylinder(s, -4.85f, 0.18f, 2.62f, 0.025f, 0.35f, wood);        // leg FL
    add_cylinder(s, -3.55f, 0.18f, 2.62f, 0.025f, 0.35f, wood);        // leg FR
    add_cylinder(s, -4.85f, 0.18f, 3.38f, 0.025f, 0.35f, wood);        // leg BL
    add_cylinder(s, -3.55f, 0.18f, 3.38f, 0.025f, 0.35f, wood);        // leg BR

    // Suitcase
    add_wall(s, 2.2f, 0.2f, 3.8f, 0.8f, 0.35f, 0.5f, (Color){150,100,60,255});
    add_wall(s, 2.05f, 0.32f, 4.02f, 0.18f, 0.13f, 0.02f, (Color){50,80,180,255});
    add_wall(s, 2.3f, 0.28f, 4.02f, 0.13f, 0.1f, 0.02f, godard_red);

    // Balcony door
    add_wall(s, -5.7f, 1.5f, -1, 0.08f, 2.8f, 1.7f, (Color){190,205,225,100});
    set_last_material(s, MAT_GLASS);
    add_wall(s, -5.75f, 2.95f, -1, 0.06f, 0.12f, 1.85f, gold);
    set_last_material(s, MAT_BRASS);
    add_wall(s, -5.75f, 0.05f, -1, 0.06f, 0.08f, 1.85f, gold);
    set_last_material(s, MAT_BRASS);
    add_wall(s, -5.75f, 1.5f, -1.88f, 0.06f, 2.85f, 0.06f, gold);
    set_last_material(s, MAT_BRASS);
    add_wall(s, -5.75f, 1.5f, -0.12f, 0.06f, 2.85f, 0.06f, gold);
    set_last_material(s, MAT_BRASS);
    add_wall(s, -5.6f, 1.8f, -2.2f, 0.04f, 3.0f, 0.5f, cream);
    add_wall(s, -5.6f, 1.8f, 0.2f, 0.04f, 3.0f, 0.5f, cream);

    add_light_panel(s, 0, rh-0.12f, 0, 0.8f, 0.04f, 0.8f, warm_light);

    // Life details — scaled up 2x, high contrast (Rodkin)
    // Water ring on coffee table — now visible
    add_wall(s, -3.8f, 0.52f, 1.4f, 0.5f, 0.04f, 0.36f, (Color){200,210,220,200});
    // Coffee cup — bigger, white
    add_wall(s, 4.5f, 0.86f, 0.3f, 0.16f, 0.2f, 0.16f, white);
    // Book — bright Godard red, 2x size
    add_wall(s, 3.8f, 0.95f, -0.22f, 1.0f, 0.5f, 0.2f, godard_red);

    // Bathrobe — vivid yellow, hung on wall, scaled up (Rodkin)
    add_wall(s, 5.75f, 1.6f, -3.5f, 0.1f, 1.8f, 0.8f, (Color){240,220,50,255});
    add_wall(s, 5.78f, 2.6f, -3.5f, 0.06f, 0.1f, 1.2f, gold);

    // Recessed panels — back wall behind bed (paneled wall effect)
    add_recessed_panel(s, -2.5f, 2.5f, -rd/2+0.16f, 2.0f, 1.4f, 0.1f, wall_gold);
    add_recessed_panel(s, 2.5f, 2.5f, -rd/2+0.16f, 2.0f, 1.4f, 0.1f, wall_gold);
    add_recessed_panel(s, 0.0f, 2.8f, -rd/2+0.16f, 1.5f, 0.8f, 0.1f, wall_gold);
    // Recessed panel — wall opposite balcony, above desk
    add_recessed_panel(s, 5.0f, 2.2f, rd/2-0.16f, 3.0f, 1.8f, 0.1f, wall_gold);

    // LIGHT SHAFT — cool moonlight through balcony door onto floor
    // Godard contrast: cold blue shaft cutting through warm golden room
    add_wall(s, -4.5f, 0.02f, -1.0f, 2.0f, 0.02f, 1.5f, (Color){120,150,220,100});
    // Night sky visible through balcony glass — deep cold blue
    add_wall(s, -5.95f, 1.5f, -1, 0.02f, 2.8f, 1.7f, (Color){15,25,60,255});

    // Multi-step interactions: lamp=2, drawers=3, candles=3, ashtray=1, bed=2
    add_object(s, -2.5f, 1.2f, -3.8f, "lamp", (Color){240,210,120,255}, 2);
    add_object(s, 5.0f, 1.0f, 0, "drawers", (Color){200,155,90,255}, 3);
    add_object(s, -4.2f, 0.6f, 3.0f, "candles", (Color){240,220,160,255}, 3);
    add_object(s, 5.5f, 0.6f, -2, "ashtray", (Color){180,170,155,255}, 1);
    add_object(s, 0, 0.8f, -3.5f, "bed", white, 2);

    // Phone on desk — mobile face-down
    add_wall(s, 5.2f, 0.85f, 0.15f, 0.14f, 0.05f, 0.08f, (Color){35,35,38,255});

    // Wall clock — above desk, on the back wall. Tells time without words.
    // Circle face (flat disc on wall)
    add_wall(s, 5.0f, 2.4f, rd/2-0.12f, 0.5f, 0.5f, 0.04f, (Color){235,232,226,255});  // face
    add_wall(s, 5.0f, 2.4f, rd/2-0.13f, 0.52f, 0.52f, 0.02f, gold);                      // rim
    // Hour hand — short thick
    add_wall(s, 5.0f, 2.52f, rd/2-0.14f, 0.04f, 0.16f, 0.02f, (Color){40,38,35,255});
    // Minute hand — longer, thinner
    add_wall(s, 5.08f, 2.48f, rd/2-0.14f, 0.12f, 0.025f, 0.02f, (Color){40,38,35,255});

    // Environmental story objects — visual only, not interactable
    // Boarding pass on desk — white rectangle + blue airline stripe (tells "06:00" departure)
    add_wall(s, 4.5f, 0.85f, -0.3f, 0.4f, 0.03f, 0.2f, (Color){245,242,235,255});  // paper
    add_wall(s, 4.5f, 0.86f, -0.35f, 0.4f, 0.03f, 0.06f, (Color){50,80,180,255});  // blue stripe

    // Pilot's logbook near suitcase — thick brown leather rectangle
    add_wall(s, 1.8f, 0.22f, 4.1f, 0.35f, 0.08f, 0.25f, (Color){95,60,30,255});     // cover
    add_wall(s, 1.8f, 0.27f, 4.1f, 0.33f, 0.01f, 0.23f, (Color){235,230,215,255});  // pages edge

    // Face-down photograph on nightstand — NOT interactable. You can't flip it.
    add_wall(s, -2.5f, 0.62f, -3.5f, 0.2f, 0.01f, 0.15f, (Color){240,238,230,255}); // photo back

    // En-suite bathroom door — right wall, near back
    add_wall(s, rw/2-0.12f, 1.3f, -3.0f, 0.12f, 2.6f, 1.0f, (Color){210,207,202,255});  // door
    add_wall(s, rw/2-0.12f, 2.75f, -3.0f, 0.12f, 0.12f, 1.1f, gold);                    // frame top
    add_object(s, rw/2-0.3f, 1.2f, -3.0f, "bathroom", (Color){200,195,188,255}, 1);

    // WARDROBE — dark wood, against left wall near sofa
    // Tall rectangle, mysterious, hides a secret passage
    Color wardrobe_c = {75, 55, 32, 255};  // dark walnut
    add_wall(s, -5.5f, 1.2f, 3.5f, 0.8f, 2.4f, 0.6f, wardrobe_c);     // wardrobe body
    set_last_material(s, MAT_WOOD);
    add_wall(s, -5.5f, 1.2f, 3.18f, 0.76f, 2.3f, 0.02f, (Color){65,48,28,255}); // front panel (darker)
    set_last_material(s, MAT_WOOD);
    add_wall(s, -5.5f, 2.0f, 3.18f, 0.04f, 0.08f, 0.04f, gold);       // handle left
    set_last_material(s, MAT_BRASS);
    add_wall(s, -5.5f, 1.6f, 3.18f, 0.04f, 0.08f, 0.04f, gold);       // handle right
    set_last_material(s, MAT_BRASS);
    add_object(s, -5.5f, 1.2f, 3.2f, "wardrobe", (Color){140,120,80,255}, 1);

    // ============================================================
    // ARCHITECTURAL DETAIL — picture frames, bookshelf, door frames,
    // baseboards, crown moldings
    // ============================================================

    // Picture frames above bed — abstract hotel art (Godard canvases)
    // Offset 0.02f from back wall to prevent z-fighting
    add_picture_frame(s, -1.5f, 2.8f, -rd/2+0.18f, 0.7f, 0.5f, dark_wood, godard_red);
    add_picture_frame(s, 1.5f, 2.8f, -rd/2+0.18f, 0.7f, 0.5f, dark_wood, (Color){40,65,160,255});

    // Bookshelf on wall near desk — 3 rows
    add_bookshelf(s, rw/2-0.2f, 1.6f, 0.02f, 1.2f, 1.2f, 3, dark_wood);

    // Door frame around bathroom door
    add_door_frame(s, rw/2-0.12f, 1.3f, -3.0f, 1.0f, 2.6f, 0.14f, dark_wood);

    // Door frame around balcony door
    add_door_frame(s, -rw/2+0.06f, 1.5f, -1.0f, 1.7f, 2.8f, 0.12f, dark_wood);

    // Baseboards — all 4 walls at floor level
    // Back wall (along X)
    add_baseboard(s, 0, 0, -rd/2+0.08f, rw, 0, dark_wood);
    // Front wall (along X)
    add_baseboard(s, 0, 0, rd/2-0.08f, rw, 0, dark_wood);
    // Left wall (along Z)
    add_baseboard(s, -rw/2+0.08f, 0, 0, rd, 1.0f, dark_wood);
    // Right wall (along Z)
    add_baseboard(s, rw/2-0.08f, 0, 0, rd, 1.0f, dark_wood);

    // Crown moldings — all 4 wall-ceiling joins
    // Back wall (along X)
    add_crown_molding(s, 0, rh, -rd/2+0.08f, rw, 0, gold);
    // Front wall (along X)
    add_crown_molding(s, 0, rh, rd/2-0.08f, rw, 0, gold);
    // Left wall (along Z)
    add_crown_molding(s, -rw/2+0.08f, rh, 0, rd, 1.0f, gold);
    // Right wall (along Z)
    add_crown_molding(s, rw/2-0.08f, rh, 0, rd, 1.0f, gold);

    // BLOCKING WALLS — extra collision barriers to prevent room escape
    // Corner reinforcement: thin invisible walls at all four corners
    // These ensure diagonal movement can't clip through wall intersections
    // NW corner
    add_wall(s, -rw/2+0.1f, rh/2, -rd/2+0.1f, 0.5f, rh, 0.5f, wall_gold);
    // NE corner
    add_wall(s, rw/2-0.1f, rh/2, -rd/2+0.1f, 0.5f, rh, 0.5f, wall_gold);
    // SW corner
    add_wall(s, -rw/2+0.1f, rh/2, rd/2-0.1f, 0.5f, rh, 0.5f, wall_gold);
    // SE corner
    add_wall(s, rw/2-0.1f, rh/2, rd/2-0.1f, 0.5f, rh, 0.5f, wall_gold);

    s->spawn = (Vector3){0, 1.6f, 4};
    s->has_exit = false;

    // ── THE MOTIF: red book ──
    // On the desk, half-hidden under the boarding pass. Brought from home.
    add_red_book(s, 4.8f, 0.86f, 0.1f, 0.0f);
}

void build_balcony(Scene *s) {
    *s = (Scene){0};
    // OBSERVATION DECK — open to the void, Earth below
    // Parisian balcony furniture, infinite space outside
    s->surface = SURFACE_MARBLE;

    s->fog_color = (Color){2, 3, 8, 255};  // deep space
    s->fog_density = 0.001f;  // almost no fog — vacuum clarity

    Color stone = PAL_MARBLE_A;
    Color railing = PAL_MID;
    Color gold = PAL_BRASS;
    Color hull = PAL_HULL;

    // Floor + back wall (hull interior, but warm)
    add_wall(s, 0, -0.1f, 0, 5, 0.2f, 3, stone);
    set_last_material(s, MAT_MARBLE);
    add_wall(s, 0, 2, 1.5f, 5, 4, 0.3f, (Color){200,175,120,255});
    set_last_material(s, MAT_WALLPAPER);
    add_wall(s, 0, 2.8f, 1.35f, 1.8f, 0.12f, 0.06f, gold);
    set_last_material(s, MAT_BRASS);
    add_wall(s, -0.95f, 1.5f, 1.35f, 0.06f, 2.8f, 0.06f, gold);
    set_last_material(s, MAT_BRASS);
    add_wall(s, 0.95f, 1.5f, 1.35f, 0.06f, 2.8f, 0.06f, gold);
    set_last_material(s, MAT_BRASS);

    // Railing — the only thing between you and the void
    add_wall(s, 0, 0.95f, -1.5f, 5, 0.06f, 0.08f, railing);
    set_last_material(s, MAT_BRASS);
    add_wall(s, 0, 0.5f, -1.5f, 5, 0.04f, 0.06f, railing);
    set_last_material(s, MAT_BRASS);
    add_wall(s, -2.5f, 0.95f, 0, 0.08f, 0.06f, 3, railing);
    set_last_material(s, MAT_BRASS);
    add_wall(s, 2.5f, 0.95f, 0, 0.08f, 0.06f, 3, railing);
    set_last_material(s, MAT_BRASS);

    // Side hull walls — the observation deck is a cutout in the station hull
    add_wall(s, -2.7f, 2, 0, 0.3f, 4, 3.2f, hull);
    add_wall(s, 2.7f, 2, 0, 0.3f, 4, 3.2f, hull);

    // Wine + chair — same luxury, absurd context
    add_wall(s, -1.5f, 0.45f, -0.5f, 0.45f, 0.04f, 0.45f, (Color){120,100,70,255});
    add_wall(s, -1.5f, 0.22f, -0.5f, 0.05f, 0.45f, 0.05f, (Color){100,85,60,255});
    add_wall(s, -1.5f, 0.49f, -0.5f, 0.05f, 0.12f, 0.05f, (Color){210,210,215,160});
    add_wall(s, -1.5f, 0.52f, -0.5f, 0.035f, 0.05f, 0.035f, (Color){140,35,45,200});
    add_wall(s, -1.5f, 0.22f, 0.2f, 0.45f, 0.44f, 0.45f, (Color){120,100,70,255});
    add_wall(s, -1.5f, 0.58f, 0.42f, 0.45f, 0.4f, 0.06f, (Color){120,100,70,255});

    // Second wine glass
    add_wall(s, -1.0f, 0.49f, -0.8f, 0.05f, 0.12f, 0.05f, (Color){210,210,215,160});
    add_wall(s, -1.0f, 0.52f, -0.8f, 0.035f, 0.05f, 0.035f, (Color){140,35,45,200});

    // Ashtray with cigarette on railing table
    add_cylinder(s, -1.5f, 0.49f, -0.3f, 0.12f, 0.04f, (Color){140,135,130,255});
    add_object(s, -1.5f, 0.55f, -0.3f, "cigarette", (Color){200,195,185,255}, 1);

    // ============================================================
    // EARTH — Rothko fields, not a ball.
    // Five layers: glass → atmosphere → planet surface → city lights → stars
    // The planet is a HORIZON LINE filling the lower view.
    // Two color fields meeting at an edge. Rothko. Not geometry.
    // ============================================================

    // LAYER 1: Glass panel — barely visible boundary between you and death
    add_wall(s, 0, 0.5f, -1.55f, 5, 1.0f, 0.02f, (Color){140, 180, 220, 15});
    set_last_material(s, MAT_GLASS);

    // LAYER 2: Atmosphere — wide thin band at the horizon
    // Three overlapping panels at different depths for glow bleed
    // BRIGHT — this is the hero visual, it must read at 480x300
    add_wall(s, 0, -0.8f, -18, 80, 0.8f, 0.1f, (Color){100, 170, 230, 220});
    add_wall(s, 0, -0.6f, -17, 90, 0.5f, 0.1f, (Color){140, 195, 240, 180});
    add_wall(s, 0, -0.4f, -16, 100, 0.25f, 0.1f, (Color){200, 225, 248, 140});

    // LAYER 3: Planet surface — dark fields below the atmosphere line
    // Near field (looking down)
    add_wall(s, 0, -3, -10, 60, 0.1f, 12, (Color){10, 18, 38, 255});
    // Mid field — ocean bulk
    add_wall(s, 0, -4, -20, 80, 0.1f, 15, (Color){12, 22, 45, 255});
    // Land masses — slightly green-shifted patches
    add_wall(s, -15, -3.5f, -14, 20, 0.1f, 8, (Color){14, 28, 25, 255});
    add_wall(s, 12, -3.2f, -18, 12, 0.1f, 6, (Color){16, 30, 28, 255});
    // Far field — fading to black
    add_wall(s, 0, -5, -35, 100, 0.1f, 20, (Color){8, 14, 30, 255});

    // LAYER 3b: City lights — BIG at 480x300 (Rodkin rule: visible or dead)
    add_wall(s, -8, -2.8f, -11, 3.0f, 0.1f, 2.0f, (Color){240, 200, 110, 120});
    add_wall(s, -7, -2.7f, -10.5f, 1.5f, 0.1f, 1.0f, (Color){255, 220, 130, 140});
    add_wall(s, 5, -3.0f, -13, 2.5f, 0.1f, 1.5f, (Color){240, 200, 110, 100});
    add_wall(s, 15, -3.3f, -16, 2.0f, 0.1f, 1.2f, (Color){240, 200, 110, 90});
    add_wall(s, -20, -3.5f, -17, 1.8f, 0.1f, 1.0f, (Color){255, 210, 120, 110});
    // Auckland — brighter cluster, recognizable
    add_wall(s, -6, -2.6f, -9.5f, 2.0f, 0.1f, 1.5f, (Color){255, 230, 140, 180});
    // Sky Tower beacon — single bright dot
    add_wall(s, -5.5f, -2.5f, -9, 0.6f, 0.2f, 0.6f, (Color){255, 245, 200, 220});

    // LAYER 4: Stars — dense field above atmosphere, multiple depths
    // Far field — dim, many, creates depth
    for (int i = 0; i < 40; i++) {
        float sx = -50.0f + (float)((i*41)%100);
        float sy = 1.0f + (float)((i*17)%20);
        float sz = -40.0f - (float)((i*13)%40);
        float size = 0.15f + (float)((i*7)%3) * 0.08f;
        add_wall(s, sx, sy, sz, size, size, size,
                 (Color){220,218,210,(unsigned char)(80+(i*19)%60)});
    }
    // Near field — bright, sparse, hero stars
    for (int i = 0; i < 15; i++) {
        float sx = -25.0f + (float)((i*37)%50);
        float sy = 3.0f + (float)((i*23)%12);
        float sz = -15.0f - (float)((i*31)%20);
        float size = 0.3f + (float)((i*11)%4) * 0.15f;
        // Warm-cool star color variation
        unsigned char sr = (i % 3 == 0) ? 255 : 230;
        unsigned char sg = (i % 3 == 0) ? 220 : 235;
        unsigned char sb = (i % 3 == 1) ? 255 : 225;
        add_wall(s, sx, sy, sz, size, size, size,
                 (Color){sr,sg,sb,(unsigned char)(180+(i*13)%75)});
    }

    // LAYER 5: Nebula hint — soft colored glow, Rothko field in the void
    // A faint warm rectangle above the atmosphere — like a distant cloud
    add_wall(s, 15, 8, -30, 20, 6, 0.08f, (Color){80, 50, 100, 40});
    // Cool counterpoint
    add_wall(s, -20, 6, -35, 15, 4, 0.08f, (Color){40, 70, 110, 28});

    // Milky Way band — very faint diagonal streak across the sky
    for (int i = 0; i < 8; i++) {
        float mx = -20.0f + i * 5.5f;
        float my = 4.0f + i * 1.2f;
        float mz = -28.0f - i * 2.0f;
        add_wall(s, mx, my, mz, 4.0f, 1.5f, 0.06f,
                 (Color){180, 175, 165, (unsigned char)(20 + (i*8)%20)});
    }

    tag_materials_by_color(s);

    s->spawn = (Vector3){0, 1.6f, 0.5f};
    s->has_exit = false;

    // ── THE MOTIF: red book ──
    // On the railing ledge, pages fluttering in nothing. No wind in vacuum.
    add_red_book(s, 1.8f, 0.98f, -1.4f, 15.0f);
}

void build_bathroom(Scene *s) {
    *s = (Scene){0};
    // BOUNDS: 5m x 4m, fully enclosed (4 walls + floor + ceiling)
    s->surface = SURFACE_MARBLE;

    Color concrete = PAL_CONCRETE;
    Color ceiling = PAL_WHITE;
    Color porcelain = PAL_WHITE;
    Color brass = PAL_BRASS;
    Color floor_concrete = {100,98,95,255};    // dark concrete floor
    Color mirror = {210,215,222,180};          // bluer, translucent
    Color terracotta = PAL_TERRACOTTA;

    float bw = 5, bd = 4, bh = 3;

    s->fog_color = PAL_FOG_CONCRETE;
    s->fog_density = 0.005f;

    // Floor — darker concrete
    add_wall(s, 0, -0.05f, 0, bw, 0.1f, bd, floor_concrete);
    set_last_material(s, MAT_TILE);
    // Drain — small flat dark cylinder on floor
    add_cylinder(s, 1.0f, 0.01f, 0.5f, 0.15f, 0.02f, (Color){60,58,55,255});

    // Ceiling — white plaster
    add_wall(s, 0, bh, 0, bw, 0.2f, bd, ceiling);

    // Walls — concrete
    add_wall(s, 0, bh/2, -bd/2, bw, bh, 0.2f, concrete);       // back wall
    set_last_material(s, MAT_CONCRETE);
    add_wall(s, 0, bh/2, bd/2, bw, bh, 0.2f, concrete);        // front wall (entrance)
    set_last_material(s, MAT_CONCRETE);
    add_wall(s, -bw/2, bh/2, 0, 0.2f, bh, bd, concrete);       // left wall
    set_last_material(s, MAT_CONCRETE);
    add_wall(s, bw/2, bh/2, 0, 0.2f, bh, bd, concrete);        // right wall
    set_last_material(s, MAT_CONCRETE);

    // Bathtub — large freestanding, with curved half-cylinder ends and brass faucet
    add_wall(s, -1.2f, 0.25f, -1.0f, 1.8f, 0.5f, 0.9f, porcelain);  // tub body
    set_last_material(s, MAT_TILE);
    // Raised edges
    add_wall(s, -1.2f, 0.55f, -1.45f, 1.8f, 0.1f, 0.06f, porcelain); // back edge
    add_wall(s, -1.2f, 0.55f, -0.55f, 1.8f, 0.1f, 0.06f, porcelain); // front edge
    add_wall(s, -2.1f, 0.55f, -1.0f, 0.06f, 0.1f, 0.9f, porcelain);  // left edge
    add_wall(s, -0.3f, 0.55f, -1.0f, 0.06f, 0.1f, 0.9f, porcelain);  // right edge
    // Curved ends — half-cylinders at each end of the tub
    add_cylinder(s, -2.1f, 0.3f, -1.0f, 0.5f, 0.5f, porcelain);      // left end
    add_cylinder(s, -0.3f, 0.3f, -1.0f, 0.5f, 0.5f, porcelain);      // right end
    // Brass faucet — on back wall above tub
    add_cylinder(s, -1.2f, 0.75f, -1.42f, 0.06f, 0.15f, brass);      // faucet pipe
    set_last_material(s, MAT_BRASS);
    add_cylinder(s, -1.2f, 0.85f, -1.38f, 0.04f, 0.06f, brass);      // faucet spout
    set_last_material(s, MAT_BRASS);

    // Sink — wall-mounted slab on right wall
    add_wall(s, 2.2f, 0.85f, 0.5f, 0.8f, 0.06f, 0.5f, porcelain);   // sink basin
    set_last_material(s, MAT_TILE);
    add_wall(s, 2.35f, 0.85f, 0.5f, 0.06f, 0.5f, 0.5f, porcelain);  // wall bracket
    // Brass tap knobs — sphere shapes
    add_sphere(s, 2.0f, 0.95f, 0.5f, 0.08f, brass);
    set_last_material(s, MAT_BRASS);
    add_sphere(s, 1.9f, 0.95f, 0.5f, 0.08f, brass);
    set_last_material(s, MAT_BRASS);

    // Mirror — large rectangle above sink
    add_wall(s, 2.38f, 1.7f, 0.5f, 0.04f, 1.0f, 0.8f, mirror);
    set_last_material(s, MAT_GLASS);

    // Toilet — simple block
    add_wall(s, 2.0f, 0.25f, -1.2f, 0.5f, 0.5f, 0.4f, porcelain);   // base
    add_wall(s, 2.0f, 0.55f, -1.35f, 0.5f, 0.6f, 0.08f, porcelain);  // tank

    // Terracotta towel — ONE accent, hung on left wall
    add_wall(s, -2.38f, 1.4f, 0.0f, 0.06f, 0.9f, 0.5f, terracotta);
    set_last_material(s, MAT_FABRIC);
    // Towel bar
    add_wall(s, -2.4f, 1.9f, 0.0f, 0.04f, 0.04f, 0.7f, brass);
    set_last_material(s, MAT_BRASS);

    // Toiletries bag on counter near sink
    add_wall(s, 1.8f, 0.92f, 0.8f, 0.25f, 0.15f, 0.12f, (Color){60,55,50,255});
    add_wall(s, 1.8f, 1.0f, 0.8f, 0.25f, 0.02f, 0.01f, (Color){178,155,107,255}); // brass zipper

    // Ando moment — horizontal slot window near ceiling on back wall
    // Narrow opening letting light pour in
    add_light_panel(s, 0, bh-0.4f, -bd/2+0.12f, 3.5f, 0.3f, 0.06f,
                    (Color){245,242,235,200});
    // Light shaft on floor from the slot window
    add_wall(s, 0, 0.02f, -0.5f, 3.0f, 0.02f, 1.5f, (Color){240,238,230,100});

    // Picture frame around mirror — wood frame wrapping existing mirror
    add_picture_frame(s, 2.40f, 1.7f, 0.5f, 0.8f, 1.0f, brass, (Color){210,215,222,180});

    // Door frame around bathroom entrance
    add_door_frame(s, 0, 1.3f, bd/2-0.1f, 1.0f, 2.6f, 0.12f, brass);

    // Baseboards — all 4 walls
    add_baseboard(s, 0, 0, -bd/2+0.08f, bw, 0, (Color){158,155,150,255});
    add_baseboard(s, 0, 0, bd/2-0.08f, bw, 0, (Color){158,155,150,255});
    add_baseboard(s, -bw/2+0.08f, 0, 0, bd, 1.0f, (Color){158,155,150,255});
    add_baseboard(s, bw/2-0.08f, 0, 0, bd, 1.0f, (Color){158,155,150,255});

    // Interact objects
    add_object(s, 2.0f, 0.95f, 0.5f, "tap", (Color){200,205,210,255}, 1);
    add_object(s, 2.38f, 1.7f, 0.5f, "mirror", (Color){210,215,220,255}, 1);

    // Door interact object — return to room
    add_object(s, 0, 1.2f, bd/2-0.3f, "door", (Color){200,195,188,255}, 1);

    s->spawn = (Vector3){0, 1.6f, 1.0f};
    s->exit_pos = (Vector3){0, 1.6f, bd/2-0.2f};
    s->has_exit = false;  // exit via interact object
}

void build_stairwell(Scene *s) {
    *s = (Scene){0};
    // BOUNDS: 6m x 6m x 12m tall, fully enclosed (4 walls + floor + ceiling)
    s->surface = SURFACE_MARBLE;

    Color concrete = {175,172,168,255};      // rougher concrete
    Color metal = {140,138,135,255};         // metal handrail
    Color step_color = {185,182,178,255};    // step surface
    Color fire_ext = {200,50,40,255};        // ONE accent — fire extinguisher

    float sw = 6, sd = 6, sh = 12;

    s->fog_color = (Color){160,158,155,255};   // darker — more contrast with step_color
    s->fog_density = 0.008f;

    // Floor
    add_wall(s, 0, -0.05f, 0, sw, 0.1f, sd, concrete);
    set_last_material(s, MAT_CONCRETE);

    // Walls — tall concrete
    add_wall(s, 0, sh/2, -sd/2, sw, sh, 0.3f, concrete);       // back
    set_last_material(s, MAT_CONCRETE);
    add_wall(s, 0, sh/2, sd/2, sw, sh, 0.3f, concrete);        // front
    set_last_material(s, MAT_CONCRETE);
    add_wall(s, -sw/2, sh/2, 0, 0.3f, sh, sd, concrete);       // left
    set_last_material(s, MAT_CONCRETE);
    add_wall(s, sw/2, sh/2, 0, 0.3f, sh, sd, concrete);        // right
    set_last_material(s, MAT_CONCRETE);

    // Ceiling
    add_wall(s, 0, sh, 0, sw, 0.2f, sd, (Color){195,192,188,255});
    set_last_material(s, MAT_CONCRETE);

    // Skylight at top — bright light panel
    add_light_panel(s, 0, sh-0.1f, 0, 2.5f, 0.05f, 2.5f, (Color){245,242,235,220});

    // Stairs — zigzag pattern, 4 flights
    // Flight 1: back wall, going up left to right
    for (int i = 0; i < 6; i++) {
        float sx = -2.0f + i * 0.6f;
        float sy = 0.15f + i * 0.3f;
        add_wall(s, sx, sy, -2.0f, 0.55f, 0.3f, 1.2f, step_color);
    }
    // Landing 1
    add_wall(s, 2.0f, 1.8f, -1.5f, 1.5f, 0.2f, 2.0f, step_color);
    // Handrail flight 1
    add_wall(s, -2.0f, 1.1f, -1.4f, 3.5f, 0.06f, 0.06f, metal);
    set_last_material(s, MAT_BRASS);

    // Flight 2: right wall, going up right to left
    for (int i = 0; i < 6; i++) {
        float sx = 2.0f - i * 0.6f;
        float sy = 2.0f + i * 0.3f;
        add_wall(s, sx, sy, 1.5f, 0.55f, 0.3f, 1.2f, step_color);
    }
    // Landing 2
    add_wall(s, -2.0f, 3.8f, 1.5f, 1.5f, 0.2f, 2.0f, step_color);
    // Handrail flight 2
    add_wall(s, 0, 3.1f, 2.1f, 3.5f, 0.06f, 0.06f, metal);
    set_last_material(s, MAT_BRASS);

    // Flight 3: back again
    for (int i = 0; i < 6; i++) {
        float sx = -2.0f + i * 0.6f;
        float sy = 4.0f + i * 0.3f;
        add_wall(s, sx, sy, -2.0f, 0.55f, 0.3f, 1.2f, step_color);
    }
    // Landing 3
    add_wall(s, 2.0f, 5.8f, -1.5f, 1.5f, 0.2f, 2.0f, step_color);

    // Flight 4: back to left
    for (int i = 0; i < 6; i++) {
        float sx = 2.0f - i * 0.6f;
        float sy = 6.0f + i * 0.3f;
        add_wall(s, sx, sy, 1.5f, 0.55f, 0.3f, 1.2f, step_color);
    }
    // Landing 4 (top)
    add_wall(s, -2.0f, 7.8f, 1.5f, 1.5f, 0.2f, 2.0f, step_color);

    // Handrail supports — thin vertical cylinders on landings
    add_cylinder(s, 1.2f, 1.0f, -0.5f, 0.06f, 1.5f, metal);
    set_last_material(s, MAT_BRASS);
    add_cylinder(s, -1.2f, 3.0f, 2.5f, 0.06f, 1.5f, metal);
    set_last_material(s, MAT_BRASS);
    add_cylinder(s, 1.2f, 5.0f, -0.5f, 0.06f, 1.5f, metal);
    set_last_material(s, MAT_BRASS);
    add_cylinder(s, -1.2f, 7.0f, 2.5f, 0.06f, 1.5f, metal);
    set_last_material(s, MAT_BRASS);

    // Fire extinguisher — ONE accent, on the left wall
    add_wall(s, -sw/2+0.15f, 1.2f, 0, 0.2f, 0.5f, 0.15f, fire_ext);
    add_wall(s, -sw/2+0.12f, 1.55f, 0, 0.08f, 0.15f, 0.06f, (Color){50,50,50,255});  // nozzle

    // Graffiti — small blue paint rectangle on left wall at odd height
    add_wall(s, -sw/2+0.12f, 3.2f, -1.5f, 0.04f, 0.4f, 0.6f, (Color){80,120,180,255});

    // Light shaft from skylight falling down center
    add_wall(s, 0, 0.02f, 0, 2.0f, 0.02f, 2.0f, (Color){240,238,232,80});

    // Two exit points: lower landing goes to hallway, upper goes to roof
    // exit_pos is the lower exit (hallway); roof exit checked by y-position in main.c
    s->spawn = (Vector3){0, 1.6f, 2.0f};
    s->exit_pos = (Vector3){0, 1.6f, -2.5f};
    s->has_exit = true;
}

void build_roof(Scene *s) {
    *s = (Scene){0};
    // BOUNDS: 20m x 20m, open-air (parapet walls 0.8m high, no ceiling)
    s->surface = SURFACE_MARBLE;

    Color concrete = {175,172,168,255};
    Color concrete_dark = {155,152,148,255};
    Color metal = {140,138,135,255};
    Color tower = {70,75,95,255};
    Color bldg_near = {60,65,85,255};
    Color bldg_mid = {45,50,72,255};
    Color bldg_far = {35,40,62,255};
    Color window = {240,200,110,140};

    s->fog_color = (Color){20,25,45,255};  // night blue — minimal fog
    s->fog_density = 0.002f;

    // Concrete floor — 20x20m
    add_wall(s, 0, -0.1f, 0, 20, 0.2f, 20, concrete);
    set_last_material(s, MAT_CONCRETE);

    // Parapet walls — 0.8m high around edges
    add_wall(s, 0, 0.4f, -10, 20, 0.8f, 0.3f, concrete);   // back
    set_last_material(s, MAT_CONCRETE);
    add_wall(s, 0, 0.4f, 10, 20, 0.8f, 0.3f, concrete);    // front
    set_last_material(s, MAT_CONCRETE);
    add_wall(s, -10, 0.4f, 0, 0.3f, 0.8f, 20, concrete);   // left
    set_last_material(s, MAT_CONCRETE);
    add_wall(s, 10, 0.4f, 0, 0.3f, 0.8f, 20, concrete);    // right
    set_last_material(s, MAT_CONCRETE);

    // Water tank / AC unit — blocky industrial in corner
    add_wall(s, 7, 1.2f, 7, 3, 2.4f, 2.5f, concrete_dark);
    add_wall(s, 7, 2.5f, 7, 3.2f, 0.1f, 2.7f, concrete);   // lid
    // Smaller AC unit
    add_wall(s, 7, 0.6f, 4, 1.5f, 1.2f, 1.2f, concrete_dark);
    add_wall(s, 7, 1.3f, 4, 1.6f, 0.08f, 1.3f, metal);      // grate top

    // Metal pipes/vents
    add_cylinder(s, -6, 0.8f, 6, 0.3f, 1.6f, metal);
    add_cylinder(s, -7, 0.5f, 7, 0.2f, 1.0f, metal);
    add_cylinder(s, 5, 0.6f, -7, 0.25f, 1.2f, metal);

    // Stairwell exit structure — small concrete housing
    add_wall(s, 0, 1.5f, 8, 2.5f, 3, 2, concrete);
    // Door opening
    add_wall(s, 0, 1.3f, 7, 1.0f, 2.6f, 0.12f, (Color){120,100,75,255}); // rusted door
    set_last_material(s, MAT_BRASS);

    // Distant cityscape — same buildings as balcony but from higher vantage
    add_wall(s, -12, 2, -30, 4, 8, 2, bldg_near);
    add_wall(s, -8, 3.5f, -28, 3, 11, 2, bldg_near);
    add_wall(s, -16, 1, -32, 5, 6, 2, bldg_near);
    add_wall(s, 16, 2.5f, -25, 3.5f, 9, 2, bldg_near);
    add_wall(s, 5, 4, -50, 6, 12, 3, bldg_mid);
    add_wall(s, -5, 3, -55, 8, 10, 3, bldg_mid);
    add_wall(s, 15, 2, -45, 5, 8, 3, bldg_mid);
    add_wall(s, -20, 3.5f, -48, 4, 11, 3, bldg_mid);
    add_wall(s, 0, 1, -80, 25, 6, 4, bldg_far);
    add_wall(s, -25, 2, -85, 15, 8, 4, bldg_far);
    add_wall(s, 22, 1.5f, -82, 12, 7, 4, bldg_far);

    // Building windows
    for (int r = 0; r < 4; r++)
        for (int c = 0; c < 2; c++)
            if ((r+c)%2==0) {
                add_wall(s, -8.5f+c*1.2f, -0.5f+r*2.0f, -26.9f, 0.4f, 0.7f, 0.04f, window);
                set_last_material(s, MAT_GLASS);
            }
    for (int r = 0; r < 4; r++)
        for (int c = 0; c < 3; c++)
            if ((r*3+c)%2==0) {
                add_wall(s, 3.5f+c*1.5f, -0.5f+r*2.5f, -48.5f, 0.45f, 0.7f, 0.04f, window);
                set_last_material(s, MAT_GLASS);
            }

    // Sky Tower — from higher vantage on the roof
    add_skytower(s, 8, -2, -90, 4.0f, tower);
    add_light_panel(s, 8, 36.0f, -90, 0.4f, 0.6f, 0.4f, (Color){255,245,200,180});

    // Stars scattered in the sky
    for (int i = 0; i < 20; i++) {
        float sx = -45+(i*41)%90, sy = 20+(i*17)%18, sz = -95-(i*13)%25;
        add_wall(s, sx, sy, sz, 0.12f, 0.12f, 0.12f,
                 (Color){240,235,225,(unsigned char)(120+(i*19)%80)});
    }

    // Exit — goes to STATE_BALCONY (back to the room eventually)
    s->spawn = (Vector3){0, 8.0f, 6};
    s->exit_pos = (Vector3){0, 8.0f, -8};
    s->has_exit = true;
}

void build_elevator(Scene *s) {
    *s = (Scene){0};
    // BOUNDS: 2m x 2m, fully enclosed (4 walls + floor + ceiling)
    s->surface = SURFACE_MARBLE;

    Color brass = PAL_BRASS;
    Color floor_marble = PAL_DARK;
    Color mirror = {210, 215, 220, 180};
    Color warm_panel = PAL_LIGHT_WARM;
    Color btn_dark = {140, 125, 90, 255};
    Color btn_lit = {240, 220, 140, 255};

    float ew = 2, ed = 2, eh = 2.5f;

    s->fog_color = PAL_FOG_BRASS;
    s->fog_density = 0.002f;

    // Floor — dark marble
    add_wall(s, 0, -0.05f, 0, ew, 0.1f, ed, floor_marble);
    set_last_material(s, MAT_MARBLE);

    // Ceiling
    add_wall(s, 0, eh, 0, ew, 0.1f, ed, brass);
    set_last_material(s, MAT_BRASS);

    // Ceiling light panel — single warm panel
    add_light_panel(s, 0, eh - 0.08f, 0, 1.0f, 0.04f, 1.0f, warm_panel);

    // Walls — brass, except left wall is GLASS
    add_wall(s, 0, eh/2, -ed/2, ew, eh, 0.08f, brass);           // back wall
    set_last_material(s, MAT_BRASS);
    add_wall(s, 0, eh/2, ed/2, ew, eh, 0.08f, brass);            // front (doors, closed)
    set_last_material(s, MAT_BRASS);
    add_wall(s, ew/2, eh/2, 0, 0.08f, eh, ed, brass);            // right wall
    set_last_material(s, MAT_BRASS);

    // LEFT WALL — glass viewport. The player SEES Auckland drop away.
    // Thin brass frame around a transparent panel
    add_wall(s, -ew/2, eh/2, 0, 0.02f, eh, ed, (Color){100, 130, 160, 20});  // glass
    set_last_material(s, MAT_GLASS);
    add_wall(s, -ew/2, eh-0.02f, 0, 0.04f, 0.06f, ed, brass);   // top frame
    set_last_material(s, MAT_BRASS);
    add_wall(s, -ew/2, 0.02f, 0, 0.04f, 0.06f, ed, brass);      // bottom frame
    set_last_material(s, MAT_BRASS);
    add_wall(s, -ew/2, eh/2, -ed/2, 0.04f, eh, 0.06f, brass);   // left frame
    set_last_material(s, MAT_BRASS);
    add_wall(s, -ew/2, eh/2, ed/2, 0.04f, eh, 0.06f, brass);    // right frame
    set_last_material(s, MAT_BRASS);

    // Mirror on back wall
    add_wall(s, 0, 1.4f, -ed/2 + 0.06f, 1.2f, 1.4f, 0.03f, mirror);
    set_last_material(s, MAT_GLASS);

    // Button panel on right wall
    float panel_x = ew/2 - 0.06f;
    float panel_z = 0.3f;
    add_wall(s, panel_x, 1.2f, panel_z, 0.04f, 0.7f, 0.2f, (Color){150, 135, 100, 255});
    set_last_material(s, MAT_BRASS);
    for (int i = 0; i < 4; i++) {
        float by = 0.95f + i * 0.15f;
        Color bc = (i == 1) ? btn_lit : btn_dark;
        add_wall(s, panel_x - 0.01f, by, panel_z, 0.03f, 0.08f, 0.08f, bc);
    }

    // Carpet runner on marble floor — warm against cold stone
    add_wall(s, 0, 0.01f, 0, 0.8f, 0.02f, 1.4f, (Color){120, 45, 38, 255});
    set_last_material(s, MAT_CARPET);

    // Brass door seam on front wall
    add_wall(s, 0, eh/2, ed/2 - 0.02f, 0.04f, eh, 0.02f, (Color){30, 28, 25, 255});

    // Floor indicator above door — glowing number panel
    add_wall(s, 0, eh - 0.3f, ed/2 - 0.06f, 0.5f, 0.25f, 0.02f, (Color){60, 55, 45, 255});
    set_last_material(s, MAT_BRASS);
    // Indicator glow — warm amber "G" (ground floor lit)
    add_light_panel(s, 0, eh - 0.3f, ed/2 - 0.04f, 0.35f, 0.15f, 0.01f, (Color){240, 200, 120, 180});

    // Safety notice on right wall — reads at 480x300 as a cream rectangle
    add_wall(s, ew/2 - 0.06f, 1.8f, -0.3f, 0.02f, 0.4f, 0.3f, (Color){235, 232, 228, 255});
    set_last_material(s, MAT_WALLPAPER);

    // Lower wall panels — dark wood wainscoting (Hotel Chevalier warmth)
    add_wall(s, 0, 0.45f, -ed/2 + 0.06f, ew - 0.2f, 0.9f, 0.03f, PAL_WOOD_DARK);
    set_last_material(s, MAT_WOOD);
    add_wall(s, ew/2 - 0.07f, 0.45f, 0, 0.03f, 0.9f, ed - 0.2f, PAL_WOOD_DARK);
    set_last_material(s, MAT_WOOD);

    // Leather pad on handrail — tactile detail
    add_wall(s, 0, 0.92f, -ed/2 + 0.09f, 1.3f, 0.05f, 0.04f, PAL_LEATHER);
    set_last_material(s, MAT_LEATHER);

    // Handrail on back wall — brass bar
    add_wall(s, 0, 0.9f, -ed/2 + 0.08f, 1.4f, 0.03f, 0.03f, brass);
    set_last_material(s, MAT_BRASS);

    // ============================================================
    // EXTERIOR — Auckland visible through the glass, dropping away
    // These walls are STATIC; the camera rises, they stay put.
    // The player watches the city shrink below them.
    // ============================================================

    // Sky Tower shaft — you're INSIDE it, rising past structural rings
    for (int i = 0; i < 20; i++) {
        float ry = -5.0f + i * 8.0f;
        add_wall(s, -3, ry, 0, 0.15f, 0.4f, 3, (Color){80, 85, 95, 200});  // ring segment
    }

    // Auckland city lights — far below, visible at start, shrinking
    Color city = {240, 200, 110, 100};
    Color bldg = {20, 24, 35, 255};
    // Building silhouettes (left of elevator shaft)
    add_wall(s, -8, -2, -5, 3, 6, 2, bldg);
    add_wall(s, -12, -1, -8, 4, 8, 2, (Color){18, 22, 32, 255});
    add_wall(s, -6, -3, 3, 5, 4, 2, (Color){22, 26, 38, 255});
    add_wall(s, -15, 0, 2, 3, 10, 2, bldg);
    // Windows on buildings — amber glow
    for (int b = 0; b < 4; b++) {
        float bx = -8.0f - b * 3.5f;
        for (int f = 0; f < 3; f++) {
            add_wall(s, bx, -3.0f + f * 2.0f, -5.5f, 0.8f, 0.6f, 0.04f, city);
        }
    }
    // Ground — road and sidewalk far below
    add_wall(s, -8, -6, 0, 30, 0.1f, 30, (Color){25, 28, 32, 255});
    // Streetlights
    for (int i = 0; i < 5; i++) {
        float lz = -10.0f + i * 5.0f;
        add_wall(s, -5, -4, lz, 0.3f, 0.3f, 0.3f, (Color){240, 210, 120, 140});
    }

    // Stars — visible once above the city (high Y)
    for (int i = 0; i < 15; i++) {
        float sx = -20.0f + (float)((i*31)%40);
        float sy = 60.0f + (float)((i*17)%40);
        float sz = -15.0f + (float)((i*13)%30);
        add_wall(s, sx, sy, sz, 0.2f, 0.2f, 0.2f,
                 (Color){240,235,225,(unsigned char)(120+(i*23)%100)});
    }

    tag_materials_by_color(s);

    s->spawn = (Vector3){0, 1.6f, 0};
    s->has_exit = false;

    // ── THE MOTIF: cigarette ──
    // Wedged in the panel gap. Someone rode this elevator before you.
    add_cylinder(s, 0.48f, 1.0f, -0.45f, 0.025f, 0.05f, (Color){230,225,215,180});

}

void build_elevator_space(Scene *s) {
    *s = (Scene){0};
    // SPACE ELEVATOR — same brass box, but you're riding between floors
    // of an orbital hotel. Through the glass: the lobby shrinks below,
    // station skeleton passes, Earth glows through hull gaps.
    s->surface = SURFACE_MARBLE;

    Color brass = PAL_BRASS;
    Color floor_marble = PAL_DARK;
    Color mirror = {210, 215, 220, 180};
    Color warm_panel = PAL_LIGHT_WARM;
    Color btn_dark = {140, 125, 90, 255};
    Color btn_lit = {240, 220, 140, 255};
    Color hull = PAL_HULL;
    Color hull_dark = {22, 24, 32, 255};
    Color cream = PAL_CREAM;

    float ew = 2, ed = 2, eh = 2.5f;

    s->fog_color = PAL_FOG_STATION;
    s->fog_density = 0.001f;

    // ============================================================
    // INTERIOR — same brass box as terrestrial, you recognize it
    // ============================================================

    // Floor — dark marble
    add_wall(s, 0, -0.05f, 0, ew, 0.1f, ed, floor_marble);
    set_last_material(s, MAT_MARBLE);

    // Ceiling
    add_wall(s, 0, eh, 0, ew, 0.1f, ed, brass);
    set_last_material(s, MAT_BRASS);

    // Ceiling light panel
    add_light_panel(s, 0, eh - 0.08f, 0, 1.0f, 0.04f, 1.0f, warm_panel);

    // Back wall — brass
    add_wall(s, 0, eh/2, -ed/2, ew, eh, 0.08f, brass);
    set_last_material(s, MAT_BRASS);
    // Front wall — doors (closed)
    add_wall(s, 0, eh/2, ed/2, ew, eh, 0.08f, brass);
    set_last_material(s, MAT_BRASS);
    // Right wall — brass
    add_wall(s, ew/2, eh/2, 0, 0.08f, eh, ed, brass);
    set_last_material(s, MAT_BRASS);

    // LEFT WALL — glass viewport. Now you see the station, not Auckland.
    add_wall(s, -ew/2, eh/2, 0, 0.02f, eh, ed, (Color){100, 130, 160, 20});
    set_last_material(s, MAT_GLASS);
    add_wall(s, -ew/2, eh-0.02f, 0, 0.04f, 0.06f, ed, brass);   // top frame
    set_last_material(s, MAT_BRASS);
    add_wall(s, -ew/2, 0.02f, 0, 0.04f, 0.06f, ed, brass);      // bottom frame
    set_last_material(s, MAT_BRASS);
    add_wall(s, -ew/2, eh/2, -ed/2, 0.04f, eh, 0.06f, brass);   // left frame
    set_last_material(s, MAT_BRASS);
    add_wall(s, -ew/2, eh/2, ed/2, 0.04f, eh, 0.06f, brass);    // right frame
    set_last_material(s, MAT_BRASS);

    // FLOOR — glass panel so you see the lobby dropping away
    add_wall(s, 0, -0.02f, 0, ew-0.2f, 0.01f, ed-0.2f, (Color){80, 110, 140, 15});
    set_last_material(s, MAT_GLASS);
    // Brass grid lines on glass floor
    add_wall(s, 0, -0.01f, 0, 0.03f, 0.02f, ed-0.2f, brass);
    set_last_decal(s);
    add_wall(s, 0, -0.01f, 0, ew-0.2f, 0.02f, 0.03f, brass);
    set_last_decal(s);

    // Mirror on back wall
    add_wall(s, 0, 1.4f, -ed/2 + 0.06f, 1.2f, 1.4f, 0.03f, mirror);
    set_last_material(s, MAT_GLASS);

    // Button panel — second button lit (floor 2: corridor level)
    float panel_x = ew/2 - 0.06f;
    float panel_z = 0.3f;
    add_wall(s, panel_x, 1.2f, panel_z, 0.04f, 0.7f, 0.2f, (Color){150, 135, 100, 255});
    set_last_material(s, MAT_BRASS);
    for (int i = 0; i < 4; i++) {
        float by = 0.95f + i * 0.15f;
        Color bc = (i == 1) ? btn_lit : btn_dark;
        add_wall(s, panel_x - 0.01f, by, panel_z, 0.03f, 0.08f, 0.08f, bc);
    }

    // Door seam
    add_wall(s, 0, eh/2, ed/2 - 0.02f, 0.04f, eh, 0.02f, (Color){30, 28, 25, 255});

    // Floor indicator
    add_wall(s, 0, eh - 0.3f, ed/2 - 0.06f, 0.5f, 0.25f, 0.02f, (Color){60, 55, 45, 255});
    set_last_material(s, MAT_BRASS);
    add_light_panel(s, 0, eh - 0.3f, ed/2 - 0.04f, 0.35f, 0.15f, 0.01f, (Color){240, 200, 120, 180});

    // Handrail
    add_wall(s, 0, 0.9f, -ed/2 + 0.08f, 1.4f, 0.03f, 0.03f, brass);
    set_last_material(s, MAT_BRASS);

    // ============================================================
    // EXTERIOR — the space station dropping away below
    // Through the glass wall and glass floor: lobby receding,
    // station skeleton, Earth through hull gaps.
    // ============================================================

    // LOBBY BELOW — checkerboard floor, chandelier, columns receding
    Color marble_a = {185, 180, 172, 200};
    Color marble_b = {45, 42, 38, 200};
    for (int tx = -3; tx <= 3; tx++) {
        for (int tz = -2; tz <= 2; tz++) {
            Color mc = ((tx + tz) % 2 == 0) ? marble_a : marble_b;
            add_wall(s, tx * 2.0f - 3, -2.0f, tz * 2.0f - 4, 1.9f, 0.05f, 1.9f, mc);
        }
    }
    // Chandelier below — warm glow shrinking as you rise
    add_light_panel(s, 0, -1.0f, -3, 0.8f, 0.6f, 0.8f, warm_panel);
    add_cylinder(s, 0, -0.5f, -3, 0.6f, 0.06f, brass);
    // Reception desk — brass rectangle
    add_wall(s, -6, -1.5f, -5, 3.5f, 0.8f, 1.0f, brass);
    set_last_material(s, MAT_WOOD);
    // Columns — cream pillars dropping away
    add_cylinder(s, -4, -2.0f, -4, 0.35f, 4.0f, cream);
    add_cylinder(s,  4, -2.0f, -4, 0.35f, 4.0f, cream);
    add_cylinder(s, -4, -2.0f,  4, 0.35f, 4.0f, cream);
    add_cylinder(s,  4, -2.0f,  4, 0.35f, 4.0f, cream);
    // Observation window glow — blue light from below
    add_wall(s, 0, -2.2f, -6, 10, 0.02f, 3, (Color){50, 120, 200, 60});
    set_last_decal(s);

    // ELEVATOR SHAFT — brass rings you rise past
    for (int i = 0; i < 12; i++) {
        float ry = -4.0f + i * 3.0f;
        float ring_r = 1.3f + sinf(i * 1.1f) * 0.15f;
        add_cylinder(s, 0, ry, 0, ring_r * 2 + 0.2f, 0.08f, brass);
        set_last_material(s, MAT_BRASS);
    }
    // Vertical guide rails — 4 brass posts
    for (int p = 0; p < 4; p++) {
        float angle = p * 1.5708f;  // 90 degrees
        float px = cosf(angle) * 1.4f;
        float pz = sinf(angle) * 1.4f;
        add_wall(s, px, 14, pz, 0.06f, 32, 0.06f, brass);
        set_last_material(s, MAT_BRASS);
    }

    // STATION SKELETON — hull ribs, cables, infrastructure
    for (int i = 0; i < 8; i++) {
        float ry = 2.0f + i * 4.0f;
        // Hull ribs visible through glass wall
        add_wall(s, -3.5f, ry, 0, 2.0f, 0.2f, 4.0f, hull);
        set_last_material(s, MAT_CONCRETE);
        add_wall(s, 3.5f, ry, 0, 2.0f, 0.2f, 4.0f, hull);
        set_last_material(s, MAT_CONCRETE);
    }
    // Vertical cable bundles
    add_wall(s, -2.8f, 14, -1.5f, 0.08f, 30, 0.08f, hull_dark);
    add_wall(s, -3.2f, 14, 0.8f, 0.06f, 30, 0.06f, hull_dark);
    add_wall(s, -2.5f, 14, 1.2f, 0.05f, 30, 0.05f, (Color){35, 38, 48, 255});

    // Brass pipe with condensation
    add_cylinder(s, -3.0f, 14, -0.5f, 0.1f, 28, brass);
    set_last_material(s, MAT_BRASS);

    // HULL GAPS — Earth visible through the station's skin
    for (int i = 0; i < 5; i++) {
        float gy = 4.0f + i * 5.0f;
        add_wall(s, -5, gy, 0, 1.5f, 2.5f, 5, hull_dark);
        set_last_material(s, MAT_CONCRETE);
        // Earth glow bleeding through
        add_wall(s, -4.5f, gy, 0, 0.5f, 1.5f, 3.0f, (Color){40, 90, 180, 25});
    }

    // Earth — the anchor, always present
    add_sphere(s, -8, 8, -6, 4.0f, (Color){35, 75, 140, 255});
    add_sphere(s, -8, 8, -6, 4.4f, (Color){120, 170, 220, 80});

    // DESTINATION ABOVE — hint of corridor appearing
    add_wall(s, 0, 28, 0, 4, 0.05f, 4, (Color){60, 50, 38, 255});
    add_light_panel(s, 0, 27, 0, 3, 0.5f, 3, (Color){240, 210, 140, 40});
    // Corridor carpet color bleeding through — deep violet
    add_wall(s, 0, 26, 0, 3, 0.02f, 3, (Color){55, 45, 75, 30});

    // Stars through hull gaps
    for (int i = 0; i < 10; i++) {
        float sx = -12.0f + (float)((i*31)%20);
        float sy = 5.0f + (float)((i*17)%25);
        float sz = -8.0f + (float)((i*13)%16);
        add_wall(s, sx, sy, sz, 0.12f, 0.12f, 0.12f,
                 (Color){240,235,225,(unsigned char)(100+(i*23)%120)});
    }

    tag_materials_by_color(s);

    s->spawn = (Vector3){0, 1.6f, 0};
    s->has_exit = false;

    // ── THE MOTIF: cigarette ──
    // Same gap in the panel. This elevator remembers your last ride.
    add_cylinder(s, 0.48f, 1.0f, -0.45f, 0.025f, 0.05f, (Color){230,225,215,180});

    // ── THE MOTIF: red book ──
    // On the floor, against the back wall. Kicked into the corner.
    add_red_book(s, -0.7f, 0.02f, -0.85f, 18.0f);
}

void build_taxi_ride(Scene *s) {
    *s = (Scene){0};
    // BOUNDS: taxi interior — no player movement, mouse-look only
    s->surface = SURFACE_MARBLE;

    // Michael Mann night palette — boosted for 480x300 readability
    Color fog_teal = {12, 18, 28, 255};
    Color dash_dark = {50, 44, 36, 255};
    Color leather = {75, 58, 40, 255};
    Color driver_coat = {40, 36, 30, 255};
    Color driver_head = {180, 155, 120, 255};
    Color window_frame = {35, 32, 28, 255};
    Color meter_bg = {40, 48, 38, 255};
    Color meter_glow = {100, 220, 120, 200};
    Color mirror_c = {80, 85, 90, 200};

    // City palette — boosted for readability
    Color road = {35, 38, 45, 255};
    Color bldg_a = {28, 34, 48, 255};
    Color bldg_b = {35, 40, 55, 255};
    Color bldg_c = {22, 28, 40, 255};
    Color window_amber = {240, 190, 90, 140};
    Color streetlight_glow = {250, 200, 100, 220};
    Color streetlight_pole = {65, 60, 55, 255};
    Color road_marking = {200, 195, 185, 100};
    Color wet_reflect = {240, 190, 90, 50};

    s->fog_color = fog_teal;
    s->fog_density = 0.004f;

    // Player sits in backseat at (0, 1.0, 0), looking forward (-Z)
    // Taxi interior — walls 0..static_wall_count move with camera

    // Dashboard — wide dark slab in front, below eye level
    add_wall(s, 0, 0.55f, -1.2f, 1.6f, 0.15f, 0.6f, dash_dark);
    // Dashboard top surface
    add_wall(s, 0, 0.64f, -1.2f, 1.6f, 0.03f, 0.6f, (Color){30, 27, 22, 255});

    // Steering wheel — front-left, small cylinder
    add_cylinder(s, -0.35f, 0.75f, -1.3f, 0.25f, 0.03f, (Color){35, 32, 28, 255});
    // Steering column
    add_cylinder(s, -0.35f, 0.65f, -1.35f, 0.04f, 0.2f, (Color){40, 38, 35, 255});

    // ============================================================
    // DRIVER — high fidelity cube-person, viewed from backseat
    // Segmented limbs, face detail, clothing: collar flaps, cuffs, watch
    // ============================================================
    s->driver_wall_start = s->wall_count;  // mark for return taxi to strip
    Color driver_collar = {225, 220, 210, 255};
    Color driver_cap = {200, 50, 42, 255};  // Godard red — the same cap you'll see again
    Color driver_skin = {205, 180, 145, 255};
    Color driver_cuff = {180, 175, 168, 255};
    Color driver_hair = {50, 42, 35, 255};
    float dx = -0.4f, dz = -0.88f;

    // Torso
    add_wall(s, dx, 0.72f, dz, 0.44f, 0.50f, 0.30f, driver_coat);
    set_last_material(s, MAT_FABRIC);
    // Shoulders
    add_wall(s, dx, 0.96f, dz, 0.58f, 0.12f, 0.30f, driver_coat);
    set_last_material(s, MAT_FABRIC);
    // Collar — bright white V
    add_wall(s, dx, 1.03f, dz + 0.02f, 0.26f, 0.08f, 0.18f, driver_collar);
    set_last_material(s, MAT_FABRIC);
    // Collar flaps — V shape visible from front/side
    add_wall(s, dx - 0.08f, 1.00f, dz - 0.14f, 0.10f, 0.06f, 0.06f, driver_collar);
    add_wall(s, dx + 0.08f, 1.00f, dz - 0.14f, 0.10f, 0.06f, 0.06f, driver_collar);

    // Neck
    add_cylinder(s, dx, 1.08f, dz + 0.01f, 0.14f, 0.10f, driver_skin);

    // Head
    add_wall(s, dx, 1.20f, dz + 0.02f, 0.24f, 0.26f, 0.24f, driver_head);
    // Jaw — wider at bottom
    add_wall(s, dx, 1.10f, dz + 0.02f, 0.22f, 0.06f, 0.20f, driver_skin);
    // Ears
    add_wall(s, dx + 0.13f, 1.18f, dz + 0.02f, 0.04f, 0.08f, 0.06f, driver_skin);
    add_wall(s, dx - 0.13f, 1.18f, dz + 0.02f, 0.04f, 0.08f, 0.06f, driver_skin);
    // Hair — dark patch on back of head (player sees this)
    add_wall(s, dx, 1.22f, dz + 0.11f, 0.22f, 0.18f, 0.08f, driver_hair);
    // Eyes — visible in rearview mirror angle, dark
    add_wall(s, dx - 0.06f, 1.22f, dz - 0.11f, 0.05f, 0.04f, 0.02f, (Color){30,25,20,255});
    add_wall(s, dx + 0.06f, 1.22f, dz - 0.11f, 0.05f, 0.04f, 0.02f, (Color){30,25,20,255});

    // Flat cap
    add_wall(s, dx, 1.35f, dz + 0.02f, 0.30f, 0.07f, 0.30f, driver_cap);
    set_last_material(s, MAT_FABRIC);
    // Cap brim
    add_wall(s, dx, 1.33f, dz - 0.12f, 0.28f, 0.025f, 0.14f, driver_cap);
    set_last_material(s, MAT_FABRIC);

    // Left upper arm
    add_wall(s, dx - 0.22f, 0.84f, dz - 0.05f, 0.13f, 0.28f, 0.13f, driver_coat);
    set_last_material(s, MAT_FABRIC);
    // Left forearm — reaching to wheel
    add_wall(s, dx - 0.20f, 0.76f, dz - 0.28f, 0.11f, 0.14f, 0.22f, driver_coat);
    set_last_material(s, MAT_FABRIC);
    // Left cuff — white shirt
    add_wall(s, dx - 0.18f, 0.74f, dz - 0.36f, 0.10f, 0.05f, 0.06f, driver_cuff);
    // Left hand on wheel
    add_wall(s, dx - 0.16f, 0.74f, dz - 0.42f, 0.10f, 0.07f, 0.08f, driver_skin);
    // Watch — brass glint
    add_wall(s, dx - 0.19f, 0.74f, dz - 0.34f, 0.08f, 0.06f, 0.08f, (Color){210,180,100,255});
    set_last_material(s, MAT_BRASS);

    // Right upper arm
    add_wall(s, dx + 0.22f, 0.84f, dz + 0.02f, 0.13f, 0.28f, 0.13f, driver_coat);
    set_last_material(s, MAT_FABRIC);
    // Right forearm — resting
    add_wall(s, dx + 0.24f, 0.70f, dz + 0.08f, 0.11f, 0.22f, 0.11f, driver_coat);
    set_last_material(s, MAT_FABRIC);
    // Right cuff
    add_wall(s, dx + 0.24f, 0.60f, dz + 0.08f, 0.10f, 0.05f, 0.06f, driver_cuff);
    // Right hand
    add_wall(s, dx + 0.24f, 0.56f, dz + 0.08f, 0.10f, 0.07f, 0.08f, driver_skin);

    // Jacket center seam — visible from behind
    add_wall(s, dx, 0.80f, dz + 0.15f, 0.02f, 0.40f, 0.02f, (Color){15,13,10,255});
    s->driver_wall_end = s->wall_count;  // end of driver walls

    // Rear-view mirror — small rectangle above dashboard center
    add_wall(s, 0, 1.15f, -1.1f, 0.2f, 0.1f, 0.04f, mirror_c);
    set_last_material(s, MAT_GLASS);
    // Mirror support
    add_wall(s, 0, 1.2f, -1.05f, 0.02f, 0.06f, 0.08f, (Color){40, 38, 35, 255});

    // Backseat — player sits here
    add_wall(s, 0, 0.35f, 0.1f, 1.4f, 0.12f, 0.5f, leather);
    set_last_material(s, MAT_LEATHER);
    // Seat back
    add_wall(s, 0, 0.65f, 0.35f, 1.4f, 0.5f, 0.1f, leather);
    set_last_material(s, MAT_LEATHER);

    // A-pillars (window frames) — left and right
    add_wall(s, -0.85f, 0.85f, -0.5f, 0.06f, 1.0f, 1.4f, window_frame);
    add_wall(s, 0.85f, 0.85f, -0.5f, 0.06f, 1.0f, 1.4f, window_frame);

    // B-pillars — behind player
    add_wall(s, -0.85f, 0.85f, 0.4f, 0.06f, 1.0f, 0.1f, window_frame);
    add_wall(s, 0.85f, 0.85f, 0.4f, 0.06f, 1.0f, 0.1f, window_frame);

    // Roof
    add_wall(s, 0, 1.35f, -0.2f, 1.8f, 0.05f, 1.2f, (Color){22, 20, 17, 255});

    // Floor
    add_wall(s, 0, 0.18f, -0.2f, 1.8f, 0.04f, 1.2f, (Color){18, 16, 14, 255});

    // Front windshield frame — top
    add_wall(s, 0, 1.3f, -1.4f, 1.7f, 0.06f, 0.04f, window_frame);

    // Center console / gear area
    add_wall(s, 0, 0.45f, -0.6f, 0.15f, 0.3f, 0.5f, dash_dark);

    // Taxi meter — on dashboard, right side
    add_wall(s, 0.45f, 0.72f, -1.1f, 0.18f, 0.1f, 0.08f, meter_bg);
    add_wall(s, 0.45f, 0.72f, -1.06f, 0.15f, 0.07f, 0.02f, meter_glow);

    // Door panels — inner surfaces
    add_wall(s, -0.82f, 0.5f, -0.1f, 0.04f, 0.5f, 0.8f, (Color){30, 27, 22, 255});
    add_wall(s, 0.82f, 0.5f, -0.1f, 0.04f, 0.5f, 0.8f, (Color){30, 27, 22, 255});

    // Mark end of taxi interior walls
    s->static_wall_count = s->wall_count;

    // ====== CITY — scrolling walls (indices >= static_wall_count) ======

    // Road — long continuous surface, segments every 20m
    for (int i = 0; i < 11; i++) {
        float z = -10.0f - i * 20.0f;
        add_wall(s, 0, -0.3f, z, 10.0f, 0.05f, 20.0f, road);
        // Road markings — center dashes
        add_wall(s, 0, -0.27f, z - 3.0f, 0.12f, 0.02f, 2.0f, road_marking);
        add_wall(s, 0, -0.27f, z - 9.0f, 0.12f, 0.02f, 2.0f, road_marking);
        add_wall(s, 0, -0.27f, z - 15.0f, 0.12f, 0.02f, 2.0f, road_marking);
    }

    // Sidewalks
    for (int i = 0; i < 11; i++) {
        float z = -10.0f - i * 20.0f;
        add_wall(s, -5.5f, -0.15f, z, 1.5f, 0.2f, 20.0f, (Color){35, 38, 42, 255});
        add_wall(s, 5.5f, -0.15f, z, 1.5f, 0.2f, 20.0f, (Color){35, 38, 42, 255});
    }

    // Buildings — LEFT side, varying heights and depths
    float left_bldg_x = -8.0f;
    for (int i = 0; i < 10; i++) {
        float z = -5.0f - i * 22.0f;
        float h = 6.0f + (i * 7 % 5) * 2.0f;
        float w = 4.0f + (i * 3 % 3);
        Color bc = (i % 3 == 0) ? bldg_a : (i % 3 == 1) ? bldg_b : bldg_c;
        add_wall(s, left_bldg_x, h / 2.0f, z, w, h, 3.0f, bc);

        // Windows — amber sodium vapor glow, 2 columns, multiple floors
        for (int floor = 0; floor < (int)(h / 2.5f); floor++) {
            for (int col = 0; col < 2; col++) {
                if ((floor + col + i) % 3 == 0) continue; // some dark
                float wx = left_bldg_x + 1.2f + col * 1.5f;
                float wy = 1.5f + floor * 2.5f;
                if (wy > h - 1.0f) continue;
                add_wall(s, wx, wy, z + 1.55f, 0.6f, 0.9f, 0.04f, window_amber);
            }
        }
    }

    // Buildings — RIGHT side
    float right_bldg_x = 8.0f;
    for (int i = 0; i < 10; i++) {
        float z = -12.0f - i * 20.0f;
        float h = 5.0f + (i * 5 % 6) * 2.0f;
        float w = 3.5f + (i * 4 % 4);
        Color bc = (i % 3 == 0) ? bldg_b : (i % 3 == 1) ? bldg_c : bldg_a;
        add_wall(s, right_bldg_x, h / 2.0f, z, w, h, 3.0f, bc);

        // Windows
        for (int floor = 0; floor < (int)(h / 2.5f); floor++) {
            for (int col = 0; col < 2; col++) {
                if ((floor + col + i) % 4 == 0) continue;
                float wx = right_bldg_x - 1.2f - col * 1.5f;
                float wy = 1.5f + floor * 2.5f;
                if (wy > h - 1.0f) continue;
                add_wall(s, wx, wy, z - 1.55f, 0.6f, 0.9f, 0.04f, window_amber);
            }
        }
    }

    // Streetlights — left side every ~25m
    for (int i = 0; i < 8; i++) {
        float z = -15.0f - i * 28.0f;
        // Pole
        add_wall(s, -4.8f, 2.0f, z, 0.08f, 4.0f, 0.08f, streetlight_pole);
        // Light fixture
        add_wall(s, -4.8f, 4.2f, z, 0.4f, 0.15f, 0.2f, streetlight_glow);
        // Wet road reflection below
        add_wall(s, -3.0f, -0.26f, z, 1.5f, 0.02f, 2.0f, wet_reflect);
    }

    // Streetlights — right side, offset
    for (int i = 0; i < 8; i++) {
        float z = -28.0f - i * 28.0f;
        add_wall(s, 4.8f, 2.0f, z, 0.08f, 4.0f, 0.08f, streetlight_pole);
        add_wall(s, 4.8f, 4.2f, z, 0.4f, 0.15f, 0.2f, streetlight_glow);
        add_wall(s, 3.0f, -0.26f, z, 1.5f, 0.02f, 2.0f, wet_reflect);
    }

    // Parked cars (dark silhouettes on sides of road)
    for (int i = 0; i < 5; i++) {
        float z = -20.0f - i * 40.0f;
        float side = (i % 2 == 0) ? -4.0f : 4.0f;
        // Car body
        add_wall(s, side, 0.35f, z, 1.8f, 0.6f, 0.8f, (Color){15, 15, 18, 255});
        // Car roof
        add_wall(s, side, 0.7f, z, 1.2f, 0.3f, 0.7f, (Color){18, 18, 22, 255});
        // Taillight (red glow if behind camera direction)
        add_wall(s, side - 0.85f, 0.35f, z + 0.35f, 0.06f, 0.06f, 0.04f,
                 (Color){180, 20, 20, 120});
    }

    // Distant city skyline — tall dark silhouettes at far Z
    add_wall(s, -15, 8, -220, 6, 16, 3, (Color){10, 14, 22, 255});
    add_wall(s, 12, 6, -215, 8, 12, 3, (Color){8, 12, 20, 255});
    add_wall(s, -8, 10, -225, 5, 20, 3, (Color){11, 15, 23, 255});
    add_wall(s, 20, 7, -220, 5, 14, 3, (Color){9, 13, 21, 255});
    // Sky Tower — rising above the skyline, first glimpse from the taxi
    add_skytower(s, 0, 0, -230, 6.0f, (Color){40, 45, 65, 255});

    tag_materials_by_color(s);

    s->spawn = (Vector3){0, 1.0f, 0};
    s->has_exit = false;

    // ── THE MOTIF: cigarette ──
    // Stubbed out in the cupholder. The driver's? Yours?
    // It was here before you got in.
    add_cylinder(s, 0.35f, 0.62f, -0.2f, 0.025f, 0.05f, (Color){230,225,215,255});
    // Ash tip — still warm
    add_wall(s, 0.35f, 0.64f, -0.2f, 0.015f, 0.008f, 0.015f, (Color){60,55,50,255});

    // ── THE MOTIF: red book ──
    // Wedged between the seat and the door. You'd have to reach for it.
    add_red_book(s, 0.55f, 0.58f, 0.15f, 12.0f);

    // ── THE TWOS: second ticket on the seat beside you ──
    // Booking confirmation — "2 guests." The first evidence.
    // Small cream rectangle on the passenger seat. Conspicuous.
    add_wall(s, -0.45f, 0.62f, 0.1f, 0.18f, 0.005f, 0.12f, (Color){245,242,235,255});
    set_last_decal(s);
    // Printed text implied — a thin dark line across it
    add_wall(s, -0.45f, 0.623f, 0.1f, 0.14f, 0.002f, 0.01f, (Color){40,40,45,80});
    set_last_decal(s);
    // Second line
    add_wall(s, -0.45f, 0.623f, 0.07f, 0.12f, 0.002f, 0.01f, (Color){40,40,45,60});
    set_last_decal(s);

}

// ============================================================
// SPACE HOTEL — The Great Glass Elevator
// A luxury hotel that has no business being in orbit.
// Parisian warmth inside, infinite void outside.
// ============================================================

void build_return_taxi(Scene *s) {
    // RETURN TAXI — dawn Auckland. Same interior, different world.
    build_taxi_ride(s);

    // Empty driver seat — dawn light falls on absence. The player notices.
    for (int i = s->driver_wall_start; i < s->driver_wall_end; i++)
        s->walls[i].color.a = 0;

    // Override fog — dawn pink-gold instead of teal
    s->fog_color = (Color){45, 38, 32, 255};
    s->fog_density = 0.003f;

    // Recolor exterior walls (indices >= static_wall_count) to dawn palette
    for (int i = s->static_wall_count; i < s->wall_count; i++) {
        Wall *w = &s->walls[i];
        unsigned char r = w->color.r, g = w->color.g, b = w->color.b;
        // Road surfaces — lighten from near-black to warm grey
        if (r < 35 && g < 35 && b < 40 && w->size.y < 0.2f)
            w->color = (Color){55, 52, 48, 255};
        // Buildings — dark blue → warm ochre silhouettes
        else if (r < 30 && b > r && w->size.y > 2.0f)
            w->color = (Color){60 + (r*2)%20, 52 + (g*2)%15, 42 + (b)%10, 255};
        // Windows — sodium amber → pale dawn reflections
        else if (r > 200 && g > 150 && b < 120 && w->color.a < 200)
            w->color = (Color){140, 160, 180, 50};
        // Streetlight glow — nearly off at dawn
        else if (r > 220 && g > 160 && w->size.y < 0.2f && w->size.x < 0.5f)
            w->color = (Color){80, 75, 70, 30};
        // Wet road reflections — pink-gold sky
        else if (w->color.a < 50 && r > 200)
            w->color = (Color){180, 140, 110, 15};
    }

    // Add Sky Tower — just a building drifting past, no wormhole
    add_skytower(s, 6, 0, -180, 5.0f, (Color){55, 58, 70, 255});

    // ── THE MOTIF: cigarette ──
    // Fresh one this time. Dawn light. The circle closes.
    add_cylinder(s, 0.3f, 0.62f, -0.15f, 0.025f, 0.05f, (Color){230,225,215,255});

    // ── THE MOTIF: red book ──
    // On the passenger seat. Where Gibbons sat. It wasn't there before.
    add_red_book(s, -0.4f, 0.52f, -0.6f, 8.0f);

}

void build_hyperspace(Scene *s) {
    *s = (Scene){0};
    // HYPERSPACE TUNNEL — the Sky Tower on its side becomes a wormhole
    // Player flies through it at accelerating speed. 2001 Stargate meets Auckland.
    s->surface = SURFACE_MARBLE;

    Color hull = PAL_HULL;
    Color void_black = PAL_PORTHOLE;
    Color godard_red = PAL_RED;
    Color godard_blue = PAL_BLUE;

    s->fog_color = (Color){2, 2, 6, 255};
    s->fog_density = 0.0005f;  // almost no fog — speed clarity

    // The Sky Tower itself — laid on its side, centered on Z axis
    // Player flies THROUGH it along -Z
    add_skytower(s, 0, 0, -40, 8.0f, hull);

    // Structural rings — brass cylinders the player whips past
    for (int i = 0; i < 16; i++) {
        float rz = -5.0f - i * 6.0f;
        float ring_r = 2.5f + sinf(i * 0.7f) * 0.8f;
        // Ring color shifts along the tunnel — warm → cool → warm
        float t = (float)i / 15.0f;
        unsigned char cr = (unsigned char)(178 + (int)(60 * sinf(t * 6.28f)));
        unsigned char cg = (unsigned char)(155 + (int)(40 * cosf(t * 4.0f)));
        unsigned char cb = (unsigned char)(107 + (int)(80 * sinf(t * 3.14f + 1.0f)));
        add_cylinder(s, 0, 0, rz, ring_r * 2, 0.15f, (Color){cr, cg, cb, 255});
        set_last_material(s, MAT_BRASS);
    }

    // Color accent panels — Godard red and blue flash past
    add_wall(s, -1.5f, 0.5f, -12, 0.1f, 1.5f, 0.8f, godard_red);
    set_last_material(s, MAT_FABRIC);
    add_wall(s, 1.2f, -0.3f, -28, 0.1f, 1.2f, 0.6f, godard_blue);
    set_last_material(s, MAT_FABRIC);
    add_wall(s, -0.8f, 1.0f, -45, 0.8f, 0.1f, 0.5f, godard_red);
    set_last_material(s, MAT_FABRIC);
    add_wall(s, 0.5f, -0.8f, -60, 0.6f, 0.1f, 0.8f, godard_blue);
    set_last_material(s, MAT_FABRIC);
    add_wall(s, -1.0f, -0.5f, -75, 0.1f, 0.8f, 1.0f, godard_red);
    set_last_material(s, MAT_FABRIC);

    // Light streaks — long thin bright panels rushing past
    for (int i = 0; i < 8; i++) {
        float angle = i * 0.785f;  // 45° spacing
        float lx = cosf(angle) * 2.0f;
        float ly = sinf(angle) * 2.0f;
        float lz = -8.0f - i * 10.0f;
        unsigned char la = (unsigned char)(140 + (i * 15) % 80);
        add_wall(s, lx, ly, lz, 0.04f, 0.04f, 4.0f, (Color){240, 220, 160, la});
        set_last_material(s, MAT_GLASS);
    }

    // Stars streaking past — elongated along Z (motion blur effect)
    for (int i = 0; i < 20; i++) {
        float sx = -4.0f + (float)((i*41)%80) * 0.1f;
        float sy = -3.0f + (float)((i*17)%60) * 0.1f;
        float sz = -3.0f - (float)((i*31)%90);
        add_wall(s, sx, sy, sz, 0.06f, 0.06f, 1.5f,
                 (Color){240,238,232,(unsigned char)(120+(i*23)%100)});
        set_last_material(s, MAT_GLASS);
    }

    // Second star field — tighter, closer, brighter (depth layering)
    for (int i = 0; i < 12; i++) {
        float sx = -1.5f + (float)((i*29)%30) * 0.1f;
        float sy = -1.0f + (float)((i*13)%20) * 0.1f;
        float sz = -10.0f - (float)((i*37)%50);
        add_wall(s, sx, sy, sz, 0.03f, 0.03f, 3.0f,
                 (Color){255,250,240,(unsigned char)(180+(i*17)%75)});
        set_last_material(s, MAT_GLASS);
    }

    // Convergence point — bright orb at the far end (the destination)
    add_sphere(s, 0, 0, -95.0f, 3.0f, (Color){200, 220, 255, 180});
    add_sphere(s, 0, 0, -95.0f, 5.0f, (Color){150, 180, 230, 60});

    // Void floor/ceiling — barely visible, gives orientation
    add_wall(s, 0, -3.0f, -45, 6, 0.02f, 90, void_black);
    set_last_material(s, MAT_MARBLE);
    add_wall(s, 0, 3.0f, -45, 6, 0.02f, 90, void_black);
    set_last_material(s, MAT_MARBLE);

    // Additional Godard color flashes — denser in the middle section
    add_wall(s, 0.7f, 0.8f, -35, 0.08f, 0.4f, 0.3f, godard_red);
    set_last_material(s, MAT_FABRIC);
    add_wall(s, -0.5f, -0.6f, -52, 0.3f, 0.08f, 0.4f, godard_blue);
    set_last_material(s, MAT_FABRIC);
    add_wall(s, 1.3f, 0.2f, -68, 0.06f, 0.6f, 0.2f, godard_red);
    set_last_material(s, MAT_FABRIC);
    add_wall(s, -1.1f, 0.4f, -82, 0.2f, 0.06f, 0.5f, godard_blue);
    set_last_material(s, MAT_FABRIC);

    // ── THE MOTIF: cigarette ──
    // Not a cigarette here. Just its ember — a red point drifting in the tunnel.
    // You recognize the color before you recognize the shape.
    add_sphere(s, 0.3f, 0.1f, -15.0f, 0.04f, (Color){200,80,40,200});

    s->spawn = (Vector3){0, 0, 2};
    s->has_exit = false;
}

void build_space_lobby(Scene *s) {
    *s = (Scene){0};
    // BOUNDS: 24m x 16m, fully enclosed cylindrical station interior
    s->surface = SURFACE_MARBLE;

    Color hull = PAL_HULL;
    Color brass = PAL_BRASS;
    Color cream = PAL_CREAM;
    Color void_black = PAL_PORTHOLE;
    Color gold = PAL_GOLD;
    Color godard_red = PAL_RED;
    Color godard_blue = PAL_BLUE;
    Color marble_a = PAL_MARBLE_A;
    Color warm_light = PAL_LIGHT_WARM;

    s->fog_color = PAL_FOG_STATION;
    s->fog_density = 0.001f;

    float lw = 24, ld = 16, lh = 8;  // original height — furniture positions depend on this

    // Floor — single plane, checkerboard marble
    add_wall(s, 0, -0.05f, 0, lw, 0.1f, ld, marble_a);
    set_last_material(s, MAT_CHECKERBOARD);

    // Ceiling — hull, thinner
    add_wall(s, 0, lh, 0, lw, 0.2f, ld, hull);
    set_last_material(s, MAT_CONCRETE);
    // Structural trim — 3 thin brass lines, not 6 heavy ribs
    for (int i = 0; i < 3; i++) {
        float rz = -ld/2 + 3 + i * (ld/3.0f);
        add_wall(s, 0, lh - 0.08f, rz, lw, 0.12f, 0.06f, brass);
        set_last_material(s, MAT_BRASS);
    }

    // Walls — hull exterior with cream paneling inside
    // Back wall — hull with large observation window
    add_wall(s, 0, lh/2, -ld/2, lw, lh, 0.4f, hull);
    set_last_material(s, MAT_CONCRETE);
    // Cream paneling on lower half
    add_wall(s, 0, lh*0.25f, -ld/2+0.22f, lw-2, lh*0.5f, 0.05f, cream);
    set_last_material(s, MAT_WALLPAPER);

    // OBSERVATION WINDOW — expanded but fits the 8m wall. The money shot.
    // 14m wide x 6.5m tall — dominates without exceeding the wall
    add_wall(s, 0, lh/2, -ld/2+0.1f, 14, 6.5f, 0.08f, void_black);
    set_last_material(s, MAT_GLASS);
    // Brass frame
    add_wall(s, 0, lh/2+3.35f, -ld/2+0.12f, 14.4f, 0.15f, 0.1f, brass);  // top
    set_last_material(s, MAT_BRASS);
    add_wall(s, 0, lh/2-3.35f, -ld/2+0.12f, 14.4f, 0.15f, 0.1f, brass);  // bottom
    set_last_material(s, MAT_BRASS);
    add_wall(s, -7.1f, lh/2, -ld/2+0.12f, 0.15f, 6.7f, 0.1f, brass);    // left
    set_last_material(s, MAT_BRASS);
    add_wall(s, 7.1f, lh/2, -ld/2+0.12f, 0.15f, 6.7f, 0.1f, brass);     // right
    set_last_material(s, MAT_BRASS);
    // Mullion — single vertical divider
    add_wall(s, 0, lh/2, -ld/2+0.14f, 0.08f, 6.5f, 0.06f, brass);
    set_last_material(s, MAT_BRASS);

    // Earth glow — wider, brighter, flooding the floor
    add_wall(s, 0, 0.02f, -ld/2+2, 14, 0.02f, 5, (Color){60, 130, 200, 180});
    set_last_decal(s);
    // Earth glow on ceiling — reflected
    add_wall(s, 0, lh-0.05f, -ld/2+2, 10, 0.02f, 4, (Color){45, 100, 180, 25});
    set_last_decal(s);
    // Earth — solid blue sphere visible through the window, far behind the glass
    add_sphere(s, 3, lh/2+1, -ld/2-12, 5.0f, (Color){35, 75, 140, 255});
    // Atmosphere rim — bright thin ring (slightly larger sphere, lighter)
    add_sphere(s, 3, lh/2+1, -ld/2-12, 5.4f, (Color){120, 170, 220, 255});

    // Side walls — hull with brass trim
    add_wall(s, -lw/2, lh/2, 0, 0.4f, lh, ld, hull);
    set_last_material(s, MAT_CONCRETE);
    add_wall(s, lw/2, lh/2, 0, 0.4f, lh, ld, hull);
    set_last_material(s, MAT_CONCRETE);
    // Cream paneling on side walls — below window line only (windows go from 1.1m up)
    add_wall(s, -lw/2+0.22f, 0.5f, 0, 0.05f, 1.0f, ld-2, cream);
    set_last_material(s, MAT_WALLPAPER);
    add_wall(s, lw/2-0.22f, 0.5f, 0, 0.05f, 1.0f, ld-2, cream);
    set_last_material(s, MAT_WALLPAPER);

    // Front wall
    add_wall(s, 0, lh/2, ld/2, lw, lh, 0.4f, hull);
    set_last_material(s, MAT_CONCRETE);
    add_wall(s, 0, lh*0.25f, ld/2-0.22f, lw-2, lh*0.5f, 0.05f, cream);
    set_last_material(s, MAT_WALLPAPER);

    // Brass trim at wainscoting line — all walls
    add_wall(s, 0, lh*0.5f+0.02f, -ld/2+0.24f, lw-2, 0.04f, 0.02f, brass);
    set_last_material(s, MAT_BRASS);
    add_wall(s, -lw/2+0.24f, lh*0.5f+0.02f, 0, 0.02f, 0.04f, ld-2, brass);
    set_last_material(s, MAT_BRASS);
    add_wall(s, lw/2-0.24f, lh*0.5f+0.02f, 0, 0.02f, 0.04f, ld-2, brass);
    set_last_material(s, MAT_BRASS);

    // TALL SIDE WINDOWS — glass house, not portholes
    for (int i = 0; i < 3; i++) {
        float pz = -4 + i * 4;
        // Left wall windows
        add_wall(s, -lw/2+0.18f, lh*0.45f, pz, 0.06f, 5.0f, 1.5f, void_black);
        set_last_material(s, MAT_GLASS);
        add_wall(s, -lw/2+0.2f, lh*0.45f+2.6f, pz, 0.04f, 0.12f, 1.6f, brass);
        set_last_material(s, MAT_BRASS);
        add_wall(s, -lw/2+0.2f, lh*0.45f-2.6f, pz, 0.04f, 0.12f, 1.6f, brass);
        set_last_material(s, MAT_BRASS);
        // Right wall windows
        add_wall(s, lw/2-0.18f, lh*0.45f, pz, 0.06f, 5.0f, 1.5f, void_black);
        set_last_material(s, MAT_GLASS);
        add_wall(s, lw/2-0.2f, lh*0.45f+2.6f, pz, 0.04f, 0.12f, 1.6f, brass);
        set_last_material(s, MAT_BRASS);
        add_wall(s, lw/2-0.2f, lh*0.45f-2.6f, pz, 0.04f, 0.12f, 1.6f, brass);
        set_last_material(s, MAT_BRASS);
        // Earth glow on floor from side windows
        add_wall(s, -lw/2+3, 0.02f, pz, 2.0f, 0.02f, 1.5f, (Color){45, 100, 180, 30});
        set_last_decal(s);
        add_wall(s, lw/2-3, 0.02f, pz, 2.0f, 0.02f, 1.5f, (Color){45, 100, 180, 30});
        set_last_decal(s);
    }

    // GLASS ELEVATOR SHAFT — structural rings + light column
    // No solid cylinder — brass rings define the form, light fills the void
    float shaft_r = 1.0f;
    // 5 brass rings stacked vertically — the skeleton of the elevator
    for (int i = 0; i < 5; i++) {
        float ry = 0.2f + i * (lh - 0.4f) / 4.0f;
        add_cylinder(s, 0, ry, 0, shaft_r*2+0.15f, 0.1f, brass);
    }
    // 4 vertical brass posts connecting the rings
    add_wall(s, -shaft_r, lh/2, 0, 0.08f, lh, 0.08f, brass);
    add_wall(s, shaft_r, lh/2, 0, 0.08f, lh, 0.08f, brass);
    add_wall(s, 0, lh/2, -shaft_r, 0.08f, lh, 0.08f, brass);
    add_wall(s, 0, lh/2, shaft_r, 0.08f, lh, 0.08f, brass);
    // Light column inside — the warm glow that makes it the Glass Elevator
    add_light_panel(s, 0, lh/2, 0, 0.4f, lh-0.5f, 0.4f, warm_light);

    // CHANDELIER — hanging from ceiling, brass + warm light
    // Above the observation window area
    add_cylinder(s, 0, lh-0.5f, -3, 0.04f, 1.0f, brass);     // chain
    add_cylinder(s, 0, lh-1.5f, -3, 0.8f, 0.08f, brass);      // ring
    add_cone(s, 0, lh-1.8f, -3, 0.6f, 0.3f, cream);           // shade
    add_light_panel(s, 0, lh-1.6f, -3, 1.0f, 0.8f, 1.0f, warm_light);

    // COLUMNS — cylindrical, brass-capped, framing the view
    add_column(s, -4, -4, 0.4f, lh, cream);
    add_column(s, 4, -4, 0.4f, lh, cream);
    add_column(s, -4, 4, 0.4f, lh, cream);
    add_column(s, 4, 4, 0.4f, lh, cream);
    // Brass caps
    for (int i = 0; i < 4; i++) {
        float cx = (i < 2) ? -4 : 4;
        float cz = (i % 2 == 0) ? -4 : 4;
        add_cylinder(s, cx, 0.02f, cz, 1.0f, 0.04f, brass);
        add_cylinder(s, cx, lh-0.02f, cz, 1.0f, 0.04f, brass);
    }

    // FLOATING FURNITURE — weightlessness tells the story
    // Wine glass hovering mid-air, no table beneath it
    add_wall(s, 6, 1.8f, -2, 0.05f, 0.12f, 0.05f, (Color){210,210,215,160});
    add_wall(s, 6, 1.85f, -2, 0.035f, 0.05f, 0.035f, (Color){140,35,45,200});
    // Book drifting near ceiling
    add_wall(s, -7, 5.5f, 2, 0.8f, 0.06f, 0.5f, godard_red);
    // Suitcase floating at waist height
    add_wall(s, 8, 1.2f, 3, 0.8f, 0.35f, 0.5f, (Color){150,100,60,255});
    add_wall(s, 7.85f, 1.32f, 3.22f, 0.18f, 0.13f, 0.02f, godard_blue);
    // Napkin — white rectangle drifting
    add_wall(s, -3, 3.2f, 5, 0.3f, 0.01f, 0.3f, (Color){245,242,238,200});

    // RECEPTION DESK — same brass desk as Paris, but floating 2cm above floor
    add_wall(s, -6, 0.52f, -5, 4, 1.0f, 1.2f, brass);
    set_last_material(s, MAT_WOOD);
    add_wall(s, -6, 1.04f, -5, 4.1f, 0.04f, 1.25f, gold);
    set_last_material(s, MAT_BRASS);
    // Desk lamp — floating
    add_cylinder(s, -4.5f, 1.1f, -5, 0.06f, 0.04f, brass);
    add_cylinder(s, -4.5f, 1.2f, -5, 0.03f, 0.15f, brass);
    add_cone(s, -4.5f, 1.38f, -5, 0.15f, 0.12f, cream);
    add_light_panel(s, -4.5f, 1.3f, -5, 0.2f, 0.25f, 0.2f, warm_light);

    // Hotel bell — floating just above desk
    add_cylinder(s, -5.5f, 1.12f, -4.5f, 0.06f, 0.03f, (Color){200,195,180,255});

    // Dropped ceiling sections with light panels
    add_dropped_ceiling(s, -6, lh, -2, 6, 5, 0.25f, hull, warm_light);
    add_dropped_ceiling(s, 6, lh, -2, 6, 5, 0.25f, hull, warm_light);

    // SEATING — Godard red bench, floating
    add_wall(s, 7, 0.55f, -4, 2.5f, 0.5f, 0.7f, godard_red);
    set_last_material(s, MAT_FABRIC);

    // Recessed panels on hull walls — industrial-meets-luxury
    add_recessed_panel(s, -8, lh*0.6f, -ld/2+0.26f, 3, 2, 0.08f, hull);
    add_recessed_panel(s, 8, lh*0.6f, -ld/2+0.26f, 3, 2, 0.08f, hull);

    // Stars visible through portholes — tiny bright cubes far behind walls
    for (int i = 0; i < 15; i++) {
        float sx = -30 + (i*41)%60;
        float sy = 2 + (i*17)%6;
        float sz = -ld/2 - 5 - (i*13)%15;
        add_wall(s, sx, sy, sz, 0.1f, 0.1f, 0.1f,
                 (Color){240,238,232,(unsigned char)(140+(i*23)%80)});
    }

    // ============================================================
    // ARCHITECTURAL DETAIL — Space Lobby
    // ============================================================

    // Bookshelves in back corners — station library
    add_bookshelf(s, -lw/2+0.3f, 1.6f, -ld/2+1.5f, 1.8f, 1.4f, 3, cream);
    add_bookshelf(s, lw/2-0.3f, 1.6f, -ld/2+1.5f, 1.8f, 1.4f, 3, cream);

    // Picture frames flanking observation window
    add_picture_frame(s, -7.0f, lh*0.55f, -ld/2+0.26f, 1.0f, 0.8f, brass, (Color){195,45,40,255});
    add_picture_frame(s, 7.0f, lh*0.55f, -ld/2+0.26f, 1.0f, 0.8f, brass, (Color){35,75,140,255});

    // Light shaft from observation window — Earth glow projected onto floor
    add_wall(s, 0, 0.02f, -5, 6, 0.02f, 3, (Color){35, 75, 140, 30});
    set_last_decal(s);
    add_wall(s, 0, 0.02f, -3, 8, 0.02f, 2, (Color){30, 65, 120, 20});
    set_last_decal(s);
    add_wall(s, 0, 0.02f, -1, 10, 0.02f, 2, (Color){25, 55, 100, 12});
    set_last_decal(s);

    // Volumetric light shafts — visible cones of light in the air
    // Dust catches Earth glow streaming through the observation window.
    // Tall transparent boxes, fading with distance from window.
    add_wall(s, -3, lh*0.35f, -6, 2.0f, lh*0.5f, 1.5f, (Color){40, 90, 180, 8});
    add_wall(s, 3, lh*0.4f, -5.5f, 1.5f, lh*0.45f, 2.0f, (Color){35, 80, 160, 6});
    add_wall(s, 0, lh*0.3f, -4, 3.0f, lh*0.4f, 2.5f, (Color){30, 70, 140, 4});
    // Side window light shafts — smaller, angled
    add_wall(s, -lw/2+4, lh*0.35f, -4, 1.5f, lh*0.4f, 1.0f, (Color){35, 85, 170, 6});
    add_wall(s, lw/2-4, lh*0.35f, 0, 1.5f, lh*0.4f, 1.0f, (Color){35, 85, 170, 6});

    // Floating debris — zero-g life (more items, varied heights)
    // Pen floating end-over-end
    add_wall(s, 3, 3.5f, 1, 0.04f, 0.04f, 0.18f, gold);
    // Napkin — slow tumble
    add_wall(s, -4, 4.2f, -1, 0.3f, 0.005f, 0.25f, cream);
    // Key card
    add_wall(s, 8, 2.8f, -3, 0.12f, 0.03f, 0.08f, (Color){240,238,232,200});
    // Coffee cup (inverted — spilled in zero-g)
    add_cylinder(s, -8, 3.0f, 4, 0.08f, 0.1f, cream);
    // Single coffee droplet floating nearby
    add_sphere(s, -7.8f, 3.3f, 3.8f, 0.04f, (Color){90, 60, 30, 180});

    // ── Hospitality touches — someone runs this hotel ──
    // Welcome drink on reception desk — champagne flute, golden
    add_cylinder(s, -5.2f, 1.2f, -4.0f, 0.04f, 0.12f, (Color){210,210,215,160});
    set_last_material(s, MAT_GLASS);
    add_wall(s, -5.2f, 1.14f, -4.0f, 0.04f, 0.06f, 0.04f, (Color){240,210,100,180});
    // Guest book — open, leather bound, pen resting
    add_wall(s, -6.0f, 1.15f, -3.8f, 0.4f, 0.03f, 0.3f, (Color){60,40,25,255});
    set_last_material(s, MAT_LEATHER);
    add_wall(s, -6.0f, 1.17f, -3.8f, 0.35f, 0.005f, 0.28f, cream);  // pages
    add_wall(s, -5.85f, 1.18f, -3.7f, 0.03f, 0.02f, 0.15f, gold);  // pen
    // Fresh flowers — small arrangement on side table
    add_cylinder(s, 7, 0.5f, -5, 0.06f, 0.12f, (Color){200,210,220,140});
    set_last_material(s, MAT_GLASS);  // vase
    // Flower stems — tiny colored blocks above vase
    add_wall(s, 6.95f, 0.65f, -5, 0.04f, 0.1f, 0.04f, (Color){60,120,50,255});
    add_wall(s, 7.05f, 0.68f, -5, 0.04f, 0.08f, 0.04f, (Color){60,120,50,255});
    // Flower heads — small colored dots
    add_sphere(s, 6.95f, 0.72f, -5, 0.08f, (Color){220,180,190,255});
    add_sphere(s, 7.05f, 0.74f, -5, 0.07f, (Color){200,160,170,255});
    // Room key — brass, on the desk, waiting for you
    add_wall(s, -5.8f, 1.16f, -4.5f, 0.10f, 0.03f, 0.06f, brass);
    set_last_material(s, MAT_BRASS);

    // ── Leading lines — brass floor inlays guide toward observation window ──
    add_wall(s, 0, 0.01f, 0, 0.1f, 0.02f, ld*0.7f, brass);
    set_last_material(s, MAT_BRASS);
    set_last_decal(s);
    // Diagonal accent lines converging at window
    add_wall(s, -3, 0.01f, -2, 0.06f, 0.02f, 8, brass);
    set_last_material(s, MAT_BRASS);
    set_last_decal(s);
    set_last_rotation(s, 10.0f);
    add_wall(s, 3, 0.01f, -2, 0.06f, 0.02f, 8, brass);
    set_last_material(s, MAT_BRASS);
    set_last_decal(s);
    set_last_rotation(s, -10.0f);

    // Interactive objects
    add_object(s, -5.5f, 1.2f, -4.5f, "bell", (Color){200,195,180,255}, 1);
    add_object(s, 6, 1.8f, -2, "wineglass", (Color){210,210,215,255}, 1);

    // Newspaper — floating in zero-g near the bench, drifting toward the window
    add_wall(s, 5.5f, 1.4f, -5.5f, 0.5f, 0.02f, 0.35f, (Color){235,232,228,220});
    add_object(s, 5.5f, 1.5f, -5.5f, "newspaper", (Color){235,232,228,255}, 1);

    // ── TWOS — everything in this lobby was designed for a couple ──

    // Bench facing Earth — two-person width, invitation to sit together
    add_wall(s, 0, 0.25f, -5, 1.8f, 0.5f, 0.5f, cream);
    set_last_material(s, MAT_FABRIC);
    // Armrest dividing two seats
    add_wall(s, 0, 0.45f, -5, 0.06f, 0.3f, 0.5f, brass);
    set_last_material(s, MAT_BRASS);

    // Luggage rack — brass, two positions marked
    add_wall(s, 3, 0.3f, 3, 1.2f, 0.04f, 0.6f, brass);
    set_last_material(s, MAT_BRASS);
    // Your one bag — conspicuously light for a suite booked for two
    add_wall(s, 3.3f, 0.35f, 3, 0.4f, 0.25f, 0.3f, (Color){60,55,50,255});
    set_last_material(s, MAT_LEATHER);
    // The empty space where the second bag would be — just the brass rack showing

    tag_materials_by_color(s);

    // Spawn facing the observation window, not the exit
    s->spawn = (Vector3){0, 1.6f, 4};
    s->exit_pos = (Vector3){0, 1.6f, ld/2-1};
    s->has_exit = true;

    // ── THE MOTIF: cigarette ──
    // Floating in zero-g near reception. No gravity, no ashtray.
    // The previous guest's last act before checking in.
    add_cylinder(s, 2.5f, 1.8f, 1.0f, 0.025f, 0.05f, (Color){230,225,215,160});
    // Ash drifting away — tiny grey dot
    add_wall(s, 2.6f, 1.9f, 1.1f, 0.01f, 0.01f, 0.01f, (Color){120,115,110,80});

    // ================================================================
    // MEZZANINE — brass observation platform at 4m height
    // Reachable by wall-running the back wall + mantling the window frame
    // Reward: Earth fills your entire view from up here
    // ================================================================
    // Main platform — brass walkway along the observation window
    add_wall(s, 0, 4.0f, -7.0f, 10.0f, 0.1f, 2.0f, (Color){180,155,90,255});
    set_last_material(s, 6);  // brass
    // Brass railing — front edge
    add_wall(s, 0, 4.4f, -6.0f, 10.0f, 0.04f, 0.04f, (Color){180,155,90,220});
    // Railing posts
    add_cylinder(s, -4.0f, 4.2f, -6.0f, 0.03f, 0.4f, (Color){180,155,90,220});
    add_cylinder(s,  0.0f, 4.2f, -6.0f, 0.03f, 0.4f, (Color){180,155,90,220});
    add_cylinder(s,  4.0f, 4.2f, -6.0f, 0.03f, 0.4f, (Color){180,155,90,220});
    // Step ledges on the side walls — parkour stepping stones
    // Left wall: low ledge at 2m, then high ledge at 3.2m
    add_wall(s, -11.8f, 2.0f, -4.0f, 0.2f, 0.15f, 3.0f, (Color){180,155,90,200});
    set_last_material(s, 6);
    add_wall(s, -11.8f, 3.2f, -6.0f, 0.2f, 0.15f, 2.0f, (Color){180,155,90,200});
    set_last_material(s, 6);
    // Right wall: matching ledges
    add_wall(s, 11.8f, 2.0f, -4.0f, 0.2f, 0.15f, 3.0f, (Color){180,155,90,200});
    set_last_material(s, 6);
    add_wall(s, 11.8f, 3.2f, -6.0f, 0.2f, 0.15f, 2.0f, (Color){180,155,90,200});
    set_last_material(s, 6);

    // ── THE MOTIF: red book ──
    // On the reception desk edge, facing the observation window.
    add_red_book(s, 4.2f, 1.05f, 2.8f, 0.0f);

}

void build_space_corridor(Scene *s) {
    *s = (Scene){0};
    // BOUNDS: curved corridor — 4m wide, 30m long, hull walls with portholes
    s->surface = SURFACE_CARPET;

    Color hull = PAL_HULL;
    Color hull_lt = PAL_HULL_LIGHT;
    Color brass = PAL_BRASS;
    Color cream = PAL_CREAM;
    Color void_black = PAL_PORTHOLE;
    Color carpet_a = {85, 70, 105, 255};     // warm violet — readable at 480x300
    Color carpet_b = {95, 78, 112, 255};
    Color warm_amber = PAL_GLOW_AMBER;
    Color door_colors[2] = {PAL_RED, PAL_BLUE};

    s->fog_color = (Color){6, 10, 22, 255};  // blue-tinted fog — glass walkway
    s->fog_density = 0.0008f;  // clearer — glass corridors are transparent

    float W = 4.5f, H = 3.5f;  // original height — door/decoration positions depend on this

    // CURVED CORRIDOR — approximate a gentle arc with 8 straight segments
    // Each segment is 4m long, angled slightly to create the curve
    // Commandment 7: Spatial impossibility — later segments are 5% longer.
    // The corridor toward the suite stretches imperceptibly.
    // Not enough to notice. Enough to feel.
    int segs = 8;
    float seg_len = 4.0f;        // nominal segment length (for sizing geometry)
    float curve_radius = 40.0f;  // gentle curve
    // Pre-compute cumulative angles for asymmetric segment lengths
    float seg_lengths[8];
    float total_arc = 0;
    for (int i = 0; i < segs; i++) {
        seg_lengths[i] = 4.0f + (i > 4 ? (i - 4) * 0.07f : 0);  // last 3 segs: +7/14/21cm
        total_arc += seg_lengths[i];
    }
    float total_angle = total_arc / curve_radius;
    float start_angle = -total_angle / 2;

    // Cumulative angle offsets
    float cum_angle[9];
    cum_angle[0] = start_angle;
    for (int i = 0; i < segs; i++) {
        cum_angle[i + 1] = cum_angle[i] + seg_lengths[i] / curve_radius;
    }

    for (int i = 0; i < segs; i++) {
        float a0 = cum_angle[i];
        float a1 = cum_angle[i + 1];
        float amid = (a0 + a1) / 2;

        // Center of this segment
        float cx = sinf(amid) * curve_radius;
        float cz = -cosf(amid) * curve_radius + curve_radius;

        // Approximate segment with axis-aligned geometry
        // Floor tiles
        for (int t = 0; t < 3; t++) {
            float tx = cx - W/2 + t*1.5f + 0.75f;
            float tz = cz;
            add_wall(s, tx, -0.05f, tz, 1.48f, 0.1f, seg_len-0.1f,
                     ((i+t)%2==0) ? carpet_a : carpet_b);
            set_last_material(s, MAT_CARPET);
        }

        // Ceiling — thinner, less oppressive
        add_wall(s, cx, H, cz, W, 0.15f, seg_len, hull);
        set_last_material(s, MAT_CONCRETE);
        // Structural trim — thin brass line, not heavy rib
        add_wall(s, cx, H-0.05f, cz, W+0.1f, 0.08f, 0.06f, brass);
        set_last_material(s, MAT_BRASS);

        // Walls — thinner (glass house, not pressure vessel)
        add_wall(s, cx-W/2, H/2, cz, 0.15f, H, seg_len, hull);
        set_last_material(s, MAT_CONCRETE);
        add_wall(s, cx+W/2, H/2, cz, 0.15f, H, seg_len, hull);
        set_last_material(s, MAT_CONCRETE);

        // Cream paneling — tall panels on non-window walls (75% height)
        // Window side gets no cream so void reads uninterrupted
        if (i % 2 == 0 || (i % 4 != 1)) {
            add_wall(s, cx-W/2+0.14f, H*0.375f, cz, 0.04f, H*0.75f, seg_len-0.2f, cream);
            set_last_material(s, MAT_WALLPAPER);
        }
        if (i % 2 == 0 || (i % 4 == 1)) {
            add_wall(s, cx+W/2-0.14f, H*0.375f, cz, 0.04f, H*0.75f, seg_len-0.2f, cream);
            set_last_material(s, MAT_WALLPAPER);
        }

        // Brass trim at wainscot
        add_wall(s, cx-W/2+0.15f, H*0.5f, cz, 0.02f, 0.04f, seg_len-0.2f, brass);
        set_last_material(s, MAT_BRASS);
        add_wall(s, cx+W/2-0.15f, H*0.5f, cz, 0.02f, 0.04f, seg_len-0.2f, brass);
        set_last_material(s, MAT_BRASS);

        // Side cove lighting — NOT overhead (glass house, not submarine)
        if (i % 2 == 0) {
            add_light_panel(s, cx-W/2+0.2f, H-0.15f, cz, 0.15f, 0.08f, seg_len*0.8f, warm_amber);
            add_light_panel(s, cx+W/2-0.2f, H-0.15f, cz, 0.15f, 0.08f, seg_len*0.8f, warm_amber);
        }

        // TALL WINDOWS — glass panels, not portholes (glass house, not submarine)
        if (i % 2 == 1) {
            float side = (i % 4 == 1) ? -(W/2-0.1f) : (W/2-0.1f);
            float wx = cx + side;
            // TALL WINDOW — fits within 3.5m wall (0.3 to 3.2m)
            float win_h = 2.8f;
            float win_cy = 0.3f + win_h/2;
            add_wall(s, wx, win_cy, cz, 0.06f, win_h, 1.4f, void_black);
            set_last_material(s, MAT_GLASS);
            // Brass frame — four edges
            add_wall(s, wx, win_cy+win_h/2+0.06f, cz, 0.04f, 0.1f, 1.5f, brass);
            set_last_material(s, MAT_BRASS);
            add_wall(s, wx, 0.25f, cz, 0.04f, 0.1f, 1.5f, brass);
            set_last_material(s, MAT_BRASS);
            add_wall(s, wx, win_cy, cz-0.75f, 0.04f, win_h+0.1f, 0.08f, brass);
            set_last_material(s, MAT_BRASS);
            add_wall(s, wx, win_cy, cz+0.75f, 0.04f, win_h+0.1f, 0.08f, brass);
            set_last_material(s, MAT_BRASS);
            // Horizontal mullion at eye height
            add_wall(s, wx, 1.6f, cz, 0.03f, 0.06f, 1.4f, brass);
            set_last_material(s, MAT_BRASS);
            // Stars through window — two depths
            add_wall(s, wx + (side > 0 ? 3 : -3), H*0.55f, cz-1,
                     0.08f, 0.08f, 0.08f, (Color){240,238,232,180});
            add_wall(s, wx + (side > 0 ? 5 : -5), H*0.55f+0.5f, cz+0.5f,
                     0.05f, 0.05f, 0.05f, (Color){200,210,230,120});
            // Earth visible through window — shifts position per segment
            // As you walk the corridor, Earth drifts downward (planet rotation)
            float earth_offset = (side > 0 ? 8.0f : -8.0f);
            float earth_y = 0.8f - (float)i * 0.15f;  // sinks as you walk deeper
            add_sphere(s, wx + earth_offset, earth_y, cz,
                       2.5f, (Color){30, 60, 140, 100});
            // Atmosphere rim
            add_sphere(s, wx + earth_offset, earth_y, cz,
                       2.7f, (Color){80, 140, 220, 55});
            // Earth glow shaft — wide blue wash across floor
            float shaft_x = cx + (side > 0 ? 0.8f : -0.8f);
            add_wall(s, shaft_x, 0.02f, cz, 2.0f, 0.02f, 1.8f,
                     (Color){50, 100, 180, 40});
            set_last_decal(s);
            // Glow on ceiling from window reflection
            add_wall(s, shaft_x, H-0.05f, cz, 1.5f, 0.02f, 1.2f,
                     (Color){40, 85, 160, 20});
            set_last_decal(s);
        }

        // DOORS — Godard red/blue, on alternating sides
        if (i >= 2 && i <= 6 && i % 2 == 0) {
            float side = (i % 4 == 0) ? -(W/2-0.1f) : (W/2-0.1f);
            Color door_c = door_colors[(i / 4) % 2];
            add_wall(s, cx+side, 1.3f, cz, 0.12f, 2.6f, 1.0f, door_c);
            // Door frame
            add_wall(s, cx+side, 2.75f, cz, 0.12f, 0.12f, 1.1f, brass);
            add_wall(s, cx+side, 1.3f, cz-0.55f, 0.12f, 2.6f, 0.05f, brass);
            add_wall(s, cx+side, 1.3f, cz+0.55f, 0.12f, 2.6f, 0.05f, brass);
            // Room number plate
            add_wall(s, cx+side*0.95f, 2, cz-0.7f, 0.06f, 0.2f, 0.15f, brass);
        }
    }

    // End caps — walls closing the corridor
    float end0_z = -cosf(start_angle) * curve_radius + curve_radius;
    float end1_z = -cosf(start_angle + total_angle) * curve_radius + curve_radius;
    float end0_x = sinf(start_angle) * curve_radius;
    float end1_x = sinf(start_angle + total_angle) * curve_radius;
    add_wall(s, end0_x, H/2, end0_z - seg_lengths[0]/2, W, H, 0.25f, hull);
    add_wall(s, end1_x, H/2, end1_z + seg_lengths[segs-1]/2, W, H, 0.25f, hull);

    // ============================================================
    // LIGHT UNDER DOORS — each door leaks a different life
    // ============================================================
    for (int i = 0; i < segs; i++) {
        if (i >= 2 && i <= 6 && i % 2 == 0) {
            float a_m = start_angle + (i + 0.5f) * (total_angle / segs);
            float door_cx = sinf(a_m) * curve_radius;
            float door_cz = -cosf(a_m) * curve_radius + curve_radius;
            float side = (i % 4 == 0) ? -(W/2-0.1f) : (W/2-0.1f);
            int door_idx = (i - 2) / 2;
            Color door_light;
            switch (door_idx) {
                case 0: door_light = (Color){240,210,120,140}; break;  // warm reading lamp
                case 1: door_light = (Color){80,120,200,100};  break;  // TV flicker blue
                default: door_light = (Color){0,0,0,0};        break;  // darkness — empty
            }
            if (door_light.a > 0) {
                add_wall(s, door_cx+side*0.85f, 0.02f, door_cz,
                         0.06f, 0.02f, 0.9f, door_light);
                set_last_decal(s);
            }
        }
    }

    // ============================================================
    // THE IMPOSSIBLE DOOR — opens to raw hull. Not a room.
    // ============================================================
    {
        float a_imp = start_angle + 6.5f * (total_angle / segs);
        float imp_cx = sinf(a_imp) * curve_radius;
        float imp_cz = -cosf(a_imp) * curve_radius + curve_radius;
        float imp_side = (W/2-0.1f);
        add_wall(s, imp_cx+imp_side-0.02f, 1.3f, imp_cz, 0.06f, 2.6f, 0.95f,
                 (Color){8,8,12,255});
        set_last_material(s, MAT_CONCRETE);
        add_wall(s, imp_cx+imp_side+0.04f, 1.3f, imp_cz-0.45f, 0.08f, 2.6f, 0.08f,
                 door_colors[0]);
        add_wall(s, imp_cx+imp_side, 2.75f, imp_cz, 0.12f, 0.12f, 1.1f, brass);
        add_wall(s, imp_cx+imp_side, 1.3f, imp_cz-0.55f, 0.12f, 2.6f, 0.05f, brass);
        add_wall(s, imp_cx+imp_side, 1.3f, imp_cz+0.55f, 0.12f, 2.6f, 0.05f, brass);
        add_wall(s, imp_cx+imp_side*0.95f, 2, imp_cz-0.7f, 0.06f, 0.2f, 0.15f, brass);
    }

    // ============================================================
    // STRATEGIC EMPTINESS — missing hull panels, exposed ribs
    // ============================================================
    {
        float a_empty = start_angle + 4.5f * (total_angle / segs);
        float empty_cx = sinf(a_empty) * curve_radius;
        float empty_cz = -cosf(a_empty) * curve_radius + curve_radius;
        for (int r = 0; r < 3; r++) {
            add_wall(s, empty_cx+W/2-0.08f, 0.5f + r*1.2f, empty_cz,
                     0.04f, 0.06f, 3.0f, hull_lt);
            set_last_material(s, MAT_CONCRETE);
        }
        add_cylinder(s, empty_cx+W/2-0.06f, H/2, empty_cz+0.8f,
                     0.04f, H-0.5f, (Color){120,110,95,255});
        set_last_material(s, MAT_BRASS);
        // Dark void behind ribs — passage-height gap (2.8m opening)
        // Only the top strip blocks, the rest is open
        add_wall(s, empty_cx+W/2+0.05f, H - 0.2f, empty_cz,
                 0.1f, 0.5f, 3.0f, (Color){15,15,20,255});

        // ── THE VOID BETWEEN — maintenance shaft ──────────────────
        // A secret space through the hull gap. Tall (12m), vertical,
        // industrial. Wall running, mantling, bunny hopping — let loose.
        // The hotel's skeleton. What holds it all together.
        float vx = empty_cx + W/2 + 4.0f;  // 4m behind the hull wall
        float vz = empty_cz;
        float VH = 12.0f;  // tall — 3.5x corridor height
        float VW = 6.0f;

        // Floor — metal grating
        add_wall(s, vx, -0.05f, vz, VW, 0.1f, 8.0f, (Color){40,42,48,255});
        set_last_material(s, MAT_CONCRETE);

        // Walls — raw hull, no paneling
        add_wall(s, vx - VW/2, VH/2, vz, 0.15f, VH, 8.0f, (Color){22,24,32,255});
        set_last_material(s, MAT_CONCRETE);
        add_wall(s, vx + VW/2, VH/2, vz, 0.15f, VH, 8.0f, (Color){22,24,32,255});
        set_last_material(s, MAT_CONCRETE);
        // Back wall
        add_wall(s, vx, VH/2, vz - 4.0f, VW, VH, 0.15f, (Color){18,20,28,255});
        set_last_material(s, MAT_CONCRETE);
        // Front wall (with gap for entry from corridor)
        add_wall(s, vx, VH/2, vz + 4.0f, VW, VH, 0.15f, (Color){18,20,28,255});
        set_last_material(s, MAT_CONCRETE);

        // Ceiling — high up, not a problem
        add_wall(s, vx, VH, vz, VW, 0.15f, 8.0f, (Color){15,16,22,255});
        set_last_material(s, MAT_CONCRETE);

        // CATWALKS — wall-runnable platforms at various heights
        // Level 1 (3m) — first jump target
        add_wall(s, vx - 1.5f, 3.0f, vz, 1.2f, 0.08f, 4.0f, (Color){55,52,48,255});
        set_last_material(s, MAT_BRASS);
        // Level 2 (5.5m) — wall run from level 1
        add_wall(s, vx + 1.5f, 5.5f, vz - 1.0f, 1.2f, 0.08f, 3.0f, (Color){55,52,48,255});
        set_last_material(s, MAT_BRASS);
        // Level 3 (8m) — mantle ledge
        add_wall(s, vx - 0.5f, 8.0f, vz + 1.0f, 2.0f, 0.08f, 2.0f, (Color){55,52,48,255});
        set_last_material(s, MAT_BRASS);

        // Vertical pipes — wall-runnable surfaces
        add_cylinder(s, vx - VW/2 + 0.3f, VH/2, vz - 1.5f, 0.12f, VH, (Color){70,65,58,255});
        set_last_material(s, MAT_BRASS);
        add_cylinder(s, vx + VW/2 - 0.3f, VH/2, vz + 1.0f, 0.12f, VH, (Color){70,65,58,255});
        set_last_material(s, MAT_BRASS);
        add_cylinder(s, vx - 1.0f, VH/2, vz - 3.5f, 0.10f, VH, (Color){65,60,55,255});
        set_last_material(s, MAT_BRASS);

        // Horizontal pipes — mantle targets
        add_cylinder(s, vx, 4.2f, vz - 3.0f, 0.08f, VW - 1.0f, (Color){60,58,52,255});
        set_last_rotation(s, 90.0f);
        set_last_material(s, MAT_BRASS);
        add_cylinder(s, vx, 7.0f, vz + 2.0f, 0.08f, VW - 2.0f, (Color){60,58,52,255});
        set_last_rotation(s, 90.0f);
        set_last_material(s, MAT_BRASS);

        // Reward at the top — a small observation window showing Earth
        add_wall(s, vx, 9.5f, vz - 3.9f, 1.5f, 1.0f, 0.06f, void_black);
        set_last_material(s, MAT_GLASS);
        // Earth glow through the window
        add_wall(s, vx, 9.5f, vz - 5.0f, 0.5f, 0.5f, 0.5f, (Color){60,120,200,120});

        // Dim ambient light — emergency lighting only
        add_light_panel(s, vx, 0.3f, vz, 0.3f, 0.05f, 0.3f, (Color){180,100,60,120});
        add_light_panel(s, vx, 6.0f, vz - 3.8f, 0.2f, 0.05f, 0.2f, (Color){100,80,60,100});
        add_light_panel(s, vx - 0.5f, 2.0f, vz + 0.5f, 0.25f, 0.05f, 0.25f, (Color){200,120,60,110});
    }

    // ============================================================
    // WINDOW-INTO-WINDOW — porthole into someone else's room
    // Through it: their porthole, looking out at Earth.
    // ============================================================
    {
        float a_win = start_angle + 3.5f * (total_angle / segs);
        float win_cx = sinf(a_win) * curve_radius;
        float win_cz = -cosf(a_win) * curve_radius + curve_radius;
        float win_side = -(W/2-0.1f);
        add_sphere(s, win_cx+win_side, H*0.55f, win_cz, 0.9f, void_black);
        add_cylinder(s, win_cx+win_side, H*0.55f, win_cz, 1.1f, 0.06f, brass);
        float room_x = win_cx+win_side - 2.5f;
        add_wall(s, room_x, H*0.55f, win_cz, 0.3f, 0.5f, 0.3f,
                 (Color){200,170,120,120});
        add_sphere(s, room_x-0.3f, H*0.55f, win_cz, 0.3f, void_black);
        add_cylinder(s, room_x-0.3f, H*0.55f, win_cz, 0.35f, 0.03f, brass);
        add_wall(s, room_x-1.5f, H*0.55f, win_cz,
                 0.06f, 0.06f, 0.06f, (Color){240,238,232,200});

        // Sprint 4A: PORTHOLE GHOST — Gibbons silhouette through the porthole
        // Not the real NPC — just geometry. The player catches a glimpse.
        // Wonder, not dread: he was here before you, not watching you
        // Barton Fink: Charlie was always in the next room.
        float room_x2 = win_cx + win_side - 3.5f;
        Color ghost = {25, 28, 45, 200};
        // Body
        add_wall(s, room_x2, H*0.55f - 0.1f, win_cz, 0.20f, 0.45f, 0.15f, ghost);
        // Head
        add_wall(s, room_x2, H*0.55f + 0.25f, win_cz, 0.14f, 0.16f, 0.12f, ghost);
        // Red cap — the beacon, even in silhouette
        add_wall(s, room_x2, H*0.55f + 0.35f, win_cz, 0.18f, 0.08f, 0.16f,
                 (Color){140, 35, 30, 180});
        // Briefcase — golden glint
        add_wall(s, room_x2 + 0.12f, H*0.55f - 0.25f, win_cz, 0.14f, 0.10f, 0.05f,
                 (Color){120, 85, 35, 160});
    }

    // FLOATING OBJECTS — life's detritus in zero-g
    add_wall(s, end0_x - W/2+0.3f, 1.8f, end0_z, 0.2f, 0.5f, 0.15f, door_colors[0]);
    float mid_x = sinf(0) * curve_radius;
    float mid_z = -cosf(0) * curve_radius + curve_radius;
    add_wall(s, mid_x+0.5f, 2.2f, mid_z, 0.5f, 0.01f, 0.35f, (Color){235,232,228,200});
    // Playing card tumbling
    add_wall(s, mid_x-0.8f, 1.4f, mid_z+3, 0.12f, 0.18f, 0.01f, (Color){245,242,238,220});
    // Sock — escaped laundry
    add_wall(s, end1_x+0.3f, 2.8f, end1_z-2, 0.06f, 0.15f, 0.04f, (Color){40,38,42,200});
    // Pen cap
    add_cylinder(s, mid_x-1.2f, 3.0f, mid_z-2, 0.04f, 0.08f, brass);
    // Toothbrush
    add_wall(s, end0_x+1.0f, 2.5f, end0_z+5, 0.04f, 0.04f, 0.18f, (Color){240,238,234,220});
    add_wall(s, end0_x+1.0f, 2.5f, end0_z+5.09f, 0.06f, 0.04f, 0.06f, (Color){60,140,180,255});

    // Light shaft near first porthole
    add_wall(s, end0_x, 0.02f, end0_z+3, 1.0f, 0.02f, 1.5f, (Color){60,130,200,40});

    // Runner strip — brass inlay in floor
    add_wall(s, 0, 0.01f, mid_z, 0.3f, 0.02f, segs*seg_len*0.8f, brass);
    set_last_material(s, MAT_BRASS);

    // ============================================================
    // ARCHITECTURAL DETAIL — Space Corridor
    // ============================================================

    // Picture frames on walls between doors
    {
        // Between seg 1 and 3 (left wall area)
        float a_mid1 = start_angle + 1.5f * (total_angle / segs);
        float cx1 = sinf(a_mid1) * curve_radius;
        float cz1 = -cosf(a_mid1) * curve_radius + curve_radius;
        add_picture_frame(s, cx1-(W/2-0.08f), 2.0f, cz1, 0.5f, 0.4f, brass, (Color){195,45,40,255});

        // Between seg 5 and 7 (right wall area)
        float a_mid5 = start_angle + 5.5f * (total_angle / segs);
        float cx5 = sinf(a_mid5) * curve_radius;
        float cz5 = -cosf(a_mid5) * curve_radius + curve_radius;
        add_picture_frame(s, cx5+(W/2-0.08f), 2.0f, cz5, 0.5f, 0.4f, brass, (Color){40,65,160,255});
    }

    // ============================================================
    // TEMPORAL IMPOSSIBILITY — Commandment 7
    // The last window shows warm amber light, not void.
    // Dawn. But it's 2 AM. The player can't name what's wrong,
    // but the corridor is telling them time doesn't work here.
    // ============================================================
    {
        float a_last = start_angle + 7.5f * (total_angle / segs);
        float last_cx = sinf(a_last) * curve_radius;
        float last_cz = -cosf(a_last) * curve_radius + curve_radius;
        float last_side = -(W/2 - 0.1f);
        // Warm dawn glow behind the glass — wrong time, wrong place
        add_wall(s, last_cx + last_side - 0.5f, 1.0f, last_cz,
                 0.02f, 2.0f, 1.2f, (Color){220, 160, 80, 60});
        // Dawn sky backing — amber gradient where void should be
        add_wall(s, last_cx + last_side - 1.0f, 1.5f, last_cz,
                 0.02f, 3.0f, 1.6f, (Color){180, 120, 50, 30});
        // Warm floor shaft — dawn light pooling on carpet
        add_wall(s, last_cx + last_side + 0.8f, 0.02f, last_cz,
                 1.5f, 0.02f, 1.2f, (Color){200, 150, 70, 35});
        set_last_decal(s);
    }

    // Interactive objects — diegetic details
    // Floating newspaper (zero-g) — read the headlines, learn about the station
    add_object(s, mid_x + 0.5f, 2.2f, mid_z, "newspaper", (Color){235,232,228,200}, 1);

    tag_materials_by_color(s);

    s->spawn = (Vector3){end0_x, 1.6f, end0_z};
    s->exit_pos = (Vector3){end1_x, 1.6f, end1_z};
    s->has_exit = true;

    // ── THE MOTIF: cigarette ──
    // Visible through the porthole — in the neighbor's room.
    // Someone else is waiting too.
    add_cylinder(s, -3.4f, 1.1f, 6.05f, 0.025f, 0.05f, (Color){230,225,215,100});

    // ================================================================
    // HIGH ROUTE — parkour path along the corridor ceiling
    // Wall-run + mantle onto window/door frames for an alternate view
    // ================================================================
    // Maintenance ledges running along both walls at 2.6m height
    // Left wall — continuous brass rail (mantleable)
    add_wall(s, -W/2+0.1f, 2.6f, seg_len*4, 0.12f, 0.08f, seg_len*7.5f,
             (Color){160,140,80,180});
    set_last_material(s, 6);  // brass
    // Right wall — matching rail
    add_wall(s, W/2-0.1f, 2.6f, seg_len*4, 0.12f, 0.08f, seg_len*7.5f,
             (Color){160,140,80,180});
    set_last_material(s, 6);
    // Ceiling vent grates — look down through these from the high route
    // You can see into the rooms below (narrative reward for parkour)
    for (int vi = 0; vi < 3; vi++) {
        float vz = 3.0f + vi * 8.0f;
        // Vent frame
        add_wall(s, -W/2+0.5f, H-0.15f, vz, 0.8f, 0.1f, 0.5f,
                 (Color){80,75,70,200});
        // Warm light leaking through — someone's room below
        add_wall(s, -W/2+0.5f, H-0.16f, vz, 0.6f, 0.01f, 0.35f,
                 (Color){240,200,120, (unsigned char)(100 - vi*25)});
    }
    // Pipe obstacles on the high route — must duck or jump over
    add_cylinder(s, 0, 2.9f, 8.0f, 0.06f, W, (Color){90,85,80,255});
    add_cylinder(s, 0, 2.9f, 20.0f, 0.06f, W, (Color){90,85,80,255});

    // ── THE MOTIF: red book ──
    // On a maintenance ledge near a door frame. Left by a guest who took the high route.
    add_red_book(s, -1.8f, 1.15f, 12.0f, 22.0f);

}

void build_space_suite(Scene *s) {
    *s = (Scene){0};
    // BOUNDS: 14m x 12m, fully enclosed — the Glass Elevator suite
    // Dahl's space hotel room: absurd luxury in zero gravity
    s->surface = SURFACE_WOOD;

    Color hull = PAL_HULL;
    Color brass = PAL_BRASS;
    Color cream = PAL_CREAM;
    Color white = PAL_WHITE;
    Color void_black = PAL_PORTHOLE;
    Color gold = PAL_GOLD;
    Color wood = PAL_WOOD_DARK;
    Color dark_wood = {105, 78, 48, 255};
    Color warm_light = PAL_LIGHT_WARM;
    Color earth_glow = PAL_EARTH_GLOW;
    Color navy = PAL_NAVY;

    s->fog_color = PAL_FOG_STATION;
    s->fog_density = 0.001f;

    float rw = 14, rd = 12, rh = 5;  // taller than Paris room — double height

    // Floor — single plane, herringbone wood
    add_wall(s, 0, -0.05f, 0, rw, 0.1f, rd, wood);
    set_last_material(s, MAT_HERRINGBONE);

    // Ceiling — hull, thinner
    add_wall(s, 0, rh, 0, rw, 0.2f, rd, hull);
    set_last_material(s, MAT_CONCRETE);
    // 2 thin brass trim lines — not heavy ribs
    for (int i = 0; i < 2; i++) {
        float rz = -rd/2 + 3 + i * (rd/2.0f);
        add_wall(s, 0, rh-0.05f, rz, rw, 0.1f, 0.06f, brass);
        set_last_material(s, MAT_BRASS);
    }

    // Walls — hull exterior, cream paneling interior (80% height — luxury, not bunker)
    // Back wall — behind bed
    add_wall(s, 0, rh/2, -rd/2, rw, rh, 0.3f, hull);
    set_last_material(s, MAT_CONCRETE);
    add_wall(s, 0, rh*0.4f, -rd/2+0.17f, rw-1, rh*0.8f, 0.04f, cream);
    set_last_material(s, MAT_WALLPAPER);
    // Front wall
    add_wall(s, 0, rh/2, rd/2, rw, rh, 0.3f, hull);
    set_last_material(s, MAT_CONCRETE);
    add_wall(s, 0, rh*0.4f, rd/2-0.17f, rw-1, rh*0.8f, 0.04f, cream);
    set_last_material(s, MAT_WALLPAPER);
    // Side walls
    add_wall(s, -rw/2, rh/2, 0, 0.3f, rh, rd, hull);
    set_last_material(s, MAT_CONCRETE);
    add_wall(s, rw/2, rh/2, 0, 0.3f, rh, rd, hull);
    set_last_material(s, MAT_CONCRETE);
    add_wall(s, -rw/2+0.17f, rh*0.4f, 0, 0.04f, rh*0.8f, rd-1, cream);
    set_last_material(s, MAT_WALLPAPER);
    add_wall(s, rw/2-0.17f, rh*0.4f, 0, 0.04f, rh*0.8f, rd-1, cream);
    set_last_material(s, MAT_WALLPAPER);

    // Wainscoting trim — brass line
    add_wall(s, 0, rh*0.5f, -rd/2+0.19f, rw-1, 0.04f, 0.02f, brass);
    set_last_material(s, MAT_BRASS);
    add_wall(s, 0, rh*0.5f, rd/2-0.19f, rw-1, 0.04f, 0.02f, brass);
    set_last_material(s, MAT_BRASS);
    add_wall(s, -rw/2+0.19f, rh*0.5f, 0, 0.02f, 0.04f, rd-1, brass);
    set_last_material(s, MAT_BRASS);
    add_wall(s, rw/2-0.19f, rh*0.5f, 0, 0.02f, 0.04f, rd-1, brass);
    set_last_material(s, MAT_BRASS);

    // FLOOR-TO-CEILING WINDOW — left wall, the Glass Elevator view
    // Massive void panel with thin brass frame
    add_wall(s, -rw/2+0.08f, rh/2, -1, 0.06f, rh-1, 5, void_black);
    set_last_material(s, MAT_GLASS);
    // Brass frame
    add_wall(s, -rw/2+0.1f, rh-0.4f, -1, 0.04f, 0.15f, 5.3f, brass);   // top
    set_last_material(s, MAT_BRASS);
    add_wall(s, -rw/2+0.1f, 0.4f, -1, 0.04f, 0.15f, 5.3f, brass);      // bottom
    set_last_material(s, MAT_BRASS);
    add_wall(s, -rw/2+0.1f, rh/2, -3.65f, 0.04f, rh-0.8f, 0.15f, brass); // left
    set_last_material(s, MAT_BRASS);
    add_wall(s, -rw/2+0.1f, rh/2, 1.65f, 0.04f, rh-0.8f, 0.15f, brass);  // right
    set_last_material(s, MAT_BRASS);
    // Mullions — horizontal brass bars
    add_wall(s, -rw/2+0.12f, rh*0.33f, -1, 0.03f, 0.06f, 5, brass);
    set_last_material(s, MAT_BRASS);
    add_wall(s, -rw/2+0.12f, rh*0.66f, -1, 0.03f, 0.06f, 5, brass);
    set_last_material(s, MAT_BRASS);

    // Earth glow on floor from window
    add_wall(s, -rw/2+3, 0.02f, -1, 4, 0.02f, 4, earth_glow);
    set_last_decal(s);

    // Stars outside the window
    for (int i = 0; i < 12; i++) {
        float sx = -rw/2 - 3 - (i*7)%10;
        float sy = 1 + (i*13)%5;
        float sz = -3 + (i*11)%7;
        add_wall(s, sx, sy, sz, 0.08f, 0.08f, 0.08f,
                 (Color){240,238,232,(unsigned char)(120+(i*29)%100)});
    }

    // PORTHOLE — right wall, circular
    add_sphere(s, rw/2-0.15f, rh*0.55f, 0, 1.5f, void_black);
    add_cylinder(s, rw/2-0.13f, rh*0.55f, 0, 1.7f, 0.08f, brass);

    // ============================================================
    // FURNITURE — grounded luxury with floating accents
    // ============================================================

    // BED — same composition as Paris, but headboard is taller (double height room)
    add_wall(s, 0, 0.2f, -4.5f, 3.4f, 0.4f, 2.0f, dark_wood);       // frame
    set_last_material(s, MAT_WOOD);
    add_wall(s, 0, 0.5f, -4.5f, 3.2f, 0.25f, 1.8f, white);           // mattress
    set_last_material(s, MAT_FABRIC);
    add_wall(s, -0.6f, 0.68f, -5.2f, 0.7f, 0.2f, 0.4f, white);       // pillow L
    set_last_material(s, MAT_FABRIC);
    add_wall(s, 0.6f, 0.68f, -5.2f, 0.7f, 0.2f, 0.4f, white);        // pillow R
    set_last_material(s, MAT_FABRIC);
    // Headboard — tall navy panel (Godard blue relative)
    add_wall(s, 0, 1.8f, -5.5f, 3.6f, 2.8f, 0.12f, navy);
    set_last_material(s, MAT_FABRIC);
    add_wall(s, 0, 3.25f, -5.48f, 3.7f, 0.05f, 0.06f, brass);        // brass cap
    set_last_material(s, MAT_BRASS);

    // Nightstands — floating 2cm above floor
    add_wall(s, -2.5f, 0.32f, -4.8f, 0.6f, 0.6f, 0.6f, wood);
    set_last_material(s, MAT_WOOD);
    add_wall(s, 2.5f, 0.32f, -4.8f, 0.6f, 0.6f, 0.6f, wood);
    set_last_material(s, MAT_WOOD);

    // Bedside lamps
    add_cylinder(s, -2.5f, 0.64f, -4.8f, 0.08f, 0.04f, brass);
    add_cylinder(s, -2.5f, 0.76f, -4.8f, 0.03f, 0.2f, brass);
    add_cone(s, -2.5f, 0.92f, -4.8f, 0.18f, 0.14f, cream);
    add_light_panel(s, -2.5f, 0.87f, -4.8f, 0.2f, 0.35f, 0.2f, warm_light);
    add_cylinder(s, 2.5f, 0.64f, -4.8f, 0.08f, 0.04f, brass);
    add_cylinder(s, 2.5f, 0.76f, -4.8f, 0.03f, 0.2f, brass);
    add_cone(s, 2.5f, 0.92f, -4.8f, 0.18f, 0.14f, cream);
    add_light_panel(s, 2.5f, 0.87f, -4.8f, 0.2f, 0.35f, 0.2f, warm_light);

    // DESK — against right wall
    add_wall(s, 5.5f, 0.4f, -2, 2.5f, 0.8f, 0.9f, wood);
    set_last_material(s, MAT_WOOD);
    add_wall(s, 5.5f, 0.82f, -2, 2.6f, 0.03f, 0.95f, brass);
    set_last_material(s, MAT_BRASS);

    // SOFA — facing window (the view IS the television)
    add_wall(s, -3, 0.25f, 2, 2.4f, 0.5f, 0.9f, navy);
    set_last_material(s, MAT_FABRIC);
    add_wall(s, -3, 0.6f, 2.45f, 2.4f, 0.6f, 0.15f, navy);
    set_last_material(s, MAT_FABRIC);
    add_wall(s, -4.2f, 0.4f, 2, 0.08f, 0.45f, 0.9f, navy);
    set_last_material(s, MAT_FABRIC);
    add_wall(s, -1.8f, 0.4f, 2, 0.08f, 0.45f, 0.9f, navy);
    set_last_material(s, MAT_FABRIC);

    // Throw pillow on sofa — Godard red against navy
    add_wall(s, -3.5f, 0.55f, 1.8f, 0.35f, 0.30f, 0.30f, PAL_RED);
    set_last_material(s, MAT_FABRIC);

    // Coffee table — brass, low
    add_wall(s, -3, 0.35f, 3.5f, 1.2f, 0.03f, 0.7f, brass);
    set_last_material(s, MAT_BRASS);
    add_cylinder(s, -3.5f, 0.17f, 3.2f, 0.025f, 0.33f, brass);
    add_cylinder(s, -2.5f, 0.17f, 3.2f, 0.025f, 0.33f, brass);
    // Book on coffee table — bold blue spine, cream pages
    add_wall(s, -3.1f, 0.37f, 3.5f, 0.3f, 0.04f, 0.2f, PAL_BLUE);
    set_last_material(s, MAT_LEATHER);
    add_cylinder(s, -3.5f, 0.17f, 3.8f, 0.025f, 0.33f, brass);
    add_cylinder(s, -2.5f, 0.17f, 3.8f, 0.025f, 0.33f, brass);

    // ============================================================
    // FLOATING OBJECTS — zero gravity storytelling
    // ============================================================

    // Bathrobe hovering — Dahl absurdity, golden fabric catching light
    add_wall(s, 5.5f, 2.8f, 1, 0.8f, 1.6f, 0.1f, (Color){240,220,50,255});

    // Pen floating above desk — tiny brass cylinder, spinning implied
    add_cylinder(s, 5.8f, 1.6f, -2.2f, 0.02f, 0.2f, brass);

    // Champagne glass inverted near ceiling — the absurd detail
    add_cone(s, -5, 4.2f, -2, 0.08f, 0.1f, (Color){210,210,215,160});
    add_cylinder(s, -5, 4.05f, -2, 0.02f, 0.12f, (Color){210,210,215,160});
    // Champagne droplets — gold spheres drifting (scaled for 480x300 visibility)
    add_sphere(s, -4.8f, 3.8f, -1.8f, 0.18f, gold);
    add_sphere(s, -5.1f, 3.5f, -2.2f, 0.14f, gold);
    add_sphere(s, -4.6f, 3.3f, -1.6f, 0.16f, gold);

    // Photograph floating face-down — same as Paris (you can't flip it here either)
    add_wall(s, 3, 2.5f, 0, 0.2f, 0.01f, 0.15f, (Color){240,238,230,255});

    // Book open, pages fanned — floating near sofa
    add_wall(s, -2, 1.5f, 3, 0.6f, 0.04f, 0.4f, white);
    add_wall(s, -2.05f, 1.53f, 3, 0.55f, 0.02f, 0.38f, (Color){30,30,35,255});

    // ============================================================
    // STORYTELLING OBJECTS — traces of the trip planned for two
    // She was supposed to be here. The hotel prepared for both of you.
    // ============================================================

    // Suitcase by the door — yours, half-unpacked. You arrived alone.
    add_wall(s, 5, 0.15f, 4.5f, 0.7f, 0.3f, 0.45f, dark_wood);
    set_last_material(s, MAT_LEATHER);
    add_wall(s, 5, 0.32f, 4.5f, 0.72f, 0.02f, 0.47f, brass);
    set_last_material(s, MAT_BRASS);
    set_last_decal(s);
    add_wall(s, 5, 0.34f, 4.72f, 0.68f, 0.15f, 0.02f, dark_wood);
    set_last_material(s, MAT_LEATHER);
    // Your shirt spilling out — you didn't finish unpacking
    add_wall(s, 5.3f, 0.28f, 4.7f, 0.25f, 0.04f, 0.35f, (Color){55,85,175,220});
    set_last_material(s, MAT_FABRIC);

    // Shoes by the bed — placed neatly, toes facing out
    add_wall(s, -1.8f, 0.06f, -3.5f, 0.12f, 0.12f, 0.28f, (Color){35,30,28,255});
    add_wall(s, -1.55f, 0.06f, -3.5f, 0.12f, 0.12f, 0.28f, (Color){35,30,28,255});

    // Postcard face-down on nightstand — you can't read it
    add_wall(s, 2.5f, 0.64f, -4.6f, 0.16f, 0.03f, 0.11f, cream);
    set_last_decal(s);

    // HALF-WRITTEN LETTER on desk — pen left mid-sentence
    add_wall(s, 5.3f, 0.85f, -2.2f, 0.3f, 0.005f, 0.22f, (Color){245,242,235,255});
    set_last_decal(s);
    // Pen — resting at an angle across the letter
    add_cylinder(s, 5.45f, 0.86f, -2.1f, 0.03f, 0.16f, (Color){30,28,25,255});
    set_last_rotation(s, 35.0f);

    // Wine glass on coffee table — hers, from the minibar. Left here by the hotel.
    // Stem
    add_cylinder(s, -3.2f, 0.4f, 3.3f, 0.02f, 0.12f, (Color){210,210,215,160});
    // Bowl
    add_wall(s, -3.2f, 0.49f, 3.3f, 0.05f, 0.06f, 0.05f, (Color){210,210,215,160});
    // Lipstick mark — removed (invisible at 960x600)

    // Photograph face-down on nightstand — the two of you, Paris café
    add_wall(s, -2.5f, 0.64f, -4.5f, 0.2f, 0.01f, 0.15f, (Color){240,238,230,255});

    // Her scarf draped over the sofa arm — she always left things on chairs
    add_wall(s, -4.2f, 0.7f, 2.0f, 0.08f, 0.4f, 0.3f, (Color){200,50,45,200});
    set_last_material(s, MAT_FABRIC);

    // ============================================================
    // RECESSED PANELS — depth on hull walls
    // ============================================================
    add_recessed_panel(s, -3, rh*0.6f, -rd/2+0.2f, 2.5f, 1.5f, 0.08f, hull);
    add_recessed_panel(s, 3, rh*0.6f, -rd/2+0.2f, 2.5f, 1.5f, 0.08f, hull);
    add_recessed_panel(s, 5, rh*0.6f, rd/2-0.2f, 3, 1.8f, 0.08f, hull);

    // Light shaft from window across floor
    add_wall(s, -rw/2+4, 0.02f, -1, 5, 0.02f, 3, (Color){60,130,200,100});
    set_last_decal(s);
    // Secondary earth glow — warm band on ceiling reflected
    add_wall(s, -rw/2+2, rh-0.1f, -1, 3, 0.02f, 3, (Color){45,100,180,30});
    set_last_decal(s);

    // DROPPED CEILING with warm light — above bed area
    add_dropped_ceiling(s, 0, rh, -4, 5, 4, 0.2f, hull, warm_light);

    // ============================================================
    // ARCHITECTURAL DETAIL — Space Suite
    // ============================================================

    // Picture frames above bed — Earth photography, brass frames
    add_picture_frame(s, -1.5f, 3.8f, -rd/2+0.2f, 0.8f, 0.6f, brass, (Color){35,75,140,255});
    add_picture_frame(s, 1.5f, 3.8f, -rd/2+0.2f, 0.8f, 0.6f, brass, (Color){60,130,200,255});

    // Bookshelf on side wall — 3 rows, cream shelves
    add_bookshelf(s, rw/2-0.2f, 1.6f, 0.02f, 1.4f, 1.2f, 3, cream);

    // Baseboards — all 4 walls (brass, matching station trim)
    add_baseboard(s, 0, 0, -rd/2+0.08f, rw, 0, brass);
    add_baseboard(s, 0, 0, rd/2-0.08f, rw, 0, brass);
    add_baseboard(s, -rw/2+0.08f, 0, 0, rd, 1.0f, brass);
    add_baseboard(s, rw/2-0.08f, 0, 0, rd, 1.0f, brass);

    // Crown moldings at ceiling — brass
    add_crown_molding(s, 0, rh, -rd/2+0.08f, rw, 0, brass);
    add_crown_molding(s, 0, rh, rd/2-0.08f, rw, 0, brass);
    add_crown_molding(s, -rw/2+0.08f, rh, 0, rd, 1.0f, brass);
    add_crown_molding(s, rw/2-0.08f, rh, 0, rd, 1.0f, brass);

    // Corner reinforcement — prevent room escape
    add_wall(s, -rw/2+0.1f, rh/2, -rd/2+0.1f, 0.5f, rh, 0.5f, hull);
    add_wall(s, rw/2-0.1f, rh/2, -rd/2+0.1f, 0.5f, rh, 0.5f, hull);
    add_wall(s, -rw/2+0.1f, rh/2, rd/2-0.1f, 0.5f, rh, 0.5f, hull);
    add_wall(s, rw/2-0.1f, rh/2, rd/2-0.1f, 0.5f, rh, 0.5f, hull);

    // ── P5: Non-interactive storytelling objects ──
    // Half-packed suitcase by door — someone arrived but hasn't settled
    add_wall(s, 4.5f, 0.15f, 4.0f, 0.6f, 0.3f, 0.4f, dark_wood);  // open case
    set_last_material(s, MAT_LEATHER);
    add_wall(s, 4.5f, 0.31f, 4.2f, 0.55f, 0.02f, 0.15f, cream);   // fabric draped over edge
    set_last_material(s, MAT_FABRIC);
    add_wall(s, 4.5f, 0.08f, 4.0f, 0.35f, 0.08f, 0.25f, navy);    // folded shirt inside
    set_last_material(s, MAT_FABRIC);

    // Postcard face-down on nightstand — unread, unknowable
    add_wall(s, -2.3f, 0.88f, -4.5f, 0.18f, 0.005f, 0.12f, cream);
    set_last_rotation(s, 8.0f);

    // Shoes by door — two small dark boxes, slightly angled
    add_wall(s, 5.2f, 0.04f, 3.5f, 0.12f, 0.08f, 0.22f, (Color){35,30,25,255});
    set_last_material(s, MAT_LEATHER);
    set_last_rotation(s, -5.0f);
    add_wall(s, 5.5f, 0.04f, 3.4f, 0.12f, 0.08f, 0.22f, (Color){35,30,25,255});
    set_last_material(s, MAT_LEATHER);
    set_last_rotation(s, 12.0f);

    // ── Clerestory window above bed — stars peek in above the headboard ──
    // When you lie down, you see infinity above you
    add_wall(s, 0, rh-0.5f, -rd/2+0.1f, 6, 0.8f, 0.06f, (Color){4,5,12,255});
    set_last_material(s, MAT_GLASS);
    add_wall(s, 0, rh-0.08f, -rd/2+0.12f, 6.2f, 0.06f, 0.04f, brass);
    set_last_material(s, MAT_BRASS);
    add_wall(s, 0, rh-0.92f, -rd/2+0.12f, 6.2f, 0.06f, 0.04f, brass);
    set_last_material(s, MAT_BRASS);

    // ── Furniture detail in suite ──
    // Bed pillows — two small boxes at head
    add_wall(s, -0.5f, 0.72f, -5.3f, 0.5f, 0.12f, 0.35f, white);
    set_last_material(s, MAT_FABRIC);
    add_wall(s, 0.5f, 0.72f, -5.3f, 0.5f, 0.12f, 0.35f, white);
    set_last_material(s, MAT_FABRIC);

    // Wardrobe slightly ajar — door box rotated 15°
    add_wall(s, rw/2-0.5f, 1.2f, -3.0f, 0.6f, 2.2f, 0.5f, dark_wood);
    set_last_material(s, MAT_WOOD);
    add_wall(s, rw/2-0.82f, 1.2f, -2.75f, 0.04f, 2.1f, 0.45f, dark_wood);
    set_last_material(s, MAT_WOOD);
    set_last_rotation(s, 15.0f);

    // ── Turn-down service — someone prepared this room for you ──
    // Towel swan on bed — the universal hotel welcome gesture
    add_wall(s, 0, 0.66f, -4.0f, 0.2f, 0.15f, 0.12f, white);       // body
    set_last_material(s, MAT_FABRIC);
    add_wall(s, 0, 0.78f, -3.9f, 0.06f, 0.12f, 0.04f, white);      // neck
    set_last_material(s, MAT_FABRIC);
    // Slippers by bed — aligned, waiting
    add_wall(s, -0.6f, 0.03f, -3.5f, 0.1f, 0.04f, 0.2f, cream);
    set_last_material(s, MAT_FABRIC);
    add_wall(s, -0.4f, 0.03f, -3.5f, 0.1f, 0.04f, 0.2f, cream);
    set_last_material(s, MAT_FABRIC);
    // ── TWO ROBES — the room was prepared for two ──
    // Robe 1 (yours) — white, on the bathroom door hook
    add_wall(s, rw/2-0.2f, 1.8f, 2.0f, 0.04f, 0.8f, 0.3f, white);
    set_last_material(s, MAT_FABRIC);
    // Robe 2 (hers) — slightly smaller, different hook, still wrapped
    // The hotel prepared both. Nobody came to wear this one.
    add_wall(s, rw/2-0.2f, 1.7f, 2.6f, 0.04f, 0.7f, 0.25f, white);
    set_last_material(s, MAT_FABRIC);

    // ── TWO CHAMPAGNE GLASSES — the ritual was shared ──
    // Second glass on the tray next to the champagne bottle
    // It will stay empty for the entire game. The washing-line moment.
    add_cone(s, -3.5f, 0.39f, 3.5f, 0.06f, 0.08f, (Color){210,210,215,200});
    add_cylinder(s, -3.5f, 0.44f, 3.5f, 0.02f, 0.08f, (Color){210,210,215,200});

    // ── HER BOOK — on the nightstand, spine cracked mid-read ──
    // She was halfway through. Page 120-something.
    // The bookmark is a boarding pass. Her name on it.
    add_wall(s, 2.5f, 0.66f, -4.6f, 0.22f, 0.04f, 0.15f, (Color){140,45,50,255});
    set_last_material(s, MAT_LEATHER);
    // Pages fanned slightly open — she put it down meaning to come back
    add_wall(s, 2.5f, 0.69f, -4.6f, 0.2f, 0.005f, 0.14f, cream);

    // ── HOTEL SLIPPERS — still wrapped, nobody coming ──
    // Second pair of slippers, plastic wrap implied by slight sheen
    add_wall(s, 0.6f, 0.03f, -3.5f, 0.1f, 0.04f, 0.2f, cream);
    set_last_material(s, MAT_FABRIC);
    add_wall(s, 0.8f, 0.03f, -3.5f, 0.1f, 0.04f, 0.2f, cream);
    set_last_material(s, MAT_FABRIC);

    // ── ROOM SERVICE CARD — two handwritings ──
    // On the desk. Your handwriting and hers, planning together.
    // Some items circled twice. A disagreement about the cheese plate.
    add_wall(s, 5.7f, 0.85f, -1.6f, 0.2f, 0.005f, 0.28f, cream);
    set_last_decal(s);
    // Her handwriting — slightly different colored ink
    add_wall(s, 5.7f, 0.856f, -1.5f, 0.08f, 0.003f, 0.12f, (Color){60,60,120,60});
    set_last_decal(s);

    // ── ONE SOCK UNDER THE BED — the most mundane object. The most real. ──
    add_wall(s, 0.8f, 0.05f, -4.0f, 0.08f, 0.02f, 0.15f, (Color){65,60,55,200});
    set_last_material(s, MAT_FABRIC);

    // Mint on nightstand — small bright green square
    add_wall(s, -2.3f, 0.86f, -4.7f, 0.08f, 0.03f, 0.08f, (Color){120,180,100,255});
    // Water carafe + glass — clear, formal
    add_cylinder(s, -2.5f, 0.92f, -4.3f, 0.06f, 0.14f, (Color){200,210,220,140});
    set_last_material(s, MAT_GLASS);
    add_cylinder(s, -2.35f, 0.88f, -4.3f, 0.04f, 0.08f, (Color){200,210,220,120});
    set_last_material(s, MAT_GLASS);

    // ============================================================
    // BOLAÑO OBJECTS — the abstract-mundane
    // These resist interpretation. They're the geometry textbook
    // on the washing line. Don't explain them.
    // ============================================================

    // Geometry textbook — open on the desk, spine cracked
    // Someone studying orbital mechanics for a conversation that never happened
    add_wall(s, 5.3f, 0.85f, -2.5f, 0.25f, 0.04f, 0.18f, (Color){50,75,120,255});  // cover
    set_last_material(s, MAT_LEATHER);
    add_wall(s, 5.3f, 0.875f, -2.5f, 0.23f, 0.005f, 0.16f, cream);  // pages open
    // Pencil marks — orbits drawn in margin
    add_wall(s, 5.4f, 0.877f, -2.45f, 0.06f, 0.002f, 0.002f, (Color){80,80,85,100});
    set_last_decal(s);
    add_wall(s, 5.35f, 0.877f, -2.55f, 0.04f, 0.002f, 0.003f, (Color){80,80,85,80});
    set_last_decal(s);

    // Half-written postcard — addressed to no one
    // Handwriting changes mood halfway through. Unreadable at 960x600.
    // That's the point.
    add_wall(s, 2.8f, 0.65f, -4.3f, 0.14f, 0.003f, 0.1f, cream);
    set_last_rotation(s, 12.0f);
    // Two ink tones — the mood shifted mid-sentence
    add_wall(s, 2.82f, 0.653f, -4.33f, 0.06f, 0.001f, 0.015f, (Color){30,30,60,70});
    set_last_decal(s);
    add_wall(s, 2.78f, 0.653f, -4.27f, 0.05f, 0.001f, 0.012f, (Color){60,30,30,50});
    set_last_decal(s);

    // ── Leading line — brass floor inlay guides eye to window ──
    // Narrow brass strip from entrance toward the left-wall window
    add_wall(s, -2, 0.01f, 0, 0.08f, 0.02f, rd*0.6f, brass);
    set_last_material(s, MAT_BRASS);
    set_last_decal(s);

    // ── Bath — the ritual of preparation ──
    // Freestanding tub near the window wall, facing Earth
    add_wall(s, -5.0f, 0.25f, -1.5f, 1.8f, 0.5f, 0.7f, (Color){245,243,238,255});  // tub body
    set_last_material(s, MAT_MARBLE);
    add_wall(s, -5.0f, 0.15f, -1.5f, 1.6f, 0.25f, 0.5f, (Color){180,210,230,100}); // water inside
    set_last_material(s, MAT_GLASS);
    // Brass taps
    add_cylinder(s, -5.6f, 0.6f, -1.4f, 0.04f, 0.15f, brass);
    set_last_material(s, MAT_BRASS);
    add_cylinder(s, -5.6f, 0.6f, -1.6f, 0.04f, 0.15f, brass);
    set_last_material(s, MAT_BRASS);

    // ── THERMOSTAT — a compromise temperature ──
    // Set to 22°. Her preference. You can change it.
    // Small brass rectangle on the wall near the door
    add_wall(s, 5.5f, 1.3f, 4.5f, 0.12f, 0.08f, 0.03f, brass);
    set_last_material(s, MAT_BRASS);
    // Digital display — small emissive rectangle
    add_wall(s, 5.5f, 1.3f, 4.48f, 0.06f, 0.04f, 0.01f, (Color){100,200,120,200});
    set_last_material(s, MAT_EMISSIVE);

    // Interactive objects
    add_object(s, -2.5f, 1.2f, -4.8f, "lamp", (Color){240,210,120,255}, 2);
    add_object(s, 5.5f, 1.0f, -2, "desk", (Color){200,155,90,255}, 2);
    add_object(s, -3, 0.5f, 3.5f, "champagne", gold, 2);
    add_object(s, 0, 0.8f, -4.5f, "bed", white, 2);
    add_object(s, -5.0f, 0.5f, -1.5f, "bath", (Color){180,210,230,200}, 2);
    add_object(s, 5.5f, 1.3f, 4.5f, "thermostat", brass, 1);

    tag_materials_by_color(s);

    s->spawn = (Vector3){0, 1.6f, 4};
    s->has_exit = false;

    // ── THE MOTIF: cigarette ──
    // On the coffee table, unlit. From the previous guest's effects.
    // You could light it. You won't.
    add_cylinder(s, -2.8f, 0.36f, 3.2f, 0.025f, 0.05f, (Color){230,225,215,220});

    // ── THE MOTIF: red book ──
    // On the nightstand, beside the face-down photograph. Always together.
    add_red_book(s, -2.6f, 0.86f, -4.2f, 5.0f);

}

void build_space_suite_cleaned(Scene *s) {
    build_space_suite(s);

    // The hotel has moved on. Housekeeping has erased her.
    // One pillow. One robe. One glass. The most violent cut in the montage.

    for (int i = 0; i < s->wall_count; i++) {
        Wall *w = &s->walls[i];

        // Robe 2 (hers) — at (6.8, 1.7, 2.6) — GONE
        if (fabsf(w->pos.x - 6.8f) < 0.15f && fabsf(w->pos.y - 1.7f) < 0.2f && fabsf(w->pos.z - 2.6f) < 0.2f)
            w->active = false;

        // Second champagne glass — cone at (-3.5, 0.39, 3.5) + cylinder at (-3.5, 0.44, 3.5) — GONE
        if (fabsf(w->pos.x + 3.5f) < 0.1f && fabsf(w->pos.z - 3.5f) < 0.1f && w->pos.y > 0.35f && w->pos.y < 0.5f)
            w->active = false;

        // Her book on nightstand — at (2.5, 0.66, -4.6) + pages at (2.5, 0.69, -4.6) — GONE
        if (fabsf(w->pos.x - 2.5f) < 0.1f && fabsf(w->pos.z + 4.6f) < 0.1f && w->pos.y > 0.6f && w->pos.y < 0.75f)
            w->active = false;

        // Second pair of slippers — at (0.6, 0.03, -3.5) and (0.8, 0.03, -3.5) — GONE
        if (w->pos.x > 0.5f && w->pos.x < 0.9f && fabsf(w->pos.y - 0.03f) < 0.02f && fabsf(w->pos.z + 3.5f) < 0.1f)
            w->active = false;

        // Room service card — at (5.7, 0.85, -1.6) + ink at (5.7, 0.856, -1.5) — GONE
        if (fabsf(w->pos.x - 5.7f) < 0.1f && w->pos.y > 0.84f && w->pos.y < 0.86f && w->pos.z > -1.7f && w->pos.z < -1.4f)
            w->active = false;

        // The sock under the bed — at (0.8, 0.05, -4.0) — GONE
        if (fabsf(w->pos.x - 0.8f) < 0.1f && fabsf(w->pos.y - 0.05f) < 0.02f && fabsf(w->pos.z + 4.0f) < 0.1f)
            w->active = false;

        // Her scarf on sofa arm — at (-4.2, 0.7, 2.0), red — GONE
        if (fabsf(w->pos.x + 4.2f) < 0.1f && fabsf(w->pos.y - 0.7f) < 0.15f && fabsf(w->pos.z - 2.0f) < 0.15f && w->color.r > 150)
            w->active = false;
    }

    // Remove the right pillow — at (0.6, 0.68, -5.2). One remains.
    for (int i = 0; i < s->wall_count; i++) {
        if (fabsf(s->walls[i].pos.x - 0.6f) < 0.15f &&
            fabsf(s->walls[i].pos.y - 0.68f) < 0.1f &&
            fabsf(s->walls[i].pos.z + 5.2f) < 0.15f) {
            s->walls[i].active = false;
            break;
        }
    }

    // Remove the duplicate right pillow — at (0.5, 0.72, -5.3)
    for (int i = 0; i < s->wall_count; i++) {
        if (fabsf(s->walls[i].pos.x - 0.5f) < 0.15f &&
            fabsf(s->walls[i].pos.y - 0.72f) < 0.1f &&
            fabsf(s->walls[i].pos.z + 5.3f) < 0.15f) {
            s->walls[i].active = false;
            break;
        }
    }
}

void build_space_hotel(Scene *s) {
    *s = (Scene){0};
    // BOUNDS: 50m x 40m, curved glass atrium — the Gehry hotel
    // Bilbao in orbit. The impossible space you didn't know you were inside.
    s->surface = SURFACE_MARBLE;

    Color hull = PAL_HULL;
    Color brass = PAL_BRASS;
    Color cream = PAL_CREAM;
    Color gold = PAL_GOLD;
    Color godard_red = PAL_RED;
    Color marble_a = PAL_MARBLE_A;
    Color warm_light = PAL_LIGHT_WARM;

    s->fog_color = (Color){4, 6, 14, 255};
    s->fog_density = 0.0004f;

    float aw = 50, ad = 40, ah = 20;  // atrium width, depth, height

    // ============================================================
    // FLOOR — single marble checkerboard plane
    // ============================================================
    add_wall(s, 0, -0.05f, 18, aw, 0.1f, ad, marble_a);
    set_last_material(s, MAT_CHECKERBOARD);

    // ============================================================
    // CEILING — hull panels with brass structural ribs
    // ============================================================
    add_wall(s, 0, ah, 18, aw, 0.3f, ad, hull);
    set_last_material(s, MAT_CONCRETE);
    // 4 brass ribs spanning the width
    for (int i = 0; i < 4; i++) {
        float rz = 2 + i * 11.0f;  // spread across z=2..35
        add_wall(s, 0, ah - 0.1f, rz, aw, 0.16f, 0.08f, brass);
        set_last_material(s, MAT_BRASS);
    }

    // ============================================================
    // END WALLS — hull with cream paneling
    // ============================================================
    // Spawn-side wall (z = -2)
    add_wall(s, 0, ah/2, -2, aw, ah, 0.4f, hull);
    set_last_material(s, MAT_CONCRETE);
    add_wall(s, 0, ah*0.25f, -1.78f, aw - 4, ah*0.5f, 0.05f, cream);
    set_last_material(s, MAT_WALLPAPER);
    // Exit-side wall (z = 38)
    add_wall(s, 0, ah/2, 38, aw, ah, 0.4f, hull);
    set_last_material(s, MAT_CONCRETE);
    add_wall(s, 0, ah*0.25f, 37.78f, aw - 4, ah*0.5f, 0.05f, cream);
    set_last_material(s, MAT_WALLPAPER);

    // ============================================================
    // CURVED GLASS ARCS — 24 panels per side, Gehry language
    // ============================================================
    float arc_radius = 30.0f;

    // Left arc — curving outward to the left
    for (int i = 0; i < 24; i++) {
        float t = (float)i / 23.0f;
        float angle = -0.35f + t * 0.7f;
        float px = -arc_radius + cosf(angle) * arc_radius;
        float pz = sinf(angle) * arc_radius + 18;
        float rot_deg = angle * (180.0f / PI);
        // Glass panel
        add_wall(s, px, 10, pz, 0.08f, 20, 2.0f, (Color){8, 12, 24, 180});
        set_last_material(s, MAT_GLASS);
        set_last_rotation(s, rot_deg);
        // Brass mullion between panels
        if (i > 0) {
            add_wall(s, px, 10, pz, 0.04f, 20, 0.06f, (Color){180, 155, 90, 255});
            set_last_material(s, MAT_BRASS);
            set_last_rotation(s, rot_deg);
        }
    }

    // Right arc — mirror, curving outward to the right
    for (int i = 0; i < 24; i++) {
        float t = (float)i / 23.0f;
        float angle = -0.35f + t * 0.7f;
        float px = arc_radius - cosf(angle) * arc_radius;
        float pz = sinf(angle) * arc_radius + 18;
        float rot_deg = -angle * (180.0f / PI);
        // Glass panel
        add_wall(s, px, 10, pz, 0.08f, 20, 2.0f, (Color){8, 12, 24, 180});
        set_last_material(s, MAT_GLASS);
        set_last_rotation(s, rot_deg);
        // Brass mullion
        if (i > 0) {
            add_wall(s, px, 10, pz, 0.04f, 20, 0.06f, (Color){180, 155, 90, 255});
            set_last_material(s, MAT_BRASS);
            set_last_rotation(s, rot_deg);
        }
    }

    // ============================================================
    // 8 SUPPORT COLUMNS — floor to ceiling, brass-capped
    // ============================================================
    float col_positions[][2] = {
        {-15, 6}, {-15, 18}, {-15, 30}, {-8, 18},
        {15, 6},  {15, 18},  {15, 30},  {8, 18},
    };
    for (int i = 0; i < 8; i++) {
        float cx = col_positions[i][0], cz = col_positions[i][1];
        add_column(s, cx, cz, 0.5f, ah, cream);
        // Brass caps — top and bottom
        add_cylinder(s, cx, 0.02f, cz, 1.2f, 0.06f, brass);
        add_cylinder(s, cx, ah - 0.02f, cz, 1.2f, 0.06f, brass);
    }

    // ============================================================
    // RED GRAND PIANO — Godard color event at center (z=12)
    // ============================================================
    Color piano_red = (Color){180, 40, 45, 255};
    // Body
    add_wall(s, -2, 0.7f, 12, 2.0f, 0.4f, 1.5f, piano_red);
    set_last_material(s, MAT_WOOD);
    // Lid — propped open, angled
    add_wall(s, -2, 1.1f, 12.2f, 2.0f, 0.06f, 1.2f, piano_red);
    set_last_material(s, MAT_WOOD);
    set_last_rotation(s, 8.0f);
    // Keys — thin white strip
    add_wall(s, -2, 0.9f, 11.2f, 1.8f, 0.04f, 0.2f, (Color){240, 238, 232, 255});
    // Black keys accent
    add_wall(s, -2, 0.92f, 11.22f, 1.6f, 0.04f, 0.1f, (Color){20, 20, 22, 255});
    // 3 brass legs
    add_cylinder(s, -2.8f, 0.35f, 11.4f, 0.08f, 0.7f, brass);
    add_cylinder(s, -1.2f, 0.35f, 11.4f, 0.08f, 0.7f, brass);
    add_cylinder(s, -2.0f, 0.35f, 12.6f, 0.08f, 0.7f, brass);
    // Warm light pool under the piano
    add_light_panel(s, -2, 0.3f, 12, 3.0f, 0.5f, 2.5f, warm_light);

    // ============================================================
    // BLUE SCULPTURE — Noguchi/Judd installation near exit (z=24)
    // ============================================================
    Color sculpt_blue = (Color){40, 60, 180, 255};
    // Brass plinth
    add_wall(s, 3, 0.15f, 24, 1.2f, 0.3f, 1.2f, brass);
    set_last_material(s, MAT_BRASS);
    // Cube base
    add_wall(s, 3, 0.8f, 24, 0.8f, 0.8f, 0.8f, sculpt_blue);
    // Tilted slab
    add_wall(s, 3, 1.6f, 24, 0.6f, 0.5f, 0.9f, sculpt_blue);
    set_last_rotation(s, 15.0f);
    // Sphere on top
    add_sphere(s, 3, 2.3f, 24, 0.5f, sculpt_blue);

    // ============================================================
    // PLANTED TERRACES — 4 raised beds, life persists in orbit
    // ============================================================
    Color soil = (Color){80, 60, 40, 255};
    Color plant_green = PAL_GREEN;
    float terrace_pos[][2] = {{-18, 8}, {-18, 28}, {18, 8}, {18, 28}};
    for (int i = 0; i < 4; i++) {
        float tx = terrace_pos[i][0], tz = terrace_pos[i][1];
        // Raised bed
        add_wall(s, tx, 0.3f, tz, 3.0f, 0.6f, 2.0f, soil);
        set_last_material(s, MAT_CONCRETE);
        // Vegetation — bold cubes and spheres
        add_wall(s, tx - 0.5f, 0.9f, tz, 0.6f, 0.6f, 0.6f, plant_green);
        add_sphere(s, tx + 0.6f, 1.0f, tz + 0.3f, 0.5f, plant_green);
    }

    // ============================================================
    // SEATING CLUSTERS — 3 groups
    // ============================================================
    // Cluster 1: Near spawn — two cream chairs facing each other
    // ── THE TWOS: designed for conversation. For two people. ──
    add_chair(s, -6, 0, 2, 0, brass, cream);
    add_chair(s, -6, 0, 4, 180, brass, cream);
    // Side table with two coasters
    add_wall(s, -6, 0.5f, 3, 0.5f, 0.04f, 0.5f, brass);
    set_last_material(s, MAT_BRASS);
    add_wall(s, -5.85f, 0.52f, 2.85f, 0.1f, 0.005f, 0.1f, (Color){180,155,90,100});  // coaster 1
    set_last_decal(s);
    add_wall(s, -6.15f, 0.52f, 3.15f, 0.1f, 0.005f, 0.1f, (Color){180,155,90,100});  // coaster 2
    set_last_decal(s);
    add_light_panel(s, -6, 3.5f, 3, 1.5f, 0.5f, 1.5f, warm_light);

    // Cluster 2: Near piano — navy sofa facing glass (Earth-watching)
    // ── THE TWOS: a sofa for two. Looking at Earth together. ──
    add_sofa(s, -8, 0, 14, 270, (Color){30, 40, 75, 255});
    add_wall(s, -9.2f, 0.5f, 14, 0.4f, 0.04f, 0.4f, brass);
    set_last_material(s, MAT_BRASS);
    // Two wine glasses on the table — the hotel assumes couples
    add_cone(s, -9.1f, 0.55f, 13.8f, 0.04f, 0.06f, (Color){210,210,215,160});
    add_cone(s, -9.3f, 0.55f, 14.2f, 0.04f, 0.06f, (Color){210,210,215,160});
    add_light_panel(s, -8, 3.5f, 14, 1.5f, 0.5f, 1.5f, warm_light);

    // Cluster 3: Near sculpture — single Godard red armchair
    // The only seat for one. You haven't found it yet.
    add_chair(s, 6, 0, 26, 180, brass, godard_red);
    add_wall(s, 6.6f, 0.5f, 26, 0.4f, 0.04f, 0.4f, brass);
    set_last_material(s, MAT_BRASS);
    add_light_panel(s, 6, 3.5f, 26, 1.5f, 0.5f, 1.5f, warm_light);

    // ============================================================
    // UPPER BALCONIES — 3 levels (y=5, y=10, y=15), visual only
    // ============================================================
    float balc_heights[] = {5.0f, 10.0f, 15.0f};
    for (int lv = 0; lv < 3; lv++) {
        float by = balc_heights[lv];
        // Left platform
        add_wall(s, -22, by, 18, 6.0f, 0.15f, 30.0f, hull);
        set_last_material(s, MAT_CONCRETE);
        // Right platform
        add_wall(s, 22, by, 18, 6.0f, 0.15f, 30.0f, hull);
        set_last_material(s, MAT_CONCRETE);
        // Brass railings — inner edge
        add_wall(s, -19, by + 0.5f, 18, 0.04f, 1.0f, 30.0f, brass);
        set_last_material(s, MAT_BRASS);
        add_wall(s, 19, by + 0.5f, 18, 0.04f, 1.0f, 30.0f, brass);
        set_last_material(s, MAT_BRASS);
    }
    // Level 1 detail: warm light pool + TWO chairs, TWO figures
    add_light_panel(s, -21, 5.4f, 12, 2.0f, 0.3f, 2.0f, warm_light);
    add_wall(s, -21.3f, 5.3f, 12, 0.5f, 0.5f, 0.5f, cream);
    add_wall(s, -20.5f, 5.3f, 12, 0.5f, 0.5f, 0.5f, cream);
    // Two figures — distant, sitting together. Other people's lives.
    add_wall(s, -21.3f, 5.9f, 12, 0.25f, 0.7f, 0.2f, (Color){45,40,38,180});
    add_wall(s, -20.5f, 5.9f, 12, 0.22f, 0.65f, 0.2f, (Color){45,40,38,160});
    // Level 2 detail: single warm lamp glow
    add_light_panel(s, 21, 10.4f, 20, 1.5f, 0.3f, 1.5f, warm_light);
    // Level 3 detail: coat rack silhouette
    add_wall(s, -21.5f, 15.6f, 25, 0.08f, 1.2f, 0.08f, (Color){60, 55, 50, 200});

    // ============================================================
    // FLOATING ZERO-G OBJECTS — storytelling debris
    // ============================================================
    // Napkin tumbling at y=6
    add_wall(s, 4, 6.0f, 10, 0.3f, 0.01f, 0.3f, (Color){245, 242, 238, 200});
    // Champagne cork at y=3.5
    add_cylinder(s, -5, 3.5f, 16, 0.06f, 0.08f, (Color){150, 120, 70, 255});
    // Rose petal at y=8
    add_wall(s, -10, 8.0f, 22, 0.15f, 0.01f, 0.12f, (Color){200, 80, 90, 180});
    // Room service menu at y=2 (brass-tinted)
    add_wall(s, 8, 2.0f, 20, 0.35f, 0.02f, 0.25f, (Color){180, 155, 90, 200});
    // Pen near the piano at y=4
    add_wall(s, -1, 4.0f, 11, 0.03f, 0.03f, 0.18f, gold);

    // ============================================================
    // BRASS FLOOR INLAY LINES — central walkway guide
    // ============================================================
    // Main central line from spawn to exit
    add_wall(s, 0, 0.01f, 18, 0.1f, 0.02f, 36.0f, brass);
    set_last_material(s, MAT_BRASS);
    set_last_decal(s);
    // Flanking accent lines
    add_wall(s, -1.5f, 0.01f, 18, 0.06f, 0.02f, 30.0f, brass);
    set_last_material(s, MAT_BRASS);
    set_last_decal(s);
    add_wall(s, 1.5f, 0.01f, 18, 0.06f, 0.02f, 30.0f, brass);
    set_last_material(s, MAT_BRASS);
    set_last_decal(s);
    // Compass rose at center — cross pattern
    add_wall(s, 0, 0.01f, 18, 2.0f, 0.02f, 0.08f, gold);
    set_last_material(s, MAT_BRASS);
    set_last_decal(s);

    // ============================================================
    // EARTH SPHERES — behind the glass arcs
    // ============================================================
    // Left arc: lower Earth, rising
    add_sphere(s, -28, 4, 14, 6.0f, (Color){35, 75, 140, 255});
    add_sphere(s, -28, 4, 14, 6.5f, (Color){120, 170, 220, 180});
    // Right arc: higher Earth
    add_sphere(s, 28, 10, 22, 5.5f, (Color){35, 75, 140, 255});
    add_sphere(s, 28, 10, 22, 6.0f, (Color){120, 170, 220, 180});

    // Earth glow decals — graduated blue wash on floor
    add_wall(s, -12, 0.02f, 14, 6.0f, 0.02f, 8.0f, (Color){50, 110, 190, 50});
    set_last_decal(s);
    add_wall(s, -6, 0.02f, 16, 8.0f, 0.02f, 6.0f, (Color){40, 90, 160, 25});
    set_last_decal(s);
    add_wall(s, 0, 0.02f, 18, 10.0f, 0.02f, 6.0f, (Color){30, 70, 130, 10});
    set_last_decal(s);
    // Ceiling reflections
    add_wall(s, -10, ah - 0.05f, 14, 4.0f, 0.02f, 5.0f, (Color){30, 65, 120, 15});
    set_last_decal(s);
    add_wall(s, 10, ah - 0.05f, 22, 4.0f, 0.02f, 5.0f, (Color){25, 55, 100, 10});
    set_last_decal(s);

    // Stars behind glass — tiny bright cubes far behind walls
    for (int i = 0; i < 20; i++) {
        float sx = -35 + (i * 41) % 70;
        float sy = 2 + (i * 17) % 16;
        float sz = -5 + (i * 13) % 45;
        add_wall(s, sx, sy, sz, 0.1f, 0.1f, 0.1f,
                 (Color){240, 238, 232, (unsigned char)(140 + (i * 23) % 80)});
    }

    // ============================================================
    // LIGHT PANELS — warm overhead pools
    // ============================================================
    add_light_panel(s, -8, ah - 1.0f, 10, 3.0f, 1.0f, 3.0f, warm_light);
    add_light_panel(s, 8, ah - 1.0f, 24, 3.0f, 1.0f, 3.0f, warm_light);
    add_light_panel(s, 0, ah - 1.0f, 18, 4.0f, 1.0f, 4.0f, warm_light);
    // Piano spotlight — tight warm pool
    add_light_panel(s, -2, 4.0f, 12, 2.0f, 0.5f, 2.0f, (Color){240, 195, 90, 150});
    // Exit beacon — warm panel above the corridor door
    add_light_panel(s, 0, 3.5f, 36, 2.0f, 0.5f, 1.0f, warm_light);

    // ============================================================
    // EXIT DOOR FRAME — at z=36
    // ============================================================
    add_door_frame(s, 0, 1.3f, 36.5f, 1.4f, 2.8f, 0.3f, brass);

    // ============================================================
    // VOLUMETRIC LIGHT SHAFTS — transparent boxes for atmosphere
    // ============================================================
    add_wall(s, -12, 8, 12, 4.0f, 14.0f, 3.0f, (Color){40, 90, 180, 6});
    add_wall(s, 12, 9, 24, 3.0f, 12.0f, 4.0f, (Color){35, 80, 160, 5});
    add_wall(s, 0, 7, 18, 5.0f, 10.0f, 5.0f, (Color){30, 70, 140, 3});

    // Interactive objects
    add_object(s, -2, 0.9f, 12, "piano", piano_red, 1);

    // ── The Motif: red book on the piano lid ──
    add_red_book(s, -1.5f, 1.12f, 12.5f, 10.0f);

    tag_materials_by_color(s);

    s->spawn = (Vector3){0, 1.6f, -2};
    s->exit_pos = (Vector3){0, 1.6f, 36};
    s->has_exit = true;
}

// ============================================================
// COMPOSITION HELPERS — Rich furniture from simple primitives
// ============================================================

void add_dining_table(Scene *s, float x, float y, float z, float w, float d, float angle, Color wood) {
    add_wall(s, x, y + S_TABLE_H, z, w, S_TABLE_TOP_T, d, wood);
    set_last_material(s, MAT_WOOD); set_last_rotation(s, angle);
    float lx = w/2 - S_TABLE_LEG_INS, lz = d/2 - S_TABLE_LEG_INS;
    float ca = cosf(angle * DEG2RAD), sa = sinf(angle * DEG2RAD);
    for (int i = 0; i < 4; i++) {
        float ox = (i & 1) ? lx : -lx;
        float oz = (i & 2) ? lz : -lz;
        float rx = ox * ca - oz * sa, rz = ox * sa + oz * ca;
        add_cylinder(s, x + rx, y + S_TABLE_H/2, z + rz, S_TABLE_LEG_D, S_TABLE_H, wood);
        set_last_material(s, MAT_WOOD);
    }
}

void add_chair(Scene *s, float x, float y, float z, float angle, Color wood, Color seat) {
    add_wall(s, x, y + S_CHAIR_SEAT_H, z, S_CHAIR_SEAT_W, S_CHAIR_SEAT_T, S_CHAIR_SEAT_D, seat);
    set_last_material(s, MAT_FABRIC); set_last_rotation(s, angle);
    float ca = cosf(angle * DEG2RAD), sa = sinf(angle * DEG2RAD);
    float boff = S_CHAIR_SEAT_D/2 - S_CHAIR_BACK_T/2;
    float bx = x - boff * sa, bz = z - boff * ca;
    add_wall(s, bx, y + S_CHAIR_SEAT_H + S_CHAIR_BACK_H/2, bz,
             S_CHAIR_SEAT_W - 0.04f, S_CHAIR_BACK_H, S_CHAIR_BACK_T, wood);
    set_last_material(s, MAT_WOOD); set_last_rotation(s, angle);
    for (int i = 0; i < 4; i++) {
        float ox = (i & 1) ? S_CHAIR_LEG_INS : -S_CHAIR_LEG_INS;
        float oz = (i & 2) ? S_CHAIR_LEG_INS : -S_CHAIR_LEG_INS;
        float rx = ox * ca - oz * sa, rz = ox * sa + oz * ca;
        add_cylinder(s, x + rx, y + S_CHAIR_SEAT_H/2, z + rz, S_CHAIR_LEG_D, S_CHAIR_SEAT_H, wood);
        set_last_material(s, MAT_WOOD);
    }
}

void add_chandelier(Scene *s, float x, float y, float z, int arms, float radius, Color metal, Color light) {
    add_cylinder(s, x, y + 0.3f, z, S_CHAN_ROD_D, 0.6f, metal);
    set_last_material(s, MAT_BRASS);
    for (int i = 0; i < arms; i++) {
        float angle = (float)i / arms * 6.2832f;
        float ax = x + cosf(angle) * radius;
        float az = z + sinf(angle) * radius;
        add_wall(s, (x + ax)/2, y, (z + az)/2, S_CHAN_ARM_T, S_CHAN_ARM_T, radius, metal);
        set_last_material(s, MAT_BRASS);
        set_last_rotation(s, angle * 57.2958f);
        add_sphere(s, ax, y - 0.1f, az, S_CHAN_GLOBE_D, light);
        add_light_panel(s, ax, y - 0.1f, az, S_CHAN_GLOBE_D+0.05f, S_CHAN_GLOBE_D+0.05f, S_CHAN_GLOBE_D+0.05f, light);
    }
}

void add_column_row(Scene *s, float x_start, float z, float spacing, int count, float radius, float height, Color c) {
    for (int i = 0; i < count; i++) {
        float x = x_start + i * spacing;
        add_cylinder(s, x, height/2, z, radius * 2, height, c);
        set_last_material(s, MAT_MARBLE);
        add_cylinder(s, x, height - S_COL_CAP_H/2, z, radius * 2 * S_COL_CAP_MULT, S_COL_CAP_H, c);
        set_last_material(s, MAT_MARBLE);
        add_cylinder(s, x, S_COL_BASE_H/2, z, radius * 2 * S_COL_BASE_MULT, S_COL_BASE_H, c);
        set_last_material(s, MAT_MARBLE);
    }
}

void add_wainscoting(Scene *s, float x, float y, float z, float length, float height, bool along_z, Color panel, Color trim) {
    if (along_z) {
        add_wall(s, x, y + height/2, z, S_WAINSCOT_D, height, length, panel);
        set_last_material(s, MAT_WOOD);
        add_wall(s, x, y + height, z, S_WAINSCOT_D + 0.03f, S_WAINSCOT_RAIL, length, trim);
        set_last_material(s, MAT_WOOD);
        add_wall(s, x, y + height * 0.6f, z, S_WAINSCOT_D + 0.02f, S_WAINSCOT_RAIL, length, trim);
        set_last_material(s, MAT_WOOD);
    } else {
        add_wall(s, x, y + height/2, z, length, height, S_WAINSCOT_D, panel);
        set_last_material(s, MAT_WOOD);
        add_wall(s, x, y + height, z, length, S_WAINSCOT_RAIL, S_WAINSCOT_D + 0.03f, trim);
        set_last_material(s, MAT_WOOD);
        add_wall(s, x, y + height * 0.6f, z, length, S_WAINSCOT_RAIL, S_WAINSCOT_D + 0.02f, trim);
        set_last_material(s, MAT_WOOD);
    }
}

void add_fireplace(Scene *s, float x, float y, float z, Color stone, Color glow) {
    add_wall(s, x, y + S_FIRE_MANTEL_H, z, S_FIRE_W, 0.12f, 0.5f, stone);
    set_last_material(s, MAT_MARBLE);
    float px = S_FIRE_W/2 - S_FIRE_PILLAR_W/2;
    add_wall(s, x - px, y + S_FIRE_MANTEL_H/2, z, S_FIRE_PILLAR_W, S_FIRE_MANTEL_H, 0.45f, stone);
    set_last_material(s, MAT_MARBLE);
    add_wall(s, x + px, y + S_FIRE_MANTEL_H/2, z, S_FIRE_PILLAR_W, S_FIRE_MANTEL_H, 0.45f, stone);
    set_last_material(s, MAT_MARBLE);
    add_wall(s, x, y + 0.5f, z + 0.05f, S_FIRE_W - S_FIRE_PILLAR_W*2, 1.0f, 0.35f, (Color){20,18,15,255});
    add_light_panel(s, x, y + 0.25f, z + 0.1f, 0.7f, 0.35f, 0.2f, glow);
}

void add_bar_counter(Scene *s, float x, float y, float z, float length, bool along_z, Color counter, Color front) {
    if (along_z) {
        add_wall(s, x, y + S_BAR_H, z, S_BAR_D + 0.12f, S_BAR_TOP_T, length, counter);
        set_last_material(s, MAT_MARBLE);
        add_wall(s, x, y + S_BAR_H/2, z, S_BAR_D, S_BAR_H, length, front);
        set_last_material(s, MAT_WOOD);
    } else {
        add_wall(s, x, y + S_BAR_H, z, length, S_BAR_TOP_T, S_BAR_D + 0.12f, counter);
        set_last_material(s, MAT_MARBLE);
        add_wall(s, x, y + S_BAR_H/2, z, length, S_BAR_H, S_BAR_D, front);
        set_last_material(s, MAT_WOOD);
    }
}

void add_rug(Scene *s, float x, float y, float z, float w, float d, Color primary, Color border) {
    add_wall(s, x, y + S_RUG_T, z, w, S_RUG_T, d, primary);
    set_last_material(s, MAT_CARPET);
    float bw = S_RUG_BORDER;
    add_wall(s, x, y + S_RUG_T + 0.005f, z - d/2 + bw/2, w, 0.01f, bw, border);
    set_last_decal(s);
    add_wall(s, x, y + S_RUG_T + 0.005f, z + d/2 - bw/2, w, 0.01f, bw, border);
    set_last_decal(s);
    add_wall(s, x - w/2 + bw/2, y + S_RUG_T + 0.005f, z, bw, 0.01f, d, border);
    set_last_decal(s);
    add_wall(s, x + w/2 - bw/2, y + S_RUG_T + 0.005f, z, bw, 0.01f, d, border);
    set_last_decal(s);
}

void add_desk(Scene *s, float x, float y, float z, float angle, Color wood) {
    float ca = cosf(angle * DEG2RAD), sa = sinf(angle * DEG2RAD);
    add_wall(s, x, y + S_DESK_H, z, S_DESK_W, S_DESK_TOP_T, S_DESK_D, wood);
    set_last_material(s, MAT_WOOD); set_last_rotation(s, angle);
    float foff = S_DESK_D/2 - S_DESK_PANEL_T/2;
    float fx = x + foff * sa, fz = z + foff * ca;
    add_wall(s, fx, y + S_DESK_H/2, fz, S_DESK_W - 0.06f, S_DESK_H, S_DESK_PANEL_T, wood);
    set_last_material(s, MAT_WOOD); set_last_rotation(s, angle);
    for (int side = -1; side <= 1; side += 2) {
        float sx = x + side * (S_DESK_W/2 - S_DESK_PANEL_T/2) * ca;
        float sz = z - side * (S_DESK_W/2 - S_DESK_PANEL_T/2) * sa;
        add_wall(s, sx, y + S_DESK_H/2, sz, S_DESK_PANEL_T, S_DESK_H, S_DESK_D - 0.06f, wood);
        set_last_material(s, MAT_WOOD); set_last_rotation(s, angle);
    }
}

void add_sofa(Scene *s, float x, float y, float z, float angle, Color fabric) {
    float ca = cosf(angle * DEG2RAD), sa = sinf(angle * DEG2RAD);
    Color dark = {(unsigned char)(fabric.r*0.75f), (unsigned char)(fabric.g*0.75f),
                  (unsigned char)(fabric.b*0.75f), fabric.a};
    add_wall(s, x, y + S_SOFA_SEAT_H, z, S_SOFA_W, S_SOFA_SEAT_T, S_SOFA_D, fabric);
    set_last_material(s, MAT_FABRIC); set_last_rotation(s, angle);
    float boff = S_SOFA_D/2 + S_SOFA_BACK_T/2 - 0.05f;
    float bx = x - boff * sa, bz = z - boff * ca;
    add_wall(s, bx, y + S_SOFA_SEAT_H + S_SOFA_BACK_H/2, bz,
             S_SOFA_W, S_SOFA_BACK_H, S_SOFA_BACK_T, dark);
    set_last_material(s, MAT_FABRIC); set_last_rotation(s, angle);
    float arm_off = S_SOFA_W/2 + S_SOFA_ARM_W/2 - 0.03f;
    float lx = x - arm_off * ca, lz = z + arm_off * sa;
    add_wall(s, lx, y + S_SOFA_SEAT_H + S_SOFA_ARM_H/2 - 0.05f, lz,
             S_SOFA_ARM_W, S_SOFA_ARM_H, S_SOFA_D + S_SOFA_BACK_T, dark);
    set_last_material(s, MAT_FABRIC); set_last_rotation(s, angle);
    float rx = x + arm_off * ca, rz = z - arm_off * sa;
    add_wall(s, rx, y + S_SOFA_SEAT_H + S_SOFA_ARM_H/2 - 0.05f, rz,
             S_SOFA_ARM_W, S_SOFA_ARM_H, S_SOFA_D + S_SOFA_BACK_T, dark);
    set_last_material(s, MAT_FABRIC); set_last_rotation(s, angle);
}
