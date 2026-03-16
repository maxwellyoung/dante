// ENDEARING VOID — Custom engine on Raylib
// Glitched Games — Maxwell Young & Adam Van der Voorn
//
// First-person exploration. Walter Neistat. Paris hotel. Then space.
// Lo-fi PS1 aesthetic. Entirely code. No editor.

#include "raylib.h"
#include "raymath.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

// ============================================================
// CONSTANTS
// ============================================================

#define RENDER_W 480
#define RENDER_H 300
#define MOVE_SPEED 4.0f
#define SPRINT_SPEED 7.0f
#define MOUSE_SENS 0.003f
#define MAX_OBJECTS 64
#define MAX_WALLS 512

// ============================================================
// TYPES
// ============================================================

typedef enum {
    STATE_TITLE,
    STATE_CAR,
    STATE_DRIVING,
    STATE_HOTEL_EXT,
    STATE_LOBBY,
    STATE_HALLWAY,
    STATE_ROOM,
    STATE_BED,
    STATE_STARS,
} GameState;

typedef struct {
    Vector3 pos;
    Vector3 size;
    Color color;
    bool active;
} Wall;

typedef struct {
    Vector3 pos;
    Color color;
    const char *name;
    const char *prompt;
    const char *done_text;
    bool done;
    bool active;
    float radius;
} InteractObject;

typedef struct {
    Camera3D camera;
    Vector3 velocity;
    bool sprinting;
    float sprint_stamina;
    float bob_timer;
} Player;

typedef struct {
    Wall walls[MAX_WALLS];
    int wall_count;
    InteractObject objects[MAX_OBJECTS];
    int object_count;
    Vector3 spawn;
    Vector3 exit_pos;
    bool has_exit;
    Color floor_color;
    Color ceiling_color;
    Color fog_color;
    float fog_density;
} Scene;

// ============================================================
// GLOBALS
// ============================================================

static GameState state = STATE_TITLE;
static float state_time = 0;
static float fade_alpha = 1.0f;
static float fade_target = 0.0f;
static GameState next_state = STATE_TITLE;
static bool transitioning = false;

static Player player = {0};
static Scene scene = {0};

static RenderTexture2D render_target;
static float planet_angle = 0;
static float drive_speed = 0;
static float drive_time = 0;

static const char *screen_text = NULL;
static float screen_text_timer = 0;

static int tasks_done = 0;
static const int total_tasks = 5;

// ============================================================
// SCENE BUILDING — entirely in code, no files
// ============================================================

static void add_wall(Scene *s, float x, float y, float z, float w, float h, float d, Color c) {
    if (s->wall_count >= MAX_WALLS) return;
    s->walls[s->wall_count++] = (Wall){
        .pos = {x, y, z},
        .size = {w, h, d},
        .color = c,
        .active = true,
    };
}

static void add_object(Scene *s, float x, float y, float z, const char *name,
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

    // Floor
    add_wall(s, cx, -0.1f, cz, w, 0.2f, d, floor_c);
    // Ceiling
    add_wall(s, cx, h, cz, w, 0.2f, d, ceil_c);
    // Walls — North
    add_wall(s, cx, h/2, cz - hd, w, h, 0.3f, wall_c);
    // South
    add_wall(s, cx, h/2, cz + hd, w, h, 0.3f, wall_c);
    // East
    add_wall(s, cx + hw, h/2, cz, 0.3f, h, d, wall_c);
    // West
    add_wall(s, cx - hw, h/2, cz, 0.3f, h, d, wall_c);
}

// Add a pillar (brutalist concrete column)
static void add_pillar(Scene *s, float x, float z, float radius, float height, Color c) {
    add_wall(s, x, height/2, z, radius*2, height, radius*2, c);
}

// Add a doorway (gap in a wall) — represented by two wall segments with a gap
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
    // Door frame top
    add_wall(s, x - total_w/2 + door_offset, y + h*0.35f, z, door_w, h*0.3f, d, c);
}

// Jeremy Blake color helpers — luminous, saturated, layered
static Color blake_lerp(Color a, Color b, float t) {
    return (Color){
        (unsigned char)(a.r + (b.r - a.r) * t),
        (unsigned char)(a.g + (b.g - a.g) * t),
        (unsigned char)(a.b + (b.b - a.b) * t),
        (unsigned char)(a.a + (b.a - a.a) * t),
    };
}

// Octagonal column approximation — 8 thin walls arranged in a circle
static void add_column(Scene *s, float x, float z, float r, float h, Color c) {
    for (int i = 0; i < 8; i++) {
        float angle = i * 3.14159f / 4.0f;
        float wx = x + cosf(angle) * r;
        float wz = z + sinf(angle) * r;
        float nx = cosf(angle);
        float nz = sinf(angle);
        // Wall segment tangent to the circle
        float seg_len = r * 0.8f;
        add_wall(s, wx, h/2, wz,
                 fabsf(nz) * seg_len + 0.1f, h,
                 fabsf(nx) * seg_len + 0.1f, c);
    }
}

// Graduated color wall — multiple thin slices that shift color top to bottom
static void add_gradient_wall(Scene *s, float x, float z, float w, float d,
                              float h, int slices, Color bottom, Color top) {
    float slice_h = h / slices;
    for (int i = 0; i < slices; i++) {
        float t = (float)i / (slices - 1);
        Color c = blake_lerp(bottom, top, t);
        add_wall(s, x, slice_h * i + slice_h/2, z, w, slice_h + 0.02f, d, c);
    }
}

