// npc.c — Gibbons: geometric cube-person NPC
// Walks waypoint paths, waits for player, leads without words.
// Gravity Bone / Thirty Flights of Loving — architecture guides, NPCs confirm.
#include "npc.h"
#include "raymath.h"
#include <math.h>
#include <string.h>

void init_npc(NPC *npc, Vector3 start, Vector3 *waypoints, int count,
              float speed, float wait_radius) {
    memset(npc, 0, sizeof(NPC));
    npc->pos = start;
    npc->speed = speed;
    npc->wait_radius = wait_radius;
    npc->active = true;
    npc->waiting = false;
    npc->current_waypoint = 0;
    npc->waypoint_count = count > MAX_NPC_WAYPOINTS ? MAX_NPC_WAYPOINTS : count;
    for (int i = 0; i < npc->waypoint_count; i++) {
        npc->waypoints[i] = waypoints[i];
    }
    if (count > 0) {
        npc->target_pos = waypoints[0];
    }
    // Bellboy uniform — dark navy body, cream head, red cap
    npc->body_color = (Color){30, 35, 55, 255};
    npc->head_color = (Color){210, 190, 160, 255};
    npc->cap_color = (Color){180, 45, 40, 255};
    npc->leg_color = (Color){25, 28, 45, 255};
}

void update_npc(NPC *npc, Vector3 player_pos, float dt) {
    if (!npc->active) return;
    if (npc->current_waypoint >= npc->waypoint_count) {
        npc->active = false;
        return;
    }

    Vector3 target = npc->waypoints[npc->current_waypoint];
    Vector3 to_target = Vector3Subtract(target, npc->pos);
    float dist = Vector3Length(to_target);

    if (dist < 0.3f) {
        // Arrived at waypoint — wait for player
        npc->waiting = true;
        npc->pos = target;
    }

    if (npc->waiting) {
        // Face toward the player while waiting
        Vector3 to_player = Vector3Subtract(player_pos, npc->pos);
        npc->yaw = atan2f(to_player.x, to_player.z);

        // Check if player is close enough to advance
        float player_dist = Vector3Length(to_player);
        if (player_dist < npc->wait_radius) {
            npc->current_waypoint++;
            npc->waiting = false;
            if (npc->current_waypoint >= npc->waypoint_count) {
                npc->active = false;
                return;
            }
            npc->target_pos = npc->waypoints[npc->current_waypoint];
        }
        return;
    }

    // Move toward waypoint
    Vector3 dir = Vector3Normalize(to_target);
    float move = npc->speed * dt;
    if (move > dist) move = dist;
    npc->pos = Vector3Add(npc->pos, Vector3Scale(dir, move));

    // Face movement direction
    npc->yaw = atan2f(dir.x, dir.z);

    // Walk cycle
    npc->bob_timer += dt * npc->speed * 2.5f;
}

void draw_npc(NPC *npc, Model *cube_model, Model *cyl_model __attribute__((unused)), EVLighting *lighting) {
    if (!npc->active) return;

    float bob = sinf(npc->bob_timer) * 0.03f;
    Vector3 p = npc->pos;
    p.y += bob;

    // Scale: ~1.2m tall total (small bellboy, not imposing)
    float body_w = 0.28f, body_h = 0.45f, body_d = 0.18f;
    float head_s = 0.18f;
    float leg_w = 0.08f, leg_h = 0.35f;
    float cap_w = 0.22f, cap_h = 0.06f;

    // Walk cycle — legs swing
    float leg_swing = npc->waiting ? 0 : sinf(npc->bob_timer * 1.5f) * 0.12f;

    // Body
    cube_model->materials[0].maps[MATERIAL_MAP_DIFFUSE].color = npc->body_color;
    SetMaterialId(lighting, 9);  // MAT_FABRIC
    DrawModelEx(*cube_model,
        (Vector3){p.x, p.y + leg_h + body_h/2, p.z},
        (Vector3){0,1,0}, npc->yaw * RAD2DEG,
        (Vector3){body_w, body_h, body_d}, WHITE);

    // Head
    cube_model->materials[0].maps[MATERIAL_MAP_DIFFUSE].color = npc->head_color;
    SetMaterialId(lighting, 0);  // MAT_CONCRETE (skin-ish)
    DrawModelEx(*cube_model,
        (Vector3){p.x, p.y + leg_h + body_h + head_s/2 + 0.02f, p.z},
        (Vector3){0,1,0}, npc->yaw * RAD2DEG,
        (Vector3){head_s, head_s, head_s}, WHITE);

    // Bellboy cap — red, sits on top of head
    cube_model->materials[0].maps[MATERIAL_MAP_DIFFUSE].color = npc->cap_color;
    SetMaterialId(lighting, 9);  // MAT_FABRIC
    DrawModelEx(*cube_model,
        (Vector3){p.x, p.y + leg_h + body_h + head_s + cap_h/2 + 0.02f, p.z},
        (Vector3){0,1,0}, npc->yaw * RAD2DEG,
        (Vector3){cap_w, cap_h, cap_w}, WHITE);

    // Left leg
    cube_model->materials[0].maps[MATERIAL_MAP_DIFFUSE].color = npc->leg_color;
    SetMaterialId(lighting, 9);
    float lx_off = body_w * 0.3f;
    DrawModelEx(*cube_model,
        (Vector3){p.x - lx_off * cosf(npc->yaw),
                  p.y + leg_h/2 + leg_swing * 0.5f,
                  p.z - lx_off * sinf(npc->yaw)},
        (Vector3){0,1,0}, npc->yaw * RAD2DEG,
        (Vector3){leg_w, leg_h, leg_w}, WHITE);

    // Right leg
    DrawModelEx(*cube_model,
        (Vector3){p.x + lx_off * cosf(npc->yaw),
                  p.y + leg_h/2 - leg_swing * 0.5f,
                  p.z + lx_off * sinf(npc->yaw)},
        (Vector3){0,1,0}, npc->yaw * RAD2DEG,
        (Vector3){leg_w, leg_h, leg_w}, WHITE);
}
