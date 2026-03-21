// audio.c — Procedural audio engine
// NO CREEPY DRONES. Pleasant or silent. Bioshock vibes.
#include "audio.h"
#include "config.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SAMPLE_RATE 44100

static Wave gen_wave(int samples) {
    Wave w = {0};
    w.frameCount = samples;
    w.sampleRate = SAMPLE_RATE;
    w.sampleSize = 16;
    w.channels = 1;
    w.data = calloc(samples, sizeof(short));
    if (!w.data) {
        fprintf(stderr, "[EV] FATAL: audio calloc failed (%d samples)\n", samples);
        // Fallback: 1-sample silent buffer to prevent null deref in callers
        w.data = calloc(1, sizeof(short));
        w.frameCount = w.data ? 1 : 0;
    }
    return w;
}

// Deterministic PRNG — better than rand(), no global state
static float ev_randf(unsigned int *seed) {
    *seed = (*seed * 1103515245 + 12345);
    return (float)((*seed >> 16) & 0x7fff) / 32768.0f;
}

// --- FOOTSTEPS ---

static Sound gen_step_marble(int seed) {
    int len = SAMPLE_RATE / 8;  // longer for richer tail
    int reverb_delay = (int)(SAMPLE_RATE * 0.08f);
    int reverb2_delay = (int)(SAMPLE_RATE * 0.14f);  // second tap — marble hall
    int total = len + reverb2_delay;
    Wave w = gen_wave(total);
    short *d = (short *)w.data;
    unsigned int rng = (unsigned int)seed;
    float pitch = 0.9f + ev_randf(&rng) * 0.2f;
    for (int i = 0; i < len; i++) {
        float t = (float)i / SAMPLE_RATE;
        float lt = (float)i / len;
        float env = (lt < 0.02f) ? lt / 0.02f : expf(-12.0f * (lt - 0.02f));
        // CONTACT — sharp heel click, high freq transient
        float click = sinf(2 * PI * 2200 * pitch * t) * expf(-30.0f * lt);
        click += sinf(2 * PI * 3800 * pitch * t) * expf(-50.0f * lt) * 0.15f;  // upper harmonic
        // BODY — material resonance, marble has warm mid tone
        float body = sinf(2 * PI * 180 * pitch * t) * expf(-10.0f * lt);
        body += sinf(2 * PI * 360 * pitch * t) * expf(-14.0f * lt) * 0.2f;  // 2nd harmonic
        // DEBRIS — tiny grit particles scattering on smooth surface
        float debris = (ev_randf(&rng) * 2.0f - 1.0f) * expf(-25.0f * lt) * 0.15f;
        // TAIL — room reflection character (marble is reflective)
        float tail = sinf(2 * PI * 140 * pitch * t) * expf(-6.0f * lt) * 0.1f;
        d[i] = (short)((click * 0.25f + body * 0.35f + debris + tail) * env * 10000);
    }
    // Two-tap reverb — marble halls have early + late reflections
    for (int i = reverb_delay; i < total; i++)
        d[i] += (short)(d[i - reverb_delay] * 0.12f);
    for (int i = reverb2_delay; i < total; i++)
        d[i] += (short)(d[i - reverb2_delay] * 0.06f);
    w.frameCount = total;
    Sound s = LoadSoundFromWave(w); UnloadWave(w); return s;
}

static Sound gen_step_carpet(int seed) {
    int len = SAMPLE_RATE / 6;  // longer — carpet absorbs, slow decay
    int reverb_delay = (int)(SAMPLE_RATE * 0.06f);  // short reverb — carpet deadens room
    int total = len + reverb_delay;
    Wave w = gen_wave(total);
    short *d = (short *)w.data;
    unsigned int rng = (unsigned int)seed;
    float pitch = 0.85f + ev_randf(&rng) * 0.3f;
    for (int i = 0; i < len; i++) {
        float t = (float)i / SAMPLE_RATE;
        float lt = (float)i / len;
        float env = (lt < 0.08f) ? lt / 0.08f : expf(-6.0f * (lt - 0.08f));
        // CONTACT — muffled thud, no sharp transient (carpet absorbs)
        float thud = sinf(2 * PI * 45 * pitch * t) * expf(-8.0f * lt);
        thud += sinf(2 * PI * 90 * pitch * t) * expf(-12.0f * lt) * 0.15f;
        // BODY — fabric compression
        float fabric = (ev_randf(&rng) * 2.0f - 1.0f) * expf(-10.0f * lt) * 0.12f;
        // FIBER RUSTLE — very quiet high-frequency scatter (carpet fibers)
        float rustle = (ev_randf(&rng) * 2.0f - 1.0f) * expf(-30.0f * lt) * 0.05f;
        d[i] = (short)((thud * 0.7f + fabric + rustle) * env * 7000);
    }
    // Minimal reverb — carpet rooms are dead
    for (int i = reverb_delay; i < total; i++)
        d[i] += (short)(d[i - reverb_delay] * 0.05f);
    w.frameCount = total;
    Sound s = LoadSoundFromWave(w); UnloadWave(w); return s;
}

static Sound gen_step_wood(int seed) {
    int len = SAMPLE_RATE / 8;  // medium length
    int reverb_delay = (int)(SAMPLE_RATE * 0.10f);
    int reverb2_delay = (int)(SAMPLE_RATE * 0.18f);  // wood rooms ring
    int total = len + reverb2_delay;
    Wave w = gen_wave(total);
    short *d = (short *)w.data;
    unsigned int rng = (unsigned int)seed;
    float pitch = 0.85f + ev_randf(&rng) * 0.3f;
    for (int i = 0; i < len; i++) {
        float t = (float)i / SAMPLE_RATE;
        float lt = (float)i / len;
        float env = (lt < 0.03f) ? lt / 0.03f : expf(-10.0f * (lt - 0.03f));
        // CONTACT — sharp wooden tap, prominent
        float tap = sinf(2 * PI * 400 * pitch * t) * expf(-20.0f * lt);
        tap += sinf(2 * PI * 800 * pitch * t) * expf(-35.0f * lt) * 0.12f;  // knock overtone
        // BODY — wood panel resonance, warm low
        float body = sinf(2 * PI * 80 * pitch * t) * expf(-8.0f * lt);
        body += sinf(2 * PI * 160 * pitch * t) * expf(-12.0f * lt) * 0.15f;
        // CREAK — very subtle, occasional board flex
        float creak = sinf(2 * PI * 1200 * pitch * t * (1.0f + lt * 0.5f)) * expf(-40.0f * lt) * 0.04f;
        d[i] = (short)((tap * 0.4f + body * 0.4f + creak) * env * 9000);
    }
    // Two-tap reverb — wood rooms ring nicely
    for (int i = reverb_delay; i < total; i++)
        d[i] += (short)(d[i - reverb_delay] * 0.1f);
    for (int i = reverb2_delay; i < total; i++) {
        d[i] += (short)(d[i - reverb2_delay] * 0.05f);
    }
    w.frameCount = total;
    Sound s = LoadSoundFromWave(w); UnloadWave(w); return s;
}

// --- AMBIENT — NOT DRONES. Musical, pleasant, short loops ---

