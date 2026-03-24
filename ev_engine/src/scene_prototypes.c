#include "game_ctx.h"
#include "raymath.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

void load_state(GameState s);
void set_exposure(float exp);
void show_text(const char *text);
void hide_text(void);

typedef struct {
    PrototypeId id;
    GameState state;
    const char *title;
    const char *subtitle;
    const char *objective;
    PrototypeQAExpectation qa;
} PrototypeSlice;

typedef enum {
    SHOOTER_TARGET_STANDARD = 0,
    SHOOTER_TARGET_ARMORED,
    SHOOTER_TARGET_THREAT,
} ShooterTargetType;

static const PrototypeSlice prototype_slices[] = {
    {
        .id = PROTOTYPE_MOVEMENT,
        .state = STATE_PROTO_MOVEMENT,
        .title = "MOVEMENT / PARKOUR",
        .subtitle = "Safe line, fast line, mastery line. Rerun to clean the route.",
        .objective = "Read the lane, trigger the route nodes, and hit the finish while the window is live.",
        .qa = {
            .display_name = "Movement / Parkour",
            .core_question = "Does rerunning the route immediately feel better than the first clear?",
            .allowed_verbs = "Sprint, jump, dash, wall-run, recover, finish-window routing",
            .success_condition = "First run reads quickly, second run gets cleaner, mistakes become scrambles not confusion.",
            .score_fields = "time, resets, distance, jumps, dashes, route nodes, shortcuts, recoveries, finish cleanliness",
            .recommended_session_length = 4,
            .max_resets = 6,
            .max_completion_time = 40.0f,
            .min_distance = 35.0f,
            .min_jumps = 2,
            .min_dashes = 1
        },
    },
    {
        .id = PROTOTYPE_SHOOTER,
        .state = STATE_PROTO_SHOOTER,
        .title = "SHOOTER",
        .subtitle = "Hook anchors, bank pulse shots, and breach what direct fire cannot.",
        .objective = "Use MMB grapple, LMB pulse, RMB breach, and route through pressure with a bank-shot line.",
        .qa = {
            .display_name = "Shooter",
            .core_question = "Does the encounter feel like a route you execute, not a room you slowly clear?",
            .allowed_verbs = "Pulse fire, breach fire, authored grapple anchors, bank shots, dash, coolant, breach recharge",
            .success_condition = "Combat stays fused to locomotion and the player can describe a repeatable anchor-and-bank route.",
            .score_fields = "time, resets, distance, grapples, bank hits, direct hits, breach kills, armored kills, exposure hits",
            .recommended_session_length = 5,
            .max_resets = 6,
            .max_completion_time = 45.0f,
            .min_distance = 25.0f,
            .min_shots_hit = 6
        },
    },
    {
        .id = PROTOTYPE_PUZZLE,
        .state = STATE_PROTO_PUZZLE,
        .title = "PUZZLE",
        .subtitle = "Route the links, read the indicators, and time the final crossing.",
        .objective = "Activate the chain room-by-room, reroute the middle lane, then cross the final pulse bridge.",
        .qa = {
            .display_name = "Puzzle",
            .core_question = "Does the room teach the routing system without leaning on explanation text?",
            .allowed_verbs = "Relay activation, router toggling, bridge reading, timing the final pulse",
            .success_condition = "The player debugs the room spatially and gets one real insight moment before the finish.",
            .score_fields = "time, resets, relay interactions, invalid states, stage clears, hintless solve time",
            .recommended_session_length = 6,
            .max_resets = 8,
            .max_completion_time = 60.0f,
            .min_distance = 18.0f,
            .min_puzzle_actions = 4,
            .max_puzzle_misreads = 4
        },
    },
};

static const char *prototype_lab_action_labels[PROTO_LAB_ACTION_COUNT] = {
    "PLAY SLICE",
    "REVIEW METRICS",
    "COMPARE LAST RUNS",
    "RESET SLICE SCORES",
};

static const char *prototype_best_moment_labels[PROTOTYPE_COUNT][4] = {
    {"", "", "", ""},
    {"Clean route", "Risk route", "Mastery shortcut", "Recovery scramble"},
    {"Anchor swing", "Bank shot", "Breach break", "Pressure route"},
    {"Relay read", "Router click", "Path reveal", "Final timing"},
};

static const char *prototype_main_friction_labels[PROTOTYPE_COUNT][4] = {
    {"", "", "", ""},
    {"Route unclear", "Window too tight", "Recovery too weak", "Pacing flat"},
    {"Anchors unclear", "Banks too vague", "Threats too soft", "Combat too static"},
    {"Links too opaque", "Router too fiddly", "Escalation too flat", "Timing too punishing"},
};

static void prototype_apply_grapple(float dt);

static const PrototypeSlice *prototype_slice_by_id(PrototypeId id) {
    for (int i = 0; i < (int)(sizeof(prototype_slices) / sizeof(prototype_slices[0])); i++) {
        if (prototype_slices[i].id == id) return &prototype_slices[i];
    }
    return NULL;
}

static const PrototypeSlice *prototype_slice_by_state(GameState state) {
    for (int i = 0; i < (int)(sizeof(prototype_slices) / sizeof(prototype_slices[0])); i++) {
        if (prototype_slices[i].state == state) return &prototype_slices[i];
    }
    return NULL;
}

static void prototype_clear_scene(Scene *s) {
    memset(s, 0, sizeof(*s));
    s->floor_color = (Color){20, 22, 30, 255};
    s->ceiling_color = (Color){8, 10, 14, 255};
    s->fog_color = (Color){10, 12, 18, 255};
    s->fog_density = 0.008f;
    s->surface = SURFACE_MARBLE;
}

static void prototype_mark_meaningful_action(void) {
    if (!g.prototype_first_action_recorded) {
        g.prototype_run.first_meaningful_action_time = g.state_time;
        g.prototype_first_action_recorded = true;
    }
}

static void prototype_reset_runtime_counters(void) {
    g.prototype_shot_cooldown = 0.0f;
    g.prototype_alt_shot_cooldown = 0.0f;
    g.prototype_shot_flash_timer = 0.0f;
    g.prototype_hit_flash_timer = 0.0f;
    g.prototype_miss_flash_timer = 0.0f;
    g.prototype_breach_flash_timer = 0.0f;
    g.prototype_bank_flash_timer = 0.0f;
    g.prototype_hook_flash_timer = 0.0f;
    g.prototype_hint_timer = 0.0f;
    g.prototype_shooter_heat = 0.0f;
    g.prototype_shooter_overload_timer = 0.0f;
    g.prototype_shooter_suppression_pause = 0.0f;
    g.prototype_grapple_cooldown = 0.0f;
    g.prototype_grapple_timer = 0.0f;
    g.prototype_grapple_anchor = (Vector3){0};
    g.prototype_grapple_active = false;
    g.prototype_movement_finish_window = 0.0f;
    g.prototype_movement_route_timer = 0.0f;
    g.prototype_wall_count = 0;
    g.prototype_ricochet_count = 0;
    g.prototype_target_total = 0;
    g.prototype_targets_hit = 0;
    g.prototype_threat_total = 0;
    g.prototype_threats_disabled = 0;
    g.prototype_gate_wall_index = -1;
    g.prototype_gate_open = false;
    g.prototype_route_wall_index_a = -1;
    g.prototype_route_wall_index_b = -1;
    g.prototype_route_wall_index_c = -1;
    g.prototype_recovery_wall_index = -1;
    g.prototype_route_nodes_live = 0;
    g.prototype_puzzle_stage = 0;
    g.prototype_puzzle_goal = 0;
    g.prototype_puzzle_router_state = 0;
}

static void prototype_reset_slice_records(PrototypeId id) {
    if (id <= PROTOTYPE_NONE || id >= PROTOTYPE_COUNT) return;
    memset(&g.prototype_last_run[id], 0, sizeof(g.prototype_last_run[id]));
    memset(&g.prototype_best_run[id], 0, sizeof(g.prototype_best_run[id]));
    memset(&g.prototype_last_eval[id], 0, sizeof(g.prototype_last_eval[id]));
}

static int prototype_run_score(const PrototypeRunStats *run) {
    if (!run) return -100000;
    int score = 0;
    if (run->completed) score += 10000;
    score -= (int)(run->completion_time * 120.0f);
    score -= run->resets * 320;
    score += run->shots_hit * 80;
    score += run->bank_shot_hits * 130;
    score += run->grapples_latched * 90;
    score += run->anchor_assisted_clears * 120;
    score += run->armored_kills * 160;
    score += run->breach_kills * 120;
    score -= run->exposure_hits * 220;
    score += run->shortcut_uses * 140;
    score += run->recovery_uses * 70;
    score += run->finish_cleanliness * 110;
    score += run->puzzle_stage_clears * 120;
    score -= run->invalid_states * 90;
    return score;
}

static bool prototype_run_is_better(const PrototypeRunStats *candidate, const PrototypeRunStats *current_best) {
    if (!candidate || !candidate->completed) return false;
    if (!current_best || !current_best->completed) return true;
    return prototype_run_score(candidate) > prototype_run_score(current_best);
}

static void prototype_begin_scene(PrototypeId id) {
    if (!g.prototype_preserve_run_stats || g.prototype_active != id) {
        memset(&g.prototype_run, 0, sizeof(g.prototype_run));
    }
    g.prototype_selected = id;
    g.prototype_active = id;
    g.prototype_eval_active = false;
    g.prototype_eval_cursor = 0;
    for (int i = 0; i < 6; i++) g.prototype_eval_draft[i] = (i < 4) ? 3 : 0;
    g.prototype_first_action_recorded = g.prototype_run.first_meaningful_action_time > 0.0f;
    g.prototype_preserve_run_stats = false;
    prototype_reset_runtime_counters();
}

static void prototype_finalize_run(PrototypeId id) {
    if (id <= PROTOTYPE_NONE || id >= PROTOTYPE_COUNT) return;
    g.prototype_last_run[id] = g.prototype_run;
    if (prototype_run_is_better(&g.prototype_run, &g.prototype_best_run[id])) {
        g.prototype_best_run[id] = g.prototype_run;
    }
}

static void prototype_start_eval(void) {
    PrototypeId id = g.prototype_active;
    if (id <= PROTOTYPE_NONE || id >= PROTOTYPE_COUNT) return;

    g.prototype_run.completed = true;
    g.prototype_run.completion_time = g.state_time;
    if (id == PROTOTYPE_MOVEMENT) {
        int clean = 5 - g.prototype_run.resets - g.prototype_run.recovery_uses;
        if (clean < 1) clean = 1;
        if (clean > 5) clean = 5;
        g.prototype_run.finish_cleanliness = clean;
    }
    if (id == PROTOTYPE_PUZZLE && g.prototype_run.hintless_solve_time <= 0.0f) {
        g.prototype_run.hintless_solve_time = g.prototype_run.completion_time;
    }

    prototype_finalize_run(id);
    g.prototype_eval_active = true;
    g.prototype_eval_cursor = 0;

    PrototypeEval prev = g.prototype_last_eval[id];
    g.prototype_eval_draft[0] = prev.submitted ? prev.would_replay : 3;
    g.prototype_eval_draft[1] = prev.submitted ? prev.readability : 3;
    g.prototype_eval_draft[2] = prev.submitted ? prev.mechanical_depth : 3;
    g.prototype_eval_draft[3] = prev.submitted ? prev.ship_confidence : 3;
    g.prototype_eval_draft[4] = prev.submitted ? prev.best_moment : 0;
    g.prototype_eval_draft[5] = prev.submitted ? prev.main_friction : 0;
    show_text("RUN COMPLETE");
}

static void prototype_finish_eval(void) {
    PrototypeId id = g.prototype_active;
    if (id <= PROTOTYPE_NONE || id >= PROTOTYPE_COUNT) return;

    g.prototype_last_eval[id].would_replay = g.prototype_eval_draft[0];
    g.prototype_last_eval[id].readability = g.prototype_eval_draft[1];
    g.prototype_last_eval[id].mechanical_depth = g.prototype_eval_draft[2];
    g.prototype_last_eval[id].ship_confidence = g.prototype_eval_draft[3];
    g.prototype_last_eval[id].best_moment = g.prototype_eval_draft[4];
    g.prototype_last_eval[id].main_friction = g.prototype_eval_draft[5];
    g.prototype_last_eval[id].submitted = true;
    g.prototype_eval_active = false;
    hide_text();
    load_state(STATE_PROTO_LAB);
    g.fade_alpha = 0.0f;
    g.fade_target = 0.0f;
}

static void prototype_restart_scene(void) {
    if (!prototype_slice_by_state(g.state)) return;
    g.prototype_run.resets += 1;
    g.prototype_preserve_run_stats = true;
    hide_text();
    load_state(g.state);
    g.fade_alpha = 0.0f;
    g.fade_target = 0.0f;
}

static void prototype_track_motion_metrics(float dt) {
    (void)dt;
    Vector3 now = g.player.camera.position;
    float moved = Vector3Distance(now, g.prototype_prev_pos);
    g.prototype_run.distance += moved;
    g.prototype_prev_pos = now;

    if (IsKeyPressed(KEY_SPACE)) {
        g.prototype_run.jumps += 1;
        prototype_mark_meaningful_action();
    }
    if (IsKeyPressed(KEY_Q)) {
        g.prototype_run.dashes += 1;
        prototype_mark_meaningful_action();
    }
    if (!g.prototype_first_action_recorded && g.player.speed_current > 1.0f) {
        prototype_mark_meaningful_action();
    }
}

static int prototype_eval_row_limit(PrototypeId id, int row) {
    (void)id;
    if (row < 4) return 5;
    return 3;
}

static void prototype_update_eval_input(void) {
    if (!g.prototype_eval_active) return;
    if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) g.prototype_eval_cursor = (g.prototype_eval_cursor + 5) % 6;
    if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) g.prototype_eval_cursor = (g.prototype_eval_cursor + 1) % 6;
    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
        int *v = &g.prototype_eval_draft[g.prototype_eval_cursor];
        int limit = prototype_eval_row_limit(g.prototype_active, g.prototype_eval_cursor);
        if (*v < limit) *v += 1;
    }
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
        int *v = &g.prototype_eval_draft[g.prototype_eval_cursor];
        int floor = (g.prototype_eval_cursor < 4) ? 1 : 0;
        if (*v > floor) *v -= 1;
    }
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
        prototype_finish_eval();
    }
}

