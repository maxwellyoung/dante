// scene_montage.c — STATE_MONTAGE
// The ending. Thirty Flights of Loving. Every two becomes one.
// 13 rapid cuts. 2-5 second holds. Some single frames.
// The music returns as three notes. Then silence.
#include "game_ctx.h"
#include "palette.h"
#include <math.h>
#include <string.h>

extern GameCtx g;

void set_exposure(float exp);
void hard_cut_to(GameState s);
void show_text(const char *text);
void hide_text(void);

// ── Shot definitions ────────────────────────────────────────────────
// Each shot: build minimal geometry, position camera, hold, cut.

#define MONTAGE_SHOT_COUNT 13

typedef struct {
    float hold_time;     // seconds to hold this shot
    float warmth;
    float grain;
    float saturation;
    float ca;
    float contrast;
    float exposure;
} ShotConfig;

static const ShotConfig shots[MONTAGE_SHOT_COUNT] = {
    // 0: Taxi — second ticket on the seat
    { .hold_time = 3.0f, .warmth = 0.4f, .grain = 0.5f, .saturation = 0.85f, .ca = 2.0f, .contrast = 1.1f, .exposure = -0.05f },
    // 1: Lobby — Gibbons glancing behind you
    { .hold_time = 2.0f, .warmth = 0.2f, .grain = 0.3f, .saturation = 0.9f, .ca = 1.5f, .contrast = 1.0f, .exposure = 0.0f },
    // 2: Suite — two robes on the door
    { .hold_time = 2.0f, .warmth = 0.5f, .grain = 0.4f, .saturation = 0.95f, .ca = 2.0f, .contrast = 1.0f, .exposure = -0.1f },
    // 3: Paris — her coat on the chair (B&W, only the red survives)
    { .hold_time = 2.0f, .warmth = 0.15f, .grain = 0.7f, .saturation = 0.12f, .ca = 3.5f, .contrast = 1.4f, .exposure = 0.1f },
    // 4: Balcony — Earth, alone
    { .hold_time = 3.5f, .warmth = 0.0f, .grain = 0.2f, .saturation = 0.9f, .ca = 1.0f, .contrast = 1.0f, .exposure = 0.1f },
    // 5: Elevator — going down
    { .hold_time = 2.0f, .warmth = 0.3f, .grain = 0.35f, .saturation = 0.9f, .ca = 2.0f, .contrast = 1.0f, .exposure = -0.15f },
    // 6: Cleaned suite — one pillow, one robe, one glass
    { .hold_time = 3.5f, .warmth = 0.0f, .grain = 0.5f, .saturation = 0.7f, .ca = 1.5f, .contrast = 0.9f, .exposure = -0.1f },
    // 7: Paris — toothbrush in glass (SINGLE FRAME, B&W)
    { .hold_time = 0.15f, .warmth = 0.15f, .grain = 0.8f, .saturation = 0.12f, .ca = 4.0f, .contrast = 1.4f, .exposure = 0.0f },
    // 8: Hotel — two chairs by window, empty
    { .hold_time = 2.5f, .warmth = 0.1f, .grain = 0.3f, .saturation = 0.85f, .ca = 1.5f, .contrast = 1.0f, .exposure = 0.0f },
    // 9: Gibbons in the taxi — going home
    { .hold_time = 3.0f, .warmth = 0.4f, .grain = 0.5f, .saturation = 0.8f, .ca = 2.5f, .contrast = 1.1f, .exposure = -0.05f },
    // 10: Photograph — face up, she's laughing
    { .hold_time = 4.0f, .warmth = 0.6f, .grain = 0.4f, .saturation = 1.0f, .ca = 2.0f, .contrast = 1.0f, .exposure = 0.05f },
    // 11: Bed — the indent on the other pillow
    { .hold_time = 5.5f, .warmth = 0.3f, .grain = 0.5f, .saturation = 0.8f, .ca = 1.5f, .contrast = 0.9f, .exposure = -0.15f },
    // 12: The void — empty glass, silence
    { .hold_time = 5.0f, .warmth = 0.0f, .grain = 0.6f, .saturation = 0.5f, .ca = 1.0f, .contrast = 0.8f, .exposure = -0.3f },
};

