// scene.c — Scene construction — entirely in code, no files
#include "scene.h"
#include <math.h>
#include <string.h>

void add_wall(Scene *s, float x, float y, float z, float w, float h, float d, Color c) {
    if (s->wall_count >= MAX_WALLS) return;
    s->walls[s->wall_count++] = (Wall){
        .pos = {x, y, z},
        .size = {w, h, d},
        .color = c,
        .active = true,
    };
}

void add_object(Scene *s, float x, float y, float z, const char *name,
                const char *prompt, const char *done_text, Color c) {
    if (s->object_count >= MAX_OBJECTS) return;
    s->objects[s->object_count++] = (InteractObject){
        .pos = {x, y, z},
        .color = c,
        .name = name,
        .prompt = prompt,
        .done_text = done_text,
        .done = false,
        .active = true,
        .radius = 2.0f,
    };
}

// Build a box room from walls
static void build_room(Scene *s, float cx, float cz, float w, float d, float h,
                       Color wall_c, Color floor_c, Color ceil_c) {
    float hw = w / 2, hd = d / 2;
    s->floor_color = floor_c;
    s->ceiling_color = ceil_c;
    add_wall(s, cx, -0.1f, cz, w, 0.2f, d, floor_c);
    add_wall(s, cx, h, cz, w, 0.2f, d, ceil_c);
    add_wall(s, cx, h/2, cz - hd, w, h, 0.3f, wall_c);
    add_wall(s, cx, h/2, cz + hd, w, h, 0.3f, wall_c);
    add_wall(s, cx + hw, h/2, cz, 0.3f, h, d, wall_c);
    add_wall(s, cx - hw, h/2, cz, 0.3f, h, d, wall_c);
}

static void add_pillar(Scene *s, float x, float z, float radius, float height, Color c) {
    add_wall(s, x, height/2, z, radius*2, height, radius*2, c);
}

static void add_wall_with_door(Scene *s, float x, float y, float z,
                               float total_w, float h, float d,
                               float door_offset, float door_w, Color c) {
    float left_w = door_offset - door_w/2;
    float right_w = total_w - door_offset - door_w/2;
    if (left_w > 0.1f) {
        add_wall(s, x - total_w/2 + left_w/2, y, z, left_w, h, d, c);
    }
    if (right_w > 0.1f) {
        add_wall(s, x + total_w/2 - right_w/2, y, z, right_w, h, d, c);
    }
    add_wall(s, x - total_w/2 + door_offset, y + h*0.35f, z, door_w, h*0.3f, d, c);
}

static Color blake_lerp(Color a, Color b, float t) {
    return (Color){
        (unsigned char)(a.r + (b.r - a.r) * t),
        (unsigned char)(a.g + (b.g - a.g) * t),
        (unsigned char)(a.b + (b.b - a.b) * t),
        (unsigned char)(a.a + (b.a - a.a) * t),
    };
}

static void add_column(Scene *s, float x, float z, float r, float h, Color c) {
    for (int i = 0; i < 8; i++) {
        float angle = i * 3.14159f / 4.0f;
        float wx = x + cosf(angle) * r;
        float wz = z + sinf(angle) * r;
        float nx = cosf(angle);
        float nz = sinf(angle);
        float seg_len = r * 0.8f;
        add_wall(s, wx, h/2, wz,
                 fabsf(nz) * seg_len + 0.1f, h,
                 fabsf(nx) * seg_len + 0.1f, c);
    }
}

static void add_gradient_wall(Scene *s, float x, float z, float w, float d,
                              float h, int slices, Color bottom, Color top) {
    float slice_h = h / slices;
    for (int i = 0; i < slices; i++) {
        float t = (float)i / (slices - 1);
        Color c = blake_lerp(bottom, top, t);
        add_wall(s, x, slice_h * i + slice_h/2, z, w, slice_h + 0.02f, d, c);
    }
}

static void add_light_panel(Scene *s, float x, float y, float z,
                            float w, float h, float d, Color c) {
    Color glow = {c.r, c.g, c.b, 120};
    add_wall(s, x, y, z, w, h, d, glow);
    add_wall(s, x, y, z + (d > 0.2f ? 0 : 0.05f), w + 0.3f, h + 0.3f, d + 0.1f,
             (Color){c.r, c.g, c.b, 40});
}

