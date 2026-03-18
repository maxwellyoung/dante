// scene_splash.c — STATE_TITLE
#include "game_ctx.h"
#include <stdio.h>

extern GameCtx g;

// Forward declarations from main.c (until fully extracted)
void set_exposure(float exp);
void transition_to(GameState s);

void title_load(void) {
    DisableCursor();
    StopAmbient(&g.audio);
    StopSuiteMusic(&g.audio); StopBalconyMusic(&g.audio); StopCorridorMusic(&g.audio);
    reset_title_state();
    PlayTitleMusic(&g.audio);
    SetPostFXCA(&g.postfx, 0.0f);
    SetPostFXGrain(&g.postfx, 0.0f);
    SetPostFXWarmth(&g.postfx, 0.0f);
    SetPostFXSaturation(&g.postfx, 1.0f);
    SetPostFXContrast(&g.postfx, 0.0f);
    SetPostFXVignette(&g.postfx, 0.0f);
    set_exposure(0.0f);
}

void title_update(float dt) {
    (void)dt;
    static bool title_breath_played = false;
    if (g.state_time > 1.0f && !title_breath_played) {
        PlayTitleBreath(&g.audio);
        title_breath_played = true;
    }
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
        title_breath_played = false;
        transition_to(STATE_CAR);
    }
}