// ── Shot geometry builders ──────────────────────────────────────────
// Minimal. Just the object. Just the story.

static void build_shot_taxi_ticket(Scene *s) {
    memset(s, 0, sizeof(Scene));
    s->surface = SURFACE_CARPET;
    s->fog_color = (Color){15, 18, 25, 255};
    s->fog_density = 0.01f;
    // Taxi seat — dark leather
    add_wall(s, 0, 0, 0, 2.0f, 0.3f, 1.5f, (Color){35, 30, 28, 255});
    set_last_material(s, MAT_LEATHER);
    // Back of seat
    add_wall(s, 0, 0.6f, -0.7f, 2.0f, 1.0f, 0.15f, (Color){35, 30, 28, 255});
    set_last_material(s, MAT_LEATHER);
    // THE SECOND TICKET — white rectangle on dark seat. The first "two."
    add_wall(s, 0.3f, 0.16f, 0.1f, 0.3f, 0.005f, 0.15f, (Color){245, 242, 235, 255});
    // Blue stripe on ticket
    add_wall(s, 0.3f, 0.165f, 0.05f, 0.3f, 0.003f, 0.03f, (Color){50, 80, 180, 255});
    set_last_decal(s);
    // "2 guests" text area
    add_wall(s, 0.25f, 0.166f, 0.15f, 0.15f, 0.002f, 0.04f, (Color){60, 60, 65, 200});
    set_last_decal(s);
    s->spawn = (Vector3){0, 1.0f, 1.0f};
}

static void build_shot_gibbons_glance(Scene *s) {
    memset(s, 0, sizeof(Scene));
    s->surface = SURFACE_MARBLE;
    s->fog_color = (Color){8, 10, 15, 255};
    s->fog_density = 0.005f;
    // Lobby floor — checkerboard marble
    add_wall(s, 0, -0.05f, 0, 8.0f, 0.1f, 8.0f, (Color){180, 175, 168, 255});
    set_last_material(s, MAT_CHECKERBOARD);
    // Column behind Gibbons
    add_cylinder(s, 0, 1.5f, -2.0f, 0.3f, 3.0f, (Color){190, 185, 178, 255});
    set_last_material(s, MAT_MARBLE);
    // Gibbons — geometric cube-person, head turned back
    // Body
    add_wall(s, 0, 1.0f, 0, 0.3f, 0.5f, 0.2f, (Color){30, 35, 50, 255}); // jacket
    set_last_material(s, MAT_FABRIC);
    // Head — turned to look behind (offset in X)
    add_wall(s, 0.08f, 1.45f, 0, 0.15f, 0.18f, 0.15f, (Color){200, 175, 150, 255}); // head
    // Hat
    add_wall(s, 0.08f, 1.58f, 0, 0.18f, 0.06f, 0.18f, (Color){30, 25, 22, 255}); // brim
    add_wall(s, 0.08f, 1.65f, 0, 0.14f, 0.1f, 0.14f, (Color){30, 25, 22, 255}); // crown
    // Legs
    add_wall(s, -0.08f, 0.4f, 0, 0.1f, 0.45f, 0.1f, (Color){30, 35, 50, 255});
    add_wall(s, 0.08f, 0.4f, 0, 0.1f, 0.45f, 0.1f, (Color){30, 35, 50, 255});
    s->spawn = (Vector3){0, 1.6f, 3.0f};
}

