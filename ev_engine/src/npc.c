// npc.c — Gibbons
// "He had the air of someone who has been waiting in a hotel lobby for so long
//  that he has become part of the furniture." — after Bolaño
//
// Geometric cube-person. Bellboy uniform. Carries a briefcase he never opens.
// Adjusts his tie while waiting. Tilts his head when the player gets close.
// Walks too fast, then stops and waits with infinite patience.
// He knows more than he lets on.
#include "npc.h"
#include "raymath.h"
#include <math.h>
#include <string.h>

// ── Initialization ──────────────────────────────────────────────────

static void init_common(NPC *npc, Vector3 start, Vector3 *waypoints, int count,
                        float speed, float wait_radius) {
    memset(npc, 0, sizeof(NPC));
    npc->pos = start;
    npc->speed = speed;
    npc->wait_radius = wait_radius;
    npc->active = true;
    npc->waiting = false;
    npc->current_waypoint = 0;
    npc->idle_timer = 0;
    npc->waypoint_count = count > MAX_NPC_WAYPOINTS ? MAX_NPC_WAYPOINTS : count;
    for (int i = 0; i < npc->waypoint_count; i++)
        npc->waypoints[i] = waypoints[i];
    if (count > 0) npc->target_pos = waypoints[0];

    // Gibbons — Bolaño bellboy
    // Dark navy uniform, cream skin, red cap, burgundy tie, leather briefcase
    npc->body_color      = (Color){28, 32, 52, 255};    // deep navy
    npc->head_color      = (Color){215, 195, 165, 255}; // warm skin
    npc->cap_color       = (Color){175, 40, 38, 255};   // Godard red cap
    npc->leg_color       = (Color){22, 25, 42, 255};    // darker navy
    npc->tie_color       = (Color){140, 30, 35, 255};   // burgundy
    npc->briefcase_color = (Color){95, 60, 30, 255};    // worn leather
}

void init_npc(NPC *npc, Vector3 start, Vector3 *waypoints, int count,
              float speed, float wait_radius) {
    init_common(npc, start, waypoints, count, speed, wait_radius);
    npc->use_physics = false;
    npc->ground_y = start.y - 1.6f;  // assume eye height
}

void init_npc_grounded(NPC *npc, Vector3 start, Vector3 *waypoints, int count,
                       float speed, float wait_radius) {
    init_common(npc, start, waypoints, count, speed, wait_radius);
    npc->use_physics = true;
    npc->ground_y = 0;
    npc->vy = 0;
    npc->grounded = false;
    npc->collision_radius = 0.22f;
    // Start pos is eye-level; actual feet pos derived from ground_y
}

void npc_set_dialogue(NPC *npc, const char **lines, int count, float duration) {
    npc->lines = lines;
    npc->line_count = count;
    npc->current_line = 0;
    npc->line_timer = 0;
    npc->line_duration = duration > 0 ? duration : 3.0f;
    npc->line_showing = false;
}

const char *npc_current_dialogue(NPC *npc) {
    if (!npc->active || !npc->line_showing) return NULL;
    if (npc->current_line >= npc->line_count) return NULL;
    return npc->lines[npc->current_line];
}

// ── Physics ─────────────────────────────────────────────────────────

static void npc_apply_gravity(NPC *npc, float dt) {
    if (!npc->use_physics) return;
    float gravity = 18.0f;
    npc->vy -= gravity * dt;
    npc->pos.y += npc->vy * dt;

    // Floor at ground_y
    float feet_y = npc->ground_y;
    if (npc->pos.y <= feet_y) {
        npc->pos.y = feet_y;
        npc->vy = 0;
        npc->grounded = true;
    }
}

