// audio.h — Procedural audio engine
#ifndef EV_AUDIO_H
#define EV_AUDIO_H

#include "raylib.h"
#include <stdbool.h>

typedef struct {
    Sound footstep[4];
    Sound interact;
    Sound door;
    Sound ambient_drone;
    bool ambient_playing;
    float step_timer;
    float step_interval;
    int step_index;
    bool initialized;
} EVAudio;

void InitEVAudio(EVAudio *audio);
void UnloadEVAudio(EVAudio *audio);
void UpdateEVAudio(EVAudio *audio, bool moving, bool sprinting, float dt);
void PlayInteractSound(EVAudio *audio);
void PlayDoorSound(EVAudio *audio);
void StartAmbient(EVAudio *audio);
void StopAmbient(EVAudio *audio);

#endif
