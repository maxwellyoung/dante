// scene.c — Scene construction
// Material palette. Clean volumes. Dramatic light.
#include "scene.h"
#include "game_ctx.h"
#include "scale.h"
#include "palette.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifndef DEG2RAD
#define DEG2RAD 0.017453293f
#endif
#ifndef PI
#define PI 3.14159265358979f
#endif

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

void add_model(Scene *s, float x, float y, float z, float sx, float sy, float sz,
               float rotation_deg, int model_index, MaterialType mat, Color c) {
    if (s->wall_count >= MAX_WALLS) {
        fprintf(stderr, "[EV] WARNING: wall overflow (%d/%d) — model at (%.1f,%.1f,%.1f) dropped\n",
                s->wall_count, MAX_WALLS, x, y, z);
        return;
    }
    s->walls[s->wall_count++] = (Wall){
        .pos = {x, y, z}, .size = {sx, sy, sz}, .color = c,
        .active = true, .shape = SHAPE_MODEL,
        .material = mat, .rotation_y = rotation_deg,
        .model_index = model_index,
    };
}

// Find a loaded model asset by name — returns index or -1
int find_model_asset(const char *name) {
    // Access global game context
    extern GameCtx g;
    for (int i = 0; i < g.model_asset_count; i++) {
        if (strcmp(g.model_assets[i].name, name) == 0 && g.model_assets[i].loaded)
            return i;
    }
    return -1;
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

// Convenience: add_wall + auto-mark as decal (floor overlays, light shafts, puddles)
void add_wall_decal(Scene *s, float x, float y, float z, float w, float h, float d, Color c) {
    add_wall(s, x, y, z, w, h, d, c);
    set_last_decal(s);
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
    // Thin strip at floor level — flush against wall, so mark as decal
    float h = 0.08f;
    if (depth_axis > 0.01f) {
        add_wall(s, x, y + h/2, z, 0.03f, h, length, c);
    } else {
        add_wall(s, x, y + h/2, z, length, h, 0.03f, c);
    }
    set_last_material(s, MAT_WOOD);
    set_last_decal(s);
}

void add_crown_molding(Scene *s, float x, float y, float z,
                       float length, float along_z, Color c) {
    // Flush against wall/ceiling junction — mark as decal
    float h = 0.06f;
    if (along_z > 0.01f) {
        add_wall(s, x, y - h/2, z, 0.04f, h, length, c);
    } else {
        add_wall(s, x, y - h/2, z, length, h, 0.04f, c);
    }
    set_last_material(s, MAT_WOOD);
    set_last_decal(s);
}

void add_picture_frame(Scene *s, float x, float y, float z,
                       float w, float h, Color frame_color, Color canvas_color) {
    float t = 0.05f;
    // Canvas — sits against wall, mark as decal to avoid z-fight
    add_wall(s, x, y, z, w, h, 0.01f, canvas_color);
    set_last_material(s, MAT_FABRIC);
    set_last_decal(s);
    // Frame strips — slightly proud of canvas (0.02f offset), no z-fight
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
    // Glow halo — decal to avoid z-fight with core panel
    add_wall(s, x, y, z + (d > 0.2f ? 0 : 0.05f), w + 0.2f, h + 0.2f, d + 0.08f,
             (Color){c.r, c.g, c.b, 50});
    set_last_decal(s);
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

// ============================================================
// SCENES
// ============================================================

void build_hotel_exterior(Scene *s) {
    memset(s, 0, sizeof(Scene));
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
    add_wall(s, 28, 10, 22, 6, 20, 4, (Color){14, 16, 28, 255});
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

    // Additional street density
    // Parking meter
    add_cylinder(s, 6, 0.6f, -4, 0.05f, 1.2f, (Color){70,70,75,255});
    add_wall(s, 6, 1.3f, -4, 0.15f, 0.2f, 0.1f, (Color){80,80,85,255});
    // Fire hydrant
    add_cylinder(s, -10, 0.25f, -2, 0.12f, 0.5f, (Color){180,40,40,255});
    add_cylinder(s, -10, 0.55f, -2, 0.18f, 0.08f, (Color){180,40,40,255});
    // Bench on sidewalk
    add_wall(s, 5, 0.25f, -1, 1.5f, 0.06f, 0.5f, PAL_WOOD_DARK);
    set_last_material(s, MAT_WOOD);
    add_wall(s, 5, 0.55f, -0.73f, 1.5f, 0.5f, 0.06f, PAL_WOOD_DARK);
    set_last_material(s, MAT_WOOD);
    add_cylinder(s, 4.3f, 0.12f, -1, 0.04f, 0.25f, (Color){60,60,65,255});
    add_cylinder(s, 5.7f, 0.12f, -1, 0.04f, 0.25f, (Color){60,60,65,255});
    // Curb edge
    add_wall(s, 0, 0.05f, -7, 24, 0.1f, 0.2f, (Color){100,98,95,255});
    // Road center line dashes
    for (int i = 0; i < 8; i++) {
        add_wall(s, 0, -0.04f, -10-i*3.0f, 0.15f, 0.01f, 1.5f, (Color){220,210,60,100});
        set_last_decal(s);
    }
    // Newspaper box
    add_wall(s, -4, 0.5f, -2.5f, 0.4f, 1.0f, 0.35f, (Color){35,55,140,255});
    // Doorman's umbrella stand
    add_cylinder(s, 1.8f, 0.3f, 0.3f, 0.15f, 0.6f, PAL_BRASS);
    set_last_material(s, MAT_BRASS);
    // Potted plant by entrance
    add_cylinder(s, -2.0f, 0.35f, 0.3f, 0.3f, 0.7f, PAL_BRASS);
    set_last_material(s, MAT_BRASS);
    add_sphere(s, -2.0f, 0.85f, 0.3f, 0.5f, (Color){30,55,30,255});

    tag_materials_by_color(s);

    s->spawn = (Vector3){0, 1.6f, -4};
    s->exit_pos = (Vector3){0, 1.6f, 0.5f};
    s->has_exit = true;

    // ── THE MOTIF: cigarette ──
    // Someone just stepped inside. Still glowing on the wet pavement.
    add_cylinder(s, 1.2f, 0.01f, -4.0f, 0.1f, 0.45f, (Color){230,225,215,200});
    set_last_decal(s);
    add_wall(s, 1.22f, 0.015f, -4.0f, 0.1f, 0.06f, 0.1f, (Color){200,80,40,180});
    set_last_decal(s);

}

void build_lobby(Scene *s) {
    memset(s, 0, sizeof(Scene));
    // ================================================================
    // GRAND HOTEL LOBBY — Lutetia / Ritz at 2 AM
    // 20m wide (X) x 16m deep (Z) x 7.5m ceiling
    // Player enters from +Z (front doors), elevator at -Z (back wall)
    // Reception desk on the right, sitting area on the left
    // ================================================================
    s->surface = SURFACE_MARBLE;

    // Palette
    Color cream      = PAL_CREAM;
    Color white       = PAL_WHITE;
    Color gold        = PAL_BRASS;
    Color wood_dark   = PAL_WOOD_DARK;
    Color wood        = PAL_WOOD;
    Color marble_floor = {185, 178, 168, 255};   // warm grey veined marble
    Color marble_col  = PAL_MARBLE_A;
    Color wallpaper   = {215, 210, 200, 255};     // warm plaster / parchment
    Color leather_brn = PAL_LEATHER;
    Color burgundy    = {120, 35, 40, 255};       // deep Lutetia red
    Color navy        = PAL_NAVY;
    Color plant_green = {55, 115, 58, 255};
    Color pot_brass   = {165, 145, 95, 255};
    Color lamp_shade  = {235, 228, 215, 255};
    Color lamp_glow   = {240, 220, 160, 160};

    // Room dimensions
    float W = 20.0f, D = 16.0f, H = 7.5f;
    float hw = W / 2, hd = D / 2;

    s->fog_color = (Color){95, 82, 65, 255};  // warm amber haze
    s->fog_density = 0.003f;

    // ============================================================
    // FLOOR — polished marble
    // ============================================================
    add_wall(s, 0, -0.05f, 0, W, 0.1f, D, marble_floor);
    set_last_material(s, MAT_MARBLE);

    // ============================================================
    // CEILING — coffered panels
    // ============================================================
    add_wall(s, 0, H, 0, W, 0.2f, D, cream);
    set_last_material(s, MAT_WALLPAPER);
    // Coffered grid beams (4 across X, 3 across Z)
    for (int i = 0; i < 4; i++) {
        float bx = -hw + 2.5f + i * 5.0f;
        add_wall(s, bx, H - 0.15f, 0, 0.2f, 0.3f, D, wood_dark);
        set_last_material(s, MAT_WOOD);
    }
    for (int i = 0; i < 3; i++) {
        float bz = -hd + 4.0f + i * 5.33f;
        add_wall(s, 0, H - 0.15f, bz, W, 0.3f, 0.2f, wood_dark);
        set_last_material(s, MAT_WOOD);
    }

    // ============================================================
    // WALLS — lower wood paneling, upper plaster/wallpaper
    // Chair rail brass strip at 1.2m
    // ============================================================
    float panel_h = 1.2f;

    // Back wall (-Z) — upper
    add_wall(s, 0, panel_h + (H - panel_h) / 2, -hd, W, H - panel_h, 0.3f, wallpaper);
    set_last_material(s, MAT_WALLPAPER);
    // Back wall — lower wood paneling
    add_wall(s, 0, panel_h / 2, -hd, W, panel_h, 0.28f, wood_dark);
    set_last_material(s, MAT_WOOD);

    // Front wall (+Z) — with entrance gap in center (4m wide opening)
    float front_side_w = (W - 4.0f) / 2;
    // Left section
    add_wall(s, -hw + front_side_w / 2, (H) / 2, hd, front_side_w, H, 0.3f, wallpaper);
    set_last_material(s, MAT_WALLPAPER);
    add_wall(s, -hw + front_side_w / 2, panel_h / 2, hd - 0.01f, front_side_w, panel_h, 0.28f, wood_dark);
    set_last_material(s, MAT_WOOD);
    // Right section
    add_wall(s, hw - front_side_w / 2, (H) / 2, hd, front_side_w, H, 0.3f, wallpaper);
    set_last_material(s, MAT_WALLPAPER);
    add_wall(s, hw - front_side_w / 2, panel_h / 2, hd - 0.01f, front_side_w, panel_h, 0.28f, wood_dark);
    set_last_material(s, MAT_WOOD);
    // Transom above entrance
    add_wall(s, 0, H - 1.0f, hd, 4.0f, 2.0f, 0.3f, wallpaper);
    set_last_material(s, MAT_WALLPAPER);

    // Left wall (-X)
    add_wall(s, -hw, panel_h + (H - panel_h) / 2, 0, 0.3f, H - panel_h, D, wallpaper);
    set_last_material(s, MAT_WALLPAPER);
    add_wall(s, -hw, panel_h / 2, 0, 0.28f, panel_h, D, wood_dark);
    set_last_material(s, MAT_WOOD);

    // Right wall (+X)
    add_wall(s, hw, panel_h + (H - panel_h) / 2, 0, 0.3f, H - panel_h, D, wallpaper);
    set_last_material(s, MAT_WALLPAPER);
    add_wall(s, hw, panel_h / 2, 0, 0.28f, panel_h, D, wood_dark);
    set_last_material(s, MAT_WOOD);

    // ── Chair rail (brass strip at panel height) ──
    add_wall(s, 0, panel_h, -hd + 0.01f, W, 0.04f, 0.06f, gold);
    set_last_material(s, MAT_BRASS);
    add_wall(s, -hw + 0.01f, panel_h, 0, 0.06f, 0.04f, D, gold);
    set_last_material(s, MAT_BRASS);
    add_wall(s, hw - 0.01f, panel_h, 0, 0.06f, 0.04f, D, gold);
    set_last_material(s, MAT_BRASS);

    // ── Baseboards ──
    add_baseboard(s, 0, 0, -hd + 0.01f, W, 0, wood_dark);
    add_baseboard(s, -hw + 0.01f, 0, 0, D, 1.0f, wood_dark);
    add_baseboard(s, hw - 0.01f, 0, 0, D, 1.0f, wood_dark);

    // ── Crown molding ──
    add_crown_molding(s, 0, H, -hd + 0.01f, W, 0, wood);
    add_crown_molding(s, -hw + 0.01f, H, 0, D, 1.0f, wood);
    add_crown_molding(s, hw - 0.01f, H, 0, D, 1.0f, wood);
    add_crown_molding(s, 0, H, hd - 0.01f, W, 0, wood);

    // ============================================================
    // ENTRANCE DOOR FRAME — grand, arched
    // ============================================================
    add_door_frame(s, 0, 2.75f, hd, 4.0f, 5.5f, 0.35f, wood_dark);

    // ============================================================
    // COLUMNS — 6 marble columns, 3 per side, defining a central aisle
    // ============================================================
    add_column_row(s, -6, -4.5f, 6.0f, 3, 0.4f, H, marble_col);
    add_column_row(s, -6, 4.5f, 6.0f, 3, 0.4f, H, marble_col);

    // ============================================================
    // CHANDELIER — hero light, center of lobby
    // ============================================================
    add_chandelier(s, 0, H - 1.5f, 0, 8, 1.2f, gold, PAL_LIGHT_WARM);

    // ============================================================
    // RECEPTION DESK — right side, facing entrance
    // Long polished wood counter with brass edge strip
    // ============================================================
    float desk_x = 6.0f, desk_z = -2.0f;
    // Main counter body
    add_wall(s, desk_x, 0.45f, desk_z, 1.2f, 0.9f, 5.0f, wood_dark);
    set_last_material(s, MAT_WOOD);
    // Counter top — polished
    add_wall(s, desk_x, 0.92f, desk_z, 1.3f, 0.04f, 5.1f, wood);
    set_last_material(s, MAT_WOOD);
    // Brass edge strip along counter top
    add_wall(s, desk_x - 0.6f, 0.92f, desk_z, 0.04f, 0.06f, 5.1f, gold);
    set_last_material(s, MAT_BRASS);

    // ── Desk lamp (left end of counter) ──
    add_cylinder(s, desk_x - 0.2f, 0.96f, desk_z + 2.0f, 0.08f, 0.03f, gold);
    set_last_material(s, MAT_BRASS);
    add_cylinder(s, desk_x - 0.2f, 1.08f, desk_z + 2.0f, 0.03f, 0.2f, gold);
    set_last_material(s, MAT_BRASS);
    add_cone(s, desk_x - 0.2f, 1.22f, desk_z + 2.0f, 0.22f, 0.1f, lamp_shade);
    add_light_panel(s, desk_x - 0.2f, 1.15f, desk_z + 2.0f, 0.3f, 0.2f, 0.3f, lamp_glow);

    // ── Hotel bell on desk ──
    add_cylinder(s, desk_x - 0.3f, 0.96f, desk_z + 0.5f, 0.1f, 0.02f, gold);
    set_last_material(s, MAT_BRASS);
    add_sphere(s, desk_x - 0.3f, 1.0f, desk_z + 0.5f, 0.08f, gold);
    set_last_material(s, MAT_BRASS);
    add_object(s, desk_x - 0.3f, 1.05f, desk_z + 0.5f, "bell", gold, 1);

    // ── Guest register (open book) ──
    add_wall(s, desk_x - 0.2f, 0.95f, desk_z, 0.5f, 0.03f, 0.35f, (Color){180,170,150,255});
    set_last_material(s, MAT_LEATHER);

    // ── Key left on desk — someone checked in ──
    add_wall(s, desk_x - 0.1f, 0.95f, desk_z - 0.8f, 0.08f, 0.02f, 0.04f, gold);
    set_last_material(s, MAT_BRASS);

    // ============================================================
    // KEY RACK — behind reception desk, on back wall area
    // Brass hooks with numbered slots
    // ============================================================
    float rack_x = hw - 0.4f, rack_z = desk_z;
    // Backing board
    add_wall(s, rack_x, 2.0f, rack_z, 0.08f, 1.6f, 3.0f, wood_dark);
    set_last_material(s, MAT_WOOD);
    // Brass hooks — 3 rows of 5
    for (int row = 0; row < 3; row++) {
        for (int col = 0; col < 5; col++) {
            float hy = 1.4f + row * 0.5f;
            float hz = desk_z - 1.2f + col * 0.55f;
            add_wall(s, rack_x - 0.06f, hy, hz, 0.04f, 0.04f, 0.03f, gold);
            set_last_material(s, MAT_BRASS);
        }
    }
    // A few keys hanging (small brass rectangles dangling from hooks)
    add_wall(s, rack_x - 0.08f, 1.25f, desk_z - 1.2f, 0.03f, 0.12f, 0.02f, gold);
    set_last_material(s, MAT_BRASS);
    add_wall(s, rack_x - 0.08f, 1.75f, desk_z + 0.5f, 0.03f, 0.12f, 0.02f, gold);
    set_last_material(s, MAT_BRASS);
    add_wall(s, rack_x - 0.08f, 2.25f, desk_z - 0.1f, 0.03f, 0.12f, 0.02f, gold);
    set_last_material(s, MAT_BRASS);

    // ============================================================
    // CLOCK — on wall behind reception, above key rack
    // ============================================================
    add_cylinder(s, rack_x - 0.02f, 3.5f, desk_z, 0.6f, 0.06f, wood_dark);
    set_last_material(s, MAT_WOOD);
    add_cylinder(s, rack_x - 0.05f, 3.5f, desk_z, 0.55f, 0.03f, white);
    // Clock hands (thin dark lines)
    add_wall(s, rack_x - 0.07f, 3.6f, desk_z, 0.02f, 0.2f, 0.02f, PAL_BLACK);
    add_wall(s, rack_x - 0.07f, 3.5f, desk_z + 0.05f, 0.02f, 0.02f, 0.15f, PAL_BLACK);

    // ============================================================
    // ELEVATOR — brass doors on back wall, center
    // ============================================================
    float elev_x = 0.0f, elev_z = -hd + 0.15f;
    // Dark recess behind doors
    add_wall(s, elev_x, 1.5f, -hd + 0.05f, 2.0f, 3.0f, 0.08f, PAL_BLACK);
    // Left door
    add_wall(s, elev_x - 0.55f, 1.5f, elev_z, 0.9f, 3.0f, 0.08f, gold);
    set_last_material(s, MAT_BRASS);
    // Right door
    add_wall(s, elev_x + 0.55f, 1.5f, elev_z, 0.9f, 3.0f, 0.08f, gold);
    set_last_material(s, MAT_BRASS);
    // Gap between doors
    add_wall(s, elev_x, 1.5f, elev_z - 0.02f, 0.06f, 3.0f, 0.04f, PAL_BLACK);
    // Frame
    add_door_frame(s, elev_x, 1.5f, elev_z, 2.2f, 3.0f, 0.1f, gold);
    // Floor indicator above elevator — small lit panel
    add_wall(s, elev_x, 3.3f, elev_z + 0.02f, 0.6f, 0.25f, 0.04f, PAL_BLACK);
    add_wall(s, elev_x, 3.3f, elev_z + 0.04f, 0.5f, 0.18f, 0.02f, (Color){200, 160, 80, 140});
    // Interactive trigger
    add_object(s, elev_x, 1.5f, elev_z + 0.4f, "elevator", gold, 1);

    // ── GRAND PIANO — right side of lobby, the muffled piano source ──
    {
        int piano_mdl = find_model_asset("piano");
        if (piano_mdl >= 0) {
            add_model(s, 5.0f, 0, 2.0f, 1,1,1, -30, piano_mdl, MAT_WOOD, (Color){255,255,255,255});
        } else {
            // Fallback: simplified piano silhouette (boxes)
            add_wall(s, 5.0f, 0.5f, 2.0f, 1.5f, 0.3f, 1.0f, PAL_BLACK);
            set_last_material(s, MAT_WOOD);
            // Lid propped
            add_wall(s, 5.0f, 0.85f, 2.3f, 1.4f, 0.02f, 0.6f, PAL_BLACK);
            set_last_material(s, MAT_WOOD);
        }
    }

    // ============================================================
    // SITTING AREA — left side of lobby
    // Two leather armchairs facing each other, side table between
    // Reading lamp on side table
    // ============================================================
    float sit_x = -6.5f, sit_z = 2.0f;

    // Armchair 1 — facing right (+X)
    add_chair(s, sit_x - 0.8f, 0, sit_z, 90, wood_dark, leather_brn);
    // Armchair 2 — facing left (-X)
    add_chair(s, sit_x + 0.8f, 0, sit_z, -90, wood_dark, leather_brn);

    // Side table between chairs
    add_wall(s, sit_x, 0.35f, sit_z, 0.5f, 0.04f, 0.5f, wood_dark);
    set_last_material(s, MAT_WOOD);
    // Table legs
    for (int i = 0; i < 4; i++) {
        float lx = sit_x + ((i & 1) ? 0.18f : -0.18f);
        float lz = sit_z + ((i & 2) ? 0.18f : -0.18f);
        add_cylinder(s, lx, 0.17f, lz, 0.03f, 0.34f, wood_dark);
        set_last_material(s, MAT_WOOD);
    }

    // Reading lamp on side table
    add_cylinder(s, sit_x, 0.39f, sit_z, 0.06f, 0.03f, gold);
    set_last_material(s, MAT_BRASS);
    add_cylinder(s, sit_x, 0.52f, sit_z, 0.02f, 0.22f, gold);
    set_last_material(s, MAT_BRASS);
    add_cone(s, sit_x, 0.66f, sit_z, 0.18f, 0.1f, lamp_shade);
    add_light_panel(s, sit_x, 0.58f, sit_z, 0.25f, 0.2f, 0.25f, lamp_glow);

    // ── Rug under sitting area ──
    add_rug(s, sit_x, 0, sit_z, 3.5f, 3.0f, burgundy, gold);

    // ============================================================
    // CONSOLE TABLE + MIRROR — against left wall
    // ============================================================
    float con_x = -hw + 0.4f, con_z = -3.0f;
    // Console table
    add_wall(s, con_x, 0.4f, con_z, 0.35f, 0.04f, 1.5f, wood_dark);
    set_last_material(s, MAT_WOOD);
    // Console legs (tapered — just thin cylinders)
    add_cylinder(s, con_x, 0.2f, con_z - 0.6f, 0.03f, 0.4f, wood_dark);
    set_last_material(s, MAT_WOOD);
    add_cylinder(s, con_x, 0.2f, con_z + 0.6f, 0.03f, 0.4f, wood_dark);
    set_last_material(s, MAT_WOOD);
    // Mirror above console — brass frame
    add_picture_frame(s, con_x + 0.02f, 2.2f, con_z, 1.0f, 1.4f, gold, (Color){150, 155, 160, 200});

    // ── Vase on console table ──
    add_cylinder(s, con_x + 0.02f, 0.55f, con_z, 0.1f, 0.25f, (Color){180, 175, 165, 255});
    set_last_material(s, MAT_MARBLE);

    // ============================================================
    // LUGGAGE TROLLEY — near entrance, left of doors
    // Brass frame cart with dark wooden platform
    // ============================================================
    float lug_x = -2.5f, lug_z = 5.5f;
    // Platform
    add_wall(s, lug_x, 0.35f, lug_z, 1.0f, 0.04f, 0.6f, wood_dark);
    set_last_material(s, MAT_WOOD);
    // Frame rails (brass)
    add_wall(s, lug_x, 0.55f, lug_z - 0.28f, 1.0f, 0.03f, 0.03f, gold);
    set_last_material(s, MAT_BRASS);
    add_wall(s, lug_x, 0.55f, lug_z + 0.28f, 1.0f, 0.03f, 0.03f, gold);
    set_last_material(s, MAT_BRASS);
    // Vertical posts
    for (int i = 0; i < 4; i++) {
        float px = lug_x + ((i & 1) ? 0.45f : -0.45f);
        float pz = lug_z + ((i & 2) ? 0.25f : -0.25f);
        add_cylinder(s, px, 0.45f, pz, 0.025f, 0.55f, gold);
        set_last_material(s, MAT_BRASS);
    }
    // Handle bar on top
    add_wall(s, lug_x, 0.75f, lug_z, 1.0f, 0.025f, 0.025f, gold);
    set_last_material(s, MAT_BRASS);
    // Wheels
    add_cylinder(s, lug_x - 0.4f, 0.06f, lug_z, 0.12f, 0.04f, PAL_CHARCOAL);
    add_cylinder(s, lug_x + 0.4f, 0.06f, lug_z, 0.12f, 0.04f, PAL_CHARCOAL);
    // Suitcase ON the trolley — someone's luggage waiting
    add_wall(s, lug_x, 0.5f, lug_z, 0.6f, 0.28f, 0.4f, leather_brn);
    set_last_material(s, MAT_LEATHER);
    add_wall(s, lug_x, 0.66f, lug_z, 0.62f, 0.02f, 0.42f, gold);
    set_last_material(s, MAT_BRASS);

    // ============================================================
    // POTTED PLANTS — large, in brass pots
    // ============================================================
    // Near elevator, left side
    add_cylinder(s, -3.5f, 0.25f, -6.0f, 0.5f, 0.5f, pot_brass);
    set_last_material(s, MAT_BRASS);
    add_sphere(s, -3.5f, 0.9f, -6.0f, 0.7f, plant_green);
    set_last_material(s, MAT_FABRIC);  // fabric approximates leaf texture
    add_sphere(s, -3.5f, 1.2f, -6.0f, 0.5f, plant_green);
    set_last_material(s, MAT_FABRIC);

    // Near entrance, right side
    add_cylinder(s, 3.5f, 0.25f, 6.0f, 0.5f, 0.5f, pot_brass);
    set_last_material(s, MAT_BRASS);
    add_sphere(s, 3.5f, 0.9f, 6.0f, 0.7f, plant_green);
    set_last_material(s, MAT_FABRIC);
    add_sphere(s, 3.5f, 1.2f, 6.0f, 0.5f, plant_green);
    set_last_material(s, MAT_FABRIC);

    // ============================================================
    // COAT RACK / UMBRELLA STAND — near entrance, right
    // ============================================================
    float coat_x = 7.5f, coat_z = 6.5f;
    // Stand pole
    add_cylinder(s, coat_x, 0.75f, coat_z, 0.04f, 1.5f, wood_dark);
    set_last_material(s, MAT_WOOD);
    // Base disc
    add_cylinder(s, coat_x, 0.04f, coat_z, 0.3f, 0.08f, wood_dark);
    set_last_material(s, MAT_WOOD);
    // Hooks (4 brass nubs at top)
    for (int i = 0; i < 4; i++) {
        float a = i * 1.5708f;
        add_wall(s, coat_x + cosf(a) * 0.1f, 1.5f, coat_z + sinf(a) * 0.1f,
                 0.04f, 0.04f, 0.04f, gold);
        set_last_material(s, MAT_BRASS);
    }
    // Forgotten coat draped on one hook — dark wool
    add_wall(s, coat_x + 0.08f, 1.1f, coat_z, 0.05f, 0.7f, 0.25f, (Color){35,32,28,255});
    set_last_material(s, MAT_FABRIC);

    // Umbrella stand — brass cylinder beside coat rack
    add_cylinder(s, coat_x + 0.5f, 0.3f, coat_z, 0.2f, 0.6f, pot_brass);
    set_last_material(s, MAT_BRASS);
    // Umbrella poking out
    add_cylinder(s, coat_x + 0.5f, 0.5f, coat_z, 0.02f, 0.7f, PAL_CHARCOAL);

    // ============================================================
    // WALL SCONCES — warm brass, every ~3.5m along side walls
    // ============================================================
    for (int i = 0; i < 4; i++) {
        float sz = -hd + 2.5f + i * 3.8f;
        // Left wall sconces
        add_wall(s, -hw + 0.2f, 2.8f, sz, 0.08f, 0.15f, 0.12f, gold);
        set_last_material(s, MAT_BRASS);
        add_light_panel(s, -hw + 0.25f, 2.9f, sz, 0.15f, 0.2f, 0.15f, lamp_glow);
        // Right wall sconces
        add_wall(s, hw - 0.2f, 2.8f, sz, 0.08f, 0.15f, 0.12f, gold);
        set_last_material(s, MAT_BRASS);
        add_light_panel(s, hw - 0.25f, 2.9f, sz, 0.15f, 0.2f, 0.15f, lamp_glow);
    }

    // ============================================================
    // PICTURE FRAMES — on back wall, flanking elevator
    // ============================================================
    // Godard red — left of elevator
    add_picture_frame(s, -4.5f, 3.0f, -hd + 0.18f, 1.2f, 0.9f, gold, (Color){195,45,40,255});
    // French blue — right of elevator
    add_picture_frame(s, 4.5f, 3.0f, -hd + 0.18f, 1.2f, 0.9f, gold, (Color){40,65,160,255});
    // Smaller frame — near sitting area wall
    add_picture_frame(s, -hw + 0.18f, 2.8f, 0, 0.8f, 0.6f, wood_dark, (Color){170,155,130,255});

    // ============================================================
    // WAINSCOTING — decorative panels on back wall sections
    // ============================================================
    // Left of elevator
    add_wainscoting(s, -5.5f, 0, -hd + 0.18f, 6.0f, panel_h, false, wood_dark, wood);
    // Right of elevator
    add_wainscoting(s, 5.5f, 0, -hd + 0.18f, 6.0f, panel_h, false, wood_dark, wood);

    // ============================================================
    // LIGHT SHAFT — light pouring in from entrance onto lobby floor
    // ============================================================
    add_wall(s, 0, 0.02f, 6.5f, 3.5f, 0.02f, 3.0f, (Color){235,230,218,80});
    set_last_decal(s);
    // Subtle light band on back wall from chandelier
    add_wall(s, 0, 4.5f, -hd + 0.18f, 6.0f, 0.6f, 0.02f, (Color){245,240,230,60});
    set_last_decal(s);

    // ============================================================
    // EXIT SIGN — small red, above entrance
    // ============================================================
    add_wall(s, 0, H - 0.5f, hd - 0.15f, 0.4f, 0.15f, 0.04f, PAL_BLACK);
    add_wall(s, 0, H - 0.5f, hd - 0.13f, 0.35f, 0.1f, 0.02f, (Color){200,40,35,180});

    // ============================================================
    // FIRE EXTINGUISHER — on right wall
    // ============================================================
    add_cylinder(s, hw - 0.2f, 1.3f, 3.0f, 0.08f, 0.35f, (Color){190,40,35,255});
    // Bracket
    add_wall(s, hw - 0.18f, 1.3f, 3.0f, 0.04f, 0.1f, 0.12f, gold);
    set_last_material(s, MAT_BRASS);

    // ============================================================
    // VENTILATION GRILLE — near ceiling, right wall
    // ============================================================
    add_wall(s, hw - 0.16f, H - 0.8f, -3.0f, 0.04f, 0.4f, 0.8f, (Color){160,158,155,255});

    // ============================================================
    // SOFA — against left wall, behind sitting chairs
    // ============================================================
    add_sofa(s, -hw + 1.2f, 0, -1.0f, 90, burgundy);

    // ============================================================
    // EVIDENCE OF DEPARTURE — the lobby is populated by absence
    // Gravity Bone: objects tell stories without words
    // ============================================================

    // Abandoned suitcase by entrance — someone left without it
    add_wall(s, 1.5f, 0.15f, 6.5f, 0.6f, 0.3f, 0.4f, (Color){95,60,30,255});
    set_last_material(s, MAT_LEATHER);
    add_wall(s, 1.5f, 0.32f, 6.5f, 0.62f, 0.02f, 0.42f, gold);
    set_last_material(s, MAT_BRASS);
    set_last_decal(s);

    // Newspaper on armchair — someone was reading
    add_wall(s, sit_x - 0.6f, 0.42f, sit_z + 0.1f, 0.5f, 0.02f, 0.35f, (Color){235,232,228,255});
    add_object(s, sit_x - 0.6f, 0.5f, sit_z + 0.1f, "newspaper", (Color){235,232,228,255}, 1);

    // Spilled coffee ring near sitting area
    add_wall(s, sit_x + 0.1f, 0.01f, sit_z - 0.5f, 0.3f, 0.01f, 0.3f, (Color){60,45,30,50});
    set_last_decal(s);

    // ── THE MOTIF: cigarette ──
    // On the console table. The lobby is non-smoking. Someone didn't care.
    add_cylinder(s, con_x + 0.15f, 0.44f, con_z + 0.3f, 0.1f, 0.45f, (Color){230,225,215,200});
    set_last_decal(s);
    // Ashtray — small brass dish (shouldn't exist in a hotel lobby)
    add_cylinder(s, con_x + 0.15f, 0.42f, con_z + 0.3f, 0.08f, 0.02f, gold);
    set_last_material(s, MAT_BRASS);

    // ── Second cigarette — by the entrance, in a crack of marble ──
    add_cylinder(s, 1.8f, 0.02f, 7.0f, 0.1f, 0.45f, (Color){230,225,215,200});
    set_last_decal(s);
    add_wall(s, 1.81f, 0.025f, 7.0f, 0.1f, 0.06f, 0.1f, (Color){200,80,40,160});
    set_last_decal(s);

    // ============================================================
    // ADDITIONAL DENSITY — small touches that make it feel real
    // ============================================================

    // Rope stanchion near elevator (velvet rope on brass poles)
    add_cylinder(s, -2.5f, 0.45f, -5.5f, 0.06f, 0.9f, gold);
    set_last_material(s, MAT_BRASS);
    add_cylinder(s, 2.5f, 0.45f, -5.5f, 0.06f, 0.9f, gold);
    set_last_material(s, MAT_BRASS);
    add_wall(s, 0, 0.85f, -5.5f, 5.0f, 0.04f, 0.04f, burgundy);
    set_last_material(s, MAT_VELVET);
    // Sphere finials on stanchion poles
    add_sphere(s, -2.5f, 0.95f, -5.5f, 0.08f, gold);
    set_last_material(s, MAT_BRASS);
    add_sphere(s, 2.5f, 0.95f, -5.5f, 0.08f, gold);
    set_last_material(s, MAT_BRASS);

    // Small rug in front of elevator
    add_rug(s, elev_x, 0, -5.0f, 2.5f, 2.0f, navy, gold);

    // Second side table — near sofa, with another lamp
    add_wall(s, -hw + 1.2f, 0.35f, -2.8f, 0.4f, 0.04f, 0.4f, wood_dark);
    set_last_material(s, MAT_WOOD);
    add_cylinder(s, -hw + 1.2f, 0.17f, -2.6f, 0.03f, 0.34f, wood_dark);
    set_last_material(s, MAT_WOOD);
    add_cylinder(s, -hw + 1.2f, 0.17f, -3.0f, 0.03f, 0.34f, wood_dark);
    set_last_material(s, MAT_WOOD);

    // Room number plaque beside elevator
    add_wall(s, elev_x + 1.8f, 2.0f, -hd + 0.18f, 0.3f, 0.15f, 0.03f, gold);
    set_last_material(s, MAT_BRASS);

    // Additional picture frames on side walls
    add_picture_frame(s, hw - 0.18f, 3.0f, -5.0f, 0.9f, 0.7f, gold, navy);
    add_picture_frame(s, hw - 0.18f, 3.0f, 1.0f, 0.7f, 0.9f, wood_dark, burgundy);

    // Bookshelf behind reception — visible past the counter
    add_bookshelf(s, hw - 0.3f, 2.5f, desk_z - 1.5f, 1.2f, 2.0f, 3, wood_dark);

    // Dropped ceiling detail over reception area
    add_dropped_ceiling(s, desk_x, H, desk_z, 3.0f, 5.0f, 0.25f, cream, gold);

    // Bell desk sign — small brass plate on reception counter
    add_wall(s, desk_x - 0.3f, 0.96f, desk_z + 1.2f, 0.25f, 0.12f, 0.02f, gold);
    set_last_material(s, MAT_BRASS);

    // ============================================================
    // TABLE FOR TWO — by the window, near entrance
    // Commandment 9: the game is full of twos and the player is one.
    // A small café table with two chairs. Two coffee cups.
    // The chairs face each other. One has a coat draped over it.
    // ============================================================
    {
        float tx = 6.5f, tz = 5.0f;  // right side, near entrance

        // Small round table — brass pedestal
        add_wall(s, tx, 0.4f, tz, 0.7f, 0.03f, 0.7f, gold);
        set_last_material(s, MAT_BRASS);
        add_cylinder(s, tx, 0.2f, tz, 0.06f, 0.4f, gold);
        set_last_material(s, MAT_BRASS);
        add_cylinder(s, tx, 0.01f, tz, 0.2f, 0.02f, gold);
        set_last_material(s, MAT_BRASS);

        // TWO CHAIRS — facing each other
        add_chair(s, tx, 0, tz - 0.7f, 0, wood_dark, leather_brn);
        add_chair(s, tx, 0, tz + 0.7f, 180, wood_dark, leather_brn);

        // Two coffee cups — one used (dark inside), one untouched
        add_cylinder(s, tx - 0.15f, 0.43f, tz - 0.1f, 0.07f, 0.1f, white);
        add_cylinder(s, tx - 0.15f, 0.44f, tz - 0.1f, 0.05f, 0.03f, (Color){45,30,20,200});
        add_cylinder(s, tx + 0.15f, 0.43f, tz + 0.1f, 0.07f, 0.1f, white);
        // Second cup: still full, untouched — she never came down for breakfast

        // Coat draped over second chair back — someone was expected
        add_wall(s, tx, 0.65f, tz + 0.75f, 0.4f, 0.5f, 0.06f, (Color){55,48,42,220});
        set_last_material(s, MAT_FABRIC);
    }

    // ============================================================
    // BLOCKING — seal gaps to prevent escape into void
    // ============================================================
    // Front wall gap is handled by wall sections + transom above
    // Extra seal behind back wall
    add_wall(s, 0, H / 2, -hd - 0.5f, W + 2, H + 1, 0.3f, PAL_BLACK);

    s->spawn = (Vector3){0, 1.6f, 7.0f};
    s->exit_pos = (Vector3){elev_x, 1.6f, -5.5f};
    s->has_exit = true;
}

void build_hallway(Scene *s) {
    memset(s, 0, sizeof(Scene));
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
    add_wall_decal(s, 0, 0.02f, -L+2, 1.5f, 0.02f, 2.0f, (Color){230,225,215,90});
    add_wall_decal(s, 0, 0.02f, -L+4, 2.0f, 0.02f, 2.0f, (Color){230,225,215,70});
    add_wall_decal(s, 0, 0.02f, -L+6, 2.5f, 0.02f, 2.0f, (Color){230,225,215,50});

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
    memset(s, 0, sizeof(Scene));
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
    add_wall(s, 5.2f, 0.85f, 0.15f, 0.12f, 0.02f, 0.06f, (Color){35,35,38,255});

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
    add_wall(s, 4.5f, 0.85f, -0.3f, 0.4f, 0.01f, 0.2f, (Color){245,242,235,255});  // paper
    add_wall(s, 4.5f, 0.86f, -0.35f, 0.4f, 0.005f, 0.04f, (Color){50,80,180,255});  // blue stripe

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
}

void build_balcony(Scene *s) {
    memset(s, 0, sizeof(Scene));
    // OBSERVATION DECK — open to the void, Earth below
    // Parisian balcony furniture, infinite space outside
    s->surface = SURFACE_MARBLE;

    s->fog_color = (Color){2, 3, 8, 255};  // deep space
    s->fog_density = 0.001f;  // almost no fog — vacuum clarity

    Color stone = PAL_MARBLE_A;
    Color railing = PAL_MID;
    Color gold = PAL_BRASS;
    Color hull = PAL_HULL;

    // Floor — dark hull grating with brass drainage strips
    float bw = 8, bd = 6;
    add_wall(s, 0, -0.1f, 0, bw, 0.2f, bd, hull);
    set_last_material(s, MAT_TILE);
    // Drainage strips
    for (int i = 0; i < 5; i++) {
        float dz = -bd/2 + 0.5f + i * (bd/5.0f);
        add_wall(s, 0, 0.01f, dz, bw-0.4f, 0.02f, 0.06f, gold);
        set_last_material(s, MAT_BRASS);
        set_last_decal(s);
    }
    // Perimeter floor lighting (blue-white, recessed)
    add_wall(s, 0, 0.01f, -bd/2+0.2f, bw-0.4f, 0.01f, 0.08f, (Color){120,160,220,100});
    set_last_decal(s);
    add_wall(s, -bw/2+0.2f, 0.01f, 0, 0.08f, 0.01f, bd-0.4f, (Color){120,160,220,80});
    set_last_decal(s);
    add_wall(s, bw/2-0.2f, 0.01f, 0, 0.08f, 0.01f, bd-0.4f, (Color){120,160,220,80});
    set_last_decal(s);

    // Back wall — suite exterior hull with door
    add_wall(s, 0, 2.5f, bd/2, bw, 5, 0.3f, hull);
    set_last_material(s, MAT_CONCRETE);
    // Hull panel seams
    add_wall(s, -2, 2.5f, bd/2-0.14f, 0.04f, 4.5f, 0.02f, (Color){85,90,100,255});
    add_wall(s, 2, 2.5f, bd/2-0.14f, 0.04f, 4.5f, 0.02f, (Color){85,90,100,255});
    // Door back to suite — warm light spilling through
    add_door_frame(s, 0, 0, bd/2-0.15f, 1.2f, 2.6f, 0.1f, gold);
    add_light_panel(s, 0, 1.3f, bd/2-0.2f, 1.0f, 2.4f, 0.02f, (Color){240,220,175,60});
    // Emergency airlock panel
    add_wall(s, 3.0f, 1.2f, bd/2-0.14f, 0.3f, 0.4f, 0.04f, (Color){180,40,40,200});
    add_wall(s, 3.0f, 1.2f, bd/2-0.12f, 0.26f, 0.36f, 0.02f, hull);

    // Side hull walls with external pipes
    add_wall(s, -bw/2, 2.5f, 0, 0.3f, 5, bd+0.6f, hull);
    set_last_material(s, MAT_CONCRETE);
    add_wall(s, bw/2, 2.5f, 0, 0.3f, 5, bd+0.6f, hull);
    set_last_material(s, MAT_CONCRETE);
    // Conduit pipes on hull walls
    add_cylinder(s, -bw/2-0.1f, 3.5f, 0, 0.04f, bd, (Color){90,85,80,255});
    add_cylinder(s, bw/2+0.1f, 3.8f, 0, 0.03f, bd, (Color){90,85,80,255});
    add_cylinder(s, -bw/2-0.15f, 1.5f, 0, 0.05f, bd, (Color){100,95,90,255});
    // Navigation lights
    add_sphere(s, -bw/2-0.2f, 4.5f, -bd/2, 0.12f, (Color){255,40,40,200}); // port red
    add_sphere(s, bw/2+0.2f, 4.5f, -bd/2, 0.12f, (Color){40,255,40,200});  // starboard green

    // Railing — detailed: posts, top rail, mid rail, glass infill
    // Front railing
    add_wall(s, 0, 1.0f, -bd/2, bw, 0.06f, 0.06f, gold);
    set_last_material(s, MAT_BRASS);
    add_wall(s, 0, 0.5f, -bd/2, bw, 0.04f, 0.04f, gold);
    set_last_material(s, MAT_BRASS);
    for (int i = 0; i <= 8; i++) {
        float px = -bw/2 + i * (bw/8.0f);
        add_cylinder(s, px, 0.5f, -bd/2, 0.03f, 0.56f, gold);
    }
    // Glass infill panels
    for (int i = 0; i < 8; i++) {
        float px = -bw/2 + 0.5f + i * (bw/8.0f);
        add_wall(s, px, 0.75f, -bd/2+0.01f, bw/8.0f-0.1f, 0.44f, 0.02f, (Color){140,180,220,20});
        set_last_material(s, MAT_GLASS);
    }
    // Side railings
    add_wall(s, -bw/2, 1.0f, -bd/4, 0.06f, 0.06f, bd/2, gold);
    set_last_material(s, MAT_BRASS);
    add_wall(s, bw/2, 1.0f, -bd/4, 0.06f, 0.06f, bd/2, gold);
    set_last_material(s, MAT_BRASS);

    // TWO CHAIRS — side by side, facing the void. The emotional gut-punch.
    // Chair 1 (left)
    add_wall(s, -1.0f, 0.42f, -1.0f, 0.5f, 0.06f, 0.5f, PAL_WOOD);   // seat
    set_last_material(s, MAT_LEATHER);
    add_wall(s, -1.0f, 0.72f, -0.73f, 0.5f, 0.55f, 0.06f, PAL_WOOD);  // back
    set_last_material(s, MAT_LEATHER);
    add_cylinder(s, -1.22f, 0.21f, -1.22f, 0.03f, 0.42f, PAL_WOOD_DARK);
    add_cylinder(s, -0.78f, 0.21f, -1.22f, 0.03f, 0.42f, PAL_WOOD_DARK);
    add_cylinder(s, -1.22f, 0.21f, -0.78f, 0.03f, 0.42f, PAL_WOOD_DARK);
    add_cylinder(s, -0.78f, 0.21f, -0.78f, 0.03f, 0.42f, PAL_WOOD_DARK);
    // Chair 2 (right) — identical, empty
    add_wall(s, 1.0f, 0.42f, -1.0f, 0.5f, 0.06f, 0.5f, PAL_WOOD);
    set_last_material(s, MAT_LEATHER);
    add_wall(s, 1.0f, 0.72f, -0.73f, 0.5f, 0.55f, 0.06f, PAL_WOOD);
    set_last_material(s, MAT_LEATHER);
    add_cylinder(s, 0.78f, 0.21f, -1.22f, 0.03f, 0.42f, PAL_WOOD_DARK);
    add_cylinder(s, 1.22f, 0.21f, -1.22f, 0.03f, 0.42f, PAL_WOOD_DARK);
    add_cylinder(s, 0.78f, 0.21f, -0.78f, 0.03f, 0.42f, PAL_WOOD_DARK);
    add_cylinder(s, 1.22f, 0.21f, -0.78f, 0.03f, 0.42f, PAL_WOOD_DARK);

    // Side table between chairs — ashtray, two wine glasses
    add_wall(s, 0, 0.45f, -0.8f, 0.4f, 0.03f, 0.4f, gold);
    set_last_material(s, MAT_BRASS);
    add_cylinder(s, 0, 0.22f, -0.8f, 0.04f, 0.45f, gold);
    // Wine glass 1
    add_cylinder(s, -0.1f, 0.49f, -0.7f, 0.02f, 0.1f, (Color){210,210,215,160});
    add_wall(s, -0.1f, 0.55f, -0.7f, 0.04f, 0.05f, 0.04f, (Color){210,210,215,160});
    // Wine glass 2
    add_cylinder(s, 0.1f, 0.49f, -0.9f, 0.02f, 0.1f, (Color){210,210,215,160});
    add_wall(s, 0.1f, 0.55f, -0.9f, 0.04f, 0.05f, 0.04f, (Color){210,210,215,160});
    // Ashtray
    add_cylinder(s, 0, 0.49f, -0.8f, 0.1f, 0.03f, (Color){140,135,130,255});
    add_object(s, 0, 0.55f, -0.8f, "cigarette", (Color){200,195,185,255}, 1);

    // Distant station modules (tiny lit rectangles)
    add_wall(s, -30, 5, -50, 1.5f, 0.8f, 0.3f, (Color){140,150,165,80});
    add_wall(s, -30, 5.2f, -50, 0.8f, 0.3f, 0.1f, (Color){240,220,150,60});
    add_wall(s, 25, 8, -60, 1.2f, 0.6f, 0.3f, (Color){130,140,155,60});
    add_wall(s, 25, 8.1f, -60, 0.5f, 0.2f, 0.1f, (Color){200,210,230,40});
    // Connecting truss
    add_wall(s, -2.5f, 6.5f, -55, 55, 0.04f, 0.04f, (Color){100,105,115,30});

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
    add_wall(s, 15, 8, -30, 20, 6, 0.08f, (Color){80, 50, 100, 18});
    // Cool counterpoint
    add_wall(s, -20, 6, -35, 15, 4, 0.08f, (Color){40, 70, 110, 12});

    // Milky Way band — very faint diagonal streak across the sky
    for (int i = 0; i < 8; i++) {
        float mx = -20.0f + i * 5.5f;
        float my = 4.0f + i * 1.2f;
        float mz = -28.0f - i * 2.0f;
        add_wall(s, mx, my, mz, 4.0f, 1.5f, 0.06f,
                 (Color){180, 175, 165, (unsigned char)(8 + (i*3)%8)});
    }

    tag_materials_by_color(s);

    s->spawn = (Vector3){0, 1.6f, 0.5f};
    s->has_exit = false;
}

void build_bathroom(Scene *s) {
    memset(s, 0, sizeof(Scene));
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

    // ── BATHTUB — freestanding, the Chevalier moment ──
    {
        int tub_mdl = find_model_asset("bathtub");
        if (tub_mdl >= 0) {
            add_model(s, -1.2f, 0, -0.8f, 1,1,1, 0, tub_mdl, MAT_MARBLE, (Color){255,255,255,255});
        } else {
            // Fallback: simple box tub
            add_wall(s, -1.2f, 0.3f, -0.8f, 1.4f, 0.5f, 0.6f, porcelain);
            set_last_material(s, MAT_TILE);
            // Rim
            add_wall(s, -1.2f, 0.56f, -0.8f, 1.5f, 0.04f, 0.65f, porcelain);
        }
    }

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
    memset(s, 0, sizeof(Scene));
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
    memset(s, 0, sizeof(Scene));
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
    memset(s, 0, sizeof(Scene));
    // BOUNDS: 3m x 3m x 3.5m — intimate, compressed. Hotel Chevalier energy.
    // Two people in a box with unspoken knowledge. Gibbons adjusts his tie.
    s->surface = SURFACE_MARBLE;

    Color brass = PAL_BRASS;
    Color brass_dark = {155, 135, 92, 255};
    Color floor_dark = PAL_DARK;
    Color mirror = {210, 215, 220, 180};
    Color warm_panel = PAL_LIGHT_WARM;
    Color btn_dark = {140, 125, 90, 255};
    Color btn_lit = {240, 220, 140, 255};
    Color wood_dark = PAL_WOOD_DARK;
    Color wood_mid = PAL_WOOD;
    Color wallpaper = {195, 188, 175, 255};
    Color burgundy = {120, 35, 30, 255};
    Color navy_carpet = {35, 30, 55, 255};
    Color cream = PAL_CREAM;

    float ew = 3.0f, ed = 3.0f, eh = 3.5f;
    float wainscot_h = 1.2f;  // lower half: wood

    s->fog_color = PAL_FOG_BRASS;
    s->fog_density = 0.002f;

    // ============================================================
    // FLOOR — dark carpet with brass threshold
    // ============================================================
    add_wall(s, 0, -0.05f, 0, ew, 0.1f, ed, floor_dark);
    set_last_material(s, MAT_CARPET);

    // Carpet runner — burgundy/navy strip down center
    add_wall(s, 0, 0.01f, 0, 1.0f, 0.02f, ed - 0.4f, burgundy);
    set_last_material(s, MAT_CARPET);
    // Navy border on runner
    add_wall(s, -0.55f, 0.015f, 0, 0.08f, 0.02f, ed - 0.5f, navy_carpet);
    set_last_material(s, MAT_CARPET);
    add_wall(s, 0.55f, 0.015f, 0, 0.08f, 0.02f, ed - 0.5f, navy_carpet);
    set_last_material(s, MAT_CARPET);

    // Brass threshold strip at door
    add_wall(s, 0, 0.02f, ed/2 - 0.08f, ew - 0.3f, 0.04f, 0.08f, brass);
    set_last_material(s, MAT_BRASS);

    // ============================================================
    // CEILING — recessed panel with warm downlight
    // ============================================================
    add_wall(s, 0, eh, 0, ew, 0.1f, ed, brass_dark);
    set_last_material(s, MAT_BRASS);
    // Recessed ceiling panel (slightly lower, darker)
    add_wall(s, 0, eh - 0.08f, 0, ew - 0.5f, 0.04f, ed - 0.5f, (Color){60, 55, 48, 255});
    set_last_material(s, MAT_WOOD);
    // Warm ceiling rim around recess
    add_wall(s, 0, eh - 0.06f, -ed/2 + 0.15f, ew - 0.4f, 0.06f, 0.08f, brass);
    set_last_material(s, MAT_BRASS);
    add_wall(s, 0, eh - 0.06f, ed/2 - 0.15f, ew - 0.4f, 0.06f, 0.08f, brass);
    set_last_material(s, MAT_BRASS);
    add_wall(s, -ew/2 + 0.15f, eh - 0.06f, 0, 0.08f, 0.06f, ed - 0.4f, brass);
    set_last_material(s, MAT_BRASS);
    add_wall(s, ew/2 - 0.15f, eh - 0.06f, 0, 0.08f, 0.06f, ed - 0.4f, brass);
    set_last_material(s, MAT_BRASS);

    // Central downlight — warm globe pendant
    add_light_panel(s, 0, eh - 0.12f, 0, 0.8f, 0.04f, 0.8f, warm_panel);
    // Pendant rod
    add_cylinder(s, 0, eh - 0.2f, 0, 0.03f, 0.3f, brass);
    set_last_material(s, MAT_BRASS);
    // Brass globe housing
    add_sphere(s, 0, eh - 0.4f, 0, 0.22f, (Color){220, 195, 130, 200});
    set_last_material(s, MAT_BRASS);
    // Warm glow sphere
    add_sphere(s, 0, eh - 0.4f, 0, 0.18f, (Color){250, 230, 170, 140});

    // ============================================================
    // WALLS — wood wainscoting (lower), brass panel (upper), wallpaper above
    // ============================================================

    // BACK WALL — wood wainscoting + brass + wallpaper
    add_wall(s, 0, eh/2, -ed/2, ew, eh, 0.08f, brass_dark);
    set_last_material(s, MAT_BRASS);
    // Wainscoting panels — dark wood lower half
    add_wall(s, 0, wainscot_h/2, -ed/2 + 0.06f, ew - 0.2f, wainscot_h, 0.03f, wood_dark);
    set_last_material(s, MAT_WOOD);
    // Wainscoting panel divisions (3 panels)
    for (int i = 0; i < 4; i++) {
        float px = -ew/2 + 0.2f + i * (ew - 0.4f) / 3.0f;
        add_wall(s, px, wainscot_h/2, -ed/2 + 0.075f, 0.04f, wainscot_h - 0.1f, 0.02f, wood_mid);
        set_last_material(s, MAT_WOOD);
    }
    // Wainscoting cap rail
    add_wall(s, 0, wainscot_h, -ed/2 + 0.07f, ew - 0.15f, 0.06f, 0.04f, wood_mid);
    set_last_material(s, MAT_WOOD);
    // Wallpaper above wainscoting
    add_wall(s, 0, wainscot_h + (eh - wainscot_h - 0.5f)/2 + 0.1f, -ed/2 + 0.06f,
             ew - 0.2f, eh - wainscot_h - 0.6f, 0.02f, wallpaper);
    set_last_material(s, MAT_WALLPAPER);

    // FRONT WALL — doors (closed)
    add_wall(s, 0, eh/2, ed/2, ew, eh, 0.08f, brass_dark);
    set_last_material(s, MAT_BRASS);
    // Door panels — two halves with seam
    add_wall(s, -0.4f, eh/2 - 0.2f, ed/2 - 0.02f, 0.76f, eh - 0.6f, 0.02f, brass);
    set_last_material(s, MAT_BRASS);
    add_wall(s, 0.4f, eh/2 - 0.2f, ed/2 - 0.02f, 0.76f, eh - 0.6f, 0.02f, brass);
    set_last_material(s, MAT_BRASS);
    // Door seam — dark line
    add_wall(s, 0, eh/2, ed/2 - 0.015f, 0.03f, eh - 0.4f, 0.02f, (Color){25, 23, 20, 255});
    // Door frame — brass surround
    add_wall(s, 0, eh - 0.15f, ed/2 - 0.03f, 1.7f, 0.1f, 0.04f, brass);
    set_last_material(s, MAT_BRASS);
    add_wall(s, -0.85f, eh/2 - 0.2f, ed/2 - 0.03f, 0.06f, eh - 0.3f, 0.04f, brass);
    set_last_material(s, MAT_BRASS);
    add_wall(s, 0.85f, eh/2 - 0.2f, ed/2 - 0.03f, 0.06f, eh - 0.3f, 0.04f, brass);
    set_last_material(s, MAT_BRASS);

    // RIGHT WALL — wainscoting + wallpaper + control panel
    add_wall(s, ew/2, eh/2, 0, 0.08f, eh, ed, brass_dark);
    set_last_material(s, MAT_BRASS);
    // Wainscoting
    add_wall(s, ew/2 - 0.06f, wainscot_h/2, 0, 0.03f, wainscot_h, ed - 0.2f, wood_dark);
    set_last_material(s, MAT_WOOD);
    // Panel divisions (3 panels)
    for (int i = 0; i < 4; i++) {
        float pz = -ed/2 + 0.2f + i * (ed - 0.4f) / 3.0f;
        add_wall(s, ew/2 - 0.075f, wainscot_h/2, pz, 0.02f, wainscot_h - 0.1f, 0.04f, wood_mid);
        set_last_material(s, MAT_WOOD);
    }
    // Cap rail
    add_wall(s, ew/2 - 0.07f, wainscot_h, 0, 0.04f, 0.06f, ed - 0.15f, wood_mid);
    set_last_material(s, MAT_WOOD);
    // Wallpaper above
    add_wall(s, ew/2 - 0.06f, wainscot_h + (eh - wainscot_h - 0.5f)/2 + 0.1f, 0,
             0.02f, eh - wainscot_h - 0.6f, ed - 0.2f, wallpaper);
    set_last_material(s, MAT_WALLPAPER);

    // LEFT WALL — glass viewport. The player SEES Auckland drop away.
    add_wall(s, -ew/2, eh/2, 0, 0.02f, eh, ed, (Color){100, 130, 160, 20});
    set_last_material(s, MAT_GLASS);
    // Brass frame around glass
    add_wall(s, -ew/2, eh - 0.03f, 0, 0.04f, 0.08f, ed, brass);
    set_last_material(s, MAT_BRASS);
    add_wall(s, -ew/2, 0.03f, 0, 0.04f, 0.08f, ed, brass);
    set_last_material(s, MAT_BRASS);
    add_wall(s, -ew/2, eh/2, -ed/2, 0.04f, eh, 0.06f, brass);
    set_last_material(s, MAT_BRASS);
    add_wall(s, -ew/2, eh/2, ed/2, 0.04f, eh, 0.06f, brass);
    set_last_material(s, MAT_BRASS);
    // Horizontal mullion
    add_wall(s, -ew/2, 1.7f, 0, 0.03f, 0.05f, ed - 0.1f, brass);
    set_last_material(s, MAT_BRASS);

    // ============================================================
    // BRASS HANDRAIL — running around 3 walls at waist height
    // ============================================================
    // Back wall handrail
    add_wall(s, 0, 0.95f, -ed/2 + 0.09f, ew - 0.3f, 0.04f, 0.04f, brass);
    set_last_material(s, MAT_BRASS);
    // Leather pad on back handrail
    add_wall(s, 0, 0.97f, -ed/2 + 0.1f, ew - 0.4f, 0.05f, 0.05f, PAL_LEATHER);
    set_last_material(s, MAT_LEATHER);
    // Right wall handrail
    add_wall(s, ew/2 - 0.09f, 0.95f, 0, 0.04f, 0.04f, ed - 0.3f, brass);
    set_last_material(s, MAT_BRASS);
    // Leather pad on right handrail
    add_wall(s, ew/2 - 0.1f, 0.97f, 0, 0.05f, 0.05f, ed - 0.4f, PAL_LEATHER);
    set_last_material(s, MAT_LEATHER);
    // Front wall handrail (flanking door)
    add_wall(s, -1.1f, 0.95f, ed/2 - 0.09f, 0.3f, 0.04f, 0.04f, brass);
    set_last_material(s, MAT_BRASS);
    add_wall(s, 1.1f, 0.95f, ed/2 - 0.09f, 0.3f, 0.04f, 0.04f, brass);
    set_last_material(s, MAT_BRASS);
    // Handrail brackets (4 on back wall, 2 on right)
    for (int i = 0; i < 4; i++) {
        float bx = -ew/2 + 0.3f + i * (ew - 0.6f) / 3.0f;
        add_wall(s, bx, 0.92f, -ed/2 + 0.07f, 0.06f, 0.08f, 0.03f, brass_dark);
        set_last_material(s, MAT_BRASS);
    }
    for (int i = 0; i < 3; i++) {
        float bz = -ed/2 + 0.4f + i * (ed - 0.8f) / 2.0f;
        add_wall(s, ew/2 - 0.07f, 0.92f, bz, 0.03f, 0.08f, 0.06f, brass_dark);
        set_last_material(s, MAT_BRASS);
    }

    // ============================================================
    // CORNER TRIM — brass strips at all vertical edges
    // ============================================================
    // Back-left corner
    add_wall(s, -ew/2 + 0.02f, eh/2, -ed/2 + 0.02f, 0.04f, eh, 0.04f, brass);
    set_last_material(s, MAT_BRASS);
    // Back-right corner
    add_wall(s, ew/2 - 0.02f, eh/2, -ed/2 + 0.02f, 0.04f, eh, 0.04f, brass);
    set_last_material(s, MAT_BRASS);
    // Front-left corner
    add_wall(s, -ew/2 + 0.02f, eh/2, ed/2 - 0.02f, 0.04f, eh, 0.04f, brass);
    set_last_material(s, MAT_BRASS);
    // Front-right corner
    add_wall(s, ew/2 - 0.02f, eh/2, ed/2 - 0.02f, 0.04f, eh, 0.04f, brass);
    set_last_material(s, MAT_BRASS);

    // ============================================================
    // MIRROR — back wall, brass frame, dark glass
    // ============================================================
    add_wall(s, 0, 1.8f, -ed/2 + 0.065f, 1.6f, 1.2f, 0.03f, mirror);
    set_last_material(s, MAT_GLASS);
    // Mirror frame — brass
    add_wall(s, 0, 2.45f, -ed/2 + 0.07f, 1.7f, 0.06f, 0.04f, brass);
    set_last_material(s, MAT_BRASS);
    add_wall(s, 0, 1.15f, -ed/2 + 0.07f, 1.7f, 0.06f, 0.04f, brass);
    set_last_material(s, MAT_BRASS);
    add_wall(s, -0.83f, 1.8f, -ed/2 + 0.07f, 0.06f, 1.3f, 0.04f, brass);
    set_last_material(s, MAT_BRASS);
    add_wall(s, 0.83f, 1.8f, -ed/2 + 0.07f, 0.06f, 1.3f, 0.04f, brass);
    set_last_material(s, MAT_BRASS);

    // ============================================================
    // CONTROL PANEL — column of brass buttons, plate, indicator
    // ============================================================
    float panel_x = ew/2 - 0.065f;
    float panel_z = 0.4f;
    // Main panel plate
    add_wall(s, panel_x, 1.3f, panel_z, 0.04f, 0.9f, 0.3f, (Color){150, 135, 100, 255});
    set_last_material(s, MAT_BRASS);
    // Panel frame
    add_wall(s, panel_x + 0.005f, 1.3f, panel_z - 0.16f, 0.03f, 0.92f, 0.02f, brass_dark);
    set_last_material(s, MAT_BRASS);
    add_wall(s, panel_x + 0.005f, 1.3f, panel_z + 0.16f, 0.03f, 0.92f, 0.02f, brass_dark);
    set_last_material(s, MAT_BRASS);
    // Floor number buttons — 8 buttons in a column
    for (int i = 0; i < 8; i++) {
        float by = 0.95f + i * 0.1f;
        Color bc = (i == 2) ? btn_lit : btn_dark;  // floor 3 lit
        add_wall(s, panel_x - 0.01f, by, panel_z, 0.03f, 0.06f, 0.06f, bc);
    }
    // Emergency stop button (red, at bottom)
    add_wall(s, panel_x - 0.01f, 0.88f, panel_z, 0.03f, 0.06f, 0.06f, PAL_RED);
    // Call indicator — small brass circle
    add_wall(s, panel_x - 0.01f, 1.82f, panel_z, 0.03f, 0.08f, 0.08f, btn_lit);

    // ============================================================
    // FLOOR INDICATOR — above door, lit brass numbers
    // ============================================================
    add_wall(s, 0, eh - 0.25f, ed/2 - 0.06f, 0.6f, 0.3f, 0.02f, (Color){55, 50, 42, 255});
    set_last_material(s, MAT_BRASS);
    // Brass frame around indicator
    add_wall(s, 0, eh - 0.25f, ed/2 - 0.055f, 0.66f, 0.06f, 0.03f, brass);
    set_last_material(s, MAT_BRASS);
    add_wall(s, 0, eh - 0.25f, ed/2 - 0.055f, 0.06f, 0.36f, 0.03f, brass);
    set_last_material(s, MAT_BRASS);
    // Glow — warm amber (current floor lit)
    add_light_panel(s, 0, eh - 0.25f, ed/2 - 0.04f, 0.4f, 0.18f, 0.01f, (Color){240, 200, 120, 180});
    // Arrow indicator (up direction)
    add_wall(s, 0.22f, eh - 0.18f, ed/2 - 0.045f, 0.06f, 0.1f, 0.01f, btn_lit);

    // ============================================================
    // BRASS AIR VENT — near ceiling, right wall
    // ============================================================
    add_wall(s, ew/2 - 0.06f, eh - 0.35f, -0.5f, 0.02f, 0.2f, 0.4f, brass_dark);
    set_last_material(s, MAT_BRASS);
    // Vent slats (horizontal lines)
    for (int i = 0; i < 4; i++) {
        float vy = eh - 0.42f + i * 0.06f;
        add_wall(s, ew/2 - 0.055f, vy, -0.5f, 0.015f, 0.02f, 0.35f, brass);
        set_last_material(s, MAT_BRASS);
    }

    // ============================================================
    // EMERGENCY PHONE PANEL — small brass plate, left of door
    // ============================================================
    add_wall(s, -0.95f, 1.4f, ed/2 - 0.06f, 0.2f, 0.25f, 0.02f, brass_dark);
    set_last_material(s, MAT_BRASS);
    // Phone icon (small dark square)
    add_wall(s, -0.95f, 1.4f, ed/2 - 0.05f, 0.1f, 0.12f, 0.01f, (Color){40, 35, 28, 255});

    // ============================================================
    // CERTIFICATE / INSPECTION PLATE — right wall
    // ============================================================
    add_wall(s, ew/2 - 0.06f, 2.0f, -0.5f, 0.02f, 0.35f, 0.25f, cream);
    set_last_material(s, MAT_WALLPAPER);
    // Thin brass frame around certificate
    add_wall(s, ew/2 - 0.055f, 2.0f, -0.5f - 0.135f, 0.015f, 0.37f, 0.02f, brass);
    set_last_material(s, MAT_BRASS);
    add_wall(s, ew/2 - 0.055f, 2.0f, -0.5f + 0.135f, 0.015f, 0.37f, 0.02f, brass);
    set_last_material(s, MAT_BRASS);
    add_wall(s, ew/2 - 0.055f, 2.19f, -0.5f, 0.015f, 0.02f, 0.25f, brass);
    set_last_material(s, MAT_BRASS);
    add_wall(s, ew/2 - 0.055f, 1.81f, -0.5f, 0.015f, 0.02f, 0.25f, brass);
    set_last_material(s, MAT_BRASS);

    // ============================================================
    // BASEBOARD — dark wood strip at floor level, 3 walls
    // ============================================================
    add_wall(s, 0, 0.06f, -ed/2 + 0.05f, ew - 0.1f, 0.12f, 0.03f, wood_dark);
    set_last_material(s, MAT_WOOD);
    add_wall(s, ew/2 - 0.05f, 0.06f, 0, 0.03f, 0.12f, ed - 0.1f, wood_dark);
    set_last_material(s, MAT_WOOD);
    add_wall(s, 0, 0.06f, ed/2 - 0.05f, ew - 0.1f, 0.12f, 0.03f, wood_dark);
    set_last_material(s, MAT_WOOD);

    // ============================================================
    // EXTERIOR — Auckland visible through the glass, dropping away
    // These walls are STATIC; the camera rises, they stay put.
    // The player watches the city shrink below them.
    // ============================================================

    // Sky Tower shaft — you're INSIDE it, rising past structural rings
    for (int i = 0; i < 20; i++) {
        float ry = -5.0f + i * 8.0f;
        add_wall(s, -3, ry, 0, 0.15f, 0.4f, 3, (Color){80, 85, 95, 200});
    }

    // Auckland city lights — far below, visible at start, shrinking
    Color city = {240, 200, 110, 100};
    Color bldg = {20, 24, 35, 255};
    add_wall(s, -8, -2, -5, 3, 6, 2, bldg);
    add_wall(s, -12, -1, -8, 4, 8, 2, (Color){18, 22, 32, 255});
    add_wall(s, -6, -3, 3, 5, 4, 2, (Color){22, 26, 38, 255});
    add_wall(s, -15, 0, 2, 3, 10, 2, bldg);
    add_wall(s, -10, -1, 6, 2, 5, 2, (Color){24, 28, 38, 255});
    add_wall(s, -18, 1, -3, 3, 12, 2, (Color){16, 20, 30, 255});
    // Windows on buildings — amber glow
    for (int b = 0; b < 5; b++) {
        float bx = -8.0f - b * 3.0f;
        for (int f = 0; f < 4; f++) {
            add_wall(s, bx, -3.0f + f * 1.8f, -5.5f, 0.7f, 0.5f, 0.04f, city);
        }
    }
    // Ground — road and sidewalk far below
    add_wall(s, -8, -6, 0, 30, 0.1f, 30, (Color){25, 28, 32, 255});
    // Streetlights
    for (int i = 0; i < 6; i++) {
        float lz = -12.0f + i * 5.0f;
        add_wall(s, -5, -4, lz, 0.3f, 0.3f, 0.3f, (Color){240, 210, 120, 140});
    }

    // Stars — visible once above the city (high Y)
    for (int i = 0; i < 18; i++) {
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
    add_cylinder(s, 0.48f, 1.0f, -0.6f, 0.008f, 0.03f, (Color){230,225,215,180});

}


void build_elevator_space(Scene *s) {
    memset(s, 0, sizeof(Scene));
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
    add_wall(s, 0, -0.01f, 0, ew-0.2f, 0.02f, 0.03f, brass);

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
    add_cylinder(s, 0.48f, 1.0f, -0.45f, 0.1f, 0.45f, (Color){230,225,215,180});
    set_last_decal(s);
}

void build_taxi_ride(Scene *s) {
    memset(s, 0, sizeof(Scene));
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
    Color driver_collar = {225, 220, 210, 255};
    Color driver_cap = {42, 40, 36, 255};
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
    add_wall(s, dx - 0.19f, 0.74f, dz - 0.34f, 0.04f, 0.03f, 0.06f, (Color){210,180,100,255});
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
    add_cylinder(s, 0.35f, 0.62f, -0.2f, 0.1f, 0.45f, (Color){230,225,215,255});
    set_last_decal(s);
    // Ash tip — still warm
    add_wall(s, 0.35f, 0.64f, -0.2f, 0.1f, 0.06f, 0.1f, (Color){60,55,50,255});
    set_last_decal(s);

    // ── SECOND TICKET — Commandment 9. The trip was for two. ──
    // Two paper receipts on the backseat, right side. Not interactive.
    // Ticket 1 — yours, slightly crumpled
    add_wall(s, 0.55f, 0.42f, 0.2f, 0.22f, 0.005f, 0.1f, (Color){245,242,235,255});
    set_last_decal(s);
    // Blue print line
    add_wall(s, 0.55f, 0.425f, 0.18f, 0.18f, 0.002f, 0.015f, (Color){50,80,180,200});
    set_last_decal(s);
    // Ticket 2 — hers, untouched, slightly overlapping
    add_wall(s, 0.62f, 0.42f, 0.25f, 0.22f, 0.005f, 0.1f, (Color){245,242,235,255});
    set_last_rotation(s, 8.0f);
    set_last_decal(s);
    add_wall(s, 0.62f, 0.425f, 0.23f, 0.18f, 0.002f, 0.015f, (Color){50,80,180,200});
    set_last_rotation(s, 8.0f);
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
    add_cylinder(s, 0.3f, 0.62f, -0.15f, 0.1f, 0.45f, (Color){230,225,215,255});
    set_last_decal(s);

}

void build_hyperspace(Scene *s) {
    memset(s, 0, sizeof(Scene));
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
    memset(s, 0, sizeof(Scene));
    // BOUNDS: 40m x 30m, 12m ceiling — the Gehry moment, Bioshock Infinite energy
    // You arrive through hyperspace into something impossible.
    s->surface = SURFACE_MARBLE;

    Color hull = PAL_HULL;
    Color hull_lt = PAL_HULL_LIGHT;
    Color brass = PAL_BRASS;
    Color cream = PAL_CREAM;
    Color white = PAL_WHITE;
    Color void_black = PAL_PORTHOLE;
    Color gold = PAL_GOLD;
    Color godard_red = PAL_RED;
    Color godard_blue = PAL_BLUE;
    Color navy = PAL_NAVY;
    Color marble_a = PAL_MARBLE_A;
    Color marble_b = PAL_MARBLE_B;
    Color warm_light = PAL_LIGHT_WARM;
    s->fog_color = PAL_FOG_STATION;
    s->fog_density = 0.0006f;

    float lw = 40, ld = 30, lh = 12;

    // ================================================================
    // FLOOR — checkerboard marble with brass inlay strips
    // ================================================================
    add_wall(s, 0, -0.05f, 0, lw, 0.1f, ld, marble_a);
    set_last_material(s, MAT_CHECKERBOARD);

    // Brass inlay strips — pathways defining circulation
    // Central axis toward observation window
    add_wall(s, 0, 0.01f, 0, 0.08f, 0.02f, ld*0.8f, brass);
    set_last_material(s, MAT_BRASS); set_last_decal(s);
    // Cross axis
    add_wall(s, 0, 0.01f, 0, lw*0.6f, 0.02f, 0.08f, brass);
    set_last_material(s, MAT_BRASS); set_last_decal(s);
    // Diagonal convergence lines toward window
    add_wall(s, -4, 0.01f, -3, 0.06f, 0.02f, 12, brass);
    set_last_material(s, MAT_BRASS); set_last_decal(s);
    set_last_rotation(s, 12.0f);
    add_wall(s, 4, 0.01f, -3, 0.06f, 0.02f, 12, brass);
    set_last_material(s, MAT_BRASS); set_last_decal(s);
    set_last_rotation(s, -12.0f);
    // Perimeter inlay — rectangle around elevator area
    add_wall(s, 0, 0.01f, 0, 8, 0.02f, 0.06f, brass);
    set_last_material(s, MAT_BRASS); set_last_decal(s);
    add_wall(s, 0, 0.01f, 4, 8, 0.02f, 0.06f, brass);
    set_last_material(s, MAT_BRASS); set_last_decal(s);
    add_wall(s, -4, 0.01f, 2, 0.06f, 0.02f, 4, brass);
    set_last_material(s, MAT_BRASS); set_last_decal(s);
    add_wall(s, 4, 0.01f, 2, 0.06f, 0.02f, 4, brass);
    set_last_material(s, MAT_BRASS); set_last_decal(s);

    // Floor-embedded directional lighting strips — soft blue glow
    for (int i = 0; i < 5; i++) {
        float sz = -10 + i * 5;
        add_wall(s, -0.5f, 0.02f, sz, 0.3f, 0.02f, 0.8f, (Color){60, 130, 200, 60});
        set_last_decal(s);
        add_wall(s, 0.5f, 0.02f, sz, 0.3f, 0.02f, 0.8f, (Color){60, 130, 200, 60});
        set_last_decal(s);
    }

    // ================================================================
    // CEILING — hull with structural ribs
    // ================================================================
    add_wall(s, 0, lh, 0, lw, 0.25f, ld, hull);
    set_last_material(s, MAT_CONCRETE);
    // Structural ribs — 5 brass lines across ceiling
    for (int i = 0; i < 5; i++) {
        float rz = -ld/2 + 3 + i * (ld/4.0f);
        add_wall(s, 0, lh - 0.1f, rz, lw, 0.15f, 0.08f, brass);
        set_last_material(s, MAT_BRASS);
    }
    // Cross ribs
    for (int i = 0; i < 3; i++) {
        float rx = -lw/3 + i * (lw/3.0f);
        add_wall(s, rx, lh - 0.1f, 0, 0.08f, 0.15f, ld, brass);
        set_last_material(s, MAT_BRASS);
    }

    // ================================================================
    // WALLS — hull exterior, observation window wall is the back (negative Z)
    // ================================================================

    // BACK WALL (observation side) — hull with massive window
    add_wall(s, 0, lh/2, -ld/2, lw, lh, 0.5f, hull);
    set_last_material(s, MAT_CONCRETE);

    // OBSERVATION WINDOW — 28m wide x 10m tall. The money shot.
    // Curved glass wall — Earth below, the emotional anchor
    add_wall(s, 0, lh/2, -ld/2+0.12f, 28, 10.0f, 0.1f, void_black);
    set_last_material(s, MAT_GLASS);
    // Brass frame — top, bottom, left, right
    add_wall(s, 0, lh/2+5.1f, -ld/2+0.14f, 28.4f, 0.2f, 0.12f, brass);
    set_last_material(s, MAT_BRASS);
    add_wall(s, 0, lh/2-5.1f, -ld/2+0.14f, 28.4f, 0.2f, 0.12f, brass);
    set_last_material(s, MAT_BRASS);
    add_wall(s, -14.1f, lh/2, -ld/2+0.14f, 0.2f, 10.2f, 0.12f, brass);
    set_last_material(s, MAT_BRASS);
    add_wall(s, 14.1f, lh/2, -ld/2+0.14f, 0.2f, 10.2f, 0.12f, brass);
    set_last_material(s, MAT_BRASS);
    // Mullions — 3 vertical dividers
    for (int i = 0; i < 3; i++) {
        float mx = -7 + i * 7;
        add_wall(s, mx, lh/2, -ld/2+0.16f, 0.1f, 10.0f, 0.08f, brass);
        set_last_material(s, MAT_BRASS);
    }
    // Horizontal mullion at midpoint
    add_wall(s, 0, lh/2, -ld/2+0.16f, 28, 0.08f, 0.06f, brass);
    set_last_material(s, MAT_BRASS);

    // Earth — the emotional anchor. Massive blue sphere behind the glass.
    add_sphere(s, 4, lh/2+2, -ld/2-18, 8.0f, (Color){35, 75, 140, 255});
    // Atmosphere rim
    add_sphere(s, 4, lh/2+2, -ld/2-18, 8.6f, (Color){120, 170, 220, 255});

    // Earth glow flooding the floor — wide, bright, unmistakable
    add_wall(s, 0, 0.02f, -ld/2+3, 20, 0.02f, 8, (Color){60, 130, 200, 180});
    set_last_decal(s);
    add_wall(s, 0, 0.02f, -ld/2+8, 26, 0.02f, 6, (Color){45, 100, 180, 80});
    set_last_decal(s);
    add_wall(s, 0, 0.02f, -ld/2+12, 30, 0.02f, 4, (Color){30, 70, 150, 40});
    set_last_decal(s);
    // Earth glow on ceiling — reflected blue
    add_wall(s, 0, lh-0.05f, -ld/2+4, 16, 0.02f, 6, (Color){45, 100, 180, 25});
    set_last_decal(s);

    // LEFT WALL — observation glass wall with tall windows
    add_wall(s, -lw/2, lh/2, 0, 0.5f, lh, ld, hull);
    set_last_material(s, MAT_CONCRETE);
    // Tall side windows — 5 windows along left wall
    for (int i = 0; i < 5; i++) {
        float pz = -10 + i * 5;
        add_wall(s, -lw/2+0.2f, lh*0.45f, pz, 0.08f, 7.0f, 2.0f, void_black);
        set_last_material(s, MAT_GLASS);
        // Brass frame top/bottom
        add_wall(s, -lw/2+0.22f, lh*0.45f+3.6f, pz, 0.06f, 0.14f, 2.1f, brass);
        set_last_material(s, MAT_BRASS);
        add_wall(s, -lw/2+0.22f, lh*0.45f-3.6f, pz, 0.06f, 0.14f, 2.1f, brass);
        set_last_material(s, MAT_BRASS);
        // Earth glow from side windows
        add_wall(s, -lw/2+4, 0.02f, pz, 3.0f, 0.02f, 2.0f, (Color){45, 100, 180, 35});
        set_last_decal(s);
    }

    // RIGHT WALL
    add_wall(s, lw/2, lh/2, 0, 0.5f, lh, ld, hull);
    set_last_material(s, MAT_CONCRETE);
    // Tall side windows — 5 windows along right wall
    for (int i = 0; i < 5; i++) {
        float pz = -10 + i * 5;
        add_wall(s, lw/2-0.2f, lh*0.45f, pz, 0.08f, 7.0f, 2.0f, void_black);
        set_last_material(s, MAT_GLASS);
        add_wall(s, lw/2-0.22f, lh*0.45f+3.6f, pz, 0.06f, 0.14f, 2.1f, brass);
        set_last_material(s, MAT_BRASS);
        add_wall(s, lw/2-0.22f, lh*0.45f-3.6f, pz, 0.06f, 0.14f, 2.1f, brass);
        set_last_material(s, MAT_BRASS);
        add_wall(s, lw/2-4, 0.02f, pz, 3.0f, 0.02f, 2.0f, (Color){45, 100, 180, 35});
        set_last_decal(s);
    }

    // FRONT WALL — entry side, hull with corridor exits
    add_wall(s, 0, lh/2, ld/2, lw, lh, 0.5f, hull);
    set_last_material(s, MAT_CONCRETE);
    // Cream paneling on front wall lower half
    add_wall(s, 0, lh*0.2f, ld/2-0.26f, lw-4, lh*0.4f, 0.06f, cream);
    set_last_material(s, MAT_WALLPAPER);

    // Recessed wall panels — industrial-meets-luxury on all hull walls
    add_recessed_panel(s, -14, lh*0.6f, -ld/2+0.3f, 4, 3, 0.1f, hull);
    add_recessed_panel(s, 14, lh*0.6f, -ld/2+0.3f, 4, 3, 0.1f, hull);
    // Side wall panels
    for (int i = 0; i < 3; i++) {
        float pz = -8 + i * 8;
        add_recessed_panel(s, -lw/2+0.3f, lh*0.7f, pz, 3, 2.5f, 0.1f, hull);
        add_recessed_panel(s, lw/2-0.3f, lh*0.7f, pz, 3, 2.5f, 0.1f, hull);
    }
    // Front wall panels
    add_recessed_panel(s, -10, lh*0.6f, ld/2-0.3f, 4, 3, 0.1f, hull);
    add_recessed_panel(s, 10, lh*0.6f, ld/2-0.3f, 4, 3, 0.1f, hull);

    // Wainscoting on side walls — hull panels below window line
    add_wainscoting(s, -lw/2+0.26f, 0, 0, ld-4, 1.2f, true, hull_lt, brass);
    add_wainscoting(s, lw/2-0.26f, 0, 0, ld-4, 1.2f, true, hull_lt, brass);

    // Baseboard — all walls
    add_baseboard(s, 0, 0, -ld/2+0.26f, lw-4, 0, cream);
    add_baseboard(s, -lw/2+0.26f, 0, 0, ld-4, 1, cream);
    add_baseboard(s, lw/2-0.26f, 0, 0, ld-4, 1, cream);
    add_baseboard(s, 0, 0, ld/2-0.26f, lw-4, 0, cream);

    // Crown molding — all walls
    add_crown_molding(s, 0, lh, -ld/2+0.26f, lw-4, 0, brass);
    add_crown_molding(s, -lw/2+0.26f, lh, 0, ld-4, 1, brass);
    add_crown_molding(s, lw/2-0.26f, lh, 0, ld-4, 1, brass);
    add_crown_molding(s, 0, lh, ld/2-0.26f, lw-4, 0, brass);

    // ================================================================
    // GLASS ELEVATOR SHAFT — center of the lobby, the grand feature
    // Transparent, lit from within, brass frame. Goes through the ceiling.
    // ================================================================
    float ex = 0, ez = 2;  // elevator slightly toward player from center
    float shaft_r = 1.4f;
    // 7 brass rings stacked vertically
    for (int i = 0; i < 7; i++) {
        float ry = 0.2f + i * (lh - 0.4f) / 6.0f;
        add_cylinder(s, ex, ry, ez, shaft_r*2+0.2f, 0.12f, brass);
        set_last_material(s, MAT_BRASS);
    }
    // 8 vertical brass posts — octagonal frame
    for (int i = 0; i < 8; i++) {
        float angle = i * (3.14159f * 2.0f / 8.0f);
        float px = ex + cosf(angle) * shaft_r;
        float pz = ez + sinf(angle) * shaft_r;
        add_wall(s, px, lh/2, pz, 0.1f, lh, 0.1f, brass);
        set_last_material(s, MAT_BRASS);
    }
    // Glass panels between posts — 8 transparent panels
    for (int i = 0; i < 8; i++) {
        float a0 = i * (3.14159f * 2.0f / 8.0f);
        float a1 = (i+1) * (3.14159f * 2.0f / 8.0f);
        float mx = ex + cosf((a0+a1)/2) * (shaft_r - 0.02f);
        float mz = ez + sinf((a0+a1)/2) * (shaft_r - 0.02f);
        add_wall(s, mx, lh/2, mz, 0.9f, lh-1, 0.04f, (Color){180, 210, 230, 40});
        set_last_material(s, MAT_GLASS);
        set_last_rotation(s, i * 45.0f);
    }
    // Light column inside — warm glow pillar
    add_light_panel(s, ex, lh/2, ez, 0.6f, lh-0.5f, 0.6f, warm_light);
    // Elevator platform — brass disc at ground level
    add_cylinder(s, ex, 0.15f, ez, shaft_r*2-0.3f, 0.15f, brass);
    set_last_material(s, MAT_BRASS);
    // Platform detail — brass inlay cross
    add_wall(s, ex, 0.24f, ez, shaft_r*1.5f, 0.02f, 0.06f, gold);
    set_last_material(s, MAT_BRASS); set_last_decal(s);
    add_wall(s, ex, 0.24f, ez, 0.06f, 0.02f, shaft_r*1.5f, gold);
    set_last_material(s, MAT_BRASS); set_last_decal(s);

    // ================================================================
    // MARBLE COLUMNS — 6 total, framing circulation paths
    // ================================================================
    // Inner colonnade — flanking the elevator
    add_column_row(s, -6, -4, 12, 2, 0.35f, lh, marble_a);
    add_column_row(s, -6, 8, 12, 2, 0.35f, lh, marble_a);
    // Outer columns near observation window
    add_column_row(s, -10, -10, 20, 2, 0.3f, lh, marble_b);

    // ================================================================
    // CHANDELIERS — three hanging from ceiling
    // ================================================================
    // Main chandelier above lobby center
    add_chandelier(s, 0, lh-1.5f, -4, 6, 1.2f, brass, warm_light);
    // Flanking chandeliers
    add_chandelier(s, -10, lh-1.0f, 2, 4, 0.8f, brass, warm_light);
    add_chandelier(s, 10, lh-1.0f, 2, 4, 0.8f, brass, warm_light);

    // Dropped ceiling sections with recessed lighting
    add_dropped_ceiling(s, -10, lh, -6, 8, 6, 0.3f, hull, warm_light);
    add_dropped_ceiling(s, 10, lh, -6, 8, 6, 0.3f, hull, warm_light);
    add_dropped_ceiling(s, -10, lh, 6, 8, 6, 0.3f, hull, warm_light);
    add_dropped_ceiling(s, 10, lh, 6, 8, 6, 0.3f, hull, warm_light);

    // ================================================================
    // RECEPTION / CHECK-IN COUNTER — long, marble top, brass edge
    // ================================================================
    // Main counter — 6m long, along left side facing the room
    add_bar_counter(s, -12, 0, -4, 6, false, marble_a, hull_lt);
    // Brass edge trim on counter top
    add_wall(s, -12, 1.12f, -4, 6.2f, 0.03f, 0.03f, brass);
    set_last_material(s, MAT_BRASS);
    // Desk lamp — green banker's lamp
    add_cylinder(s, -10, 1.15f, -4, 0.06f, 0.04f, brass);
    set_last_material(s, MAT_BRASS);
    add_cylinder(s, -10, 1.25f, -4, 0.03f, 0.18f, brass);
    set_last_material(s, MAT_BRASS);
    add_cone(s, -10, 1.45f, -4, 0.18f, 0.12f, (Color){40, 100, 60, 200});
    add_light_panel(s, -10, 1.35f, -4, 0.25f, 0.3f, 0.25f, warm_light);
    // Second desk lamp
    add_cylinder(s, -14, 1.15f, -4, 0.06f, 0.04f, brass);
    set_last_material(s, MAT_BRASS);
    add_cylinder(s, -14, 1.25f, -4, 0.03f, 0.18f, brass);
    set_last_material(s, MAT_BRASS);
    add_cone(s, -14, 1.45f, -4, 0.18f, 0.12f, (Color){40, 100, 60, 200});
    add_light_panel(s, -14, 1.35f, -4, 0.25f, 0.3f, 0.25f, warm_light);
    // Hotel bell
    add_cylinder(s, -11, 1.18f, -3.5f, 0.07f, 0.04f, (Color){200,195,180,255});
    set_last_material(s, MAT_BRASS);
    // Guest book — open, leather bound
    add_wall(s, -13, 1.15f, -3.6f, 0.45f, 0.03f, 0.35f, (Color){60,40,25,255});
    set_last_material(s, MAT_LEATHER);
    add_wall(s, -13, 1.17f, -3.6f, 0.4f, 0.005f, 0.32f, cream);  // pages
    add_wall(s, -12.85f, 1.18f, -3.5f, 0.02f, 0.005f, 0.16f, gold);  // pen
    // Room key — brass, waiting for you
    add_wall(s, -11.5f, 1.16f, -3.8f, 0.1f, 0.006f, 0.05f, brass);
    set_last_material(s, MAT_BRASS);
    // Champagne flute — welcome drink
    add_cylinder(s, -12.5f, 1.2f, -3.4f, 0.04f, 0.14f, (Color){210,210,215,160});
    set_last_material(s, MAT_GLASS);
    add_wall(s, -12.5f, 1.14f, -3.4f, 0.04f, 0.06f, 0.04f, (Color){240,210,100,180});

    // ================================================================
    // SEATING CLUSTER — 2 sofas facing each other, coffee table between
    // ================================================================
    // Right side of lobby
    add_sofa(s, 12, 0, 0, 270, godard_red);    // facing left (toward center)
    add_sofa(s, 8, 0, 0, 90, godard_red);      // facing right (toward first sofa)
    // Coffee table between sofas
    add_wall(s, 10, 0.4f, 0, 1.2f, 0.04f, 0.6f, marble_a);
    set_last_material(s, MAT_MARBLE);
    // Table legs — brass
    add_cylinder(s, 9.5f, 0.2f, -0.2f, 0.03f, 0.4f, brass);
    add_cylinder(s, 10.5f, 0.2f, -0.2f, 0.03f, 0.4f, brass);
    add_cylinder(s, 9.5f, 0.2f, 0.2f, 0.03f, 0.4f, brass);
    add_cylinder(s, 10.5f, 0.2f, 0.2f, 0.03f, 0.4f, brass);
    // Navy rug under seating cluster
    add_rug(s, 10, 0, 0, 6, 4, navy, brass);

    // ================================================================
    // TWO CHAIRS angled toward Earth window — Commandment 9
    // For watching together. The quiet emotional beat.
    // ================================================================
    add_chair(s, -4, 0, -10, 200, brass, navy);    // angled toward window
    add_chair(s, -2, 0, -10, 160, brass, navy);    // angled toward window
    // Small side table between the chairs
    add_wall(s, -3, 0.5f, -10.2f, 0.5f, 0.03f, 0.5f, marble_a);
    set_last_material(s, MAT_MARBLE);
    add_cylinder(s, -3, 0.25f, -10.2f, 0.04f, 0.5f, brass);
    set_last_material(s, MAT_BRASS);

    // ================================================================
    // POTTED TREES / PLANTS — organic life in a station, brass planters
    // ================================================================
    // Large planter 1 — near elevator
    add_cylinder(s, 4, 0.4f, 5, 0.5f, 0.8f, brass);
    set_last_material(s, MAT_BRASS);
    add_wall(s, 4, 0.82f, 5, 0.45f, 0.05f, 0.45f, (Color){50, 35, 20, 255}); // soil
    add_cylinder(s, 4, 1.3f, 5, 0.06f, 0.9f, (Color){70, 55, 35, 255}); // trunk
    // Foliage — clusters of green spheres
    add_sphere(s, 4, 1.9f, 5, 0.5f, (Color){45, 95, 50, 220});
    add_sphere(s, 3.8f, 2.1f, 4.8f, 0.35f, (Color){55, 110, 55, 200});
    add_sphere(s, 4.2f, 2.0f, 5.2f, 0.4f, (Color){40, 85, 45, 210});

    // Large planter 2 — other side of elevator
    add_cylinder(s, -4, 0.4f, 5, 0.5f, 0.8f, brass);
    set_last_material(s, MAT_BRASS);
    add_wall(s, -4, 0.82f, 5, 0.45f, 0.05f, 0.45f, (Color){50, 35, 20, 255});
    add_cylinder(s, -4, 1.3f, 5, 0.06f, 0.9f, (Color){70, 55, 35, 255});
    add_sphere(s, -4, 1.9f, 5, 0.5f, (Color){45, 95, 50, 220});
    add_sphere(s, -4.2f, 2.1f, 4.8f, 0.35f, (Color){55, 110, 55, 200});
    add_sphere(s, -3.8f, 2.0f, 5.2f, 0.4f, (Color){40, 85, 45, 210});

    // Smaller planter near reception
    add_cylinder(s, -15.5f, 0.3f, -3, 0.35f, 0.6f, brass);
    set_last_material(s, MAT_BRASS);
    add_sphere(s, -15.5f, 1.0f, -3, 0.4f, (Color){50, 100, 55, 220});
    add_sphere(s, -15.3f, 1.15f, -3.1f, 0.3f, (Color){55, 110, 55, 200});

    // Planter near corridor exit
    add_cylinder(s, 8, 0.3f, 12, 0.35f, 0.6f, brass);
    set_last_material(s, MAT_BRASS);
    add_sphere(s, 8, 1.0f, 12, 0.4f, (Color){50, 100, 55, 220});
    add_sphere(s, 8.2f, 1.15f, 11.9f, 0.3f, (Color){45, 90, 50, 200});

    // ================================================================
    // ART PIECE / SCULPTURE — striking, center of the space
    // Abstract brass form on a marble pedestal, between elevator and window
    // ================================================================
    // Marble pedestal
    add_wall(s, 0, 0.4f, -6, 1.2f, 0.8f, 1.2f, marble_b);
    set_last_material(s, MAT_MARBLE);
    add_wall(s, 0, 0.82f, -6, 1.4f, 0.04f, 1.4f, marble_a);
    set_last_material(s, MAT_MARBLE);
    // Sculpture — abstract ascending form (brass + red accent)
    add_cylinder(s, 0, 1.2f, -6, 0.15f, 0.7f, brass);
    set_last_material(s, MAT_BRASS);
    add_sphere(s, 0, 1.8f, -6, 0.3f, brass);
    set_last_material(s, MAT_BRASS);
    add_wall(s, 0.15f, 2.0f, -6, 0.08f, 0.6f, 0.08f, brass);
    set_last_material(s, MAT_BRASS);
    set_last_rotation(s, 15.0f);
    add_wall(s, -0.1f, 2.2f, -6, 0.06f, 0.5f, 0.06f, brass);
    set_last_material(s, MAT_BRASS);
    set_last_rotation(s, -20.0f);
    add_sphere(s, 0.05f, 2.5f, -6, 0.2f, godard_red); // red accent — the Godard touch
    // Spot light on sculpture
    add_light_panel(s, 0, 0.86f, -6, 1.5f, 0.15f, 1.5f, warm_light);

    // ================================================================
    // PICTURE FRAMES — flanking the observation window
    // ================================================================
    add_picture_frame(s, -15, lh*0.55f, -ld/2+0.3f, 1.5f, 1.0f, brass, godard_red);
    add_picture_frame(s, 15, lh*0.55f, -ld/2+0.3f, 1.5f, 1.0f, brass, godard_blue);
    // Additional art on side walls
    add_picture_frame(s, -lw/2+0.3f, lh*0.5f, -2, 1.2f, 0.9f, brass, navy);
    add_picture_frame(s, lw/2-0.3f, lh*0.5f, -2, 1.2f, 0.9f, brass, (Color){140, 45, 40, 255});

    // ================================================================
    // UPPER MEZZANINE — brass observation gallery at 6m
    // Implies the hotel has more floors. Brass railing, inaccessible but visible.
    // ================================================================
    // Main platform — wraps around three sides (not the observation window wall)
    // Left mezzanine
    add_wall(s, -lw/2+2.5f, 6.0f, 0, 5, 0.12f, ld-4, hull_lt);
    set_last_material(s, MAT_CONCRETE);
    // Right mezzanine
    add_wall(s, lw/2-2.5f, 6.0f, 0, 5, 0.12f, ld-4, hull_lt);
    set_last_material(s, MAT_CONCRETE);
    // Front mezzanine (connecting bridge)
    add_wall(s, 0, 6.0f, ld/2-2.5f, lw-10, 0.12f, 5, hull_lt);
    set_last_material(s, MAT_CONCRETE);

    // Brass railings — inner edge
    // Left railing
    add_wall(s, -lw/2+5, 6.5f, 0, 0.06f, 0.9f, ld-4, brass);
    set_last_material(s, MAT_BRASS);
    // Right railing
    add_wall(s, lw/2-5, 6.5f, 0, 0.06f, 0.9f, ld-4, brass);
    set_last_material(s, MAT_BRASS);
    // Front railing
    add_wall(s, 0, 6.5f, ld/2-5, lw-10, 0.9f, 0.06f, brass);
    set_last_material(s, MAT_BRASS);
    // Railing posts — every 3m
    for (int i = 0; i < 9; i++) {
        float rz = -ld/2+3 + i * 3;
        add_cylinder(s, -lw/2+5, 6.3f, rz, 0.04f, 0.6f, brass);
        set_last_material(s, MAT_BRASS);
        add_cylinder(s, lw/2-5, 6.3f, rz, 0.04f, 0.6f, brass);
        set_last_material(s, MAT_BRASS);
    }
    for (int i = 0; i < 7; i++) {
        float rx = -lw/2+6 + i * 4;
        add_cylinder(s, rx, 6.3f, ld/2-5, 0.04f, 0.6f, brass);
        set_last_material(s, MAT_BRASS);
    }

    // Mezzanine support brackets — brass corbels
    for (int i = 0; i < 4; i++) {
        float bz = -8 + i * 5;
        // Left brackets
        add_wall(s, -lw/2+1, 5.5f, bz, 2, 0.6f, 0.15f, brass);
        set_last_material(s, MAT_BRASS);
        // Right brackets
        add_wall(s, lw/2-1, 5.5f, bz, 2, 0.6f, 0.15f, brass);
        set_last_material(s, MAT_BRASS);
    }

    // Mezzanine step ledges — parkour stepping stones
    // Left wall: low ledge at 3m, high at 4.8m
    add_wall(s, -lw/2+0.3f, 3.0f, 6, 0.25f, 0.18f, 4, brass);
    set_last_material(s, MAT_BRASS);
    add_wall(s, -lw/2+0.3f, 4.8f, 4, 0.25f, 0.18f, 3, brass);
    set_last_material(s, MAT_BRASS);
    // Right wall matching
    add_wall(s, lw/2-0.3f, 3.0f, 6, 0.25f, 0.18f, 4, brass);
    set_last_material(s, MAT_BRASS);
    add_wall(s, lw/2-0.3f, 4.8f, 4, 0.25f, 0.18f, 3, brass);
    set_last_material(s, MAT_BRASS);

    // ================================================================
    // STATION INFRASTRUCTURE — air vents, conduit, emergency lighting
    // ================================================================
    // Air vents — horizontal slotted grilles on walls near ceiling
    for (int i = 0; i < 4; i++) {
        float vz = -8 + i * 6;
        // Left wall vents
        add_wall(s, -lw/2+0.28f, lh-1.5f, vz, 0.06f, 0.4f, 1.0f, hull_lt);
        set_last_material(s, MAT_CONCRETE);
        // Vent slats (3 horizontal strips)
        for (int j = 0; j < 3; j++) {
            add_wall(s, -lw/2+0.3f, lh-1.65f+j*0.15f, vz, 0.04f, 0.03f, 0.9f, (Color){40,45,55,255});
        }
        // Right wall vents
        add_wall(s, lw/2-0.28f, lh-1.5f, vz, 0.06f, 0.4f, 1.0f, hull_lt);
        set_last_material(s, MAT_CONCRETE);
        for (int j = 0; j < 3; j++) {
            add_wall(s, lw/2-0.3f, lh-1.65f+j*0.15f, vz, 0.04f, 0.03f, 0.9f, (Color){40,45,55,255});
        }
    }

    // Conduit pipes — running along ceiling edges
    add_cylinder(s, -lw/2+1.5f, lh-0.3f, 0, 0.08f, ld-2, hull_lt);
    set_last_rotation(s, 90.0f);
    add_cylinder(s, lw/2-1.5f, lh-0.3f, 0, 0.08f, ld-2, hull_lt);
    set_last_rotation(s, 90.0f);

    // Emergency lighting — small red indicators near exits
    add_wall(s, -3, lh-0.8f, ld/2-0.3f, 0.2f, 0.08f, 0.04f, godard_red);
    add_light_panel(s, -3, lh-0.8f, ld/2-0.28f, 0.25f, 0.12f, 0.08f, (Color){200, 50, 40, 60});
    add_wall(s, 3, lh-0.8f, ld/2-0.3f, 0.2f, 0.08f, 0.04f, godard_red);
    add_light_panel(s, 3, lh-0.8f, ld/2-0.28f, 0.25f, 0.12f, 0.08f, (Color){200, 50, 40, 60});

    // ================================================================
    // CORRIDOR EXITS — doorways to elevator and hotel wings
    // ================================================================
    // Main exit — back to elevator/hyperspace (front wall, center)
    add_door_frame(s, 0, 1.5f, ld/2-0.24f, 2.0f, 3.0f, 0.3f, brass);
    // Left wing exit
    add_door_frame(s, -8, 1.5f, ld/2-0.24f, 1.6f, 3.0f, 0.3f, brass);
    // Right wing exit
    add_door_frame(s, 8, 1.5f, ld/2-0.24f, 1.6f, 3.0f, 0.3f, brass);

    // ================================================================
    // INFORMATION KIOSK — near entrance
    // ================================================================
    add_wall(s, 6, 0.6f, 10, 0.8f, 1.2f, 0.6f, hull_lt);
    set_last_material(s, MAT_CONCRETE);
    // Screen — dark with subtle glow
    add_wall(s, 6, 1.0f, 9.68f, 0.6f, 0.4f, 0.02f, (Color){15, 25, 45, 255});
    add_light_panel(s, 6, 1.0f, 9.66f, 0.65f, 0.45f, 0.06f, (Color){60, 130, 200, 30});

    // ================================================================
    // LUGGAGE TROLLEY — brass frame, someone just arrived
    // ================================================================
    // Frame
    add_wall(s, -6, 0.35f, 8, 1.0f, 0.04f, 0.6f, brass);
    set_last_material(s, MAT_BRASS);
    // Wheels
    add_cylinder(s, -6.4f, 0.08f, 7.75f, 0.1f, 0.04f, brass);
    add_cylinder(s, -5.6f, 0.08f, 7.75f, 0.1f, 0.04f, brass);
    add_cylinder(s, -6.4f, 0.08f, 8.25f, 0.1f, 0.04f, brass);
    add_cylinder(s, -5.6f, 0.08f, 8.25f, 0.1f, 0.04f, brass);
    // Handle — vertical post + cross bar
    add_wall(s, -6, 0.7f, 8.3f, 0.04f, 0.7f, 0.04f, brass);
    set_last_material(s, MAT_BRASS);
    add_wall(s, -6, 1.05f, 8.3f, 0.6f, 0.04f, 0.04f, brass);
    set_last_material(s, MAT_BRASS);
    // Suitcase on trolley
    add_wall(s, -6, 0.55f, 8, 0.7f, 0.35f, 0.45f, (Color){150,100,60,255});
    set_last_material(s, MAT_LEATHER);
    // Luggage tag — blue
    add_wall(s, -5.7f, 0.6f, 8.2f, 0.12f, 0.08f, 0.02f, godard_blue);

    // ================================================================
    // SIGNAGE — station directional signs
    // ================================================================
    // Hanging sign — "SUITES" (represented as dark panel with brass border)
    add_cylinder(s, -3, lh-0.5f, 8, 0.02f, 0.6f, brass); // chain
    add_wall(s, -3, lh-1.3f, 8, 1.2f, 0.4f, 0.04f, navy);
    add_wall(s, -3, lh-1.1f, 8, 1.1f, 0.06f, 0.03f, white); // text line
    // Frame
    add_wall(s, -3, lh-1.1f, 8.02f, 1.25f, 0.04f, 0.02f, brass);
    set_last_material(s, MAT_BRASS);
    add_wall(s, -3, lh-1.5f, 8.02f, 1.25f, 0.04f, 0.02f, brass);
    set_last_material(s, MAT_BRASS);

    // ================================================================
    // BOOKSHELVES — station library corners
    // ================================================================
    add_bookshelf(s, -lw/2+0.35f, 1.8f, -ld/2+2, 2.2f, 1.8f, 3, cream);
    add_bookshelf(s, lw/2-0.35f, 1.8f, -ld/2+2, 2.2f, 1.8f, 3, cream);

    // ================================================================
    // FRESH FLOWERS — in brass vases on surfaces
    // ================================================================
    // On coffee table
    add_cylinder(s, 10, 0.45f, 0, 0.05f, 0.14f, (Color){200,210,220,140});
    set_last_material(s, MAT_GLASS);
    add_wall(s, 9.95f, 0.62f, 0, 0.02f, 0.12f, 0.02f, (Color){60,120,50,255});
    add_wall(s, 10.05f, 0.65f, 0, 0.02f, 0.1f, 0.02f, (Color){60,120,50,255});
    add_sphere(s, 9.95f, 0.72f, 0, 0.04f, (Color){220,180,190,255});
    add_sphere(s, 10.05f, 0.74f, 0, 0.035f, (Color){200,160,170,255});

    // Near Earth-watching chairs
    add_cylinder(s, -3, 0.55f, -10.2f, 0.04f, 0.1f, (Color){200,210,220,140});
    set_last_material(s, MAT_GLASS);
    add_sphere(s, -3.02f, 0.68f, -10.2f, 0.035f, (Color){195,45,40,255}); // red bloom

    // ================================================================
    // VOLUMETRIC LIGHT SHAFTS — Earth glow streaming through windows
    // ================================================================
    add_wall(s, -4, lh*0.35f, -10, 3.0f, lh*0.5f, 2.5f, (Color){40, 90, 180, 8});
    add_wall(s, 4, lh*0.4f, -9, 2.5f, lh*0.45f, 3.0f, (Color){35, 80, 160, 6});
    add_wall(s, 0, lh*0.3f, -7, 5.0f, lh*0.4f, 4.0f, (Color){30, 70, 140, 4});
    // Side window shafts
    for (int i = 0; i < 3; i++) {
        float sz = -8 + i * 5;
        add_wall(s, -lw/2+5, lh*0.35f, sz, 2.0f, lh*0.4f, 1.5f, (Color){35, 85, 170, 5});
        add_wall(s, lw/2-5, lh*0.35f, sz, 2.0f, lh*0.4f, 1.5f, (Color){35, 85, 170, 5});
    }

    // ================================================================
    // FLOATING DEBRIS — zero-g life, the hotel breathes
    // ================================================================
    // Wine glass hovering mid-air
    add_wall(s, 8, 2.2f, -5, 0.05f, 0.14f, 0.05f, (Color){210,210,215,160});
    add_wall(s, 8, 2.26f, -5, 0.035f, 0.05f, 0.035f, (Color){140,35,45,200});
    // Book drifting near ceiling
    add_wall(s, -12, 7.5f, 3, 0.8f, 0.06f, 0.5f, godard_red);
    set_last_material(s, MAT_LEATHER);
    // Suitcase floating at waist height
    add_wall(s, 14, 1.5f, 6, 0.8f, 0.35f, 0.5f, (Color){150,100,60,255});
    set_last_material(s, MAT_LEATHER);
    add_wall(s, 13.85f, 1.62f, 6.22f, 0.18f, 0.13f, 0.02f, godard_blue);
    // Napkin drifting
    add_wall(s, -5, 4.5f, 7, 0.3f, 0.01f, 0.3f, white);
    // Pen floating end-over-end
    add_wall(s, 5, 4.2f, 3, 0.02f, 0.02f, 0.2f, gold);
    // Key card
    add_wall(s, 13, 3.5f, -6, 0.12f, 0.005f, 0.08f, white);
    // Coffee cup inverted
    add_cylinder(s, -14, 4.0f, 6, 0.08f, 0.1f, cream);
    // Coffee droplet
    add_sphere(s, -13.8f, 4.3f, 5.8f, 0.04f, (Color){90, 60, 30, 180});
    // Second napkin — slow tumble
    add_wall(s, -8, 5.5f, -3, 0.25f, 0.005f, 0.22f, cream);

    // ================================================================
    // STARS — behind all windows
    // ================================================================
    for (int i = 0; i < 20; i++) {
        float sx = -40 + (i*41)%80;
        float sy = 2 + (i*17)%8;
        float sz = -ld/2 - 5 - (i*13)%20;
        add_wall(s, sx, sy, sz, 0.12f, 0.12f, 0.12f,
                 (Color){240,238,232,(unsigned char)(140+(i*23)%80)});
    }
    // Stars behind side windows
    for (int i = 0; i < 10; i++) {
        float side = (i % 2 == 0) ? -1 : 1;
        float sx = side * (lw/2 + 3 + (i*7)%10);
        float sy = 2 + (i*13)%8;
        float sz = -10 + (i*11)%20;
        add_wall(s, sx, sy, sz, 0.1f, 0.1f, 0.1f,
                 (Color){240,238,232,(unsigned char)(120+(i*31)%100)});
    }

    // ================================================================
    // THE MOTIF: cigarette — floating in zero-g near reception
    // The previous guest's last act before checking in.
    // ================================================================
    add_cylinder(s, -8, 2.2f, -2, 0.1f, 0.45f, (Color){230,225,215,160});
    set_last_decal(s);
    // Ash drifting
    add_wall(s, -7.9f, 2.3f, -1.9f, 0.1f, 0.1f, 0.1f, (Color){120,115,110,80});
    set_last_decal(s);
    // Smoke wisp — faint transparent rectangle
    add_wall(s, -7.85f, 2.4f, -1.85f, 0.04f, 0.15f, 0.04f, (Color){180,178,175,15});

    // Interactive objects
    add_object(s, -11, 1.18f, -3.5f, "bell", (Color){200,195,180,255}, 1);
    add_object(s, 8, 2.2f, -5, "wineglass", (Color){210,210,215,255}, 1);

    tag_materials_by_color(s);

    // ── GIBBONS MODEL — if the GLB is loaded, place him at the desk ──
    {
        int gi = find_model_asset("gibbons");
        if (gi >= 0) {
            // WHITE = preserve Blender's own material colors (multi-material model)
            add_model(s, -10, 0, -2, 1,1,1, 0, gi, MAT_CONCRETE, (Color){255,255,255,255});
        }
    }

    // Spawn facing the observation window
    s->spawn = (Vector3){0, 1.6f, 8};
    s->exit_pos = (Vector3){0, 1.6f, ld/2-1};
    s->has_exit = true;

}


void build_space_corridor(Scene *s) {
    memset(s, 0, sizeof(Scene));
    // BOUNDS: curved corridor — 4.5m wide, 32m long arc, hull walls with windows
    // Kubrick hallway. Gibbons leads. Doors suggest other guests' lives.
    s->surface = SURFACE_CARPET;

    Color hull = PAL_HULL;
    Color hull_lt = PAL_HULL_LIGHT;
    Color hull_dark = {22, 24, 32, 255};
    Color brass = PAL_BRASS;
    Color brass_dark = {155, 135, 92, 255};
    Color cream = PAL_CREAM;
    Color void_black = PAL_PORTHOLE;
    Color carpet_a = {85, 70, 105, 255};     // warm violet
    Color carpet_b = {95, 78, 112, 255};
    Color carpet_runner = {120, 35, 30, 255};  // burgundy center strip
    Color warm_amber = PAL_GLOW_AMBER;
    Color door_colors[3] = {PAL_RED, PAL_BLUE, PAL_NAVY};
    Color pipe_metal = {70, 68, 62, 255};

    s->fog_color = (Color){6, 10, 22, 255};
    s->fog_density = 0.0008f;

    float W = 4.5f, H = 3.5f;

    // CURVED CORRIDOR — 8 segments along a gentle arc
    int segs = 8;
    float seg_len = 4.0f;
    float curve_radius = 40.0f;
    float total_angle = (segs * seg_len) / curve_radius;
    float start_angle = -total_angle / 2;

    for (int i = 0; i < segs; i++) {
        float a0 = start_angle + i * (total_angle / segs);
        float a1 = start_angle + (i + 1) * (total_angle / segs);
        float amid = (a0 + a1) / 2;

        float cx = sinf(amid) * curve_radius;
        float cz = -cosf(amid) * curve_radius + curve_radius;

        // ── FLOOR ──
        // Hull floor edges visible on sides
        add_wall(s, cx - W/2 + 0.3f, -0.05f, cz, 0.6f, 0.1f, seg_len - 0.1f, hull_dark);
        set_last_material(s, MAT_CONCRETE);
        add_wall(s, cx + W/2 - 0.3f, -0.05f, cz, 0.6f, 0.1f, seg_len - 0.1f, hull_dark);
        set_last_material(s, MAT_CONCRETE);
        // Carpet tiles (center area)
        for (int t = 0; t < 3; t++) {
            float tx = cx - W/2 + 0.6f + t * 1.1f + 0.55f;
            add_wall(s, tx, -0.05f, cz, 1.08f, 0.1f, seg_len - 0.1f,
                     ((i + t) % 2 == 0) ? carpet_a : carpet_b);
            set_last_material(s, MAT_CARPET);
        }
        // Carpet runner — burgundy center strip
        add_wall(s, cx, 0.01f, cz, 0.8f, 0.02f, seg_len - 0.15f, carpet_runner);
        set_last_material(s, MAT_CARPET);
        set_last_decal(s);

        // ── CEILING ──
        add_wall(s, cx, H, cz, W, 0.15f, seg_len, hull);
        set_last_material(s, MAT_CONCRETE);
        // Structural trim — brass line at each segment join
        add_wall(s, cx, H - 0.05f, cz - seg_len/2 + 0.03f, W + 0.1f, 0.08f, 0.06f, brass);
        set_last_material(s, MAT_BRASS);
        add_wall(s, cx, H - 0.05f, cz + seg_len/2 - 0.03f, W + 0.1f, 0.08f, 0.06f, brass);
        set_last_material(s, MAT_BRASS);

        // Ceiling light strip — amber pool at each segment (rhythmic)
        if (i % 2 == 0) {
            // Recessed light housing
            add_wall(s, cx, H - 0.06f, cz, 0.6f, 0.04f, 1.5f, hull_dark);
            set_last_material(s, MAT_CONCRETE);
            add_light_panel(s, cx, H - 0.1f, cz, 0.4f, 0.03f, 1.2f, warm_amber);
        }

        // ── WALLS ──
        add_wall(s, cx - W/2, H/2, cz, 0.15f, H, seg_len, hull);
        set_last_material(s, MAT_CONCRETE);
        add_wall(s, cx + W/2, H/2, cz, 0.15f, H, seg_len, hull);
        set_last_material(s, MAT_CONCRETE);

        // Inner paneling — cream on non-window, non-door segments
        // Window segments (odd) get panels on the non-window side only
        bool left_panel = true, right_panel = true;
        if (i % 2 == 1) {
            if (i % 4 == 1) left_panel = false;   // window on left
            else right_panel = false;               // window on right
        }
        if (i >= 2 && i <= 6 && i % 2 == 0) {
            // Door segments — panels on non-door side
            if (i % 4 == 0) left_panel = false;
            else right_panel = false;
        }
        if (left_panel) {
            add_wall(s, cx - W/2 + 0.14f, H * 0.375f, cz, 0.04f, H * 0.75f, seg_len - 0.2f, cream);
            set_last_material(s, MAT_WALLPAPER);
            // Recessed wall panel — darker inset for depth
            add_wall(s, cx - W/2 + 0.15f, H * 0.35f, cz, 0.02f, H * 0.45f, seg_len * 0.6f,
                     (Color){215, 210, 200, 255});
            set_last_material(s, MAT_WALLPAPER);
        }
        if (right_panel) {
            add_wall(s, cx + W/2 - 0.14f, H * 0.375f, cz, 0.04f, H * 0.75f, seg_len - 0.2f, cream);
            set_last_material(s, MAT_WALLPAPER);
            add_wall(s, cx + W/2 - 0.15f, H * 0.35f, cz, 0.02f, H * 0.45f, seg_len * 0.6f,
                     (Color){215, 210, 200, 255});
            set_last_material(s, MAT_WALLPAPER);
        }

        // Brass trim at wainscot height — both walls
        add_wall(s, cx - W/2 + 0.15f, H * 0.5f, cz, 0.02f, 0.04f, seg_len - 0.2f, brass);
        set_last_material(s, MAT_BRASS);
        add_wall(s, cx + W/2 - 0.15f, H * 0.5f, cz, 0.02f, 0.04f, seg_len - 0.2f, brass);
        set_last_material(s, MAT_BRASS);
        // Baseboard — brass strip at floor
        add_wall(s, cx - W/2 + 0.12f, 0.06f, cz, 0.03f, 0.12f, seg_len - 0.2f, brass_dark);
        set_last_material(s, MAT_BRASS);
        add_wall(s, cx + W/2 - 0.12f, 0.06f, cz, 0.03f, 0.12f, seg_len - 0.2f, brass_dark);
        set_last_material(s, MAT_BRASS);

        // Side cove lighting — warm amber wash
        if (i % 2 == 0) {
            add_light_panel(s, cx - W/2 + 0.2f, H - 0.15f, cz, 0.15f, 0.08f, seg_len * 0.8f, warm_amber);
            add_light_panel(s, cx + W/2 - 0.2f, H - 0.15f, cz, 0.15f, 0.08f, seg_len * 0.8f, warm_amber);
        }

        // ── PIPE CONDUITS along ceiling ──
        // Thin conduit pipes running along the ceiling edge
        add_wall(s, cx - W/2 + 0.25f, H - 0.12f, cz, 0.06f, 0.06f, seg_len, pipe_metal);
        set_last_material(s, MAT_BRASS);
        add_wall(s, cx + W/2 - 0.25f, H - 0.12f, cz, 0.06f, 0.06f, seg_len, pipe_metal);
        set_last_material(s, MAT_BRASS);

        // ── PORTHOLE WINDOWS — one side shows void/stars ──
        if (i % 2 == 1) {
            float side = (i % 4 == 1) ? -(W/2 - 0.1f) : (W/2 - 0.1f);
            float wx = cx + side;
            // CIRCULAR PORTHOLE — reads as bold circle at 960x600
            add_sphere(s, wx, H * 0.5f, cz, 1.0f, void_black);
            set_last_material(s, MAT_GLASS);
            // Brass porthole ring
            add_cylinder(s, wx, H * 0.5f, cz, 1.2f, 0.06f, brass);
            set_last_material(s, MAT_BRASS);
            // Inner ring — darker
            add_cylinder(s, wx, H * 0.5f, cz, 1.05f, 0.04f, brass_dark);
            set_last_material(s, MAT_BRASS);
            // Horizontal mullion across porthole
            add_wall(s, wx, H * 0.5f, cz, 0.03f, 0.05f, 1.0f, brass);
            set_last_material(s, MAT_BRASS);

            // Stars through porthole — two depths
            float star_dir = (side > 0) ? 1.0f : -1.0f;
            add_wall(s, wx + star_dir * 3.0f, H * 0.55f, cz - 1.0f,
                     0.08f, 0.08f, 0.08f, (Color){240, 238, 232, 180});
            add_wall(s, wx + star_dir * 5.0f, H * 0.55f + 0.5f, cz + 0.5f,
                     0.05f, 0.05f, 0.05f, (Color){200, 210, 230, 120});
            add_wall(s, wx + star_dir * 4.0f, H * 0.3f, cz + 1.2f,
                     0.04f, 0.04f, 0.04f, (Color){230, 225, 218, 100});

            // Earth visible through porthole — drifts as you walk
            float earth_offset = star_dir * 8.0f;
            float earth_y = 0.8f - (float)i * 0.15f;
            add_sphere(s, wx + earth_offset, earth_y, cz, 2.5f, (Color){30, 60, 140, 100});
            add_sphere(s, wx + earth_offset, earth_y, cz, 2.7f, (Color){80, 140, 220, 25});
            // Earth glow on floor
            float shaft_x = cx + (side > 0 ? 0.8f : -0.8f);
            add_wall(s, shaft_x, 0.02f, cz, 2.0f, 0.02f, 1.8f, (Color){50, 100, 180, 40});
            set_last_decal(s);
            // Earth glow on ceiling
            add_wall(s, shaft_x, H - 0.05f, cz, 1.5f, 0.02f, 1.2f, (Color){40, 85, 160, 20});
            set_last_decal(s);
        }

        // ── DOORS — 3 doors on alternating sides ──
        // Each with different colored light underneath
        if (i >= 2 && i <= 6 && i % 2 == 0) {
            float side = (i % 4 == 0) ? -(W/2 - 0.1f) : (W/2 - 0.1f);
            int door_idx = (i - 2) / 2;
            Color door_c = door_colors[door_idx % 3];
            // Door panel
            add_wall(s, cx + side, 1.3f, cz, 0.12f, 2.6f, 1.0f, door_c);
            // Door frame — brass
            add_wall(s, cx + side, 2.75f, cz, 0.12f, 0.12f, 1.15f, brass);
            set_last_material(s, MAT_BRASS);
            add_wall(s, cx + side, 1.3f, cz - 0.57f, 0.12f, 2.6f, 0.06f, brass);
            set_last_material(s, MAT_BRASS);
            add_wall(s, cx + side, 1.3f, cz + 0.57f, 0.12f, 2.6f, 0.06f, brass);
            set_last_material(s, MAT_BRASS);
            // Room number plate — brass
            add_wall(s, cx + side * 0.95f, 2.0f, cz - 0.7f, 0.06f, 0.2f, 0.15f, brass);
            set_last_material(s, MAT_BRASS);
            // Number text (dark rectangle on plate)
            add_wall(s, cx + side * 0.94f, 2.0f, cz - 0.7f, 0.04f, 0.12f, 0.08f,
                     (Color){40, 35, 28, 255});
            // Door handle — brass sphere
            float handle_z = (side > 0) ? cz - 0.35f : cz + 0.35f;
            add_sphere(s, cx + side * 0.9f, 1.1f, handle_z, 0.06f, brass);
            set_last_material(s, MAT_BRASS);
            // Peephole — tiny brass circle
            add_cylinder(s, cx + side * 0.92f, 1.6f, cz, 0.04f, 0.03f, brass_dark);
            set_last_material(s, MAT_BRASS);

            // LIGHT UNDER DOOR — each leaks a different life
            Color door_light;
            switch (door_idx) {
                case 0: door_light = (Color){240, 210, 120, 140}; break;  // warm amber — reading
                case 1: door_light = (Color){80, 120, 200, 100};  break;  // cool blue — TV flicker
                default: door_light = (Color){0, 0, 0, 0};        break;  // darkness — empty room
            }
            if (door_light.a > 0) {
                add_wall(s, cx + side * 0.85f, 0.02f, cz, 0.06f, 0.02f, 0.9f, door_light);
                set_last_decal(s);
                // Light spill on adjacent carpet
                add_wall(s, cx + side * 0.6f, 0.015f, cz, 0.5f, 0.01f, 0.7f,
                         (Color){door_light.r, door_light.g, door_light.b, 40});
                set_last_decal(s);
            }
        }

        // ── BETWEEN-DOOR DETAIL — recessed panels, brass strips, frames ──
        if (i % 2 == 0 && (i < 2 || i > 6)) {
            // Picture frame on left wall
            add_picture_frame(s, cx - (W/2 - 0.08f), 1.8f, cz,
                              0.5f, 0.4f, brass, (Color){195, 45, 40, 255});
            // Picture frame on right wall
            add_picture_frame(s, cx + (W/2 - 0.08f), 1.8f, cz,
                              0.5f, 0.4f, brass, (Color){40, 65, 160, 255});
        }

        // ── EMERGENCY LIGHTING — small red indicator per segment ──
        add_wall(s, cx - W/2 + 0.13f, 0.3f, cz + seg_len/2 - 0.3f,
                 0.02f, 0.06f, 0.06f, (Color){200, 40, 30, 120});

        // ── FIRE SUPPRESSION — small nozzle on ceiling every other segment ──
        if (i % 3 == 0) {
            add_cylinder(s, cx + 0.8f, H - 0.1f, cz, 0.05f, 0.08f, (Color){85, 80, 75, 255});
            set_last_material(s, MAT_BRASS);
        }
    }

    // End caps — walls closing the corridor
    float end0_z = -cosf(start_angle) * curve_radius + curve_radius;
    float end1_z = -cosf(start_angle + total_angle) * curve_radius + curve_radius;
    float end0_x = sinf(start_angle) * curve_radius;
    float end1_x = sinf(start_angle + total_angle) * curve_radius;
    add_wall(s, end0_x, H/2, end0_z - seg_len/2, W, H, 0.25f, hull);
    add_wall(s, end1_x, H/2, end1_z + seg_len/2, W, H, 0.25f, hull);
    // End cap paneling
    add_wall(s, end0_x, H * 0.375f, end0_z - seg_len/2 + 0.14f,
             W - 0.5f, H * 0.75f, 0.04f, cream);
    set_last_material(s, MAT_WALLPAPER);
    add_wall(s, end1_x, H * 0.375f, end1_z + seg_len/2 - 0.14f,
             W - 0.5f, H * 0.75f, 0.04f, cream);
    set_last_material(s, MAT_WALLPAPER);

    // ============================================================
    // THE IMPOSSIBLE DOOR — opens to raw hull. Not a room.
    // ============================================================
    {
        float a_imp = start_angle + 6.5f * (total_angle / segs);
        float imp_cx = sinf(a_imp) * curve_radius;
        float imp_cz = -cosf(a_imp) * curve_radius + curve_radius;
        float imp_side = (W/2 - 0.1f);
        // Dark void behind door
        add_wall(s, imp_cx + imp_side - 0.02f, 1.3f, imp_cz, 0.06f, 2.6f, 0.95f,
                 (Color){8, 8, 12, 255});
        set_last_material(s, MAT_CONCRETE);
        // Ajar red door edge
        add_wall(s, imp_cx + imp_side + 0.04f, 1.3f, imp_cz - 0.45f, 0.08f, 2.6f, 0.08f,
                 door_colors[0]);
        // Frame
        add_wall(s, imp_cx + imp_side, 2.75f, imp_cz, 0.12f, 0.12f, 1.1f, brass);
        set_last_material(s, MAT_BRASS);
        add_wall(s, imp_cx + imp_side, 1.3f, imp_cz - 0.55f, 0.12f, 2.6f, 0.05f, brass);
        set_last_material(s, MAT_BRASS);
        add_wall(s, imp_cx + imp_side, 1.3f, imp_cz + 0.55f, 0.12f, 2.6f, 0.05f, brass);
        set_last_material(s, MAT_BRASS);
        // Number plate
        add_wall(s, imp_cx + imp_side * 0.95f, 2.0f, imp_cz - 0.7f, 0.06f, 0.2f, 0.15f, brass);
        set_last_material(s, MAT_BRASS);
    }

    // ============================================================
    // STRATEGIC EMPTINESS — missing hull panels, exposed ribs
    // ============================================================
    {
        float a_empty = start_angle + 4.5f * (total_angle / segs);
        float empty_cx = sinf(a_empty) * curve_radius;
        float empty_cz = -cosf(a_empty) * curve_radius + curve_radius;
        // Exposed structural ribs
        for (int r = 0; r < 3; r++) {
            add_wall(s, empty_cx + W/2 - 0.08f, 0.5f + r * 1.2f, empty_cz,
                     0.04f, 0.06f, 3.0f, hull_lt);
            set_last_material(s, MAT_CONCRETE);
        }
        add_cylinder(s, empty_cx + W/2 - 0.06f, H/2, empty_cz + 0.8f,
                     0.04f, H - 0.5f, (Color){120, 110, 95, 255});
        set_last_material(s, MAT_BRASS);
        // Dark void behind ribs
        add_wall(s, empty_cx + W/2 + 0.05f, H - 0.2f, empty_cz,
                 0.1f, 0.5f, 3.0f, (Color){15, 15, 20, 255});
        // Dangling cable
        add_cylinder(s, empty_cx + W/2 - 0.02f, H * 0.6f, empty_cz - 0.5f,
                     0.02f, 1.2f, (Color){50, 48, 42, 200});

        // ── THE VOID BETWEEN — maintenance shaft ──
        float vx = empty_cx + W/2 + 4.0f;
        float vz = empty_cz;
        float VH = 12.0f;
        float VW = 6.0f;

        // Floor — metal grating
        add_wall(s, vx, -0.05f, vz, VW, 0.1f, 8.0f, (Color){40, 42, 48, 255});
        set_last_material(s, MAT_CONCRETE);

        // Walls — raw hull
        add_wall(s, vx - VW/2, VH/2, vz, 0.15f, VH, 8.0f, hull_dark);
        set_last_material(s, MAT_CONCRETE);
        add_wall(s, vx + VW/2, VH/2, vz, 0.15f, VH, 8.0f, hull_dark);
        set_last_material(s, MAT_CONCRETE);
        add_wall(s, vx, VH/2, vz - 4.0f, VW, VH, 0.15f, (Color){18, 20, 28, 255});
        set_last_material(s, MAT_CONCRETE);
        add_wall(s, vx, VH/2, vz + 4.0f, VW, VH, 0.15f, (Color){18, 20, 28, 255});
        set_last_material(s, MAT_CONCRETE);
        add_wall(s, vx, VH, vz, VW, 0.15f, 8.0f, (Color){15, 16, 22, 255});
        set_last_material(s, MAT_CONCRETE);

        // CATWALKS — wall-runnable platforms
        add_wall(s, vx - 1.5f, 3.0f, vz, 1.2f, 0.08f, 4.0f, (Color){55, 52, 48, 255});
        set_last_material(s, MAT_BRASS);
        add_wall(s, vx + 1.5f, 5.5f, vz - 1.0f, 1.2f, 0.08f, 3.0f, (Color){55, 52, 48, 255});
        set_last_material(s, MAT_BRASS);
        add_wall(s, vx - 0.5f, 8.0f, vz + 1.0f, 2.0f, 0.08f, 2.0f, (Color){55, 52, 48, 255});
        set_last_material(s, MAT_BRASS);

        // Vertical pipes
        add_cylinder(s, vx - VW/2 + 0.3f, VH/2, vz - 1.5f, 0.12f, VH, pipe_metal);
        set_last_material(s, MAT_BRASS);
        add_cylinder(s, vx + VW/2 - 0.3f, VH/2, vz + 1.0f, 0.12f, VH, pipe_metal);
        set_last_material(s, MAT_BRASS);
        add_cylinder(s, vx - 1.0f, VH/2, vz - 3.5f, 0.10f, VH, (Color){65, 60, 55, 255});
        set_last_material(s, MAT_BRASS);

        // Horizontal pipes — mantle targets
        add_cylinder(s, vx, 4.2f, vz - 3.0f, 0.08f, VW - 1.0f, (Color){60, 58, 52, 255});
        set_last_rotation(s, 90.0f);
        set_last_material(s, MAT_BRASS);
        add_cylinder(s, vx, 7.0f, vz + 2.0f, 0.08f, VW - 2.0f, (Color){60, 58, 52, 255});
        set_last_rotation(s, 90.0f);
        set_last_material(s, MAT_BRASS);

        // Observation window at top — reward
        add_wall(s, vx, 9.5f, vz - 3.9f, 1.5f, 1.0f, 0.06f, void_black);
        set_last_material(s, MAT_GLASS);
        add_wall(s, vx, 9.5f, vz - 5.0f, 0.5f, 0.5f, 0.5f, (Color){60, 120, 200, 120});

        // Emergency lighting
        add_light_panel(s, vx, 0.3f, vz, 0.3f, 0.05f, 0.3f, (Color){180, 100, 60, 80});
        add_light_panel(s, vx, 6.0f, vz - 3.8f, 0.2f, 0.05f, 0.2f, (Color){100, 80, 60, 60});
    }

    // ============================================================
    // WINDOW-INTO-WINDOW — porthole into someone else's room
    // ============================================================
    {
        float a_win = start_angle + 3.5f * (total_angle / segs);
        float win_cx = sinf(a_win) * curve_radius;
        float win_cz = -cosf(a_win) * curve_radius + curve_radius;
        float win_side = -(W/2 - 0.1f);
        add_sphere(s, win_cx + win_side, H * 0.55f, win_cz, 0.9f, void_black);
        add_cylinder(s, win_cx + win_side, H * 0.55f, win_cz, 1.1f, 0.06f, brass);
        float room_x = win_cx + win_side - 2.5f;
        add_wall(s, room_x, H * 0.55f, win_cz, 0.3f, 0.5f, 0.3f,
                 (Color){200, 170, 120, 120});
        add_sphere(s, room_x - 0.3f, H * 0.55f, win_cz, 0.3f, void_black);
        add_cylinder(s, room_x - 0.3f, H * 0.55f, win_cz, 0.35f, 0.03f, brass);
        add_wall(s, room_x - 1.5f, H * 0.55f, win_cz,
                 0.06f, 0.06f, 0.06f, (Color){240, 238, 232, 200});

        // PORTHOLE GHOST — Gibbons silhouette through the porthole
        float room_x2 = win_cx + win_side - 3.5f;
        Color ghost = {25, 28, 45, 200};
        add_wall(s, room_x2, H * 0.55f - 0.1f, win_cz, 0.20f, 0.45f, 0.15f, ghost);
        add_wall(s, room_x2, H * 0.55f + 0.25f, win_cz, 0.14f, 0.16f, 0.12f, ghost);
        // Red cap
        add_wall(s, room_x2, H * 0.55f + 0.35f, win_cz, 0.16f, 0.04f, 0.14f,
                 (Color){140, 35, 30, 180});
        // Briefcase
        add_wall(s, room_x2 + 0.12f, H * 0.55f - 0.25f, win_cz, 0.14f, 0.10f, 0.05f,
                 (Color){120, 85, 35, 160});

        // ── CIGARETTE through porthole — the motif ──
        // In the neighbor's room. Someone else is waiting too.
        add_cylinder(s, room_x2 + 0.25f, H * 0.45f, win_cz + 0.1f,
                     0.008f, 0.03f, (Color){230, 225, 215, 100});
    }

    // ============================================================
    // FLOATING OBJECTS — life's detritus in zero-g
    // ============================================================
    float mid_x = sinf(0) * curve_radius;
    float mid_z = -cosf(0) * curve_radius + curve_radius;

    add_wall(s, end0_x - W/2 + 0.3f, 1.8f, end0_z, 0.2f, 0.5f, 0.15f, door_colors[0]);
    add_wall(s, mid_x + 0.5f, 2.2f, mid_z, 0.5f, 0.01f, 0.35f, (Color){235, 232, 228, 200});
    // Playing card
    add_wall(s, mid_x - 0.8f, 1.4f, mid_z + 3, 0.12f, 0.18f, 0.01f, (Color){245, 242, 238, 220});
    // Sock
    add_wall(s, end1_x + 0.3f, 2.8f, end1_z - 2, 0.06f, 0.15f, 0.04f, (Color){40, 38, 42, 200});
    // Pen cap
    add_cylinder(s, mid_x - 1.2f, 3.0f, mid_z - 2, 0.02f, 0.06f, brass);
    // Toothbrush
    add_wall(s, end0_x + 1.0f, 2.5f, end0_z + 5, 0.02f, 0.02f, 0.18f, (Color){240, 238, 234, 220});
    add_wall(s, end0_x + 1.0f, 2.5f, end0_z + 5.09f, 0.03f, 0.02f, 0.03f, (Color){60, 140, 180, 255});
    // Key card — floating face-up
    add_wall(s, mid_x + 1.5f, 1.9f, mid_z + 5, 0.12f, 0.01f, 0.08f, (Color){220, 215, 200, 200});
    // Wine glass stem (tiny, brass glint)
    add_cylinder(s, mid_x - 0.5f, 2.6f, mid_z + 7, 0.02f, 0.12f, brass);

    // Light shaft near first porthole
    add_wall(s, end0_x, 0.02f, end0_z + 3, 1.0f, 0.02f, 1.5f, (Color){60, 130, 200, 40});

    // ============================================================
    // ARCHITECTURAL DETAIL — brass runner, picture frames, signage
    // ============================================================

    // Brass runner inlay in floor — full corridor length
    add_wall(s, 0, 0.01f, mid_z, 0.3f, 0.02f, segs * seg_len * 0.8f, brass);
    set_last_material(s, MAT_BRASS);
    // Second runner line (parallel)
    add_wall(s, 0.5f, 0.01f, mid_z, 0.08f, 0.02f, segs * seg_len * 0.8f, brass_dark);
    set_last_material(s, MAT_BRASS);
    add_wall(s, -0.5f, 0.01f, mid_z, 0.08f, 0.02f, segs * seg_len * 0.8f, brass_dark);
    set_last_material(s, MAT_BRASS);

    // Picture frames between doors (segments without doors/windows)
    {
        float a_mid0 = start_angle + 0.5f * (total_angle / segs);
        float cx0 = sinf(a_mid0) * curve_radius;
        float cz0 = -cosf(a_mid0) * curve_radius + curve_radius;
        add_picture_frame(s, cx0 - (W/2 - 0.08f), 1.9f, cz0, 0.5f, 0.4f, brass, (Color){195, 45, 40, 255});
        add_picture_frame(s, cx0 + (W/2 - 0.08f), 1.9f, cz0, 0.5f, 0.4f, brass, (Color){40, 65, 160, 255});

        float a_mid7 = start_angle + 7.5f * (total_angle / segs);
        float cx7 = sinf(a_mid7) * curve_radius;
        float cz7 = -cosf(a_mid7) * curve_radius + curve_radius;
        add_picture_frame(s, cx7 - (W/2 - 0.08f), 1.9f, cz7, 0.5f, 0.4f, brass, PAL_GOLD);
        add_picture_frame(s, cx7 + (W/2 - 0.08f), 1.9f, cz7, 0.5f, 0.4f, brass, (Color){60, 90, 50, 255});
    }

    // ============================================================
    // TEMPORAL IMPOSSIBILITY — Commandment 7
    // The last window shows warm amber light, not void.
    // Dawn at 2 AM. Time doesn't work here.
    // ============================================================
    {
        float a_last = start_angle + 7.5f * (total_angle / segs);
        float last_cx = sinf(a_last) * curve_radius;
        float last_cz = -cosf(a_last) * curve_radius + curve_radius;
        float last_side = -(W/2 - 0.1f);
        // Dawn glow behind the glass
        add_wall(s, last_cx + last_side - 0.5f, 1.0f, last_cz,
                 0.02f, 2.0f, 1.2f, (Color){220, 160, 80, 60});
        // Dawn sky backing
        add_wall(s, last_cx + last_side - 1.0f, 1.5f, last_cz,
                 0.02f, 3.0f, 1.6f, (Color){180, 120, 50, 30});
        // Horizon line — thin warm band
        add_wall(s, last_cx + last_side - 0.8f, 0.5f, last_cz,
                 0.02f, 0.15f, 1.4f, (Color){240, 180, 80, 45});
        // Dawn light pooling on carpet
        add_wall(s, last_cx + last_side + 0.8f, 0.02f, last_cz,
                 1.5f, 0.02f, 1.2f, (Color){200, 150, 70, 35});
        set_last_decal(s);
        // Dawn glow on ceiling
        add_wall(s, last_cx + last_side + 0.5f, H - 0.04f, last_cz,
                 1.0f, 0.02f, 0.8f, (Color){180, 130, 60, 20});
        set_last_decal(s);
    }

    // Interactive objects
    add_object(s, mid_x + 0.5f, 2.2f, mid_z, "newspaper", (Color){235, 232, 228, 200}, 1);

    tag_materials_by_color(s);

    s->spawn = (Vector3){end0_x, 1.6f, end0_z};
    s->exit_pos = (Vector3){end1_x, 1.6f, end1_z};
    s->has_exit = true;

    // ================================================================
    // HIGH ROUTE — parkour path along corridor ceiling
    // Maintenance ledges for wall-run (already established, keep)
    // ================================================================
    // Left wall — continuous brass rail (mantleable)
    add_wall(s, -W/2 + 0.1f, 2.6f, seg_len * 4, 0.12f, 0.08f, seg_len * 7.5f,
             (Color){160, 140, 80, 180});
    set_last_material(s, MAT_BRASS);
    // Right wall — matching rail
    add_wall(s, W/2 - 0.1f, 2.6f, seg_len * 4, 0.12f, 0.08f, seg_len * 7.5f,
             (Color){160, 140, 80, 180});
    set_last_material(s, MAT_BRASS);
    // Rail support brackets
    for (int bi = 0; bi < 4; bi++) {
        float bz = seg_len + bi * seg_len * 2;
        add_wall(s, -W/2 + 0.08f, 2.55f, bz, 0.08f, 0.12f, 0.06f, brass_dark);
        set_last_material(s, MAT_BRASS);
        add_wall(s, W/2 - 0.08f, 2.55f, bz, 0.08f, 0.12f, 0.06f, brass_dark);
        set_last_material(s, MAT_BRASS);
    }
    // Ceiling vent grates — look down from high route
    for (int vi = 0; vi < 3; vi++) {
        float vz = 3.0f + vi * 8.0f;
        add_wall(s, -W/2 + 0.5f, H - 0.15f, vz, 0.8f, 0.1f, 0.5f,
                 (Color){80, 75, 70, 200});
        add_wall(s, -W/2 + 0.5f, H - 0.16f, vz, 0.6f, 0.01f, 0.35f,
                 (Color){240, 200, 120, (unsigned char)(100 - vi * 25)});
    }
    // Pipe obstacles on high route
    add_cylinder(s, 0, 2.9f, 8.0f, 0.06f, W, (Color){90, 85, 80, 255});
    add_cylinder(s, 0, 2.9f, 20.0f, 0.06f, W, (Color){90, 85, 80, 255});

}


void build_space_suite(Scene *s) {
    memset(s, 0, sizeof(Scene));
    // ============================================================
    // THE GLASS ELEVATOR SUITE — Orbital Hotel, Room 2046
    // Hotel Chevalier meets 2001. A room for two where only one arrived.
    // 60% of playtime happens here. Every surface earns its keep.
    // ============================================================
    s->surface = SURFACE_WOOD;

    Color hull      = PAL_HULL;
    Color hull_lt   = PAL_HULL_LIGHT;
    Color brass     = PAL_BRASS;
    Color cream     = PAL_CREAM;
    Color white     = PAL_WHITE;
    Color void_black= PAL_PORTHOLE;
    Color gold      = PAL_GOLD;
    Color wood      = PAL_WOOD_DARK;
    Color dark_wood = {105, 78, 48, 255};
    Color warm_light= PAL_LIGHT_WARM;
    Color earth_glow= PAL_EARTH_GLOW;
    Color navy      = PAL_NAVY;
    Color charcoal  = PAL_CHARCOAL;
    Color glass_clr = {200,210,220,140};

    s->fog_color = PAL_FOG_STATION;
    s->fog_density = 0.001f;

    // Room dimensions: 14m wide (X), 12m deep (Z), 5m tall (Y)
    // Left wall (X-) = window wall, Right wall (X+) = service wall
    // Back wall (Z-) = bed wall, Front wall (Z+) = entry wall
    float rw = 14, rd = 12, rh = 5;

    // ============================================================
    // 1. FLOOR — herringbone wood with area rug
    // ============================================================
    add_wall(s, 0, -0.05f, 0, rw, 0.1f, rd, wood);
    set_last_material(s, MAT_HERRINGBONE);

    // Living area rug — deep red with brass border, defines the zone
    add_rug(s, -3, 0, 2.5f, 5.0f, 4.0f, (Color){120,35,30,255}, brass);

    // ============================================================
    // 2. CEILING — hull panels with structural ribs
    // ============================================================
    add_wall(s, 0, rh, 0, rw, 0.2f, rd, hull);
    set_last_material(s, MAT_CONCRETE);

    // 3 longitudinal ceiling ribs — structural, not decorative
    for (int i = 0; i < 3; i++) {
        float rz = -rd/2 + 3 + i * (rd/4.0f);
        add_wall(s, 0, rh-0.05f, rz, rw, 0.1f, 0.08f, brass);
        set_last_material(s, MAT_BRASS);
    }
    // 2 transverse ribs — cross-bracing
    add_wall(s, -rw/4, rh-0.05f, 0, 0.08f, 0.1f, rd, hull_lt);
    add_wall(s, rw/4, rh-0.05f, 0, 0.08f, 0.1f, rd, hull_lt);

    // Dropped ceiling with warm light — above bed area
    add_dropped_ceiling(s, 0, rh, -4.5f, 5, 4, 0.25f, hull, warm_light);

    // Recessed light strip along window wall ceiling
    add_light_panel(s, -rw/2+0.5f, rh-0.12f, 0, 0.15f, 0.06f, rd*0.7f, warm_light);

    // ============================================================
    // 3. WALLS — hull exterior, cream wallpaper interior
    // ============================================================

    // BACK WALL (Z-) — behind bed
    add_wall(s, 0, rh/2, -rd/2, rw, rh, 0.3f, hull);
    set_last_material(s, MAT_CONCRETE);
    add_wall(s, 0, rh*0.4f, -rd/2+0.17f, rw-1, rh*0.8f, 0.04f, cream);
    set_last_material(s, MAT_WALLPAPER);
    // Wainscoting on back wall — proper panels, not just a line
    add_wainscoting(s, 0, 0, -rd/2+0.2f, rw-1.5f, 1.2f, false, cream, brass);

    // FRONT WALL (Z+) — entry
    add_wall(s, 0, rh/2, rd/2, rw, rh, 0.3f, hull);
    set_last_material(s, MAT_CONCRETE);
    add_wall(s, 0, rh*0.4f, rd/2-0.17f, rw-1, rh*0.8f, 0.04f, cream);
    set_last_material(s, MAT_WALLPAPER);
    // Wainscoting on front wall
    add_wainscoting(s, 0, 0, rd/2-0.2f, rw-1.5f, 1.2f, false, cream, brass);

    // LEFT WALL (X-) — window wall (mostly glass, cream above/below)
    add_wall(s, -rw/2, rh/2, 0, 0.3f, rh, rd, hull);
    set_last_material(s, MAT_CONCRETE);
    // Cream panel only on solid sections (above/below window, and ends)
    add_wall(s, -rw/2+0.17f, rh*0.4f, -rd/2+1.5f, 0.04f, rh*0.8f, 2.5f, cream);
    set_last_material(s, MAT_WALLPAPER);
    add_wall(s, -rw/2+0.17f, rh*0.4f, rd/2-1.5f, 0.04f, rh*0.8f, 2.5f, cream);
    set_last_material(s, MAT_WALLPAPER);

    // RIGHT WALL (X+) — service wall (bathroom door, wardrobe, desk)
    add_wall(s, rw/2, rh/2, 0, 0.3f, rh, rd, hull);
    set_last_material(s, MAT_CONCRETE);
    add_wall(s, rw/2-0.17f, rh*0.4f, 0, 0.04f, rh*0.8f, rd-1, cream);
    set_last_material(s, MAT_WALLPAPER);
    // Wainscoting on right wall
    add_wainscoting(s, rw/2-0.2f, 0, 0, rd-1.5f, 1.2f, true, cream, brass);

    // ============================================================
    // 4. FLOOR-TO-CEILING WINDOW — left wall, the Glass Elevator view
    //    Massive void panel, brass mullions, Earth glow pooling in
    // ============================================================

    // Main glass pane — 7m wide, nearly floor-to-ceiling
    float win_w = 7.0f, win_cz = -0.5f;
    add_wall(s, -rw/2+0.08f, rh/2, win_cz, 0.06f, rh-0.8f, win_w, void_black);
    set_last_material(s, MAT_GLASS);

    // Brass frame — top, bottom, left, right
    add_wall(s, -rw/2+0.1f, rh-0.3f, win_cz, 0.04f, 0.15f, win_w+0.3f, brass);
    set_last_material(s, MAT_BRASS);
    add_wall(s, -rw/2+0.1f, 0.3f, win_cz, 0.04f, 0.15f, win_w+0.3f, brass);
    set_last_material(s, MAT_BRASS);
    add_wall(s, -rw/2+0.1f, rh/2, win_cz-win_w/2-0.1f, 0.04f, rh-0.6f, 0.15f, brass);
    set_last_material(s, MAT_BRASS);
    add_wall(s, -rw/2+0.1f, rh/2, win_cz+win_w/2+0.1f, 0.04f, rh-0.6f, 0.15f, brass);
    set_last_material(s, MAT_BRASS);

    // Vertical mullions — 3 brass bars dividing the window
    for (int i = 1; i <= 3; i++) {
        float mz = win_cz - win_w/2 + i * (win_w/4.0f);
        add_wall(s, -rw/2+0.11f, rh/2, mz, 0.03f, rh-0.9f, 0.06f, brass);
        set_last_material(s, MAT_BRASS);
    }
    // Horizontal mullion — one at 1/3 height
    add_wall(s, -rw/2+0.12f, rh*0.33f, win_cz, 0.03f, 0.06f, win_w, brass);
    set_last_material(s, MAT_BRASS);

    // Window sill — brass ledge at bottom of glass
    add_wall(s, -rw/2+0.15f, 0.22f, win_cz, 0.12f, 0.04f, win_w+0.2f, brass);
    set_last_material(s, MAT_BRASS);

    // Earth glow on floor — the emotional anchor
    add_wall(s, -rw/2+3.5f, 0.02f, win_cz, 5, 0.02f, 5, earth_glow);
    set_last_decal(s);
    // Earth glow reflected on ceiling — subtle
    add_wall(s, -rw/2+2.5f, rh-0.1f, win_cz, 3.5f, 0.02f, 3.5f, (Color){45,100,180,30});
    set_last_decal(s);
    // Light shaft across floor — blue-white band from window
    add_wall(s, -rw/2+5, 0.02f, win_cz, 6, 0.02f, 3, (Color){60,130,200,80});
    set_last_decal(s);

    // Stars outside the window — scattered points of light
    for (int i = 0; i < 16; i++) {
        float sx = -rw/2 - 3 - (i*7)%12;
        float sy = 0.5f + (i*13)%5;
        float sz = win_cz - 4 + (i*11)%9;
        add_wall(s, sx, sy, sz, 0.08f, 0.08f, 0.08f,
                 (Color){240,238,232,(unsigned char)(100+(i*29)%120)});
    }

    // ============================================================
    // 5. PORTHOLE — right wall, circular window with deep brass ring
    // ============================================================
    add_sphere(s, rw/2-0.15f, rh*0.55f, -1.5f, 1.5f, void_black);
    add_cylinder(s, rw/2-0.13f, rh*0.55f, -1.5f, 1.7f, 0.08f, brass);
    // Inner ring — depth
    add_cylinder(s, rw/2-0.12f, rh*0.55f, -1.5f, 1.4f, 0.04f, dark_wood);

    // ============================================================
    // 6. CLERESTORY WINDOW — above bed, infinity when you lie down
    // ============================================================
    add_wall(s, 0, rh-0.5f, -rd/2+0.1f, 6, 0.8f, 0.06f, (Color){4,5,12,255});
    set_last_material(s, MAT_GLASS);
    add_wall(s, 0, rh-0.08f, -rd/2+0.12f, 6.2f, 0.06f, 0.04f, brass);
    set_last_material(s, MAT_BRASS);
    add_wall(s, 0, rh-0.92f, -rd/2+0.12f, 6.2f, 0.06f, 0.04f, brass);
    set_last_material(s, MAT_BRASS);

    // ============================================================
    // 7. DOORS — bathroom, adjoining room, entry
    // ============================================================

    // ENTRY DOOR — front wall, right of center
    add_door_frame(s, 3, 1.3f, rd/2-0.15f, 1.2f, 2.6f, 0.3f, brass);
    add_wall(s, 3, 1.3f, rd/2-0.18f, 1.1f, 2.5f, 0.06f, dark_wood);
    set_last_material(s, MAT_WOOD);
    // Room number plate — "2046" (a brass rectangle)
    add_wall(s, 3, 2.7f, rd/2-0.12f, 0.3f, 0.12f, 0.02f, brass);
    set_last_material(s, MAT_BRASS);

    // BATHROOM DOOR — right wall, toward front
    add_door_frame(s, rw/2-0.15f, 1.3f, 2.5f, 1.2f, 2.6f, 0.3f, brass);
    add_wall(s, rw/2-0.18f, 1.3f, 2.5f, 0.06f, 2.5f, 1.1f, white);
    set_last_material(s, MAT_WOOD);
    // Towel bar visible on bathroom door frame
    add_cylinder(s, rw/2-0.08f, 1.4f, 2.5f, 0.02f, 0.6f, brass);

    // ADJOINING DOOR — right wall, toward back (leads to darkness)
    add_door_frame(s, rw/2-0.15f, 1.3f, -3.5f, 1.2f, 2.6f, 0.3f, brass);
    add_wall(s, rw/2-0.18f, 1.3f, -3.5f, 0.06f, 2.5f, 1.1f, cream);
    set_last_material(s, MAT_WOOD);
    // Brass door handle — you can almost reach through
    add_sphere(s, rw/2-0.1f, 1.1f, -3.1f, 0.08f, brass);
    set_last_material(s, MAT_BRASS);
    // ── ADJOINING ROOM INTERIOR — visible through the crack ──
    // A dark mirror of your suite. The room she would have had.
    // Just enough geometry to read through a 6cm gap.
    {
        float ax = rw/2 + 3.5f;  // center of adjoining room (3.5m beyond wall)
        float az = -3.5f;
        Color adj_dark = {18, 16, 14, 255};
        Color adj_wall = {35, 32, 28, 255};

        // Back wall
        add_wall(s, ax, 1.5f, az-2.5f, 5, 3, 0.1f, adj_wall);
        // Far wall
        add_wall(s, ax+2.5f, 1.5f, az, 0.1f, 3, 5, adj_wall);
        // Floor — dark
        add_wall(s, ax, 0, az, 5, 0.05f, 5, adj_dark);
        // Ceiling
        add_wall(s, ax, 3, az, 5, 0.05f, 5, adj_dark);

        // Single pillow on a bed — one person, mirroring your suite
        // Bed frame
        add_wall(s, ax, 0.18f, az-1.0f, 2.0f, 0.3f, 1.4f, (Color){40,30,20,255});
        // Mattress
        add_wall(s, ax, 0.38f, az-1.0f, 1.8f, 0.2f, 1.2f, (Color){50,48,45,255});
        set_last_material(s, MAT_FABRIC);
        // ONE pillow — centered
        add_wall(s, ax, 0.52f, az-1.4f, 0.55f, 0.14f, 0.3f, (Color){200,195,185,255});
        set_last_material(s, MAT_FABRIC);

        // Light seam under the door — warm amber, someone was here recently
        add_wall(s, rw/2+0.05f, 0.02f, az, 0.04f, 0.04f, 1.0f, (Color){240,200,100,80});
        set_last_decal(s);
    }

    // ── BED MODEL — if available, place at bed zone origin ──
    {
        int bed_mdl = find_model_asset("bed");
        if (bed_mdl >= 0) {
            add_model(s, 0, 0, -4.5f, 1,1,1, 0, bed_mdl, MAT_FABRIC, (Color){255,255,255,255});
        }
    }

    // ============================================================
    // 8. BED ZONE — back-center, the emotional core
    // ============================================================

    // Bed frame — dark wood, generous
    add_wall(s, 0, 0.18f, -4.5f, 3.4f, 0.36f, 2.2f, dark_wood);
    set_last_material(s, MAT_WOOD);
    // Mattress
    add_wall(s, 0, 0.48f, -4.5f, 3.2f, 0.28f, 2.0f, white);
    set_last_material(s, MAT_FABRIC);

    // TWO PILLOWS — the room is for two
    add_wall(s, -0.6f, 0.68f, -5.2f, 0.65f, 0.18f, 0.4f, white);
    set_last_material(s, MAT_FABRIC);
    add_wall(s, 0.6f, 0.68f, -5.2f, 0.65f, 0.18f, 0.4f, white);
    set_last_material(s, MAT_FABRIC);

    // Duvet — slightly rumpled, cream with folded edge
    add_wall(s, 0, 0.66f, -4.2f, 3.0f, 0.06f, 1.4f, cream);
    set_last_material(s, MAT_FABRIC);
    // Folded edge at top — darker strip
    add_wall(s, 0, 0.68f, -4.8f, 3.0f, 0.04f, 0.3f, (Color){215,210,200,255});
    set_last_material(s, MAT_FABRIC);

    // HEADBOARD — tall navy velvet panel (Godard blue)
    add_wall(s, 0, 1.8f, -5.55f, 3.6f, 2.8f, 0.12f, navy);
    set_last_material(s, MAT_VELVET);
    // Brass cap on headboard
    add_wall(s, 0, 3.25f, -5.52f, 3.7f, 0.06f, 0.08f, brass);
    set_last_material(s, MAT_BRASS);

    // LEFT NIGHTSTAND — her side
    add_wall(s, -2.5f, 0.32f, -4.8f, 0.6f, 0.6f, 0.6f, wood);
    set_last_material(s, MAT_WOOD);
    // Drawer pull — brass line
    add_wall(s, -2.5f, 0.25f, -4.49f, 0.25f, 0.02f, 0.02f, brass);
    set_last_material(s, MAT_BRASS);
    // HER BOOK on left nightstand — dog-eared, blue cover
    add_wall(s, -2.6f, 0.64f, -4.9f, 0.25f, 0.04f, 0.18f, PAL_BLUE);
    set_last_material(s, MAT_LEATHER);

    // RIGHT NIGHTSTAND — his side
    add_wall(s, 2.5f, 0.32f, -4.8f, 0.6f, 0.6f, 0.6f, wood);
    set_last_material(s, MAT_WOOD);
    add_wall(s, 2.5f, 0.25f, -4.49f, 0.25f, 0.02f, 0.02f, brass);
    set_last_material(s, MAT_BRASS);
    // PHOTOGRAPH FACE-DOWN on right nightstand — you cannot see who's in it
    add_wall(s, 2.5f, 0.64f, -4.6f, 0.2f, 0.01f, 0.15f, (Color){240,238,230,255});
    set_last_decal(s);
    // Postcard — unread, unknowable
    add_wall(s, 2.3f, 0.65f, -4.9f, 0.16f, 0.005f, 0.11f, cream);
    set_last_decal(s);
    set_last_rotation(s, 8.0f);

    // BEDSIDE LAMPS — matching pair, warm pools
    // Left lamp
    add_cylinder(s, -2.5f, 0.64f, -4.8f, 0.08f, 0.04f, brass);
    add_cylinder(s, -2.5f, 0.76f, -4.8f, 0.03f, 0.2f, brass);
    add_cone(s, -2.5f, 0.92f, -4.8f, 0.2f, 0.16f, cream);
    add_light_panel(s, -2.5f, 0.87f, -4.8f, 0.25f, 0.4f, 0.25f, warm_light);
    // Right lamp
    add_cylinder(s, 2.5f, 0.64f, -4.8f, 0.08f, 0.04f, brass);
    add_cylinder(s, 2.5f, 0.76f, -4.8f, 0.03f, 0.2f, brass);
    add_cone(s, 2.5f, 0.92f, -4.8f, 0.2f, 0.16f, cream);
    add_light_panel(s, 2.5f, 0.87f, -4.8f, 0.25f, 0.4f, 0.25f, warm_light);

    // Towel swan on bed — the universal hotel welcome
    add_wall(s, 0, 0.66f, -4.0f, 0.22f, 0.16f, 0.14f, white);
    set_last_material(s, MAT_FABRIC);
    add_wall(s, 0, 0.80f, -3.9f, 0.07f, 0.14f, 0.05f, white);
    set_last_material(s, MAT_FABRIC);

    // Slippers by bed — still wrapped, placed neatly
    add_wall(s, -0.6f, 0.03f, -3.4f, 0.12f, 0.05f, 0.22f, cream);
    set_last_material(s, MAT_FABRIC);
    add_wall(s, -0.4f, 0.03f, -3.4f, 0.12f, 0.05f, 0.22f, cream);
    set_last_material(s, MAT_FABRIC);

    // Sock under bed — barely visible, navy
    add_wall(s, 1.2f, 0.02f, -3.8f, 0.08f, 0.02f, 0.15f, navy);
    set_last_material(s, MAT_FABRIC);
    set_last_decal(s);

    // Mint on left nightstand
    add_wall(s, -2.3f, 0.64f, -4.7f, 0.06f, 0.01f, 0.06f, (Color){120,180,100,255});

    // Water carafe + glass — clear, formal
    add_cylinder(s, -2.4f, 0.72f, -4.4f, 0.07f, 0.16f, glass_clr);
    set_last_material(s, MAT_GLASS);
    add_cylinder(s, -2.25f, 0.68f, -4.4f, 0.05f, 0.10f, (Color){200,210,220,120});
    set_last_material(s, MAT_GLASS);

    // Picture frames above bed — Earth photography, brass frames
    add_picture_frame(s, -1.5f, 3.8f, -rd/2+0.22f, 0.8f, 0.6f, brass, (Color){35,75,140,255});
    add_picture_frame(s, 0, 3.4f, -rd/2+0.22f, 0.5f, 0.5f, brass, (Color){60,90,50,255});
    add_picture_frame(s, 1.5f, 3.8f, -rd/2+0.22f, 0.8f, 0.6f, brass, (Color){60,130,200,255});

    // ============================================================
    // 9. LIVING ZONE — left-center, facing window (the view IS the TV)
    // ============================================================

    // SOFA — navy, facing window
    add_sofa(s, -3, 0, 2.0f, -90.0f, navy);

    // Throw pillow — Godard red against navy
    add_wall(s, -3.5f, 0.52f, 1.6f, 0.35f, 0.30f, 0.30f, PAL_RED);
    set_last_material(s, MAT_VELVET);
    // Second throw pillow — blue (her favorite color)
    add_wall(s, -2.4f, 0.48f, 1.7f, 0.30f, 0.28f, 0.28f, PAL_BLUE);
    set_last_material(s, MAT_VELVET);
    set_last_rotation(s, 12.0f);

    // Scarf draped over sofa arm — someone sat here
    add_wall(s, -4.1f, 0.72f, 2.0f, 0.08f, 0.45f, 0.35f, (Color){200,50,45,200});
    set_last_material(s, MAT_FABRIC);

    // COFFEE TABLE — brass and glass, low
    add_wall(s, -3, 0.35f, 3.5f, 1.4f, 0.04f, 0.8f, brass);
    set_last_material(s, MAT_BRASS);
    // Glass top
    add_wall(s, -3, 0.37f, 3.5f, 1.3f, 0.02f, 0.7f, (Color){210,215,220,80});
    set_last_material(s, MAT_GLASS);
    // 4 legs
    add_cylinder(s, -3.6f, 0.17f, 3.15f, 0.03f, 0.33f, brass);
    add_cylinder(s, -2.4f, 0.17f, 3.15f, 0.03f, 0.33f, brass);
    add_cylinder(s, -3.6f, 0.17f, 3.85f, 0.03f, 0.33f, brass);
    add_cylinder(s, -2.4f, 0.17f, 3.85f, 0.03f, 0.33f, brass);

    // Book on coffee table — blue spine (geometry textbook)
    add_wall(s, -3.2f, 0.39f, 3.5f, 0.35f, 0.04f, 0.22f, PAL_BLUE);
    set_last_material(s, MAT_LEATHER);

    // TWO CHAMPAGNE GLASSES on tray — one poured, one empty
    {
        int glasses_mdl = find_model_asset("champagne_glasses");
        if (glasses_mdl >= 0) {
            // Use 3D model — tray + both glasses + wine glass in one piece
            add_model(s, -2.7f, 0.39f, 3.3f, 1,1,1, 0, glasses_mdl, MAT_GLASS, glass_clr);
        } else {
            // Fallback: procedural geometry
            // Tray — brass oval
            add_wall(s, -2.7f, 0.39f, 3.3f, 0.5f, 0.02f, 0.3f, brass);
            set_last_material(s, MAT_BRASS);
            // Glass 1 — half full (golden liquid inside)
            add_cylinder(s, -2.8f, 0.44f, 3.3f, 0.03f, 0.14f, glass_clr);
            set_last_material(s, MAT_GLASS);
            add_wall(s, -2.8f, 0.44f, 3.3f, 0.04f, 0.07f, 0.04f, gold);
            // Glass 2 — empty, tilted
            add_cylinder(s, -2.6f, 0.44f, 3.35f, 0.03f, 0.14f, glass_clr);
            set_last_material(s, MAT_GLASS);
            set_last_rotation(s, 5.0f);
            // Wine glass with lipstick mark
            add_cylinder(s, -3.3f, 0.42f, 3.6f, 0.03f, 0.14f, (Color){210,210,215,160});
            add_wall(s, -3.3f, 0.51f, 3.6f, 0.06f, 0.06f, 0.06f, (Color){210,210,215,160});
            // Lipstick mark — red crescent on rim
            add_wall(s, -3.28f, 0.54f, 3.58f, 0.03f, 0.02f, 0.01f, (Color){180,45,55,220});
        }
    }

    // TWO CHAIRS — angled toward each other, for watching Earth together
    add_chair(s, -5.2f, 0, 0.5f, 45.0f, dark_wood, navy);
    add_chair(s, -5.2f, 0, 2.5f, -30.0f, dark_wood, navy);
    // Small side table between chairs
    add_wall(s, -5.5f, 0.45f, 1.5f, 0.4f, 0.04f, 0.4f, brass);
    set_last_material(s, MAT_BRASS);
    add_cylinder(s, -5.5f, 0.22f, 1.5f, 0.04f, 0.44f, brass);

    // ============================================================
    // 10. DESK ZONE — right wall, work/writing area
    // ============================================================

    // Desk — against right wall
    add_desk(s, rw/2-1.2f, 0, -1.5f, 90.0f, wood);

    // Desk lamp — wall-mounted above, brass arm
    add_wall(s, rw/2-0.2f, 1.6f, -1.5f, 0.06f, 0.04f, 0.5f, brass);
    set_last_material(s, MAT_BRASS);
    add_cone(s, rw/2-0.4f, 1.55f, -1.5f, 0.18f, 0.12f, cream);
    add_light_panel(s, rw/2-0.4f, 1.4f, -1.5f, 0.25f, 0.3f, 0.25f, warm_light);

    // HALF-WRITTEN LETTER on desk
    add_wall(s, rw/2-1.0f, S_DESK_H+0.02f, -1.5f, 0.3f, 0.005f, 0.22f, (Color){245,242,235,255});
    set_last_decal(s);
    // Geometry textbook open
    add_wall(s, rw/2-1.3f, S_DESK_H+0.02f, -1.2f, 0.4f, 0.04f, 0.3f, (Color){180,40,35,255});
    set_last_material(s, MAT_LEATHER);

    // Telephone on desk — rings unanswered
    {
        int phone_mdl = find_model_asset("telephone");
        if (phone_mdl >= 0) {
            add_model(s, rw/2-0.8f, S_DESK_H+0.01f, -1.8f, 1,1,1, -90, phone_mdl, MAT_BRASS, brass);
        } else {
            // Fallback: simple box phone
            add_wall(s, rw/2-0.8f, S_DESK_H+0.04f, -1.8f, 0.14f, 0.06f, 0.1f, brass);
            set_last_material(s, MAT_BRASS);
            // Handset
            add_wall(s, rw/2-0.8f, S_DESK_H+0.08f, -1.8f, 0.04f, 0.02f, 0.12f, (Color){35,30,25,255});
            set_last_material(s, MAT_LEATHER);
        }
    }

    // Desk chair
    add_chair(s, rw/2-2.0f, 0, -1.5f, -90.0f, dark_wood, (Color){60,55,50,255});

    // ============================================================
    // 11. ENTRY ZONE — front wall area
    // ============================================================

    // SUITCASE — half-packed, by the door
    add_wall(s, 4.5f, 0.15f, 4.5f, 0.7f, 0.3f, 0.45f, dark_wood);
    set_last_material(s, MAT_LEATHER);
    // Suitcase brass trim
    add_wall(s, 4.5f, 0.32f, 4.5f, 0.72f, 0.02f, 0.47f, brass);
    set_last_material(s, MAT_BRASS);
    set_last_decal(s);
    // Lid propped open
    add_wall(s, 4.5f, 0.34f, 4.72f, 0.68f, 0.16f, 0.02f, dark_wood);
    set_last_material(s, MAT_LEATHER);
    // Shirt spilling out — blue fabric
    add_wall(s, 4.8f, 0.28f, 4.7f, 0.25f, 0.04f, 0.35f, (Color){55,85,175,220});
    set_last_material(s, MAT_FABRIC);
    // Folded shirt inside
    add_wall(s, 4.3f, 0.08f, 4.4f, 0.35f, 0.08f, 0.25f, navy);
    set_last_material(s, MAT_FABRIC);

    // Luggage rack — brass frame near entry
    add_wall(s, 5.5f, 0.5f, 4.8f, 1.0f, 0.04f, 0.5f, brass);
    set_last_material(s, MAT_BRASS);
    add_cylinder(s, 5.0f, 0.25f, 4.6f, 0.03f, 0.5f, brass);
    add_cylinder(s, 6.0f, 0.25f, 4.6f, 0.03f, 0.5f, brass);
    add_cylinder(s, 5.0f, 0.25f, 5.0f, 0.03f, 0.5f, brass);
    add_cylinder(s, 6.0f, 0.25f, 5.0f, 0.03f, 0.5f, brass);

    // Shoes by door — placed neatly, his
    add_wall(s, 3.8f, 0.04f, 4.8f, 0.12f, 0.08f, 0.24f, (Color){35,30,25,255});
    set_last_material(s, MAT_LEATHER);
    set_last_rotation(s, -5.0f);
    add_wall(s, 4.05f, 0.04f, 4.7f, 0.12f, 0.08f, 0.24f, (Color){35,30,25,255});
    set_last_material(s, MAT_LEATHER);
    set_last_rotation(s, 8.0f);

    // Coat hooks — brass, by entry door
    add_wall(s, 2, 1.8f, rd/2-0.15f, 0.8f, 0.04f, 0.04f, brass);
    set_last_material(s, MAT_BRASS);
    add_sphere(s, 1.7f, 1.75f, rd/2-0.12f, 0.05f, brass);
    add_sphere(s, 2.0f, 1.75f, rd/2-0.12f, 0.05f, brass);
    add_sphere(s, 2.3f, 1.75f, rd/2-0.12f, 0.05f, brass);
    // ONE BATHROBE hung on coat hook
    add_wall(s, 2.0f, 1.2f, rd/2-0.18f, 0.5f, 1.0f, 0.08f, white);
    set_last_material(s, MAT_FABRIC);

    // Mirror near entry — brass-framed
    add_wall(s, 1, 1.6f, rd/2-0.14f, 0.6f, 0.9f, 0.02f, (Color){180,185,190,200});
    set_last_material(s, MAT_GLASS);
    // Mirror frame
    add_wall(s, 1, 2.1f, rd/2-0.13f, 0.7f, 0.04f, 0.03f, brass);
    set_last_material(s, MAT_BRASS);
    add_wall(s, 1, 1.1f, rd/2-0.13f, 0.7f, 0.04f, 0.03f, brass);
    set_last_material(s, MAT_BRASS);
    add_wall(s, 0.65f, 1.6f, rd/2-0.13f, 0.04f, 0.9f, 0.03f, brass);
    set_last_material(s, MAT_BRASS);
    add_wall(s, 1.35f, 1.6f, rd/2-0.13f, 0.04f, 0.9f, 0.03f, brass);
    set_last_material(s, MAT_BRASS);

    // Light switch panel by entry door
    add_wall(s, 3.6f, 1.2f, rd/2-0.14f, 0.12f, 0.16f, 0.02f, cream);
    // Light switch panel by bed
    add_wall(s, -1.9f, 0.7f, -rd/2+0.22f, 0.12f, 0.16f, 0.02f, cream);

    // ============================================================
    // 12. WARDROBE — right wall, slightly ajar
    // ============================================================
    add_wall(s, rw/2-0.5f, 1.2f, -0.2f, 0.6f, 2.4f, 0.8f, dark_wood);
    set_last_material(s, MAT_WOOD);
    // Door ajar — rotated panel
    add_wall(s, rw/2-0.82f, 1.2f, 0.05f, 0.04f, 2.3f, 0.75f, dark_wood);
    set_last_material(s, MAT_WOOD);
    set_last_rotation(s, 15.0f);
    // Brass handles on wardrobe
    add_sphere(s, rw/2-0.85f, 1.2f, 0.0f, 0.04f, brass);
    set_last_material(s, MAT_BRASS);

    // ============================================================
    // 13. MINI-BAR — small refrigerator, right wall near bathroom
    // ============================================================
    add_wall(s, rw/2-0.3f, 0.35f, 3.5f, 0.5f, 0.7f, 0.45f, charcoal);
    set_last_material(s, MAT_LEATHER);
    // Handle
    add_wall(s, rw/2-0.57f, 0.45f, 3.5f, 0.02f, 0.2f, 0.02f, brass);
    set_last_material(s, MAT_BRASS);

    // ============================================================
    // 14. BOOKSHELF — right wall
    // ============================================================
    add_bookshelf(s, rw/2-0.2f, 1.6f, 0.8f, 1.2f, 1.0f, 3, cream);

    // ============================================================
    // 15. INFRASTRUCTURE — pipes, vents, panels (the hotel IS a station)
    // ============================================================

    // Air vents — brass grilles near ceiling (4 locations)
    add_wall(s, -3, rh-0.2f, -rd/2+0.15f, 0.6f, 0.3f, 0.04f, brass);
    set_last_material(s, MAT_BRASS);
    add_wall(s, 4, rh-0.2f, -rd/2+0.15f, 0.6f, 0.3f, 0.04f, brass);
    set_last_material(s, MAT_BRASS);
    add_wall(s, -3, rh-0.2f, rd/2-0.15f, 0.6f, 0.3f, 0.04f, brass);
    set_last_material(s, MAT_BRASS);
    add_wall(s, 4, rh-0.2f, rd/2-0.15f, 0.6f, 0.3f, 0.04f, brass);
    set_last_material(s, MAT_BRASS);

    // Pipe conduits — running along ceiling-wall junction (right wall)
    add_cylinder(s, rw/2-0.2f, rh-0.15f, 0, 0.06f, rd*0.8f, hull_lt);
    add_cylinder(s, rw/2-0.2f, rh-0.3f, 0, 0.04f, rd*0.8f, hull_lt);
    // Pipe conduit — back wall
    add_cylinder(s, 0, rh-0.15f, -rd/2+0.15f, 0.05f, rw*0.6f, hull_lt);

    // Pipe bracket supports — every 3m
    for (int i = -1; i <= 1; i++) {
        add_wall(s, rw/2-0.18f, rh-0.22f, i*3.0f, 0.08f, 0.2f, 0.08f, hull);
    }

    // Fire sprinkler head — center ceiling
    add_cylinder(s, 0, rh-0.05f, 1, 0.04f, 0.06f, brass);
    add_sphere(s, 0, rh-0.1f, 1, 0.06f, (Color){200,60,50,255});

    // Smoke detector — near entry
    add_cylinder(s, 2, rh-0.05f, 3, 0.08f, 0.03f, white);

    // Thermostat on wall — 22 degrees (the compromise)
    add_wall(s, rw/2-0.14f, 1.4f, 1.0f, 0.04f, 0.12f, 0.08f, white);
    // Temperature display — small warm rectangle
    add_wall(s, rw/2-0.12f, 1.4f, 1.0f, 0.02f, 0.06f, 0.04f, (Color){200,220,180,200});

    // ============================================================
    // 16. RECESSED PANELS — depth articulation on walls
    // ============================================================

    // Back wall panels — flanking the headboard
    add_recessed_panel(s, -4, rh*0.6f, -rd/2+0.2f, 2.0f, 1.2f, 0.08f, hull);
    add_recessed_panel(s, 4, rh*0.6f, -rd/2+0.2f, 2.0f, 1.2f, 0.08f, hull);
    // Front wall panel — above entry
    add_recessed_panel(s, 0, rh*0.7f, rd/2-0.2f, 4, 1.2f, 0.08f, hull);
    // Right wall panels — between doors
    add_recessed_panel(s, rw/2-0.2f, rh*0.65f, -0.5f, 1.5f, 1.0f, 0.06f, hull);
    add_recessed_panel(s, rw/2-0.2f, rh*0.65f, 4.5f, 1.5f, 1.0f, 0.06f, hull);

    // Wall sconces — brass, flanking headboard
    add_cylinder(s, -2.8f, 2.2f, -rd/2+0.22f, 0.06f, 0.04f, brass);
    add_cone(s, -2.8f, 2.3f, -rd/2+0.22f, 0.12f, 0.1f, cream);
    add_light_panel(s, -2.8f, 2.3f, -rd/2+0.25f, 0.15f, 0.2f, 0.15f, warm_light);
    add_cylinder(s, 2.8f, 2.2f, -rd/2+0.22f, 0.06f, 0.04f, brass);
    add_cone(s, 2.8f, 2.3f, -rd/2+0.22f, 0.12f, 0.1f, cream);
    add_light_panel(s, 2.8f, 2.3f, -rd/2+0.25f, 0.15f, 0.2f, 0.15f, warm_light);

    // ============================================================
    // 17. BASEBOARDS AND CROWN MOLDING — every edge
    // ============================================================

    // Baseboards — all 4 walls (brass)
    add_baseboard(s, 0, 0, -rd/2+0.08f, rw, 0, brass);
    add_baseboard(s, 0, 0, rd/2-0.08f, rw, 0, brass);
    add_baseboard(s, -rw/2+0.08f, 0, 0, rd, 1.0f, brass);
    add_baseboard(s, rw/2-0.08f, 0, 0, rd, 1.0f, brass);

    // Crown moldings — brass
    add_crown_molding(s, 0, rh, -rd/2+0.08f, rw, 0, brass);
    add_crown_molding(s, 0, rh, rd/2-0.08f, rw, 0, brass);
    add_crown_molding(s, -rw/2+0.08f, rh, 0, rd, 1.0f, brass);
    add_crown_molding(s, rw/2-0.08f, rh, 0, rd, 1.0f, brass);

    // ============================================================
    // 18. CORNER REINFORCEMENT — prevent room escape + visual mass
    // ============================================================
    add_wall(s, -rw/2+0.1f, rh/2, -rd/2+0.1f, 0.5f, rh, 0.5f, hull);
    add_wall(s, rw/2-0.1f, rh/2, -rd/2+0.1f, 0.5f, rh, 0.5f, hull);
    add_wall(s, -rw/2+0.1f, rh/2, rd/2-0.1f, 0.5f, rh, 0.5f, hull);
    add_wall(s, rw/2-0.1f, rh/2, rd/2-0.1f, 0.5f, rh, 0.5f, hull);

    // ============================================================
    // 19. FLOATING OBJECTS — zero gravity storytelling
    // ============================================================

    // BATHROBE floating mid-room — white fabric, arms spread
    add_wall(s, 2, 2.6f, 1, 0.7f, 1.4f, 0.08f, white);
    set_last_material(s, MAT_FABRIC);
    // Sleeve — extended
    add_wall(s, 2.5f, 2.8f, 1, 0.5f, 0.15f, 0.08f, white);
    set_last_material(s, MAT_FABRIC);
    set_last_rotation(s, 20.0f);

    // PEN floating above desk — brass cylinder, drifting
    add_cylinder(s, rw/2-1.5f, 1.6f, -1.8f, 0.025f, 0.2f, brass);
    set_last_rotation(s, 25.0f);

    // CHAMPAGNE GLASS inverted near ceiling — the absurd detail
    add_cone(s, -4.5f, 4.2f, 1.5f, 0.1f, 0.12f, (Color){210,210,215,160});
    add_cylinder(s, -4.5f, 4.05f, 1.5f, 0.025f, 0.14f, (Color){210,210,215,160});
    // Gold droplets drifting
    add_sphere(s, -4.3f, 3.8f, 1.3f, 0.18f, gold);
    add_sphere(s, -4.6f, 3.5f, 1.7f, 0.14f, gold);
    add_sphere(s, -4.1f, 3.3f, 1.1f, 0.16f, gold);
    add_sphere(s, -4.8f, 4.0f, 1.9f, 0.10f, gold);

    // PHOTOGRAPH floating face-down — mid-room, slowly tumbling
    add_wall(s, 3, 2.5f, 0, 0.22f, 0.01f, 0.16f, (Color){240,238,230,255});
    set_last_rotation(s, 15.0f);

    // BOOK open, pages fanned — floating near sofa area
    add_wall(s, -2, 1.5f, 3, 0.6f, 0.04f, 0.4f, white);
    add_wall(s, -2.05f, 1.53f, 3, 0.55f, 0.02f, 0.38f, (Color){30,30,35,255});
    set_last_rotation(s, -8.0f);

    // ============================================================
    // 20. LEADING LINES — brass floor inlays
    // ============================================================

    // Brass strip from entry toward window
    add_wall(s, -2, 0.01f, 0, 0.06f, 0.02f, rd*0.6f, brass);
    set_last_material(s, MAT_BRASS);
    set_last_decal(s);
    // Second strip — perpendicular, toward bed
    add_wall(s, 0, 0.01f, -2, rw*0.4f, 0.02f, 0.06f, brass);
    set_last_material(s, MAT_BRASS);
    set_last_decal(s);

    // ============================================================
    // 21. INTERACTIVE OBJECTS
    // ============================================================
    add_object(s, -2.5f, 1.2f, -4.8f, "lamp", (Color){240,210,120,255}, 2);
    add_object(s, rw/2-1.2f, 1.0f, -1.5f, "desk", (Color){200,155,90,255}, 2);
    add_object(s, -3, 0.5f, 3.5f, "champagne", gold, 2);
    add_object(s, 0, 0.8f, -4.5f, "bed", white, 2);

    // ============================================================
    // 22. THE MOTIF: cigarette
    // On the coffee table, unlit. From the previous guest.
    // You could light it. You won't.
    // ============================================================
    add_cylinder(s, -2.8f, 0.39f, 3.2f, 0.1f, 0.45f, (Color){230,225,215,220});
    set_last_decal(s);

    tag_materials_by_color(s);

    s->spawn = (Vector3){0, 1.6f, 4};
    s->has_exit = false;
}

// ============================================================
// CLEANED SUITE — montage variant. Every two becomes one.
// The room after. Housekeeping has been. She was never here.
// ============================================================
void build_space_suite_cleaned(Scene *s) {
    // Start with the full suite
    build_space_suite(s);

    // Now strip the "twos" — walk the wall array and remove her presence.
    // The montage shows: what's left when you subtract a person from a room.

    // We rebuild the bed zone with ONE pillow, ONE side set up
    // And strip floating objects (zero-g storytelling is over — gravity resumed)
    // Strip her book, her throw pillow, her scarf, lipstick glass, second champagne glass

    // Strategy: mark specific walls as inactive by matching position+color.
    // This is fragile but the geometry is code-defined and won't change without us knowing.
    for (int i = 0; i < s->wall_count; i++) {
        Wall *w = &s->walls[i];
        if (!w->active) continue;

        // ── SECOND PILLOW (her side, left, x=-0.6) ──
        if (w->pos.z < -5.0f && w->pos.z > -5.4f &&
            w->pos.x < -0.3f && w->pos.x > -0.9f &&
            w->pos.y > 0.6f && w->pos.y < 0.75f &&
            w->size.x > 0.5f && w->size.y < 0.3f) {
            w->active = false;
            continue;
        }

        // ── HER BOOK on left nightstand (blue cover, x=-2.6) ──
        if (w->pos.x < -2.4f && w->pos.x > -2.8f &&
            w->pos.y > 0.6f && w->pos.y < 0.7f &&
            w->pos.z < -4.7f && w->pos.z > -5.1f &&
            w->color.b > 150) {
            w->active = false;
            continue;
        }

        // ── SECOND THROW PILLOW on sofa (blue, x=-2.4, her favorite color) ──
        if (w->pos.x > -2.6f && w->pos.x < -2.2f &&
            w->pos.y > 0.4f && w->pos.y < 0.55f &&
            w->pos.z > 1.5f && w->pos.z < 2.0f &&
            w->color.b > 150) {
            w->active = false;
            continue;
        }

        // ── SCARF draped over sofa arm (red, x=-4.1) ──
        if (w->pos.x < -3.9f && w->pos.x > -4.3f &&
            w->pos.y > 0.6f && w->pos.y < 0.8f &&
            w->pos.z > 1.8f && w->pos.z < 2.2f &&
            w->color.r > 180 && w->color.g < 80) {
            w->active = false;
            continue;
        }

        // ── SECOND CHAMPAGNE GLASS (empty, tilted, x=-2.6) ──
        if (w->pos.x > -2.7f && w->pos.x < -2.5f &&
            w->pos.y > 0.4f && w->pos.y < 0.5f &&
            w->pos.z > 3.2f && w->pos.z < 3.5f &&
            w->shape == SHAPE_CYLINDER) {
            w->active = false;
            continue;
        }

        // ── WINE GLASS with lipstick mark + the mark itself ──
        if (w->pos.x < -3.1f && w->pos.x > -3.5f &&
            w->pos.z > 3.4f && w->pos.z < 3.8f &&
            w->pos.y > 0.35f && w->pos.y < 0.6f) {
            w->active = false;
            continue;
        }

        // ── FLOATING OBJECTS — zero-g is over, gravity resumed ──
        // Bathrobe floating mid-room (y=2.6)
        if (w->pos.y > 2.0f && w->pos.y < 3.5f &&
            w->pos.x > 1.5f && w->pos.x < 3.0f &&
            w->pos.z > 0.5f && w->pos.z < 1.5f) {
            w->active = false;
            continue;
        }
        // Floating pen (y=1.6, near desk)
        if (w->pos.y > 1.4f && w->pos.y < 1.8f &&
            w->pos.x > 5.0f &&
            w->pos.z < -1.5f && w->pos.z > -2.1f &&
            w->shape == SHAPE_CYLINDER && w->size.x < 0.04f) {
            w->active = false;
            continue;
        }
        // Inverted champagne glass near ceiling (y>3.5)
        if (w->pos.y > 3.2f && w->pos.x < -3.8f &&
            (w->shape == SHAPE_CONE || w->shape == SHAPE_CYLINDER || w->shape == SHAPE_SPHERE)) {
            w->active = false;
            continue;
        }
        // Floating photograph (y=2.5)
        if (w->pos.y > 2.3f && w->pos.y < 2.7f &&
            w->pos.x > 2.5f && w->pos.x < 3.5f &&
            w->size.y < 0.02f) {
            w->active = false;
            continue;
        }
        // Floating book (y=1.5, near sofa)
        if (w->pos.y > 1.3f && w->pos.y < 1.7f &&
            w->pos.x < -1.5f && w->pos.x > -2.5f &&
            w->pos.z > 2.5f && w->pos.z < 3.5f &&
            w->size.x > 0.4f) {
            w->active = false;
            continue;
        }
    }

    // ── Replace the remaining pillow — centered, singular ──
    // Find the surviving right pillow (x=0.6) and center it
    for (int i = 0; i < s->wall_count; i++) {
        Wall *w = &s->walls[i];
        if (!w->active) continue;
        if (w->pos.z < -5.0f && w->pos.z > -5.4f &&
            w->pos.x > 0.3f && w->pos.x < 0.9f &&
            w->pos.y > 0.6f && w->pos.y < 0.75f &&
            w->size.x > 0.5f && w->size.y < 0.3f) {
            w->pos.x = 0.0f;  // centered — one person
            break;
        }
    }

    // ── ONE glass, washed, centered on tray ──
    // Find the surviving glass 1 (x=-2.8) and center it on the tray
    for (int i = 0; i < s->wall_count; i++) {
        Wall *w = &s->walls[i];
        if (!w->active) continue;
        if (w->pos.x > -2.9f && w->pos.x < -2.7f &&
            w->pos.y > 0.4f && w->pos.y < 0.5f &&
            w->pos.z > 3.2f && w->pos.z < 3.5f &&
            w->shape == SHAPE_CYLINDER) {
            w->pos.x = -2.7f;  // centered on tray
            break;
        }
    }

    // ── Remove all interactive objects — nothing left to do ──
    s->object_count = 0;

    // The room is clean. The room is empty. The room is for one.
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
    // Panel sits against wall — mark as decal
    if (along_z) {
        add_wall(s, x, y + height/2, z, S_WAINSCOT_D, height, length, panel);
        set_last_material(s, MAT_WOOD);
        set_last_decal(s);
        // Trim rails proud of panel — no z-fight (wider than panel)
        add_wall(s, x, y + height, z, S_WAINSCOT_D + 0.03f, S_WAINSCOT_RAIL, length, trim);
        set_last_material(s, MAT_WOOD);
        add_wall(s, x, y + height * 0.6f, z, S_WAINSCOT_D + 0.02f, S_WAINSCOT_RAIL, length, trim);
        set_last_material(s, MAT_WOOD);
    } else {
        add_wall(s, x, y + height/2, z, length, height, S_WAINSCOT_D, panel);
        set_last_material(s, MAT_WOOD);
        set_last_decal(s);
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