// Light panel — thin translucent glowing surface
static void add_light_panel(Scene *s, float x, float y, float z,
                            float w, float h, float d, Color c) {
    Color glow = {c.r, c.g, c.b, 120};
    add_wall(s, x, y, z, w, h, d, glow);
    // Slightly larger soft glow behind
    add_wall(s, x, y, z + (d > 0.2f ? 0 : 0.05f), w + 0.3f, h + 0.3f, d + 0.1f,
             (Color){c.r, c.g, c.b, 40});
}

// Floor tiles — alternating color pattern
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

static void build_lobby(Scene *s) {
    memset(s, 0, sizeof(Scene));

    // Jeremy Blake palette — saturated, luminous
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

    // Checkered marble floor
    add_floor_tiles(s, 0, 0, 30, 20, 2.0f, dark_floor_a, dark_floor_b);

    // Ceiling — warm with recessed light panels
    add_wall(s, 0, 7, 0, 30, 0.2f, 20, (Color){45, 35, 20, 255});
    // Ceiling light strips — Blake luminous accents
    add_light_panel(s, -5, 6.8f, 0, 8, 0.1f, 0.8f, warm_cream);
    add_light_panel(s, 5, 6.8f, -3, 6, 0.1f, 0.8f, gold);
    add_light_panel(s, 0, 6.8f, 5, 10, 0.1f, 0.6f, blake_coral);

    // Back wall — gradient from concrete to Blake violet at top
    add_gradient_wall(s, 0, -10, 30, 0.3f, 7, 6, concrete, blake_violet);

    // Front wall with entrance — gradient to magenta
    add_wall(s, -12, 3.5f, 10, 10, 7, 0.3f, concrete);
    add_wall(s, 10, 3.5f, 10, 8, 7, 0.3f, concrete);
    // Entrance opening
    add_wall(s, -2, 5.5f, 10, 6, 3, 0.3f, concrete);
    // Magenta glow above entrance
    add_light_panel(s, -2, 4.5f, 10.1f, 6, 1.5f, 0.1f, blake_magenta);

    // Left wall — layered: concrete base, plant mid, teal accent top
    add_wall(s, -15, 2, 0, 0.3f, 4, 20, concrete);
    add_wall(s, -15, 5, 0, 0.3f, 2, 20, (Color){60, 70, 80, 255});
    add_wall(s, -15, 6.5f, 0, 0.3f, 1, 20, blake_teal);

    // Right wall — similar layering with exit
    add_wall(s, 15, 2, -5, 0.3f, 4, 10, concrete);
    add_wall(s, 15, 2, 6, 0.3f, 4, 8, concrete);
    add_wall(s, 15, 5, 0, 0.3f, 2, 20, deep_gold);
    add_wall(s, 15, 6.5f, 0, 0.3f, 1, 20, gold);
    // Exit glow
    add_light_panel(s, 14.9f, 1.5f, 1, 0.1f, 3, 2.5f, gold);

    // Gehry columns — octagonal, varying height, titanium + concrete mix
    add_column(s, -9, -5, 0.6f, 7, titanium);
    add_column(s, -4, 1, 0.5f, 7, concrete);
    add_column(s, 4, -6, 0.55f, 7, titanium);
    add_column(s, 8, 2, 0.45f, 7, (Color){170, 160, 130, 255});
    add_column(s, -1, -3, 0.4f, 7, gold);

    // Eco-brutalist plant walls — vertical gardens
    add_gradient_wall(s, -11, -8, 3, 0.5f, 4, 4, dark_plant, plant);
    add_gradient_wall(s, 7, -8, 2.5f, 0.5f, 3.5f, 4, dark_plant, plant);
    // Smaller plant accents
    add_wall(s, -6, 1.5f, -9.5f, 1.5f, 3, 0.3f, plant);
    add_wall(s, 10, 1, -9.5f, 1, 2, 0.3f, plant);

    // Reception desk — multi-layered
    add_wall(s, 0, 0.55f, -7, 7, 1.1f, 1.8f, deep_gold);
    add_wall(s, 0, 0.3f, -7, 7.2f, 0.6f, 1.9f, (Color){90, 65, 35, 255});
    // Desk top accent
    add_wall(s, 0, 1.12f, -7, 7.1f, 0.05f, 1.85f, gold);
    // Desk lamp glow
    add_light_panel(s, -2, 1.3f, -7, 0.3f, 0.5f, 0.3f, warm_cream);

    // Blake color field panels — the signature move
    // Large luminous panels on walls, layered and saturated
    add_light_panel(s, -14.8f, 3.5f, -6, 0.1f, 3, 4, blake_magenta);
    add_light_panel(s, -14.8f, 2.5f, 3, 0.1f, 2.5f, 3, blake_teal);
    add_light_panel(s, -14.8f, 4.5f, 0, 0.1f, 1.5f, 5, blake_coral);

    // Right wall Blake accents
    add_light_panel(s, 14.8f, 4, -4, 0.1f, 2, 3, blake_violet);
    add_light_panel(s, 14.8f, 2, -8, 0.1f, 1.5f, 2, blake_coral);

    // Freestanding art piece — angular Gehry-style sculptural element
    add_wall(s, 6, 1, 3, 0.8f, 2, 0.15f, blake_magenta);
    add_wall(s, 6.3f, 1.5f, 3.2f, 0.15f, 1.5f, 0.6f, blake_coral);
    add_wall(s, 5.7f, 0.8f, 2.8f, 0.5f, 0.3f, 0.8f, titanium);

    // Benches
    add_wall(s, -8, 0.3f, 5, 3, 0.6f, 0.8f, (Color){80, 60, 40, 255});
    add_wall(s, -8, 0.3f, -1, 3, 0.6f, 0.8f, (Color){80, 60, 40, 255});

    s->spawn = (Vector3){0, 1.6f, 8};
    s->exit_pos = (Vector3){14.5f, 1.6f, 1};
    s->has_exit = true;
}

