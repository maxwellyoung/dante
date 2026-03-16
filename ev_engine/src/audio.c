// audio.c — Procedural audio engine
// NO CREEPY DRONES. Pleasant or silent. Bioshock vibes.
#include "audio.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define SAMPLE_RATE 44100
#ifndef PI
#define PI 3.14159265358979f
#endif

static Wave gen_wave(int samples) {
    Wave w = {0};
    w.frameCount = samples;
    w.sampleRate = SAMPLE_RATE;
    w.sampleSize = 16;
    w.channels = 1;
    w.data = calloc(samples, sizeof(short));
    return w;
}

// Deterministic PRNG — better than rand(), no global state
static float ev_randf(unsigned int *seed) {
    *seed = (*seed * 1103515245 + 12345);
    return (float)((*seed >> 16) & 0x7fff) / 32768.0f;
}

// --- FOOTSTEPS ---

static Sound gen_step_marble(int seed) {
    int len = SAMPLE_RATE / 10;
    // Extra samples for reverb tail
    int reverb_delay = (int)(SAMPLE_RATE * 0.08f);
    int total = len + reverb_delay;
    Wave w = gen_wave(total);
    short *d = (short *)w.data;
    unsigned int rng = (unsigned int)seed;
    float pitch = 0.9f + ev_randf(&rng) * 0.2f;
    for (int i = 0; i < len; i++) {
        float t = (float)i / len;
        float env = (t < 0.02f) ? t / 0.02f : expf(-12.0f * (t - 0.02f));
        float click = sinf(2 * PI * 2200 * pitch * t) * expf(-30.0f * t);
        float body = sinf(2 * PI * 180 * pitch * t) * expf(-10.0f * t);
        float noise = (ev_randf(&rng) * 2.0f - 1.0f) * expf(-20.0f * t);
        d[i] = (short)((click * 0.3f + body * 0.4f + noise * 0.3f) * env * 10000);
    }
    // Indoor reverb tail
    for (int i = reverb_delay; i < total; i++) {
        d[i] += (short)(d[i - reverb_delay] * 0.1f);
    }
    w.frameCount = total;
    Sound s = LoadSoundFromWave(w); UnloadWave(w); return s;
}

static Sound gen_step_carpet(int seed) {
    int len = SAMPLE_RATE / 8;
    int reverb_delay = (int)(SAMPLE_RATE * 0.08f);
    int total = len + reverb_delay;
    Wave w = gen_wave(total);
    short *d = (short *)w.data;
    unsigned int rng = (unsigned int)seed;
    float pitch = 0.85f + ev_randf(&rng) * 0.3f;
    for (int i = 0; i < len; i++) {
        float t = (float)i / len;
        float env = (t < 0.08f) ? t / 0.08f : expf(-6.0f * (t - 0.08f));
        float thud = sinf(2 * PI * 45 * pitch * t) * expf(-8.0f * t);
        float noise = (ev_randf(&rng) * 2.0f - 1.0f);
        float lp = noise * 0.15f + thud * 0.85f;
        d[i] = (short)(lp * env * 7000);
    }
    // Indoor reverb tail
    for (int i = reverb_delay; i < total; i++) {
        d[i] += (short)(d[i - reverb_delay] * 0.1f);
    }
    w.frameCount = total;
    Sound s = LoadSoundFromWave(w); UnloadWave(w); return s;
}

static Sound gen_step_wood(int seed) {
    int len = SAMPLE_RATE / 10;
    int reverb_delay = (int)(SAMPLE_RATE * 0.08f);
    int total = len + reverb_delay;
    Wave w = gen_wave(total);
    short *d = (short *)w.data;
    unsigned int rng = (unsigned int)seed;
    float pitch = 0.85f + ev_randf(&rng) * 0.3f;
    for (int i = 0; i < len; i++) {
        float t = (float)i / len;
        float env = (t < 0.03f) ? t / 0.03f : expf(-10.0f * (t - 0.03f));
        float tap = sinf(2 * PI * 400 * pitch * t) * expf(-20.0f * t);
        float body = sinf(2 * PI * 80 * pitch * t) * expf(-8.0f * t);
        d[i] = (short)((tap * 0.5f + body * 0.5f) * env * 9000);
    }
    // Indoor reverb tail
    for (int i = reverb_delay; i < total; i++) {
        d[i] += (short)(d[i - reverb_delay] * 0.1f);
    }
    w.frameCount = total;
    Sound s = LoadSoundFromWave(w); UnloadWave(w); return s;
}