static void build_shot_two_robes(Scene *s) {
    memset(s, 0, sizeof(Scene));
    s->surface = SURFACE_WOOD;
    s->fog_color = (Color){20, 18, 15, 255};
    s->fog_density = 0.008f;
    // Door — dark wood
    add_wall(s, 0, 1.3f, 0, 1.0f, 2.6f, 0.1f, (Color){85, 65, 40, 255});
    set_last_material(s, MAT_WOOD);
    // Brass hook bar
    add_wall(s, 0, 1.8f, 0.08f, 0.6f, 0.04f, 0.04f, PAL_BRASS);
    set_last_material(s, MAT_BRASS);
    // TWO ROBES — different sizes. Hers and yours.
    add_wall(s, -0.2f, 1.2f, 0.12f, 0.05f, 1.4f, 0.4f, (Color){225, 215, 200, 255}); // hers — cream, smaller
    set_last_material(s, MAT_FABRIC);
    add_wall(s, 0.2f, 1.3f, 0.12f, 0.06f, 1.6f, 0.5f, (Color){50, 55, 80, 255}); // yours — navy, larger
    set_last_material(s, MAT_FABRIC);
    s->spawn = (Vector3){0, 1.6f, 2.0f};
}

static void build_shot_paris_coat(Scene *s) {
    memset(s, 0, sizeof(Scene));
    s->surface = SURFACE_WOOD;
    s->fog_color = (Color){30, 22, 10, 255};
    s->fog_density = 0.012f;
    // Floor
    add_wall(s, 0, -0.05f, 0, 4.0f, 0.1f, 4.0f, (Color){120, 95, 65, 255});
    set_last_material(s, MAT_PARQUET);
    // Chair — simple wooden
    add_wall(s, 0, 0.35f, 0, 0.4f, 0.04f, 0.4f, (Color){140, 100, 55, 255}); // seat
    set_last_material(s, MAT_WOOD);
    add_wall(s, 0, 0.7f, -0.18f, 0.4f, 0.55f, 0.04f, (Color){140, 100, 55, 255}); // back
    set_last_material(s, MAT_WOOD);
    // HER COAT — draped over the chair back. She'll be right back.
    add_wall(s, 0, 0.75f, -0.14f, 0.5f, 0.6f, 0.08f, (Color){55, 45, 40, 220});
    set_last_material(s, MAT_FABRIC);
    s->spawn = (Vector3){0, 1.4f, 2.0f};
}

static void build_shot_balcony_earth(Scene *s) {
    memset(s, 0, sizeof(Scene));
    s->surface = SURFACE_MARBLE;
    s->fog_color = (Color){2, 3, 8, 255};
    s->fog_density = 0.001f;
    // Railing
    add_wall(s, 0, 0.9f, 0, 3.0f, 0.04f, 0.04f, PAL_BRASS); // top rail
    set_last_material(s, MAT_BRASS);
    // Floor
    add_wall(s, 0, -0.05f, 0.5f, 4.0f, 0.1f, 2.0f, (Color){40, 42, 48, 255});
    set_last_material(s, MAT_TILE);
    // Two chairs — empty, facing the void
    add_wall(s, -0.5f, 0.35f, 0.3f, 0.35f, 0.04f, 0.35f, (Color){140, 100, 55, 255});
    set_last_material(s, MAT_WOOD);
    add_wall(s, 0.5f, 0.35f, 0.3f, 0.35f, 0.04f, 0.35f, (Color){140, 100, 55, 255});
    set_last_material(s, MAT_WOOD);
    // Earth glow below — large blue-green sphere hint
    add_wall(s, 0, -3.0f, -5.0f, 8.0f, 0.1f, 8.0f, (Color){40, 80, 120, 60});
    s->spawn = (Vector3){0, 1.6f, 1.5f};
}

