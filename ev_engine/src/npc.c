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
    // HIGH CONTRAST for 480x300: bright cap, visible skin, navy body pops against marble
    npc->body_color      = (Color){35, 40, 65, 255};    // navy — lighter than before
    npc->head_color      = (Color){230, 210, 175, 255}; // BRIGHT warm skin — must read
    npc->cap_color       = (Color){200, 50, 42, 255};   // VIVID Godard red — the beacon
    npc->leg_color       = (Color){30, 34, 55, 255};    // navy
    npc->tie_color       = (Color){195, 45, 40, 255};   // red — matches cap, visible
    npc->briefcase_color = (Color){160, 110, 50, 255};  // GOLDEN brown — catches light
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
        bool dialogue_done = !npc->lines || npc->current_line >= npc->line_count;
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
// High fidelity cube-person: segmented limbs, facial features, clothing
// detail. Every piece hand-placed for 480x300 readability.

void draw_npc(NPC *npc, Model *cube_model, Model *cyl_model,
              EVLighting *lighting) {
    if (!npc->active) return;

    float t = npc->bob_timer;
    float idle = npc->idle_timer;
    bool walking = !npc->waiting;
    float base_y = npc->use_physics ? npc->ground_y : (npc->pos.y - 1.6f);
    float yaw = npc->yaw;
    float cy = cosf(yaw), sy = sinf(yaw);

    float walk_bob = walking ? sinf(t) * 0.05f : 0;
    float idle_sway = sinf(idle * 0.8f) * 0.015f;
    float idle_tilt = sinf(idle * 1.3f) * 2.5f;

    // ── Proportions — total ~1.72m ──
    float shoe_h = 0.06f, shoe_w = 0.18f, shoe_d = 0.22f;
    float shin_h = 0.28f, shin_w = 0.15f;
    float thigh_h = 0.28f, thigh_w = 0.17f;
    float body_w = 0.46f, body_h = 0.50f, body_d = 0.28f;
    float shoulder_w = 0.58f, shoulder_h = 0.10f;
    float upper_arm_h = 0.24f, upper_arm_w = 0.13f;
    float forearm_h = 0.22f, forearm_w = 0.11f;
    float hand_s = 0.08f;
    float head_w = 0.26f, head_h = 0.30f, head_d = 0.24f;
    float cap_w = 0.32f, cap_h = 0.08f;
    float neck_w = 0.12f, neck_h = 0.06f;
    float tie_w = 0.09f, tie_h = 0.26f;
    float lapel_w = 0.06f, lapel_h = 0.30f;
    float pocket_w = 0.10f, pocket_h = 0.06f;
    float belt_h = 0.05f;
    float case_w = 0.28f, case_h = 0.20f, case_d = 0.10f;

    // ── Derived Y positions ──
    float foot_y = base_y;
    float ankle_y = foot_y + shoe_h;
    float knee_y = ankle_y + shin_h;
    float hip_y = knee_y + thigh_h;
    float torso_mid = hip_y + body_h / 2;
    float shoulder_y = hip_y + body_h;
    float neck_y2 = shoulder_y + shoulder_h;
    float head_y = neck_y2 + neck_h + head_h / 2;
    float cap_y = neck_y2 + neck_h + head_h + cap_h / 2;

    // Walk cycle
    float phase = t * 1.5f;
    float leg_swing = walking ? sinf(phase) * 0.18f : 0;
    float arm_swing = walking ? sinf(phase) * 0.12f : 0;
    float tie_adjust = 0;
    if (!walking && idle > 2.0f) {
        float cycle = fmodf(idle - 2.0f, 4.0f);
        if (cycle < 0.8f) tie_adjust = sinf(cycle / 0.8f * 3.14159f) * 0.14f;
    }

    #define P(lx, ly, lz) (Vector3){ \
        npc->pos.x + (lx)*cy - (lz)*sy, \
        (ly) + walk_bob + idle_sway, \
        npc->pos.z + (lx)*sy + (lz)*cy }
    // D macro: set color + material for next draw. Use DC() to wrap compound Color literals.
    #define DC(...) ((Color){__VA_ARGS__})
    #define D(m,c,mat) cube_model->materials[0].maps[MATERIAL_MAP_DIFFUSE].color=(c); SetMaterialId(lighting,(mat))
    #define DRAW(px,py,pz, sx,sy2,sz) DrawModelEx(*cube_model, P(px,py,pz), \
        (Vector3){0,1,0}, yaw*RAD2DEG, (Vector3){sx,sy2,sz}, WHITE)
    #define DRAW_TILT(px,py,pz, sx,sy2,sz) DrawModelEx(*cube_model, P(px,py,pz), \
        (Vector3){0,1,0}, yaw*RAD2DEG+idle_tilt, (Vector3){sx,sy2,sz}, WHITE)
    // Cylinder helper
    #define DRAWCYL(px,py,pz, diam,h) DrawModelEx(*cyl_model, P(px,py,pz), \
        (Vector3){0,1,0}, yaw*RAD2DEG, (Vector3){diam,h,diam}, WHITE)

    float ls = body_w * 0.28f;  // leg spread from center

    // ── SHOES — dark, distinct from legs ──
    D(m, DC(35, 30, 25, 255),8);  // MAT_LEATHER
    DRAW(-ls, foot_y + shoe_h/2 + leg_swing*0.3f, -0.02f, shoe_w, shoe_h, shoe_d);
    DRAW( ls, foot_y + shoe_h/2 - leg_swing*0.3f,  0.02f, shoe_w, shoe_h, shoe_d);

    // ── SHINS (lower leg) ──
    D(m, npc->leg_color, 9);
    DRAW(-ls, ankle_y + shin_h/2 + leg_swing*0.4f, 0, shin_w, shin_h, shin_w);
    DRAW( ls, ankle_y + shin_h/2 - leg_swing*0.4f, 0, shin_w, shin_h, shin_w);

    // ── THIGHS (upper leg) ──
    D(m, npc->leg_color, 9);
    DRAW(-ls, knee_y + thigh_h/2 + leg_swing*0.2f, 0, thigh_w, thigh_h, thigh_w);
    DRAW( ls, knee_y + thigh_h/2 - leg_swing*0.2f, 0, thigh_w, thigh_h, thigh_w);

    // ── BELT ──
    D(m, DC(45, 38, 28, 255),8);  // leather
    DRAW(0, hip_y + belt_h/2, 0, body_w + 0.02f, belt_h, body_d + 0.01f);
    // Belt buckle — brass
    D(m, DC(210, 180, 100, 255),6);
    DRAW(0, hip_y + belt_h/2, -body_d/2 - 0.01f, 0.06f, 0.05f, 0.02f);

    // ── TORSO ──
    D(m, npc->body_color, 9);
    DRAW(0, torso_mid, 0, body_w, body_h, body_d);

    // ── LAPELS — lighter navy V on chest ──
    D(m, DC(50, 55, 80, 255),9);
    DRAW(-body_w/2 + lapel_w/2 + 0.02f, torso_mid + 0.08f, -body_d/2 - 0.005f,
         lapel_w, lapel_h, 0.02f);
    DRAW( body_w/2 - lapel_w/2 - 0.02f, torso_mid + 0.08f, -body_d/2 - 0.005f,
         lapel_w, lapel_h, 0.02f);

    // ── TIE — red, between lapels ──
    D(m, npc->tie_color, 9);
    DRAW(0, torso_mid + 0.06f, -body_d/2 - 0.008f, tie_w, tie_h, 0.02f);

    // ── POCKET SQUARE — white, left breast ──
    D(m, DC(240, 238, 232, 255),9);
    DRAW(-body_w/2 + 0.10f, shoulder_y - 0.10f, -body_d/2 - 0.005f,
         pocket_w, pocket_h, 0.015f);

    // ── SHOULDERS ──
    D(m, npc->body_color, 9);
    DRAW(0, shoulder_y + shoulder_h/2, 0, shoulder_w, shoulder_h, body_d);

    // ── BUTTONS — two rows of two, brass ──
    D(m, DC(210, 180, 100, 255),6);
    DRAW( 0.08f, torso_mid + 0.14f, -body_d/2 - 0.01f, 0.045f, 0.045f, 0.02f);
    DRAW( 0.08f, torso_mid - 0.06f, -body_d/2 - 0.01f, 0.045f, 0.045f, 0.02f);
    DRAW(-0.08f, torso_mid + 0.14f, -body_d/2 - 0.01f, 0.045f, 0.045f, 0.02f);
    DRAW(-0.08f, torso_mid - 0.06f, -body_d/2 - 0.01f, 0.045f, 0.045f, 0.02f);

    // ── ARMS — segmented: upper arm + forearm ──
    float arm_x = shoulder_w/2 + upper_arm_w/2;
    float l_sw = walking ? arm_swing * 0.3f : 0;
    float r_sw = walking ? -arm_swing : 0;

    // Left upper arm
    D(m, npc->body_color, 9);
    DRAW(-arm_x, shoulder_y - upper_arm_h/2 + l_sw, 0, upper_arm_w, upper_arm_h, upper_arm_w);
    // Left forearm — slightly lighter (shirt cuff showing)
    D(m, DC(45, 50, 72, 255),9);
    float l_elbow_y = shoulder_y - upper_arm_h + l_sw;
    DRAW(-arm_x, l_elbow_y - forearm_h/2, 0, forearm_w, forearm_h, forearm_w);
    // Left hand — skin
    D(m, npc->head_color, 0);
    float l_hand_y = l_elbow_y - forearm_h;
    DRAW(-arm_x, l_hand_y - hand_s/2, 0, hand_s, hand_s, hand_s);

    // Right upper arm
    D(m, npc->body_color, 9);
    float r_off = tie_adjust > 0 ? tie_adjust : 0;
    DRAW(arm_x, shoulder_y - upper_arm_h/2 + r_sw + r_off, 0,
         upper_arm_w, upper_arm_h, upper_arm_w);
    // Right forearm
    D(m, DC(45, 50, 72, 255),9);
    float r_elbow_y = shoulder_y - upper_arm_h + r_sw + r_off;
    DRAW(arm_x, r_elbow_y - forearm_h/2, 0, forearm_w, forearm_h, forearm_w);
    // Right hand
    D(m, npc->head_color, 0);
    float r_hand_y = r_elbow_y - forearm_h;
    DRAW(arm_x, r_hand_y - hand_s/2, 0, hand_s, hand_s, hand_s);

    // ── BRIEFCASE — left hand ──
    D(m, npc->briefcase_color, 8);
    float case_y2 = l_hand_y - hand_s - case_h/2;
    DRAW(-arm_x, case_y2, 0, case_d, case_h, case_w);
    // Brass latch
    D(m, DC(210, 180, 100, 255),6);
    DRAW(-arm_x, case_y2 + case_h * 0.3f, -case_w/2 - 0.01f, 0.04f, 0.05f, 0.03f);
    // Handle — dark strip on top
    D(m, DC(60, 45, 25, 255),8);
    DRAW(-arm_x, case_y2 + case_h/2 + 0.02f, 0, 0.03f, 0.03f, 0.12f);

    // ── NECK — cylinder ──
    D(m, npc->head_color, 0);
    DRAWCYL(0, neck_y2 + neck_h/2, 0, neck_w, neck_h);

    // ── HEAD ──
    D(m, npc->head_color, 0);
    DRAW_TILT(0, head_y, 0, head_w, head_h, head_d);

    // ── EYES — two dark cubes on face ──
    D(m, DC(25, 22, 18, 255),0);
    float eye_y = head_y + head_h * 0.1f;
    float eye_x = head_w * 0.22f;
    float eye_z = -head_d/2 - 0.005f;
    DRAW_TILT(-eye_x, eye_y, eye_z, 0.05f, 0.04f, 0.02f);
    DRAW_TILT( eye_x, eye_y, eye_z, 0.05f, 0.04f, 0.02f);

    // ── MOUTH — thin dark line ──
    D(m, DC(140, 100, 75, 255),0);
    DRAW_TILT(0, head_y - head_h * 0.18f, eye_z, 0.08f, 0.02f, 0.02f);

    // ── BELLBOY CAP ──
    D(m, npc->cap_color, 9);
    DRAW_TILT(0, cap_y, 0, cap_w, cap_h, cap_w);
    // Brim — extends forward
    DRAW_TILT(0, cap_y - cap_h * 0.4f, -head_d * 0.25f,
              cap_w + 0.06f, 0.025f, cap_w * 0.55f);
    // Cap band — gold trim
    D(m, DC(210, 180, 100, 255),6);
    DRAW_TILT(0, cap_y - cap_h * 0.35f, 0, cap_w + 0.01f, 0.025f, cap_w + 0.01f);

    #undef P
    #undef D
    #undef DRAW
    #undef DRAW_TILT
    #undef DRAWCYL
}