static void prototype_register_last_wall(void) {
    if (g.scene.wall_count <= 0 || g.prototype_wall_count >= 16) return;
    int idx = g.scene.wall_count - 1;
    g.prototype_wall_indices[g.prototype_wall_count] = idx;
    g.prototype_wall_home[g.prototype_wall_count] = g.scene.walls[idx].pos;
    g.prototype_wall_count += 1;
}

static void prototype_register_last_ricochet(void) {
    if (g.scene.wall_count <= 0 || g.prototype_ricochet_count >= 8) return;
    g.prototype_ricochet_indices[g.prototype_ricochet_count++] = g.scene.wall_count - 1;
}

static void prototype_set_gate_wall(void) {
    if (g.scene.wall_count <= 0) return;
    g.prototype_gate_wall_index = g.scene.wall_count - 1;
}

static void prototype_close_gate(void) {
    if (g.prototype_gate_wall_index >= 0 && g.prototype_gate_wall_index < g.scene.wall_count) {
        g.scene.walls[g.prototype_gate_wall_index].active = true;
        g.prototype_gate_open = false;
    }
}

static void prototype_open_gate(void) {
    if (g.prototype_gate_wall_index >= 0 && g.prototype_gate_wall_index < g.scene.wall_count) {
        g.scene.walls[g.prototype_gate_wall_index].active = false;
        g.prototype_gate_open = true;
    }
}

static bool prototype_object_is_anchor(const InteractObject *obj) {
    return obj && obj->name && strncmp(obj->name, "anchor_", 7) == 0;
}

static bool prototype_find_best_anchor(Vector3 origin, Vector3 forward, float max_dist, Vector3 *out_anchor, const char **out_name) {
    float best_score = 0.94f;
    bool found = false;
    for (int i = 0; i < g.scene.object_count; i++) {
        InteractObject *obj = &g.scene.objects[i];
        if (!obj->active || !prototype_object_is_anchor(obj)) continue;
        Vector3 delta = Vector3Subtract(obj->pos, origin);
        float dist = Vector3Length(delta);
        if (dist <= 0.001f || dist > max_dist) continue;
        Vector3 dir = Vector3Scale(delta, 1.0f / dist);
        float facing = Vector3DotProduct(forward, dir);
        if (facing > best_score) {
            best_score = facing;
            found = true;
            if (out_anchor) *out_anchor = obj->pos;
            if (out_name) *out_name = obj->name;
        }
    }
    return found;
}

static void build_prototype_lab_room(Scene *s) {
    prototype_clear_scene(s);
    s->spawn = (Vector3){0.0f, 1.6f, 0.0f};
}

static void build_movement_slice(Scene *s) {
    prototype_clear_scene(s);

    add_wall(s, 0, -0.3f, 15, 8, 0.6f, 12, (Color){36, 40, 52, 255});
    set_last_material(s, MAT_TILE);
    add_wall(s, -2.4f, 0.1f, 7.0f, 3.4f, 0.2f, 3.2f, (Color){42, 46, 58, 255});
    set_last_material(s, MAT_TILE);
    add_wall(s, 2.8f, 0.4f, 1.0f, 2.4f, 0.8f, 2.4f, (Color){208, 112, 68, 255});
    set_last_material(s, MAT_BRASS);
    add_wall(s, -2.2f, 1.0f, -4.0f, 1.4f, 2.0f, 1.4f, (Color){208, 112, 68, 255});
    set_last_material(s, MAT_BRASS);
    add_wall(s, 1.8f, 1.5f, -8.5f, 1.2f, 3.0f, 1.2f, (Color){208, 112, 68, 255});
    set_last_material(s, MAT_BRASS);
    add_wall(s, -1.4f, 0.3f, -13.5f, 2.2f, 0.6f, 3.4f, (Color){220, 190, 120, 255});
    set_last_material(s, MAT_WOOD);
    add_wall(s, 3.6f, 0.1f, -15.5f, 2.0f, 0.2f, 2.8f, (Color){42, 46, 58, 255});
    set_last_material(s, MAT_TILE);

    add_wall(s, 0, -1.7f, -10.0f, 9.0f, 0.6f, 6.0f, (Color){30, 34, 44, 255});
    set_last_material(s, MAT_CONCRETE);
    add_wall(s, -3.4f, -1.0f, -12.4f, 2.0f, 0.6f, 2.4f, (Color){60, 72, 98, 255});
    set_last_material(s, MAT_GLASS);

    add_wall(s, -6, 2.5f, -2.0f, 0.4f, 5.0f, 42, (Color){26, 30, 40, 255});
    set_last_material(s, MAT_MARBLE);
    add_wall(s, 6, 2.5f, -2.0f, 0.4f, 5.0f, 42, (Color){26, 30, 40, 255});
    set_last_material(s, MAT_MARBLE);
    add_wall(s, 0, 2.5f, -23, 12.4f, 5.0f, 0.4f, (Color){26, 30, 40, 255});
    set_last_material(s, MAT_MARBLE);

    add_light_panel(s, 0.0f, 3.8f, 15.0f, 3.0f, 0.08f, 1.4f, (Color){200, 220, 255, 255});
    add_light_panel(s, 0.0f, 3.8f, 6.5f, 2.8f, 0.08f, 1.4f, (Color){200, 220, 255, 255});
    add_light_panel(s, 0.0f, 3.8f, -4.5f, 2.8f, 0.08f, 1.4f, (Color){255, 205, 150, 255});
    add_light_panel(s, 0.0f, 3.8f, -14.5f, 3.2f, 0.08f, 1.4f, (Color){255, 205, 150, 255});
    add_light_panel(s, 3.6f, 1.0f, 6.8f, 0.25f, 0.8f, 0.25f, (Color){120, 220, 255, 255});
    add_light_panel(s, -3.2f, 1.0f, -5.0f, 0.25f, 0.8f, 0.25f, (Color){255, 220, 120, 255});
    add_light_panel(s, 1.9f, 2.2f, -9.4f, 0.25f, 1.0f, 0.25f, (Color){180, 255, 160, 255});
    add_light_panel(s, -3.4f, -0.7f, -12.4f, 0.25f, 0.8f, 0.25f, (Color){120, 255, 210, 255});

    add_wall(s, -0.6f, 0.9f, 2.0f, 4.8f, 0.25f, 1.6f, (Color){120, 210, 255, 255});
    set_last_material(s, MAT_GLASS);
    g.prototype_route_wall_index_a = s->wall_count - 1;
    s->walls[g.prototype_route_wall_index_a].active = false;

    add_wall(s, 0.0f, 1.5f, -19.0f, 2.2f, 3.0f, 0.6f, (Color){90, 180, 140, 255});
    set_last_material(s, MAT_GLASS);
    prototype_set_gate_wall();

    add_wall(s, 2.6f, 2.0f, -12.5f, 2.6f, 0.25f, 1.2f, (Color){255, 216, 120, 255});
    set_last_material(s, MAT_BRASS);
    g.prototype_route_wall_index_b = s->wall_count - 1;
    s->walls[g.prototype_route_wall_index_b].active = false;

    add_wall(s, 0.8f, 2.6f, -16.0f, 2.0f, 0.25f, 1.2f, (Color){180, 255, 160, 255});
    set_last_material(s, MAT_GLASS);
    g.prototype_route_wall_index_c = s->wall_count - 1;
    s->walls[g.prototype_route_wall_index_c].active = false;

    add_wall(s, -1.0f, -0.7f, -15.5f, 4.2f, 0.25f, 1.6f, (Color){120, 255, 210, 255});
    set_last_material(s, MAT_GLASS);
    g.prototype_recovery_wall_index = s->wall_count - 1;
    s->walls[g.prototype_recovery_wall_index].active = false;

    add_object(s, 2.0f, 1.0f, 7.6f, "flow_a", (Color){120, 220, 255, 255}, 1);
    add_object(s, -1.4f, 1.0f, -2.6f, "flow_b", (Color){255, 220, 120, 255}, 1);
    add_object(s, 1.2f, 1.0f, -6.6f, "flow_c", (Color){180, 255, 160, 255}, 1);
    add_object(s, 3.8f, 3.0f, -10.2f, "anchor_route", (Color){120, 255, 220, 255}, 1);

    s->spawn = (Vector3){0.0f, 1.6f, 18.0f};
    s->has_exit = true;
    s->exit_pos = (Vector3){0.0f, 1.6f, -20.0f};
}

static void build_shooter_slice(Scene *s) {
    prototype_clear_scene(s);

    add_wall(s, 0, -0.3f, 0, 14, 0.6f, 38, (Color){38, 42, 54, 255});
    set_last_material(s, MAT_TILE);
    add_wall(s, -7, 2.5f, 0, 0.5f, 5.0f, 38, (Color){24, 26, 34, 255});
    set_last_material(s, MAT_MARBLE);
    add_wall(s, 7, 2.5f, 0, 0.5f, 5.0f, 38, (Color){24, 26, 34, 255});
    set_last_material(s, MAT_MARBLE);
    add_wall(s, 0, 2.5f, -19, 14.5f, 5.0f, 0.5f, (Color){24, 26, 34, 255});
    set_last_material(s, MAT_MARBLE);

    add_wall(s, -2.8f, 0.8f, 8.8f, 2.4f, 1.6f, 1.4f, (Color){54, 58, 72, 255});
    set_last_material(s, MAT_BRASS);
    add_wall(s, 2.8f, 0.8f, 4.2f, 2.2f, 1.6f, 1.2f, (Color){54, 58, 72, 255});
    set_last_material(s, MAT_BRASS);
    add_wall(s, -1.8f, 0.8f, -0.8f, 2.2f, 1.6f, 1.2f, (Color){54, 58, 72, 255});
    set_last_material(s, MAT_BRASS);
    add_wall(s, 2.2f, 0.8f, -5.8f, 2.2f, 1.6f, 1.2f, (Color){54, 58, 72, 255});
    set_last_material(s, MAT_BRASS);
    add_wall(s, -3.8f, 1.0f, -10.4f, 1.4f, 2.0f, 1.4f, (Color){70, 74, 86, 255});
    set_last_material(s, MAT_MARBLE);
    add_wall(s, 3.8f, 1.0f, -12.8f, 1.4f, 2.0f, 1.4f, (Color){70, 74, 86, 255});
    set_last_material(s, MAT_MARBLE);

    add_wall(s, 0.0f, 0.1f, 10.5f, 10.0f, 0.2f, 0.6f, (Color){150, 42, 42, 255});
    set_last_material(s, MAT_EMISSIVE);
    add_wall(s, 0.0f, 0.1f, 1.0f, 10.0f, 0.2f, 0.6f, (Color){150, 42, 42, 255});
    set_last_material(s, MAT_EMISSIVE);
    add_wall(s, 0.0f, 0.1f, -8.5f, 10.0f, 0.2f, 0.6f, (Color){150, 42, 42, 255});
    set_last_material(s, MAT_EMISSIVE);

    add_wall(s, -5.2f, 1.8f, 7.0f, 0.45f, 2.8f, 4.4f, (Color){90, 200, 255, 255});
    set_last_material(s, MAT_GLASS);
    prototype_register_last_ricochet();
    add_wall(s, 5.2f, 1.8f, -4.4f, 0.45f, 2.8f, 4.4f, (Color){90, 200, 255, 255});
    set_last_material(s, MAT_GLASS);
    prototype_register_last_ricochet();
    add_wall(s, 0.0f, 2.0f, -12.0f, 2.6f, 2.4f, 0.4f, (Color){90, 200, 255, 255});
    set_last_material(s, MAT_GLASS);
    prototype_register_last_ricochet();

    add_light_panel(s, 0.0f, 3.9f, 12.0f, 4.0f, 0.08f, 1.4f, (Color){230, 240, 255, 255});
    add_light_panel(s, 0.0f, 3.9f, 3.0f, 4.0f, 0.08f, 1.4f, (Color){230, 240, 255, 255});
    add_light_panel(s, 0.0f, 3.9f, -6.0f, 4.0f, 0.08f, 1.4f, (Color){255, 200, 170, 255});
    add_light_panel(s, 0.0f, 3.9f, -15.0f, 4.0f, 0.08f, 1.4f, (Color){255, 200, 170, 255});
    add_light_panel(s, -6.5f, 1.2f, 10.5f, 0.12f, 1.8f, 1.6f, (Color){220, 70, 70, 255});
    add_light_panel(s, -6.5f, 1.2f, 1.0f, 0.12f, 1.8f, 1.6f, (Color){220, 70, 70, 255});
    add_light_panel(s, -6.5f, 1.2f, -8.5f, 0.12f, 1.8f, 1.6f, (Color){220, 70, 70, 255});
    add_light_panel(s, -5.8f, 1.0f, -1.0f, 0.25f, 0.7f, 0.25f, (Color){120, 220, 255, 255});
    add_light_panel(s, 5.8f, 1.0f, 5.5f, 0.25f, 0.7f, 0.25f, (Color){120, 255, 210, 255});
    add_light_panel(s, 5.8f, 1.0f, -10.8f, 0.25f, 0.7f, 0.25f, (Color){255, 180, 120, 255});

    add_sphere(s, -3.5f, 2.0f, 6.5f, 1.0f, (Color){255, 110, 90, 255});
    set_last_material(s, MAT_EMISSIVE);
    g.scene.walls[g.scene.wall_count - 1].no_collide = true;
    prototype_register_last_wall();

    add_sphere(s, 3.6f, 2.4f, 1.0f, 1.15f, (Color){120, 220, 255, 255});
    set_last_material(s, MAT_EMISSIVE);
    g.scene.walls[g.scene.wall_count - 1].no_collide = true;
    prototype_register_last_wall();

    add_sphere(s, -2.8f, 2.8f, -4.0f, 1.05f, (Color){110, 255, 200, 255});
    set_last_material(s, MAT_EMISSIVE);
    g.scene.walls[g.scene.wall_count - 1].no_collide = true;
    prototype_register_last_wall();

    add_sphere(s, 2.4f, 2.2f, -9.0f, 1.15f, (Color){120, 220, 255, 255});
    set_last_material(s, MAT_EMISSIVE);
    g.scene.walls[g.scene.wall_count - 1].no_collide = true;
    prototype_register_last_wall();

    add_sphere(s, -3.2f, 2.4f, -12.2f, 1.0f, (Color){255, 110, 90, 255});
    set_last_material(s, MAT_EMISSIVE);
    g.scene.walls[g.scene.wall_count - 1].no_collide = true;
    prototype_register_last_wall();

    add_sphere(s, 3.4f, 2.6f, -13.8f, 1.05f, (Color){110, 255, 200, 255});
    set_last_material(s, MAT_EMISSIVE);
    g.scene.walls[g.scene.wall_count - 1].no_collide = true;
    prototype_register_last_wall();

    add_sphere(s, 0.0f, 2.8f, -16.2f, 1.0f, (Color){255, 110, 90, 255});
    set_last_material(s, MAT_EMISSIVE);
    g.scene.walls[g.scene.wall_count - 1].no_collide = true;
    prototype_register_last_wall();

    g.prototype_target_total = g.prototype_wall_count;
    g.prototype_threat_total = 2;

    add_wall(s, 0.0f, 1.5f, -17.8f, 2.2f, 3.0f, 0.6f, (Color){90, 180, 140, 255});
    set_last_material(s, MAT_GLASS);
    prototype_set_gate_wall();

    add_object(s, -4.2f, 1.0f, 1.2f, "breach_cell", (Color){110, 220, 255, 255}, 1);
    add_object(s, 4.2f, 1.0f, 5.8f, "coolant_tap", (Color){120, 255, 210, 255}, 1);
    add_object(s, 0.0f, 1.0f, -6.8f, "signal_console", (Color){255, 180, 120, 255}, 1);
    add_object(s, -4.6f, 3.1f, 8.2f, "anchor_safe", (Color){120, 255, 220, 255}, 1);
    add_object(s, 4.8f, 3.6f, -0.8f, "anchor_flank", (Color){120, 255, 220, 255}, 1);
    add_object(s, 0.0f, 3.9f, -13.2f, "anchor_finish", (Color){120, 255, 220, 255}, 1);

    s->spawn = (Vector3){0.0f, 1.6f, 15.5f};
    s->has_exit = true;
    s->exit_pos = (Vector3){0.0f, 1.6f, -18.4f};
}

