// scene.c — Scene construction
// Material palette. Clean volumes. Dramatic light.
#include "scene.h"
#include "palette.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>

void add_wall(Scene *s, float x, float y, float z, float w, float h, float d, Color c) {
    if (s->wall_count >= MAX_WALLS) return;
    s->walls[s->wall_count++] = (Wall){
        .pos = {x, y, z}, .size = {w, h, d}, .color = c, .active = true,
        .shape = SHAPE_CUBE,
    };
}

void add_cylinder(Scene *s, float x, float y, float z, float diameter, float height, Color c) {
    if (s->wall_count >= MAX_WALLS) return;
    s->walls[s->wall_count++] = (Wall){
        .pos = {x, y, z}, .size = {diameter, height, diameter}, .color = c,
        .active = true, .shape = SHAPE_CYLINDER,
    };
}

void add_sphere(Scene *s, float x, float y, float z, float diameter, Color c) {
    if (s->wall_count >= MAX_WALLS) return;
    s->walls[s->wall_count++] = (Wall){
        .pos = {x, y, z}, .size = {diameter, diameter, diameter}, .color = c,
        .active = true, .shape = SHAPE_SPHERE,
    };
}

void add_cone(Scene *s, float x, float y, float z, float diameter, float height, Color c) {
    if (s->wall_count >= MAX_WALLS) return;
    s->walls[s->wall_count++] = (Wall){
        .pos = {x, y, z}, .size = {diameter, height, diameter}, .color = c,
        .active = true, .shape = SHAPE_CONE,
    };
}

void add_object(Scene *s, float x, float y, float z, const char *name, Color c, int max_steps) {
    if (s->object_count >= MAX_OBJECTS) return;
    s->objects[s->object_count++] = (InteractObject){
        .pos = {x, y, z}, .color = c, .name = name,
        .done = false, .active = true, .radius = 2.0f,
        .reward_timer = 0, .step = 0, .max_steps = max_steps,
    };
}

