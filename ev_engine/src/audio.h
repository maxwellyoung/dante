// audio.h — Procedural audio engine
#ifndef EV_AUDIO_H
#define EV_AUDIO_H

#include "ev_types.h"
#include <stdbool.h>

typedef enum {
    DRONE_LOBBY,
    DRONE_HALLWAY,
    DRONE_ROOM,
    DRONE_SPACE_LOBBY,
    DRONE_SPACE_CORRIDOR,
    DRONE_SPACE_SUITE,
} DroneType;

typedef enum {
    INTERACT_CLICK,
    INTERACT_FABRIC,
    INTERACT_FLAME,
    INTERACT_CORK_POP,
    INTERACT_GLASS_CLINK,
} InteractSoundType;

typedef struct {
    Sound step_marble[4];
    Sound step_carpet[4];
    Sound step_wood[4];

    Sound snd_click;
    Sound snd_fabric;
    Sound snd_flame;
    Sound snd_cork_pop;
    Sound snd_glass_clink;
    Sound snd_reward;    // warm chime — task complete, the room opens up
    Sound snd_sparkle;   // Eiffel Tower sparkle

    Sound door;
    Sound elevator_hum;
    Sound elevator_ding;

    Sound drone_lobby;
    Sound drone_hallway;
    Sound drone_room;
    Sound drone_space_lobby;
    Sound drone_space_corridor;
    Sound drone_space_suite;
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

    // Through-wall sounds — inaccessible spaces communicate
    Sound snd_muffled_piano;     // someone else's room — piano through wall
    Sound snd_distant_voices;    // conversation you'll never hear clearly
    Sound snd_footsteps_above;   // someone walking upstairs
    bool muffled_piano_playing;
    bool distant_voices_playing;
    bool footsteps_above_playing;

    // Sprint 1: Bed drone + held chord for ending
    Sound snd_bed_drone;         // low ~50Hz, 20s loop — surrender
    Sound snd_held_chord;        // C3-G3-D4 stacked fifths, 3s attack, infinite sustain
    bool bed_drone_playing;
    bool held_chord_playing;

    // Sprint 2: Per-door spatial sounds (corridor)
    Sound snd_running_water;     // filtered noise — bathroom behind door
    Sound snd_tv_murmur;         // modulated noise bursts — TV behind door

    // Sprint 3: Hyperspace rising tone
    Sound snd_hyperspace_tone;   // 80Hz→400Hz glissando over 6s
    bool hyperspace_tone_playing;

    // Transit sounds — risers, impacts, whooshes
    Sound snd_hyperspace_riser;  // layered noise+harmonic riser, 6s, overwhelming crescendo
    Sound snd_arrival_thump;     // deep bass impact + reverb tail — landing
    Sound snd_elevator_whoosh;   // ascending filtered noise — elevator ascent
    Sound snd_door_thud;         // heavy door close — latch click + room resonance
    Sound snd_airlock_hiss;      // pressurization burst — entering space scenes
    Sound snd_gravity_settle;    // hull creak/groan — ship acknowledging weight
    Sound snd_bed_impact;        // soft muffled thud + fabric — surrender
    Sound snd_balcony_gust;      // one-shot wind rush — the void entering
    Sound snd_title_breath;      // inhale/exhale texture — before title text
    Sound snd_hard_cut_punch;    // micro-transient — Blendo-style instant cuts
    Sound snd_earth_presence;    // 30Hz sub-bass — Earth's gravitational hum, near window only
    bool hyperspace_riser_playing;
    bool elevator_whoosh_playing;
    bool earth_presence_playing;

    // Clock rate — for pitch modulation
    float clock_rate;            // 1.0 = normal, 0.0 = stopped

    // File-based music — Maxwell's compositions, played once
    Music music_suite;           // "lighthouse" — the one composed piece, triggered once
    Music music_balcony;         // "unsaid" — melancholy, the void outside
    Music music_corridor;        // "stt" — the long walk
    Music music_title;           // "ambient1" — title screen atmosphere
    bool suite_music_playing;
    bool balcony_music_playing;
    bool corridor_music_playing;
    bool title_music_playing;
    bool music_loaded;

    float step_timer;
    float step_interval;
    int step_index;

    // Ducking — brief ambient reduction on interaction
    float duck_timer;        // >0 = ducking active, counts down
    float duck_amount;       // 0-1, how much to reduce ambient (0.3 = 30% reduction)

    // Drone crossfade — smooth transition between ambient drones
    DroneType next_drone;    // target drone for crossfade
    float crossfade_timer;   // >0 = crossfading, counts down
    float crossfade_duration;

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

// Through-wall sounds — inaccessible spaces communicate
void PlayMuffledPiano(EVAudio *audio);
void StopMuffledPiano(EVAudio *audio);
void PlayDistantVoices(EVAudio *audio);
void StopDistantVoices(EVAudio *audio);
void PlayFootstepsAbove(EVAudio *audio);
void StopFootstepsAbove(EVAudio *audio);

// Sprint 1: Ending sounds
void PlayBedDrone(EVAudio *audio);
void StopBedDrone(EVAudio *audio);
void PlayHeldChord(EVAudio *audio);
void StopHeldChord(EVAudio *audio);

// Sprint 2: Clock rate modulation
void SetClockRate(EVAudio *audio, float rate);

// Sprint 2: Per-door spatial sounds
void SetDoorSoundVolume(EVAudio *audio, int door_index, float volume);

// Sprint 3: Hyperspace tone
void PlayHyperspaceTone(EVAudio *audio);
void StopHyperspaceTone(EVAudio *audio);

// Transit sounds — risers, impacts, whooshes
void PlayHyperspaceRiser(EVAudio *audio);
void StopHyperspaceRiser(EVAudio *audio);
void PlayArrivalThump(EVAudio *audio);
void PlayElevatorWhoosh(EVAudio *audio);
void StopElevatorWhoosh(EVAudio *audio);
void PlayDoorThud(EVAudio *audio);
void PlayAirlockHiss(EVAudio *audio);
void PlayGravitySettle(EVAudio *audio);
void PlayBedImpact(EVAudio *audio);
void PlayBalconyGust(EVAudio *audio);
void PlayTitleBreath(EVAudio *audio);
void PlayHardCutPunch(EVAudio *audio);

// Earth presence — sub-bass drone near observation window
void PlayEarthPresence(EVAudio *audio);
void StopEarthPresence(EVAudio *audio);
void SetEarthPresenceVolume(EVAudio *audio, float vol);

// File-based music — Maxwell's compositions
void LoadFileMusic(EVAudio *audio);
void UnloadFileMusic(EVAudio *audio);
void UpdateFileMusic(EVAudio *audio);
void PlaySuiteMusic(EVAudio *audio);
void StopSuiteMusic(EVAudio *audio);
void PlayBalconyMusic(EVAudio *audio);
void StopBalconyMusic(EVAudio *audio);
void PlayCorridorMusic(EVAudio *audio);
void StopCorridorMusic(EVAudio *audio);
void PlayTitleMusic(EVAudio *audio);
void StopTitleMusic(EVAudio *audio);
void SetFileMusicVolume(EVAudio *audio, float vol);

#endif