static void build_puzzle_slice(Scene *s) {
    prototype_clear_scene(s);

    add_wall(s, 0, -0.3f, 0, 18, 0.6f, 32, (Color){16, 18, 24, 255});
    set_last_material(s, MAT_TILE);
    add_wall(s, -10, 2.5f, 0, 0.5f, 5.0f, 32, (Color){24, 26, 34, 255});
    set_last_material(s, MAT_MARBLE);
    add_wall(s, 10, 2.5f, 0, 0.5f, 5.0f, 32, (Color){24, 26, 34, 255});
    set_last_material(s, MAT_MARBLE);
    add_wall(s, 0, 2.5f, -16, 20, 5.0f, 0.5f, (Color){24, 26, 34, 255});
    set_last_material(s, MAT_MARBLE);
    add_wall(s, 0, 2.5f, 16, 20, 5.0f, 0.5f, (Color){24, 26, 34, 255});
    set_last_material(s, MAT_MARBLE);

    add_wall(s, 0.0f, 0.0f, 11.0f, 4.0f, 0.4f, 4.0f, (Color){46, 50, 62, 255});
    set_last_material(s, MAT_GLASS);
    add_wall(s, 0.0f, 0.0f, 4.0f, 4.0f, 0.4f, 4.0f, (Color){46, 50, 62, 255});
    set_last_material(s, MAT_GLASS);
    add_wall(s, -6.0f, 0.0f, 0.0f, 4.0f, 0.4f, 4.0f, (Color){46, 50, 62, 255});
    set_last_material(s, MAT_GLASS);
    add_wall(s, 6.0f, 0.0f, 0.0f, 4.0f, 0.4f, 4.0f, (Color){46, 50, 62, 255});
    set_last_material(s, MAT_GLASS);
    add_wall(s, 6.0f, 0.0f, -6.5f, 4.0f, 0.4f, 4.0f, (Color){46, 50, 62, 255});
    set_last_material(s, MAT_GLASS);
    add_wall(s, 6.0f, 0.0f, -12.5f, 4.0f, 0.4f, 4.0f, (Color){46, 50, 62, 255});
    set_last_material(s, MAT_GLASS);

    add_wall(s, 0.0f, 0.2f, 7.5f, 2.8f, 0.3f, 1.0f, (Color){100, 190, 255, 255});
    set_last_material(s, MAT_EMISSIVE);
    prototype_register_last_wall();

    add_wall(s, -3.0f, 0.2f, 2.0f, 3.0f, 0.3f, 1.0f, (Color){100, 190, 255, 255});
    set_last_material(s, MAT_EMISSIVE);
    prototype_register_last_wall();

    add_wall(s, 3.0f, 0.2f, 2.0f, 3.0f, 0.3f, 1.0f, (Color){100, 190, 255, 255});
    set_last_material(s, MAT_EMISSIVE);
    prototype_register_last_wall();

    add_wall(s, 6.0f, 0.2f, -3.3f, 2.8f, 0.3f, 1.0f, (Color){100, 190, 255, 255});
    set_last_material(s, MAT_EMISSIVE);
    prototype_register_last_wall();

    add_wall(s, 6.0f, 0.2f, -9.8f, 2.8f, 0.3f, 1.0f, (Color){140, 235, 190, 255});
    set_last_material(s, MAT_EMISSIVE);
    prototype_register_last_wall();

    add_light_panel(s, 0.0f, 3.9f, 11.0f, 3.2f, 0.08f, 1.4f, (Color){170, 215, 255, 255});
    add_light_panel(s, 0.0f, 3.9f, 4.0f, 3.2f, 0.08f, 1.4f, (Color){170, 215, 255, 255});
    add_light_panel(s, -6.0f, 3.9f, 0.0f, 3.2f, 0.08f, 1.4f, (Color){150, 205, 255, 255});
    add_light_panel(s, 6.0f, 3.9f, 0.0f, 3.2f, 0.08f, 1.4f, (Color){150, 205, 255, 255});
    add_light_panel(s, 6.0f, 3.9f, -6.5f, 3.2f, 0.08f, 1.4f, (Color){170, 215, 255, 255});
    add_light_panel(s, 6.0f, 3.9f, -12.5f, 3.2f, 0.08f, 1.4f, (Color){140, 235, 190, 255});

    add_wall(s, 6.0f, 1.5f, -14.6f, 2.2f, 3.0f, 0.6f, (Color){90, 180, 140, 255});
    set_last_material(s, MAT_GLASS);
    prototype_set_gate_wall();

    add_object(s, 0.0f, 1.0f, 11.0f, "relay_a", (Color){80, 180, 255, 255}, 1);
    add_object(s, 0.0f, 1.0f, 4.0f, "router", (Color){255, 214, 120, 255}, 1);
    add_object(s, 6.0f, 1.0f, 0.0f, "relay_b", (Color){80, 180, 255, 255}, 1);
    add_object(s, 6.0f, 1.0f, -6.5f, "relay_c", (Color){80, 255, 190, 255}, 1);
    add_object(s, 3.0f, 3.4f, -9.4f, "anchor_puzzle", (Color){120, 255, 220, 255}, 1);

    g.prototype_puzzle_goal = 3;
    s->spawn = (Vector3){0.0f, 1.6f, 13.0f};
    s->has_exit = true;
    s->exit_pos = (Vector3){6.0f, 1.6f, -15.2f};
}

static void prototype_common_load(void) {
    DisableCursor();
    StopAmbient(&g.audio);
    StopClockAmbient(&g.audio);
    StopCityAmbient(&g.audio);
    StopWindAmbient(&g.audio);
    StopEarthPresence(&g.audio);
    StopSuiteMusic(&g.audio);
    StopBalconyMusic(&g.audio);
    StopCorridorMusic(&g.audio);

    g.player.gravity_mult = 1.0f;
    SetSceneLighting(&g.lighting, LightingPreset_SpaceSuite());
    set_exposure(0.0f);
    SetPostFXGrain(&g.postfx, 0.18f);
    SetPostFXWarmth(&g.postfx, 0.08f);
    SetPostFXCA(&g.postfx, 0.0f);
    g.prototype_prev_pos = g.player.camera.position;
}

static void apply_movement_slice_lighting(void) {
    SceneLighting preset = LightingPreset_Hallway();
    preset.keyDir = Vector3Normalize((Vector3){-0.18f, -0.94f, -0.22f});
    preset.keyColor[0] = 0.72f; preset.keyColor[1] = 0.80f; preset.keyColor[2] = 0.92f;
    preset.fillColor[0] = 0.08f; preset.fillColor[1] = 0.12f; preset.fillColor[2] = 0.18f;
    preset.ambient[0] = 0.05f; preset.ambient[1] = 0.06f; preset.ambient[2] = 0.08f;
    preset.pointPos[0] = (Vector3){0.0f, 3.8f, 15.0f};
    preset.pointPos[1] = (Vector3){0.0f, 3.8f, 6.5f};
    preset.pointPos[2] = (Vector3){0.0f, 3.8f, -4.5f};
    preset.pointPos[3] = (Vector3){0.0f, 3.8f, -15.0f};
    preset.pointColor[0][0] = 0.55f; preset.pointColor[0][1] = 0.68f; preset.pointColor[0][2] = 0.92f;
    preset.pointColor[1][0] = 0.55f; preset.pointColor[1][1] = 0.68f; preset.pointColor[1][2] = 0.92f;
    preset.pointColor[2][0] = 0.92f; preset.pointColor[2][1] = 0.72f; preset.pointColor[2][2] = 0.42f;
    preset.pointColor[3][0] = 0.92f; preset.pointColor[3][1] = 0.72f; preset.pointColor[3][2] = 0.42f;
    preset.pointRadius[0] = 9.0f; preset.pointRadius[1] = 8.0f; preset.pointRadius[2] = 8.0f; preset.pointRadius[3] = 9.0f;
    preset.pointPulse[0] = 0.08f; preset.pointPulse[1] = 0.06f; preset.pointPulse[2] = 0.07f; preset.pointPulse[3] = 0.09f;
    preset.pointPhase[0] = 2.2f; preset.pointPhase[1] = 2.8f; preset.pointPhase[2] = 1.7f; preset.pointPhase[3] = 2.4f;
    SetSceneLighting(&g.lighting, preset);
    set_exposure(0.08f);
    SetPostFXGrain(&g.postfx, 0.14f);
    SetPostFXWarmth(&g.postfx, 0.02f);
}

static void apply_shooter_slice_lighting(void) {
    SceneLighting preset = LightingPreset_Bathroom();
    preset.keyDir = Vector3Normalize((Vector3){0.05f, -0.98f, -0.15f});
    preset.keyColor[0] = 0.92f; preset.keyColor[1] = 0.96f; preset.keyColor[2] = 1.00f;
    preset.fillColor[0] = 0.10f; preset.fillColor[1] = 0.13f; preset.fillColor[2] = 0.20f;
    preset.ambient[0] = 0.035f; preset.ambient[1] = 0.04f; preset.ambient[2] = 0.05f;
    preset.pointPos[0] = (Vector3){0.0f, 3.9f, 12.0f};
    preset.pointPos[1] = (Vector3){0.0f, 3.9f, 3.0f};
    preset.pointPos[2] = (Vector3){0.0f, 3.9f, -6.0f};
    preset.pointPos[3] = (Vector3){0.0f, 3.9f, -15.0f};
    preset.pointPos[4] = (Vector3){-6.4f, 1.1f, 10.5f};
    preset.pointPos[5] = (Vector3){-6.4f, 1.1f, 1.0f};
    preset.pointPos[6] = (Vector3){-6.4f, 1.1f, -8.5f};
    preset.pointColor[0][0] = 0.62f; preset.pointColor[0][1] = 0.72f; preset.pointColor[0][2] = 0.92f;
    preset.pointColor[1][0] = 0.62f; preset.pointColor[1][1] = 0.72f; preset.pointColor[1][2] = 0.92f;
    preset.pointColor[2][0] = 0.95f; preset.pointColor[2][1] = 0.62f; preset.pointColor[2][2] = 0.42f;
    preset.pointColor[3][0] = 0.95f; preset.pointColor[3][1] = 0.62f; preset.pointColor[3][2] = 0.42f;
    preset.pointColor[4][0] = 0.95f; preset.pointColor[4][1] = 0.22f; preset.pointColor[4][2] = 0.18f;
    preset.pointColor[5][0] = 0.95f; preset.pointColor[5][1] = 0.22f; preset.pointColor[5][2] = 0.18f;
    preset.pointColor[6][0] = 0.95f; preset.pointColor[6][1] = 0.22f; preset.pointColor[6][2] = 0.18f;
    preset.pointRadius[0] = 8.0f; preset.pointRadius[1] = 8.0f; preset.pointRadius[2] = 8.0f; preset.pointRadius[3] = 8.0f;
    preset.pointRadius[4] = 6.0f; preset.pointRadius[5] = 6.0f; preset.pointRadius[6] = 6.0f;
    preset.pointPulse[0] = 0.10f; preset.pointPulse[1] = 0.08f; preset.pointPulse[2] = 0.10f; preset.pointPulse[3] = 0.12f;
    preset.pointPulse[4] = 0.18f; preset.pointPulse[5] = 0.18f; preset.pointPulse[6] = 0.18f;
    preset.pointFlicker[4] = 0.08f; preset.pointFlicker[5] = 0.08f; preset.pointFlicker[6] = 0.08f;
    preset.pointPhase[0] = 2.4f; preset.pointPhase[1] = 2.8f; preset.pointPhase[2] = 2.1f; preset.pointPhase[3] = 2.5f;
    preset.pointPhase[4] = 5.0f; preset.pointPhase[5] = 4.5f; preset.pointPhase[6] = 5.4f;
    SetSceneLighting(&g.lighting, preset);
    set_exposure(0.14f);
    SetPostFXGrain(&g.postfx, 0.10f);
    SetPostFXContrast(&g.postfx, 0.08f);
    SetPostFXWarmth(&g.postfx, -0.04f);
}

