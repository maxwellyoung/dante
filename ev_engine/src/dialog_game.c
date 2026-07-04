// dialog_game.c — engine binding for the contextual dialogue system
// Builds the query (the flat pile of world facts) from GameCtx, drives the
// speak lifecycle through show_dialogue(), chains followups, and owns the
// automatic triggers: Gibbons' waypoint stops and the idle poll.
//
// Content lives in assets/dialogue/ev.rules — edit + F6 to hot reload.
#include "dialog.h"
#include "game_ctx.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

void show_dialogue(const char *speaker, const char *text);
void hide_dialogue(void);

#define DLG_RULES_PATH "assets/dialogue/ev.rules"
#define DLG_IDLE_PERIOD 8.0f

// Scene fact names — indexed by GameState (keep in sync with ev_types.h)
static const char *state_names[] = {
    "title", "proto_lab", "proto_movement", "proto_shooter", "proto_puzzle",
    "car", "driving", "hotel_ext", "lobby", "elevator",
    "hallway", "room", "bathroom", "balcony", "bed",
    "stars", "hyperspace", "space_lobby", "space_corridor", "space_suite",
    "paris_dream", "cleaned_suite", "montage", "return_taxi", "glasshouse",
    "shell_test",
};

static const char *behavior_names[] = {
    "walking", "waiting", "reading", "sitting", "gazing", "gesturing",
};

// Speak lifecycle + scheduled followup
static struct {
    bool speaking;
    int who;             // speaker symbol
    float elapsed;
    float duration;
    bool follow_pending;
    float follow_at;     // g.total_time to fire
    int follow_who;
    int follow_concept;
    float idle_timer;
    GameState prev_state;
    // Reactive music (Remo): detect interaction valleys — no content hit
    // in a while, none imminent — and let a soft musical figure carry it.
    float valley_timer;
    float music_cooldown;
} sp = { .prev_state = (GameState)-1 };

bool dlg_game_speaking(void) { return sp.speaking; }

void dlg_game_init(void) {
    dlg_reset();
    if (dlg_load_file(DLG_RULES_PATH))
        TraceLog(LOG_INFO, "DIALOG: loaded %d rules from %s", dlg_rule_count(), DLG_RULES_PATH);
    else
        TraceLog(LOG_WARNING, "DIALOG: %s", dlg_last_error());
}

// Every fact that might be relevant, thrown at the database every time —
// so writers can hang new rules on state without programmer round-trips.
static void build_query(DlgQuery *q, const char *who, const char *concept) {
    dlg_query_init(q);
    dlg_query_add_sym(q, "concept", concept);
    dlg_query_add_sym(q, "who", who);
    if ((int)g.state >= 0 && (size_t)g.state < sizeof(state_names) / sizeof(state_names[0]))
        dlg_query_add_sym(q, "scene", state_names[g.state]);
    dlg_query_add(q, "scene_time", g.state_time);
    dlg_query_add(q, "total_time", g.total_time);
    dlg_query_add(q, "tasks_done", (float)g.tasks_done);
    dlg_query_add(q, "lobby_visits", (float)g.lobby_visit_count);
    dlg_query_add(q, "backstory_count", (float)g.backstory_count);
    dlg_query_add(q, "random", (float)(rand() % 100));

    float sx = g.player.vel.x, sz = g.player.vel.z;
    float speed = sqrtf(sx * sx + sz * sz);
    dlg_query_add(q, "player_speed", speed);
    dlg_query_add(q, "player_still", speed < 0.5f ? 1.0f : 0.0f);

    if (g.gibbons.active) {
        Vector3 d = Vector3Subtract(g.player.camera.position, g.gibbons.pos);
        dlg_query_add(q, "gibbons_dist", Vector3Length(d));
        dlg_query_add(q, "waypoint", (float)g.gibbons.current_waypoint);
        if (g.gibbons.behavior >= 0
            && (size_t)g.gibbons.behavior < sizeof(behavior_names) / sizeof(behavior_names[0]))
            dlg_query_add_sym(q, "gibbons_behavior", behavior_names[g.gibbons.behavior]);
    }

    if (g.state == STATE_SPACE_SUITE) {
        dlg_query_add(q, "window_revealed", g.suite_window_revealed ? 1.0f : 0.0f);
        dlg_query_add(q, "phone_ringing", g.suite_phone_ringing ? 1.0f : 0.0f);
    }

    // Long-horizon facts — what the ending remembers about how you lived here
    dlg_query_add(q, "photograph_flipped", g.photograph_flipped ? 1.0f : 0.0f);

    // Carry facts — the boombox move (Remo): rules see what's in your hands.
    // Tagged props (set_last_tag) expose carrying=<tag>; anything held at
    // all exposes holding=1. Carrying her book onto the balcony is a fact.
    if (g.grab.state == GRAB_CARRYING && g.grab.wall_index >= 0
        && g.grab.wall_index < g.scene.wall_count) {
        dlg_query_add(q, "holding", 1.0f);
        const char *tag = g.scene.walls[g.grab.wall_index].tag;
        if (tag) dlg_query_add_sym(q, "carrying", tag);
    }
}