static void build_shot_elevator_down(Scene *s) {
    memset(s, 0, sizeof(Scene));
    s->surface = SURFACE_MARBLE;
    s->fog_color = (Color){25, 20, 12, 255};
    s->fog_density = 0.01f;
    // Elevator box — brass walls
    float ew = 2.0f, eh = 3.0f, ed = 2.0f;
    add_wall(s, 0, -0.05f, 0, ew, 0.1f, ed, (Color){80, 75, 68, 255});
    set_last_material(s, MAT_TILE);
    add_wall(s, -ew/2, eh/2, 0, 0.08f, eh, ed, PAL_BRASS);
    set_last_material(s, MAT_BRASS);
    add_wall(s, ew/2, eh/2, 0, 0.08f, eh, ed, PAL_BRASS);
    set_last_material(s, MAT_BRASS);
    add_wall(s, 0, eh/2, -ed/2, ew, eh, 0.08f, PAL_BRASS);
    set_last_material(s, MAT_BRASS);
    // Doors — closed, going down now
    add_wall(s, -0.4f, eh/2, ed/2, 0.75f, eh, 0.06f, (Color){160, 150, 135, 255});
    set_last_material(s, MAT_BRASS);
    add_wall(s, 0.4f, eh/2, ed/2, 0.75f, eh, 0.06f, (Color){160, 150, 135, 255});
    set_last_material(s, MAT_BRASS);
    // Floor indicator — "G" (going down)
    add_wall(s, 0, eh-0.3f, ed/2-0.02f, 0.15f, 0.15f, 0.01f, (Color){220, 180, 60, 255});
    // Mirror on back wall — you alone
    add_wall(s, 0, 1.5f, -ed/2+0.06f, 1.2f, 1.5f, 0.03f, (Color){210, 215, 222, 180});
    set_last_material(s, MAT_GLASS);
    s->spawn = (Vector3){0, 1.6f, 0.3f};
}

static void build_shot_cleaned_suite(Scene *s) {
    memset(s, 0, sizeof(Scene));
    s->surface = SURFACE_WOOD;
    s->fog_color = (Color){10, 10, 15, 255};
    s->fog_density = 0.006f;
    // Floor
    add_wall(s, 0, -0.05f, 0, 6.0f, 0.1f, 5.0f, (Color){120, 95, 65, 255});
    set_last_material(s, MAT_HERRINGBONE);
    // Bed — ONE pillow, centered. The hotel has already cleaned you out.
    add_wall(s, 0, 0.2f, -1.5f, 2.8f, 0.4f, 1.4f, (Color){85, 65, 40, 255});
    set_last_material(s, MAT_WOOD);
    add_wall(s, 0, 0.48f, -1.5f, 2.6f, 0.2f, 1.2f, (Color){245, 242, 235, 255});
    set_last_material(s, MAT_FABRIC);
    // ONE pillow — centered. Not two. One.
    add_wall(s, 0, 0.64f, -2.0f, 0.5f, 0.15f, 0.35f, (Color){240, 238, 232, 255});
    set_last_material(s, MAT_FABRIC);
    // ONE robe on door hook (in background)
    add_wall(s, 2.5f, 1.3f, -2.2f, 0.06f, 1.6f, 0.5f, (Color){50, 55, 80, 255});
    set_last_material(s, MAT_FABRIC);
    // ONE glass on nightstand — the other one cleared away
    add_wall(s, -1.5f, 0.3f, -1.8f, 0.5f, 0.6f, 0.5f, (Color){85, 65, 40, 255}); // nightstand
    set_last_material(s, MAT_WOOD);
    add_cone(s, -1.5f, 0.62f, -1.8f, 0.04f, 0.08f, (Color){210, 210, 215, 200}); // glass
    // Window — void outside
    add_wall(s, -3.0f, 1.5f, 0, 0.08f, 3.0f, 4.0f, (Color){10, 15, 25, 200});
    set_last_material(s, MAT_GLASS);
    s->spawn = (Vector3){0, 1.6f, 3.0f};
}