static void apply_puzzle_slice_lighting(void) {
    SceneLighting preset = LightingPreset_SpaceCorridor();
    preset.keyDir = Vector3Normalize((Vector3){-0.10f, -0.92f, -0.30f});
    preset.keyColor[0] = 0.60f; preset.keyColor[1] = 0.74f; preset.keyColor[2] = 0.90f;
    preset.fillColor[0] = 0.08f; preset.fillColor[1] = 0.12f; preset.fillColor[2] = 0.18f;
    preset.ambient[0] = 0.05f; preset.ambient[1] = 0.06f; preset.ambient[2] = 0.08f;
    preset.pointPos[0] = (Vector3){0.0f, 3.9f, 11.0f};
    preset.pointPos[1] = (Vector3){0.0f, 3.9f, 4.0f};
    preset.pointPos[2] = (Vector3){6.0f, 3.9f, 0.0f};
    preset.pointPos[3] = (Vector3){6.0f, 3.9f, -12.5f};
    preset.pointColor[0][0] = 0.48f; preset.pointColor[0][1] = 0.72f; preset.pointColor[0][2] = 0.92f;
    preset.pointColor[1][0] = 0.48f; preset.pointColor[1][1] = 0.72f; preset.pointColor[1][2] = 0.92f;
    preset.pointColor[2][0] = 0.48f; preset.pointColor[2][1] = 0.72f; preset.pointColor[2][2] = 0.92f;
    preset.pointColor[3][0] = 0.42f; preset.pointColor[3][1] = 0.86f; preset.pointColor[3][2] = 0.62f;
    preset.pointRadius[0] = 7.0f; preset.pointRadius[1] = 7.0f; preset.pointRadius[2] = 7.0f; preset.pointRadius[3] = 8.0f;
    preset.pointPulse[0] = 0.10f; preset.pointPulse[1] = 0.10f; preset.pointPulse[2] = 0.10f; preset.pointPulse[3] = 0.16f;
    preset.pointPhase[0] = 1.8f; preset.pointPhase[1] = 2.2f; preset.pointPhase[2] = 2.6f; preset.pointPhase[3] = 1.6f;
    SetSceneLighting(&g.lighting, preset);
    set_exposure(0.10f);
    SetPostFXGrain(&g.postfx, 0.08f);
    SetPostFXWarmth(&g.postfx, -0.02f);
}

static void shooter_refresh_lighting(void) {
    float progress = 0.0f;
    if (g.prototype_target_total > 0) {
        progress = (float)g.prototype_targets_hit / (float)g.prototype_target_total;
    }
    for (int i = 0; i < 4; i++) {
        float warm = 0.36f + progress * 0.36f;
        float cool = 0.90f - progress * 0.18f;
        SetPointLightIdx(&g.lighting,
                         i,
                         g.lighting.activePreset.pointPos[i].x,
                         g.lighting.activePreset.pointPos[i].y,
                         g.lighting.activePreset.pointPos[i].z,
                         cool,
                         0.62f + progress * 0.18f,
                         warm,
                         g.lighting.activePreset.pointRadius[i]);
    }
    for (int i = 4; i <= 6; i++) {
        float alert = 0.95f - progress * 0.35f;
        SetPointLightIdx(&g.lighting,
                         i,
                         g.lighting.activePreset.pointPos[i].x,
                         g.lighting.activePreset.pointPos[i].y,
                         g.lighting.activePreset.pointPos[i].z,
                         alert,
                         0.16f + progress * 0.34f,
                         0.14f + progress * 0.10f,
                         g.lighting.activePreset.pointRadius[i]);
    }
    SetPostFXContrast(&g.postfx, 0.08f + progress * 0.12f + g.prototype_shooter_heat * 0.08f);
}

static ShooterTargetType shooter_target_type(int slot) {
    if (slot == 1 || slot == 3) return SHOOTER_TARGET_ARMORED;
    if (slot == 2 || slot == 5) return SHOOTER_TARGET_THREAT;
    return SHOOTER_TARGET_STANDARD;
}

static float shooter_lane_center_z(int lane) {
    static const float centers[3] = {10.5f, 1.0f, -8.5f};
    if (lane < 0) lane = 0;
    if (lane > 2) lane = 2;
    return centers[lane];
}

static void shooter_refresh_target_visuals(void) {
    for (int i = 0; i < g.prototype_wall_count; i++) {
        int idx = g.prototype_wall_indices[i];
        if (idx < 0 || idx >= g.scene.wall_count) continue;
        Wall *w = &g.scene.walls[idx];
        if (!w->active) continue;

        ShooterTargetType type = shooter_target_type(i);
        float pulse = 0.5f + 0.5f * sinf(g.state_time * (2.0f + 0.4f * (float)i));
        if (type == SHOOTER_TARGET_ARMORED) {
            w->size = (Vector3){1.25f, 1.25f, 1.25f};
            w->color = (Color){
                (unsigned char)(90 + pulse * 30.0f),
                (unsigned char)(210 + pulse * 25.0f),
                255,
                255
            };
        } else if (type == SHOOTER_TARGET_THREAT) {
            w->size = (Vector3){1.08f, 1.08f, 1.08f};
            w->color = (Color){
                (unsigned char)(90 + pulse * 30.0f),
                255,
                (unsigned char)(180 + pulse * 50.0f),
                255
            };
        } else {
            w->size = (Vector3){1.0f, 1.0f, 1.0f};
            w->color = (Color){
                255,
                (unsigned char)(110 + pulse * 40.0f),
                (unsigned char)(90 + pulse * 20.0f),
                255
            };
        }
    }
}

static float shooter_safe_pocket_x(int hot_lane) {
    return sinf(g.state_time * (1.4f + 0.2f * (float)hot_lane) + (float)hot_lane) * 2.6f;
}

static void shooter_refresh_lane_visuals(void) {
    float cycle = 1.45f + 0.16f * (float)(g.prototype_threats_disabled);
    int hot_lane = ((int)floorf(g.state_time / cycle)) % 3;
    float phase = fmodf(g.state_time, cycle) / cycle;
    float pocket_x = shooter_safe_pocket_x(hot_lane);
    for (int i = 0; i < 3; i++) {
        int idx = 10 + i;
        if (idx < 0 || idx >= g.scene.wall_count) continue;
        float intensity = 0.28f;
        if (i == hot_lane) intensity = (phase > 0.32f) ? 1.0f : 0.58f;
        g.scene.walls[idx].color = (Color){
            (unsigned char)(130 + intensity * 100.0f),
            (unsigned char)(24 + intensity * 44.0f),
            (unsigned char)(24 + intensity * 30.0f),
            255
        };
        if (i == hot_lane) {
            g.scene.walls[idx].pos.x = pocket_x * 0.12f;
        } else {
            g.scene.walls[idx].pos.x = 0.0f;
        }
    }
}

static void shooter_update_suppression(void) {
    if (g.prototype_shooter_suppression_pause > 0.0f || g.prototype_shooter_overload_timer > 0.0f) return;

    float cycle = 1.45f + 0.16f * (float)(g.prototype_threats_disabled);
    int hot_lane = ((int)floorf(g.state_time / cycle)) % 3;
    float phase = fmodf(g.state_time, cycle) / cycle;
    if (phase < 0.32f) return;

    float lane_z = shooter_lane_center_z(hot_lane);
    float pocket_x = shooter_safe_pocket_x(hot_lane);
    bool in_lane = fabsf(g.player.camera.position.z - lane_z) < 2.0f;
    bool exposed = fabsf(g.player.camera.position.x - pocket_x) > 1.35f && fabsf(g.player.camera.position.x) < 4.4f;
    if (in_lane && exposed) {
        g.prototype_run.exposure_hits += 1;
        show_text("SUPPRESSED");
        prototype_restart_scene();
    }
}

static void movement_refresh_route(void) {
    if (g.prototype_route_wall_index_a >= 0 && g.prototype_route_wall_index_a < g.scene.wall_count) {
        g.scene.walls[g.prototype_route_wall_index_a].color = (Color){120, 210, 255, 255};
    }
    if (g.prototype_route_wall_index_b >= 0 && g.prototype_route_wall_index_b < g.scene.wall_count) {
        bool live = g.prototype_movement_finish_window > 0.0f;
        g.scene.walls[g.prototype_route_wall_index_b].active = live;
        g.scene.walls[g.prototype_route_wall_index_b].color = live
            ? (Color){255, 228, 140, 255}
            : (Color){110, 98, 70, 255};
    }
    if (g.prototype_route_wall_index_c >= 0 && g.prototype_route_wall_index_c < g.scene.wall_count) {
        float pulse = 0.5f + 0.5f * sinf(g.state_time * 4.0f);
        g.scene.walls[g.prototype_route_wall_index_c].color = (Color){
            (unsigned char)(140 + pulse * 30.0f),
            (unsigned char)(240 + pulse * 15.0f),
            (unsigned char)(180 + pulse * 20.0f),
            255
        };
    }
    if (g.prototype_recovery_wall_index >= 0 && g.prototype_recovery_wall_index < g.scene.wall_count) {
        float pulse = 0.5f + 0.5f * sinf(g.state_time * 3.5f);
        g.scene.walls[g.prototype_recovery_wall_index].color = (Color){
            (unsigned char)(90 + pulse * 40.0f),
            (unsigned char)(220 + pulse * 35.0f),
            (unsigned char)(210 + pulse * 25.0f),
            255
        };
    }
}

static void movement_handle_interactions(void) {
    for (int i = 0; i < g.scene.object_count; i++) {
        InteractObject *obj = &g.scene.objects[i];
        if (!obj->active || obj->done) continue;
        if (Vector3Distance(g.player.camera.position, obj->pos) > obj->radius) continue;

        obj->done = true;
        prototype_mark_meaningful_action();

        if (strcmp(obj->name, "flow_a") == 0) {
            g.prototype_route_nodes_live += 1;
            g.prototype_run.route_nodes_triggered += 1;
            if (g.prototype_route_wall_index_a >= 0 && g.prototype_route_wall_index_a < g.scene.wall_count) {
                g.scene.walls[g.prototype_route_wall_index_a].active = true;
            }
            show_text("FAST LINE LIVE");
        } else if (strcmp(obj->name, "flow_b") == 0) {
            g.prototype_route_nodes_live += 1;
            g.prototype_run.route_nodes_triggered += 1;
            g.prototype_movement_finish_window = 5.5f;
            prototype_open_gate();
            if (g.prototype_route_wall_index_b >= 0 && g.prototype_route_wall_index_b < g.scene.wall_count) {
                g.scene.walls[g.prototype_route_wall_index_b].active = true;
            }
            show_text("FINISH WINDOW OPEN");
        } else if (strcmp(obj->name, "flow_c") == 0) {
            g.prototype_route_nodes_live += 1;
            g.prototype_run.route_nodes_triggered += 1;
            g.prototype_run.shortcut_uses += 1;
            if (g.prototype_route_wall_index_c >= 0 && g.prototype_route_wall_index_c < g.scene.wall_count) {
                g.scene.walls[g.prototype_route_wall_index_c].active = true;
            }
            show_text("MASTERY LINE LIVE");
        }
    }
}

static void movement_update_recovery(void) {
    if (g.prototype_run.recovery_uses > 0) return;
    if (g.player.camera.position.y > 0.25f) return;
    if (g.player.camera.position.z > -8.5f) return;
    if (g.prototype_recovery_wall_index >= 0 && g.prototype_recovery_wall_index < g.scene.wall_count) {
        g.scene.walls[g.prototype_recovery_wall_index].active = true;
    }
    g.prototype_run.recovery_uses += 1;
    show_text("SCRAMBLE ROUTE LIVE");
}

static void shooter_handle_interactions(void) {
    if (!IsKeyPressed(KEY_E) || g.prototype_eval_active) return;
    for (int i = 0; i < g.scene.object_count; i++) {
        InteractObject *obj = &g.scene.objects[i];
        if (!obj->active) continue;
        if (Vector3Distance(g.player.camera.position, obj->pos) > obj->radius) continue;

        if (strcmp(obj->name, "breach_cell") == 0) {
            g.prototype_alt_shot_cooldown = 0.0f;
            g.prototype_run.recharge_pickups += 1;
            show_text("BREACH CHARGED");
            return;
        }
        if (strcmp(obj->name, "coolant_tap") == 0) {
            g.prototype_shooter_heat = 0.0f;
            g.prototype_shooter_overload_timer = 0.0f;
            g.prototype_run.recharge_pickups += 1;
            show_text("HEAT DUMPED");
            return;
        }
        if (strcmp(obj->name, "signal_console") == 0) {
            g.prototype_shooter_suppression_pause = 2.4f;
            g.prototype_run.recharge_pickups += 1;
            show_text("WINDOW STOLEN");
            return;
        }
    }
}

static void puzzle_refresh_lighting(void) {
    bool right_route = g.prototype_puzzle_router_state == 1;
    for (int i = 0; i < 4; i++) {
        float r = 0.18f;
        float gcol = 0.34f;
        float b = 0.52f;
        if (i == 0 && g.prototype_puzzle_stage >= 1) { r = 0.36f; gcol = 0.84f; b = 0.72f; }
        if (i == 1) {
            r = right_route ? 0.18f : 0.78f;
            gcol = right_route ? 0.28f : 0.66f;
            b = right_route ? 0.26f : 0.36f;
        }
        if (i == 2) {
            r = right_route ? 0.36f : 0.16f;
            gcol = right_route ? 0.84f : 0.26f;
            b = right_route ? 0.72f : 0.22f;
        }
        if (i == 3 && g.prototype_gate_open) { r = 0.34f; gcol = 0.92f; b = 0.52f; }
        SetPointLightIdx(&g.lighting,
                         i,
                         g.lighting.activePreset.pointPos[i].x,
                         g.lighting.activePreset.pointPos[i].y,
                         g.lighting.activePreset.pointPos[i].z,
                         r,
                         gcol,
                         b,
                         g.lighting.activePreset.pointRadius[i]);
    }
}

