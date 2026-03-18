// scene_cleaned.c — STATE_CLEANED_SUITE
// The room after. Every two becomes one.
// Hard cut from Paris dream. 3 seconds of silence. Then bed.
#include "game_ctx.h"
#include <math.h>

extern GameCtx g;

void set_exposure(float exp);
void hard_cut_to(GameState s);

void cleaned_suite_load(void) {
    build_space_suite_cleaned(&g.scene);
    // Spawn at the entry, looking toward the bed — the whole room laid bare
    init_player(&g.player, (Vector3){0, 1.6f, 4.5f});
    g.player.camera.target = (Vector3){0, 1.2f, -4.5f};
    g.player.control_mult = 0.0f;  // no movement — you just look

    // Nuclear stop — every sound dies. The hum you didn't notice is gone.
    StopAllAudio(&g.audio);
    // Total silence. Not even a drone. The absence IS the sound.
    // The hotel hum that's been present since SPACE_LOBBY just stopped.
    // The player doesn't know what changed. They just feel the room is emptier.

    SetSceneLighting(&g.lighting, LightingPreset_SpaceSuite());
    set_exposure(-0.1f);  // slightly underexposed — morning after
    SetPostFXWarmth(&g.postfx, 0.0f);  // cold — the warmth is gone
    SetPostFXGrain(&g.postfx, 0.5f);
    SetPostFXSaturation(&g.postfx, 0.7f);  // desaturated — drained
    SetPostFXCA(&g.postfx, 1.5f);
    SetPostFXVignette(&g.postfx, 1.5f);
    SetPostFXContrast(&g.postfx, 0.9f);
}

void cleaned_suite_update(float dt) {
    (void)dt;

    // Slow camera drift — breathing, looking at what's left
    {
        float drift = sinf(g.state_time * 0.3f) * 0.001f;
        g.player.camera.target.x += drift;
        g.player.camera.target.y += sinf(g.state_time * 0.5f) * 0.0005f;
    }

    // Progressive exposure — eyes adjusting to the empty room
    if (g.state_time < 1.5f) {
        float t = g.state_time / 1.5f;
        set_exposure(-0.1f + t * 0.1f);
    }

    // After 3 seconds — hard cut to bed
    if (g.state_time > 3.0f) {
        SetPostFXWarmth(&g.postfx, 0.0f);
        SetPostFXSaturation(&g.postfx, 0.92f);
        SetPostFXContrast(&g.postfx, 1.0f);
        hard_cut_to(STATE_BED);
    }
}