// --- AMBIENT — NOT DRONES. Musical, pleasant, short loops ---

// Lobby: same melody as room but one octave higher, slower decay (reverb-like)
// Heard through walls — distant, ethereal
static Sound gen_ambient_lobby(void) {
    float beat = 0.6f;  // slower tempo — more contemplative
    float loop_len = 16.0f * beat * 2;
    int len = (int)(SAMPLE_RATE * loop_len);
    int reverb_delay = (int)(SAMPLE_RATE * 0.1f);
    Wave w = gen_wave(len);
    short *d = (short *)w.data;

    // Same progression, one octave up
    float melody[][3] = {
        {659.26f, 0, 2}, {587.33f, 2, 1}, {523.25f, 3, 1},
        {440.00f, 4, 4},
        {659.26f, 8, 2}, {698.46f, 10, 1}, {659.26f, 11, 1},
        {587.33f, 12, 4},
        {659.26f, 16, 2}, {587.33f, 18, 1}, {523.25f, 19, 1},
        {440.00f, 20, 4},
        {659.26f, 24, 2}, {698.46f, 26, 1}, {659.26f, 27, 1},
        {587.33f, 28, 4},
    };
    int note_count = 16;

    for (int i = 0; i < len; i++) {
        float t = (float)i / SAMPLE_RATE;
        float sample = 0;
        for (int n = 0; n < note_count; n++) {
            float freq = melody[n][0];
            float start = melody[n][1] * beat;
            float dur = melody[n][2] * beat;
            float nt = t - start;
            if (nt < 0 || nt > dur + 3.0f) continue;
            // Hammer noise on attack (first 10ms)
            float noise_burst = 0;
            if (nt < 0.01f) {
                unsigned int ns = (unsigned int)(n * 997 + i);
                noise_burst = (ev_randf(&ns) * 2.0f - 1.0f) * (1.0f - nt / 0.01f) * 0.2f;
            }
            float attack = (nt < 0.05f) ? nt / 0.05f : 1.0f;
            float env = attack * expf(-0.8f * nt);
            // Rich harmonics + detuning for chorus warmth
            float detune = 0.5f;  // Hz
            float tone = sinf(2 * PI * freq * t) +
                         0.3f * sinf(2 * PI * (freq * 2.0f + detune) * t) +
                         0.1f * sinf(2 * PI * (freq * 3.0f - detune) * t) +
                         0.05f * sinf(2 * PI * freq * 5.0f * t);
            sample += (tone * env + noise_burst);
        }
        float lt = (float)i / len;
        if (lt > 0.95f) sample *= (1.0f - lt) / 0.05f;
        if (lt < 0.02f) sample *= lt / 0.02f;
        d[i] = (short)(sample * 1800);
    }
    // Reverb approximation
    for (int i = reverb_delay; i < len; i++) {
        d[i] += (short)(d[i - reverb_delay] * 0.15f);
    }
    Sound s = LoadSoundFromWave(w); UnloadWave(w); return s;
}

// Hallway: just root notes as sustained tones — building anticipation
// E3, A2, E3, D3 — the skeleton of the melody, stripped bare
static Sound gen_ambient_hallway(void) {
    float beat = 0.6f;  // slower tempo
    float loop_len = 16.0f * beat * 2;
    int len = (int)(SAMPLE_RATE * loop_len);
    int reverb_delay = (int)(SAMPLE_RATE * 0.1f);
    Wave w = gen_wave(len);
    short *d = (short *)w.data;

    // Root notes only — sustained, hollow
    float roots[][3] = {
        {164.81f, 0, 8},   // E3 — bars 1-2
        {110.00f, 8, 8},   // A2 — bars 3-4
        {164.81f, 16, 8},  // E3 — bars 5-6
        {146.83f, 24, 8},  // D3 — bars 7-8
    };
    int root_count = 4;

    for (int i = 0; i < len; i++) {
        float t = (float)i / SAMPLE_RATE;
        float sample = 0;
        for (int n = 0; n < root_count; n++) {
            float freq = roots[n][0];
            float start = roots[n][1] * beat;
            float dur = roots[n][2] * beat;
            float nt = t - start;
            if (nt < 0 || nt > dur) continue;
            // Hammer noise on attack
            float noise_burst = 0;
            if (nt < 0.01f) {
                unsigned int ns = (unsigned int)(n * 1013 + i);
                noise_burst = (ev_randf(&ns) * 2.0f - 1.0f) * (1.0f - nt / 0.01f) * 0.15f;
            }
            float attack = (nt < 0.1f) ? nt / 0.1f : 1.0f;
            float release = (nt > dur - 0.5f) ? (dur - nt) / 0.5f : 1.0f;
            float env = attack * release;
            // Rich harmonics + detuning
            float detune = 0.5f;
            float tone = sinf(2 * PI * freq * t) +
                         0.3f * sinf(2 * PI * (freq * 2.0f + detune) * t) +
                         0.1f * sinf(2 * PI * (freq * 3.0f - detune) * t) +
                         0.05f * sinf(2 * PI * freq * 5.0f * t);
            sample += (tone * env + noise_burst);
        }
        float lt = (float)i / len;
        if (lt > 0.95f) sample *= (1.0f - lt) / 0.05f;
        if (lt < 0.02f) sample *= lt / 0.02f;
        d[i] = (short)(sample * 2200);
    }
    // Reverb approximation
    for (int i = reverb_delay; i < len; i++) {
        d[i] += (short)(d[i - reverb_delay] * 0.15f);
    }
    Sound s = LoadSoundFromWave(w); UnloadWave(w); return s;
}

