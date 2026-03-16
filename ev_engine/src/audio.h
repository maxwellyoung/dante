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
    Sound elevator_hum;
    Sound elevator_ding;

    Sound drone_lobby;
    Sound drone_hallway;
    Sound drone_room;
    DroneType current_drone;
    bool ambient_playing;

    Sound snd_city;
    Sound snd_clock;
    Sound snd_stairwell;
    Sound snd_wind;
    bool city_playing;
    bool clock_playing;
    bool stairwell_playing;
    bool wind_playing;

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
void PlayCityAmbient(EVAudio *audio);
void StopCityAmbient(EVAudio *audio);
void PlayClockAmbient(EVAudio *audio);
void StopClockAmbient(EVAudio *audio);
void PlayStairwellAmbient(EVAudio *audio);
void StopStairwellAmbient(EVAudio *audio);
void PlayWindAmbient(EVAudio *audio);
void StopWindAmbient(EVAudio *audio);
void PlayElevatorHum(EVAudio *audio);
void PlayElevatorDing(EVAudio *audio);
void SetCityAmbientVolume(EVAudio *audio, float vol);

#endif
