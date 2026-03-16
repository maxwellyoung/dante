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

// --- FOOTSTEPS ---

static Sound gen_step_marble(int seed) {
    int len = SAMPLE_RATE / 10;
    Wave w = gen_wave(len);
    short *d = (short *)w.data;
    srand(seed);
    float pitch = 0.9f + (rand() % 20) / 100.0f;
    for (int i = 0; i < len; i++) {
        float t = (float)i / len;
        float env = (t < 0.02f) ? t / 0.02f : expf(-12.0f * (t - 0.02f));
        float click = sinf(2 * PI * 2200 * pitch * t) * expf(-30.0f * t);
        float body = sinf(2 * PI * 180 * pitch * t) * expf(-10.0f * t);
        float noise = ((rand() % 32768) / 16384.0f - 1.0f) * expf(-20.0f * t);
        d[i] = (short)((click * 0.3f + body * 0.4f + noise * 0.3f) * env * 10000);
    }
    Sound s = LoadSoundFromWave(w); UnloadWave(w); return s;
}

static Sound gen_step_carpet(int seed) {
    int len = SAMPLE_RATE / 8;
    Wave w = gen_wave(len);
    short *d = (short *)w.data;
    srand(seed);
    float pitch = 0.85f + (rand() % 30) / 100.0f;
    for (int i = 0; i < len; i++) {
        float t = (float)i / len;
        float env = (t < 0.08f) ? t / 0.08f : expf(-6.0f * (t - 0.08f));
        float thud = sinf(2 * PI * 45 * pitch * t) * expf(-8.0f * t);
        float noise = ((rand() % 32768) / 16384.0f - 1.0f);
        float lp = noise * 0.15f + thud * 0.85f;
        d[i] = (short)(lp * env * 7000);
    }
    Sound s = LoadSoundFromWave(w); UnloadWave(w); return s;
}

static Sound gen_step_wood(int seed) {
    int len = SAMPLE_RATE / 10;
    Wave w = gen_wave(len);
    short *d = (short *)w.data;
    srand(seed);
    float pitch = 0.85f + (rand() % 30) / 100.0f;
    for (int i = 0; i < len; i++) {
        float t = (float)i / len;
        float env = (t < 0.03f) ? t / 0.03f : expf(-10.0f * (t - 0.03f));
        float tap = sinf(2 * PI * 400 * pitch * t) * expf(-20.0f * t);
        float body = sinf(2 * PI * 80 * pitch * t) * expf(-8.0f * t);
        d[i] = (short)((tap * 0.5f + body * 0.5f) * env * 9000);
    }
    Sound s = LoadSoundFromWave(w); UnloadWave(w); return s;
}

// --- AMBIENT — NOT DRONES. Musical, pleasant, short loops ---

// Lobby: same melody as room but one octave higher, slower decay (reverb-like)
// Heard through walls — distant, ethereal
static Sound gen_ambient_lobby(void) {
    float beat = 0.5f;
    float loop_len = 16.0f * beat * 2;
    int len = (int)(SAMPLE_RATE * loop_len);
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
            // Slower decay — more reverb-like, heard through walls
            float attack = (nt < 0.05f) ? nt / 0.05f : 1.0f;
            float env = attack * expf(-0.8f * nt);
            float tone = sinf(2 * PI * freq * t) +
                         0.15f * sinf(2 * PI * freq * 2.0f * t);
            sample += tone * env;
        }
        float lt = (float)i / len;
        if (lt > 0.95f) sample *= (1.0f - lt) / 0.05f;
        if (lt < 0.02f) sample *= lt / 0.02f;
        d[i] = (short)(sample * 2000);
    }
    Sound s = LoadSoundFromWave(w); UnloadWave(w); return s;
}

// Hallway: just root notes as sustained tones — building anticipation
// E3, A2, E3, D3 — the skeleton of the melody, stripped bare
static Sound gen_ambient_hallway(void) {
    float beat = 0.5f;
    float loop_len = 16.0f * beat * 2;
    int len = (int)(SAMPLE_RATE * loop_len);
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
            // Long sustain, gentle fade at end of each note
            float attack = (nt < 0.1f) ? nt / 0.1f : 1.0f;
            float release = (nt > dur - 0.5f) ? (dur - nt) / 0.5f : 1.0f;
            float env = attack * release;
            float tone = sinf(2 * PI * freq * t) +
                         0.15f * sinf(2 * PI * freq * 2.0f * t);
            sample += tone * env;
        }
        float lt = (float)i / len;
        if (lt > 0.95f) sample *= (1.0f - lt) / 0.05f;
        if (lt < 0.02f) sample *= lt / 0.02f;
        d[i] = (short)(sample * 2500);
    }
    Sound s = LoadSoundFromWave(w); UnloadWave(w); return s;
}

// Room: composed piano melody — Satie-inspired descending phrase
// 8 bars at ~120 BPM (0.5s per beat), loops every 16 seconds
static Sound gen_ambient_room(void) {
    float beat = 0.5f;  // 120 BPM
    float loop_len = 16.0f * beat * 2;  // 8 bars of 4 beats = 16 seconds
    int len = (int)(SAMPLE_RATE * loop_len);
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

    for (int i = 0; i < len; i++) {
        float t = (float)i / SAMPLE_RATE;
        float sample = 0;
        for (int n = 0; n < note_count; n++) {
            float freq = melody[n][0];
            float start = melody[n][1] * beat;
            float dur = melody[n][2] * beat;
            float nt = t - start;
            if (nt < 0 || nt > dur + 1.5f) continue;  // note + tail
            // Gentle attack (50ms ramp) + exponential decay
            float attack = (nt < 0.05f) ? nt / 0.05f : 1.0f;
            float env = attack * expf(-1.5f * nt);
            // Fundamental + soft 2nd harmonic for warmth
            float tone = sinf(2 * PI * freq * t) +
                         0.15f * sinf(2 * PI * freq * 2.0f * t);
            sample += tone * env;
        }
        // Loop crossfade
        float lt = (float)i / len;
        if (lt > 0.95f) sample *= (1.0f - lt) / 0.05f;
        if (lt < 0.02f) sample *= lt / 0.02f;
        d[i] = (short)(sample * 3000);
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
        float env = (t < 0.05f) ? t/0.05f : expf(-4.0f*(t-0.05f));
        // Rising pitch — arrival, not descent (Remo)
        d[i] = (short)(sinf(2*PI*(150+120*t)*t)*env * 8000);
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
    audio->drone_lobby = gen_ambient_lobby();
    audio->drone_hallway = gen_ambient_hallway();
    audio->drone_room = gen_ambient_room();
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
    SetSoundVolume(audio->drone_lobby, 0.06f);    // barely there — through walls
    SetSoundVolume(audio->drone_hallway, 0.05f);  // anticipation, not presence
    SetSoundVolume(audio->drone_room, 0.10f);     // the room's own music
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
    UnloadSound(audio->drone_lobby); UnloadSound(audio->drone_hallway);
    UnloadSound(audio->drone_room);
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
