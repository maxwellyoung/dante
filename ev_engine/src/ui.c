// ui.c — HUD, crosshair, compass, interaction prompts
// Spring physics for all animated elements. Susan Kare pixel icons.
// Architectural film aesthetic — warm cream on dark, never game-y.
#include "ui.h"
#include "config.h"
#include "palette.h"
#include "raymath.h"
#include <math.h>
#include <string.h>
#include <stdio.h>

// PI provided by config.h

// ── Spring state ───────────────────────────────────────────────────
// Intentional file-local runtime state: ui_init() / ui_reset() own its lifetime,
// and main.c resets it on startup plus every scene load so it stays deterministic.
static float crosshair_scale     = 1.0f;
static float crosshair_scale_vel = 0.0f;

// Compass tick alpha (fades in when moving)
static float compass_alpha     = 0.0f;
static float compass_alpha_vel = 0.0f;

// Interaction ring — grows from 0 to 1 when targeting
static float interact_ring     = 0.0f;
static float interact_ring_vel = 0.0f;

// "E" prompt bounce
static float prompt_scale     = 0.0f;
static float prompt_scale_vel = 0.0f;

// ── Spring helper ──────────────────────────────────────────────────
static float spring_tick(float *val, float *vel, float target, float dt) {
    float f = -SPRING_K * (*val - target) - SPRING_D * (*vel);
    *vel += (f / SPRING_M) * dt;
    *val += *vel * dt;
    return *val;
}

// ── Init / Reset ───────────────────────────────────────────────────
void ui_init(void) {
    ui_reset();
}

void ui_reset(void) {
    crosshair_scale = 1.0f;
    crosshair_scale_vel = 0.0f;
    compass_alpha = 0.0f;
    compass_alpha_vel = 0.0f;
    interact_ring = 0.0f;
    interact_ring_vel = 0.0f;
    prompt_scale = 0.0f;
    prompt_scale_vel = 0.0f;
}

// ── Pixel Icons ────────────────────────────────────────────────────
// Susan Kare style — 5x5 or 7x7 bitmap symbols for each object type
void ui_draw_pixel_icon(int cx, int cy, const char *name, unsigned char a) {
    Color c = {248, 245, 238, a};
    int s = UI_SCALE;
    #define PX(px, py) DrawRectangle(cx + (px)*s, cy + (py)*s, s, s, c)

    if (strcmp(name, "lamp") == 0) {
        PX(-1,-2); PX(0,-2); PX(1,-2);
        PX(-1,-1); PX(1,-1);
        PX(0, 0);
        PX(-1, 1); PX(1, 1);
    } else if (strcmp(name, "candles") == 0) {
        PX(0,-2);
        PX(-1,-1); PX(0,-1); PX(1,-1);
        PX(0, 0);
        PX(0, 1);
    } else if (strcmp(name, "bed") == 0) {
        PX(-2,-1); PX(-1,-1);
        for (int x = -2; x <= 2; x++) PX(x, 0);
        for (int x = -2; x <= 2; x++) PX(x, 1);
    } else if (strcmp(name, "drawers") == 0) {
        for (int x = -2; x <= 2; x++) PX(x,-2);
        for (int x = -2; x <= 2; x++) PX(x, 0);
        for (int x = -2; x <= 2; x++) PX(x, 2);
    } else if (strcmp(name, "ashtray") == 0) {
        PX(-1,-1); PX(0,-1); PX(1,-1);
        PX(-1, 0); PX(1, 0);
        PX(-1, 1); PX(0, 1); PX(1, 1);
    } else if (strcmp(name, "door") == 0 || strcmp(name, "bathroom") == 0) {
        for (int y = -2; y <= 2; y++) { PX(-1,y); PX(1,y); }
        PX(0,-2); PX(0, 2);
        PX(1, 0); // handle
    } else if (strcmp(name, "tap") == 0) {
        PX(0,-2);
        PX(-1,-1); PX(0,-1); PX(1,-1);
        PX(-1, 0); PX(1, 0);
        PX(0, 1);
    } else if (strcmp(name, "mirror") == 0) {
        for (int y = -2; y <= 2; y++) { PX(-1,y); PX(1,y); }
        PX(0,-2); PX(0, 2);
    } else if (strcmp(name, "wardrobe") == 0) {
        for (int y = -3; y <= 2; y++) { PX(-2,y); PX(2,y); }
        PX(-1,-3); PX(0,-3); PX(1,-3);
        PX(-1, 2); PX(0, 2); PX(1, 2);
        for (int y = -2; y <= 1; y++) PX(0,y);
    } else if (strcmp(name, "newspaper") == 0) {
        for (int x = -2; x <= 2; x++) PX(x,-1);
        for (int x = -2; x <= 1; x++) PX(x, 0);
        for (int x = -2; x <= 2; x++) PX(x, 1);
    } else if (strcmp(name, "elevator") == 0) {
        PX(0,-2);
        PX(-1,-1); PX(1,-1);
        PX(-1, 1); PX(1, 1);
        PX(0, 2);
    } else if (strcmp(name, "cigarette") == 0) {
        PX(-2, 1); PX(-1, 0); PX(0, 0); PX(1,-1);
        PX(2,-2); // smoke
    } else if (strcmp(name, "piano") == 0) {
        // Piano keys — 5 white, 3 black
        for (int x = -2; x <= 2; x++) PX(x, 0);
        for (int x = -2; x <= 2; x++) PX(x, 1);
        PX(-1,-1); PX(0,-1); PX(1,-1); // black keys
    } else if (strcmp(name, "record") == 0 || strcmp(name, "vinyl") == 0) {
        // Disc with center dot
        PX(-1,-1); PX(0,-1); PX(1,-1);
        PX(-1, 0); PX(1, 0);
        PX(-1, 1); PX(0, 1); PX(1, 1);
        PX(0, 0); // spindle
    } else if (strcmp(name, "bottle") == 0 || strcmp(name, "wine") == 0) {
        PX(0,-3); PX(0,-2);
        PX(-1,-1); PX(0,-1); PX(1,-1);
        PX(-1, 0); PX(1, 0);
        PX(-1, 1); PX(0, 1); PX(1, 1);
    } else if (strcmp(name, "phone") == 0) {
        for (int y = -2; y <= 2; y++) { PX(-1,y); PX(0,y); PX(1,y); }
    } else {
        // Default: diamond
        PX(0,-1);
        PX(-1, 0); PX(1, 0);
        PX(0, 1);
    }
    #undef PX
}