static void build_hallway(Scene *s) {
    memset(s, 0, sizeof(Scene));

    // Blake hallway palette
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

    // Floor — alternating runner strip
    add_floor_tiles(s, 0, -L/2, W, L, 1.5f, warm_floor, dark_floor);

    // Ceiling with recessed light troughs
    add_wall(s, 0, H, -L/2, W, 0.2f, L, (Color){38, 30, 18, 255});
    // Warm ceiling lights — every 5 meters
    for (int i = 0; i < 8; i++) {
        float z = -3 - i * 5;
        add_light_panel(s, 0, H - 0.1f, z, 2, 0.05f, 0.6f, blake_amber);
    }

    // Side walls — gradient from dark base to lighter top
    add_gradient_wall(s, -W/2, -L/2, 0.3f, L, H, 5, (Color){70, 65, 60, 255}, concrete);
    add_gradient_wall(s, W/2, -L/2, 0.3f, L, H, 5, (Color){70, 65, 60, 255}, concrete);

    // End walls
    add_wall(s, 0, H/2, 0, W, H, 0.3f, concrete);
    add_wall(s, 0, H/2, -L, W, H, 0.3f, concrete);

    // Wainscoting — dark wood strip at bottom of walls
    add_wall(s, -W/2 + 0.05f, 0.5f, -L/2, 0.1f, 1, L, (Color){60, 42, 25, 255});
    add_wall(s, W/2 - 0.05f, 0.5f, -L/2, 0.1f, 1, L, (Color){60, 42, 25, 255});
    // Gold trim strip above wainscoting
    add_wall(s, -W/2 + 0.05f, 1.02f, -L/2, 0.1f, 0.04f, L, gold);
    add_wall(s, W/2 - 0.05f, 1.02f, -L/2, 0.1f, 0.04f, L, gold);

    // Doors — alternating Godard red/blue with Blake glow panels
    for (int i = 0; i < 8; i++) {
        float z = -3.5f - i * 4.5f;
        Color door_c = (i % 2 == 0) ? red : blue;
        Color glow_c = (i % 2 == 0) ? blake_pink : (Color){80, 120, 220, 200};
        float side = (i % 2 == 0) ? -(W/2 - 0.1f) : (W/2 - 0.1f);

        // Door body
        add_wall(s, side, 1.3f, z, 0.12f, 2.6f, 1.3f, door_c);
        // Gold frame
        add_wall(s, side, 2.75f, z, 0.12f, 0.2f, 1.5f, gold);
        add_wall(s, side, 1.3f, z - 0.7f, 0.12f, 2.6f, 0.08f, gold);
        add_wall(s, side, 1.3f, z + 0.7f, 0.12f, 2.6f, 0.08f, gold);
        // Blake glow panel above door
        add_light_panel(s, side, 3.2f, z, 0.08f, 0.6f, 1.4f, glow_c);

        // Room number placard
        add_wall(s, side * 0.95f, 2, z - 0.9f, 0.08f, 0.3f, 0.2f, gold);
    }

    // Carpet runner — dark red stripe down center
    add_wall(s, 0, 0.01f, -L/2, 1.8f, 0.02f, L, (Color){100, 30, 25, 255});

    s->spawn = (Vector3){0, 1.6f, -1};
    s->exit_pos = (Vector3){0, 1.6f, -L + 1};
    s->has_exit = true;
}