// Room: composed piano melody — Satie-inspired descending phrase
// 8 bars, slower tempo (0.6s per beat), loops every ~19 seconds
// Rich harmonics, hammer noise attack, bass note in bar 3, delay reverb
static Sound gen_ambient_room(void) {
    float beat = 0.6f;  // ~100 BPM — more contemplative than 120
    float loop_len = 16.0f * beat * 2;  // 8 bars of 4 beats
    int len = (int)(SAMPLE_RATE * loop_len);
    int reverb_delay = (int)(SAMPLE_RATE * 0.1f);
    Wave w = gen_wave(len);
    short *d = (short *)w.data;

    // Melody: bar 1-4, then repeat for bars 5-8
    // Each entry: {freq_hz, start_beat, duration_beats}
    float melody[][3] = {
        // Bar 1: E4 half, D4 quarter, C4 quarter
        {329.63f, 0, 2}, {293.66f, 2, 1}, {261.63f, 3, 1},
        // Bar 2: A3 whole
        {220.00f, 4, 4},
        // Bar 3: E4 half, F4 quarter, E4 quarter
        {329.63f, 8, 2}, {349.23f, 10, 1}, {329.63f, 11, 1},
        // Bar 4: D4 whole
        {293.66f, 12, 4},
        // Bars 5-8: repeat
        {329.63f, 16, 2}, {293.66f, 18, 1}, {261.63f, 19, 1},
        {220.00f, 20, 4},
        {329.63f, 24, 2}, {349.23f, 26, 1}, {329.63f, 27, 1},
        {293.66f, 28, 4},
    };
    int note_count = 16;

    // Bass notes — octave below, entering softly in bar 3 onward
    float bass[][3] = {
        {164.81f, 8, 4},   // E3 under bar 3
        {146.83f, 12, 4},  // D3 under bar 4
        {164.81f, 24, 4},  // E3 under bar 7
        {146.83f, 28, 4},  // D3 under bar 8
    };
    int bass_count = 4;

    for (int i = 0; i < len; i++) {
        float t = (float)i / SAMPLE_RATE;
        float sample = 0;

        // Melody notes — rich harmonics
        for (int n = 0; n < note_count; n++) {
            float freq = melody[n][0];
            float start = melody[n][1] * beat;
            float dur = melody[n][2] * beat;
            float nt = t - start;
            if (nt < 0 || nt > dur + 2.0f) continue;
            // Hammer noise on attack (first 10ms) — simulates key strike
            float noise_burst = 0;
            if (nt < 0.01f) {
                unsigned int ns = (unsigned int)(n * 997 + i);
                noise_burst = (ev_randf(&ns) * 2.0f - 1.0f) * (1.0f - nt / 0.01f) * 0.25f;
            }
            float attack = (nt < 0.05f) ? nt / 0.05f : 1.0f;
            float env = attack * expf(-1.5f * nt);
            // 3 harmonics: fundamental + 2nd (0.3) + 3rd (0.1) + 5th (0.05)
            // Bell-like timbre with richer upper partials
            float tone = sinf(2 * PI * freq * t) +
                         0.3f * sinf(2 * PI * freq * 2.0f * t) +
                         0.1f * sinf(2 * PI * freq * 3.0f * t) +
                         0.05f * sinf(2 * PI * freq * 5.0f * t);
            sample += (tone * env + noise_burst) * 0.7f;
        }

        // Bass notes — soft, warm, supporting
        for (int n = 0; n < bass_count; n++) {
            float freq = bass[n][0];
            float start = bass[n][1] * beat;
            float dur = bass[n][2] * beat;
            float nt = t - start;
            if (nt < 0 || nt > dur + 1.5f) continue;
            float attack = (nt < 0.1f) ? nt / 0.1f : 1.0f;
            float env = attack * expf(-1.2f * nt);
            float tone = sinf(2 * PI * freq * t) +
                         0.2f * sinf(2 * PI * freq * 2.0f * t);
            sample += tone * env * 0.35f;
        }

        // Loop crossfade
        float lt = (float)i / len;
        if (lt > 0.95f) sample *= (1.0f - lt) / 0.05f;
        if (lt < 0.02f) sample *= lt / 0.02f;
        d[i] = (short)(sample * 2400);
    }

    // Reverb approximation — mix delayed sample at 0.15 amplitude
    for (int i = reverb_delay; i < len; i++) {
        d[i] += (short)(d[i - reverb_delay] * 0.15f);
    }

    Sound s = LoadSoundFromWave(w); UnloadWave(w); return s;
}

