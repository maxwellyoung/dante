// scene_splash.c — STATE_TITLE
// Option A: pre-title black screen choices (Firewatch-style)
// 4 choices on black, 60 seconds max. Then title card. Then taxi.
#include "game_ctx.h"
#include <stdio.h>

extern GameCtx g;

// Forward declarations from main.c (until fully extracted)
void set_exposure(float exp);
void show_text(const char *text);
void hide_text(void);
void show_choice(const char *question, const char *a, const char *b);
int poll_choice(void);
void transition_to(GameState s);

// ── Option A: prologue questions ──
static const char *prologue_questions[] = {
    "The hotel was booked in",
    "You packed",
    "The second ticket is",
};
static const char *prologue_a[] = {
    "January. Before everything.",
    "Light. One bag.",
    "In your pocket.",
};
static const char *prologue_b[] = {
    "March. After she left.",
    "Everything she forgot.",
    "On the seat beside you.",
};
#define PROLOGUE_COUNT 3

static int prologue_phase = 0;     // 0..PROLOGUE_COUNT = questions, then title
static bool prologue_done = false;
static float prologue_pause = 0;   // brief pause between choices

void title_load(void) {
    DisableCursor();
    StopAmbient(&g.audio);
    StopSuiteMusic(&g.audio); StopBalconyMusic(&g.audio); StopCorridorMusic(&g.audio);
    reset_title_state();

    // Only show prologue on first play
    if (!prologue_done && g.backstory_count == 0) {
        // Black screen, no music yet — just silence and text
        SetPostFXCA(&g.postfx, 0.0f);
        SetPostFXGrain(&g.postfx, 0.0f);
        SetPostFXWarmth(&g.postfx, 0.0f);
        SetPostFXSaturation(&g.postfx, 0.0f);
        SetPostFXContrast(&g.postfx, 0.0f);
        SetPostFXVignette(&g.postfx, 0.0f);
        set_exposure(-2.0f);  // near black
        prologue_phase = 0;
        prologue_pause = 1.5f;  // initial pause before first question
    } else {
        PlayTitleMusic(&g.audio);
        SetPostFXCA(&g.postfx, 0.0f);
        SetPostFXGrain(&g.postfx, 0.0f);
        SetPostFXWarmth(&g.postfx, 0.0f);
        SetPostFXSaturation(&g.postfx, 1.0f);
        SetPostFXContrast(&g.postfx, 0.0f);
        SetPostFXVignette(&g.postfx, 0.0f);
        set_exposure(0.0f);
        prologue_done = true;
    }
}

void title_update(float dt) {
    // ── Option A: prologue phase ──
    if (!prologue_done) {
        // Brief pause between choices
        if (prologue_pause > 0) {
            prologue_pause -= dt;
            return;
        }

        if (prologue_phase < PROLOGUE_COUNT) {
            // Show current question if not already showing a choice
            if (!g.choice_active && g.choice_result < 0) {
                show_choice(
                    prologue_questions[prologue_phase],
                    prologue_a[prologue_phase],
                    prologue_b[prologue_phase]
                );
            }
            int result = poll_choice();
            if (result >= 0) {
                // Choice made — advance
                prologue_phase++;
                g.choice_result = -1;
                prologue_pause = 0.8f;  // brief black between choices
            }
            return;
        }

        // All prologue questions answered — transition to title card
        prologue_done = true;
        PlayTitleMusic(&g.audio);
        SetPostFXSaturation(&g.postfx, 1.0f);
        set_exposure(0.0f);
        g.state_time = 0;  // reset title timer
        return;
    }

    // ── Normal title card ──
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