static void build_hotel_room(Scene *s) {
    memset(s, 0, sizeof(Scene));

    // Blake hotel room palette — warm, intimate, luminous
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

    // Floor — herringbone pattern approximation
    add_floor_tiles(s, 0, 0, rw, rd, 1.0f,
                    (Color){75, 55, 32, 255}, (Color){85, 65, 38, 255});

    // Ceiling
    add_wall(s, 0, rh, 0, rw, 0.2f, rd, (Color){50, 40, 24, 255});
    // Crown molding — gold strip at ceiling edge
    add_wall(s, 0, rh - 0.1f, -rd/2 + 0.1f, rw, 0.15f, 0.08f, gold);
    add_wall(s, 0, rh - 0.1f, rd/2 - 0.1f, rw, 0.15f, 0.08f, gold);
    add_wall(s, -rw/2 + 0.1f, rh - 0.1f, 0, 0.08f, 0.15f, rd, gold);
    add_wall(s, rw/2 - 0.1f, rh - 0.1f, 0, 0.08f, 0.15f, rd, gold);

    // Walls — gradient from darker base to warmer top
    add_gradient_wall(s, 0, -rd/2, rw, 0.3f, rh, 4, wall_base, warm_wall);      // back
    add_gradient_wall(s, 0, rd/2, rw, 0.3f, rh, 4, wall_base, warm_wall);       // front
    add_gradient_wall(s, -rw/2, 0, 0.3f, rd, rh, 4, wall_base, warm_wall);      // left
    add_gradient_wall(s, rw/2, 0, 0.3f, rd, rh, 4, wall_base, warm_wall);       // right

    // Wainscoting on all walls
    add_wall(s, 0, 0.5f, -rd/2 + 0.1f, rw, 1.0f, 0.08f, dark_wood);
    add_wall(s, 0, 0.5f, rd/2 - 0.1f, rw, 1.0f, 0.08f, dark_wood);
    add_wall(s, -rw/2 + 0.1f, 0.5f, 0, 0.08f, 1.0f, rd, dark_wood);
    add_wall(s, rw/2 - 0.1f, 0.5f, 0, 0.08f, 1.0f, rd, dark_wood);

    // == BED AREA ==
    // Bed frame — detailed
    add_wall(s, 0, 0.2f, -3.5f, 3.6f, 0.4f, 2.2f, dark_wood);   // base
    add_wall(s, 0, 0.5f, -3.5f, 3.4f, 0.3f, 2.0f, cream);        // mattress
    // Pillows
    add_wall(s, -0.7f, 0.7f, -4.3f, 0.8f, 0.25f, 0.5f, pillow_white);
    add_wall(s, 0.7f, 0.7f, -4.3f, 0.8f, 0.25f, 0.5f, pillow_white);
    // Headboard — tall, upholstered
    add_wall(s, 0, 1.2f, -4.6f, 3.8f, 1.8f, 0.15f, burgundy);
    // Headboard gold trim
    add_wall(s, 0, 2.15f, -4.58f, 3.9f, 0.06f, 0.08f, gold);

    // Bedside tables
    add_wall(s, -2.8f, 0.3f, -3.8f, 0.7f, 0.6f, 0.7f, wood);
    add_wall(s, 2.8f, 0.3f, -3.8f, 0.7f, 0.6f, 0.7f, wood);
    // Lamps on bedside tables — Blake warm glow
    add_light_panel(s, -2.8f, 0.9f, -3.8f, 0.25f, 0.4f, 0.25f, blake_warm);
    add_light_panel(s, 2.8f, 0.9f, -3.8f, 0.25f, 0.4f, 0.25f, blake_warm);

    // == DESK AREA ==
    add_wall(s, 5.0f, 0.4f, 0, 2.8f, 0.8f, 1.0f, wood);
    add_wall(s, 5.0f, 0.82f, 0, 2.9f, 0.04f, 1.05f, gold);  // desk top edge
    // Desk chair
    add_wall(s, 3.8f, 0.35f, 0, 0.55f, 0.7f, 0.55f, leather);
    add_wall(s, 3.8f, 0.8f, -0.2f, 0.55f, 0.6f, 0.1f, leather);  // chair back
    // Mirror above desk
    add_wall(s, 5.8f, 1.8f, 0, 0.05f, 1.2f, 1.5f, mirror);
    // Mirror frame
    add_wall(s, 5.82f, 1.8f, 0, 0.04f, 1.3f, 0.06f, gold);

    // == SOFA AREA ==
    add_wall(s, -4.5f, 0.25f, 1.5f, 2.8f, 0.5f, 1.0f, burgundy);   // seat
    add_wall(s, -4.5f, 0.6f, 2.0f, 2.8f, 0.7f, 0.2f, burgundy);    // back
    add_wall(s, -5.85f, 0.4f, 1.5f, 0.1f, 0.5f, 1.0f, burgundy);   // arm
    add_wall(s, -3.15f, 0.4f, 1.5f, 0.1f, 0.5f, 1.0f, burgundy);   // arm

    // Coffee table
    add_wall(s, -4.5f, 0.2f, 3.2f, 1.6f, 0.4f, 0.9f, wood);
    add_wall(s, -4.5f, 0.42f, 3.2f, 1.65f, 0.03f, 0.95f, gold);  // edge

    // == SUITCASE ==
    add_wall(s, 2.2f, 0.2f, 3.8f, 0.9f, 0.4f, 0.55f, (Color){130, 90, 55, 255});
    // Travel stickers (color patches)
    add_wall(s, 2.0f, 0.35f, 4.06f, 0.2f, 0.15f, 0.02f, (Color){40, 100, 180, 255});  // NZ blue
    add_wall(s, 2.3f, 0.3f, 4.06f, 0.15f, 0.12f, 0.02f, (Color){200, 50, 40, 255});   // Japan red
    add_wall(s, 2.45f, 0.38f, 4.06f, 0.12f, 0.1f, 0.02f, (Color){40, 140, 60, 255});   // SA green

    // == BALCONY DOOR ==
    // Glass door — translucent
    add_wall(s, -5.8f, 1.5f, -1, 0.1f, 2.8f, 1.8f, (Color){170, 190, 215, 100});
    // Gold frame
    add_wall(s, -5.85f, 3.0f, -1, 0.08f, 0.15f, 2.0f, gold);
    add_wall(s, -5.85f, 0.05f, -1, 0.08f, 0.1f, 2.0f, gold);
    add_wall(s, -5.85f, 1.5f, -1.95f, 0.08f, 2.9f, 0.08f, gold);
    add_wall(s, -5.85f, 1.5f, -0.05f, 0.08f, 2.9f, 0.08f, gold);
    // Curtains
    add_wall(s, -5.7f, 1.8f, -2.3f, 0.06f, 3.2f, 0.6f, curtain);
    add_wall(s, -5.7f, 1.8f, 0.3f, 0.06f, 3.2f, 0.6f, curtain);

    // == BLAKE LIGHT ACCENTS ==
    // Warm light wash on the wall behind the bed
    add_light_panel(s, 0, 2.5f, -4.9f, 4, 2, 0.05f, blake_rose);
    // Ceiling light
    add_light_panel(s, 0, rh - 0.15f, 0, 1, 0.05f, 1, blake_warm);

    // == EN SUITE DOOR ==
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

// ============================================================
// PLAYER
// ============================================================

static void init_player(Vector3 pos) {
    player.camera.position = pos;
    player.camera.target = (Vector3){pos.x, pos.y, pos.z - 1};
    player.camera.up = (Vector3){0, 1, 0};
    player.camera.fovy = 65.0f;
    player.camera.projection = CAMERA_PERSPECTIVE;
    player.velocity = (Vector3){0, 0, 0};
    player.sprinting = false;
    player.sprint_stamina = 1.0f;
    player.bob_timer = 0;
}

static void update_player(float dt) {
    // Mouse look
    Vector2 mouse_delta = GetMouseDelta();
    float yaw = mouse_delta.x * MOUSE_SENS;
    float pitch = -mouse_delta.y * MOUSE_SENS;

    // Get camera forward/right vectors
    Vector3 forward = Vector3Normalize(Vector3Subtract(player.camera.target, player.camera.position));
    Vector3 right = Vector3Normalize(Vector3CrossProduct(forward, player.camera.up));

    // Apply yaw
    Matrix yaw_mat = MatrixRotateY(yaw);
    forward = Vector3Transform(forward, yaw_mat);
    // Apply pitch (clamped)
    Vector3 pitch_axis = right;
    float current_pitch = asinf(forward.y);
    float new_pitch = current_pitch + pitch;
    if (new_pitch > 1.2f) pitch = 1.2f - current_pitch;
    if (new_pitch < -1.2f) pitch = -1.2f - current_pitch;
    Matrix pitch_mat = MatrixRotate(pitch_axis, pitch);
    forward = Vector3Transform(forward, pitch_mat);

    player.camera.target = Vector3Add(player.camera.position, forward);

    // Sprint
    player.sprinting = IsKeyDown(KEY_LEFT_SHIFT) && player.sprint_stamina > 0;
    if (player.sprinting) {
        player.sprint_stamina -= dt * 0.5f;
        if (player.sprint_stamina < 0) player.sprint_stamina = 0;
    } else {
        player.sprint_stamina += dt * 0.3f;
        if (player.sprint_stamina > 1) player.sprint_stamina = 1;
    }
    float speed = player.sprinting ? SPRINT_SPEED : MOVE_SPEED;

    // Movement
    Vector3 flat_forward = {forward.x, 0, forward.z};
    flat_forward = Vector3Normalize(flat_forward);
    Vector3 flat_right = {right.x, 0, right.z};
    flat_right = Vector3Normalize(flat_right);

    Vector3 move = {0, 0, 0};
    bool moving = false;
    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) { move = Vector3Add(move, flat_forward); moving = true; }
    if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) { move = Vector3Subtract(move, flat_forward); moving = true; }
    if (IsKeyDown(KEY_A)) { move = Vector3Add(move, flat_right); moving = true; }
    if (IsKeyDown(KEY_D)) { move = Vector3Subtract(move, flat_right); moving = true; }

    if (moving) {
        move = Vector3Normalize(move);
        move = Vector3Scale(move, speed * dt);
        player.bob_timer += dt * (player.sprinting ? 12 : 8);
    }

    // Simple collision with scene walls
    Vector3 new_pos = Vector3Add(player.camera.position, move);

    // Check collision against walls (simple AABB)
    bool blocked = false;
    for (int i = 0; i < scene.wall_count; i++) {
        Wall *w = &scene.walls[i];
        if (!w->active) continue;
        // Expand wall by player radius
        float pr = 0.4f;
        if (new_pos.x > w->pos.x - w->size.x/2 - pr &&
            new_pos.x < w->pos.x + w->size.x/2 + pr &&
            new_pos.z > w->pos.z - w->size.z/2 - pr &&
            new_pos.z < w->pos.z + w->size.z/2 + pr &&
            new_pos.y > w->pos.y - w->size.y/2 &&
            new_pos.y < w->pos.y + w->size.y/2) {
            // Try sliding along axes
            Vector3 try_x = {new_pos.x, player.camera.position.y, player.camera.position.z};
            Vector3 try_z = {player.camera.position.x, player.camera.position.y, new_pos.z};

            bool block_x = (try_x.x > w->pos.x - w->size.x/2 - pr &&
                           try_x.x < w->pos.x + w->size.x/2 + pr &&
                           try_x.z > w->pos.z - w->size.z/2 - pr &&
                           try_x.z < w->pos.z + w->size.z/2 + pr);
            bool block_z = (try_z.x > w->pos.x - w->size.x/2 - pr &&
                           try_z.x < w->pos.x + w->size.x/2 + pr &&
                           try_z.z > w->pos.z - w->size.z/2 - pr &&
                           try_z.z < w->pos.z + w->size.z/2 + pr);

            if (block_x) new_pos.x = player.camera.position.x;
            if (block_z) new_pos.z = player.camera.position.z;
            blocked = true;
        }
    }

    // Head bob
    float bob = sinf(player.bob_timer) * 0.06f;
    new_pos.y = 1.6f + (moving ? bob : 0);

    player.camera.position = new_pos;
    player.camera.target = Vector3Add(new_pos, forward);
}