static void npc_find_ground(NPC *npc, Scene *scene) {
    if (!npc->use_physics || !scene) return;
    float best_y = 0;  // default floor
    float pr = npc->collision_radius;

    for (int i = 0; i < scene->wall_count; i++) {
        Wall *w = &scene->walls[i];
        if (!w->active || w->shape != SHAPE_CUBE) continue;
        float top = w->pos.y + w->size.y / 2;
        if (top < npc->pos.y + 0.1f && top > best_y &&
            top <= npc->ground_y + 0.6f &&
            npc->pos.x > w->pos.x - w->size.x/2 - pr &&
            npc->pos.x < w->pos.x + w->size.x/2 + pr &&
            npc->pos.z > w->pos.z - w->size.z/2 - pr &&
            npc->pos.z < w->pos.z + w->size.z/2 + pr) {
            best_y = top;
        }
    }
    // Smooth step-up
    float spd = (best_y > npc->ground_y) ? 12.0f : 8.0f;
    float diff = best_y - npc->ground_y;
    float step = diff * fminf(1.0f, spd * 0.016f);
    npc->ground_y += step;
}

static void npc_collide_walls(NPC *npc, Scene *scene) {
    if (!npc->use_physics || !scene) return;
    float pr = npc->collision_radius;

    for (int i = 0; i < scene->wall_count; i++) {
        Wall *w = &scene->walls[i];
        if (!w->active || w->shape != SHAPE_CUBE) continue;
        float wall_top = w->pos.y + w->size.y / 2;
        float wall_bot = w->pos.y - w->size.y / 2;
        // Skip step-over-able walls
        if (wall_top <= npc->ground_y + 0.5f && wall_top > npc->ground_y - 0.1f) continue;

        if (npc->pos.x > w->pos.x - w->size.x/2 - pr &&
            npc->pos.x < w->pos.x + w->size.x/2 + pr &&
            npc->pos.z > w->pos.z - w->size.z/2 - pr &&
            npc->pos.z < w->pos.z + w->size.z/2 + pr &&
            npc->pos.y > wall_bot && npc->pos.y < wall_top) {

            float dx_l = (w->pos.x - w->size.x/2 - pr) - npc->pos.x;
            float dx_r = npc->pos.x - (w->pos.x + w->size.x/2 + pr);
            float dz_f = (w->pos.z - w->size.z/2 - pr) - npc->pos.z;
            float dz_b = npc->pos.z - (w->pos.z + w->size.z/2 + pr);
            float pen_x = (fabsf(dx_l) < fabsf(dx_r)) ? dx_l : -dx_r;
            float pen_z = (fabsf(dz_f) < fabsf(dz_b)) ? dz_f : -dz_b;

            if (fabsf(pen_x) < fabsf(pen_z)) {
                npc->pos.x += pen_x;
            } else {
                npc->pos.z += pen_z;
            }
        }
    }
}

// ── Update ──────────────────────────────────────────────────────────