// ── Crosshair ──────────────────────────────────────────────────────
// Thin cross — 4 short arms with a 1px gap from center
// Spring-scales when targeting interactables
void ui_draw_crosshair(float target_scale) {
    float dt = GetFrameTime();
    spring_tick(&crosshair_scale, &crosshair_scale_vel, target_scale, dt);
    if (crosshair_scale < 0.5f) crosshair_scale = 0.5f;

    int cx = RENDER_W / 2;
    int cy = RENDER_H / 2;
    float sc = crosshair_scale;
    int s = UI_SCALE;

    // Alpha: brighter when scaled up (targeting something)
    unsigned char base_a = (sc > 1.5f) ? 140 : 70;

    // Center dot
    DrawRectangle(cx - s/2, cy - s/2, s, s, (Color){220, 215, 205, base_a});

    // Cross arms — gap of 2px from center, length scales with spring
    int gap = 3 * s;
    int arm_len = (int)(3 * sc * s + 0.5f);
    unsigned char arm_a = (unsigned char)(base_a * 0.7f);
    Color arm_c = {220, 215, 205, arm_a};

    // Up
    DrawRectangle(cx - s/2, cy - gap - arm_len, s, arm_len, arm_c);
    // Down
    DrawRectangle(cx - s/2, cy + gap, s, arm_len, arm_c);
    // Left
    DrawRectangle(cx - gap - arm_len, cy - s/2, arm_len, s, arm_c);
    // Right
    DrawRectangle(cx + gap, cy - s/2, arm_len, s, arm_c);
}