// ============================================================
// DRAWING
// ============================================================

static void draw_scene_3d(void) {
    // Fog via background clear color
    ClearBackground(scene.fog_color);

    BeginMode3D(player.camera);

    // Distance-based color darkening for fake fog
    Vector3 cam = player.camera.position;

    for (int i = 0; i < scene.wall_count; i++) {
        Wall *w = &scene.walls[i];
        if (!w->active) continue;

        // Simple distance fog — darken color based on distance from camera
        float dx = w->pos.x - cam.x;
        float dz = w->pos.z - cam.z;
        float dist = sqrtf(dx*dx + dz*dz);
        float fog = fminf(1.0f, dist * scene.fog_density);

        Color c = w->color;
        c.r = (unsigned char)(c.r * (1 - fog) + scene.fog_color.r * fog);
        c.g = (unsigned char)(c.g * (1 - fog) + scene.fog_color.g * fog);
        c.b = (unsigned char)(c.b * (1 - fog) + scene.fog_color.b * fog);

        DrawCube(w->pos, w->size.x, w->size.y, w->size.z, c);
    }

    // Interactive objects — soft glowing orbs
    for (int i = 0; i < scene.object_count; i++) {
        InteractObject *obj = &scene.objects[i];
        if (!obj->active || obj->done) continue;
        float pulse = 0.6f + 0.4f * sinf(GetTime() * 2.5f + i * 1.7f);
        Color c = obj->color;
        c.a = (unsigned char)(180 * pulse);
        DrawSphere(obj->pos, 0.2f, c);
        DrawSphere(obj->pos, 0.4f, (Color){c.r, c.g, c.b, (unsigned char)(25 * pulse)});
    }

    // Exit beacon
    if (scene.has_exit) {
        float pulse = 0.4f + 0.3f * sinf(GetTime() * 2);
        DrawSphere(scene.exit_pos, 0.3f, (Color){230, 200, 100, (unsigned char)(80 * pulse)});
        DrawSphere(scene.exit_pos, 0.6f, (Color){230, 200, 100, (unsigned char)(20 * pulse)});
    }

    EndMode3D();
}