static void prototype_common_post_player(float dt) {
    UpdateEVAudio(&g.audio, g.player.moving, g.player.sprinting, g.scene.surface, dt);

    if (g.prototype_shot_cooldown > 0.0f) {
        g.prototype_shot_cooldown -= dt;
        if (g.prototype_shot_cooldown < 0.0f) g.prototype_shot_cooldown = 0.0f;
    }
    if (g.prototype_alt_shot_cooldown > 0.0f) {
        g.prototype_alt_shot_cooldown -= dt;
        if (g.prototype_alt_shot_cooldown < 0.0f) g.prototype_alt_shot_cooldown = 0.0f;
    }
    if (g.prototype_shot_flash_timer > 0.0f) {
        g.prototype_shot_flash_timer -= dt;
        if (g.prototype_shot_flash_timer < 0.0f) g.prototype_shot_flash_timer = 0.0f;
    }
    if (g.prototype_hit_flash_timer > 0.0f) {
        g.prototype_hit_flash_timer -= dt;
        if (g.prototype_hit_flash_timer < 0.0f) g.prototype_hit_flash_timer = 0.0f;
    }
    if (g.prototype_miss_flash_timer > 0.0f) {
        g.prototype_miss_flash_timer -= dt;
        if (g.prototype_miss_flash_timer < 0.0f) g.prototype_miss_flash_timer = 0.0f;
    }
    if (g.prototype_breach_flash_timer > 0.0f) {
        g.prototype_breach_flash_timer -= dt;
        if (g.prototype_breach_flash_timer < 0.0f) g.prototype_breach_flash_timer = 0.0f;
    }
    if (g.prototype_bank_flash_timer > 0.0f) {
        g.prototype_bank_flash_timer -= dt;
        if (g.prototype_bank_flash_timer < 0.0f) g.prototype_bank_flash_timer = 0.0f;
    }
    if (g.prototype_hook_flash_timer > 0.0f) {
        g.prototype_hook_flash_timer -= dt;
        if (g.prototype_hook_flash_timer < 0.0f) g.prototype_hook_flash_timer = 0.0f;
    }
    if (g.prototype_shooter_heat > 0.0f) {
        g.prototype_shooter_heat -= dt * 0.16f;
        if (g.prototype_shooter_heat < 0.0f) g.prototype_shooter_heat = 0.0f;
    }
    if (g.prototype_shooter_overload_timer > 0.0f) {
        g.prototype_shooter_overload_timer -= dt;
        if (g.prototype_shooter_overload_timer < 0.0f) g.prototype_shooter_overload_timer = 0.0f;
    }
    if (g.prototype_shooter_suppression_pause > 0.0f) {
        g.prototype_shooter_suppression_pause -= dt;
        if (g.prototype_shooter_suppression_pause < 0.0f) g.prototype_shooter_suppression_pause = 0.0f;
    }
    if (g.prototype_movement_finish_window > 0.0f) {
        g.prototype_movement_finish_window -= dt;
        if (g.prototype_movement_finish_window <= 0.0f) {
            g.prototype_movement_finish_window = 0.0f;
            prototype_close_gate();
            if (g.prototype_route_wall_index_b >= 0 && g.prototype_route_wall_index_b < g.scene.wall_count) {
                g.scene.walls[g.prototype_route_wall_index_b].active = false;
            }
        }
    }
    if (g.prototype_movement_route_timer > 0.0f) {
        g.prototype_movement_route_timer -= dt;
        if (g.prototype_movement_route_timer < 0.0f) g.prototype_movement_route_timer = 0.0f;
    }

    prototype_apply_grapple(dt);
    prototype_track_motion_metrics(dt);

    if (IsKeyPressed(KEY_R) && !g.prototype_eval_active) {
        prototype_restart_scene();
    }
    if (IsKeyPressed(KEY_TAB)) {
        g.prototype_active = PROTOTYPE_NONE;
        load_state(STATE_PROTO_LAB);
        g.fade_alpha = 0.0f;
        g.fade_target = 0.0f;
    }
}

static void prototype_check_finish(float radius) {
    if (g.prototype_eval_active || !g.scene.has_exit) return;
    if (Vector3Distance(g.player.camera.position, g.scene.exit_pos) < radius) {
        prototype_mark_meaningful_action();
        prototype_start_eval();
    }
}

static void prototype_handle_fall_reset(float min_y) {
    if (g.player.camera.position.y < min_y && !g.prototype_eval_active) {
        prototype_restart_scene();
    }
}

typedef struct {
    float t;
    Vector3 point;
    Vector3 normal;
} PrototypeRayHit;

static bool ray_hits_wall(const Wall *w, Vector3 origin, Vector3 dir, PrototypeRayHit *out_hit) {
    if (!w || !w->active) return false;

    if (w->shape == SHAPE_SPHERE) {
        float radius = w->size.x * 0.5f;
        Vector3 oc = Vector3Subtract(origin, w->pos);
        float a = Vector3DotProduct(dir, dir);
        float b = 2.0f * Vector3DotProduct(oc, dir);
        float c = Vector3DotProduct(oc, oc) - radius * radius;
        float disc = b * b - 4.0f * a * c;
        if (disc < 0.0f) return false;
        float t = (-b - sqrtf(disc)) / (2.0f * a);
        if (t <= 0.0f) return false;
        if (out_hit) {
            out_hit->t = t;
            out_hit->point = Vector3Add(origin, Vector3Scale(dir, t));
            out_hit->normal = Vector3Normalize(Vector3Subtract(out_hit->point, w->pos));
        }
        return true;
    }

    Vector3 half = {w->size.x * 0.5f, w->size.y * 0.5f, w->size.z * 0.5f};
    Vector3 min = Vector3Subtract(w->pos, half);
    Vector3 max = Vector3Add(w->pos, half);

    float tmin = -1e9f;
    float tmax = 1e9f;
    float *o = (float *)&origin;
    float *d = (float *)&dir;
    float *mn = (float *)&min;
    float *mx = (float *)&max;

    for (int axis = 0; axis < 3; axis++) {
        if (fabsf(d[axis]) < 1e-6f) {
            if (o[axis] < mn[axis] || o[axis] > mx[axis]) return false;
        } else {
            float t1 = (mn[axis] - o[axis]) / d[axis];
            float t2 = (mx[axis] - o[axis]) / d[axis];
            if (t1 > t2) {
                float tmp = t1;
                t1 = t2;
                t2 = tmp;
            }
            if (t1 > tmin) tmin = t1;
            if (t2 < tmax) tmax = t2;
            if (tmin > tmax) return false;
        }
    }

    if (tmin <= 0.0f) return false;
    if (out_hit) {
        out_hit->t = tmin;
        out_hit->point = Vector3Add(origin, Vector3Scale(dir, tmin));
        Vector3 local = Vector3Subtract(out_hit->point, w->pos);
        float ax = fabsf(local.x / half.x);
        float ay = fabsf(local.y / half.y);
        float az = fabsf(local.z / half.z);
        if (ax >= ay && ax >= az) out_hit->normal = (Vector3){local.x >= 0.0f ? 1.0f : -1.0f, 0.0f, 0.0f};
        else if (ay >= ax && ay >= az) out_hit->normal = (Vector3){0.0f, local.y >= 0.0f ? 1.0f : -1.0f, 0.0f};
        else out_hit->normal = (Vector3){0.0f, 0.0f, local.z >= 0.0f ? 1.0f : -1.0f};
    }
    return true;
}

static void prototype_update_grapple_visuals(void) {
    for (int i = 0; i < g.scene.object_count; i++) {
        InteractObject *obj = &g.scene.objects[i];
        if (!prototype_object_is_anchor(obj)) continue;
        float pulse = 0.5f + 0.5f * sinf(g.state_time * 4.4f + (float)i);
        obj->color = (Color){
            (unsigned char)(100 + pulse * 35.0f),
            (unsigned char)(220 + pulse * 30.0f),
            (unsigned char)(210 + pulse * 28.0f),
            255
        };
    }
}

static void prototype_try_grapple(void) {
    if (!IsMouseButtonPressed(MOUSE_BUTTON_MIDDLE) || g.prototype_eval_active) return;
    g.prototype_run.grapples_fired += 1;

    if (g.prototype_grapple_cooldown > 0.0f) {
        show_text("GRAPPLE COOLDOWN");
        return;
    }

    Vector3 origin = g.player.camera.position;
    Vector3 forward = Vector3Normalize(Vector3Subtract(g.player.camera.target, g.player.camera.position));
    Vector3 anchor = {0};
    const char *name = NULL;
    if (!prototype_find_best_anchor(origin, forward, 18.0f, &anchor, &name)) {
        show_text("NO ANCHOR");
        return;
    }

    g.prototype_grapple_active = true;
    g.prototype_grapple_timer = 0.42f;
    g.prototype_grapple_cooldown = 0.9f;
    g.prototype_hook_flash_timer = 0.28f;
    g.prototype_grapple_anchor = anchor;
    g.prototype_run.grapples_latched += 1;
    prototype_mark_meaningful_action();
    kick_camera(&g.player, -0.06f, 0.0f);
    SetPostFXFlash(&g.postfx, 0.16f, 0.50f, 1.0f, 0.85f);
    if (name && strcmp(name, "anchor_finish") == 0) show_text("FINISH ANCHOR");
    else if (name && strcmp(name, "anchor_flank") == 0) show_text("FLANK ANCHOR");
    else if (name && strcmp(name, "anchor_safe") == 0) show_text("SAFE ANCHOR");
    else show_text("ANCHOR LIVE");
}

static void prototype_apply_grapple(float dt) {
    if (g.prototype_grapple_cooldown > 0.0f) {
        g.prototype_grapple_cooldown -= dt;
        if (g.prototype_grapple_cooldown < 0.0f) g.prototype_grapple_cooldown = 0.0f;
    }
    if (!g.prototype_grapple_active) return;

    g.prototype_grapple_timer -= dt;
    Vector3 to_anchor = Vector3Subtract(g.prototype_grapple_anchor, g.player.camera.position);
    float dist = Vector3Length(to_anchor);
    if (dist > 0.001f) {
        Vector3 dir = Vector3Scale(to_anchor, 1.0f / dist);
        Vector3 step = Vector3Scale(dir, 17.5f * dt);
        if (Vector3Length(step) > dist) step = to_anchor;
        Vector3 new_pos = Vector3Add(g.player.camera.position, step);
        Vector3 look = Vector3Normalize(Vector3Subtract(g.player.camera.target, g.player.camera.position));
        g.player.camera.position = new_pos;
        g.player.camera.target = Vector3Add(new_pos, look);
        g.player.vel.x = dir.x * 10.0f;
        g.player.vel.z = dir.z * 10.0f;
        g.player.vy = dir.y * 8.0f;
        g.player.grounded = false;
    }

    if (dist < 1.6f) {
        Vector3 exit_dir = Vector3Normalize(Vector3Subtract(g.player.camera.target, g.player.camera.position));
        g.player.vel.x += exit_dir.x * 7.0f;
        g.player.vel.z += exit_dir.z * 7.0f;
        g.player.vy += 2.5f;
        g.prototype_grapple_active = false;
        return;
    }

    if (g.prototype_grapple_timer <= 0.0f) {
        g.prototype_grapple_active = false;
    }
}

static const char *prototype_anchor_prompt_name(const char *name) {
    if (!name) return "ANCHOR";
    if (strcmp(name, "anchor_safe") == 0) return "SAFE ANCHOR";
    if (strcmp(name, "anchor_flank") == 0) return "FLANK ANCHOR";
    if (strcmp(name, "anchor_finish") == 0) return "FINISH ANCHOR";
    if (strcmp(name, "anchor_route") == 0) return "ROUTE ANCHOR";
    if (strcmp(name, "anchor_puzzle") == 0) return "PUZZLE ANCHOR";
    return "ANCHOR";
}

static int shooter_trace_targets(Vector3 origin, Vector3 dir, int ignore_idx, bool breach, bool banked, bool *out_hit_armor) {
    PrototypeRayHit hits[16];
    int hit_slot[16];
    int hit_count = 0;
    for (int i = 0; i < g.prototype_wall_count; i++) {
        int idx = g.prototype_wall_indices[i];
        if (idx < 0 || idx >= g.scene.wall_count || idx == ignore_idx) continue;
        Wall *w = &g.scene.walls[idx];
        PrototypeRayHit hit;
        if (!ray_hits_wall(w, origin, dir, &hit)) continue;
        hits[hit_count] = hit;
        hit_slot[hit_count] = i;
        hit_count += 1;
    }

    for (int i = 0; i < hit_count - 1; i++) {
        for (int j = i + 1; j < hit_count; j++) {
            if (hits[j].t < hits[i].t) {
                PrototypeRayHit tmp_hit = hits[i];
                int tmp_slot = hit_slot[i];
                hits[i] = hits[j];
                hit_slot[i] = hit_slot[j];
                hits[j] = tmp_hit;
                hit_slot[j] = tmp_slot;
            }
        }
    }

    int kills = 0;
    bool armored_ping = false;
    bool anchor_bonus = g.prototype_grapple_active || g.prototype_grapple_cooldown > 0.55f;
    for (int i = 0; i < hit_count; i++) {
        int slot = hit_slot[i];
        int idx = g.prototype_wall_indices[slot];
        if (idx < 0 || idx >= g.scene.wall_count) continue;
        Wall *w = &g.scene.walls[idx];
        if (!w->active) continue;

        ShooterTargetType type = shooter_target_type(slot);
        if (type == SHOOTER_TARGET_ARMORED && !breach) {
            armored_ping = true;
            break;
        }

        w->active = false;
        g.prototype_targets_hit += 1;
        g.prototype_run.shots_hit += 1;
        if (banked) g.prototype_run.bank_shot_hits += 1;
        else g.prototype_run.direct_hits += 1;
        kills += 1;

        if (type == SHOOTER_TARGET_ARMORED) {
            g.prototype_run.armored_kills += 1;
            if (breach) g.prototype_run.breach_kills += 1;
        }
        if (type == SHOOTER_TARGET_THREAT) {
            g.prototype_threats_disabled += 1;
            if (anchor_bonus || banked) g.prototype_run.anchor_assisted_clears += 1;
        }
        if (!breach) break;
        if (kills >= 2) break;
    }

    if (out_hit_armor) *out_hit_armor = armored_ping;
    return kills;
}

static bool shooter_find_first_ricochet(Vector3 origin, Vector3 dir, PrototypeRayHit *out_hit, int *out_idx) {
    bool found = false;
    float best_t = 1e9f;
    for (int i = 0; i < g.prototype_ricochet_count; i++) {
        int idx = g.prototype_ricochet_indices[i];
        if (idx < 0 || idx >= g.scene.wall_count) continue;
        PrototypeRayHit hit;
        if (!ray_hits_wall(&g.scene.walls[idx], origin, dir, &hit)) continue;
        if (hit.t < best_t) {
            best_t = hit.t;
            found = true;
            if (out_hit) *out_hit = hit;
            if (out_idx) *out_idx = idx;
        }
    }
    return found;
}

