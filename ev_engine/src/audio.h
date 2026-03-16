// audio.h — Procedural audio engine
#ifndef EV_AUDIO_H
#define EV_AUDIO_H

#include "ev_types.h"
#include <stdbool.h>

typedef enum {
    DRONE_LOBBY,
    DRONE_HALLWAY,
    DRONE_ROOM,
} DroneType;

typedef enum {
    INTERACT_CLICK,
    INTERACT_FABRIC,
    INTERACT_FLAME,
} InteractSoundType;

typedef struct {
    Sound step_marble[4];
    Sound step_carpet[4];
    Sound step_wood[4];

    Sound snd_click;
    Sound snd_fabric;
    Sound snd_flame;
    Sound snd_reward;    // warm chime — task complete, the room opens up
    Sound snd_sparkle;   // Eiffel Tower sparkle

    Sound door;

    Sound drone_lobby;
    Sound drone_hallway;
    Sound drone_room;
    DroneType current_drone;
    bool ambient_playing;

    float step_timer;
    float step_interval;
    int step_index;
    bool initialized;
} EVAudio;

void InitEVAudio(EVAudio *audio);
void UnloadEVAudio(EVAudio *audio);
void UpdateEVAudio(EVAudio *audio, bool moving, bool sprinting, SurfaceType surface, float dt);
void PlayInteract(EVAudio *audio, InteractSoundType type);
void PlayRewardSound(EVAudio *audio);
void PlaySparkleSound(EVAudio *audio);
void PlayDoorSound(EVAudio *audio);
void StartAmbient(EVAudio *audio, DroneType type);
void StopAmbient(EVAudio *audio);

#endif