static void draw_dither_overlay(void) {
    // Scanlines
    for (int y = 0; y < RENDER_H; y += 2) {
        DrawRectangle(0, y, RENDER_W, 1, (Color){0, 0, 0, 12});
    }
}

static void draw_hud(void) {
    // Interaction prompt
    for (int i = 0; i < scene.object_count; i++) {
        InteractObject *obj = &scene.objects[i];
        if (!obj->active || obj->done) continue;
        float dist = Vector3Distance(player.camera.position, obj->pos);
        if (dist < obj->radius) {
            // Check if looking at it (dot product)
            Vector3 to_obj = Vector3Normalize(Vector3Subtract(obj->pos, player.camera.position));
            Vector3 look = Vector3Normalize(Vector3Subtract(player.camera.target, player.camera.position));
            float dot = Vector3DotProduct(to_obj, look);
            if (dot > 0.7f) {
                DrawRectangle(RENDER_W/2 - 90, RENDER_H/2 + 20, 180, 24, (Color){0, 0, 0, 140});
                DrawText(TextFormat("[E] %s", obj->prompt),
                         RENDER_W/2 - 80, RENDER_H/2 + 25, 10, (Color){240, 230, 210, 230});
            }
        }
    }

    // Task counter
    if (state == STATE_ROOM) {
        DrawText(TextFormat("%d/%d", tasks_done, total_tasks),
                 RENDER_W - 45, 10, 10, (Color){190, 155, 90, 150});
    }

    // Screen text
    if (screen_text && screen_text_timer > 0) {
        float alpha = screen_text_timer < 0.5f ? screen_text_timer / 0.5f : 1.0f;
        DrawRectangle(0, RENDER_H - 36, RENDER_W, 28, (Color){0, 0, 0, (unsigned char)(100 * alpha)});
        DrawText(screen_text, 14, RENDER_H - 30, 10, (Color){230, 225, 215, (unsigned char)(230 * alpha)});
    }
}

static void draw_title(void) {
    ClearBackground((Color){8, 10, 22, 255});

    // Stars
    SetRandomSeed(42);
    for (int i = 0; i < 120; i++) {
        int sx = GetRandomValue(0, RENDER_W);
        int sy = GetRandomValue(0, RENDER_H);
        float bri = 0.3f + (GetRandomValue(0, 100) / 100.0f) * 0.7f;
        float twinkle = 0.7f + 0.3f * sinf(GetTime() * (1 + GetRandomValue(0, 20) / 10.0f) + i);
        DrawPixel(sx, sy, (Color){240, 235, 220, (unsigned char)(255 * bri * twinkle)});
    }

    // Planet
    float cx = RENDER_W / 2, cy = RENDER_H / 2 + 15;
    DrawCircle((int)cx, (int)cy, 45, (Color){60, 55, 72, 255});
    // Craters shift with rotation
    for (int i = 0; i < 6; i++) {
        float crat_x = sinf(i * 1.7f + planet_angle) * 25;
        float crat_y = cosf(i * 2.3f) * 18;
        if (sqrtf(crat_x*crat_x + crat_y*crat_y) < 38) {
            DrawCircle((int)(cx + crat_x), (int)(cy + crat_y), 3 + i % 3, (Color){45, 40, 55, 150});
        }
    }
    // Atmosphere
    DrawCircleLines((int)cx, (int)cy, 47, (Color){100, 110, 150, 40});

    // Title
    const char *title = "E N D E A R I N G   V O I D";
    int tw = MeasureText(title, 12);
    DrawText(title, RENDER_W/2 - tw/2, 25, 12, (Color){240, 232, 210, 230});

    // Subtitle
    const char *sub = "A game by Glitched Games";
    int sw = MeasureText(sub, 8);
    DrawText(sub, RENDER_W/2 - sw/2, RENDER_H - 35, 8, (Color){180, 175, 165, 140});

    // Prompt
    float pulse = 0.4f + 0.4f * sinf(GetTime() * 3);
    const char *prompt = "PRESS ENTER";
    int pw = MeasureText(prompt, 10);
    DrawText(prompt, RENDER_W/2 - pw/2, RENDER_H - 20, 10,
             (Color){230, 190, 90, (unsigned char)(255 * pulse)});
}

// ============================================================
// STATE MANAGEMENT
// ============================================================