// --- INTERACTION SOUNDS ---

static Sound gen_click(void) {
    int len = SAMPLE_RATE / 15;
    Wave w = gen_wave(len);
    short *d = (short *)w.data;
    for (int i = 0; i < len; i++) {
        float t = (float)i / SAMPLE_RATE;
        float env = expf(-40.0f * t);
        d[i] = (short)((sinf(2*PI*1800*t)*0.4f*env + sinf(2*PI*3200*t)*0.2f*expf(-60.0f*t)) * 12000);
    }
    Sound s = LoadSoundFromWave(w); UnloadWave(w); return s;
}

static Sound gen_fabric(void) {
    int len = SAMPLE_RATE / 4;
    Wave w = gen_wave(len);
    short *d = (short *)w.data;
    float prev = 0;
    for (int i = 0; i < len; i++) {
        float t = (float)i / len;
        float env = (t < 0.1f) ? t/0.1f : (t < 0.4f) ? 1.0f : expf(-4.0f*(t-0.4f));
        float noise = ((float)(rand()%32768)/16384.0f - 1.0f);
        prev = prev*0.85f + noise*0.15f;
        d[i] = (short)(prev * env * 6000);
    }
    Sound s = LoadSoundFromWave(w); UnloadWave(w); return s;
}

static Sound gen_flame(void) {
    int len = SAMPLE_RATE / 3;
    Wave w = gen_wave(len);
    short *d = (short *)w.data;
    for (int i = 0; i < len; i++) {
        float t = (float)i / SAMPLE_RATE;
        float lt = (float)i / len;
        float strike = ((float)(rand()%32768)/16384.0f-1.0f)*expf(-20.0f*t);
        float fenv = (lt < 0.15f) ? lt/0.15f : expf(-3.0f*(lt-0.15f));
        float noise = ((float)(rand()%32768)/16384.0f-1.0f);
        float flame = noise*0.3f + sinf(2*PI*400*t+noise*2)*0.2f;
        d[i] = (short)((strike*0.5f + flame*fenv*0.5f) * 8000);
    }
    Sound s = LoadSoundFromWave(w); UnloadWave(w); return s;
}

// Reward: bright ascending chime
static Sound gen_reward(void) {
    int len = SAMPLE_RATE;
    Wave w = gen_wave(len);
    short *d = (short *)w.data;
    float notes[] = {523.25f, 659.25f, 783.99f};  // C5, E5, G5 — major
    float starts[] = {0.0f, 0.1f, 0.2f};
    for (int i = 0; i < len; i++) {
        float t = (float)i / SAMPLE_RATE;
        float sample = 0;
        for (int n = 0; n < 3; n++) {
            float nt = t - starts[n];
            if (nt < 0) continue;
            float env = expf(-2.0f * nt);
            sample += 0.3f * sinf(2*PI*notes[n]*t) * env;
            sample += 0.1f * sinf(2*PI*notes[n]*2*t) * env * 0.5f;
        }
        d[i] = (short)(sample * expf(-1.0f*t) * 8000);
    }
    Sound s = LoadSoundFromWave(w); UnloadWave(w); return s;
}