static void build_shot_paris_toothbrush(Scene *s) {
    memset(s, 0, sizeof(Scene));
    s->surface = SURFACE_MARBLE;
    s->fog_color = (Color){30, 22, 10, 255};
    s->fog_density = 0.015f;
    // Sink counter
    add_wall(s, 0, 0.8f, 0, 1.0f, 0.06f, 0.5f, (Color){220, 220, 215, 255});
    set_last_material(s, MAT_TILE);
    // Glass
    add_cylinder(s, 0, 0.9f, 0, 0.035f, 0.1f, (Color){220, 225, 230, 160});
    // HER TOOTHBRUSH — blue, still in the glass
    add_cylinder(s, 0.02f, 1.02f, 0, 0.01f, 0.12f, (Color){60, 140, 180, 255});
    // Mirror behind — dark reflection
    add_wall(s, 0, 1.5f, -0.3f, 0.8f, 0.8f, 0.03f, (Color){210, 215, 222, 180});
    set_last_material(s, MAT_GLASS);
    s->spawn = (Vector3){0, 1.2f, 0.8f};
}

static void build_shot_two_chairs(Scene *s) {
    memset(s, 0, sizeof(Scene));
    s->surface = SURFACE_MARBLE;
    s->fog_color = (Color){8, 10, 15, 255};
    s->fog_density = 0.005f;
    // Floor
    add_wall(s, 0, -0.05f, 0, 6.0f, 0.1f, 6.0f, (Color){180, 175, 168, 255});
    set_last_material(s, MAT_CHECKERBOARD);
    // Window — Earth visible through glass
    add_wall(s, 0, 2.0f, -3.0f, 5.0f, 4.0f, 0.1f, (Color){30, 60, 100, 100});
    set_last_material(s, MAT_GLASS);
    // Earth glow through window
    add_wall(s, 0, 1.5f, -2.8f, 3.0f, 2.0f, 0.05f, (Color){40, 80, 120, 60});
    // TWO CHAIRS — angled toward each other. For watching Earth together. Empty.
    // Left chair
    add_wall(s, -0.6f, 0.35f, -0.5f, 0.45f, 0.04f, 0.45f, (Color){120, 85, 45, 255});
    set_last_material(s, MAT_LEATHER);
    add_wall(s, -0.6f, 0.7f, -0.72f, 0.45f, 0.55f, 0.06f, (Color){120, 85, 45, 255});
    set_last_material(s, MAT_LEATHER);
    // Right chair
    add_wall(s, 0.6f, 0.35f, -0.5f, 0.45f, 0.04f, 0.45f, (Color){120, 85, 45, 255});
    set_last_material(s, MAT_LEATHER);
    add_wall(s, 0.6f, 0.7f, -0.72f, 0.45f, 0.55f, 0.06f, (Color){120, 85, 45, 255});
    set_last_material(s, MAT_LEATHER);
    // Side table between chairs — small
    add_wall(s, 0, 0.3f, -0.5f, 0.3f, 0.6f, 0.3f, (Color){85, 65, 40, 255});
    set_last_material(s, MAT_WOOD);
    s->spawn = (Vector3){0, 1.6f, 3.0f};
}

static void build_shot_gibbons_taxi(Scene *s) {
    memset(s, 0, sizeof(Scene));
    s->surface = SURFACE_CARPET;
    s->fog_color = (Color){15, 18, 25, 255};
    s->fog_density = 0.01f;
    // Taxi seat — same as opening
    add_wall(s, 0, 0, 0, 2.0f, 0.3f, 1.5f, (Color){35, 30, 28, 255});
    set_last_material(s, MAT_LEATHER);
    add_wall(s, 0, 0.6f, -0.7f, 2.0f, 1.0f, 0.15f, (Color){35, 30, 28, 255});
    set_last_material(s, MAT_LEATHER);
    // Gibbons — sitting in the back seat, going home. Already leaving.
    // Body (seated)
    add_wall(s, -0.3f, 0.5f, 0, 0.3f, 0.45f, 0.2f, (Color){30, 35, 50, 255});
    set_last_material(s, MAT_FABRIC);
    // Head — facing you. Brief eye contact.
    add_wall(s, -0.3f, 0.88f, 0.05f, 0.15f, 0.18f, 0.15f, (Color){200, 175, 150, 255});
    // Hat
    add_wall(s, -0.3f, 1.01f, 0.05f, 0.18f, 0.06f, 0.18f, (Color){30, 25, 22, 255});
    add_wall(s, -0.3f, 1.08f, 0.05f, 0.14f, 0.1f, 0.14f, (Color){30, 25, 22, 255});
    // Tie — he adjusts it
    add_wall(s, -0.3f, 0.65f, 0.12f, 0.04f, 0.2f, 0.02f, (Color){140, 30, 30, 255});
    // Window — dawn light
    add_wall(s, -1.5f, 0.8f, 0, 0.06f, 1.2f, 2.0f, (Color){180, 150, 100, 80});
    set_last_material(s, MAT_GLASS);
    s->spawn = (Vector3){0.5f, 1.0f, 1.2f};
}