static void shooter_handle_fire(void) {
    if (g.prototype_eval_active || g.prototype_shooter_overload_timer > 0.0f) return;

    bool primary = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    bool breach = IsMouseButtonPressed(MOUSE_BUTTON_RIGHT);
    if (!primary && !breach) return;
    if (breach && g.prototype_alt_shot_cooldown > 0.0f) return;
    if (primary && g.prototype_shot_cooldown > 0.0f) return;

    float heat_gain = breach ? 0.28f : 0.16f;
    if (g.player.dashing) heat_gain *= 0.72f;
    if (!g.player.grounded) heat_gain *= 0.84f;
    g.prototype_shooter_heat += heat_gain;
    if (g.prototype_shooter_heat >= 1.0f) {
        g.prototype_shooter_heat = 1.0f;
        g.prototype_shooter_overload_timer = 1.0f;
        g.prototype_miss_flash_timer = 0.14f;
        SetPostFXFlash(&g.postfx, 0.22f, 1.0f, 0.42f, 0.22f);
        show_text("WEAPON OVERHEAT");
        return;
    }

    if (breach) {
        g.prototype_alt_shot_cooldown = 1.0f;
        g.prototype_shot_flash_timer = 0.14f;
        g.prototype_breach_flash_timer = 0.30f;
        g.prototype_run.breach_uses += 1;
        kick_camera(&g.player, 0.30f, 0.03f);
        SetPostFXFlash(&g.postfx, 0.24f, 0.34f, 0.84f, 1.0f);
    } else {
        g.prototype_shot_cooldown = 0.18f - g.prototype_shooter_heat * 0.04f;
        if (g.prototype_shot_cooldown < 0.10f) g.prototype_shot_cooldown = 0.10f;
        g.prototype_shot_flash_timer = 0.09f;
        kick_camera(&g.player, 0.18f, 0.02f);
        SetPostFXFlash(&g.postfx, 0.18f, 1.0f, 0.62f, 0.28f);
    }

    g.prototype_run.shots_fired += 1;
    prototype_mark_meaningful_action();

    Vector3 origin = g.player.camera.position;
    Vector3 dir = Vector3Normalize(Vector3Subtract(g.player.camera.target, g.player.camera.position));
    int kills = 0;
    bool armored_ping = false;
    bool overload_bonus = false;
    bool banked = false;

    if (!breach) {
        PrototypeRayHit bank_hit;
        int bank_idx = -1;
        float target_t = 1e9f;
        for (int i = 0; i < g.prototype_wall_count; i++) {
            int idx = g.prototype_wall_indices[i];
            if (idx < 0 || idx >= g.scene.wall_count) continue;
            PrototypeRayHit hit;
            if (!ray_hits_wall(&g.scene.walls[idx], origin, dir, &hit)) continue;
            if (hit.t < target_t) {
                target_t = hit.t;
            }
        }

        if (shooter_find_first_ricochet(origin, dir, &bank_hit, &bank_idx) &&
            (!isfinite(target_t) || bank_hit.t < target_t)) {
            Vector3 bank_origin = Vector3Add(bank_hit.point, Vector3Scale(bank_hit.normal, 0.12f));
            Vector3 bank_dir = Vector3Normalize(Vector3Reflect(dir, bank_hit.normal));
            g.prototype_run.bank_shots_attempted += 1;
            banked = true;
            kills = shooter_trace_targets(bank_origin, bank_dir, bank_idx, false, true, &armored_ping);
            if (kills <= 0 && !armored_ping) {
                kills = shooter_trace_targets(origin, dir, -1, false, false, &armored_ping);
                banked = false;
            }
        } else {
            kills = shooter_trace_targets(origin, dir, -1, false, false, &armored_ping);
        }
    } else {
        kills = shooter_trace_targets(origin, dir, -1, true, false, &armored_ping);
    }

    if (kills > 0) {
        overload_bonus = !g.player.grounded || g.player.dashing || g.prototype_grapple_active || g.prototype_grapple_cooldown > 0.55f;
    }

    if (kills > 0) {
        g.prototype_hit_flash_timer = 0.16f;
        g.prototype_miss_flash_timer = 0.0f;
        kick_camera(&g.player, breach ? 0.08f : 0.05f, 0.0f);
        if (breach) {
            if (kills > 1) g.prototype_run.shot_ricochets += kills - 1;
            SetPostFXFlash(&g.postfx, 0.28f, 0.30f, 0.82f, 1.0f);
            if (kills > 1) show_text("BREACH THROUGH");
            else show_text("BREACH HIT");
        } else if (banked) {
            g.prototype_bank_flash_timer = 0.22f;
            SetPostFXFlash(&g.postfx, 0.26f, 0.48f, 0.92f, 1.0f);
            if (kills > 1) show_text("BANK CHAIN");
            else show_text("BANK HIT");
        } else {
            SetPostFXFlash(&g.postfx, 0.24f, 1.0f, 0.34f, 0.18f);
        }
        if (overload_bonus) {
            g.prototype_shooter_suppression_pause = 2.4f;
            g.prototype_shooter_heat *= 0.65f;
            if (g.prototype_grapple_active || g.prototype_grapple_cooldown > 0.55f) show_text("ANCHOR WINDOW");
            else show_text("AERIAL OVERLOAD");
        } else if (g.prototype_targets_hit < g.prototype_target_total) {
            char msg[48];
            snprintf(msg, sizeof(msg), "TARGET %d / %d", g.prototype_targets_hit, g.prototype_target_total);
            show_text(msg);
        }
        if (g.prototype_targets_hit >= g.prototype_target_total) {
            prototype_open_gate();
            show_text("LANE CLEAR");
        }
    } else if (armored_ping) {
        g.prototype_miss_flash_timer = 0.12f;
        SetPostFXFlash(&g.postfx, 0.12f, 0.30f, 0.82f, 1.0f);
        show_text("ARMOR: RMB BREACH");
    } else {
        g.prototype_miss_flash_timer = 0.12f;
        SetPostFXFlash(&g.postfx, 0.12f, 0.32f, 0.60f, 1.0f);
        if (banked) show_text("BANK MISSED");
    }
}

static void shooter_update_targets(void) {
    for (int i = 0; i < g.prototype_wall_count; i++) {
        int idx = g.prototype_wall_indices[i];
        if (idx < 0 || idx >= g.scene.wall_count) continue;
        Wall *w = &g.scene.walls[idx];
        if (!w->active) continue;

        Vector3 home = g.prototype_wall_home[i];
        ShooterTargetType type = shooter_target_type(i);
        float swing = sinf(g.state_time * (1.6f + 0.22f * (float)i) + (float)i * 0.8f);
        if (type == SHOOTER_TARGET_THREAT) {
            w->pos.y = home.y + swing * 0.65f;
            w->pos.x = home.x + cosf(g.state_time * 1.5f + (float)i) * 0.45f;
        } else {
            w->pos.x = home.x + swing * (0.7f + 0.25f * (float)(i % 2));
        }
    }
}

static void puzzle_apply_stage(void) {
    if (g.prototype_wall_count < 5) return;
    for (int i = 0; i < g.prototype_wall_count; i++) {
        int idx = g.prototype_wall_indices[i];
        if (idx < 0 || idx >= g.scene.wall_count) continue;
        g.scene.walls[idx].active = false;
    }

    if (g.prototype_puzzle_stage >= 1) g.scene.walls[g.prototype_wall_indices[0]].active = true;
    if (g.prototype_puzzle_stage >= 1 && g.prototype_puzzle_router_state == 0) g.scene.walls[g.prototype_wall_indices[1]].active = true;
    if (g.prototype_puzzle_stage >= 1 && g.prototype_puzzle_router_state == 1) g.scene.walls[g.prototype_wall_indices[2]].active = true;
    if (g.prototype_puzzle_stage >= 2) g.scene.walls[g.prototype_wall_indices[3]].active = true;
    if (g.prototype_puzzle_stage >= 3) {
        bool pulse_live = fmodf(g.state_time, 1.2f) < 0.8f;
        g.scene.walls[g.prototype_wall_indices[4]].active = pulse_live;
        if (pulse_live) prototype_open_gate();
        else prototype_close_gate();
    } else {
        if (g.prototype_wall_count > 4) g.scene.walls[g.prototype_wall_indices[4]].active = false;
        prototype_close_gate();
    }
    puzzle_refresh_lighting();
}

static void puzzle_handle_interactions(void) {
    if (g.prototype_eval_active || !IsKeyPressed(KEY_E)) return;

    for (int i = 0; i < g.scene.object_count; i++) {
        InteractObject *obj = &g.scene.objects[i];
        if (!obj->active) continue;
        if (Vector3Distance(g.player.camera.position, obj->pos) > obj->radius) continue;

        g.prototype_run.puzzle_actions += 1;
        g.prototype_run.relay_interactions += 1;
        prototype_mark_meaningful_action();

        if (strcmp(obj->name, "relay_a") == 0) {
            if (g.prototype_puzzle_stage == 0) {
                g.prototype_puzzle_stage = 1;
                g.prototype_run.puzzle_stage_clears += 1;
                show_text("ROOM 1 LIVE");
            } else {
                g.prototype_run.puzzle_misreads += 1;
                g.prototype_run.invalid_states += 1;
                show_text("ENTRY ALREADY POWERED");
            }
        } else if (strcmp(obj->name, "router") == 0) {
            if (g.prototype_puzzle_stage < 1) {
                g.prototype_run.puzzle_misreads += 1;
                g.prototype_run.invalid_states += 1;
                show_text("POWER THE ENTRY FIRST");
            } else {
                g.prototype_puzzle_router_state = 1 - g.prototype_puzzle_router_state;
                show_text(g.prototype_puzzle_router_state == 1 ? "ROUTE RIGHT" : "ROUTE LEFT");
            }
        } else if (strcmp(obj->name, "relay_b") == 0) {
            if (g.prototype_puzzle_stage == 1 && g.prototype_puzzle_router_state == 1) {
                g.prototype_puzzle_stage = 2;
                g.prototype_run.puzzle_stage_clears += 1;
                show_text("ROOM 2 CLEAR");
            } else {
                g.prototype_run.puzzle_misreads += 1;
                g.prototype_run.invalid_states += 1;
                show_text("THE ACTIVE LINE IS WRONG");
            }
        } else if (strcmp(obj->name, "relay_c") == 0) {
            if (g.prototype_puzzle_stage == 2) {
                g.prototype_puzzle_stage = 3;
                g.prototype_run.puzzle_stage_clears += 1;
                show_text("FINAL PULSE LIVE");
            } else {
                g.prototype_run.puzzle_misreads += 1;
                g.prototype_run.invalid_states += 1;
                show_text("FOLLOW THE POWER PATH");
            }
        }

        puzzle_apply_stage();
        return;
    }
}

void prototype_lab_load(void) {
    build_prototype_lab_room(&g.scene);
    g.prototype_active = PROTOTYPE_NONE;
    g.prototype_eval_active = false;
    StopAmbient(&g.audio);
    StopClockAmbient(&g.audio);
    set_exposure(-0.1f);
    SetPostFXGrain(&g.postfx, 0.12f);
    SetPostFXWarmth(&g.postfx, 0.03f);
    SetPostFXCA(&g.postfx, 0.0f);
    hide_text();
}

static void prototype_lab_cycle_slice(int dir) {
    int next = (int)g.prototype_selected + dir;
    if (next <= PROTOTYPE_NONE) next = PROTOTYPE_PUZZLE;
    if (next >= PROTOTYPE_COUNT) next = PROTOTYPE_MOVEMENT;
    g.prototype_selected = (PrototypeId)next;
}

void prototype_lab_update(float dt) {
    (void)dt;
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) prototype_lab_cycle_slice(1);
    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) prototype_lab_cycle_slice(-1);
    if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) {
        g.prototype_lab_action = (PrototypeLabAction)((g.prototype_lab_action + 1) % PROTO_LAB_ACTION_COUNT);
    }
    if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) {
        int next = (int)g.prototype_lab_action - 1;
        if (next < 0) next = PROTO_LAB_ACTION_COUNT - 1;
        g.prototype_lab_action = (PrototypeLabAction)next;
    }
    if (g.state_time <= 0.25f) return;
    if (!(IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE))) return;

    const PrototypeSlice *slice = prototype_slice_by_id(g.prototype_selected);
    if (!slice) return;

    if (g.prototype_lab_action == PROTO_LAB_PLAY) {
        load_state(slice->state);
        g.fade_alpha = 0.0f;
        g.fade_target = 0.0f;
        return;
    }
    if (g.prototype_lab_action == PROTO_LAB_RESET) {
        prototype_reset_slice_records(g.prototype_selected);
        show_text("SLICE SCORES RESET");
        return;
    }
}

void prototype_movement_load(void) {
    prototype_begin_scene(PROTOTYPE_MOVEMENT);
    build_movement_slice(&g.scene);
    init_player(&g.player, g.scene.spawn);
    prototype_common_load();
    apply_movement_slice_lighting();
    g.prototype_prev_pos = g.player.camera.position;
}

void prototype_movement_update(float dt) {
    prototype_update_eval_input();
    if (!g.prototype_eval_active) {
        prototype_update_grapple_visuals();
        update_player(&g.player, &g.scene, dt);
        prototype_try_grapple();
        movement_handle_interactions();
        movement_update_recovery();
        movement_refresh_route();
        prototype_common_post_player(dt);
        prototype_handle_fall_reset(-4.0f);
        prototype_check_finish(1.8f);
    }
}

void prototype_shooter_load(void) {
    prototype_begin_scene(PROTOTYPE_SHOOTER);
    build_shooter_slice(&g.scene);
    init_player(&g.player, g.scene.spawn);
    prototype_common_load();
    apply_shooter_slice_lighting();
    shooter_refresh_lighting();
    g.prototype_prev_pos = g.player.camera.position;
}

void prototype_shooter_update(float dt) {
    prototype_update_eval_input();
    if (!g.prototype_eval_active) {
        prototype_update_grapple_visuals();
        update_player(&g.player, &g.scene, dt);
        prototype_try_grapple();
        shooter_update_targets();
        shooter_refresh_target_visuals();
        shooter_refresh_lane_visuals();
        shooter_handle_interactions();
        shooter_handle_fire();
        shooter_update_suppression();
        prototype_common_post_player(dt);
        if (g.prototype_targets_hit >= g.prototype_target_total) prototype_open_gate();
        shooter_refresh_lighting();
        prototype_handle_fall_reset(-4.0f);
        prototype_check_finish(2.0f);
    }
}