static void add_column(Scene *s, float x, float z, float r, float h, Color c) {
    add_cylinder(s, x, h/2, z, r*2, h, c);
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

// ============================================================
// SCENES
// ============================================================

void build_hotel_exterior(Scene *s) {
    memset(s, 0, sizeof(Scene));
    // BOUNDS: open exterior — no enclosure needed (player walks to entrance)
    s->surface = SURFACE_MARBLE;

    Color gold_facade = {195, 170, 120, 255};  // DOMINANT — warm sandstone facade
    Color dark_gold = {170, 140, 80, 255};
    Color gold = PAL_BRASS;
    Color window_lit = {240, 210, 130, 180};
    Color sidewalk = PAL_CONCRETE;
    Color road = {50, 50, 55, 255};
    Color godard_red = PAL_RED;

    s->fog_color = PAL_FOG_NIGHT;
    s->fog_density = 0.005f;

    // Sidewalk
    add_wall(s, 0, -0.1f, 0, 20, 0.2f, 8, sidewalk);
    // Road
    add_wall(s, 0, -0.15f, -8, 30, 0.15f, 12, road);

    // Hotel facade — 6 floors
    float bw = 16, bh = 22, bd = 0.5f;
    add_wall(s, 0, bh/2, 4, bw, bh, bd, gold_facade);

    // Windows — 5 across, 5 floors
    for (int floor = 0; floor < 5; floor++) {
        for (int col = 0; col < 5; col++) {
            float wx = -6 + col * 3;
            float wy = 3 + floor * 3.5f;
            // Window frame
            add_wall(s, wx, wy, 3.7f, 1.8f, 2.4f, 0.1f, dark_gold);
            // Glass — warm lit
            if (floor != 2 || col != 3) {  // one dark window
                add_wall(s, wx, wy, 3.65f, 1.5f, 2.0f, 0.05f, window_lit);
            }
        }
    }

    // Ground floor — entrance
    add_wall(s, 0, 1.8f, 3.5f, 4, 3.6f, 0.1f, (Color){250, 230, 170, 200});  // lit entrance
    // Awning
    add_wall(s, 0, 3.8f, 2.5f, 5, 0.1f, 2, godard_red);
    // Awning supports
    add_wall(s, -2.4f, 2.5f, 2, 0.06f, 2.5f, 0.06f, gold);
    add_wall(s, 2.4f, 2.5f, 2, 0.06f, 2.5f, 0.06f, gold);

    // Hotel name
    add_wall(s, 0, 4.2f, 3.68f, 6, 0.4f, 0.04f, gold);

    // Cornice/trim at each floor
    for (int f = 0; f < 6; f++) {
        float y = 1.2f + f * 3.5f;
        add_wall(s, 0, y, 3.72f, bw + 0.2f, 0.1f, 0.06f, gold);
    }

    // Lamppost left — bold, visible at 480x300
    add_wall(s, -6, 1.8f, -1, 0.15f, 3.6f, 0.15f, (Color){70, 65, 55, 255});
    add_light_panel(s, -6, 3.8f, -1, 0.6f, 0.6f, 0.6f, (Color){240, 210, 120, 200});
    // Lamppost right — bold
    add_wall(s, 6, 1.8f, -1, 0.15f, 3.6f, 0.15f, (Color){70, 65, 55, 255});
    add_light_panel(s, 6, 3.8f, -1, 0.6f, 0.6f, 0.6f, (Color){240, 210, 120, 200});

    // Sky — distant building silhouettes
    add_wall(s, -20, 8, 15, 8, 16, 2, (Color){20, 22, 38, 255});
    add_wall(s, 18, 6, 18, 6, 12, 2, (Color){18, 20, 35, 255});
    add_wall(s, -12, 5, 20, 10, 10, 2, (Color){22, 24, 40, 255});

    s->spawn = (Vector3){0, 1.6f, -3};
    s->exit_pos = (Vector3){0, 1.6f, 2.5f};
    s->has_exit = true;
}

void build_lobby(Scene *s) {
    memset(s, 0, sizeof(Scene));
    // BOUNDS: 30m x 20m, partially enclosed (front wall has gaps for entrance)
    s->surface = SURFACE_MARBLE;

    Color cream = PAL_CREAM;
    Color gold = PAL_BRASS;
    Color emerald = PAL_GREEN;
    Color concrete_a = PAL_MARBLE_A;
    Color concrete_b = PAL_MARBLE_B;
    Color marble_a = {60, 58, 55, 255};       // dark marble floor
    Color marble_b = {70, 68, 65, 255};
    Color plant = {60, 130, 65, 255};
    Color terracotta = PAL_TERRACOTTA;

    s->fog_color = PAL_FOG_GREEN;
    s->fog_density = 0.008f;

    int cols = 15, rows = 10;
    for (int r = 0; r < rows; r++)
        for (int c = 0; c < cols; c++) {
            float tx = -15 + c * 2 + 1, tz = -10 + r * 2 + 1;
            add_wall(s, tx, -0.05f, tz, 1.98f, 0.1f, 1.98f, ((r+c)%2==0) ? marble_a : marble_b);
        }

    add_wall(s, 0, 7, 0, 30, 0.2f, 20, cream);
    // Hanging sphere light fixtures
    add_sphere(s, -4, 6.4f, 0, 0.5f, PAL_LIGHT_WARM);
    add_sphere(s, -2, 6.4f, -2, 0.45f, PAL_LIGHT_WARM);
    add_sphere(s, 4, 6.4f, -3, 0.5f, PAL_LIGHT_WARM);
    add_sphere(s, 2, 6.4f, 1, 0.45f, PAL_LIGHT_WARM);

    add_wall(s, 0, 3.5f, -10, 30, 7, 0.3f, emerald);   // back wall — entirely deep green
    add_wall(s, -12, 3.5f, 10, 10, 7, 0.3f, cream);
    add_wall(s, 10, 3.5f, 10, 8, 7, 0.3f, cream);
    add_wall(s, -2, 5.5f, 10, 6, 3, 0.3f, cream);
    add_wall(s, -15, 3.5f, 0, 0.3f, 7, 20, cream);     // side walls cream
    add_wall(s, 15, 3.5f, -5, 0.3f, 7, 10, cream);
    add_wall(s, 15, 3.5f, 6, 0.3f, 7, 8, cream);
    // Green wainscoting on side walls — lower third
    add_wall(s, -14.8f, 1.0f, 0, 0.1f, 2.0f, 20, emerald);
    add_wall(s, 14.8f, 1.0f, 0, 0.1f, 2.0f, 20, emerald);

    add_wall(s, 0, 1.0f, -9.85f, 30, 0.06f, 0.1f, gold);
    add_wall(s, -14.85f, 1.0f, 0, 0.1f, 0.06f, 20, gold);
    add_wall(s, 14.85f, 1.0f, 0, 0.1f, 0.06f, 20, gold);

    // Concrete reveal panel — architectural, not pop art
    add_wall(s, 0, 3.5f, -9.8f, 8, 3.5f, 0.08f, concrete_a);
    add_wall(s, 0, 3.5f, -9.75f, 7.8f, 3.3f, 0.04f, concrete_b);

    add_column(s, -8, -4, 0.5f, 7, cream);
    add_column(s, -3, 1, 0.45f, 7, cream);
    add_column(s, 4, -5, 0.5f, 7, cream);
    add_column(s, 7, 2, 0.4f, 7, cream);

    add_wall(s, -11, 1.5f, -8, 2, 3, 0.4f, plant);
    add_wall(s, 10, 1, -8, 1.5f, 2, 0.4f, plant);

    add_wall(s, 0, 0.5f, -7, 6, 1.0f, 1.5f, gold);
    add_wall(s, 0, 1.02f, -7, 6.1f, 0.04f, 1.55f, (Color){230, 200, 110, 255});

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
    add_wall(s, -7, 0.3f, -1, 2.5f, 0.6f, 0.7f, (Color){165, 130, 85, 255});

    // Terracotta bench — ONE accent
    add_wall(s, 5, 0.25f, -8, 2.5f, 0.5f, 0.7f, terracotta);

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

    // Elevator doors are architectural set dressing — not interactable

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
    }

    // Brass banister railings along staircase sides
    for (int i = 0; i < num_steps; i++) {
        float sy = (i + 1) * step_h + 0.45f;  // railing height above step
        float sz = stair_base_z + i * step_d;
        // Left railing post
        add_wall(s, -stair_w / 2 - 0.05f, sy, sz, 0.06f, 0.9f, 0.06f, gold);
        // Right railing post
        add_wall(s, stair_w / 2 + 0.05f, sy, sz, 0.06f, 0.9f, 0.06f, gold);
    }
    // Railing top rail — angled, approximated with one long bar
    float rail_y_bot = step_h + 0.9f;
    float rail_y_top = num_steps * step_h + 0.9f;
    float rail_y_mid = (rail_y_bot + rail_y_top) / 2;
    float rail_z_mid = stair_base_z + (num_steps - 1) * step_d / 2;
    float rail_len = num_steps * step_d;
    add_wall(s, -stair_w / 2 - 0.05f, rail_y_mid, rail_z_mid, 0.04f, 0.04f, rail_len, gold);
    add_wall(s, stair_w / 2 + 0.05f, rail_y_mid, rail_z_mid, 0.04f, 0.04f, rail_len, gold);

    // MEZZANINE PLATFORM — 4m x 6m at the top of the stairs
    float mezz_y = num_steps * step_h;  // 2.4m
    float mezz_z = stair_base_z + num_steps * step_d + 3.0f;  // extends back from top step
    // Platform floor
    add_wall(s, 0, mezz_y - 0.05f, mezz_z - 3.0f + step_d, stair_w + 2, 0.1f, 6.0f, concrete_b);

    // Mezzanine railing — overlooking the lobby floor
    float railing_h = 0.9f;
    // Front railing (facing lobby)
    add_wall(s, 0, mezz_y + railing_h / 2, mezz_z - 6.0f + step_d, stair_w + 2, railing_h, 0.06f, gold);
    // Side railings
    add_wall(s, -(stair_w / 2 + 1), mezz_y + railing_h / 2, mezz_z - 3.0f + step_d,
             0.06f, railing_h, 6.0f, gold);
    add_wall(s, (stair_w / 2 + 1), mezz_y + railing_h / 2, mezz_z - 3.0f + step_d,
             0.06f, railing_h, 6.0f, gold);

    // Mezzanine light panel — warm glow above
    add_light_panel(s, 0, 6.8f, mezz_z - 2.0f, 3.0f, 0.05f, 2.0f, gold);

    s->spawn = (Vector3){0, 1.6f, 8};
    // Exit is now on the mezzanine — you go UP to leave the lobby
    s->exit_pos = (Vector3){0, mezz_y + 1.6f, mezz_z - 1.0f};
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
    Color carpet_b = {170, 62, 50, 255};
    Color warm_amber = PAL_GLOW_AMBER;
    Color ceiling_dark = PAL_CHARCOAL;

    s->fog_color = PAL_FOG_RED;
    s->fog_density = 0.010f;
    float L = 20, W = 4.5f, H = 4;

    int cols = 3, rows = (int)(L / 1.5f);
    for (int r = 0; r < rows; r++)
        for (int c = 0; c < cols; c++) {
            float tx = -W/2 + c*1.5f + 0.75f, tz = -r*1.5f - 0.75f;
            add_wall(s, tx, -0.05f, tz, 1.48f, 0.1f, 1.48f, ((r+c)%2==0) ? carpet_a : carpet_b);
        }

    add_wall(s, 0, H, -L/2, W, 0.2f, L, ceiling_dark);
    for (int i = 0; i < 4; i++)
        add_light_panel(s, 0, H-0.1f, -3 - i*5, 1.5f, 0.05f, 0.5f, warm_amber);

    add_wall(s, -W/2, H/2, -L/2, 0.3f, H, L, cream);
    add_wall(s, W/2, H/2, -L/2, 0.3f, H, L, cream);
    add_wall(s, 0, H/2, 0, W, H, 0.3f, cream);
    add_wall(s, 0, H/2, -L, W, H, 0.3f, cream);

    add_wall(s, -W/2+0.05f, 1.0f, -L/2, 0.08f, 0.04f, L, gold);
    add_wall(s, W/2-0.05f, 1.0f, -L/2, 0.08f, 0.04f, L, gold);

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

    Color wall_gold = PAL_GOLD;
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

    int cols = (int)(rw/0.8f), rows = (int)(rd/0.8f);
    for (int r = 0; r < rows; r++)
        for (int c = 0; c < cols; c++) {
            float tx = -rw/2+c*0.8f+0.4f, tz = -rd/2+r*0.8f+0.4f;
            add_wall(s, tx, -0.05f, tz, 0.78f, 0.1f, 0.78f, ((r+c)%2==0) ? wood : (Color){145,108,65,255});
        }

    add_wall(s, 0, rh, 0, rw, 0.2f, rd, cream);
    add_wall(s, 0, rh-0.08f, -rd/2+0.08f, rw, 0.12f, 0.06f, gold);
    add_wall(s, 0, rh-0.08f, rd/2-0.08f, rw, 0.12f, 0.06f, gold);
    add_wall(s, -rw/2+0.08f, rh-0.08f, 0, 0.06f, 0.12f, rd, gold);
    add_wall(s, rw/2-0.08f, rh-0.08f, 0, 0.06f, 0.12f, rd, gold);

    add_wall(s, 0, rh/2, -rd/2, rw, rh, 0.3f, wall_gold);
    add_wall(s, 0, rh/2, rd/2, rw, rh, 0.3f, wall_gold);
    add_wall(s, -rw/2, rh/2, 0, 0.3f, rh, rd, wall_gold);
    add_wall(s, rw/2, rh/2, 0, 0.3f, rh, rd, wall_gold);

    add_wall(s, 0, 0.45f, -rd/2+0.08f, rw, 0.9f, 0.06f, dark_wood);
    add_wall(s, 0, 0.45f, rd/2-0.08f, rw, 0.9f, 0.06f, dark_wood);
    add_wall(s, -rw/2+0.08f, 0.45f, 0, 0.06f, 0.9f, rd, dark_wood);
    add_wall(s, rw/2-0.08f, 0.45f, 0, 0.06f, 0.9f, rd, dark_wood);

    // Bed
    add_wall(s, 0, 0.2f, -3.5f, 3.4f, 0.4f, 2.0f, dark_wood);
    add_wall(s, 0, 0.5f, -3.5f, 3.2f, 0.25f, 1.8f, white);
    add_wall(s, -0.6f, 0.68f, -4.2f, 0.7f, 0.2f, 0.4f, white);
    add_wall(s, 0.6f, 0.68f, -4.2f, 0.7f, 0.2f, 0.4f, white);
    add_wall(s, 0, 1.2f, -4.5f, 3.6f, 1.6f, 0.12f, godard_red);
    add_wall(s, 0, 2.05f, -4.48f, 3.7f, 0.05f, 0.06f, gold);

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
    add_wall(s, 5.0f, 0.82f, 0, 2.6f, 0.03f, 0.95f, gold);
    // Desk chair — seat with 4 cylinder legs + back
    Color chair_c = {160,90,50,255};
    add_wall(s, 3.8f, 0.35f, 0, 0.5f, 0.04f, 0.5f, chair_c);          // seat (thin)
    add_cylinder(s, 3.57f, 0.165f, -0.22f, 0.025f, 0.3f, chair_c);     // leg FL
    add_cylinder(s, 4.03f, 0.165f, -0.22f, 0.025f, 0.3f, chair_c);     // leg FR
    add_cylinder(s, 3.57f, 0.165f, 0.22f, 0.025f, 0.3f, chair_c);      // leg BL
    add_cylinder(s, 4.03f, 0.165f, 0.22f, 0.025f, 0.3f, chair_c);      // leg BR
    add_wall(s, 3.8f, 0.75f, -0.22f, 0.5f, 0.55f, 0.08f, chair_c);    // back (angled via offset)
    add_wall(s, 5.75f, 1.8f, 0, 0.04f, 1.1f, 1.3f, (Color){200,205,210,180});
    add_wall(s, 5.78f, 1.8f, 0, 0.03f, 1.2f, 0.05f, gold);

    // Sofa — warm gray
    add_wall(s, -4.2f, 0.25f, 1.5f, 2.4f, 0.5f, 0.9f, warm_gray);
    add_wall(s, -4.2f, 0.6f, 1.95f, 2.4f, 0.6f, 0.15f, warm_gray);
    add_wall(s, -5.4f, 0.4f, 1.5f, 0.08f, 0.45f, 0.9f, warm_gray);
    add_wall(s, -3.0f, 0.4f, 1.5f, 0.08f, 0.45f, 0.9f, warm_gray);

    // Coffee table — flat top with 4 cylinder legs
    add_wall(s, -4.2f, 0.38f, 3.0f, 1.45f, 0.03f, 0.85f, gold);       // top surface
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
    add_wall(s, -5.75f, 2.95f, -1, 0.06f, 0.12f, 1.85f, gold);
    add_wall(s, -5.75f, 0.05f, -1, 0.06f, 0.08f, 1.85f, gold);
    add_wall(s, -5.75f, 1.5f, -1.88f, 0.06f, 2.85f, 0.06f, gold);
    add_wall(s, -5.75f, 1.5f, -0.12f, 0.06f, 2.85f, 0.06f, gold);
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

    // LIGHT SHAFT — sunlight/moonlight through balcony door onto floor
    add_wall(s, -4.5f, 0.02f, -1.0f, 2.0f, 0.02f, 1.5f, (Color){240,235,225,120});

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
    add_wall(s, -5.5f, 1.2f, 3.18f, 0.76f, 2.3f, 0.02f, (Color){65,48,28,255}); // front panel (darker)
    add_wall(s, -5.5f, 2.0f, 3.18f, 0.04f, 0.08f, 0.04f, gold);       // handle left
    add_wall(s, -5.5f, 1.6f, 3.18f, 0.04f, 0.08f, 0.04f, gold);       // handle right
    add_object(s, -5.5f, 1.2f, 3.2f, "wardrobe", (Color){140,120,80,255}, 1);

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
    // BOUNDS: 5m x 3m, open-air (railing + back wall, no ceiling)
    s->surface = SURFACE_MARBLE;

    s->fog_color = PAL_FOG_NIGHT;
    s->fog_density = 0.003f;

    Color stone = PAL_MARBLE_A;
    Color railing = PAL_MID;
    Color gold = PAL_BRASS;
    Color bldg_near = {60, 65, 85, 255};
    Color bldg_mid = {45, 50, 72, 255};
    Color bldg_far = {35, 40, 62, 255};
    Color window = {240, 200, 110, 140};
    Color tower = {70, 75, 95, 255};

    add_wall(s, 0, -0.1f, 0, 5, 0.2f, 3, stone);
    add_wall(s, 0, 2, 1.5f, 5, 4, 0.3f, (Color){200,175,120,255});
    add_wall(s, 0, 2.8f, 1.35f, 1.8f, 0.12f, 0.06f, gold);
    add_wall(s, -0.95f, 1.5f, 1.35f, 0.06f, 2.8f, 0.06f, gold);
    add_wall(s, 0.95f, 1.5f, 1.35f, 0.06f, 2.8f, 0.06f, gold);

    // Railing — top rail and mid rail only, clean at 480x300 (Rodkin)
    add_wall(s, 0, 0.95f, -1.5f, 5, 0.06f, 0.08f, railing);
    add_wall(s, 0, 0.5f, -1.5f, 5, 0.04f, 0.06f, railing);
    add_wall(s, -2.5f, 0.95f, 0, 0.08f, 0.06f, 3, railing);
    add_wall(s, 2.5f, 0.95f, 0, 0.08f, 0.06f, 3, railing);

    // Wine + chair
    add_wall(s, -1.5f, 0.45f, -0.5f, 0.45f, 0.04f, 0.45f, (Color){120,100,70,255});
    add_wall(s, -1.5f, 0.22f, -0.5f, 0.05f, 0.45f, 0.05f, (Color){100,85,60,255});
    add_wall(s, -1.5f, 0.49f, -0.5f, 0.05f, 0.12f, 0.05f, (Color){210,210,215,160});
    add_wall(s, -1.5f, 0.52f, -0.5f, 0.035f, 0.05f, 0.035f, (Color){140,35,45,200});
    add_wall(s, -1.5f, 0.22f, 0.2f, 0.45f, 0.44f, 0.45f, (Color){120,100,70,255});
    add_wall(s, -1.5f, 0.58f, 0.42f, 0.45f, 0.4f, 0.06f, (Color){120,100,70,255});

    // Second wine glass — further offset so both glasses are clearly a pair
    add_wall(s, -1.0f, 0.49f, -0.8f, 0.05f, 0.12f, 0.05f, (Color){210,210,215,160}); // glass stem
    add_wall(s, -1.0f, 0.52f, -0.8f, 0.035f, 0.05f, 0.035f, (Color){140,35,45,200}); // dark liquid

    // Ashtray with cigarette on railing table
    add_cylinder(s, -1.5f, 0.49f, -0.3f, 0.12f, 0.04f, (Color){140,135,130,255});
    add_object(s, -1.5f, 0.55f, -0.3f, "cigarette", (Color){200,195,185,255}, 1);

    add_wall(s, -12, 4, -30, 4, 8, 2, bldg_near);
    add_wall(s, -8, 5.5f, -28, 3, 11, 2, bldg_near);
    add_wall(s, -16, 3, -32, 5, 6, 2, bldg_near);
    add_wall(s, 16, 4.5f, -25, 3.5f, 9, 2, bldg_near);
    add_wall(s, 5, 6, -50, 6, 12, 3, bldg_mid);
    add_wall(s, -5, 5, -55, 8, 10, 3, bldg_mid);
    add_wall(s, 15, 4, -45, 5, 8, 3, bldg_mid);
    add_wall(s, -20, 5.5f, -48, 4, 11, 3, bldg_mid);
    add_wall(s, 0, 3, -80, 25, 6, 4, bldg_far);
    add_wall(s, -25, 4, -85, 15, 8, 4, bldg_far);
    add_wall(s, 22, 3.5f, -82, 12, 7, 4, bldg_far);

    for (int r = 0; r < 3; r++)
        for (int c = 0; c < 2; c++)
            if ((r+c)%2==0)
                add_wall(s, -12.5f+c*1.5f, 1.5f+r*2.0f, -28.9f, 0.5f, 0.8f, 0.04f, window);
    for (int r = 0; r < 4; r++)
        for (int c = 0; c < 2; c++)
            if ((r+c)%3!=2)
                add_wall(s, -8.5f+c*1.2f, 1.5f+r*2.0f, -26.9f, 0.4f, 0.7f, 0.04f, window);
    for (int r = 0; r < 4; r++)
        for (int c = 0; c < 3; c++)
            if ((r*3+c)%2==0)
                add_wall(s, 3.5f+c*1.5f, 1.5f+r*2.5f, -48.5f, 0.45f, 0.7f, 0.04f, window);

    float tx = 8, tz = -90;
    add_wall(s, tx, 10, tz, 0.7f, 20, 0.7f, tower);
    add_wall(s, tx-2, 2, tz, 0.9f, 4, 0.9f, tower);
    add_wall(s, tx+2, 2, tz, 0.9f, 4, 0.9f, tower);
    add_wall(s, tx, 4.5f, tz, 4.5f, 0.35f, 0.5f, tower);
    add_wall(s, tx, 9, tz, 2.8f, 0.25f, 0.4f, tower);
    add_wall(s, tx, 14, tz, 1.3f, 0.25f, 0.35f, tower);
    add_light_panel(s, tx, 4.5f, tz-0.3f, 4.6f, 0.15f, 0.04f, (Color){240,210,130,100});
    add_light_panel(s, tx, 20.2f, tz, 0.25f, 0.4f, 0.25f, (Color){255,245,200,180});

    for (int i = 0; i < 20; i++) {
        float sx = -45+(i*41)%90, sy = 22+(i*17)%18, sz = -95-(i*13)%25;
        add_wall(s, sx, sy, sz, 0.12f, 0.12f, 0.12f,
                 (Color){240,235,225,(unsigned char)(120+(i*19)%80)});
    }

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
    // Drain — small flat dark cylinder on floor
    add_cylinder(s, 1.0f, 0.01f, 0.5f, 0.15f, 0.02f, (Color){60,58,55,255});

    // Ceiling — white plaster
    add_wall(s, 0, bh, 0, bw, 0.2f, bd, ceiling);

    // Walls — concrete
    add_wall(s, 0, bh/2, -bd/2, bw, bh, 0.2f, concrete);       // back wall
    add_wall(s, 0, bh/2, bd/2, bw, bh, 0.2f, concrete);        // front wall (entrance)
    add_wall(s, -bw/2, bh/2, 0, 0.2f, bh, bd, concrete);       // left wall
    add_wall(s, bw/2, bh/2, 0, 0.2f, bh, bd, concrete);        // right wall

    // Bathtub — large freestanding, with curved half-cylinder ends and brass faucet
    add_wall(s, -1.2f, 0.25f, -1.0f, 1.8f, 0.5f, 0.9f, porcelain);  // tub body
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
    add_cylinder(s, -1.2f, 0.85f, -1.38f, 0.04f, 0.06f, brass);      // faucet spout

    // Sink — wall-mounted slab on right wall
    add_wall(s, 2.2f, 0.85f, 0.5f, 0.8f, 0.06f, 0.5f, porcelain);   // sink basin
    add_wall(s, 2.35f, 0.85f, 0.5f, 0.06f, 0.5f, 0.5f, porcelain);  // wall bracket
    // Brass tap knobs — sphere shapes
    add_sphere(s, 2.0f, 0.95f, 0.5f, 0.08f, brass);
    add_sphere(s, 1.9f, 0.95f, 0.5f, 0.08f, brass);

    // Mirror — large rectangle above sink
    add_wall(s, 2.38f, 1.7f, 0.5f, 0.04f, 1.0f, 0.8f, mirror);

    // Toilet — simple block
    add_wall(s, 2.0f, 0.25f, -1.2f, 0.5f, 0.5f, 0.4f, porcelain);   // base
    add_wall(s, 2.0f, 0.55f, -1.35f, 0.5f, 0.6f, 0.08f, porcelain);  // tank

    // Terracotta towel — ONE accent, hung on left wall
    add_wall(s, -2.38f, 1.4f, 0.0f, 0.06f, 0.9f, 0.5f, terracotta);
    // Towel bar
    add_wall(s, -2.4f, 1.9f, 0.0f, 0.04f, 0.04f, 0.7f, brass);

    // Toiletries bag on counter near sink
    add_wall(s, 1.8f, 0.92f, 0.8f, 0.25f, 0.15f, 0.12f, (Color){60,55,50,255});
    add_wall(s, 1.8f, 1.0f, 0.8f, 0.25f, 0.02f, 0.01f, (Color){178,155,107,255}); // brass zipper

    // Ando moment — horizontal slot window near ceiling on back wall
    // Narrow opening letting light pour in
    add_light_panel(s, 0, bh-0.4f, -bd/2+0.12f, 3.5f, 0.3f, 0.06f,
                    (Color){245,242,235,200});
    // Light shaft on floor from the slot window
    add_wall(s, 0, 0.02f, -0.5f, 3.0f, 0.02f, 1.5f, (Color){240,238,230,100});

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

    // Walls — tall concrete
    add_wall(s, 0, sh/2, -sd/2, sw, sh, 0.3f, concrete);       // back
    add_wall(s, 0, sh/2, sd/2, sw, sh, 0.3f, concrete);        // front
    add_wall(s, -sw/2, sh/2, 0, 0.3f, sh, sd, concrete);       // left
    add_wall(s, sw/2, sh/2, 0, 0.3f, sh, sd, concrete);        // right

    // Ceiling
    add_wall(s, 0, sh, 0, sw, 0.2f, sd, (Color){195,192,188,255});

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
    add_cylinder(s, -1.2f, 3.0f, 2.5f, 0.06f, 1.5f, metal);
    add_cylinder(s, 1.2f, 5.0f, -0.5f, 0.06f, 1.5f, metal);
    add_cylinder(s, -1.2f, 7.0f, 2.5f, 0.06f, 1.5f, metal);

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

    // Parapet walls — 0.8m high around edges
    add_wall(s, 0, 0.4f, -10, 20, 0.8f, 0.3f, concrete);   // back
    add_wall(s, 0, 0.4f, 10, 20, 0.8f, 0.3f, concrete);    // front
    add_wall(s, -10, 0.4f, 0, 0.3f, 0.8f, 20, concrete);   // left
    add_wall(s, 10, 0.4f, 0, 0.3f, 0.8f, 20, concrete);    // right

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
            if ((r+c)%2==0)
                add_wall(s, -8.5f+c*1.2f, -0.5f+r*2.0f, -26.9f, 0.4f, 0.7f, 0.04f, window);
    for (int r = 0; r < 4; r++)
        for (int c = 0; c < 3; c++)
            if ((r*3+c)%2==0)
                add_wall(s, 3.5f+c*1.5f, -0.5f+r*2.5f, -48.5f, 0.45f, 0.7f, 0.04f, window);

    // Eiffel Tower — from higher vantage
    float tx = 8, tz = -90;
    add_wall(s, tx, 8, tz, 0.7f, 20, 0.7f, tower);
    add_wall(s, tx-2, 0, tz, 0.9f, 4, 0.9f, tower);
    add_wall(s, tx+2, 0, tz, 0.9f, 4, 0.9f, tower);
    add_wall(s, tx, 2.5f, tz, 4.5f, 0.35f, 0.5f, tower);
    add_wall(s, tx, 7, tz, 2.8f, 0.25f, 0.4f, tower);
    add_wall(s, tx, 12, tz, 1.3f, 0.25f, 0.35f, tower);
    add_light_panel(s, tx, 2.5f, tz-0.3f, 4.6f, 0.15f, 0.04f, (Color){240,210,130,100});
    add_light_panel(s, tx, 18.2f, tz, 0.25f, 0.4f, 0.25f, (Color){255,245,200,180});

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

    // Ceiling
    add_wall(s, 0, eh, 0, ew, 0.1f, ed, brass);

    // Ceiling light panel — single warm panel
    add_light_panel(s, 0, eh - 0.08f, 0, 1.0f, 0.04f, 1.0f, warm_panel);

    // Walls — brass colored
    add_wall(s, 0, eh/2, -ed/2, ew, eh, 0.08f, brass);           // back wall
    add_wall(s, 0, eh/2, ed/2, ew, eh, 0.08f, brass);            // front (doors, closed)
    add_wall(s, -ew/2, eh/2, 0, 0.08f, eh, ed, brass);           // left wall
    add_wall(s, ew/2, eh/2, 0, 0.08f, eh, ed, brass);            // right wall

    // Mirror on back wall
    add_wall(s, 0, 1.4f, -ed/2 + 0.06f, 1.2f, 1.4f, 0.03f, mirror);

    // Button panel on right wall — 4 small brass rectangles stacked
    float panel_x = ew/2 - 0.06f;
    float panel_z = 0.3f;
    // Panel backing
    add_wall(s, panel_x, 1.2f, panel_z, 0.04f, 0.7f, 0.2f, (Color){150, 135, 100, 255});
    // Buttons: 4 floors, floor 2 is lit
    for (int i = 0; i < 4; i++) {
        float by = 0.95f + i * 0.15f;
        Color bc = (i == 1) ? btn_lit : btn_dark;  // floor 2 lit
        add_wall(s, panel_x - 0.01f, by, panel_z, 0.03f, 0.08f, 0.08f, bc);
    }

    // Brass door seam on front wall
    add_wall(s, 0, eh/2, ed/2 - 0.02f, 0.04f, eh, 0.02f, (Color){30, 28, 25, 255});

    s->spawn = (Vector3){0, 1.6f, 0};
    s->has_exit = false;
}