static void build_shot_photograph(Scene *s) {
    memset(s, 0, sizeof(Scene));
    s->surface = SURFACE_WOOD;
    s->fog_color = (Color){20, 18, 15, 255};
    s->fog_density = 0.008f;
    // Nightstand
    add_wall(s, 0, 0.3f, 0, 0.5f, 0.6f, 0.5f, (Color){85, 65, 40, 255});
    set_last_material(s, MAT_WOOD);
    // THE PHOTOGRAPH — face up now. Warm sepia tones. She's laughing.
    add_wall(s, 0, 0.62f, 0, 0.2f, 0.01f, 0.15f, (Color){210, 195, 170, 255});
    // Frame
    add_wall(s, 0, 0.625f, 0, 0.22f, 0.015f, 0.17f, (Color){85, 65, 40, 255});
    set_last_material(s, MAT_WOOD);
    // Her dress — tiny red dot in the photo. The only clear image of happiness.
    add_wall(s, -0.03f, 0.63f, 0.02f, 0.025f, 0.003f, 0.02f, (Color){190, 50, 45, 200});
    set_last_decal(s);
    // Lamp glow — warm light on the photo
    add_light_panel(s, 0, 0.8f, 0, 0.3f, 0.2f, 0.3f, (Color){255, 220, 160, 150});
    s->spawn = (Vector3){0, 1.2f, 1.0f};
}

static void build_shot_pillow_indent(Scene *s) {
    memset(s, 0, sizeof(Scene));
    s->surface = SURFACE_CARPET;
    s->fog_color = (Color){10, 10, 15, 255};
    s->fog_density = 0.008f;
    // Bed surface — close up, from lying position
    add_wall(s, 0, 0, 0, 3.0f, 0.2f, 2.0f, (Color){245, 242, 235, 255});
    set_last_material(s, MAT_FABRIC);
    // Your pillow — where your head is
    add_wall(s, 0.5f, 0.15f, -0.3f, 0.5f, 0.15f, 0.35f, (Color){240, 238, 232, 255});
    set_last_material(s, MAT_FABRIC);
    // THE OTHER PILLOW — with an indent. Did housekeeping not smooth it,
    // or is that your memory?
    add_wall(s, -0.5f, 0.12f, -0.3f, 0.5f, 0.1f, 0.35f, (Color){238, 235, 228, 255});
    set_last_material(s, MAT_FABRIC);
    // The indent — slightly darker, slightly lower
    add_wall(s, -0.5f, 0.11f, -0.3f, 0.25f, 0.005f, 0.18f, (Color){225, 222, 215, 255});
    set_last_material(s, MAT_FABRIC);
    set_last_decal(s);
    // Headboard behind
    add_wall(s, 0, 0.6f, -0.8f, 3.2f, 0.8f, 0.1f, (Color){50, 45, 70, 255});
    set_last_material(s, MAT_FABRIC);
    s->spawn = (Vector3){0.5f, 0.4f, 0.5f};
}