void prototype_puzzle_load(void) {
    prototype_begin_scene(PROTOTYPE_PUZZLE);
    build_puzzle_slice(&g.scene);
    init_player(&g.player, g.scene.spawn);
    prototype_common_load();
    apply_puzzle_slice_lighting();
    puzzle_apply_stage();
    g.prototype_prev_pos = g.player.camera.position;
}

void prototype_puzzle_update(float dt) {
    prototype_update_eval_input();
    if (!g.prototype_eval_active) {
        prototype_update_grapple_visuals();
        update_player(&g.player, &g.scene, dt);
        prototype_try_grapple();
        puzzle_handle_interactions();
        puzzle_apply_stage();
        prototype_common_post_player(dt);
        prototype_handle_fall_reset(-5.0f);
        prototype_check_finish(1.8f);
    }
}

static void draw_lab_action_tabs(void) {
    int x = 40;
    for (int i = 0; i < PROTO_LAB_ACTION_COUNT; i++) {
        bool active = (int)g.prototype_lab_action == i;
        int w = (i == PROTO_LAB_COMPARE) ? 146 : 124;
        DrawRectangle(x, 104, w, 24, active ? (Color){50, 64, 88, 255} : (Color){18, 22, 30, 255});
        DrawRectangleLines(x, 104, w, 24, active ? (Color){108, 154, 220, 255} : (Color){42, 48, 58, 255});
        DrawText(prototype_lab_action_labels[i], x + 8, 112, 9, active ? (Color){236, 234, 228, 255} : (Color){166, 166, 158, 255});
        x += w + 10;
    }
}

static void draw_slice_summary_card(const PrototypeSlice *slice, int y, bool selected) {
    Color box = selected ? (Color){46, 58, 80, 255} : (Color){22, 26, 34, 255};
    Color edge = selected ? (Color){108, 154, 220, 255} : (Color){38, 42, 50, 255};
    DrawRectangle(40, y, 428, 74, box);
    DrawRectangleLines(40, y, 428, 74, edge);
    DrawText(slice->title, 54, y + 10, 16, (Color){236, 232, 220, 255});
    DrawText(slice->subtitle, 54, y + 30, 10, (Color){164, 164, 156, 255});

    PrototypeRunStats run = g.prototype_last_run[slice->id];
    PrototypeEval eval = g.prototype_last_eval[slice->id];
    char row[180];
    snprintf(row, sizeof(row), "Last %.1fs  resets %d  first action %.1fs",
             run.completed ? run.completion_time : 0.0f,
             run.resets,
             run.first_meaningful_action_time);
    DrawText(run.completed ? row : "No completed run yet.", 54, y + 50, 9, (Color){190, 190, 182, 255});
    if (eval.submitted) {
        char ratings[120];
        snprintf(ratings, sizeof(ratings), "Replay %d  Read %d  Depth %d  Ship %d",
                 eval.would_replay, eval.readability, eval.mechanical_depth, eval.ship_confidence);
        DrawText(ratings, 260, y + 10, 9, (Color){132, 174, 138, 255});
    }
}

static void draw_prototype_review_panel(const PrototypeSlice *slice) {
    PrototypeRunStats last = g.prototype_last_run[slice->id];
    PrototypeRunStats best = g.prototype_best_run[slice->id];
    PrototypeEval eval = g.prototype_last_eval[slice->id];
    DrawRectangle(500, 144, 620, 324, (Color){12, 14, 18, 230});
    DrawRectangleLines(500, 144, 620, 324, (Color){54, 60, 74, 255});
    DrawText("SLICE REVIEW", 524, 160, 18, (Color){240, 236, 224, 255});
    DrawText(slice->qa.core_question, 524, 184, 10, (Color){176, 176, 168, 255});
    DrawText(slice->qa.allowed_verbs, 524, 204, 9, (Color){150, 186, 222, 255});
    DrawText(slice->qa.success_condition, 524, 224, 9, (Color){178, 204, 168, 255});
    DrawText(slice->qa.score_fields, 524, 244, 9, (Color){206, 194, 154, 255});

    char last_row[256];
    snprintf(last_row, sizeof(last_row),
             "Last run  %.1fs  resets %d  dist %.0fm  jumps %d  dashes %d",
             last.completion_time, last.resets, last.distance, last.jumps, last.dashes);
    DrawText(last.completed ? last_row : "Last run  none", 524, 278, 10, (Color){210, 210, 202, 255});

    char best_row[256];
    snprintf(best_row, sizeof(best_row),
             "Best run  %.1fs  shortcuts %d  recoveries %d  armored %d  invalid %d",
             best.completion_time, best.shortcut_uses, best.recovery_uses, best.armored_kills, best.invalid_states);
    DrawText(best.completed ? best_row : "Best run  none", 524, 296, 10, (Color){188, 224, 188, 255});

    if (eval.submitted) {
        char eval_row[256];
        snprintf(eval_row, sizeof(eval_row),
                 "Replay %d  Read %d  Depth %d  Ship %d",
                 eval.would_replay, eval.readability, eval.mechanical_depth, eval.ship_confidence);
        DrawText(eval_row, 524, 328, 10, (Color){220, 220, 208, 255});
        DrawText(prototype_best_moment_labels[slice->id][eval.best_moment], 524, 348, 10, (Color){142, 212, 255, 255});
        DrawText(prototype_main_friction_labels[slice->id][eval.main_friction], 524, 366, 10, (Color){255, 178, 142, 255});
    } else {
        DrawText("No evaluation submitted yet.", 524, 328, 10, (Color){170, 170, 164, 255});
    }

    DrawText("ENTER on PLAY starts the slice. ENTER on RESET wipes last/best/eval for the selected slice.", 524, 430, 8, (Color){144, 144, 138, 255});
}

static void draw_prototype_compare_panel(void) {
    DrawRectangle(500, 144, 620, 324, (Color){12, 14, 18, 230});
    DrawRectangleLines(500, 144, 620, 324, (Color){54, 60, 74, 255});
    DrawText("COMPARE LAST RUNS", 524, 160, 18, (Color){240, 236, 224, 255});
    DrawText("Latest grade equivalent is subjective here: compare last run quality against best run deltas.", 524, 184, 9, (Color){176, 176, 168, 255});

    int y = 214;
    for (int i = 0; i < (int)(sizeof(prototype_slices) / sizeof(prototype_slices[0])); i++) {
        const PrototypeSlice *slice = &prototype_slices[i];
        PrototypeRunStats last = g.prototype_last_run[slice->id];
        PrototypeRunStats best = g.prototype_best_run[slice->id];
        PrototypeEval eval = g.prototype_last_eval[slice->id];
        int delta = prototype_run_score(&last) - prototype_run_score(&best);
        if (!best.completed) delta = 0;

        DrawText(slice->qa.display_name, 524, y, 11, (Color){236, 232, 220, 255});
        char row[256];
        snprintf(row, sizeof(row),
                 "last %.1fs  best %.1fs  delta %s%d  replay %d  read %d  depth %d  ship %d",
                 last.completed ? last.completion_time : 0.0f,
                 best.completed ? best.completion_time : 0.0f,
                 delta >= 0 ? "+" : "",
                 delta,
                 eval.submitted ? eval.would_replay : 0,
                 eval.submitted ? eval.readability : 0,
                 eval.submitted ? eval.mechanical_depth : 0,
                 eval.submitted ? eval.ship_confidence : 0);
        DrawText(last.completed ? row : "no completed run yet", 524, y + 14, 9, (Color){192, 192, 184, 255});
        y += 48;
    }
}

void draw_prototype_lab(void) {
    ClearBackground((Color){8, 10, 14, 255});

    DrawText("DANTE PROTOTYPE LAB", 44, 34, 24, (Color){240, 236, 224, 255});
    DrawText("Race the 3D slices. Judge them on feel, replay pull, and production fit.", 44, 64, 12, (Color){176, 176, 168, 255});
    DrawText("W-S slice  A-D mode  ENTER act  /  P from title  /  Esc menu route stays intact", 44, 82, 10, (Color){132, 132, 126, 255});

    draw_lab_action_tabs();

    int y = 150;
    for (int i = 0; i < (int)(sizeof(prototype_slices) / sizeof(prototype_slices[0])); i++) {
        const PrototypeSlice *slice = &prototype_slices[i];
        draw_slice_summary_card(slice, y, slice->id == g.prototype_selected);
        y += 88;
    }

    const PrototypeSlice *selected = prototype_slice_by_id(g.prototype_selected);
    if (selected) {
        if (g.prototype_lab_action == PROTO_LAB_COMPARE) draw_prototype_compare_panel();
        else draw_prototype_review_panel(selected);
    }
}

static bool prototype_shooter_has_bank_line(Vector3 origin, Vector3 dir) {
    PrototypeRayHit bank_hit;
    int bank_idx = -1;
    if (!shooter_find_first_ricochet(origin, dir, &bank_hit, &bank_idx)) return false;

    float target_t = 1e9f;
    for (int i = 0; i < g.prototype_wall_count; i++) {
        int idx = g.prototype_wall_indices[i];
        if (idx < 0 || idx >= g.scene.wall_count) continue;
        PrototypeRayHit hit;
        if (!ray_hits_wall(&g.scene.walls[idx], origin, dir, &hit)) continue;
        if (hit.t < target_t) target_t = hit.t;
    }
    return bank_hit.t < target_t;
}

static void draw_prototype_shooter_viewmodel(bool anchor_ready, bool bank_ready) {
    float move_bob = sinf(g.state_time * 7.5f + g.prototype_run.distance * 0.08f) * 5.0f;
    float kick = g.prototype_shot_flash_timer * 42.0f + g.prototype_hit_flash_timer * 16.0f;
    float settle = g.prototype_miss_flash_timer * 10.0f;
    float slide_pull = g.prototype_shot_flash_timer * 24.0f;
    float breach_pulse = 0.5f + 0.5f * sinf(g.state_time * 9.0f);
    float breach_glow = g.prototype_alt_shot_cooldown <= 0.0f ? 1.0f : 1.0f - fminf(g.prototype_alt_shot_cooldown / 1.0f, 1.0f);
    float hook_extend = g.prototype_hook_flash_timer > 0.0f ? g.prototype_hook_flash_timer / 0.28f : 0.0f;
    float bank_flash = g.prototype_bank_flash_timer > 0.0f ? g.prototype_bank_flash_timer / 0.22f : 0.0f;
    float breach_flash = g.prototype_breach_flash_timer > 0.0f ? g.prototype_breach_flash_timer / 0.30f : 0.0f;
    int base_x = RENDER_W - 306 + (int)move_bob;
    int base_y = RENDER_H - 132 + (int)(kick - settle);

    Color frame = (Color){18, 22, 28, 248};
    Color body = (Color){74, 86, 100, 255};
    Color slide = (Color){108, 122, 138, 255};
    Color accent = (Color){90, 200, 255, 255};
    Color hot = (Color){255, 138, 92, 255};
    Color breach = (Color){150, 190, 255, 255};

    if (g.prototype_shooter_heat > 0.55f) {
        body = ColorLerp(body, hot, (g.prototype_shooter_heat - 0.55f) / 0.45f);
        slide = ColorLerp(slide, hot, (g.prototype_shooter_heat - 0.55f) / 0.45f);
    }
    if (g.prototype_alt_shot_cooldown <= 0.0f) {
        accent = ColorLerp(accent, breach, 0.65f + breach_pulse * 0.35f);
    }
    if (g.prototype_breach_flash_timer > 0.0f) {
        frame = ColorLerp(frame, breach, breach_flash * 0.45f);
        body = ColorLerp(body, breach, breach_flash * 0.60f);
        slide = ColorLerp(slide, breach, breach_flash * 0.70f);
    }

    DrawRectangle(base_x + 38, base_y + 22, 80, 92, (Color){26, 30, 38, 228});
    DrawTriangle((Vector2){(float)base_x + 18.0f, (float)base_y + 26.0f},
                 (Vector2){(float)base_x + 66.0f, (float)base_y + 8.0f},
                 (Vector2){(float)base_x + 108.0f, (float)base_y + 98.0f},
                 (Color){52, 62, 76, 240});
    DrawRectangle(base_x + 64, base_y + 46, 14, 26, accent);
    DrawRectangle(base_x + 48, base_y + 74, 24, 44, frame);
    DrawRectangle(base_x + 76, base_y + 58, 28, 52, (Color){46, 58, 72, 255});
    DrawRectangle(base_x + 20, base_y + 18, 20, 12, anchor_ready ? (Color){120, 255, 220, 255} : (Color){70, 96, 102, 220});
    DrawRectangle(base_x + 10, base_y + 10, 16, 8, (Color){182, 240, 228, 220});
    DrawRectangle(base_x + 6, base_y + 4, 10 + (int)(hook_extend * 18.0f), 4, (Color){150, 250, 226, 220});
    if (g.prototype_grapple_active) {
        DrawLineEx((Vector2){(float)base_x + 18.0f, (float)base_y + 16.0f},
                   (Vector2){(float)RENDER_W / 2.0f - 18.0f, (float)RENDER_H / 2.0f + 12.0f},
                   2.0f,
                   (Color){120, 255, 220, 220});
    }

    DrawRectangle(base_x + 68, base_y + 8, 164, 78, frame);
    DrawTriangle((Vector2){(float)base_x + 26.0f, (float)base_y + 22.0f},
                 (Vector2){(float)base_x + 126.0f, (float)base_y + 8.0f},
                 (Vector2){(float)base_x + 158.0f, (float)base_y + 78.0f},
                 body);
    DrawRectangle(base_x + 122 - (int)slide_pull, base_y + 18, 138, 30, slide);
    DrawRectangle(base_x + 206, base_y + 24, 86, 14, accent);
    DrawRectangle(base_x + 88, base_y + 60, 32, 60, frame);
    DrawRectangle(base_x + 118, base_y + 70, 38, 48, body);
    DrawRectangle(base_x + 64, base_y + 40, 44, 12, bank_ready ? (Color){150, 214, 255, 255} : accent);
    if (bank_ready || g.prototype_bank_flash_timer > 0.0f) {
        Color bank_col = ColorLerp((Color){84, 126, 156, 255}, (Color){164, 228, 255, 255}, bank_flash > 0.0f ? bank_flash : 0.55f);
        DrawRectangle(base_x + 70, base_y + 30, 18, 6, bank_col);
        DrawRectangle(base_x + 92, base_y + 30, 10, 6, bank_col);
    }
    DrawRectangle(base_x + 148, base_y + 50, 22, 10, (Color){24, 28, 32, 255});
    DrawRectangle(base_x + 96, base_y + 0, 22, 28, accent);
    DrawRectangle(base_x + 102, base_y - 12, 10, 18, (Color){220, 230, 238, 220});
    DrawRectangle(base_x + 152, base_y + 82, 66, 8, (Color){16, 20, 26, 255});
    DrawRectangle(base_x + 154, base_y + 84, (int)(62.0f * g.prototype_shooter_heat), 4, hot);
    DrawRectangle(base_x + 178, base_y + 6, 38, 8, (Color){26, 32, 38, 255});
    DrawRectangle(base_x + 180, base_y + 8, (int)(34.0f * breach_glow), 4, breach);
    DrawRectangle(base_x + 226, base_y + 44, 16, 24, (Color){14, 18, 24, 255});
    DrawRectangle(base_x + 229, base_y + 47, 10, (int)(18.0f * breach_glow), breach);

    if (g.prototype_shot_flash_timer > 0.0f) {
        float flash = g.prototype_shot_flash_timer / 0.14f;
        if (flash > 1.0f) flash = 1.0f;
        Color muzzle = (Color){255, (unsigned char)(180 + flash * 60.0f), (unsigned char)(90 + flash * 70.0f), 220};
        DrawTriangle((Vector2){(float)base_x + 292.0f, (float)base_y + 18.0f},
                     (Vector2){(float)base_x + 334.0f + flash * 14.0f, (float)base_y + 31.0f},
                     (Vector2){(float)base_x + 292.0f, (float)base_y + 44.0f},
                     muzzle);
    }
    if (g.prototype_breach_flash_timer > 0.0f) {
        DrawCircle(base_x + 246, base_y + 31, 10.0f + breach_flash * 8.0f, (Color){150, 190, 255, 60});
    }

    if (g.prototype_grapple_active || g.prototype_grapple_cooldown > 0.0f) {
        Color tether = g.prototype_grapple_active ? (Color){120, 255, 220, 220} : (Color){90, 150, 140, 180};
        DrawRectangle(base_x + 32, base_y + 12, 16, 16, tether);
        DrawLineEx((Vector2){(float)base_x + 40.0f, (float)base_y + 20.0f},
                   (Vector2){(float)base_x + 4.0f, (float)base_y - 24.0f},
                   2.0f,
                   tether);
    }
}