void update_npc(NPC *npc, Vector3 player_pos, Scene *scene, float dt) {
    if (!npc->active) return;
    if (npc->current_waypoint >= npc->waypoint_count) {
        npc->active = false;
        return;
    }

    Vector3 target = npc->waypoints[npc->current_waypoint];
    // XZ distance only (ignore vertical for waypoint arrival)
    float dx = target.x - npc->pos.x;
    float dz = target.z - npc->pos.z;
    float dist = sqrtf(dx*dx + dz*dz);

    if (dist < 0.3f) {
        npc->waiting = true;
        npc->pos.x = target.x;
        npc->pos.z = target.z;
    }

    if (npc->waiting) {
        npc->idle_timer += dt;

        // Dialogue — deliver a line after arriving at waypoint
        if (npc->lines && npc->current_line < npc->line_count) {
            if (!npc->line_showing && npc->idle_timer > 0.5f) {
                // Start showing current line after brief pause
                npc->line_showing = true;
                npc->line_timer = 0;
            }
            if (npc->line_showing) {
                npc->line_timer += dt;
                if (npc->line_timer > npc->line_duration) {
                    // Line finished — advance to next
                    npc->line_showing = false;
                    npc->current_line++;
                }
            }
        }

        // Face toward the player — smoothly
        Vector3 to_player = Vector3Subtract(player_pos, npc->pos);
        npc->yaw_target = atan2f(to_player.x, to_player.z);
        float yaw_diff = npc->yaw_target - npc->yaw;
        while (yaw_diff > 3.14159f) yaw_diff -= 6.28318f;
        while (yaw_diff < -3.14159f) yaw_diff += 6.28318f;
        npc->yaw += yaw_diff * fminf(1.0f, 5.0f * dt);

        // Player proximity → advance (only after dialogue finishes or no dialogue)
        float player_dist = Vector3Length(to_player);
        bool dialogue_done = !npc->lines || npc->current_line >= npc->line_count || !npc->line_showing;
        if (player_dist < npc->wait_radius && dialogue_done) {
            npc->current_waypoint++;
            npc->waiting = false;
            npc->idle_timer = 0;
            if (npc->current_waypoint >= npc->waypoint_count) {
                npc->active = false;
                return;
            }
            npc->target_pos = npc->waypoints[npc->current_waypoint];
        }
    } else {
        npc->idle_timer = 0;
        // Move toward waypoint (XZ only, let physics handle Y)
        float dir_x = dx / (dist + 0.001f);
        float dir_z = dz / (dist + 0.001f);
        float move = npc->speed * dt;
        if (move > dist) move = dist;
        npc->pos.x += dir_x * move;
        npc->pos.z += dir_z * move;

        // Face movement direction — smooth
        npc->yaw_target = atan2f(dir_x, dir_z);
        float yaw_diff = npc->yaw_target - npc->yaw;
        while (yaw_diff > 3.14159f) yaw_diff -= 6.28318f;
        while (yaw_diff < -3.14159f) yaw_diff += 6.28318f;
        npc->yaw += yaw_diff * fminf(1.0f, 8.0f * dt);

        // Walk cycle
        npc->bob_timer += dt * npc->speed * 2.5f;
    }

    // Physics
    if (npc->use_physics && scene) {
        npc_find_ground(npc, scene);
        npc_apply_gravity(npc, dt);
        npc_collide_walls(npc, scene);
    }
}

// ── Drawing ─────────────────────────────────────────────────────────
// 10x the original — shoulders, arms, tie, briefcase, proper proportions.
// Think: the little geometric people in Gravity Bone, but dressed for a
// hotel that exists between reality and orbit.