// Sparkle
static Sound gen_sparkle(void) {
    int len = SAMPLE_RATE * 2;
    Wave w = gen_wave(len);
    short *d = (short *)w.data;
    for (int i = 0; i < len; i++) {
        float t = (float)i / SAMPLE_RATE;
        float lt = (float)i / len;
        float env = (lt < 0.3f) ? lt/0.3f : (lt < 0.7f) ? 1.0f : (1.0f-lt)/0.3f;
        float sample = 0.12f*sinf(2*PI*2200*t + sinf(t*5)*0.5f) +
                       0.10f*sinf(2*PI*3300*t + sinf(t*7)*0.3f) +
                       0.08f*sinf(2*PI*4400*t);
        d[i] = (short)(sample * env * (0.8f+0.2f*sinf(t*30)) * 5000);
    }
    Sound s = LoadSoundFromWave(w); UnloadWave(w); return s;
}

static Sound gen_door_sound(void) {
    int len = SAMPLE_RATE / 2;
    Wave w = gen_wave(len);
    short *d = (short *)w.data;
    for (int i = 0; i < len; i++) {
        float t = (float)i / SAMPLE_RATE;
        // Latch click — very short (5ms) high-frequency burst before main tone
        float latch = 0;
        if (t < 0.005f) {
            latch = sinf(2 * PI * 4500 * t) * (1.0f - t / 0.005f) * 0.6f;
        }
        float env = (t < 0.05f) ? t/0.05f : expf(-4.0f*(t-0.05f));
        // Rising pitch — arrival, not descent (Remo)
        float main_tone = sinf(2*PI*(150+120*t)*t)*env;
        d[i] = (short)((main_tone + latch) * 8000);
    }
    Sound s = LoadSoundFromWave(w); UnloadWave(w); return s;
}

// --- AMBIENT SCENE SOUNDS ---

// City traffic: low-pass filtered noise with occasional swells (10s loop)
static Sound gen_city_ambient(void) {
    int len = SAMPLE_RATE * 10;
    Wave w = gen_wave(len);
    short *d = (short *)w.data;
    float prev = 0;
    float prev2 = 0;
    srand(555);
    for (int i = 0; i < len; i++) {
        float t = (float)i / SAMPLE_RATE;
        float lt = (float)i / len;
        // White noise
        float noise = ((float)(rand() % 32768) / 16384.0f - 1.0f);
        // Two-pole low pass for rumble
        prev = prev * 0.95f + noise * 0.05f;
        prev2 = prev2 * 0.92f + prev * 0.08f;
        // Occasional swells (cars passing) — slow sine modulation
        float swell = 1.0f + 0.4f * sinf(t * 1.2f) + 0.3f * sinf(t * 0.7f + 1.0f);
        // Loop crossfade
        float env = 1.0f;
        if (lt < 0.02f) env = lt / 0.02f;
        if (lt > 0.98f) env = (1.0f - lt) / 0.02f;
        d[i] = (short)(prev2 * swell * env * 5000);
    }
    Sound s = LoadSoundFromWave(w); UnloadWave(w); return s;
}

// Clock tick: short click every 1 second (4s loop)
static Sound gen_clock_ambient(void) {
    int len = SAMPLE_RATE * 4;
    Wave w = gen_wave(len);
    short *d = (short *)w.data;
    for (int sec = 0; sec < 4; sec++) {
        int start = sec * SAMPLE_RATE;
        int tick_len = SAMPLE_RATE / 30;  // ~33ms tick
        for (int i = 0; i < tick_len && (start + i) < len; i++) {
            float t = (float)i / SAMPLE_RATE;
            float env = expf(-60.0f * t);
            float click = sinf(2 * PI * 3500 * t) * 0.5f + sinf(2 * PI * 1200 * t) * 0.3f;
            d[start + i] = (short)(click * env * 6000);
        }
    }
    Sound s = LoadSoundFromWave(w); UnloadWave(w); return s;
}

// Stairwell ambient: reverberant enclosed space, distant door thuds every ~2s (6s loop)
static Sound gen_stairwell_ambient(void) {
    int len = SAMPLE_RATE * 6;
    Wave w = gen_wave(len);
    short *d = (short *)w.data;
    srand(777);
    for (int thud = 0; thud < 3; thud++) {
        int start = thud * SAMPLE_RATE * 2 + (rand() % (SAMPLE_RATE / 4));
        // Low thud — 60Hz, sharp attack, quick decay
        int thud_len = SAMPLE_RATE / 3;  // ~330ms with echo tail
        for (int i = 0; i < thud_len && (start + i) < len; i++) {
            float t = (float)i / SAMPLE_RATE;
            // Initial thud
            float thud_env = expf(-8.0f * t);
            float thud_sig = sinf(2 * PI * 60 * t) * thud_env;
            // Echo tail — filtered, longer decay
            float echo_env = expf(-2.0f * t) * 0.3f;
            float echo_sig = sinf(2 * PI * 55 * t) * echo_env;
            // High-freq click for the "door" sound
            float click = sinf(2 * PI * 800 * t) * expf(-30.0f * t) * 0.2f;
            float sample = (thud_sig + echo_sig + click) * 0.4f;
            d[start + i] += (short)(sample * 4000);
        }
    }
    // Loop crossfade
    int fade_len = SAMPLE_RATE / 20;
    for (int i = 0; i < fade_len; i++) {
        float f = (float)i / fade_len;
        d[i] = (short)(d[i] * f);
        d[len - 1 - i] = (short)(d[len - 1 - i] * f);
    }
    Sound s = LoadSoundFromWave(w); UnloadWave(w); return s;
}