static void add_floor_tiles(Scene *s, float cx, float cz, float w, float d,
                            float tile_size, Color a, Color b) {
    int cols = (int)(w / tile_size);
    int rows = (int)(d / tile_size);
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            float tx = cx - w/2 + c * tile_size + tile_size/2;
            float tz = cz - d/2 + r * tile_size + tile_size/2;
            Color tc = ((r + c) % 2 == 0) ? a : b;
            add_wall(s, tx, -0.05f, tz, tile_size - 0.02f, 0.1f, tile_size - 0.02f, tc);
        }
    }
}

void build_lobby(Scene *s) {
    memset(s, 0, sizeof(Scene));

    Color concrete = {140, 140, 148, 255};
    Color gold = {210, 175, 95, 255};
    Color deep_gold = {160, 120, 55, 255};
    Color plant = {35, 100, 45, 255};
    Color dark_plant = {20, 60, 30, 255};
    Color titanium = {195, 180, 125, 255};
    Color warm_cream = {220, 200, 160, 255};
    Color blake_magenta = {180, 50, 90, 255};
    Color blake_coral = {220, 100, 80, 255};
    Color blake_teal = {40, 140, 130, 255};
    Color blake_violet = {100, 50, 150, 255};
    Color dark_floor_a = {55, 42, 22, 255};
    Color dark_floor_b = {65, 50, 28, 255};

    s->fog_color = (Color){12, 10, 5, 255};
    s->fog_density = 0.03f;

    add_floor_tiles(s, 0, 0, 30, 20, 2.0f, dark_floor_a, dark_floor_b);

    add_wall(s, 0, 7, 0, 30, 0.2f, 20, (Color){45, 35, 20, 255});
    add_light_panel(s, -5, 6.8f, 0, 8, 0.1f, 0.8f, warm_cream);
    add_light_panel(s, 5, 6.8f, -3, 6, 0.1f, 0.8f, gold);
    add_light_panel(s, 0, 6.8f, 5, 10, 0.1f, 0.6f, blake_coral);

    add_gradient_wall(s, 0, -10, 30, 0.3f, 7, 6, concrete, blake_violet);

    add_wall(s, -12, 3.5f, 10, 10, 7, 0.3f, concrete);
    add_wall(s, 10, 3.5f, 10, 8, 7, 0.3f, concrete);
    add_wall(s, -2, 5.5f, 10, 6, 3, 0.3f, concrete);
    add_light_panel(s, -2, 4.5f, 10.1f, 6, 1.5f, 0.1f, blake_magenta);

    add_wall(s, -15, 2, 0, 0.3f, 4, 20, concrete);
    add_wall(s, -15, 5, 0, 0.3f, 2, 20, (Color){60, 70, 80, 255});
    add_wall(s, -15, 6.5f, 0, 0.3f, 1, 20, blake_teal);

    add_wall(s, 15, 2, -5, 0.3f, 4, 10, concrete);
    add_wall(s, 15, 2, 6, 0.3f, 4, 8, concrete);
    add_wall(s, 15, 5, 0, 0.3f, 2, 20, deep_gold);
    add_wall(s, 15, 6.5f, 0, 0.3f, 1, 20, gold);
    add_light_panel(s, 14.9f, 1.5f, 1, 0.1f, 3, 2.5f, gold);

    add_column(s, -9, -5, 0.6f, 7, titanium);
    add_column(s, -4, 1, 0.5f, 7, concrete);
    add_column(s, 4, -6, 0.55f, 7, titanium);
    add_column(s, 8, 2, 0.45f, 7, (Color){170, 160, 130, 255});
    add_column(s, -1, -3, 0.4f, 7, gold);

    add_gradient_wall(s, -11, -8, 3, 0.5f, 4, 4, dark_plant, plant);
    add_gradient_wall(s, 7, -8, 2.5f, 0.5f, 3.5f, 4, dark_plant, plant);
    add_wall(s, -6, 1.5f, -9.5f, 1.5f, 3, 0.3f, plant);
    add_wall(s, 10, 1, -9.5f, 1, 2, 0.3f, plant);

    add_wall(s, 0, 0.55f, -7, 7, 1.1f, 1.8f, deep_gold);
    add_wall(s, 0, 0.3f, -7, 7.2f, 0.6f, 1.9f, (Color){90, 65, 35, 255});
    add_wall(s, 0, 1.12f, -7, 7.1f, 0.05f, 1.85f, gold);
    add_light_panel(s, -2, 1.3f, -7, 0.3f, 0.5f, 0.3f, warm_cream);

    add_light_panel(s, -14.8f, 3.5f, -6, 0.1f, 3, 4, blake_magenta);
    add_light_panel(s, -14.8f, 2.5f, 3, 0.1f, 2.5f, 3, blake_teal);
    add_light_panel(s, -14.8f, 4.5f, 0, 0.1f, 1.5f, 5, blake_coral);

    add_light_panel(s, 14.8f, 4, -4, 0.1f, 2, 3, blake_violet);
    add_light_panel(s, 14.8f, 2, -8, 0.1f, 1.5f, 2, blake_coral);

    add_wall(s, 6, 1, 3, 0.8f, 2, 0.15f, blake_magenta);
    add_wall(s, 6.3f, 1.5f, 3.2f, 0.15f, 1.5f, 0.6f, blake_coral);
    add_wall(s, 5.7f, 0.8f, 2.8f, 0.5f, 0.3f, 0.8f, titanium);

    add_wall(s, -8, 0.3f, 5, 3, 0.6f, 0.8f, (Color){80, 60, 40, 255});
    add_wall(s, -8, 0.3f, -1, 3, 0.6f, 0.8f, (Color){80, 60, 40, 255});

    s->spawn = (Vector3){0, 1.6f, 8};
    s->exit_pos = (Vector3){14.5f, 1.6f, 1};
    s->has_exit = true;
}