static void build_shot_void_glass(Scene *s) {
    memset(s, 0, sizeof(Scene));
    s->surface = SURFACE_MARBLE;
    s->fog_color = (Color){2, 2, 4, 255};
    s->fog_density = 0.002f;
    // Nothing. Just the glass. In blackness.
    // The champagne glass — empty. Still empty. The game's final image.
    add_cone(s, 0, 0, 0, 0.06f, 0.1f, (Color){210, 210, 215, 200});    // bowl
    add_cylinder(s, 0, 0.08f, 0, 0.015f, 0.08f, (Color){210, 210, 215, 180}); // stem
    add_cylinder(s, 0, -0.04f, 0, 0.05f, 0.02f, (Color){210, 210, 215, 180}); // base
    // Faintest light — just enough to see the glass
    add_light_panel(s, 0, 0.5f, 0, 0.3f, 0.2f, 0.3f, (Color){180, 180, 200, 30});
    s->spawn = (Vector3){0, 0.3f, 1.0f};
}

// ── Shot builder dispatch ───────────────────────────────────────────
typedef void (*BuildShotFn)(Scene *s);

static const BuildShotFn shot_builders[MONTAGE_SHOT_COUNT] = {
    build_shot_taxi_ticket,      // 0
    build_shot_gibbons_glance,   // 1
    build_shot_two_robes,        // 2
    build_shot_paris_coat,       // 3
    build_shot_balcony_earth,    // 4
    build_shot_elevator_down,    // 5
    build_shot_cleaned_suite,    // 6
    build_shot_paris_toothbrush, // 7
    build_shot_two_chairs,       // 8
    build_shot_gibbons_taxi,     // 9
    build_shot_photograph,       // 10
    build_shot_pillow_indent,    // 11
    build_shot_void_glass,       // 12
};

// Camera targets per shot — where you're looking
static const Vector3 shot_cam_targets[MONTAGE_SHOT_COUNT] = {
    {0.3f, 0.16f, 0.1f},     // 0: ticket on seat
    {0.08f, 1.45f, 0},        // 1: Gibbons' head
    {0, 1.3f, 0.12f},         // 2: robes
    {0, 0.75f, -0.14f},       // 3: coat
    {0, 0, -5.0f},            // 4: Earth below
    {0, 1.5f, -0.5f},         // 5: elevator doors
    {0, 0.5f, -1.5f},         // 6: bed, one pillow
    {0, 0.9f, 0},             // 7: toothbrush
    {0, 0.5f, -0.5f},         // 8: two chairs
    {-0.3f, 0.88f, 0.05f},    // 9: Gibbons' face
    {0, 0.62f, 0},             // 10: photograph
    {-0.5f, 0.12f, -0.3f},    // 11: other pillow
    {0, 0, 0},                 // 12: glass in void
};

// ── Load: start at shot 0 ───────────────────────────────────────────
static void load_shot(int idx) {
    if (idx < 0 || idx >= MONTAGE_SHOT_COUNT) return;

    shot_builders[idx](&g.scene);
    g.player.camera.position = g.scene.spawn;
    g.player.camera.target = shot_cam_targets[idx];
    g.player.camera.up = (Vector3){0, 1, 0};
    g.player.camera.fovy = 65.0f;
    g.player.control_mult = 0.0f;  // no movement — you just watch

    const ShotConfig *sc = &shots[idx];
    SetPostFXWarmth(&g.postfx, sc->warmth);
    SetPostFXGrain(&g.postfx, sc->grain);
    SetPostFXSaturation(&g.postfx, sc->saturation);
    SetPostFXCA(&g.postfx, sc->ca);
    SetPostFXContrast(&g.postfx, sc->contrast);
    set_exposure(sc->exposure);
    SetPostFXVignette(&g.postfx, 1.5f);

    // Per-shot lighting — match the scene each shot is from
    switch (idx) {
        case 0: case 9:  // Taxi shots
            SetSceneLighting(&g.lighting, LightingPreset_Taxi());
            break;
        case 1:  // Lobby
            SetSceneLighting(&g.lighting, LightingPreset_Lobby());
            break;
        case 3: case 7:  // Paris shots
            SetSceneLighting(&g.lighting, LightingPreset_ParisDream());
            break;
        case 4:  // Balcony
            SetSceneLighting(&g.lighting, LightingPreset_Balcony());
            break;
        case 5:  // Elevator
            SetSceneLighting(&g.lighting, LightingPreset_Elevator());
            break;
        case 11:  // Pillow close-up — intimate bedside
            SetSceneLighting(&g.lighting, LightingPreset_Room());
            break;
        case 12:  // Void — near darkness
            SetSceneLighting(&g.lighting, LightingPreset_Hyperspace());
            break;
        default:  // Suite shots (2, 6, 8, 10)
            SetSceneLighting(&g.lighting, LightingPreset_SpaceSuite());
            break;
    }

    g.montage_shot_time = 0;
    g.montage_flash = true;
    // Trigger warm white flash — same system as hard_cut_to()
    g.cut_flash_timer = 0.08f;  // shorter than normal cuts — rapid fire
    SetPostFXFlash(&g.postfx, 1.0f, 1.0f, 0.95f, 0.85f);
}