// Wind: filtered noise with slow amplitude modulation (10s loop)
static Sound gen_wind(void) {
    int len = SAMPLE_RATE * 10;
    Wave w = gen_wave(len);
    short *d = (short *)w.data;
    float prev1 = 0, prev2 = 0, prev3 = 0;
    srand(888);
    for (int i = 0; i < len; i++) {
        float t = (float)i / SAMPLE_RATE;
        float lt = (float)i / len;
        // White noise source
        float noise = ((float)(rand() % 32768) / 16384.0f - 1.0f);
        // Band-pass via cascaded filters — mid frequency
        prev1 = prev1 * 0.85f + noise * 0.15f;
        prev2 = prev2 * 0.90f + prev1 * 0.10f;
        prev3 = prev3 * 0.80f + (prev1 - prev2) * 0.20f;  // high-pass element
        float filtered = prev2 + prev3 * 0.5f;
        // Slow amplitude modulation — gusting
        float gust = 0.5f + 0.3f * sinf(t * 0.8f) + 0.2f * sinf(t * 1.3f + 0.5f)
                     + 0.15f * sinf(t * 0.3f + 1.2f);
        // Loop crossfade
        float env = 1.0f;
        if (lt < 0.02f) env = lt / 0.02f;
        if (lt > 0.98f) env = (1.0f - lt) / 0.02f;
        d[i] = (short)(filtered * gust * env * 6000);
    }
    Sound s = LoadSoundFromWave(w); UnloadWave(w); return s;
}

// Elevator hum — 4 second low-frequency mechanical tone (40Hz with wobble)
static Sound gen_elevator_hum(void) {
    int len = SAMPLE_RATE * 4;
    Wave w = gen_wave(len);
    short *d = (short *)w.data;
    for (int i = 0; i < len; i++) {
        float t = (float)i / SAMPLE_RATE;
        float lt = (float)i / len;
        // Envelope: fade in 0.3s, sustain, fade out 0.5s
        float env = 1.0f;
        if (lt < 0.075f) env = lt / 0.075f;
        if (lt > 0.875f) env = (1.0f - lt) / 0.125f;
        // 40Hz fundamental with slight pitch wobble
        float wobble = sinf(2 * PI * 0.5f * t) * 2.0f;
        float tone = sinf(2 * PI * (40.0f + wobble) * t) * 0.6f;
        // Second harmonic for body
        tone += sinf(2 * PI * (80.0f + wobble * 1.5f) * t) * 0.25f;
        // Very faint high whine (motor)
        tone += sinf(2 * PI * 320.0f * t) * 0.05f;
        d[i] = (short)(tone * env * 3000);
    }
    Sound s = LoadSoundFromWave(w); UnloadWave(w); return s;
}

// Elevator ding — classic bell: 1200Hz + 1800Hz, sharp attack, 0.3s decay
static Sound gen_elevator_ding(void) {
    int len = SAMPLE_RATE / 2;
    Wave w = gen_wave(len);
    short *d = (short *)w.data;
    for (int i = 0; i < len; i++) {
        float t = (float)i / SAMPLE_RATE;
        float env = expf(-10.0f * t);
        float bell = sinf(2 * PI * 1200 * t) * 0.5f + sinf(2 * PI * 1800 * t) * 0.35f;
        // Subtle overtone
        bell += sinf(2 * PI * 2400 * t) * 0.1f * expf(-15.0f * t);
        d[i] = (short)(bell * env * 10000);
    }
    Sound s = LoadSoundFromWave(w); UnloadWave(w); return s;
}

// --- ENGINE ---