static void transition_to(GameState s) {
    transitioning = true;
    next_state = s;
    fade_target = 1.0f;
}

static void load_state(GameState s) {
    state = s;
    state_time = 0;
    screen_text = NULL;
    screen_text_timer = 0;

    switch (s) {
        case STATE_TITLE:
            DisableCursor();
            break;
        case STATE_CAR:
            screen_text = "You wake up.";
            screen_text_timer = 3;
            break;
        case STATE_DRIVING:
            drive_time = 0;
            drive_speed = 0;
            screen_text = "Hold W to drive.";
            screen_text_timer = 3;
            break;
        case STATE_HOTEL_EXT:
            screen_text = "Hotel Chevalier.";
            screen_text_timer = 2.5f;
            break;
        case STATE_LOBBY:
            EnableCursor();
            DisableCursor();
            build_lobby(&scene);
            init_player(scene.spawn);
            screen_text = "Find your room.";
            screen_text_timer = 3;
            break;
        case STATE_HALLWAY:
            build_hallway(&scene);
            init_player(scene.spawn);
            break;
        case STATE_ROOM:
            build_hotel_room(&scene);
            init_player(scene.spawn);
            tasks_done = 0;
            screen_text = "Make this place yours.";
            screen_text_timer = 3;
            break;
        case STATE_BED:
            screen_text = NULL;
            break;
        case STATE_STARS:
            break;
    }

    fade_alpha = 1.0f;
    fade_target = 0.0f;
}

// ============================================================
// MAIN
// ============================================================