void montage_load(void) {
    StopAllAudio(&g.audio);
    // Total silence. The music is over. Only cuts and silence.
    g.montage_shot = 0;
    g.three_note_played = false;
    load_shot(0);
}

void montage_update(float dt) {
    (void)dt;
    g.montage_shot_time += GetFrameTime();

    // White flash on cut — 2 frames warm white, then gone
    if (g.montage_flash && g.montage_shot_time > 0.05f) {
        g.montage_flash = false;
    }

    // Subtle camera drift — the shots breathe
    {
        float drift = sinf(g.montage_shot_time * 0.4f) * 0.003f;
        g.player.camera.target.x += drift;
        g.player.camera.target.y += sinf(g.montage_shot_time * 0.6f) * 0.001f;
    }

    // ── Per-shot text — not all. Restraint. ──
    // Only the shots that need a word. The rest speak for themselves.
    {
        float st = g.montage_shot_time;
        int shot = g.montage_shot;
        // Shot 0: Taxi ticket — the first "two"
        if (shot == 0 && st > 1.0f && st < 1.5f)
            show_text("2 guests.");
        // Shot 6: Cleaned suite — every two becomes one
        if (shot == 6 && st > 1.5f && st < 2.0f)
            show_text("One.");
        // Shot 10: Photograph — she's laughing
        if (shot == 10 && st > 1.5f && st < 2.0f)
            show_text("She's laughing.");
        // Shot 12: The void glass — the last image
        if (shot == 12 && st > 2.5f && st < 3.0f)
            show_text("Three hours.");
        // Clear text between shots
        if (st < 0.1f) hide_text();
    }

    // Three-note callback — plays during shot 10 (photograph)
    if (g.montage_shot == 10 && !g.three_note_played && g.montage_shot_time > 0.5f) {
        PlayThreeNote(&g.audio);
        g.three_note_played = true;
    }

    // Progressive desaturation in final shots
    if (g.montage_shot >= 11) {
        float fade = fminf(1.0f, g.montage_shot_time / shots[g.montage_shot].hold_time);
        SetPostFXSaturation(&g.postfx, shots[g.montage_shot].saturation * (1.0f - fade * 0.3f));
        SetMasterVolume(fmaxf(0.0f, 1.0f - fade * 0.5f));
    }

    // Shot complete — advance
    if (g.montage_shot_time >= shots[g.montage_shot].hold_time) {
        g.montage_shot++;

        if (g.montage_shot >= MONTAGE_SHOT_COUNT) {
            // Montage over. Silence. Credits.
            SetMasterVolume(0.0f);
            SetPostFXSaturation(&g.postfx, 0.92f);
            SetPostFXContrast(&g.postfx, 1.0f);
            SetPostFXWarmth(&g.postfx, 0.0f);
            hard_cut_to(STATE_RETURN_TAXI);
            return;
        }

        // Hard cut punch between shots (not on single-frame shots)
        if (shots[g.montage_shot].hold_time > 0.5f) {
            PlayHardCutPunch(&g.audio);
        }

        load_shot(g.montage_shot);
    }
}