static bool speak_ex(const char *who, const char *concept,
                     const char *object, float step, bool has_object) {
    DlgQuery q;
    build_query(&q, who, concept);
    if (has_object) {
        dlg_query_add_sym(&q, "object", object);
        dlg_query_add(&q, "step", step);
    }
    DlgResponse res;
    if (!dlg_match(&q, &res)) return false;
    dlg_commit(&res);

    // Speaker display: uppercase symbol; "narrator" renders as unattributed
    static char display[DLG_SYMBOL_LEN];
    const char *speaker = NULL;
    if (strcmp(who, "narrator") != 0) {
        int i = 0;
        for (; who[i] && i < DLG_SYMBOL_LEN - 1; i++)
            display[i] = (char)((who[i] >= 'a' && who[i] <= 'z') ? who[i] - 32 : who[i]);
        display[i] = '\0';
        speaker = display;
    }
    show_dialogue(speaker, res.line);

    float cps = g.dlg_chars_per_sec > 1 ? g.dlg_chars_per_sec : 30.0f;
    float dur = (float)strlen(res.line) / cps + 1.2f;   // read tail
    sp.duration = fminf(10.0f, fmaxf(2.2f, dur));       // stopwatch rule: edit down
    sp.elapsed = 0;
    sp.speaking = true;
    sp.who = dlg_intern(who);

    sp.follow_pending = false;
    if (res.rule->then_concept >= 0) {
        // Stash it — scheduled when the line FINISHES, queried when it fires,
        // so a conversation self-terminates if the world changed under it.
        sp.follow_who = res.rule->then_who;
        sp.follow_concept = res.rule->then_concept;
        sp.follow_at = res.rule->then_delay;  // interpreted at line end
        sp.follow_pending = true;
    }
    TraceLog(LOG_INFO, "DIALOG: [%s] %s: \"%s\"", res.rule->name, who, res.line);
    return true;
}

bool dlg_game_speak(const char *who, const char *concept) {
    return speak_ex(who, concept, NULL, 0, false);
}

bool dlg_game_speak_about(const char *who, const char *concept,
                          const char *object, float step) {
    return speak_ex(who, concept, object, step, true);
}

bool dlg_game_remark(const char *concept, const char *object, float step) {
    return speak_ex(g.gibbons.active ? "gibbons" : "narrator", concept,
                    object, step, true);
}

void dlg_game_update(float dt) {
    dlg_set_time(g.total_time);

    // Hard scene cuts kill in-flight speech and stale followups
    if (g.state != sp.prev_state) {
        sp.prev_state = g.state;
        if (sp.speaking) { hide_dialogue(); sp.speaking = false; }
        sp.idle_timer = 0;
        g.gibbons.dlg_fired_waypoint = -1;
        // Every scene entry asks the database for an "enter" line 2.5s in.
        // Queried against accumulated facts — this is how endings permute
        // (Vanaman/TWD: the same beat recontextualized by your history).
        // No match = silence, which is most scenes.
        sp.follow_pending = true;
        sp.follow_who = dlg_intern("narrator");
        sp.follow_concept = dlg_intern("enter");
        sp.follow_at = g.total_time + 2.5f;
    }

#ifndef PLAYTEST
    if (IsKeyPressed(KEY_F6)) {
        dlg_game_init();  // hot reload — writers iterate without recompiling
    }
#endif

    // Speak lifecycle
    if (sp.speaking) {
        sp.elapsed += dt;
        if (sp.elapsed >= sp.duration) {
            sp.speaking = false;
            hide_dialogue();
            if (sp.follow_pending)
                sp.follow_at = g.total_time + sp.follow_at;  // delay → absolute
        }
    } else if (sp.follow_pending && g.total_time >= sp.follow_at) {
        sp.follow_pending = false;
        dlg_game_speak(dlg_symbol_name(sp.follow_who),
                       dlg_symbol_name(sp.follow_concept));
    }

    // dlg_hold retired from waypoint gating (walk-and-talk): Gibbons waits
    // for the player, not his subtitle. Field stays for explicit scene holds.
    g.gibbons.dlg_hold = false;

    bool legacy_line = npc_current_dialogue(&g.gibbons) != NULL;

    // Waypoint trigger — rules-driven scenes only (no legacy line array set)
    if (g.gibbons.active && !g.gibbons.lines && g.gibbons.waiting
        && g.gibbons.idle_timer > 0.5f && !sp.speaking && !legacy_line
        && g.gibbons.dlg_fired_waypoint != g.gibbons.current_waypoint) {
        g.gibbons.dlg_fired_waypoint = g.gibbons.current_waypoint;
        dlg_game_speak("gibbons", "waypoint");
    }

    // ── Reactive music — the interaction-valley manager ─────────────
    // Content resets the clock: any line on screen, any interaction beat.
    // A long valley in a spine scene earns one soft held chord, then a
    // long cooldown. The player's brain does the emotional work.
    if (sp.speaking || g.dlg_active || g.interact_freeze > 0) {
        sp.valley_timer = 0;
    } else {
        sp.valley_timer += dt;
    }
    if (sp.music_cooldown > 0) sp.music_cooldown -= dt;
    bool musical_scene = (g.state == STATE_SPACE_LOBBY || g.state == STATE_GLASSHOUSE
                          || g.state == STATE_SPACE_CORRIDOR || g.state == STATE_SPACE_SUITE
                          || g.state == STATE_BALCONY || g.state == STATE_LOBBY);
    if (musical_scene && sp.valley_timer > 50.0f && sp.music_cooldown <= 0) {
        PlayHeldChord(&g.audio);
        sp.valley_timer = 0;
        sp.music_cooldown = 140.0f;
        TraceLog(LOG_INFO, "DIALOG: interaction valley — held chord");
    }

    // Idle poll — every few seconds, ask the database if anyone has an
    // observation. No match = silence, which is also an answer.
    if (!sp.speaking && !sp.follow_pending && !legacy_line && !g.dlg_active) {
        sp.idle_timer += dt;
        if (sp.idle_timer > DLG_IDLE_PERIOD + (float)(rand() % 40) * 0.1f) {
            sp.idle_timer = 0;
            if (g.gibbons.active) dlg_game_speak("gibbons", "idle");
            else dlg_game_speak("narrator", "idle");
        }
    } else {
        sp.idle_timer = 0;
    }
}
