// scene_shell_test.c — STATE_SHELL_TEST
// Dev-only scene for validating the shell system end-to-end.
// Press '-' key in dev mode to jump here.
//
// Tests:
//   1. add_shell() renders a GLB model with no collision
//   2. add_collision_wall() creates invisible physics boundaries
//   3. Player can walk on collision floor, bump into collision walls
//   4. Code-placed furniture works inside the shell
//   5. Visual confirms: you see the model but collide with the boxes
#include "game_ctx.h"
#include <math.h>
#include <stdio.h>

// ── Build the test environment ──
static void build_shell_test(Scene *s) {
    // ── 1. The shell: the production elevator shell, reused as a debug sandbox ──
    add_shell(s, "elevator_car", 0, 0, 0, 1, 1, 1, 0, MAT_BRASS, WHITE);

    // ── 2. Collision geometry: invisible boxes defining the walkable space ──
    // Floor — the player walks on this, not the model
    add_collision_floor(s, 0, 0, 0, 20, 20);

    // Walls — invisible boundaries (would match the shell's shape)
    add_collision_wall(s, -10, 2.5f, 0, 0.2f, 5, 20);   // left
    add_collision_wall(s,  10, 2.5f, 0, 0.2f, 5, 20);   // right
    add_collision_wall(s, 0, 2.5f, -10, 20, 5, 0.2f);   // back
    add_collision_wall(s, 0, 2.5f,  10, 20, 5, 0.2f);   // front

    // Ceiling
    add_collision_ceiling(s, 0, 5, 0, 20, 20);

    // ── 3. A visible reference wall so you can see what's real vs shell ──
    // These regular walls ARE visible and DO collide
    add_wall(s, -4, 1, 0, 0.2f, 2, 3, (Color){180, 170, 160, 255});
    set_last_material(s, MAT_CONCRETE);

    add_wall(s, 4, 1, 0, 0.2f, 2, 3, (Color){180, 170, 160, 255});
    set_last_material(s, MAT_CONCRETE);

    // ── 4. Furniture inside the shell — code-placed as usual ──
    add_sofa(s, -3, 0, 4, 0, (Color){80, 60, 50, 255});
    add_chair(s, 3, 0, 3, -30, (Color){120, 90, 60, 255}, (Color){70, 55, 45, 255});

    // ── 5. A regular prop beside the shell — confirms shell/prop split stays clean ──
    {
        int piano_mdl = find_model_asset("piano");
        if (piano_mdl >= 0)
            add_model(s, 5, 0, -3, 1, 1, 1, 45, piano_mdl, MAT_WOOD, WHITE);
    }

    // ── 6. Light panels to illuminate the space ──
    add_light_panel(s, 0, 4.8f, 0, 3, 0.05f, 3, (Color){255, 240, 200, 255});

    // Spawn
    s->spawn = (Vector3){0, 1.6f, 6};
}

void shell_test_load(void) {
    build_shell_test(&g.scene);
    init_player(&g.player, g.scene.spawn);

    StopAmbient(&g.audio);

    g.player.gravity_mult = 1.0f;
    SetSceneLighting(&g.lighting, LightingPreset_SpaceSuite());
    SetPostFXGrain(&g.postfx, 0.25f);
}

void shell_test_update(float dt) {
    update_player(&g.player, &g.scene, dt);

    // Print collision debug on F3 — wall count breakdown
    if (IsKeyPressed(KEY_F3)) {
        int visible = 0, invisible = 0, shell = 0;
        for (int i = 0; i < g.scene.wall_count; i++) {
            Wall *w = &g.scene.walls[i];
            if (!w->active) continue;
            if (w->color.a == 0) invisible++;
            else if (w->no_collide) shell++;
            else visible++;
        }
        printf("[SHELL TEST] Walls: %d total — %d visible, %d invisible collision, %d shell (no_collide)\n",
               g.scene.wall_count, visible, invisible, shell);
    }
}
