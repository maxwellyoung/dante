// audio.c — Procedural audio engine
// All sounds generated from code — no external files
#include "audio.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define SAMPLE_RATE 44100
#define PI 3.14159265358979f

static Wave gen_wave(int samples) {
    Wave w = {0};
    w.frameCount = samples;
    w.sampleRate = SAMPLE_RATE;
    w.sampleSize = 16;
    w.channels = 1;
    w.data = calloc(samples, sizeof(short));
    return w;
}

// Generate a footstep: short filtered noise burst with pitch variation
static Sound gen_footstep(int seed) {
    int len = SAMPLE_RATE / 12;  // ~83ms
    Wave w = gen_wave(len);
    short *d = (short *)w.data;

    srand(seed);
    float pitch = 0.8f + (rand() % 40) / 100.0f;  // 0.8-1.2 pitch variation

    for (int i = 0; i < len; i++) {
        float t = (float)i / len;
        // Envelope: sharp attack, quick decay
        float env = (t < 0.05f) ? t / 0.05f : expf(-8.0f * (t - 0.05f));
        // Filtered noise + low thud
        float noise = ((rand() % 32768) / 16384.0f - 1.0f);
        float thud = sinf(2 * PI * 60 * pitch * t) * expf(-15.0f * t);
        float sample = (noise * 0.3f + thud * 0.7f) * env;
        d[i] = (short)(sample * 12000);
    }

    Sound s = LoadSoundFromWave(w);
    UnloadWave(w);
    return s;
}

// Generate ambient drone: layered sine waves with slow modulation
static Sound gen_ambient_drone(void) {
    int len = SAMPLE_RATE * 8;  // 8 second loop
    Wave w = gen_wave(len);
    short *d = (short *)w.data;

    // Chord frequencies — Debussy-inspired suspended voicings
    float freqs[] = {55.0f, 82.5f, 110.0f, 146.83f, 220.0f};
    float amps[] = {0.3f, 0.2f, 0.25f, 0.15f, 0.1f};
    int nfreqs = 5;

    for (int i = 0; i < len; i++) {
        float t = (float)i / SAMPLE_RATE;
        float sample = 0;
        for (int f = 0; f < nfreqs; f++) {
            // Slow vibrato per voice
            float vibrato = 1.0f + 0.003f * sinf(2 * PI * (0.1f + f * 0.07f) * t);
            sample += amps[f] * sinf(2 * PI * freqs[f] * vibrato * t);
        }
        // Gentle amplitude modulation for movement
        float mod = 0.7f + 0.3f * sinf(2 * PI * 0.08f * t);
        sample *= mod;

        // Crossfade loop end into start (last 0.5s)
        float loop_t = (float)i / len;
        if (loop_t > 0.9375f) {
            float fade = (1.0f - loop_t) / 0.0625f;
            sample *= fade;
        }
        if (loop_t < 0.0625f) {
            sample *= loop_t / 0.0625f;
        }

        d[i] = (short)(sample * 8000);
    }

    Sound s = LoadSoundFromWave(w);
    UnloadWave(w);
    return s;
}

// Generate interaction chime: soft bell-like tone
static Sound gen_interact_sound(void) {
    int len = SAMPLE_RATE / 2;  // 0.5s
    Wave w = gen_wave(len);
    short *d = (short *)w.data;

    for (int i = 0; i < len; i++) {
        float t = (float)i / SAMPLE_RATE;
        float env = expf(-4.0f * t);
        // Two harmonics for bell-like quality
        float sample = 0.6f * sinf(2 * PI * 880 * t) * env +
                       0.3f * sinf(2 * PI * 1320 * t) * expf(-6.0f * t) +
                       0.1f * sinf(2 * PI * 2640 * t) * expf(-10.0f * t);
        d[i] = (short)(sample * 10000);
    }

    Sound s = LoadSoundFromWave(w);
    UnloadWave(w);
    return s;
}

// Generate door/transition sound: low pitch sweep
static Sound gen_door_sound(void) {
    int len = SAMPLE_RATE * 3 / 4;  // 0.75s
    Wave w = gen_wave(len);
    short *d = (short *)w.data;

    for (int i = 0; i < len; i++) {
        float t = (float)i / SAMPLE_RATE;
        float env = (t < 0.1f) ? t / 0.1f : expf(-3.0f * (t - 0.1f));
        // Descending pitch sweep
        float freq = 200.0f - 120.0f * t;
        float sample = sinf(2 * PI * freq * t) * 0.5f +
                       sinf(2 * PI * freq * 0.5f * t) * 0.3f;
        // Add some noise for texture
        float noise = ((float)(rand() % 32768) / 16384.0f - 1.0f) * 0.05f;
        sample = (sample + noise) * env;
        d[i] = (short)(sample * 10000);
    }

    Sound s = LoadSoundFromWave(w);
    UnloadWave(w);
    return s;
}

void InitEVAudio(EVAudio *audio) {
    memset(audio, 0, sizeof(EVAudio));
    InitAudioDevice();

    for (int i = 0; i < 4; i++) {
        audio->footstep[i] = gen_footstep(42 + i * 7);
    }
    audio->interact = gen_interact_sound();
    audio->door = gen_door_sound();
    audio->ambient_drone = gen_ambient_drone();

    audio->step_timer = 0;
    audio->step_interval = 0.5f;
    audio->step_index = 0;
    audio->ambient_playing = false;
    audio->initialized = true;

    // Set volumes
    for (int i = 0; i < 4; i++) SetSoundVolume(audio->footstep[i], 0.35f);
    SetSoundVolume(audio->interact, 0.5f);
    SetSoundVolume(audio->door, 0.4f);
    SetSoundVolume(audio->ambient_drone, 0.15f);
}

void UnloadEVAudio(EVAudio *audio) {
    if (!audio->initialized) return;
    for (int i = 0; i < 4; i++) UnloadSound(audio->footstep[i]);
    UnloadSound(audio->interact);
    UnloadSound(audio->door);
    UnloadSound(audio->ambient_drone);
    CloseAudioDevice();
    audio->initialized = false;
}

void UpdateEVAudio(EVAudio *audio, bool moving, bool sprinting, float dt) {
    if (!audio->initialized) return;

    // Loop ambient drone
    if (audio->ambient_playing && !IsSoundPlaying(audio->ambient_drone)) {
        PlaySound(audio->ambient_drone);
    }

    // Footsteps
    if (moving) {
        audio->step_interval = sprinting ? 0.3f : 0.5f;
        audio->step_timer += dt;
        if (audio->step_timer >= audio->step_interval) {
            audio->step_timer = 0;
            PlaySound(audio->footstep[audio->step_index]);
            audio->step_index = (audio->step_index + 1) % 4;
        }
    } else {
        audio->step_timer = audio->step_interval * 0.8f;  // Quick first step on resume
    }
}

void PlayInteractSound(EVAudio *audio) {
    if (audio->initialized) PlaySound(audio->interact);
}

void PlayDoorSound(EVAudio *audio) {
    if (audio->initialized) PlaySound(audio->door);
}

void StartAmbient(EVAudio *audio) {
    if (!audio->initialized) return;
    if (!audio->ambient_playing) {
        PlaySound(audio->ambient_drone);
        audio->ambient_playing = true;
    }
}

void StopAmbient(EVAudio *audio) {
    if (!audio->initialized) return;
    if (audio->ambient_playing) {
        StopSound(audio->ambient_drone);
        audio->ambient_playing = false;
    }
}
