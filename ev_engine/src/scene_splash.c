// scene_splash.c — STATE_TITLE
// Option A: pre-title black screen choices (Firewatch-style)
// 4 choices on black, 60 seconds max. Then title card. Then taxi.
#include "game_ctx.h"
#include <stdio.h>

// Forward declarations from main.c (until fully extracted)
void set_exposure(float exp);
void show_text(const char *text);
void hide_text(void);
void show_choice(const char *question, const char *a, const char *b);
int poll_choice(void);
void transition_to(GameState s);

// ── Option A: prologue questions ──
static const char *prologue_questions[] = {
    "The booking was made",
    "You packed",
    "The second ticket",
};
static const char *prologue_a[] = {
    "Before.",
    "Light. One bag.",
    "Is in your pocket.",
};
static const char *prologue_b[] = {
    "After.",
    "Everything she left behind.",
    "Is on the seat beside you.",
};
#define PROLOGUE_COUNT 3

void title_load(void) {
    DisableCursor();
    StopAmbient(&g.audio);
    StopSuiteMusic(&g.audio); StopBalconyMusic(&g.audio); StopCorridorMusic(&g.audio);
    reset_title_state();
    g.title_breath_played = false;

    // Only show prologue on first play
    if (!g.title_prologue_done && g.backstory_count == 0) {
        // Black screen, no music yet — just silence and text
        SetPostFXCA(&g.postfx, 0.0f);
        SetPostFXGrain(&g.postfx, 0.0f);
        SetPostFXWarmth(&g.postfx, 0.0f);
        SetPostFXSaturation(&g.postfx, 0.0f);
        SetPostFXContrast(&g.postfx, 0.0f);
        SetPostFXVignette(&g.postfx, 0.0f);
        set_exposure(-2.0f);  // near black
        g.title_prologue_phase = 0;
        g.title_prologue_pause = 1.5f;  // initial pause before first question
    } else {
        PlayTitleMusic(&g.audio);
        SetPostFXCA(&g.postfx, 0.0f);
        SetPostFXGrain(&g.postfx, 0.0f);
        SetPostFXWarmth(&g.postfx, 0.0f);
        SetPostFXSaturation(&g.postfx, 1.0f);
        SetPostFXContrast(&g.postfx, 0.0f);
        SetPostFXVignette(&g.postfx, 0.0f);
        set_exposure(0.0f);
        g.title_prologue_done = true;
    }
}

void title_update(float dt) {
    // ── Option A: prologue phase ──
    if (!g.title_prologue_done) {
        // Brief pause between choices
        if (g.title_prologue_pause > 0) {
            g.title_prologue_pause -= dt;
            return;
        }

        if (g.title_prologue_phase < PROLOGUE_COUNT) {
            // Show current question if not already showing a choice
            if (!g.choice_active && g.choice_result < 0) {
                show_choice(
                    prologue_questions[g.title_prologue_phase],
                    prologue_a[g.title_prologue_phase],
                    prologue_b[g.title_prologue_phase]
                );
            }
            int result = poll_choice();
            if (result >= 0) {
                // Choice made — advance
                g.title_prologue_phase++;
                g.choice_result = -1;
                g.title_prologue_pause = 0.8f;  // brief black between choices
            }
            return;
        }

        // All prologue questions answered — transition to title card
        g.title_prologue_done = true;
        PlayTitleMusic(&g.audio);
        SetPostFXSaturation(&g.postfx, 1.0f);
        set_exposure(0.0f);
        g.state_time = 0;  // reset title timer
        return;
    }

    // ── Normal title card ──
    (void)dt;
    if (g.state_time > 1.0f && !g.title_breath_played) {
        PlayTitleBreath(&g.audio);
        g.title_breath_played = true;
    }
    if (IsKeyPressed(KEY_P)) {
        g.title_breath_played = false;
        transition_to(STATE_PROTO_LAB);
    }
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
        g.title_breath_played = false;
        transition_to(STATE_CAR);
    }
}