void InitEVAudio(EVAudio *audio) {
    memset(audio, 0, sizeof(EVAudio));
    InitAudioDevice();
    for (int i = 0; i < 4; i++) {
        audio->step_marble[i] = gen_step_marble(42+i*7);
        audio->step_carpet[i] = gen_step_carpet(100+i*11);
        audio->step_wood[i] = gen_step_wood(200+i*13);
    }
    audio->snd_click = gen_click();
    audio->snd_fabric = gen_fabric();
    audio->snd_flame = gen_flame();
    audio->snd_reward = gen_reward();
    audio->snd_sparkle = gen_sparkle();
    audio->door = gen_door_sound();
    audio->elevator_hum = gen_elevator_hum();
    audio->elevator_ding = gen_elevator_ding();
    audio->drone_lobby = gen_ambient_lobby();
    audio->drone_hallway = gen_ambient_hallway();
    audio->drone_room = gen_ambient_room();
    audio->snd_city = gen_city_ambient();
    audio->snd_clock = gen_clock_ambient();
    audio->snd_stairwell = gen_stairwell_ambient();
    audio->snd_wind = gen_wind();
    audio->city_playing = false;
    audio->clock_playing = false;
    audio->stairwell_playing = false;
    audio->wind_playing = false;
    audio->step_timer = 0;
    audio->step_interval = 0.50f;
    audio->step_index = 0;
    audio->ambient_playing = false;
    audio->initialized = true;

    for (int i = 0; i < 4; i++) {
        SetSoundVolume(audio->step_marble[i], 0.25f);
        SetSoundVolume(audio->step_carpet[i], 0.18f);
        SetSoundVolume(audio->step_wood[i], 0.25f);
    }
    SetSoundVolume(audio->snd_click, 0.4f);
    SetSoundVolume(audio->snd_fabric, 0.3f);
    SetSoundVolume(audio->snd_flame, 0.35f);
    SetSoundVolume(audio->snd_reward, 0.4f);
    SetSoundVolume(audio->snd_sparkle, 0.2f);
    SetSoundVolume(audio->door, 0.25f);
    SetSoundVolume(audio->elevator_hum, 0.04f);    // very quiet, more felt than heard
    SetSoundVolume(audio->elevator_ding, 0.3f);    // clear but not jarring
    SetSoundVolume(audio->drone_lobby, 0.04f);     // barely there — through walls
    SetSoundVolume(audio->drone_hallway, 0.03f);   // anticipation, not presence
    SetSoundVolume(audio->drone_room, 0.06f);      // the room's own music — present but not prominent
    SetSoundVolume(audio->snd_city, 0.03f);        // distant city
    SetSoundVolume(audio->snd_clock, 0.025f);      // clock — barely there
    SetSoundVolume(audio->snd_stairwell, 0.025f);  // distant door thuds — ambient
    SetSoundVolume(audio->snd_wind, 0.04f);        // wind — gentle
}

void UnloadEVAudio(EVAudio *audio) {
    if (!audio->initialized) return;
    for (int i = 0; i < 4; i++) {
        UnloadSound(audio->step_marble[i]);
        UnloadSound(audio->step_carpet[i]);
        UnloadSound(audio->step_wood[i]);
    }
    UnloadSound(audio->snd_click); UnloadSound(audio->snd_fabric);
    UnloadSound(audio->snd_flame); UnloadSound(audio->snd_reward);
    UnloadSound(audio->snd_sparkle); UnloadSound(audio->door);
    UnloadSound(audio->elevator_hum); UnloadSound(audio->elevator_ding);
    UnloadSound(audio->drone_lobby); UnloadSound(audio->drone_hallway);
    UnloadSound(audio->drone_room);
    UnloadSound(audio->snd_city); UnloadSound(audio->snd_clock);
    UnloadSound(audio->snd_stairwell); UnloadSound(audio->snd_wind);
    CloseAudioDevice();
    audio->initialized = false;
}

static Sound *get_steps(EVAudio *audio, SurfaceType s) {
    switch(s) { case SURFACE_MARBLE: return audio->step_marble;
                case SURFACE_CARPET: return audio->step_carpet;
                case SURFACE_WOOD: return audio->step_wood; }
    return audio->step_wood;
}
static Sound *get_drone(EVAudio *audio, DroneType t) {
    switch(t) { case DRONE_LOBBY: return &audio->drone_lobby;
                case DRONE_HALLWAY: return &audio->drone_hallway;
                case DRONE_ROOM: return &audio->drone_room; }
    return &audio->drone_room;
}