int main(void) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(960, 600, "Endearing Void");
    SetTargetFPS(60);

    render_target = LoadRenderTexture(RENDER_W, RENDER_H);
    SetTextureFilter(render_target.texture, TEXTURE_FILTER_POINT);

    DisableCursor();
    load_state(STATE_TITLE);

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        state_time += dt;

        // Fade
        if (fade_alpha != fade_target) {
            float dir = (fade_target > fade_alpha) ? 1 : -1;
            fade_alpha += dir * 2.0f * dt;
            if ((dir > 0 && fade_alpha >= fade_target) || (dir < 0 && fade_alpha <= fade_target)) {
                fade_alpha = fade_target;
                if (transitioning) {
                    transitioning = false;
                    load_state(next_state);
                }
            }
        }

        // Screen text timer
        if (screen_text_timer > 0) {
            screen_text_timer -= dt;
            if (screen_text_timer <= 0) screen_text = NULL;
        }

        // State update
        switch (state) {
            case STATE_TITLE:
                planet_angle += dt * 0.2f;
                if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
                    transition_to(STATE_CAR);
                }
                break;

            case STATE_CAR:
                if (state_time > 4 || IsKeyPressed(KEY_ENTER)) {
                    transition_to(STATE_DRIVING);
                }
                break;

            case STATE_DRIVING:
                drive_time += dt;
                if (IsKeyDown(KEY_W)) drive_speed = fminf(1, drive_speed + dt * 0.5f);
                else drive_speed = fmaxf(0, drive_speed - dt * 0.8f);
                if (drive_time > 10 || IsKeyPressed(KEY_ENTER)) {
                    transition_to(STATE_HOTEL_EXT);
                }
                break;

            case STATE_HOTEL_EXT:
                if (state_time > 4 || IsKeyPressed(KEY_ENTER)) {
                    transition_to(STATE_LOBBY);
                }
                break;

            case STATE_LOBBY:
            case STATE_HALLWAY:
                update_player(dt);
                // Check exit
                if (scene.has_exit) {
                    float dist = Vector3Distance(player.camera.position, scene.exit_pos);
                    if (dist < 1.5f) {
                        if (state == STATE_LOBBY) transition_to(STATE_HALLWAY);
                        else if (state == STATE_HALLWAY) transition_to(STATE_ROOM);
                    }
                }
                break;

            case STATE_ROOM:
                update_player(dt);
                // Interaction
                if (IsKeyPressed(KEY_E)) {
                    for (int i = 0; i < scene.object_count; i++) {
                        InteractObject *obj = &scene.objects[i];
                        if (!obj->active || obj->done) continue;
                        float dist = Vector3Distance(player.camera.position, obj->pos);
                        if (dist < obj->radius) {
                            Vector3 to_obj = Vector3Normalize(Vector3Subtract(obj->pos, player.camera.position));
                            Vector3 look = Vector3Normalize(Vector3Subtract(player.camera.target, player.camera.position));
                            if (Vector3DotProduct(to_obj, look) > 0.5f) {
                                obj->done = true;
                                tasks_done++;
                                screen_text = obj->done_text;
                                screen_text_timer = 2;
                                if (tasks_done >= total_tasks) {
                                    screen_text = "Time to rest.";
                                    screen_text_timer = 3;
                                    // Will transition after text
                                }
                                break;
                            }
                        }
                    }
                }
                // All tasks done → bed
                if (tasks_done >= total_tasks && screen_text_timer <= 0) {
                    transition_to(STATE_BED);
                }
                break;

            case STATE_BED:
                if (state_time > 6 || IsKeyPressed(KEY_ENTER)) {
                    transition_to(STATE_STARS);
                }
                break;

            case STATE_STARS:
                if (IsKeyPressed(KEY_ESCAPE)) {
                    CloseWindow();
                    return 0;
                }
                break;
        }

        // ---- RENDER ----
        BeginTextureMode(render_target);
        ClearBackground(scene.fog_color.a > 0 ? scene.fog_color : (Color){8, 10, 22, 255});

        switch (state) {
            case STATE_TITLE:
                draw_title();
                break;
            case STATE_CAR:
            case STATE_DRIVING:
            case STATE_HOTEL_EXT:
                // 2D scenes — simplified for now
                ClearBackground((Color){5, 5, 10, 255});
                if (state == STATE_DRIVING) {
                    // Road
                    DrawRectangle(0, RENDER_H/2, RENDER_W, RENDER_H/2, (Color){30, 30, 35, 255});
                    float scroll = drive_time * drive_speed * 200;
                    for (int i = 0; i < 8; i++) {
                        float d = fmodf(i * 40 + scroll, 320) / 320.0f;
                        if (d > 0.02f) {
                            int ly = RENDER_H/2 + (int)((RENDER_H/2) * d);
                            int lw = (int)(2 + d * 20);
                            DrawRectangle(RENDER_W/2 - lw/2, ly, lw, 2,
                                         (Color){230, 215, 130, (unsigned char)(100 * (1 - d))});
                        }
                    }
                    // Dashboard
                    DrawRectangle(0, RENDER_H - 40, RENDER_W, 40, (Color){35, 28, 22, 255});
                    DrawText(TextFormat("%d km/h", (int)(drive_speed * 80)),
                             RENDER_W - 70, RENDER_H - 18, 10, (Color){200, 190, 170, 160});
                }
                if (state == STATE_HOTEL_EXT) {
                    // Hotel facade
                    DrawRectangle(RENDER_W/6, RENDER_H/6, RENDER_W*2/3, RENDER_H*2/3,
                                 (Color){180, 150, 90, 255});
                    // Windows
                    for (int r = 0; r < 4; r++) {
                        for (int c = 0; c < 5; c++) {
                            int wx = RENDER_W/6 + 12 + c * ((RENDER_W*2/3 - 24) / 4);
                            int wy = RENDER_H/6 + 10 + r * ((RENDER_H*2/3 - 20) / 4);
                            DrawRectangle(wx, wy, 12, 18, (Color){230, 190, 100, 180});
                        }
                    }
                    DrawText("HOTEL CHEVALIER", RENDER_W/2 - 55, RENDER_H/6 - 14, 10,
                             (Color){230, 190, 90, 230});
                }
                if (state == STATE_CAR) {
                    // Phone glow
                    float glow = 0.5f + 0.5f * sinf(state_time * 4);
                    DrawRectangle(RENDER_W/2 - 20, RENDER_H/2 - 14, 40, 28,
                                 (Color){40, 55, 90, (unsigned char)(200 * glow)});
                }
                break;

            case STATE_LOBBY:
            case STATE_HALLWAY:
            case STATE_ROOM:
                draw_scene_3d();
                break;

            case STATE_BED: {
                ClearBackground((Color){30, 25, 18, 255});
                int star_count = (int)fminf(20, state_time * 4);
                SetRandomSeed(99);
                for (int i = 0; i < star_count; i++) {
                    int sx = GetRandomValue(RENDER_W/10, RENDER_W*9/10);
                    int sy = GetRandomValue(RENDER_H/10, RENDER_H*7/10);
                    float gl = 0.4f + 0.4f * sinf(GetTime() * (0.5f + i * 0.1f) + i * 2);
                    DrawCircle(sx, sy, 2, (Color){130, 230, 130, (unsigned char)(150 * gl)});
                    DrawCircle(sx, sy, 6, (Color){130, 230, 130, (unsigned char)(30 * gl)});
                }
                break;
            }

            case STATE_STARS: {
                ClearBackground((Color){4, 5, 12, 255});
                float zoom = fminf(1, state_time / 3.0f);
                SetRandomSeed(77);
                int count = 60 + (int)(zoom * 140);
                for (int i = 0; i < count; i++) {
                    int sx = GetRandomValue(0, RENDER_W);
                    int sy = GetRandomValue(0, RENDER_H);
                    float bri = (0.2f + (GetRandomValue(0, 80) / 100.0f)) * zoom;
                    float twk = 0.7f + 0.3f * sinf(GetTime() * (0.5f + GetRandomValue(0, 30)/10.0f) + i);
                    DrawPixel(sx, sy, (Color){255, 250, 230, (unsigned char)(255 * bri * twk)});
                }
                if (state_time > 2) {
                    float a = fminf(1, (state_time - 2) / 2.0f);
                    const char *t = "E N D E A R I N G   V O I D";
                    int tw2 = MeasureText(t, 14);
                    DrawText(t, RENDER_W/2 - tw2/2, RENDER_H/2 - 8, 14,
                             (Color){240, 232, 210, (unsigned char)(230 * a)});
                }
                break;
            }
        }

        // Dither overlay
        draw_dither_overlay();

        // HUD
        draw_hud();

        // Fade
        if (fade_alpha > 0.01f) {
            DrawRectangle(0, 0, RENDER_W, RENDER_H, (Color){0, 0, 0, (unsigned char)(255 * fade_alpha)});
        }

        EndTextureMode();

        // Draw render target scaled to window
        BeginDrawing();
        ClearBackground(BLACK);
        float scale = fminf((float)GetScreenWidth() / RENDER_W, (float)GetScreenHeight() / RENDER_H);
        float ox = (GetScreenWidth() - RENDER_W * scale) / 2;
        float oy = (GetScreenHeight() - RENDER_H * scale) / 2;
        DrawTexturePro(render_target.texture,
            (Rectangle){0, 0, RENDER_W, -RENDER_H},
            (Rectangle){ox, oy, RENDER_W * scale, RENDER_H * scale},
            (Vector2){0, 0}, 0, WHITE);
        EndDrawing();
    }

    UnloadRenderTexture(render_target);
    CloseWindow();
    return 0;
}