// Lobby: same melody as room but one octave higher, slower decay (reverb-like)
// Heard through walls — distant, ethereal
// 32 seconds with A/B sections and a second phrase
static Sound gen_ambient_lobby(void) {
    float beat = 0.6f;
    float loop_len = 32.0f * beat * 2;  // doubled: 32 bars
    int len = (int)(SAMPLE_RATE * loop_len);
    int reverb_delay = (int)(SAMPLE_RATE * 0.1f);
    Wave w = gen_wave(len);
    short *d = (short *)w.data;
    unsigned int timing_rng = 42;

    // A section (bars 1-8): original descending phrase, octave up
    // B section (bars 9-16): new phrase — ascending, different character
    // A' section (bars 17-24): return with variation
    // C section (bars 25-32): sparse, breathing, resolving
    float melody[][3] = {
        // A section (bars 1-8)
        {659.26f, 0, 2}, {587.33f, 2, 1}, {523.25f, 3, 1},
        {440.00f, 4, 4},
        {659.26f, 8, 2}, {698.46f, 10, 1}, {659.26f, 11, 1},
        {587.33f, 12, 4},
        // B section (bars 9-16) — new phrase, shifts harmony
        {783.99f, 16, 2}, {698.46f, 18, 1}, {659.26f, 19, 1},
        {523.25f, 20, 4},
        {783.99f, 24, 1}, {698.46f, 25, 1}, {659.26f, 26, 2},
        {587.33f, 28, 4},
        // A' section (bars 17-24) — return, slight variation
        {659.26f, 32, 2}, {587.33f, 34, 1}, {523.25f, 35, 1},
        {440.00f, 36, 4},
        {659.26f, 40, 2}, {587.33f, 42, 2},
        // C section (bars 25-32) — sparse, resolving
        {523.25f, 48, 4}, {440.00f, 52, 4},
        {659.26f, 56, 2}, {587.33f, 58, 1}, {523.25f, 59, 1},
        // rest — breathing room, silence in bars 61-64
    };
    int note_count = 25;

    for (int i = 0; i < len; i++) {
        float t = (float)i / SAMPLE_RATE;
        float sample = 0;
        for (int n = 0; n < note_count; n++) {
            float freq = melody[n][0];
            // Subtle timing humanization: +-20ms per note
            unsigned int note_seed = (unsigned int)(n * 997 + 7);
            float time_offset = (ev_randf(&note_seed) - 0.5f) * 0.04f;
            float start = melody[n][1] * beat + time_offset;
            float dur = melody[n][2] * beat;
            float nt = t - start;
            if (nt < 0 || nt > dur + 3.0f) continue;
            float noise_burst = 0;
            if (nt < 0.01f) {
                unsigned int ns = (unsigned int)(n * 997 + i);
                noise_burst = (ev_randf(&ns) * 2.0f - 1.0f) * (1.0f - nt / 0.01f) * 0.2f;
            }
            float attack = (nt < 0.05f) ? nt / 0.05f : 1.0f;
            float env = attack * expf(-0.8f * nt);
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
        d[i] = (short)(sample * 1800);
    }
    (void)timing_rng;
    for (int i = reverb_delay; i < len; i++) {
        d[i] += (short)(d[i - reverb_delay] * 0.15f);
    }
    Sound s = LoadSoundFromWave(w); UnloadWave(w); return s;
}

// Hallway: sustained root tones — building anticipation
// Doubled length (32s), with a sustained "through-the-wall" string note
// that fades in and out over 8 seconds like distant strings
static Sound gen_ambient_hallway(void) {
    float beat = 0.6f;
    float loop_len = 32.0f * beat * 2;  // doubled
    int len = (int)(SAMPLE_RATE * loop_len);
    int reverb_delay = (int)(SAMPLE_RATE * 0.1f);
    Wave w = gen_wave(len);
    short *d = (short *)w.data;

    // Root notes — sustained, hollow, now with a B section
    float roots[][3] = {
        // A section
        {164.81f, 0, 8},    // E3
        {110.00f, 8, 8},    // A2
        {164.81f, 16, 8},   // E3
        {146.83f, 24, 8},   // D3
        // B section — different roots, more tension
        {130.81f, 32, 8},   // C3 — new color
        {110.00f, 40, 8},   // A2
        {146.83f, 48, 8},   // D3
        {164.81f, 56, 8},   // E3 — resolving back
    };
    int root_count = 8;

    // Sustained "through-the-wall" string — A3 (220Hz), fades in/out over 8s cycles
    float string_freq = 220.0f;

    for (int i = 0; i < len; i++) {
        float t = (float)i / SAMPLE_RATE;
        float sample = 0;

        // Root notes with timing humanization
        for (int n = 0; n < root_count; n++) {
            float freq = roots[n][0];
            unsigned int note_seed = (unsigned int)(n * 1013 + 3);
            float time_offset = (ev_randf(&note_seed) - 0.5f) * 0.04f;
            float start = roots[n][1] * beat + time_offset;
            float dur = roots[n][2] * beat;
            float nt = t - start;
            if (nt < 0 || nt > dur) continue;
            float noise_burst = 0;
            if (nt < 0.01f) {
                unsigned int ns = (unsigned int)(n * 1013 + i);
                noise_burst = (ev_randf(&ns) * 2.0f - 1.0f) * (1.0f - nt / 0.01f) * 0.15f;
            }
            float attack = (nt < 0.1f) ? nt / 0.1f : 1.0f;
            float release = (nt > dur - 0.5f) ? (dur - nt) / 0.5f : 1.0f;
            float env = attack * release;
            float detune = 0.5f;
            float tone = sinf(2 * PI * freq * t) +
                         0.3f * sinf(2 * PI * (freq * 2.0f + detune) * t) +
                         0.1f * sinf(2 * PI * (freq * 3.0f - detune) * t) +
                         0.05f * sinf(2 * PI * freq * 5.0f * t);
            sample += (tone * env + noise_burst);
        }

        // Distant string — fades in and out over 8-second cycles
        // Like hearing music through a hotel wall
        float string_env = 0.5f + 0.5f * sinf(2 * PI * t / 8.0f);
        string_env *= string_env;  // smooth the curve
        float string_tone = sinf(2 * PI * string_freq * t) * 0.15f +
                           sinf(2 * PI * (string_freq * 2.001f) * t) * 0.06f +
                           sinf(2 * PI * (string_freq * 3.002f) * t) * 0.03f;
        sample += string_tone * string_env;

        float lt = (float)i / len;
        if (lt > 0.95f) sample *= (1.0f - lt) / 0.05f;
        if (lt < 0.02f) sample *= lt / 0.02f;
        d[i] = (short)(sample * 2200);
    }
    for (int i = reverb_delay; i < len; i++) {
        d[i] += (short)(d[i - reverb_delay] * 0.15f);
    }
    Sound s = LoadSoundFromWave(w); UnloadWave(w); return s;
}

// Room: composed piano melody — Satie-inspired, now with ABAB' structure
// 16 bars, 32 seconds — twice as long with a B section and breathing room
// Rich harmonics, hammer noise attack, varied bass, delay reverb, timing humanization
static Sound gen_ambient_room(void) {
    float beat = 0.6f;  // ~100 BPM
    float loop_len = 32.0f * beat * 2;  // 16 bars of 4 beats = 32 seconds
    int len = (int)(SAMPLE_RATE * loop_len);
    int reverb_delay = (int)(SAMPLE_RATE * 0.1f);
    Wave w = gen_wave(len);
    short *d = (short *)w.data;

    // 16-bar melody with B section:
    // Bars 1-4 (A): E4, D4, C4, A3 — the original descending phrase
    // Bars 5-8 (A): E4, F4, E4, D4 — ascending variation
    // Bars 9-12 (B): G4, F4, E4, C4 — new phrase, shifts harmony
    // Bars 13-16 (A'): E4, D4, C4, rest — ends with silence
    float melody[][3] = {
        // A section — bars 1-4
        {329.63f, 0, 2}, {293.66f, 2, 1}, {261.63f, 3, 1},  // E4, D4, C4
        {220.00f, 4, 4},                                       // A3 whole
        // A section — bars 5-8
        {329.63f, 8, 2}, {349.23f, 10, 1}, {329.63f, 11, 1},  // E4, F4, E4
        {293.66f, 12, 4},                                       // D4 whole
        // B section — bars 9-12 (new phrase, G4 entry)
        {392.00f, 16, 2}, {349.23f, 18, 1}, {329.63f, 19, 1}, // G4, F4, E4
        {261.63f, 20, 4},                                       // C4 whole
        {392.00f, 24, 1}, {349.23f, 25, 1}, {329.63f, 26, 2}, // G4, F4, E4
        {293.66f, 28, 4},                                       // D4 whole
        // A' section — bars 13-16 (resolving, then silence)
        {329.63f, 32, 2}, {293.66f, 34, 1}, {261.63f, 35, 1}, // E4, D4, C4
        {220.00f, 36, 4},                                       // A3 whole
        {329.63f, 40, 3},                                       // E4 — lingering
        // bars 44-48: rest — breathing room, silence
        // bars 49-64: second half mirrors with slight variation
        // Second A
        {329.63f, 48, 2}, {293.66f, 50, 1}, {261.63f, 51, 1},
        {220.00f, 52, 4},
        {329.63f, 56, 2}, {349.23f, 58, 1}, {329.63f, 59, 1},
        {293.66f, 60, 4},
    };
    int note_count = 26;

    // Bass notes — varies by section
    // Bars 1-8: bass on A2 (110Hz)
    // Bars 9-12: bass on C3 (130.81Hz) — new root, harmonic shift
    // Bars 13-16: bass on A2 resolving
    float bass[][3] = {
        {110.00f, 4, 4},    // A2 under bar 2
        {110.00f, 8, 4},    // A2 under bar 3
        {110.00f, 12, 4},   // A2 under bar 4
        {130.81f, 16, 4},   // C3 under bar 5 (B section!)
        {130.81f, 20, 4},   // C3 under bar 6
        {130.81f, 24, 4},   // C3 under bar 7
        {130.81f, 28, 4},   // C3 under bar 8
        {110.00f, 32, 4},   // A2 resolving
        {110.00f, 36, 4},   // A2
        // Second half bass
        {110.00f, 52, 4},
        {110.00f, 56, 4},
        {110.00f, 60, 4},
    };
    int bass_count = 12;

    for (int i = 0; i < len; i++) {
        float t = (float)i / SAMPLE_RATE;
        float sample = 0;

        // Melody notes — rich harmonics with timing humanization
        for (int n = 0; n < note_count; n++) {
            float freq = melody[n][0];
            // Subtle timing variation: +-20ms per note (not machine-perfect)
            unsigned int note_seed = (unsigned int)(n * 997 + 13);
            float time_offset = (ev_randf(&note_seed) - 0.5f) * 0.04f;
            float start = melody[n][1] * beat + time_offset;
            float dur = melody[n][2] * beat;
            float nt = t - start;
            if (nt < 0 || nt > dur + 2.0f) continue;
            float noise_burst = 0;
            if (nt < 0.01f) {
                unsigned int ns = (unsigned int)(n * 997 + i);
                noise_burst = (ev_randf(&ns) * 2.0f - 1.0f) * (1.0f - nt / 0.01f) * 0.25f;
            }
            float attack = (nt < 0.05f) ? nt / 0.05f : 1.0f;
            float env = attack * expf(-1.5f * nt);
            float tone = sinf(2 * PI * freq * t) +
                         0.3f * sinf(2 * PI * freq * 2.0f * t) +
                         0.1f * sinf(2 * PI * freq * 3.0f * t) +
                         0.05f * sinf(2 * PI * freq * 5.0f * t);
            sample += (tone * env + noise_burst) * 0.7f;
        }

        // Bass notes — soft, warm, supporting, with humanization
        for (int n = 0; n < bass_count; n++) {
            float freq = bass[n][0];
            unsigned int bass_seed = (unsigned int)(n * 1103 + 7);
            float bass_offset = (ev_randf(&bass_seed) - 0.5f) * 0.03f;
            float start = bass[n][1] * beat + bass_offset;
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

    // Reverb approximation
    for (int i = reverb_delay; i < len; i++) {
        d[i] += (short)(d[i - reverb_delay] * 0.15f);
    }

    Sound s = LoadSoundFromWave(w); UnloadWave(w); return s;
}

// --- SPACE AMBIENTS — hull resonance, not Earth ambience ---

// Space Lobby: vast pressurized hull. Deep sub-bass hum (40Hz) with slow filter sweep.
// Metallic resonance — you feel the mass of the station around you.
static Sound gen_ambient_space_lobby(void) {
    float loop_len = 20.0f;
    int len = (int)(SAMPLE_RATE * loop_len);
    int reverb_delay = (int)(SAMPLE_RATE * 0.28f);  // LONGER reverb — cathedral, not box
    int early_delay = (int)(SAMPLE_RATE * 0.06f);   // early reflections
    Wave w = gen_wave(len);
    short *d = (short *)w.data;

    for (int i = 0; i < len; i++) {
        float t = (float)i / SAMPLE_RATE;
        float lt = (float)i / len;
        // Deep sub-bass — hull vibration
        float sub = sinf(2 * PI * 40.0f * t) * 0.5f;
        // Slow filter sweep — metallic resonance shifting
        float sweep = 80.0f + 40.0f * sinf(2 * PI * t / 12.0f);
        float resonance = sinf(2 * PI * sweep * t) * 0.2f;
        // Very high harmonic — distant electrical hum
        float high = sinf(2 * PI * 320.0f * t) * 0.04f;
        // Breathing amplitude — station life support rhythm
        float breath = 0.7f + 0.3f * sinf(2 * PI * t / 8.0f);
        float sample = (sub + resonance + high) * breath;
        // Loop crossfade
        if (lt < 0.02f) sample *= lt / 0.02f;
        if (lt > 0.98f) sample *= (1.0f - lt) / 0.02f;
        d[i] = (short)(sample * 2000);
    }
    // Early reflections — glass surfaces
    for (int i = early_delay; i < len; i++) {
        d[i] += (short)(d[i - early_delay] * 0.08f);
    }
    // Late reverb — expansive cathedral tail
    for (int i = reverb_delay; i < len; i++) {
        d[i] += (short)(d[i - reverb_delay] * 0.22f);
    }
    Sound s = LoadSoundFromWave(w); UnloadWave(w); return s;
}

// Space Corridor: glass walkway — light, airy, not mechanical.
// Longer reverb (glass reflections), reduced mechanical whine.
static Sound gen_ambient_space_corridor(void) {
    float loop_len = 16.0f;
    int len = (int)(SAMPLE_RATE * loop_len);
    int reverb_delay = (int)(SAMPLE_RATE * 0.18f);  // longer — glass reflects sound
    Wave w = gen_wave(len);
    short *d = (short *)w.data;

    for (int i = 0; i < len; i++) {
        float t = (float)i / SAMPLE_RATE;
        float lt = (float)i / len;
        // Medium hum — air ducting (reduced)
        float duct = sinf(2 * PI * 65.0f * t) * 0.30f;
        duct += sinf(2 * PI * 130.0f * t) * 0.08f;  // less mechanical harmonic
        // Rhythmic pulse — air circulation, slower
        float pulse = 0.7f + 0.3f * sinf(2 * PI * 0.6f * t);
        // Reduced whine — glass walkway, not fluorescent tube
        float whine = sinf(2 * PI * 440.0f * t) * 0.008f;
        whine += sinf(2 * PI * 442.0f * t) * 0.008f;
        float sample = (duct * pulse + whine);
        // Loop crossfade
        if (lt < 0.02f) sample *= lt / 0.02f;
        if (lt > 0.98f) sample *= (1.0f - lt) / 0.02f;
        d[i] = (short)(sample * 2200);
    }
    for (int i = reverb_delay; i < len; i++) {
        d[i] += (short)(d[i - reverb_delay] * 0.15f);
    }
    Sound s = LoadSoundFromWave(w); UnloadWave(w); return s;
}

// Space Suite: near-silence. The luxury of insulation.
// Occasional soft tone emerging from quiet — warmest of the three.
static Sound gen_ambient_space_suite(void) {
    float loop_len = 24.0f;
    int len = (int)(SAMPLE_RATE * loop_len);
    int reverb_delay = (int)(SAMPLE_RATE * 0.12f);
    Wave w = gen_wave(len);
    short *d = (short *)w.data;
    unsigned int rng = 314;

    // Sparse tones that fade in and out — like hearing the station breathe
    float tones[][3] = {
        {220.0f, 2.0f, 3.0f},   // A3 — warm, fading in at 2s
        {196.0f, 8.0f, 4.0f},   // G3 — gentle
        {261.63f, 14.0f, 3.0f}, // C4 — brief brightness
        {220.0f, 20.0f, 3.0f},  // A3 — return
    };
    int tone_count = 4;

    for (int i = 0; i < len; i++) {
        float t = (float)i / SAMPLE_RATE;
        float lt = (float)i / len;
        // Very quiet base — you can barely hear the hull
        float base = sinf(2 * PI * 35.0f * t) * 0.1f;
        float sample = base;
        // Sparse tones
        for (int n = 0; n < tone_count; n++) {
            float freq = tones[n][0];
            float start = tones[n][1];
            float dur = tones[n][2];
            float nt = t - start;
            if (nt < 0 || nt > dur + 2.0f) continue;
            // Slow fade in/out — gentle bell
            float attack = (nt < 0.5f) ? nt / 0.5f : 1.0f;
            float env = attack * expf(-0.5f * nt);
            // Pure tone with slight detune — warm, not clinical
            float tone = sinf(2 * PI * freq * t) * 0.6f +
                         sinf(2 * PI * (freq * 2.001f) * t) * 0.15f;
            sample += tone * env * 0.15f;
        }
        // Loop crossfade
        if (lt < 0.03f) sample *= lt / 0.03f;
        if (lt > 0.97f) sample *= (1.0f - lt) / 0.03f;
        d[i] = (short)(sample * 1800);
    }
    (void)rng;
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

// Cork pop — sharp percussive burst with gas release hiss
static Sound gen_cork_pop(void) {
    int len = SAMPLE_RATE / 3;
    Wave w = gen_wave(len);
    short *d = (short *)w.data;
    unsigned int rng = 42;
    for (int i = 0; i < len; i++) {
        float t = (float)i / SAMPLE_RATE;
        // Initial pop — very short, percussive (< 5ms)
        float pop = 0;
        if (t < 0.005f) {
            float pt = t / 0.005f;
            pop = sinf(2 * PI * 800 * t) * (1.0f - pt) * 0.8f;
            pop += (ev_randf(&rng) * 2.0f - 1.0f) * (1.0f - pt) * 0.6f;
        }
        // Gas release hiss — filtered noise, 30ms-200ms
        float hiss = 0;
        if (t > 0.003f && t < 0.2f) {
            float ht = (t - 0.003f) / 0.197f;
            float henv = (ht < 0.1f) ? ht / 0.1f : expf(-8.0f * (ht - 0.1f));
            float noise = ev_randf(&rng) * 2.0f - 1.0f;
            // Band-pass for fizzy character
            hiss = noise * henv * 0.35f;
        }
        // Low thunk — the cork body resonance
        float thunk = sinf(2 * PI * 180 * t) * expf(-25.0f * t) * 0.4f;
        d[i] = (short)((pop + hiss + thunk) * 12000);
    }
    Sound s = LoadSoundFromWave(w); UnloadWave(w); return s;
}

// Glass clink — crystal resonance, high bell with beating harmonics
static Sound gen_glass_clink(void) {
    int len = SAMPLE_RATE;
    Wave w = gen_wave(len);
    short *d = (short *)w.data;
    for (int i = 0; i < len; i++) {
        float t = (float)i / SAMPLE_RATE;
        // Sharp initial contact
        float contact = sinf(2 * PI * 4200 * t) * expf(-60.0f * t) * 0.3f;
        // Primary crystal resonance — two close frequencies beat
        float ring1 = sinf(2 * PI * 2800 * t) * expf(-3.0f * t) * 0.35f;
        float ring2 = sinf(2 * PI * 2820 * t) * expf(-3.0f * t) * 0.25f;
        // Upper harmonic shimmer
        float shimmer = sinf(2 * PI * 5600 * t) * expf(-5.0f * t) * 0.1f;
        // Low body
        float body = sinf(2 * PI * 900 * t) * expf(-4.0f * t) * 0.15f;
        d[i] = (short)((contact + ring1 + ring2 + shimmer + body) * 8000);
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

// --- THROUGH-WALL SOUNDS — inaccessible spaces communicate ---
// "Rooms the player hears but never enters. Music through a wall.
//  A light under a door that goes out." — Commandment #6

// Muffled piano through hotel wall — you hear someone else's room
// Low-pass filtered version of the room melody with heavy reverb
// 16 seconds — long enough to feel like eavesdropping
static Sound gen_muffled_piano(void) {
    float beat = 0.6f;
    float loop_len = 16.0f * beat * 2;
    int len = (int)(SAMPLE_RATE * loop_len);
    int reverb_delay1 = (int)(SAMPLE_RATE * 0.12f);
    int reverb_delay2 = (int)(SAMPLE_RATE * 0.23f);  // second tap — room behind room
    Wave w = gen_wave(len);
    short *d = (short *)w.data;

    // Same melody as room but transposed down a 5th — heard through wall
    float melody[][3] = {
        {220.0f, 0, 2}, {196.0f, 2, 1}, {174.6f, 3, 1},
        {146.8f, 4, 4},
        {220.0f, 8, 2}, {233.1f, 10, 1}, {220.0f, 11, 1},
        {196.0f, 12, 4},
        {261.6f, 16, 2}, {233.1f, 18, 1}, {220.0f, 19, 1},
        {174.6f, 20, 4},
        {220.0f, 24, 2}, {196.0f, 26, 2},
    };
    int note_count = 14;

    // Heavy low-pass state — wall absorbs highs
    float lp1 = 0, lp2 = 0;

    for (int i = 0; i < len; i++) {
        float t = (float)i / SAMPLE_RATE;
        float sample = 0;
        for (int n = 0; n < note_count; n++) {
            float freq = melody[n][0];
            unsigned int seed = (unsigned int)(n * 997 + 31);
            float offset = (ev_randf(&seed) - 0.5f) * 0.06f;  // more sloppy timing
            float start = melody[n][1] * beat + offset;
            float dur = melody[n][2] * beat;
            float nt = t - start;
            if (nt < 0 || nt > dur + 3.0f) continue;
            float attack = (nt < 0.08f) ? nt / 0.08f : 1.0f;
            float env = attack * expf(-1.0f * nt);  // longer decay — wall reverb
            // Only fundamental and 2nd harmonic — wall eats the rest
            float tone = sinf(2 * PI * freq * t) +
                         0.2f * sinf(2 * PI * freq * 2.0f * t);
            sample += tone * env * 0.5f;
        }
        // Two-pole low-pass — simulates wall absorption
        lp1 = lp1 * 0.92f + sample * 0.08f;
        lp2 = lp2 * 0.90f + lp1 * 0.10f;
        float lt = (float)i / len;
        if (lt > 0.95f) lp2 *= (1.0f - lt) / 0.05f;
        if (lt < 0.02f) lp2 *= lt / 0.02f;
        d[i] = (short)(lp2 * 2200);
    }
    // Double reverb — two delay taps, wall + room
    for (int i = reverb_delay1; i < len; i++)
        d[i] += (short)(d[i - reverb_delay1] * 0.18f);
    for (int i = reverb_delay2; i < len; i++)
        d[i] += (short)(d[i - reverb_delay2] * 0.10f);

    Sound s = LoadSoundFromWave(w); UnloadWave(w); return s;
}

// Distant conversation murmur — filtered noise shaped like speech rhythm
// You hear voices but never words. Other lives happening in parallel.
static Sound gen_distant_voices(void) {
    float loop_len = 12.0f;
    int len = (int)(SAMPLE_RATE * loop_len);
    Wave w = gen_wave(len);
    short *d = (short *)w.data;
    unsigned int rng = 1337;
    float lp1 = 0, lp2 = 0, lp3 = 0;

    for (int i = 0; i < len; i++) {
        float t = (float)i / SAMPLE_RATE;
        float lt = (float)i / len;
        // Noise source
        float noise = ev_randf(&rng) * 2.0f - 1.0f;
        // Speech rhythm — amplitude modulation at ~3-5Hz (syllable rate)
        float syllable = 0.3f + 0.7f * fabsf(sinf(2 * PI * 3.8f * t + sinf(t * 1.2f) * 2.0f));
        // Phrase pauses — drops to near-zero every 2-4 seconds
        float phrase = 0.5f + 0.5f * sinf(2 * PI * t / 3.2f);
        phrase *= 0.6f + 0.4f * sinf(2 * PI * t / 5.7f);
        // Band-pass for voice character (200-600Hz)
        lp1 = lp1 * 0.85f + noise * 0.15f;
        lp2 = lp2 * 0.92f + lp1 * 0.08f;
        lp3 = lp3 * 0.95f + lp2 * 0.05f;
        float voice = (lp2 - lp3) * 3.0f;  // band-pass via subtraction
        float sample = voice * syllable * phrase;
        // Loop crossfade
        if (lt < 0.03f) sample *= lt / 0.03f;
        if (lt > 0.97f) sample *= (1.0f - lt) / 0.03f;
        d[i] = (short)(sample * 1200);
    }
    Sound s = LoadSoundFromWave(w); UnloadWave(w); return s;
}

// Footsteps above — someone walking in the room upstairs
// Muffled thuds with slow, irregular rhythm
static Sound gen_footsteps_above(void) {
    float loop_len = 10.0f;
    int len = (int)(SAMPLE_RATE * loop_len);
    Wave w = gen_wave(len);
    short *d = (short *)w.data;
    unsigned int rng = 2718;

    // 8-12 steps at irregular intervals
    int step_count = 8 + (int)(ev_randf(&rng) * 4);
    float step_times[16];
    float t_acc = 0.5f;
    for (int s = 0; s < step_count && s < 16; s++) {
        step_times[s] = t_acc;
        t_acc += 0.5f + ev_randf(&rng) * 0.4f;  // irregular spacing
        if (t_acc > loop_len - 0.5f) { step_count = s + 1; break; }
    }

    for (int i = 0; i < len; i++) {
        float t = (float)i / SAMPLE_RATE;
        float lt = (float)i / len;
        float sample = 0;
        for (int s = 0; s < step_count; s++) {
            float st = t - step_times[s];
            if (st < 0 || st > 0.15f) continue;
            // Low thud — ceiling absorbs all highs
            float env = expf(-25.0f * st);
            float thud = sinf(2 * PI * 50.0f * st) * env * 0.6f;
            thud += sinf(2 * PI * 90.0f * st) * env * 0.3f;
            sample += thud;
        }
        if (lt < 0.02f) sample *= lt / 0.02f;
        if (lt > 0.98f) sample *= (1.0f - lt) / 0.02f;
        d[i] = (short)(sample * 4000);
    }
    Sound s = LoadSoundFromWave(w); UnloadWave(w); return s;
}

// Forward declarations for sounds defined below InitEVAudio
static Sound gen_bed_drone(void);
static Sound gen_held_chord(void);
static Sound gen_bed_ritual(void);
static Sound gen_three_note(void);
static Sound gen_taxi_radio(void);
static Sound gen_running_water(void);
static Sound gen_tv_murmur(void);
static Sound gen_rain(void);
static Sound gen_dream_rain(void);
static Sound gen_dream_traffic(void);
static Sound gen_hyperspace_tone(void);
static Sound gen_hyperspace_riser(void);
static Sound gen_arrival_thump(void);
static Sound gen_elevator_whoosh(void);
static Sound gen_door_thud(void);
static Sound gen_airlock_hiss(void);
static Sound gen_gravity_settle(void);
static Sound gen_bed_impact(void);
static Sound gen_balcony_gust(void);
static Sound gen_title_breath(void);
static Sound gen_hard_cut_punch(void);
static Sound gen_phone_ring(void);
static Sound gen_earth_presence(void);

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
    audio->snd_cork_pop = gen_cork_pop();
    audio->snd_glass_clink = gen_glass_clink();
    audio->snd_reward = gen_reward();
    audio->snd_sparkle = gen_sparkle();
    audio->door = gen_door_sound();
    audio->elevator_hum = gen_elevator_hum();
    audio->elevator_ding = gen_elevator_ding();
    audio->drone_lobby = gen_ambient_lobby();
    audio->drone_hallway = gen_ambient_hallway();
    audio->drone_room = gen_ambient_room();
    audio->drone_space_lobby = gen_ambient_space_lobby();
    audio->drone_space_corridor = gen_ambient_space_corridor();
    audio->drone_space_suite = gen_ambient_space_suite();
    audio->snd_city = gen_city_ambient();
    audio->snd_clock = gen_clock_ambient();
    audio->snd_stairwell = gen_stairwell_ambient();
    audio->snd_wind = gen_wind();
    audio->snd_muffled_piano = gen_muffled_piano();
    audio->snd_distant_voices = gen_distant_voices();
    audio->snd_footsteps_above = gen_footsteps_above();
    audio->snd_bed_drone = gen_bed_drone();
    audio->snd_held_chord = gen_held_chord();
    audio->snd_bed_ritual = gen_bed_ritual();
    audio->snd_three_note = gen_three_note();
    audio->snd_taxi_radio = gen_taxi_radio();
    audio->snd_running_water = gen_running_water();
    audio->snd_tv_murmur = gen_tv_murmur();
    audio->snd_hyperspace_tone = gen_hyperspace_tone();
    audio->snd_hyperspace_riser = gen_hyperspace_riser();
    audio->snd_arrival_thump = gen_arrival_thump();
    audio->snd_elevator_whoosh = gen_elevator_whoosh();
    audio->snd_door_thud = gen_door_thud();
    audio->snd_airlock_hiss = gen_airlock_hiss();
    audio->snd_gravity_settle = gen_gravity_settle();
    audio->snd_bed_impact = gen_bed_impact();
    audio->snd_balcony_gust = gen_balcony_gust();
    audio->snd_title_breath = gen_title_breath();
    audio->snd_hard_cut_punch = gen_hard_cut_punch();
    audio->snd_phone_ring = gen_phone_ring();
    audio->snd_earth_presence = gen_earth_presence();
    audio->snd_rain = gen_rain();
    audio->snd_dream_rain = gen_dream_rain();
    audio->snd_dream_traffic = gen_dream_traffic();
    audio->city_playing = false;
    audio->clock_playing = false;
    audio->stairwell_playing = false;
    audio->wind_playing = false;
    audio->muffled_piano_playing = false;
    audio->distant_voices_playing = false;
    audio->footsteps_above_playing = false;
    audio->bed_drone_playing = false;
    audio->held_chord_playing = false;
    audio->bed_ritual_playing = false;
    audio->bed_ritual_played_once = false;
    audio->taxi_radio_playing = false;
    audio->hyperspace_tone_playing = false;
    audio->hyperspace_riser_playing = false;
    audio->elevator_whoosh_playing = false;
    audio->earth_presence_playing = false;
    audio->dream_rain_playing = false;
    audio->dream_traffic_playing = false;
    audio->clock_rate = 1.0f;
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
    SetSoundVolume(audio->snd_cork_pop, 0.45f);
    SetSoundVolume(audio->snd_glass_clink, 0.35f);
    SetSoundVolume(audio->snd_reward, 0.4f);
    SetSoundVolume(audio->snd_sparkle, 0.2f);
    SetSoundVolume(audio->door, 0.25f);
    SetSoundVolume(audio->elevator_hum, 0.04f);    // very quiet, more felt than heard
    SetSoundVolume(audio->elevator_ding, 0.3f);    // clear but not jarring
    SetSoundVolume(audio->drone_lobby, 0.04f);     // barely there — through walls
    SetSoundVolume(audio->drone_hallway, 0.03f);   // anticipation, not presence
    SetSoundVolume(audio->drone_room, 0.06f);      // the room's own music — present but not prominent
    SetSoundVolume(audio->drone_space_lobby, 0.05f);     // hull presence — felt
    SetSoundVolume(audio->drone_space_corridor, 0.04f);  // air circulation — tighter
    SetSoundVolume(audio->drone_space_suite, 0.03f);     // near-silence — luxury
    SetSoundVolume(audio->snd_city, 0.03f);        // distant city
    SetSoundVolume(audio->snd_clock, 0.025f);      // clock — barely there
    SetSoundVolume(audio->snd_stairwell, 0.025f);  // distant door thuds — ambient
    SetSoundVolume(audio->snd_wind, 0.04f);        // wind — gentle
    SetSoundVolume(audio->snd_muffled_piano, 0.02f);     // barely there — someone else's room
    SetSoundVolume(audio->snd_distant_voices, 0.015f);   // murmur — other lives
    SetSoundVolume(audio->snd_footsteps_above, 0.02f);   // thuds — felt more than heard
    SetSoundVolume(audio->snd_bed_drone, 0.04f);         // low hum — felt, not heard
    SetSoundVolume(audio->snd_held_chord, 0.05f);        // credits chord — present but gentle
    SetSoundVolume(audio->snd_bed_ritual, 0.06f);        // THE piece — warm, present, emotional center
    SetSoundVolume(audio->snd_three_note, 0.04f);        // callback fragment — ghostly, brief
    SetSoundVolume(audio->snd_taxi_radio, 0.025f);       // lo-fi car radio — warm background
    SetSoundVolume(audio->snd_running_water, 0.015f);    // behind door — muffled
    SetSoundVolume(audio->snd_tv_murmur, 0.015f);        // behind door — muffled
    SetSoundVolume(audio->snd_hyperspace_tone, 0.06f);   // rising tone — builds tension
    SetSoundVolume(audio->snd_hyperspace_riser, 0.08f);  // fat riser — overwhelming crescendo
    SetSoundVolume(audio->snd_arrival_thump, 0.12f);     // landing impact — felt in chest
    SetSoundVolume(audio->snd_elevator_whoosh, 0.05f);   // ascending noise — building dread
    SetSoundVolume(audio->snd_door_thud, 0.10f);         // heavy door — authoritative
    SetSoundVolume(audio->snd_airlock_hiss, 0.06f);      // pressurization — ambient but present
    SetSoundVolume(audio->snd_gravity_settle, 0.05f);    // hull groan — subtle, structural
    SetSoundVolume(audio->snd_bed_impact, 0.08f);        // cushioned surrender
    SetSoundVolume(audio->snd_balcony_gust, 0.07f);      // void rushing in
    SetSoundVolume(audio->snd_title_breath, 0.04f);      // barely there — subliminal
    SetSoundVolume(audio->snd_hard_cut_punch, 0.10f);    // physical cut — snappy
    SetSoundVolume(audio->snd_phone_ring, 0.03f);        // muffled — through walls, unanswered
    SetSoundVolume(audio->snd_earth_presence, 0.0f);     // starts silent — distance-controlled
    SetSoundVolume(audio->snd_rain, 0.045f);             // Auckland rain — present, not overwhelming
    SetSoundVolume(audio->snd_dream_rain, 0.035f);      // rain on glass — gentle, persistent
    SetSoundVolume(audio->snd_dream_traffic, 0.02f);    // distant cars — barely there, ground-floor
    audio->rain_playing = false;
}

void UnloadEVAudio(EVAudio *audio) {
    if (!audio->initialized) return;
    for (int i = 0; i < EV_AUDIO_TRANSIENT_SLOTS; i++) {
        if (!audio->transient_active[i]) continue;
        StopSound(audio->transient_sounds[i]);
        UnloadSound(audio->transient_sounds[i]);
        audio->transient_sounds[i] = (Sound){0};
        audio->transient_ttls[i] = 0.0f;
        audio->transient_active[i] = false;
    }
    for (int i = 0; i < 4; i++) {
        UnloadSound(audio->step_marble[i]);
        UnloadSound(audio->step_carpet[i]);
        UnloadSound(audio->step_wood[i]);
    }
    UnloadSound(audio->snd_click); UnloadSound(audio->snd_fabric);
    UnloadSound(audio->snd_flame); UnloadSound(audio->snd_cork_pop);
    UnloadSound(audio->snd_glass_clink); UnloadSound(audio->snd_reward);
    UnloadSound(audio->snd_sparkle); UnloadSound(audio->door);
    UnloadSound(audio->elevator_hum); UnloadSound(audio->elevator_ding);
    UnloadSound(audio->drone_lobby); UnloadSound(audio->drone_hallway);
    UnloadSound(audio->drone_room);
    UnloadSound(audio->drone_space_lobby); UnloadSound(audio->drone_space_corridor);
    UnloadSound(audio->drone_space_suite);
    UnloadSound(audio->snd_city); UnloadSound(audio->snd_clock);
    UnloadSound(audio->snd_stairwell); UnloadSound(audio->snd_wind);
    UnloadSound(audio->snd_muffled_piano); UnloadSound(audio->snd_distant_voices);
    UnloadSound(audio->snd_footsteps_above);
    UnloadSound(audio->snd_bed_drone); UnloadSound(audio->snd_held_chord);
    UnloadSound(audio->snd_bed_ritual); UnloadSound(audio->snd_three_note);
    UnloadSound(audio->snd_taxi_radio);
    UnloadSound(audio->snd_running_water); UnloadSound(audio->snd_tv_murmur);
    UnloadSound(audio->snd_hyperspace_tone);
    UnloadSound(audio->snd_hyperspace_riser);
    UnloadSound(audio->snd_arrival_thump);
    UnloadSound(audio->snd_elevator_whoosh);
    UnloadSound(audio->snd_door_thud);
    UnloadSound(audio->snd_airlock_hiss);
    UnloadSound(audio->snd_gravity_settle);
    UnloadSound(audio->snd_bed_impact);
    UnloadSound(audio->snd_balcony_gust);
    UnloadSound(audio->snd_title_breath);
    UnloadSound(audio->snd_hard_cut_punch);
    UnloadSound(audio->snd_phone_ring);
    UnloadSound(audio->snd_earth_presence);
    UnloadSound(audio->snd_rain);
    UnloadSound(audio->snd_dream_rain);
    UnloadSound(audio->snd_dream_traffic);
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
                case DRONE_ROOM: return &audio->drone_room;
                case DRONE_SPACE_LOBBY: return &audio->drone_space_lobby;
                case DRONE_SPACE_CORRIDOR: return &audio->drone_space_corridor;
                case DRONE_SPACE_SUITE: return &audio->drone_space_suite; }
    return &audio->drone_room;
}

// Base volume per drone type — used for ducking/crossfade restoration
static float get_drone_base_vol(EVAudio *audio, DroneType t) {
    (void)audio;
    switch(t) { case DRONE_LOBBY: return 0.04f;
                case DRONE_HALLWAY: return 0.03f;
                case DRONE_ROOM: return 0.06f;
                case DRONE_SPACE_LOBBY: return 0.05f;
                case DRONE_SPACE_CORRIDOR: return 0.04f;
                case DRONE_SPACE_SUITE: return 0.03f; }
    return 0.04f;
}

static void update_transient_sounds(EVAudio *audio, float dt) {
    for (int i = 0; i < EV_AUDIO_TRANSIENT_SLOTS; i++) {
        if (!audio->transient_active[i]) continue;
        audio->transient_ttls[i] -= dt;
        if (audio->transient_ttls[i] > 0.0f || IsSoundPlaying(audio->transient_sounds[i])) continue;
        UnloadSound(audio->transient_sounds[i]);
        audio->transient_sounds[i] = (Sound){0};
        audio->transient_ttls[i] = 0.0f;
        audio->transient_active[i] = false;
    }
}

static void queue_transient_sound(EVAudio *audio, Sound snd, float ttl) {
    int slot = -1;

    for (int i = 0; i < EV_AUDIO_TRANSIENT_SLOTS; i++) {
        if (!audio->transient_active[i]) {
            slot = i;
            break;
        }
    }
    if (slot < 0) {
        slot = 0;
        for (int i = 1; i < EV_AUDIO_TRANSIENT_SLOTS; i++) {
            if (audio->transient_ttls[i] < audio->transient_ttls[slot]) slot = i;
        }
        StopSound(audio->transient_sounds[slot]);
        UnloadSound(audio->transient_sounds[slot]);
    }

    audio->transient_sounds[slot] = snd;
    audio->transient_ttls[slot] = ttl;
    audio->transient_active[slot] = true;
}

void UpdateEVAudio(EVAudio *audio, bool moving, bool sprinting, SurfaceType surface, float dt) {
    if (!audio->initialized) return;
    update_transient_sounds(audio, dt);

    // ── Ducking: briefly lower ambient when interactions fire ──
    float duck_mult = 1.0f;
    if (audio->duck_timer > 0) {
        audio->duck_timer -= dt;
        if (audio->duck_timer < 0) audio->duck_timer = 0;
        // Smooth duck envelope: quick attack, slow release
        float duck_env = audio->duck_timer > 0.05f ? 1.0f : audio->duck_timer / 0.05f;
        duck_mult = 1.0f - audio->duck_amount * duck_env;
    }

    // ── Drone crossfade ──
    if (audio->crossfade_timer > 0) {
        audio->crossfade_timer -= dt;
        float t = audio->crossfade_timer / audio->crossfade_duration;  // 1→0
        // Fade out current drone
        Sound *old_drone = get_drone(audio, audio->current_drone);
        SetSoundVolume(*old_drone, get_drone_base_vol(audio, audio->current_drone) * t * duck_mult);
        // Fade in new drone
        Sound *new_drone = get_drone(audio, audio->next_drone);
        if (!IsSoundPlaying(*new_drone)) PlaySound(*new_drone);
        SetSoundVolume(*new_drone, get_drone_base_vol(audio, audio->next_drone) * (1.0f - t) * duck_mult);

        if (audio->crossfade_timer <= 0) {
            // Crossfade complete — switch
            StopSound(*old_drone);
            audio->current_drone = audio->next_drone;
            audio->crossfade_timer = 0;
        }
    } else if (audio->ambient_playing) {
        Sound *drone = get_drone(audio, audio->current_drone);
        if (!IsSoundPlaying(*drone)) PlaySound(*drone);
        // Apply ducking to drone volume
        if (duck_mult < 1.0f) {
            SetSoundVolume(*drone, get_drone_base_vol(audio, audio->current_drone) * duck_mult);
        }
    }

    // Scene ambients — loop when playing, apply ducking
    if (audio->city_playing && !IsSoundPlaying(audio->snd_city)) PlaySound(audio->snd_city);
    if (audio->clock_playing && !IsSoundPlaying(audio->snd_clock)) PlaySound(audio->snd_clock);
    if (audio->stairwell_playing && !IsSoundPlaying(audio->snd_stairwell)) PlaySound(audio->snd_stairwell);
    if (audio->wind_playing && !IsSoundPlaying(audio->snd_wind)) PlaySound(audio->snd_wind);
    // Through-wall sounds — loop when playing
    if (audio->muffled_piano_playing && !IsSoundPlaying(audio->snd_muffled_piano)) PlaySound(audio->snd_muffled_piano);
    if (audio->distant_voices_playing && !IsSoundPlaying(audio->snd_distant_voices)) PlaySound(audio->snd_distant_voices);
    if (audio->footsteps_above_playing && !IsSoundPlaying(audio->snd_footsteps_above)) PlaySound(audio->snd_footsteps_above);

    // Footsteps — pitch + timing variation (not metronomic)
    Sound *steps = get_steps(audio, surface);
    float base_interval = sprinting ? 0.32f : 0.50f;
    // ±8% timing jitter — human gait is never perfectly regular
    audio->step_interval = base_interval * (0.92f + (float)(GetRandomValue(0, 160)) / 1000.0f);
    if (moving) {
        audio->step_timer += dt;
        if (audio->step_timer >= audio->step_interval) {
            audio->step_timer = 0;
            // Pitch variation ±5%
            SetSoundPitch(steps[audio->step_index], 0.95f + (float)(GetRandomValue(0, 100)) / 1000.0f);
            // Volume varies slightly per step (±10%) — weight distribution shifts
            float vol_base = (surface == SURFACE_CARPET) ? 0.18f :
                            (surface == SURFACE_WOOD) ? 0.25f : 0.25f;
            float vol_jitter = vol_base * (0.9f + (float)(GetRandomValue(0, 200)) / 1000.0f);
            SetSoundVolume(steps[audio->step_index], vol_jitter);
            PlaySound(steps[audio->step_index]);
            audio->step_index = (audio->step_index + 1) % 4;
        }
    } else {
        audio->step_timer = audio->step_interval * 0.8f;
    }
}

// Pitch micro-variation — makes interactions feel alive
static float interact_pitch_var(void) {
    // ±5% pitch randomization
    return 0.95f + (float)(GetRandomValue(0, 100)) / 1000.0f;
}

void PlayInteract(EVAudio *audio, InteractSoundType type) {
    if (!audio->initialized) return;
    Sound *snd = NULL;
    switch(type) {
        case INTERACT_CLICK:      snd = &audio->snd_click; break;
        case INTERACT_FABRIC:     snd = &audio->snd_fabric; break;
        case INTERACT_FLAME:      snd = &audio->snd_flame; break;
        case INTERACT_CORK_POP:   snd = &audio->snd_cork_pop; break;
        case INTERACT_GLASS_CLINK: snd = &audio->snd_glass_clink; break;
    }
    if (snd) {
        SetSoundPitch(*snd, interact_pitch_var());
        PlaySound(*snd);
        // Trigger ducking — briefly lower ambient so interaction reads
        audio->duck_timer = 0.3f;
        audio->duck_amount = 0.3f;
    }
}
void PlayRewardSound(EVAudio *audio) {
    if (!audio->initialized) return;
    SetSoundPitch(audio->snd_reward, interact_pitch_var());
    PlaySound(audio->snd_reward);
    audio->duck_timer = 0.5f;  // longer duck for reward
    audio->duck_amount = 0.4f;
}
void PlaySparkleSound(EVAudio *audio) { if(audio->initialized) PlaySound(audio->snd_sparkle); }
void PlayDoorSound(EVAudio *audio) { if(audio->initialized) PlaySound(audio->door); }

void StartAmbient(EVAudio *audio, DroneType type) {
    if (!audio->initialized) return;
    if (audio->ambient_playing && audio->current_drone != type) {
        // Crossfade: start fading out current drone, queue new one
        audio->next_drone = type;
        audio->crossfade_timer = 1.0f;  // 1 second crossfade
        audio->crossfade_duration = 1.0f;
        return;  // don't start new drone yet — UpdateEVAudio handles the fade
    }
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

// Through-wall sounds — Commandment #6: inaccessible spaces communicate
void PlayMuffledPiano(EVAudio *audio) {
    if (!audio->initialized) return;
    if (!audio->muffled_piano_playing) {
        PlaySound(audio->snd_muffled_piano);
        audio->muffled_piano_playing = true;
    }
}
void StopMuffledPiano(EVAudio *audio) {
    if (!audio->initialized) return;
    StopSound(audio->snd_muffled_piano);
    audio->muffled_piano_playing = false;
}
void PlayDistantVoices(EVAudio *audio) {
    if (!audio->initialized) return;
    if (!audio->distant_voices_playing) {
        PlaySound(audio->snd_distant_voices);
        audio->distant_voices_playing = true;
    }
}
void StopDistantVoices(EVAudio *audio) {
    if (!audio->initialized) return;
    StopSound(audio->snd_distant_voices);
    audio->distant_voices_playing = false;
}
void PlayFootstepsAbove(EVAudio *audio) {
    if (!audio->initialized) return;
    if (!audio->footsteps_above_playing) {
        PlaySound(audio->snd_footsteps_above);
        audio->footsteps_above_playing = true;
    }
}
void StopFootstepsAbove(EVAudio *audio) {
    if (!audio->initialized) return;
    StopSound(audio->snd_footsteps_above);
    audio->footsteps_above_playing = false;
}

// ── Sprint 1: Bed drone — low ~50Hz, 20-second loop ────────────────
// The sound of surrender. Fades in with the ceiling.
static Sound gen_bed_drone(void) {
    int len = SAMPLE_RATE * 20;
    Wave w = gen_wave(len);
    short *d = (short *)w.data;
    for (int i = 0; i < len; i++) {
        float t = (float)i / SAMPLE_RATE;
        float lt = (float)i / len;
        // Envelope: slow fade in, sustain, fade out for loop
        float env = 1.0f;
        if (lt < 0.05f) env = lt / 0.05f;
        if (lt > 0.95f) env = (1.0f - lt) / 0.05f;
        // 50Hz fundamental — felt more than heard
        float tone = sinf(2 * PI * 50 * t) * 0.6f;
        // Second harmonic — warmth
        tone += sinf(2 * PI * 100 * t) * 0.2f;
        // Very slow amplitude modulation — breathing
        float breath = 0.7f + 0.3f * sinf(t * 0.4f);
        d[i] = (short)(tone * env * breath * 3000);
    }
    Sound s = LoadSoundFromWave(w); UnloadWave(w); return s;
}

// ── Sprint 1: Held chord — stacked fifths (C3-G3-D4) ──────────────
// String ensemble timbre. Credits as elegy.
static Sound gen_held_chord(void) {
    int len = SAMPLE_RATE * 20;  // 20s loop
    Wave w = gen_wave(len);
    short *d = (short *)w.data;
    // Stacked fifths: C3-G3-D4
    float freqs[] = {130.81f, 196.00f, 293.66f};
    for (int i = 0; i < len; i++) {
        float t = (float)i / SAMPLE_RATE;
        float lt = (float)i / len;
        // 3-second attack, infinite sustain, loop crossfade
        float env = fminf(1.0f, t / 3.0f);
        if (lt > 0.95f) env *= (1.0f - lt) / 0.05f;
        float chord = 0;
        for (int n = 0; n < 3; n++) {
            float f = freqs[n];
            // Fundamental + chorus detuning (string ensemble warmth)
            chord += sinf(2 * PI * f * t) * 0.25f;
            chord += sinf(2 * PI * f * 1.003f * t) * 0.08f;
            chord += sinf(2 * PI * f * 0.997f * t) * 0.08f;
            // 2nd harmonic — body
            chord += sinf(2 * PI * f * 2.0f * t) * 0.06f;
            // 3rd harmonic — presence
            chord += sinf(2 * PI * f * 3.0f * t) * 0.02f;
        }
        // Gentle breathing modulation
        float breath = 0.9f + 0.1f * sinf(t * 0.3f);
        d[i] = (short)(chord * env * breath * 3500);
    }
    Sound s = LoadSoundFromWave(w); UnloadWave(w); return s;
}

void PlayBedDrone(EVAudio *audio) {
    if (!audio->initialized) return;
    if (!audio->bed_drone_playing) {
        PlaySound(audio->snd_bed_drone);
        audio->bed_drone_playing = true;
    }
}
void StopBedDrone(EVAudio *audio) {
    if (!audio->initialized) return;
    StopSound(audio->snd_bed_drone);
    audio->bed_drone_playing = false;
}
void PlayHeldChord(EVAudio *audio) {
    if (!audio->initialized) return;
    if (!audio->held_chord_playing) {
        PlaySound(audio->snd_held_chord);
        audio->held_chord_playing = true;
    }
}
void StopHeldChord(EVAudio *audio) {
    if (!audio->initialized) return;
    StopSound(audio->snd_held_chord);
    audio->held_chord_playing = false;
}

// ── Bed Ritual — THE composed piece ──────────────────────────────────
// Chevalier warmth + Beau Is Afraid energy. A happy love song in a sad place.
// Plays ONCE in the bed scene. Never repeats. The emotional center of the game.
// Major key melody over warm chord bed — bittersweet because of WHERE it plays.
static Sound gen_bed_ritual(void) {
    int len = SAMPLE_RATE * 32;  // 32 seconds — long enough to hold, short enough to ache
    Wave w = gen_wave(len);
    short *d = (short *)w.data;

    // Key of D major — warm, open, the key of hope
    // Chord progression: D - A/C# - Bm - G (I - V/vii - vi - IV)
    // Each chord held 8 seconds
    float chord_freqs[4][4] = {
        {146.83f, 220.00f, 293.66f, 440.00f},  // D: D3, A3, D4, A4
        {138.59f, 220.00f, 277.18f, 440.00f},  // A/C#: C#3, A3, C#4, A4
        {123.47f, 185.00f, 246.94f, 369.99f},  // Bm: B2, F#3, B3, F#4
        {98.00f, 196.00f, 246.94f, 392.00f},   // G: G2, G3, B3, G4
    };

    // Melody — simple, singable, the kind of tune you'd hum in a hotel room
    // Pentatonic fragments over each chord change
    float melody_freqs[] = {
        587.33f, 659.26f, 493.88f, 440.00f,  // D5, E5, B4, A4 (over D)
        554.37f, 493.88f, 440.00f, 369.99f,  // C#5, B4, A4, F#4 (over A/C#)
        493.88f, 440.00f, 369.99f, 329.63f,  // B4, A4, F#4, E4 (over Bm)
        440.00f, 493.88f, 587.33f, 659.26f,  // A4, B4, D5, E5 (over G — lifts)
    };
    float melody_times[] = {
        0.0f, 2.0f, 4.5f, 6.0f,
        8.0f, 10.5f, 12.5f, 14.0f,
        16.0f, 18.0f, 20.5f, 22.0f,
        24.0f, 26.0f, 28.0f, 30.0f,
    };

    unsigned int rng = 42;
    for (int i = 0; i < len; i++) {
        float t = (float)i / SAMPLE_RATE;
        float lt = (float)i / len;

        // Global envelope — 4s fade in, sustain, 4s fade out
        float env = 1.0f;
        if (t < 4.0f) env = t / 4.0f;
        if (lt > 0.875f) env = (1.0f - lt) / 0.125f;

        // Chord bed — warm string ensemble with chorus detuning
        int chord_idx = (int)(t / 8.0f);
        if (chord_idx > 3) chord_idx = 3;
        float chord_blend = fmodf(t, 8.0f) / 8.0f;
        float chord = 0;
        for (int n = 0; n < 4; n++) {
            float f = chord_freqs[chord_idx][n];
            // Crossfade between chords at boundaries
            if (chord_blend < 0.05f && chord_idx > 0) {
                float prev_f = chord_freqs[chord_idx-1][n];
                float xf = chord_blend / 0.05f;
                f = prev_f * (1.0f - xf) + f * xf;
            }
            // String ensemble — fundamental + chorus + harmonics
            chord += sinf(2 * PI * f * t) * 0.15f;
            chord += sinf(2 * PI * f * 1.002f * t) * 0.05f;  // chorus
            chord += sinf(2 * PI * f * 0.998f * t) * 0.05f;
            chord += sinf(2 * PI * f * 2.0f * t) * 0.03f;    // octave
        }

        // Melody — bell-like tone, clear above the bed
        float melody = 0;
        for (int m = 0; m < 16; m++) {
            float mt = t - melody_times[m];
            if (mt >= 0 && mt < 3.0f) {
                float menv = expf(-1.5f * mt);  // bell decay
                float mf = melody_freqs[m];
                melody += sinf(2 * PI * mf * t) * menv * 0.2f;
                melody += sinf(2 * PI * mf * 2.0f * t) * menv * 0.04f;  // shimmer
            }
        }

        // Gentle breathing modulation — the room inhales
        float breath = 0.9f + 0.1f * sinf(t * 0.25f);

        // Subtle noise texture — warmth, not digital
        float noise = (ev_randf(&rng) * 2.0f - 1.0f) * 0.005f;

        d[i] = (short)((chord + melody + noise) * env * breath * 4000);
    }
    Sound s = LoadSoundFromWave(w); UnloadWave(w); return s;
}

// ── Three-note callback — the piece returns as a ghost ───────────────
// Three notes of the bed ritual melody. Then silence. The game's last music.
static Sound gen_three_note(void) {
    int len = SAMPLE_RATE * 6;  // 6 seconds — 3 notes + reverb tail
    Wave w = gen_wave(len);
    short *d = (short *)w.data;

    // The first three melody notes from the ritual: D5, E5, B4
    // Played slower, more fragile — a memory of the melody
    float notes[] = {587.33f, 659.26f, 493.88f};
    float times[] = {0.5f, 2.0f, 3.5f};

    int reverb_delay = (int)(SAMPLE_RATE * 0.3f);

    for (int i = 0; i < len; i++) {
        float t = (float)i / SAMPLE_RATE;
        float lt = (float)i / len;
        float env = 1.0f;
        if (lt > 0.8f) env = (1.0f - lt) / 0.2f;

        float tone = 0;
        for (int n = 0; n < 3; n++) {
            float nt = t - times[n];
            if (nt >= 0 && nt < 2.5f) {
                float nenv = expf(-1.0f * nt);  // slower decay than ritual — lingering
                float f = notes[n];
                // Thinner timbre — solo, exposed, vulnerable
                tone += sinf(2 * PI * f * t) * nenv * 0.25f;
                tone += sinf(2 * PI * f * 1.003f * t) * nenv * 0.06f;
                tone += sinf(2 * PI * f * 0.997f * t) * nenv * 0.06f;
            }
        }
        d[i] = (short)(tone * env * 4500);
    }
    // Reverb tail — the notes linger in the void
    for (int i = reverb_delay; i < len; i++)
        d[i] += (short)(d[i - reverb_delay] * 0.25f);

    Sound s = LoadSoundFromWave(w); UnloadWave(w); return s;
}

void PlayBedRitual(EVAudio *audio) {
    if (!audio->initialized) return;
    if (audio->bed_ritual_played_once) return;  // NEVER again
    if (!audio->bed_ritual_playing) {
        PlaySound(audio->snd_bed_ritual);
        audio->bed_ritual_playing = true;
        audio->bed_ritual_played_once = true;
    }
}
void StopBedRitual(EVAudio *audio) {
    if (!audio->initialized) return;
    StopSound(audio->snd_bed_ritual);
    audio->bed_ritual_playing = false;
}
void PlayThreeNote(EVAudio *audio) {
    if (!audio->initialized) return;
    PlaySound(audio->snd_three_note);
}

// ── Taxi radio — warm lo-fi song ─────────────────────────────────
// Something you'd listen to together. Cuts off at hyperspace.
// G major pentatonic melody over gentle chords — lo-fi car radio warmth.
static Sound gen_taxi_radio(void) {
    int len = SAMPLE_RATE * 20;  // 20s loop — enough for the taxi ride
    Wave w = gen_wave(len);
    short *d = (short *)w.data;
    unsigned int rng = 77;

    // Lo-fi warmth: gentle G major chord bed + simple melody
    // G2-B2-D3 chord base (low, warm, muffled by car speakers)
    float chord_freqs[] = {98.0f, 123.47f, 146.83f};

    // Melody: pentatonic, singable — G4, A4, B4, D5, E5
    float mel[] = {392.0f, 440.0f, 493.88f, 587.33f, 659.26f};
    float mel_times[] = {0.5f, 2.0f, 3.5f, 5.0f, 6.5f, 8.5f, 10.0f, 11.5f, 13.0f, 14.5f, 16.5f, 18.0f};
    int mel_notes[] =   {0, 2, 4, 3, 1, 0, 2, 1, 3, 4, 2, 0};
    int mel_count = 12;

    for (int i = 0; i < len; i++) {
        float t = (float)i / SAMPLE_RATE;
        float lt = (float)i / len;

        // Loop envelope
        float env = 1.0f;
        if (lt < 0.02f) env = lt / 0.02f;
        if (lt > 0.98f) env = (1.0f - lt) / 0.02f;

        // Chord bed — muffled, lo-fi (low-pass via fewer harmonics)
        float chord = 0;
        for (int n = 0; n < 3; n++) {
            float f = chord_freqs[n];
            chord += sinf(2 * PI * f * t) * 0.12f;
            chord += sinf(2 * PI * f * 1.004f * t) * 0.03f;  // detuned for warmth
        }

        // Melody — soft, slightly distorted (car speaker character)
        float melody = 0;
        for (int m = 0; m < mel_count; m++) {
            float mt = t - mel_times[m];
            if (mt >= 0 && mt < 2.0f) {
                float menv = expf(-2.0f * mt);
                float mf = mel[mel_notes[m]];
                melody += sinf(2 * PI * mf * t) * menv * 0.15f;
                // Slight harmonic distortion — car speaker
                melody += sinf(2 * PI * mf * 2.0f * t) * menv * 0.02f;
            }
        }

        // Lo-fi noise floor — car cabin hiss
        float hiss = (ev_randf(&rng) * 2.0f - 1.0f) * 0.008f;

        d[i] = (short)((chord + melody + hiss) * env * 3000);
    }
    Sound s = LoadSoundFromWave(w); UnloadWave(w); return s;
}

void PlayTaxiRadio(EVAudio *audio) {
    if (!audio->initialized) return;
    if (!audio->taxi_radio_playing) {
        PlaySound(audio->snd_taxi_radio);
        audio->taxi_radio_playing = true;
    }
}
void StopTaxiRadio(EVAudio *audio) {
    if (!audio->initialized) return;
    StopSound(audio->snd_taxi_radio);
    audio->taxi_radio_playing = false;
}

// ── Sprint 2: Clock rate modulation ────────────────────────────────
void SetClockRate(EVAudio *audio, float rate) {
    if (!audio->initialized) return;
    audio->clock_rate = rate;
    if (audio->clock_playing) {
        SetSoundPitch(audio->snd_clock, fmaxf(0.01f, rate));
        if (rate < 0.01f) {
            StopSound(audio->snd_clock);
            audio->clock_playing = false;
        }
    }
}

// ── Sprint 2: Per-door spatial sounds ──────────────────────────────
// Filtered noise — running water behind a bathroom door
static Sound gen_running_water(void) {
    int len = SAMPLE_RATE * 10;
    Wave w = gen_wave(len);
    short *d = (short *)w.data;
    float prev = 0;
    unsigned int rng = 5555;
    for (int i = 0; i < len; i++) {
        float lt = (float)i / len;
        float noise = ev_randf(&rng) * 2.0f - 1.0f;
        // Heavy low-pass — muffled through wall
        prev = prev * 0.92f + noise * 0.08f;
        // Slow modulation — water pressure variation
        float mod = 0.7f + 0.3f * sinf((float)i / SAMPLE_RATE * 0.6f);
        // Loop envelope
        float env = 1.0f;
        if (lt < 0.02f) env = lt / 0.02f;
        if (lt > 0.98f) env = (1.0f - lt) / 0.02f;
        d[i] = (short)(prev * mod * env * 5000);
    }
    Sound s = LoadSoundFromWave(w); UnloadWave(w); return s;
}

// Modulated noise bursts — TV behind a door
static Sound gen_tv_murmur(void) {
    int len = SAMPLE_RATE * 10;
    Wave w = gen_wave(len);
    short *d = (short *)w.data;
    unsigned int rng = 6666;
    float prev1 = 0, prev2 = 0;
    for (int i = 0; i < len; i++) {
        float t = (float)i / SAMPLE_RATE;
        float lt = (float)i / len;
        float noise = ev_randf(&rng) * 2.0f - 1.0f;
        // Band-pass — speech-like frequencies
        prev1 = prev1 * 0.85f + noise * 0.15f;
        prev2 = prev2 * 0.9f + prev1 * 0.1f;
        float filtered = prev1 - prev2;
        // Burst modulation — simulates speech rhythm
        float burst = 0.3f + 0.7f * fmaxf(0, sinf(t * 3.5f) * sinf(t * 1.7f));
        // Loop envelope
        float env = 1.0f;
        if (lt < 0.02f) env = lt / 0.02f;
        if (lt > 0.98f) env = (1.0f - lt) / 0.02f;
        d[i] = (short)(filtered * burst * env * 4000);
    }
    Sound s = LoadSoundFromWave(w); UnloadWave(w); return s;
}

// Rain on window — filtered high-frequency noise, looped
// B&W sonic palette: compressed, intimate, Paris ground-floor hotel
static Sound gen_dream_rain(void) {
    int len = SAMPLE_RATE * 12;
    Wave w = gen_wave(len);
    short *d = (short *)w.data;
    unsigned int rng = 7777;
    float prev1 = 0, prev2 = 0;
    for (int i = 0; i < len; i++) {
        float t = (float)i / SAMPLE_RATE;
        float lt = (float)i / len;
        float noise = ev_randf(&rng) * 2.0f - 1.0f;
        // High-pass filter chain — rain is bright but distant
        prev1 = prev1 * 0.6f + noise * 0.4f;
        prev2 = prev2 * 0.7f + prev1 * 0.3f;
        float rain = prev1 - prev2 * 0.8f;
        // Droplet clusters — irregular rhythm
        float cluster = 0.4f + 0.6f * fmaxf(0, sinf(t * 2.1f) * sinf(t * 5.3f));
        // Slow intensity swell — rain comes and goes
        float swell = 0.6f + 0.4f * sinf(t * 0.3f);
        // Loop envelope
        float env = 1.0f;
        if (lt < 0.03f) env = lt / 0.03f;
        if (lt > 0.97f) env = (1.0f - lt) / 0.03f;
        d[i] = (short)(rain * cluster * swell * env * 3500);
    }
    Sound s = LoadSoundFromWave(w); UnloadWave(w); return s;
}

// Distant traffic — low rumble with occasional swells
// The street outside a Paris hotel at night
static Sound gen_dream_traffic(void) {
    int len = SAMPLE_RATE * 15;
    Wave w = gen_wave(len);
    short *d = (short *)w.data;
    unsigned int rng = 8888;
    float prev = 0;
    for (int i = 0; i < len; i++) {
        float t = (float)i / SAMPLE_RATE;
        float lt = (float)i / len;
        float noise = ev_randf(&rng) * 2.0f - 1.0f;
        // Very heavy low-pass — distant rumble, felt more than heard
        prev = prev * 0.96f + noise * 0.04f;
        // Car pass-by swells — doppler-ish
        float swell1 = 0.3f + 0.7f * fmaxf(0, sinf(t * 0.4f));
        float swell2 = 0.5f + 0.5f * fmaxf(0, sinf(t * 0.15f + 1.5f));
        float pass = swell1 * swell2;
        // Loop envelope
        float env = 1.0f;
        if (lt < 0.04f) env = lt / 0.04f;
        if (lt > 0.96f) env = (1.0f - lt) / 0.04f;
        d[i] = (short)(prev * pass * env * 4000);
    }
    Sound s = LoadSoundFromWave(w); UnloadWave(w); return s;
}

// Auckland rain — you're outside in it. Wider, wetter, more present than dream rain.
static Sound gen_rain(void) {
    int len = SAMPLE_RATE * 16;  // 16 second loop
    Wave w = gen_wave(len);
    short *d = (short *)w.data;
    unsigned int rng = 42424;
    float prev1 = 0, prev2 = 0, prev3 = 0;
    for (int i = 0; i < len; i++) {
        float t = (float)i / SAMPLE_RATE;
        float lt = (float)i / len;
        float noise = ev_randf(&rng) * 2.0f - 1.0f;
        // Bandpass: keep mid-high frequencies (rain on pavement is brighter than pipe water)
        prev1 = prev1 * 0.5f + noise * 0.5f;
        prev2 = prev2 * 0.75f + prev1 * 0.25f;
        prev3 = prev3 * 0.85f + prev2 * 0.15f;
        float rain = prev1 - prev3 * 0.6f;
        // Droplet impacts — sharp transients layered on top
        float drop1 = ev_randf(&rng);
        if (drop1 > 0.997f) rain += (ev_randf(&rng) - 0.5f) * 2.0f;  // occasional loud drop
        // Intensity swell — rain comes in waves
        float swell = 0.5f + 0.5f * sinf(t * 0.25f) * sinf(t * 0.11f + 0.7f);
        // Subtle spatial variation — slight stereo-ish character in mono
        float spatial = 0.8f + 0.2f * sinf(t * 1.7f);
        // Loop envelope
        float env = 1.0f;
        if (lt < 0.02f) env = lt / 0.02f;
        if (lt > 0.98f) env = (1.0f - lt) / 0.02f;
        d[i] = (short)(rain * swell * spatial * env * 4000);
    }
    Sound s = LoadSoundFromWave(w); UnloadWave(w); return s;
}

void PlayRain(EVAudio *audio) {
    if (!audio->initialized || audio->rain_playing) return;
    PlaySound(audio->snd_rain);
    audio->rain_playing = true;
}
void StopRain(EVAudio *audio) {
    if (!audio->initialized) return;
    StopSound(audio->snd_rain);
    audio->rain_playing = false;
}

// Play/Stop for dream sounds
void PlayDreamRain(EVAudio *audio) {
    if (!audio->initialized || audio->dream_rain_playing) return;
    PlaySound(audio->snd_dream_rain);
    audio->dream_rain_playing = true;
}
void StopDreamRain(EVAudio *audio) {
    if (!audio->initialized) return;
    StopSound(audio->snd_dream_rain);
    audio->dream_rain_playing = false;
}
void PlayDreamTraffic(EVAudio *audio) {
    if (!audio->initialized || audio->dream_traffic_playing) return;
    PlaySound(audio->snd_dream_traffic);
    audio->dream_traffic_playing = true;
}
void StopDreamTraffic(EVAudio *audio) {
    if (!audio->initialized) return;
    StopSound(audio->snd_dream_traffic);
    audio->dream_traffic_playing = false;
}

void SetDoorSoundVolume(EVAudio *audio, int door_index, float volume) {
    if (!audio->initialized) return;
    if (door_index == 0) SetSoundVolume(audio->snd_muffled_piano, volume);
    else if (door_index == 1) SetSoundVolume(audio->snd_running_water, volume);
}

// ── Sprint 3: Hyperspace rising tone — 80Hz→400Hz over 6s ─────────
static Sound gen_hyperspace_tone(void) {
    int len = SAMPLE_RATE * 6;
    Wave w = gen_wave(len);
    short *d = (short *)w.data;
    // Proper phase accumulation for exponential frequency sweep 80Hz → 400Hz
    float phase = 0, phase_sub = 0;
    for (int i = 0; i < len; i++) {
        float t = (float)i / SAMPLE_RATE;
        float lt = (float)i / len;
        // Instantaneous frequency: exponential sweep
        float freq = 80.0f * powf(5.0f, lt);
        // Phase accumulation — correct way to sweep without artifacts
        float dp = 2.0f * PI * freq / SAMPLE_RATE;
        phase += dp;
        phase_sub += dp * 0.5f;
        float tone = sinf(phase) * 0.5f;
        tone += sinf(phase_sub) * 0.3f;  // sub-octave for body
        // Envelope: 1s fade-in, crescendo
        float env = fminf(1.0f, t / 1.0f);
        env *= 0.5f + 0.5f * lt;
        d[i] = (short)(tone * env * 5000);
    }
    Sound s = LoadSoundFromWave(w); UnloadWave(w); return s;
}

void PlayHyperspaceTone(EVAudio *audio) {
    if (!audio->initialized) return;
    if (!audio->hyperspace_tone_playing) {
        PlaySound(audio->snd_hyperspace_tone);
        audio->hyperspace_tone_playing = true;
    }
}
void StopHyperspaceTone(EVAudio *audio) {
    if (!audio->initialized) return;
    StopSound(audio->snd_hyperspace_tone);
    audio->hyperspace_tone_playing = false;
}

// ── Hyperspace riser — layered noise+harmonics+sub, 6s, overwhelming crescendo ──
static Sound gen_hyperspace_riser(void) {
    int len = SAMPLE_RATE * 6;
    Wave w = gen_wave(len);
    short *d = (short *)w.data;
    unsigned int rng = 0xDEAD;
    // Pre-compute phase accumulators for smooth frequency sweeps
    float phase_sub = 0.0f;
    float phase_h1 = 0.0f;
    float phase_h2 = 0.0f;
    float phase_h3 = 0.0f;
    for (int i = 0; i < len; i++) {
        float t = (float)i / SAMPLE_RATE;
        float lt = (float)i / len;  // 0→1 over 6 seconds
        // --- Layer 1: Sub rumble 40Hz→120Hz ---
        float sub_freq = 40.0f + lt * lt * 80.0f;  // quadratic sweep
        phase_sub += 2 * PI * sub_freq / SAMPLE_RATE;
        float sub = sinf(phase_sub) * (0.3f + lt * 0.7f);
        // --- Layer 2: Harmonic stack, octave-spaced, sweeping 80→600Hz ---
        float base_freq = 80.0f * powf(7.5f, lt);
        phase_h1 += 2 * PI * base_freq / SAMPLE_RATE;
        phase_h2 += 2 * PI * base_freq * 2.0f / SAMPLE_RATE;
        phase_h3 += 2 * PI * base_freq * 3.0f / SAMPLE_RATE;
        float harmonics = sinf(phase_h1) * 0.5f
                        + sinf(phase_h2) * 0.3f * lt  // upper harmonics fade in
                        + sinf(phase_h3) * 0.15f * lt * lt;
        // --- Layer 3: Filtered noise sweep (bandpass rising) ---
        float noise_raw = ev_randf(&rng) * 2.0f - 1.0f;
        // Simple resonant bandpass: multiply noise by swept sine
        float bp_freq = 200.0f + lt * lt * 2000.0f;
        float filtered = noise_raw * sinf(2 * PI * bp_freq * t) * lt;
        // --- Envelope: slow fade in, exponential crescendo ---
        float env = lt * lt;  // quadratic crescendo
        if (lt < 0.1f) env *= lt / 0.1f;  // anti-click fade-in
        // Cut at end — the silence after is the point
        if (lt > 0.92f) env *= (1.0f - lt) / 0.08f;
        float mix = sub * 0.4f + harmonics * 0.35f + filtered * 0.25f;
        d[i] = (short)(mix * env * 12000);
    }
    Sound s = LoadSoundFromWave(w); UnloadWave(w); return s;
}

void PlayHyperspaceRiser(EVAudio *audio) {
    if (!audio->initialized) return;
    if (!audio->hyperspace_riser_playing) {
        PlaySound(audio->snd_hyperspace_riser);
        audio->hyperspace_riser_playing = true;
    }
}
void StopHyperspaceRiser(EVAudio *audio) {
    if (!audio->initialized) return;
    StopSound(audio->snd_hyperspace_riser);
    audio->hyperspace_riser_playing = false;
}

// ── Arrival thump — deep bass impact + reverb tail, the floor meeting you ──
static Sound gen_arrival_thump(void) {
    int len = SAMPLE_RATE * 2;  // 2 seconds for reverb tail
    Wave w = gen_wave(len);
    short *d = (short *)w.data;
    unsigned int rng = 0xBEEF;
    for (int i = 0; i < len; i++) {
        float t = (float)i / SAMPLE_RATE;
        // Pitch-dropping sine: 120Hz→30Hz in 0.3s, then holds 30Hz
        float freq = (t < 0.3f) ? 120.0f - t * 300.0f : 30.0f;
        if (freq < 30.0f) freq = 30.0f;
        float body = sinf(2 * PI * freq * t);
        // Transient click — the actual impact
        float click = sinf(2 * PI * 800.0f * t) * expf(-60.0f * t);
        // Noise burst — rubble/air displacement
        float noise = (ev_randf(&rng) * 2.0f - 1.0f) * expf(-15.0f * t);
        // Envelope: instant attack, fast initial decay, slow tail
        float env;
        if (t < 0.005f) env = t / 0.005f;  // 5ms attack
        else if (t < 0.15f) env = 1.0f - (t - 0.005f) * 2.0f;  // fast decay
        else env = 0.7f * expf(-3.0f * (t - 0.15f));  // long tail
        if (env < 0) env = 0;
        float mix = body * 0.5f + click * 0.25f + noise * 0.25f;
        d[i] = (short)(mix * env * 16000);
    }
    // Simple room reverb — two tap delays
    int tap1 = (int)(SAMPLE_RATE * 0.07f);
    int tap2 = (int)(SAMPLE_RATE * 0.13f);
    for (int i = tap2; i < len; i++) {
        d[i] += (short)(d[i - tap1] * 0.15f);
        d[i] += (short)(d[i - tap2] * 0.08f);
    }
    Sound s = LoadSoundFromWave(w); UnloadWave(w); return s;
}

void PlayArrivalThump(EVAudio *audio) {
    if (!audio->initialized) return;
    PlaySound(audio->snd_arrival_thump);
}

// ── Elevator whoosh — ascending filtered noise, building anticipation ──
static Sound gen_elevator_whoosh(void) {
    int len = SAMPLE_RATE * 5;  // 5 seconds — matches elevator duration
    Wave w = gen_wave(len);
    short *d = (short *)w.data;
    unsigned int rng = 0xCAFE;
    float phase = 0.0f;
    for (int i = 0; i < len; i++) {
        float t = (float)i / SAMPLE_RATE;
        float lt = (float)i / len;
        // Wind noise — filtered through rising bandpass
        float noise = ev_randf(&rng) * 2.0f - 1.0f;
        // Sweep center frequency 100Hz→800Hz
        float center = 100.0f + lt * lt * 700.0f;
        float filtered = noise * sinf(2 * PI * center * t);
        // Subtle tonal component — ascending hum
        float hum_freq = 60.0f + lt * 200.0f;
        phase += 2 * PI * hum_freq / SAMPLE_RATE;
        float hum = sinf(phase) * 0.3f;
        // Envelope: gentle fade in, crescendo, abrupt cut
        float env = lt;  // linear crescendo
        if (lt < 0.05f) env *= lt / 0.05f;  // anti-click
        if (lt > 0.95f) env *= (1.0f - lt) / 0.05f;  // soft cut
        float mix = filtered * 0.6f + hum * 0.4f;
        // Add turbulence — random amplitude wobble
        float turb = 0.8f + 0.2f * sinf(t * 7.0f) * sinf(t * 3.0f);
        d[i] = (short)(mix * env * turb * 6000);
    }
    Sound s = LoadSoundFromWave(w); UnloadWave(w); return s;
}

void PlayElevatorWhoosh(EVAudio *audio) {
    if (!audio->initialized) return;
    if (!audio->elevator_whoosh_playing) {
        PlaySound(audio->snd_elevator_whoosh);
        audio->elevator_whoosh_playing = true;
    }
}
void StopElevatorWhoosh(EVAudio *audio) {
    if (!audio->initialized) return;
    StopSound(audio->snd_elevator_whoosh);
    audio->elevator_whoosh_playing = false;
}

// ── Door thud — heavy hotel door close, latch + resonance ──────────
static Sound gen_door_thud(void) {
    int len = SAMPLE_RATE * 1;  // 1 second
    Wave w = gen_wave(len);
    short *d = (short *)w.data;
    unsigned int rng = 0xD00D;
    for (int i = 0; i < len; i++) {
        float t = (float)i / SAMPLE_RATE;
        // Heavy body — pitch drops 80Hz→40Hz
        float body_f = 80.0f - t * 200.0f;
        if (body_f < 40.0f) body_f = 40.0f;
        float body = sinf(2 * PI * body_f * t) * expf(-6.0f * t);
        // Latch click — high transient
        float click = sinf(2 * PI * 3000.0f * t) * expf(-120.0f * t);
        // Wood panel resonance — midrange ring
        float panel = sinf(2 * PI * 350.0f * t) * expf(-8.0f * t);
        // Air noise — the seal
        float air = (ev_randf(&rng) * 2.0f - 1.0f) * expf(-25.0f * t);
        float mix = body * 0.4f + click * 0.15f + panel * 0.25f + air * 0.2f;
        // Attack envelope
        float env = (t < 0.003f) ? t / 0.003f : 1.0f;
        d[i] = (short)(mix * env * 14000);
    }
    // Room reverb
    int tap1 = (int)(SAMPLE_RATE * 0.05f);
    int tap2 = (int)(SAMPLE_RATE * 0.11f);
    for (int i = tap2; i < len; i++) {
        d[i] += (short)(d[i - tap1] * 0.12f);
        d[i] += (short)(d[i - tap2] * 0.06f);
    }
    Sound s = LoadSoundFromWave(w); UnloadWave(w); return s;
}

void PlayDoorThud(EVAudio *audio) {
    if (!audio->initialized) return;
    PlaySound(audio->snd_door_thud);
}

// ── Airlock hiss — pressurization burst, sharp filtered noise ──────
static Sound gen_airlock_hiss(void) {
    int len = SAMPLE_RATE * 1;  // 1 second
    Wave w = gen_wave(len);
    short *d = (short *)w.data;
    unsigned int rng = 0xA1AC;
    for (int i = 0; i < len; i++) {
        float t = (float)i / SAMPLE_RATE;
        float lt = (float)i / len;
        // High-frequency noise burst — bandpassed around 4kHz falling to 1kHz
        float noise = ev_randf(&rng) * 2.0f - 1.0f;
        float center = 4000.0f - lt * 3000.0f;
        float filtered = noise * sinf(2 * PI * center * t);
        // Sub-thud at the start — the seal breaking
        float thud = sinf(2 * PI * 60.0f * t) * expf(-20.0f * t);
        // Envelope: sharp attack, fast initial decay, slow hiss tail
        float env;
        if (t < 0.01f) env = t / 0.01f;
        else if (t < 0.1f) env = 1.0f;
        else env = expf(-4.0f * (t - 0.1f));
        float mix = filtered * 0.6f + thud * 0.4f;
        d[i] = (short)(mix * env * 8000);
    }
    Sound s = LoadSoundFromWave(w); UnloadWave(w); return s;
}

void PlayAirlockHiss(EVAudio *audio) {
    if (!audio->initialized) return;
    PlaySound(audio->snd_airlock_hiss);
}

// ── Gravity settle — hull creak/groan, ship acknowledging weight ───
static Sound gen_gravity_settle(void) {
    int len = SAMPLE_RATE * 2;  // 2 seconds — slow structural sound
    Wave w = gen_wave(len);
    short *d = (short *)w.data;
    unsigned int rng = 0x6AAF;
    for (int i = 0; i < len; i++) {
        float t = (float)i / SAMPLE_RATE;
        float lt = (float)i / len;
        // Low metallic groan — two detuned sines beating against each other
        float f1 = 55.0f + sinf(t * 0.5f) * 5.0f;  // wandering pitch
        float f2 = 58.0f + sinf(t * 0.7f) * 3.0f;
        float groan = sinf(2 * PI * f1 * t) * 0.5f + sinf(2 * PI * f2 * t) * 0.5f;
        // Creak — short mid-frequency chirps
        float creak_env = expf(-8.0f * fabsf(t - 0.4f));  // peaks at 0.4s
        creak_env += expf(-10.0f * fabsf(t - 1.0f)) * 0.6f;  // second creak
        float creak = sinf(2 * PI * (300.0f + sinf(t * 50.0f) * 100.0f) * t) * creak_env;
        // Noise texture — stress
        float stress = (ev_randf(&rng) * 2.0f - 1.0f) * 0.1f;
        // Envelope: fade in, sustain, fade out
        float env = 1.0f;
        if (lt < 0.15f) env = lt / 0.15f;
        if (lt > 0.7f) env = (1.0f - lt) / 0.3f;
        float mix = groan * 0.5f + creak * 0.35f + stress * 0.15f;
        d[i] = (short)(mix * env * 8000);
    }
    Sound s = LoadSoundFromWave(w); UnloadWave(w); return s;
}

void PlayGravitySettle(EVAudio *audio) {
    if (!audio->initialized) return;
    PlaySound(audio->snd_gravity_settle);
}

// ── Bed impact — soft muffled thud + fabric rustle, surrender ──────
static Sound gen_bed_impact(void) {
    int len = SAMPLE_RATE * 1;
    Wave w = gen_wave(len);
    short *d = (short *)w.data;
    unsigned int rng = 0xBED0;
    for (int i = 0; i < len; i++) {
        float t = (float)i / SAMPLE_RATE;
        // Very low thud — muffled by cushion, 40Hz only
        float thud = sinf(2 * PI * 40.0f * t) * expf(-8.0f * t);
        // Fabric rustle — filtered noise, gentle
        float rustle = (ev_randf(&rng) * 2.0f - 1.0f);
        // Bandpass around 800Hz — fabric character
        rustle *= sinf(2 * PI * 800.0f * t) * expf(-6.0f * t);
        // Springs — subtle high ring at 1200Hz
        float springs = sinf(2 * PI * 1200.0f * t) * expf(-15.0f * t) * 0.2f;
        // Soft envelope — no sharp transient, everything cushioned
        float env = (t < 0.02f) ? t / 0.02f : 1.0f;
        env *= expf(-3.0f * t);
        float mix = thud * 0.5f + rustle * 0.3f + springs * 0.2f;
        d[i] = (short)(mix * env * 10000);
    }
    Sound s = LoadSoundFromWave(w); UnloadWave(w); return s;
}

void PlayBedImpact(EVAudio *audio) {
    if (!audio->initialized) return;
    PlaySound(audio->snd_bed_impact);
}

// ── Balcony gust — one-shot wind rush, the void entering ──────────
static Sound gen_balcony_gust(void) {
    int len = SAMPLE_RATE * 2;  // 2 second gust
    Wave w = gen_wave(len);
    short *d = (short *)w.data;
    unsigned int rng = 0xC05A;
    for (int i = 0; i < len; i++) {
        float t = (float)i / SAMPLE_RATE;
        float lt = (float)i / len;
        // Broadband noise — the wind itself
        float noise = ev_randf(&rng) * 2.0f - 1.0f;
        // Swept bandpass — low growl rises to high whistle then falls
        float center = 200.0f + sinf(lt * PI) * 1500.0f;  // arcs up and back
        float filtered = noise * sinf(2 * PI * center * t);
        // Low pressure wave — felt not heard
        float pressure = sinf(2 * PI * 25.0f * t) * sinf(lt * PI);
        // Envelope: fast attack, peak at 30%, long tail
        float env;
        if (lt < 0.05f) env = lt / 0.05f;
        else if (lt < 0.3f) env = 1.0f;
        else env = expf(-2.5f * (lt - 0.3f));
        float mix = filtered * 0.6f + pressure * 0.3f + noise * 0.1f * expf(-5.0f * t);
        d[i] = (short)(mix * env * 10000);
    }
    Sound s = LoadSoundFromWave(w); UnloadWave(w); return s;
}

void PlayBalconyGust(EVAudio *audio) {
    if (!audio->initialized) return;
    PlaySound(audio->snd_balcony_gust);
}

// ── Title breath — inhale/exhale noise texture, subliminal ─────────
static Sound gen_title_breath(void) {
    int len = SAMPLE_RATE * 3;  // 3 seconds — one full breath cycle
    Wave w = gen_wave(len);
    short *d = (short *)w.data;
    unsigned int rng = 0xB4EA;
    for (int i = 0; i < len; i++) {
        float t = (float)i / SAMPLE_RATE;
        float lt = (float)i / len;
        // Noise source
        float noise = ev_randf(&rng) * 2.0f - 1.0f;
        // Breathing shape: inhale (0-40%), pause (40-50%), exhale (50-90%), silence (90-100%)
        float breath_env;
        if (lt < 0.4f) {
            // Inhale — crescendo
            float p = lt / 0.4f;
            breath_env = p * p;  // quadratic rise
        } else if (lt < 0.5f) {
            // Held — brief plateau
            breath_env = 1.0f;
        } else if (lt < 0.9f) {
            // Exhale — decrescendo
            float p = (lt - 0.5f) / 0.4f;
            breath_env = (1.0f - p) * (1.0f - p);
        } else {
            breath_env = 0.0f;
        }
        // Formant-like filtering — nasal/throat character
        // Inhale: higher center (500Hz), exhale: lower (250Hz)
        float formant = (lt < 0.5f) ? 500.0f : 250.0f;
        float filtered = noise * sinf(2 * PI * formant * t);
        // Add very subtle tonal component — body resonance
        float body = sinf(2 * PI * 100.0f * t) * 0.15f;
        float mix = filtered * 0.7f + body * 0.3f;
        d[i] = (short)(mix * breath_env * 5000);
    }
    Sound s = LoadSoundFromWave(w); UnloadWave(w); return s;
}

void PlayTitleBreath(EVAudio *audio) {
    if (!audio->initialized) return;
    PlaySound(audio->snd_title_breath);
}

// ── Hard cut punch — micro-transient, 10ms click + sub thump ───────
static Sound gen_hard_cut_punch(void) {
    int len = SAMPLE_RATE / 4;  // 250ms — short and physical
    Wave w = gen_wave(len);
    short *d = (short *)w.data;
    for (int i = 0; i < len; i++) {
        float t = (float)i / SAMPLE_RATE;
        // Sub thump — 50Hz, instant attack, fast decay
        float sub = sinf(2 * PI * 50.0f * t) * expf(-25.0f * t);
        // Click transient — 1kHz spike, 5ms
        float click = sinf(2 * PI * 1000.0f * t) * expf(-200.0f * t);
        // Upper crack — 3kHz, even shorter
        float crack = sinf(2 * PI * 3000.0f * t) * expf(-400.0f * t);
        float mix = sub * 0.5f + click * 0.3f + crack * 0.2f;
        // Hard envelope — instant on, fast out
        float env = (t < 0.001f) ? t / 0.001f : 1.0f;
        d[i] = (short)(mix * env * 16000);
    }
    Sound s = LoadSoundFromWave(w); UnloadWave(w); return s;
}

void PlayHardCutPunch(EVAudio *audio) {
    if (!audio->initialized) return;
    PlaySound(audio->snd_hard_cut_punch);
}

// ── Phone ring — vintage two-bell, muffled through walls ────────────
// One double-ring, then silence. Unanswered. Someone calling a room
// that's booked for two but occupied by one.
static Sound gen_phone_ring(void) {
    int len = SAMPLE_RATE * 4;  // 4 seconds — one ring cycle, then decay
    Wave w = gen_wave(len);
    short *d = (short *)w.data;

    // Two-bell pattern: ring-ring, pause, ring-ring
    // Each "ring" is 0.4s of dual-frequency bell
    float ring_starts[] = {0.2f, 0.7f, 2.0f, 2.5f};
    float ring_dur = 0.35f;

    int reverb_delay = (int)(SAMPLE_RATE * 0.12f);

    for (int i = 0; i < len; i++) {
        float t = (float)i / SAMPLE_RATE;
        float sample = 0;

        for (int r = 0; r < 4; r++) {
            float rt = t - ring_starts[r];
            if (rt >= 0 && rt < ring_dur) {
                float renv = sinf(PI * rt / ring_dur);  // smooth bell shape
                renv *= renv;
                // Two bell frequencies — slightly dissonant, vintage
                float bell1 = sinf(2 * PI * 440.0f * t) * 0.3f;
                float bell2 = sinf(2 * PI * 480.0f * t) * 0.25f;
                // Muffled — through a wall, through time
                float muffle = expf(-2.0f * rt);
                sample += (bell1 + bell2) * renv * muffle;
            }
        }

        // Overall fade — the ring gets quieter, giving up
        float fade = 1.0f;
        if (t > 3.0f) fade = (4.0f - t);
        if (fade < 0) fade = 0;

        d[i] = (short)(sample * fade * 5000);
    }
    // Reverb — room resonance
    for (int i = reverb_delay; i < len; i++)
        d[i] += (short)(d[i - reverb_delay] * 0.2f);

    Sound s = LoadSoundFromWave(w); UnloadWave(w); return s;
}

void PlayPhoneRing(EVAudio *audio) {
    if (!audio->initialized) return;
    PlaySound(audio->snd_phone_ring);
}

// ── Earth presence — 30Hz sub-bass, gravitational hum near observation window ──
static Sound gen_earth_presence(void) {
    int len = SAMPLE_RATE * 10;  // 10-second loop
    Wave w = gen_wave(len);
    short *d = (short *)w.data;
    for (int i = 0; i < len; i++) {
        float t = (float)i / SAMPLE_RATE;
        float lt = (float)i / len;
        // Deep sub-bass — 30Hz fundamental
        float sub = sinf(2 * PI * 30.0f * t);
        // Second harmonic at 60Hz for body
        float body = sinf(2 * PI * 60.0f * t) * 0.3f;
        // Very slow amplitude modulation — breathing
        float breath = 0.7f + 0.3f * sinf(2 * PI * t / 4.0f);
        // Loop crossfade
        float env = 1.0f;
        if (lt < 0.03f) env = lt / 0.03f;
        if (lt > 0.97f) env = (1.0f - lt) / 0.03f;
        float mix = (sub * 0.7f + body * 0.3f) * breath;
        d[i] = (short)(mix * env * 8000);
    }
    Sound s = LoadSoundFromWave(w); UnloadWave(w); return s;
}

void PlayEarthPresence(EVAudio *audio) {
    if (!audio->initialized) return;
    if (!audio->earth_presence_playing) {
        PlaySound(audio->snd_earth_presence);
        audio->earth_presence_playing = true;
    }
}
void StopEarthPresence(EVAudio *audio) {
    if (!audio->initialized) return;
    StopSound(audio->snd_earth_presence);
    audio->earth_presence_playing = false;
}
void SetEarthPresenceVolume(EVAudio *audio, float vol) {
    if (!audio->initialized) return;
    SetSoundVolume(audio->snd_earth_presence, vol);
}

// ============================================================
// FILE-BASED MUSIC — Maxwell's compositions
// Loaded from assets/audio/. Streamed, not buffered.
// Each piece plays once, never loops, never repeats.
// ============================================================

void LoadFileMusic(EVAudio *audio) {
    audio->music_suite = LoadMusicStream("assets/audio/lighthouse.wav");
    audio->music_balcony = LoadMusicStream("assets/audio/ambient4.wav");
    audio->music_corridor = LoadMusicStream("assets/audio/ambient1_icloud.wav");
    audio->music_title = LoadMusicStream("assets/audio/ambient3.wav");
    // No looping — each piece plays exactly once
    audio->music_suite.looping = false;
    audio->music_balcony.looping = false;
    audio->music_corridor.looping = false;
    audio->music_title.looping = true;  // title loops until player presses enter
    audio->suite_music_playing = false;
    audio->balcony_music_playing = false;
    audio->corridor_music_playing = false;
    audio->title_music_playing = false;
    audio->music_loaded = true;
}

void UnloadFileMusic(EVAudio *audio) {
    if (!audio->music_loaded) return;
    UnloadMusicStream(audio->music_suite);
    UnloadMusicStream(audio->music_balcony);
    UnloadMusicStream(audio->music_corridor);
    UnloadMusicStream(audio->music_title);
    audio->music_loaded = false;
}

void UpdateFileMusic(EVAudio *audio) {
    if (!audio->music_loaded) return;
    if (audio->suite_music_playing) UpdateMusicStream(audio->music_suite);
    if (audio->balcony_music_playing) UpdateMusicStream(audio->music_balcony);
    if (audio->corridor_music_playing) UpdateMusicStream(audio->music_corridor);
    if (audio->title_music_playing) UpdateMusicStream(audio->music_title);
}

void PlaySuiteMusic(EVAudio *audio) {
    if (!audio->music_loaded || audio->suite_music_playing) return;
    SetMusicVolume(audio->music_suite, 0.35f);  // sits under procedural audio
    PlayMusicStream(audio->music_suite);
    audio->suite_music_playing = true;
}
void StopSuiteMusic(EVAudio *audio) {
    if (!audio->music_loaded) return;
    StopMusicStream(audio->music_suite);
    audio->suite_music_playing = false;
}

void PlayBalconyMusic(EVAudio *audio) {
    if (!audio->music_loaded || audio->balcony_music_playing) return;
    SetMusicVolume(audio->music_balcony, 0.3f);
    PlayMusicStream(audio->music_balcony);
    audio->balcony_music_playing = true;
}
void StopBalconyMusic(EVAudio *audio) {
    if (!audio->music_loaded) return;
    StopMusicStream(audio->music_balcony);
    audio->balcony_music_playing = false;
}

void PlayCorridorMusic(EVAudio *audio) {
    if (!audio->music_loaded || audio->corridor_music_playing) return;
    SetMusicVolume(audio->music_corridor, 0.15f);  // underneath, not dominant
    PlayMusicStream(audio->music_corridor);
    audio->corridor_music_playing = true;
}
void StopCorridorMusic(EVAudio *audio) {
    if (!audio->music_loaded) return;
    StopMusicStream(audio->music_corridor);
    audio->corridor_music_playing = false;
}

void PlayTitleMusic(EVAudio *audio) {
    if (!audio->music_loaded || audio->title_music_playing) return;
    SetMusicVolume(audio->music_title, 0.2f);  // atmospheric, not foreground
    PlayMusicStream(audio->music_title);
    audio->title_music_playing = true;
}
void StopTitleMusic(EVAudio *audio) {
    if (!audio->music_loaded) return;
    StopMusicStream(audio->music_title);
    audio->title_music_playing = false;
}

void SetFileMusicVolume(EVAudio *audio, float vol) {
    if (!audio->music_loaded) return;
    if (audio->suite_music_playing) SetMusicVolume(audio->music_suite, vol * 0.35f);
    if (audio->balcony_music_playing) SetMusicVolume(audio->music_balcony, vol * 0.3f);
    if (audio->corridor_music_playing) SetMusicVolume(audio->music_corridor, vol * 0.15f);
    if (audio->title_music_playing) SetMusicVolume(audio->music_title, vol * 0.2f);
}

void StopAllAudio(EVAudio *audio) {
    StopAmbient(audio);
    StopCityAmbient(audio);
    StopClockAmbient(audio);
    StopWindAmbient(audio);
    StopMuffledPiano(audio);
    StopDistantVoices(audio);
    StopFootstepsAbove(audio);
    StopBedDrone(audio);
    StopHeldChord(audio);
    StopBedRitual(audio);
    StopTaxiRadio(audio);
    StopHyperspaceTone(audio);
    StopHyperspaceRiser(audio);
    StopElevatorWhoosh(audio);
    StopEarthPresence(audio);
    StopRain(audio);
    StopDreamRain(audio);
    StopDreamTraffic(audio);
    StopSuiteMusic(audio);
    StopBalconyMusic(audio);
    StopCorridorMusic(audio);
    StopTitleMusic(audio);
}

// ── Impact sounds — physics collisions ──
// All procedurally synthesized: no audio files.
void PlayImpactSound(EVAudio *audio, float intensity, ImpactType type) {
    if (!audio->initialized) return;
    intensity = fminf(1.0f, fmaxf(0.05f, intensity));

    int samples = (int)(SAMPLE_RATE * 0.15f);  // 150ms
    Wave w = gen_wave(samples);
    short *data = (short *)w.data;
    unsigned int seed = 42 + (unsigned int)(intensity * 1000);

    switch (type) {
        case IMPACT_HARD: {
            // Low thud — exponential decay noise + sub-bass
            float freq = 60.0f + intensity * 30.0f;
            for (int i = 0; i < samples; i++) {
                float t = (float)i / SAMPLE_RATE;
                float env = expf(-t * 20.0f);
                float bass = sinf(2*PI * freq * t) * env;
                float noise = (ev_randf(&seed) - 0.5f) * env * 0.3f;
                data[i] = (short)((bass + noise) * 16000 * intensity);
            }
            break;
        }
        case IMPACT_SOFT: {
            // Muffled thud — low-passed noise
            float prev = 0;
            for (int i = 0; i < samples; i++) {
                float t = (float)i / SAMPLE_RATE;
                float env = expf(-t * 25.0f);
                float raw = (ev_randf(&seed) - 0.5f) * env;
                prev = prev * 0.85f + raw * 0.15f;  // simple low-pass
                data[i] = (short)(prev * 12000 * intensity);
            }
            break;
        }
        case IMPACT_GLASS: {
            // High clink — metallic ring + noise burst
            float freq = 2200.0f + intensity * 800.0f;
            float freq2 = freq * 1.47f;
            for (int i = 0; i < samples; i++) {
                float t = (float)i / SAMPLE_RATE;
                float ring_env = expf(-t * 12.0f);
                float noise_env = expf(-t * 40.0f);
                float ring = sinf(2*PI * freq * t) * 0.4f + sinf(2*PI * freq2 * t) * 0.3f;
                float noise = (ev_randf(&seed) - 0.5f) * noise_env * 0.3f;
                data[i] = (short)((ring * ring_env + noise) * 14000 * intensity);
            }
            break;
        }
    }

    Sound snd = LoadSoundFromWave(w);
    UnloadWave(w);
    SetSoundVolume(snd, 0.4f * intensity);
    PlaySound(snd);
    queue_transient_sound(audio, snd, 0.35f);
}

// ── Bed ritual music — placeholder warm chord progression ──
