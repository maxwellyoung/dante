// scene_glasshouse.c — STATE_GLASSHOUSE
// The hotel's observation lounge. The first big space.
// The Bioshock Infinite moment. The WALL-E scale.
// Elevator doors open → glass envelope → Earth below → evidence of couples.
// Gibbons leads you through. You don't stop. The contraction begins.
//
// Contraction: Glasshouse (20m) → Corridor (4.5m) → Suite (one room) → Bed (one pillow)

#include "game_ctx.h"
#include <math.h>

void set_exposure(float exp);
void transition_to(GameState s);

void glasshouse_load(void) {
    build_glasshouse(&g.scene);
    init_player(&g.player, g.scene.spawn);

    StopAmbient(&g.audio);
    StopClockAmbient(&g.audio);
    StopCityAmbient(&g.audio);
    StopWindAmbient(&g.audio);
    StopEarthPresence(&g.audio);

    PlayAirlockHiss(&g.audio);
    g.player.gravity_mult = 0.5f;
    StartAmbient(&g.audio, DRONE_SPACE_LOBBY);
    PlayEarthPresence(&g.audio);
    PlayWindAmbient(&g.audio);  // the creak of glass under pressure

    // Gibbons leads you through — he doesn't stop
    {
        Vector3 wps[] = {
            {0, 1.6f, -2},    // enters with you
            {3, 1.6f, -8},    // across the lounge
            {0, 1.6f, -14},   // toward the far exit
        };
        init_npc(&g.gibbons, g.scene.spawn, wps, 3, 4.0f, 1.5f);
        g.gibbons.speed = 3.0f;  // purposeful pace — not rushing, not lingering
        static const char *lines_first[] = {
            "The observation lounge. Most guests spend the evening here.",
            "Your suite is just through.",
        };
        static const char *lines_return[] = {
            "Quieter tonight.",
        };
        if (g.backstory_count > 3)
            npc_set_dialogue(&g.gibbons, lines_return, 1, 3.0f);
        else
            npc_set_dialogue(&g.gibbons, lines_first, 2, 4.0f);
    }

    SetSceneLighting(&g.lighting, LightingPreset_Balcony());  // reuse balcony preset for now
    set_exposure(0.1f);
    SetPostFXGrain(&g.postfx, 0.3f);
    SetPostFXWarmth(&g.postfx, 0.15f);
    SetPostFXCA(&g.postfx, 0.8f);
}

void glasshouse_update(float dt) {
    update_player(&g.player, &g.scene, dt);
    UpdateEVAudio(&g.audio, g.player.moving, g.player.sprinting, g.scene.surface, dt);
    update_npc(&g.gibbons, g.player.camera.position, &g.scene, dt);

    // The lounge rewards looking — FOV opens slightly as you take it in
    if (g.state_time < 3.0f) {
        float reveal = g.state_time / 3.0f;
        g.player.fov_current += (80.0f - g.player.fov_current) * reveal * 0.02f;
    }

    // Earth presence — blue-white wash strengthens near windows
    {
        float px = g.player.camera.position.x;
        float edge = fabsf(px) / 10.0f;  // 0→1 as you approach side walls
        if (edge > 0.5f) {
            float glow = (edge - 0.5f) * 2.0f;
            set_exposure(0.1f + glow * 0.08f);
        }
    }

    // Transition: Gibbons reaches the far end → corridor
    if (g.scene.has_exit) {
        float dist = Vector3Distance(g.player.camera.position, g.scene.exit_pos);
        if (dist < 3.0f) {
            g.elevator_to_corridor = true;  // next elevator is the space one
            transition_to(STATE_SPACE_CORRIDOR);
        }
    }
    // Gibbons done → also transition
    if (!g.gibbons.active && g.gibbons.current_waypoint >= g.gibbons.waypoint_count) {
        g.done_pause += dt;
        if (g.done_pause > 2.0f) {
            transition_to(STATE_SPACE_CORRIDOR);
        }
    }
}