void draw_npc(NPC *npc, Model *cube_model, Model *cyl_model __attribute__((unused)),
              EVLighting *lighting) {
    if (!npc->active) return;

    float t = npc->bob_timer;
    float idle = npc->idle_timer;
    bool walking = !npc->waiting;

    // Base position — feet on ground_y (physics) or pos.y (ghost)
    float base_y = npc->use_physics ? npc->ground_y : (npc->pos.y - 1.6f);
    float yaw = npc->yaw;
    float cy = cosf(yaw), sy = sinf(yaw);

    // Walk bob — body rises and falls with stride
    float walk_bob = walking ? sinf(t) * 0.04f : 0;
    // Idle fidget — Bolaño: he shifts his weight, adjusts his tie
    float idle_sway = sinf(idle * 0.8f) * 0.01f;
    float idle_tilt = sinf(idle * 1.3f) * 1.5f;  // degrees — head tilt

    // ── Proportions (Gravity Bone scale — blocky, readable at 480x300) ──
    float leg_h = 0.42f, leg_w = 0.10f;
    float body_w = 0.34f, body_h = 0.48f, body_d = 0.20f;
    float head_s = 0.22f;
    float cap_w = 0.26f, cap_h = 0.07f;
    float shoulder_w = 0.42f, shoulder_h = 0.08f;
    float arm_w = 0.08f, arm_h = 0.36f;
    float tie_w = 0.06f, tie_h = 0.22f;
    float case_w = 0.20f, case_h = 0.14f, case_d = 0.06f;

    // ── Derived positions ──
    float foot_y = base_y;
    float hip_y = foot_y + leg_h;
    float torso_y = hip_y + body_h / 2;
    float shoulder_y = hip_y + body_h;
    float neck_y = shoulder_y + 0.02f;
    float head_y = neck_y + head_s / 2;
    float cap_y = neck_y + head_s + cap_h / 2;

    // Walk cycle offsets
    float leg_swing = walking ? sinf(t * 1.5f) * 0.15f : 0;
    float arm_swing = walking ? sinf(t * 1.5f) * 0.10f : 0;
    // Idle: left arm holds briefcase at side, right arm adjusts tie periodically
    float tie_adjust = 0;
    if (!walking && idle > 2.0f) {
        // Every ~4 seconds, reach up and adjust tie (Kaufman fidget)
        float cycle = fmodf(idle - 2.0f, 4.0f);
        if (cycle < 0.8f) {
            tie_adjust = sinf(cycle / 0.8f * 3.14159f) * 0.12f;
        }
    }

    // Helper: offset position by yaw-rotated local coords
    #define NPC_POS(lx, ly, lz) (Vector3){ \
        npc->pos.x + (lx)*cy - (lz)*sy, \
        ly + walk_bob + idle_sway, \
        npc->pos.z + (lx)*sy + (lz)*cy }

    // ── LEGS ──
    cube_model->materials[0].maps[MATERIAL_MAP_DIFFUSE].color = npc->leg_color;
    SetMaterialId(lighting, 9);  // MAT_FABRIC
    float leg_spread = body_w * 0.3f;
    // Left leg
    DrawModelEx(*cube_model,
        NPC_POS(-leg_spread, foot_y + leg_h/2 + leg_swing * 0.4f, 0),
        (Vector3){0,1,0}, yaw * RAD2DEG,
        (Vector3){leg_w, leg_h, leg_w}, WHITE);
    // Right leg
    DrawModelEx(*cube_model,
        NPC_POS(leg_spread, foot_y + leg_h/2 - leg_swing * 0.4f, 0),
        (Vector3){0,1,0}, yaw * RAD2DEG,
        (Vector3){leg_w, leg_h, leg_w}, WHITE);

    // ── BODY (torso) ──
    cube_model->materials[0].maps[MATERIAL_MAP_DIFFUSE].color = npc->body_color;
    SetMaterialId(lighting, 9);
    DrawModelEx(*cube_model,
        NPC_POS(0, torso_y, 0),
        (Vector3){0,1,0}, yaw * RAD2DEG,
        (Vector3){body_w, body_h, body_d}, WHITE);

    // ── SHOULDERS ──
    DrawModelEx(*cube_model,
        NPC_POS(0, shoulder_y - shoulder_h/2, 0),
        (Vector3){0,1,0}, yaw * RAD2DEG,
        (Vector3){shoulder_w, shoulder_h, body_d}, WHITE);

    // ── TIE — burgundy strip on chest ──
    cube_model->materials[0].maps[MATERIAL_MAP_DIFFUSE].color = npc->tie_color;
    SetMaterialId(lighting, 9);
    DrawModelEx(*cube_model,
        NPC_POS(0, torso_y + 0.05f, -body_d/2 - 0.005f),
        (Vector3){0,1,0}, yaw * RAD2DEG,
        (Vector3){tie_w, tie_h, 0.02f}, WHITE);

    // ── ARMS ──
    cube_model->materials[0].maps[MATERIAL_MAP_DIFFUSE].color = npc->body_color;
    SetMaterialId(lighting, 9);
    float arm_x = shoulder_w/2 + arm_w/2;
    // Left arm — holds briefcase, swings less
    float l_arm_swing = walking ? arm_swing * 0.3f : 0;
    DrawModelEx(*cube_model,
        NPC_POS(-arm_x, shoulder_y - arm_h/2 + l_arm_swing, 0),
        (Vector3){0,1,0}, yaw * RAD2DEG,
        (Vector3){arm_w, arm_h, arm_w}, WHITE);
    // Right arm — adjusts tie when idle, swings when walking
    float r_arm_y = shoulder_y - arm_h/2;
    if (tie_adjust > 0) r_arm_y += tie_adjust;  // reaches up
    float r_arm_swing = walking ? -arm_swing : 0;
    DrawModelEx(*cube_model,
        NPC_POS(arm_x, r_arm_y + r_arm_swing, 0),
        (Vector3){0,1,0}, yaw * RAD2DEG,
        (Vector3){arm_w, arm_h, arm_w}, WHITE);

    // ── BRIEFCASE — left hand, swings with walk ──
    cube_model->materials[0].maps[MATERIAL_MAP_DIFFUSE].color = npc->briefcase_color;
    SetMaterialId(lighting, 8);  // MAT_LEATHER
    float case_y = shoulder_y - arm_h - case_h/2 + 0.02f + l_arm_swing;
    DrawModelEx(*cube_model,
        NPC_POS(-arm_x - 0.02f, case_y, 0),
        (Vector3){0,1,0}, yaw * RAD2DEG,
        (Vector3){case_d, case_h, case_w}, WHITE);
    // Brass latch on briefcase
    cube_model->materials[0].maps[MATERIAL_MAP_DIFFUSE].color = (Color){178, 155, 107, 255};
    SetMaterialId(lighting, 6);  // MAT_BRASS
    DrawModelEx(*cube_model,
        NPC_POS(-arm_x - 0.02f, case_y + case_h * 0.3f, -case_w/2 - 0.005f),
        (Vector3){0,1,0}, yaw * RAD2DEG,
        (Vector3){0.02f, 0.03f, 0.02f}, WHITE);

    // ── HEAD ──
    cube_model->materials[0].maps[MATERIAL_MAP_DIFFUSE].color = npc->head_color;
    SetMaterialId(lighting, 0);  // MAT_CONCRETE (skin)
    // Head tilts when idle (Bolaño: studying you, deciding something)
    DrawModelEx(*cube_model,
        NPC_POS(0, head_y, 0),
        (Vector3){0,1,0}, (yaw * RAD2DEG) + idle_tilt,
        (Vector3){head_s, head_s, head_s}, WHITE);

    // ── BELLBOY CAP — Godard red ──
    cube_model->materials[0].maps[MATERIAL_MAP_DIFFUSE].color = npc->cap_color;
    SetMaterialId(lighting, 9);
    DrawModelEx(*cube_model,
        NPC_POS(0, cap_y, 0),
        (Vector3){0,1,0}, (yaw * RAD2DEG) + idle_tilt,
        (Vector3){cap_w, cap_h, cap_w}, WHITE);
    // Cap brim — slightly wider, thin
    DrawModelEx(*cube_model,
        NPC_POS(0, cap_y - cap_h/2, -head_s * 0.15f),
        (Vector3){0,1,0}, (yaw * RAD2DEG) + idle_tilt,
        (Vector3){cap_w + 0.04f, 0.02f, cap_w * 0.6f}, WHITE);

    // ── BUTTONS — two brass dots on chest ──
    cube_model->materials[0].maps[MATERIAL_MAP_DIFFUSE].color = (Color){178, 155, 107, 255};
    SetMaterialId(lighting, 6);  // MAT_BRASS
    DrawModelEx(*cube_model,
        NPC_POS(0.06f, torso_y + 0.10f, -body_d/2 - 0.005f),
        (Vector3){0,1,0}, yaw * RAD2DEG,
        (Vector3){0.03f, 0.03f, 0.01f}, WHITE);
    DrawModelEx(*cube_model,
        NPC_POS(0.06f, torso_y - 0.06f, -body_d/2 - 0.005f),
        (Vector3){0,1,0}, yaw * RAD2DEG,
        (Vector3){0.03f, 0.03f, 0.01f}, WHITE);

    #undef NPC_POS
}