// ── Compass ────────────────────────────────────────────────────────
// Subtle axis ticks around the crosshair — shows cardinal directions
// N/E/S/W as tiny one-letter labels at compass positions
void ui_draw_compass(Player *player) {
    float dt = GetFrameTime();

    // Get player yaw from camera direction
    Vector3 fwd = Vector3Subtract(player->camera.target, player->camera.position);
    float yaw = atan2f(fwd.x, fwd.z); // radians, 0 = +Z (north)

    // Compass fades in based on movement (subtle when still)
    float speed = sqrtf(player->vel.x * player->vel.x + player->vel.z * player->vel.z);
    float target_alpha = (speed > 0.5f) ? 1.0f : 0.3f;
    spring_tick(&compass_alpha, &compass_alpha_vel, target_alpha, dt);

    int cx = RENDER_W / 2;
    int cy = RENDER_H / 2;
    int radius = 28 * UI_SCALE; // distance from center

    // Cardinal directions
    const char *labels[] = {"N", "E", "S", "W"};
    float angles[] = {0, PI/2, PI, -PI/2}; // world angles (N=+Z, E=+X)

    unsigned char max_a = (unsigned char)(compass_alpha * 80);

    for (int i = 0; i < 4; i++) {
        // Screen position: rotate by negative yaw to get screen-relative
        float screen_angle = angles[i] - yaw;
        float sx = cx + sinf(screen_angle) * radius;
        float sy = cy - cosf(screen_angle) * radius;

        // Only draw if roughly in the top semicircle of the ring
        // (the compass wraps — all 4 ticks always visible)
        int fs = 8 * UI_SCALE;
        int tw = MeasureText(labels[i], fs);

        // N is brighter, others dimmer
        unsigned char a = (i == 0) ? max_a : (unsigned char)(max_a * 0.5f);

        // Tick mark — tiny line pointing inward
        float inward = 3.0f * UI_SCALE;
        float dx = sinf(screen_angle);
        float dy = -cosf(screen_angle);
        DrawLine((int)(sx + dx * inward), (int)(sy + dy * inward),
                 (int)(sx - dx * inward), (int)(sy - dy * inward),
                 (Color){220, 215, 205, a});

        // Letter
        DrawText(labels[i], (int)(sx - tw/2), (int)(sy - fs/2),
                 fs, (Color){220, 215, 205, a});
    }

    // Subtle ring — very faint circle connecting the ticks
    // Draw as discrete dots for a pixelated feel
    int ring_dots = 32;
    unsigned char ring_a = (unsigned char)(compass_alpha * 20);
    for (int i = 0; i < ring_dots; i++) {
        float angle = (2.0f * PI * i) / ring_dots;
        int rx = cx + (int)(sinf(angle) * radius);
        int ry = cy + (int)(-cosf(angle) * radius);
        DrawRectangle(rx, ry, 1, 1, (Color){220, 215, 205, ring_a});
    }
}