void UpdateEVAudio(EVAudio *audio, bool moving, bool sprinting, SurfaceType surface, float dt) {
    if (!audio->initialized) return;
    if (audio->ambient_playing) {
        Sound *drone = get_drone(audio, audio->current_drone);
        if (!IsSoundPlaying(*drone)) PlaySound(*drone);
    }
    if (audio->city_playing && !IsSoundPlaying(audio->snd_city)) PlaySound(audio->snd_city);
    if (audio->clock_playing && !IsSoundPlaying(audio->snd_clock)) PlaySound(audio->snd_clock);
    if (audio->stairwell_playing && !IsSoundPlaying(audio->snd_stairwell)) PlaySound(audio->snd_stairwell);
    if (audio->wind_playing && !IsSoundPlaying(audio->snd_wind)) PlaySound(audio->snd_wind);
    Sound *steps = get_steps(audio, surface);
    audio->step_interval = sprinting ? 0.32f : 0.50f;
    if (moving) {
        audio->step_timer += dt;
        if (audio->step_timer >= audio->step_interval) {
            audio->step_timer = 0;
            PlaySound(steps[audio->step_index]);
            audio->step_index = (audio->step_index + 1) % 4;
        }
    } else {
        audio->step_timer = audio->step_interval * 0.8f;
    }
}

void PlayInteract(EVAudio *audio, InteractSoundType type) {
    if (!audio->initialized) return;
    switch(type) { case INTERACT_CLICK: PlaySound(audio->snd_click); break;
                   case INTERACT_FABRIC: PlaySound(audio->snd_fabric); break;
                   case INTERACT_FLAME: PlaySound(audio->snd_flame); break; }
}
void PlayRewardSound(EVAudio *audio) { if(audio->initialized) PlaySound(audio->snd_reward); }
void PlaySparkleSound(EVAudio *audio) { if(audio->initialized) PlaySound(audio->snd_sparkle); }
void PlayDoorSound(EVAudio *audio) { if(audio->initialized) PlaySound(audio->door); }

void StartAmbient(EVAudio *audio, DroneType type) {
    if (!audio->initialized) return;
    if (audio->ambient_playing && audio->current_drone != type)
        StopSound(*get_drone(audio, audio->current_drone));
    audio->current_drone = type;
    if (!IsSoundPlaying(*get_drone(audio, type))) PlaySound(*get_drone(audio, type));
    audio->ambient_playing = true;
}
void StopAmbient(EVAudio *audio) {
    if (!audio->initialized || !audio->ambient_playing) return;
    StopSound(*get_drone(audio, audio->current_drone));
    audio->ambient_playing = false;
}

void PlayCityAmbient(EVAudio *audio) {
    if (!audio->initialized) return;
    if (!audio->city_playing) {
        PlaySound(audio->snd_city);
        audio->city_playing = true;
    }
}
void StopCityAmbient(EVAudio *audio) {
    if (!audio->initialized) return;
    StopSound(audio->snd_city);
    audio->city_playing = false;
}
void PlayClockAmbient(EVAudio *audio) {
    if (!audio->initialized) return;
    if (!audio->clock_playing) {
        PlaySound(audio->snd_clock);
        audio->clock_playing = true;
    }
}
void StopClockAmbient(EVAudio *audio) {
    if (!audio->initialized) return;
    StopSound(audio->snd_clock);
    audio->clock_playing = false;
}
void PlayStairwellAmbient(EVAudio *audio) {
    if (!audio->initialized) return;
    if (!audio->stairwell_playing) {
        PlaySound(audio->snd_stairwell);
        audio->stairwell_playing = true;
    }
}
void StopStairwellAmbient(EVAudio *audio) {
    if (!audio->initialized) return;
    StopSound(audio->snd_stairwell);
    audio->stairwell_playing = false;
}
void PlayWindAmbient(EVAudio *audio) {
    if (!audio->initialized) return;
    if (!audio->wind_playing) {
        PlaySound(audio->snd_wind);
        audio->wind_playing = true;
    }
}
void StopWindAmbient(EVAudio *audio) {
    if (!audio->initialized) return;
    StopSound(audio->snd_wind);
    audio->wind_playing = false;
}
void PlayElevatorHum(EVAudio *audio) {
    if (audio->initialized) PlaySound(audio->elevator_hum);
}
void PlayElevatorDing(EVAudio *audio) {
    if (audio->initialized) PlaySound(audio->elevator_ding);
}
void SetCityAmbientVolume(EVAudio *audio, float vol) {
    if (audio->initialized) SetSoundVolume(audio->snd_city, vol);
}