void build_hallway(Scene *s) {
    memset(s, 0, sizeof(Scene));

    Color concrete = {120, 120, 128, 255};
    Color gold = {200, 165, 90, 255};
    Color red = {210, 55, 45, 255};
    Color blue = {45, 80, 190, 255};
    Color warm_floor = {50, 38, 22, 255};
    Color dark_floor = {40, 30, 18, 255};
    Color blake_pink = {220, 120, 140, 255};
    Color blake_amber = {230, 170, 60, 255};

    s->fog_color = (Color){10, 8, 5, 255};
    s->fog_density = 0.025f;

    float L = 40, W = 4.5f, H = 4;

    add_floor_tiles(s, 0, -L/2, W, L, 1.5f, warm_floor, dark_floor);

    add_wall(s, 0, H, -L/2, W, 0.2f, L, (Color){38, 30, 18, 255});
    for (int i = 0; i < 8; i++) {
        float z = -3 - i * 5;
        add_light_panel(s, 0, H - 0.1f, z, 2, 0.05f, 0.6f, blake_amber);
    }

    add_gradient_wall(s, -W/2, -L/2, 0.3f, L, H, 5, (Color){70, 65, 60, 255}, concrete);
    add_gradient_wall(s, W/2, -L/2, 0.3f, L, H, 5, (Color){70, 65, 60, 255}, concrete);

    add_wall(s, 0, H/2, 0, W, H, 0.3f, concrete);
    add_wall(s, 0, H/2, -L, W, H, 0.3f, concrete);

    add_wall(s, -W/2 + 0.05f, 0.5f, -L/2, 0.1f, 1, L, (Color){60, 42, 25, 255});
    add_wall(s, W/2 - 0.05f, 0.5f, -L/2, 0.1f, 1, L, (Color){60, 42, 25, 255});
    add_wall(s, -W/2 + 0.05f, 1.02f, -L/2, 0.1f, 0.04f, L, gold);
    add_wall(s, W/2 - 0.05f, 1.02f, -L/2, 0.1f, 0.04f, L, gold);

    for (int i = 0; i < 8; i++) {
        float z = -3.5f - i * 4.5f;
        Color door_c = (i % 2 == 0) ? red : blue;
        Color glow_c = (i % 2 == 0) ? blake_pink : (Color){80, 120, 220, 200};
        float side = (i % 2 == 0) ? -(W/2 - 0.1f) : (W/2 - 0.1f);

        add_wall(s, side, 1.3f, z, 0.12f, 2.6f, 1.3f, door_c);
        add_wall(s, side, 2.75f, z, 0.12f, 0.2f, 1.5f, gold);
        add_wall(s, side, 1.3f, z - 0.7f, 0.12f, 2.6f, 0.08f, gold);
        add_wall(s, side, 1.3f, z + 0.7f, 0.12f, 2.6f, 0.08f, gold);
        add_light_panel(s, side, 3.2f, z, 0.08f, 0.6f, 1.4f, glow_c);
        add_wall(s, side * 0.95f, 2, z - 0.9f, 0.08f, 0.3f, 0.2f, gold);
    }

    add_wall(s, 0, 0.01f, -L/2, 1.8f, 0.02f, L, (Color){100, 30, 25, 255});

    s->spawn = (Vector3){0, 1.6f, -1};
    s->exit_pos = (Vector3){0, 1.6f, -L + 1};
    s->has_exit = true;
}