void draw_prototype_overlay(void) {
    const PrototypeSlice *slice = prototype_slice_by_state(g.state);
    if (!slice) return;

    DrawRectangle(12, 12, 340, 58, (Color){8, 10, 14, 180});
    DrawRectangleLines(12, 12, 340, 58, (Color){58, 62, 74, 220});
    DrawText(slice->title, 22, 22, 14, (Color){240, 236, 224, 255});
    DrawText(slice->objective, 22, 42, 9, (Color){172, 172, 164, 255});

    if (g.state == STATE_PROTO_SHOOTER) {
        int cx = RENDER_W / 2;
        int cy = RENDER_H / 2;
        Vector3 origin = g.player.camera.position;
        Vector3 forward = Vector3Normalize(Vector3Subtract(g.player.camera.target, g.player.camera.position));
        Vector3 anchor = {0};
        const char *anchor_name = NULL;
        bool anchor_ready = prototype_find_best_anchor(origin, forward, 18.0f, &anchor, &anchor_name);
        bool bank_ready = prototype_shooter_has_bank_line(origin, forward);
        draw_prototype_shooter_viewmodel(anchor_ready, bank_ready);
        DrawText("LMB pulse  RMB breach  MMB grapple  E route tools", RENDER_W - 238, 26, 8, (Color){190, 190, 182, 255});
        Color cross = (Color){220, 226, 236, 210};
        if (g.prototype_hit_flash_timer > 0.0f) cross = (Color){255, 126, 78, 255};
        else if (g.prototype_miss_flash_timer > 0.0f) cross = (Color){116, 176, 255, 255};
        else if (g.prototype_shot_flash_timer > 0.0f) cross = (Color){255, 214, 146, 255};
        DrawLine(cx - 10, cy, cx - 3, cy, cross);
        DrawLine(cx + 3, cy, cx + 10, cy, cross);
        DrawLine(cx, cy - 10, cx, cy - 3, cross);
        DrawLine(cx, cy + 3, cx, cy + 10, cross);
        if (bank_ready) DrawCircleLines(cx, cy, 14.0f, (Color){132, 208, 255, 220});
        if (anchor_ready) DrawCircleLines(cx, cy, 20.0f, (Color){120, 255, 220, 200});
        if (g.prototype_breach_flash_timer > 0.0f) DrawCircleLines(cx, cy, 26.0f, (Color){160, 196, 255, 210});
        if (bank_ready) DrawText("BANK", cx + 18, cy - 18, 8, (Color){132, 208, 255, 255});
        if (anchor_ready) DrawText("HOOK", cx + 18, cy - 8, 8, (Color){120, 255, 220, 255});
        if (!bank_ready && !anchor_ready && g.prototype_alt_shot_cooldown <= 0.0f) {
            DrawText("BREACH READY", cx + 18, cy - 8, 8, (Color){160, 196, 255, 255});
        }
    }

    char stats[320];
    snprintf(stats, sizeof(stats),
             "t %.1fs  resets %d  dist %.0fm  jumps %d  dashes %d  hits %d/%d  bank %d/%d  grapples %d/%d",
             g.state_time,
             g.prototype_run.resets,
             g.prototype_run.distance,
             g.prototype_run.jumps,
             g.prototype_run.dashes,
             g.prototype_run.shots_hit,
             g.prototype_run.shots_fired,
             g.prototype_run.bank_shot_hits,
             g.prototype_run.bank_shots_attempted,
             g.prototype_run.grapples_latched,
             g.prototype_run.grapples_fired);
    DrawRectangle(12, RENDER_H - 28, 612, 16, (Color){8, 10, 14, 160});
    DrawText(stats, 18, RENDER_H - 24, 8, (Color){204, 204, 194, 255});

    DrawText("R restart  TAB lab", RENDER_W - 98, 16, 8, (Color){170, 170, 164, 255});

    if (g.state == STATE_PROTO_SHOOTER) {
        float cycle = 1.45f + 0.16f * (float)(g.prototype_threats_disabled);
        int hot_lane = ((int)floorf(g.state_time / cycle)) % 3;
        float phase = fmodf(g.state_time, cycle) / cycle;
        const char *heat = (phase > 0.32f) ? "HOT" : "WINDING";
        Vector3 origin = g.player.camera.position;
        Vector3 forward = Vector3Normalize(Vector3Subtract(g.player.camera.target, g.player.camera.position));
        Vector3 anchor = {0};
        const char *anchor_name = NULL;
        bool anchor_ready = prototype_find_best_anchor(origin, forward, 18.0f, &anchor, &anchor_name);
        bool bank_ready = prototype_shooter_has_bank_line(origin, forward);
        char lane[140];
        snprintf(lane, sizeof(lane), "left %d  lane %d %s  heat %d%%  breach %s  hook %s",
                 g.prototype_target_total - g.prototype_targets_hit,
                 hot_lane + 1,
                 heat,
                 (int)(g.prototype_shooter_heat * 100.0f),
                 g.prototype_alt_shot_cooldown <= 0.0f ? "READY" : "COOLDOWN",
                 g.prototype_grapple_active ? "LATCHED" : (g.prototype_grapple_cooldown <= 0.0f ? "READY" : "CD"));
        DrawRectangle(RENDER_W - 372, 34, 344, 18, (Color){12, 14, 18, 180});
        DrawText(lane, RENDER_W - 364, 39, 8, (Color){232, 224, 210, 255});
        char tech[180];
        snprintf(tech, sizeof(tech), "bank %d/%d  direct %d  breach kills %d  anchor clears %d",
                 g.prototype_run.bank_shot_hits,
                 g.prototype_run.bank_shots_attempted,
                 g.prototype_run.direct_hits,
                 g.prototype_run.breach_kills,
                 g.prototype_run.anchor_assisted_clears);
        DrawRectangle(RENDER_W - 372, 54, 344, 18, (Color){12, 14, 18, 180});
        DrawText(tech, RENDER_W - 364, 59, 8, (Color){182, 220, 236, 255});
        if (g.prototype_shooter_suppression_pause > 0.0f) {
            DrawText("stolen window", RENDER_W - 108, 76, 8, (Color){132, 216, 255, 255});
        } else if (g.prototype_shooter_overload_timer > 0.0f) {
            DrawText("overheat lock", RENDER_W - 104, 76, 8, (Color){255, 164, 118, 255});
        } else if (anchor_ready && g.prototype_grapple_cooldown <= 0.0f) {
            DrawText(prototype_anchor_prompt_name(anchor_name), RENDER_W - 118, 76, 8, (Color){120, 255, 220, 255});
        } else if (bank_ready) {
            DrawText("BANK LINE OPEN", RENDER_W - 116, 76, 8, (Color){132, 208, 255, 255});
        } else {
            DrawText("HOOK. BANK. BREACH.", RENDER_W - 128, 76, 8, (Color){214, 214, 204, 255});
        }
        for (int i = 0; i < g.scene.object_count; i++) {
            InteractObject *obj = &g.scene.objects[i];
            if (!obj->active) continue;
            if (Vector3Distance(g.player.camera.position, obj->pos) > obj->radius) continue;
            DrawRectangle(RENDER_W / 2 - 84, RENDER_H - 64, 168, 16, (Color){10, 14, 18, 190});
            if (strcmp(obj->name, "breach_cell") == 0) DrawText("E recharge breach", RENDER_W / 2 - 58, RENDER_H - 59, 8, (Color){182, 228, 255, 255});
            if (strcmp(obj->name, "coolant_tap") == 0) DrawText("E vent weapon heat", RENDER_W / 2 - 58, RENDER_H - 59, 8, (Color){160, 255, 220, 255});
            if (strcmp(obj->name, "signal_console") == 0) DrawText("E steal a safe window", RENDER_W / 2 - 70, RENDER_H - 59, 8, (Color){255, 212, 150, 255});
        }
    } else if (g.state == STATE_PROTO_MOVEMENT) {
        Vector3 origin = g.player.camera.position;
        Vector3 forward = Vector3Normalize(Vector3Subtract(g.player.camera.target, g.player.camera.position));
        Vector3 anchor = {0};
        const char *anchor_name = NULL;
        bool anchor_ready = prototype_find_best_anchor(origin, forward, 18.0f, &anchor, &anchor_name);
        char route[96];
        snprintf(route, sizeof(route), "nodes %d  shortcuts %d  recoveries %d  finish %.1fs  hook %s",
                 g.prototype_run.route_nodes_triggered,
                 g.prototype_run.shortcut_uses,
                 g.prototype_run.recovery_uses,
                 g.prototype_movement_finish_window,
                 g.prototype_grapple_active ? "LATCHED" : (g.prototype_grapple_cooldown <= 0.0f ? "READY" : "CD"));
        DrawRectangle(RENDER_W - 340, 34, 312, 18, (Color){12, 14, 18, 180});
        DrawText(route, RENDER_W - 332, 39, 8, (Color){218, 228, 220, 255});
        if (anchor_ready && g.prototype_grapple_cooldown <= 0.0f) {
            DrawText(prototype_anchor_prompt_name(anchor_name), RENDER_W - 118, 56, 8, (Color){120, 255, 220, 255});
        }
    } else if (g.state == STATE_PROTO_PUZZLE) {
        Vector3 origin = g.player.camera.position;
        Vector3 forward = Vector3Normalize(Vector3Subtract(g.player.camera.target, g.player.camera.position));
        Vector3 anchor = {0};
        const char *anchor_name = NULL;
        bool anchor_ready = prototype_find_best_anchor(origin, forward, 18.0f, &anchor, &anchor_name);
        char stage[96];
        snprintf(stage, sizeof(stage), "stage %d / %d  router %s  invalid %d  hook %s",
                 g.prototype_puzzle_stage,
                 g.prototype_puzzle_goal,
                 g.prototype_puzzle_router_state == 1 ? "RIGHT" : "LEFT",
                 g.prototype_run.invalid_states,
                 g.prototype_grapple_active ? "LATCHED" : (g.prototype_grapple_cooldown <= 0.0f ? "READY" : "CD"));
        DrawRectangle(RENDER_W - 328, 34, 296, 18, (Color){12, 14, 18, 180});
        DrawText(stage, RENDER_W - 320, 39, 8, (Color){210, 232, 224, 255});
        if (anchor_ready && g.prototype_grapple_cooldown <= 0.0f) {
            DrawText(prototype_anchor_prompt_name(anchor_name), RENDER_W - 122, 56, 8, (Color){120, 255, 220, 255});
        }
    }

    if (g.prototype_eval_active) {
        static const char *labels[6] = {
            "Would Replay",
            "Readability",
            "Mechanical Depth",
            "Ship Confidence",
            "Best Moment",
            "Main Friction",
        };
        DrawRectangle(64, 74, 404, 186, (Color){6, 8, 12, 228});
        DrawRectangleLines(64, 74, 404, 186, (Color){98, 104, 118, 255});
        DrawText("Slice Debrief", 82, 88, 18, (Color){240, 236, 224, 255});
        DrawText("LEFT/RIGHT category  UP/DOWN score  ENTER submit", 82, 108, 8, (Color){160, 160, 154, 255});
        for (int i = 0; i < 6; i++) {
            Color c = (i == g.prototype_eval_cursor)
                ? (Color){130, 184, 255, 255}
                : (Color){210, 206, 196, 255};
            char row[128];
            if (i < 4) {
                snprintf(row, sizeof(row), "%s  %d", labels[i], g.prototype_eval_draft[i]);
            } else if (i == 4) {
                snprintf(row, sizeof(row), "%s  %s", labels[i], prototype_best_moment_labels[g.prototype_active][g.prototype_eval_draft[i]]);
            } else {
                snprintf(row, sizeof(row), "%s  %s", labels[i], prototype_main_friction_labels[g.prototype_active][g.prototype_eval_draft[i]]);
            }
            DrawText(row, 86, 134 + i * 18, 10, c);
        }
    }
}