void build_taxi_ride(Scene *s) {
    memset(s, 0, sizeof(Scene));
    // BOUNDS: taxi interior — no player movement, mouse-look only
    s->surface = SURFACE_MARBLE;

    // Michael Mann night palette
    Color fog_teal = {12, 18, 28, 255};
    Color dash_dark = {25, 22, 18, 255};
    Color leather = {45, 35, 25, 255};
    Color driver_coat = {20, 18, 15, 255};
    Color driver_head = {150, 130, 100, 255};
    Color window_frame = {20, 18, 15, 255};
    Color meter_bg = {30, 35, 28, 255};
    Color meter_glow = {80, 200, 100, 180};
    Color mirror_c = {60, 65, 70, 180};

    // City palette
    Color road = {25, 28, 32, 255};
    Color bldg_a = {18, 22, 30, 255};
    Color bldg_b = {22, 26, 35, 255};
    Color bldg_c = {15, 18, 25, 255};
    Color window_amber = {230, 180, 80, 100};
    Color streetlight_glow = {240, 190, 90, 180};
    Color streetlight_pole = {50, 48, 45, 255};
    Color road_marking = {180, 175, 165, 80};
    Color wet_reflect = {240, 190, 90, 30};

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

    // Driver — simple figure in front-left seat
    // Body (dark coat)
    add_wall(s, -0.4f, 0.75f, -0.9f, 0.4f, 0.5f, 0.3f, driver_coat);
    // Head
    add_wall(s, -0.4f, 1.12f, -0.85f, 0.18f, 0.2f, 0.18f, driver_head);
    // Shoulders
    add_wall(s, -0.4f, 0.95f, -0.9f, 0.5f, 0.1f, 0.3f, driver_coat);

    // Rear-view mirror — small rectangle above dashboard center
    add_wall(s, 0, 1.15f, -1.1f, 0.2f, 0.1f, 0.04f, mirror_c);
    // Mirror support
    add_wall(s, 0, 1.2f, -1.05f, 0.02f, 0.06f, 0.08f, (Color){40, 38, 35, 255});

    // Backseat — player sits here
    add_wall(s, 0, 0.35f, 0.1f, 1.4f, 0.12f, 0.5f, leather);
    // Seat back
    add_wall(s, 0, 0.65f, 0.35f, 1.4f, 0.5f, 0.1f, leather);

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
    add_wall(s, 0, 12, -230, 4, 24, 3, (Color){12, 16, 24, 255});
    add_wall(s, 12, 6, -215, 8, 12, 3, (Color){8, 12, 20, 255});
    add_wall(s, -8, 10, -225, 5, 20, 3, (Color){11, 15, 23, 255});
    add_wall(s, 20, 7, -220, 5, 14, 3, (Color){9, 13, 21, 255});

    s->spawn = (Vector3){0, 1.0f, 0};
    s->has_exit = false;
}