// ── Interaction Prompt ─────────────────────────────────────────────
// When targeting an interactable:
//   - Ring expands around crosshair (spring)
//   - Pixel icon appears below
//   - "E" key label bounces in
void ui_draw_interaction(Player *player, Scene *scene) {
    float dt = GetFrameTime();
    int cx = RENDER_W / 2;
    int cy = RENDER_H / 2;

    // Find targeted object
    bool has_target = false;
    const char *target_name = NULL;
    float best_dot = 0.75f;

    for (int i = 0; i < scene->object_count; i++) {
        InteractObject *obj = &scene->objects[i];
        if (!obj->active || obj->done) continue;
        float dist = Vector3Distance(player->camera.position, obj->pos);
        if (dist >= obj->radius) continue;
        Vector3 to_obj = Vector3Normalize(Vector3Subtract(obj->pos, player->camera.position));
        Vector3 look = Vector3Normalize(Vector3Subtract(player->camera.target, player->camera.position));
        float dot = Vector3DotProduct(to_obj, look);
        if (dot > best_dot) {
            best_dot = dot;
            has_target = true;
            target_name = obj->name;
        }
    }

    // Check pushable walls too
    if (!has_target) {
        Vector3 origin = player->camera.position;
        Vector3 dir = Vector3Normalize(Vector3Subtract(player->camera.target, player->camera.position));
        for (int i = 0; i < scene->wall_count; i++) {
            Wall *w = &scene->walls[i];
            if (!w->pushable || !w->active) continue;
            Vector3 to_w = Vector3Subtract(w->pos, origin);
            float along = Vector3DotProduct(to_w, dir);
            if (along < 0 || along > 2.5f) continue;
            Vector3 closest = Vector3Add(origin, Vector3Scale(dir, along));
            float perp = Vector3Distance(closest, w->pos);
            float radius = fmaxf(w->size.x, fmaxf(w->size.y, w->size.z)) * 0.6f;
            if (perp < radius) { has_target = true; break; }
        }
    }

    // Spring the interaction ring
    float ring_target = has_target ? 1.0f : 0.0f;
    spring_tick(&interact_ring, &interact_ring_vel, ring_target, dt);
    if (interact_ring < 0.01f) interact_ring = 0.0f;

    // Spring the prompt scale
    float prompt_target = has_target ? 1.0f : 0.0f;
    spring_tick(&prompt_scale, &prompt_scale_vel, prompt_target, dt);
    if (prompt_scale < 0.01f) prompt_scale = 0.0f;

    if (interact_ring < 0.01f) return; // nothing to draw

    // Interaction ring — expanding arc segments
    unsigned char ring_a = (unsigned char)(interact_ring * 120);
    int ring_r = (int)(12 * UI_SCALE * interact_ring);
    int segments = 4;
    float seg_arc = PI / 4; // each segment is 45 degrees
    float gap_arc = PI / 4; // gap between segments
    float time = (float)GetTime();
    float rotation = time * 0.3f; // slow rotation

    for (int seg = 0; seg < segments; seg++) {
        float start = rotation + seg * (seg_arc + gap_arc);
        int steps = 8;
        for (int j = 0; j < steps; j++) {
            float a = start + (seg_arc * j) / steps;
            int rx = cx + (int)(cosf(a) * ring_r);
            int ry = cy + (int)(sinf(a) * ring_r);
            DrawRectangle(rx, ry, UI_SCALE, UI_SCALE, (Color){248, 245, 238, ring_a});
        }
    }

    // Icon below crosshair
    if (target_name && prompt_scale > 0.1f) {
        unsigned char icon_a = (unsigned char)(220 * prompt_scale);
        ui_draw_pixel_icon(cx, cy + 12 * UI_SCALE, target_name, icon_a);

        // "E" key prompt — small box with letter
        int fs = 9 * UI_SCALE;
        int prompt_y = cy + 24 * UI_SCALE;
        unsigned char pa = (unsigned char)(200 * prompt_scale);

        // Key box background
        int box_w = 12 * UI_SCALE;
        int box_h = 12 * UI_SCALE;
        int box_x = cx - box_w / 2;
        int box_y = prompt_y;
        DrawRectangle(box_x, box_y, box_w, box_h, (Color){20, 18, 16, (unsigned char)(pa * 0.6f)});
        DrawRectangleLines(box_x, box_y, box_w, box_h, (Color){220, 215, 205, (unsigned char)(pa * 0.4f)});

        // Letter
        const char *key = "E";
        int tw = MeasureText(key, fs);
        DrawText(key, cx - tw/2, prompt_y + (box_h - fs) / 2 + 1, fs,
                 (Color){248, 245, 238, pa});
    }
}

// ── Main HUD ───────────────────────────────────────────────────────
void ui_draw_hud(Player *player, Scene *scene) {
    // Determine crosshair scale target from interaction state
    float target_scale = 1.0f;

    // Check objects
    for (int i = 0; i < scene->object_count; i++) {
        InteractObject *obj = &scene->objects[i];
        if (!obj->active || obj->done) continue;
        float dist = Vector3Distance(player->camera.position, obj->pos);
        if (dist >= obj->radius) continue;
        Vector3 to_obj = Vector3Normalize(Vector3Subtract(obj->pos, player->camera.position));
        Vector3 look = Vector3Normalize(Vector3Subtract(player->camera.target, player->camera.position));
        if (Vector3DotProduct(to_obj, look) > 0.75f) {
            target_scale = 2.5f;
            break;
        }
    }

    // Check pushable walls
    if (target_scale < 1.5f) {
        Vector3 origin = player->camera.position;
        Vector3 dir = Vector3Normalize(Vector3Subtract(player->camera.target, player->camera.position));
        for (int i = 0; i < scene->wall_count; i++) {
            Wall *w = &scene->walls[i];
            if (!w->pushable || !w->active) continue;
            Vector3 to_w = Vector3Subtract(w->pos, origin);
            float along = Vector3DotProduct(to_w, dir);
            if (along < 0 || along > 2.5f) continue;
            Vector3 closest = Vector3Add(origin, Vector3Scale(dir, along));
            float perp = Vector3Distance(closest, w->pos);
            float radius = fmaxf(w->size.x, fmaxf(w->size.y, w->size.z)) * 0.6f;
            if (perp < radius) { target_scale = 2.0f; break; }
        }
    }

    // Draw layers: compass (behind), crosshair (center), interaction (front)
    ui_draw_compass(player);
    ui_draw_crosshair(target_scale);
    ui_draw_interaction(player, scene);
}