void build_hotel_room(Scene *s) {
    memset(s, 0, sizeof(Scene));

    Color warm_wall = {155, 128, 82, 255};
    Color wall_base = {100, 78, 48, 255};
    Color gold = {210, 175, 95, 255};
    Color cream = {240, 232, 212, 255};
    Color burgundy = {145, 38, 48, 255};
    Color wood = {95, 65, 38, 255};
    Color dark_wood = {65, 45, 28, 255};
    Color pillow_white = {235, 228, 215, 255};
    Color leather = {110, 55, 35, 255};
    Color blake_warm = {230, 160, 100, 200};
    Color blake_rose = {200, 100, 110, 180};
    Color curtain = {130, 90, 65, 255};
    Color mirror = {180, 190, 200, 180};

    s->fog_color = (Color){10, 8, 4, 255};
    s->fog_density = 0.02f;

    float rw = 12, rd = 10, rh = 3.8f;

    add_floor_tiles(s, 0, 0, rw, rd, 1.0f,
                    (Color){75, 55, 32, 255}, (Color){85, 65, 38, 255});

    add_wall(s, 0, rh, 0, rw, 0.2f, rd, (Color){50, 40, 24, 255});
    add_wall(s, 0, rh - 0.1f, -rd/2 + 0.1f, rw, 0.15f, 0.08f, gold);
    add_wall(s, 0, rh - 0.1f, rd/2 - 0.1f, rw, 0.15f, 0.08f, gold);
    add_wall(s, -rw/2 + 0.1f, rh - 0.1f, 0, 0.08f, 0.15f, rd, gold);
    add_wall(s, rw/2 - 0.1f, rh - 0.1f, 0, 0.08f, 0.15f, rd, gold);

    add_gradient_wall(s, 0, -rd/2, rw, 0.3f, rh, 4, wall_base, warm_wall);
    add_gradient_wall(s, 0, rd/2, rw, 0.3f, rh, 4, wall_base, warm_wall);
    add_gradient_wall(s, -rw/2, 0, 0.3f, rd, rh, 4, wall_base, warm_wall);
    add_gradient_wall(s, rw/2, 0, 0.3f, rd, rh, 4, wall_base, warm_wall);

    add_wall(s, 0, 0.5f, -rd/2 + 0.1f, rw, 1.0f, 0.08f, dark_wood);
    add_wall(s, 0, 0.5f, rd/2 - 0.1f, rw, 1.0f, 0.08f, dark_wood);
    add_wall(s, -rw/2 + 0.1f, 0.5f, 0, 0.08f, 1.0f, rd, dark_wood);
    add_wall(s, rw/2 - 0.1f, 0.5f, 0, 0.08f, 1.0f, rd, dark_wood);

    // Bed
    add_wall(s, 0, 0.2f, -3.5f, 3.6f, 0.4f, 2.2f, dark_wood);
    add_wall(s, 0, 0.5f, -3.5f, 3.4f, 0.3f, 2.0f, cream);
    add_wall(s, -0.7f, 0.7f, -4.3f, 0.8f, 0.25f, 0.5f, pillow_white);
    add_wall(s, 0.7f, 0.7f, -4.3f, 0.8f, 0.25f, 0.5f, pillow_white);
    add_wall(s, 0, 1.2f, -4.6f, 3.8f, 1.8f, 0.15f, burgundy);
    add_wall(s, 0, 2.15f, -4.58f, 3.9f, 0.06f, 0.08f, gold);

    // Bedside tables + lamps
    add_wall(s, -2.8f, 0.3f, -3.8f, 0.7f, 0.6f, 0.7f, wood);
    add_wall(s, 2.8f, 0.3f, -3.8f, 0.7f, 0.6f, 0.7f, wood);
    add_light_panel(s, -2.8f, 0.9f, -3.8f, 0.25f, 0.4f, 0.25f, blake_warm);
    add_light_panel(s, 2.8f, 0.9f, -3.8f, 0.25f, 0.4f, 0.25f, blake_warm);

    // Desk
    add_wall(s, 5.0f, 0.4f, 0, 2.8f, 0.8f, 1.0f, wood);
    add_wall(s, 5.0f, 0.82f, 0, 2.9f, 0.04f, 1.05f, gold);
    add_wall(s, 3.8f, 0.35f, 0, 0.55f, 0.7f, 0.55f, leather);
    add_wall(s, 3.8f, 0.8f, -0.2f, 0.55f, 0.6f, 0.1f, leather);
    add_wall(s, 5.8f, 1.8f, 0, 0.05f, 1.2f, 1.5f, mirror);
    add_wall(s, 5.82f, 1.8f, 0, 0.04f, 1.3f, 0.06f, gold);

    // Sofa
    add_wall(s, -4.5f, 0.25f, 1.5f, 2.8f, 0.5f, 1.0f, burgundy);
    add_wall(s, -4.5f, 0.6f, 2.0f, 2.8f, 0.7f, 0.2f, burgundy);
    add_wall(s, -5.85f, 0.4f, 1.5f, 0.1f, 0.5f, 1.0f, burgundy);
    add_wall(s, -3.15f, 0.4f, 1.5f, 0.1f, 0.5f, 1.0f, burgundy);

    // Coffee table
    add_wall(s, -4.5f, 0.2f, 3.2f, 1.6f, 0.4f, 0.9f, wood);
    add_wall(s, -4.5f, 0.42f, 3.2f, 1.65f, 0.03f, 0.95f, gold);

    // Suitcase
    add_wall(s, 2.2f, 0.2f, 3.8f, 0.9f, 0.4f, 0.55f, (Color){130, 90, 55, 255});
    add_wall(s, 2.0f, 0.35f, 4.06f, 0.2f, 0.15f, 0.02f, (Color){40, 100, 180, 255});
    add_wall(s, 2.3f, 0.3f, 4.06f, 0.15f, 0.12f, 0.02f, (Color){200, 50, 40, 255});
    add_wall(s, 2.45f, 0.38f, 4.06f, 0.12f, 0.1f, 0.02f, (Color){40, 140, 60, 255});

    // Balcony door
    add_wall(s, -5.8f, 1.5f, -1, 0.1f, 2.8f, 1.8f, (Color){170, 190, 215, 100});
    add_wall(s, -5.85f, 3.0f, -1, 0.08f, 0.15f, 2.0f, gold);
    add_wall(s, -5.85f, 0.05f, -1, 0.08f, 0.1f, 2.0f, gold);
    add_wall(s, -5.85f, 1.5f, -1.95f, 0.08f, 2.9f, 0.08f, gold);
    add_wall(s, -5.85f, 1.5f, -0.05f, 0.08f, 2.9f, 0.08f, gold);
    add_wall(s, -5.7f, 1.8f, -2.3f, 0.06f, 3.2f, 0.6f, curtain);
    add_wall(s, -5.7f, 1.8f, 0.3f, 0.06f, 3.2f, 0.6f, curtain);

    // Blake light accents
    add_light_panel(s, 0, 2.5f, -4.9f, 4, 2, 0.05f, blake_rose);
    add_light_panel(s, 0, rh - 0.15f, 0, 1, 0.05f, 1, blake_warm);

    // En suite door
    add_wall(s, 5.8f, 1.3f, -3.5f, 0.1f, 2.5f, 1.2f, (Color){110, 85, 55, 255});
    add_wall(s, 5.82f, 2.65f, -3.5f, 0.08f, 0.12f, 1.3f, gold);

    // Interactive objects
    add_object(s, -2.8f, 1.2f, -3.8f, "lamp", "Change the lightbulb", "Better.",
               (Color){220, 190, 100, 255});
    add_object(s, 5.0f, 1.0f, 0, "drawers", "Unpack your clothes", "Home for now.",
               (Color){180, 130, 80, 255});
    add_object(s, -4.5f, 0.6f, 3.2f, "candles", "Light the candles", "Warm.",
               cream);
    add_object(s, 5.5f, 0.6f, -2, "ashtray", "Clear the cigarettes", "Clean air.",
               (Color){130, 120, 110, 255});
    add_object(s, 0, 0.8f, -3.5f, "bed", "Make the bed", "Smooth.",
               cream);

    s->spawn = (Vector3){0, 1.6f, 4};
    s->has_exit = false;
}
