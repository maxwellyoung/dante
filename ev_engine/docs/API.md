# Endearing Void Engine — API Reference

_Auto-generated on 2026-03-17 10:33_

---

## Modules

### `audio`

audio.h — Procedural audio engine

**Functions:**

```c
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
```

### `ev_types`

ev_types.h — Shared types for EV engine

**Functions:**

```c
// (no public functions)
```

### `lighting`

lighting.h — Custom shader-based lighting for the EV engine

**Functions:**

```c
EVLighting LoadEVLighting(void);
void UnloadEVLighting(EVLighting *lighting);
void UpdateEVLighting(EVLighting *lighting, Camera3D camera, Color fogColor, float fogDensity);
```

### `player`

player.h — Player movement and collision

**Functions:**

```c
void init_player(Player *p, Vector3 pos);
void update_player(Player *p, Scene *scene, float dt);
void kick_camera(Player *p, float pitch, float yaw);
```

### `render`

render.h — Rendering pipeline with post-processing

**Functions:**

```c
EVPostFX LoadEVPostFX(void);
void UnloadEVPostFX(EVPostFX *pfx);
void SetPostFXWarmth(EVPostFX *pfx, float warmth);
void draw_hud(Player *player, Scene *scene);
void draw_title(float planet_angle);
void draw_postfx(EVPostFX *pfx, RenderTexture2D render_target);
```

### `scene`

scene.h — Scene construction

**Functions:**

```c
void add_wall(Scene *s, float x, float y, float z, float w, float h, float d, Color c);
void add_light_panel(Scene *s, float x, float y, float z, float w, float h, float d, Color c);
void add_object(Scene *s, float x, float y, float z, const char *name, Color c, int max_steps);
void build_lobby(Scene *s);
void build_hallway(Scene *s);
void build_hotel_room(Scene *s);
void build_hotel_exterior(Scene *s);
void build_balcony(Scene *s);
void build_bathroom(Scene *s);
void build_stairwell(Scene *s);
```

---

## All Function Definitions

### `audio.c`

```c
static Wave gen_wave(int samples);
static Sound gen_step_marble(int seed);
static Sound gen_step_carpet(int seed);
static Sound gen_step_wood(int seed);
static Sound gen_ambient_lobby(void);
static Sound gen_ambient_hallway(void);
static Sound gen_ambient_room(void);
static Sound gen_click(void);
static Sound gen_fabric(void);
static Sound gen_flame(void);
static Sound gen_reward(void);
static Sound gen_sparkle(void);
static Sound gen_door_sound(void);
static Sound gen_city_ambient(void);
static Sound gen_clock_ambient(void);
void InitEVAudio(EVAudio *audio);
void UnloadEVAudio(EVAudio *audio);
static Sound *get_steps(EVAudio *audio, SurfaceType s);
static Sound *get_drone(EVAudio *audio, DroneType t);
void UpdateEVAudio(EVAudio *audio, bool moving, bool sprinting, SurfaceType surface, float dt);
void PlayInteract(EVAudio *audio, InteractSoundType type);
void PlayRewardSound(EVAudio *audio) { if(audio->initialized) PlaySound(audio->snd_reward); }
void PlaySparkleSound(EVAudio *audio) { if(audio->initialized) PlaySound(audio->snd_sparkle); }
void PlayDoorSound(EVAudio *audio) { if(audio->initialized) PlaySound(audio->door); }
void StartAmbient(EVAudio *audio, DroneType type);
void StopAmbient(EVAudio *audio);
void PlayCityAmbient(EVAudio *audio);
void StopCityAmbient(EVAudio *audio);
void PlayClockAmbient(EVAudio *audio);
void StopClockAmbient(EVAudio *audio);
```

### `lighting.c`

```c
EVLighting LoadEVLighting(void);
void UnloadEVLighting(EVLighting *lighting);
void UpdateEVLighting(EVLighting *lighting, Camera3D camera, Color fogColor, float fogDensity);
```

### `main.c`

```c
static void show_text(const char *text);
static void hide_text(void);
static InteractSoundType get_interact_sound(const char *name);
static void transition_to(GameState s);
static void transition_to_slow(GameState s, float spd);
static void load_state(GameState s);
static void draw_taxi_ride(float t);
static void draw_arrival(float t);
static void draw_vignette_text(void);
int main(void);
```

### `player.c`

```c
void init_player(Player *p, Vector3 pos);
void kick_camera(Player *p, float pitch, float yaw);
void update_player(Player *p, Scene *scene, float dt);
```

### `render.c`

```c
EVPostFX LoadEVPostFX(void);
void UnloadEVPostFX(EVPostFX *pfx);
void SetPostFXWarmth(EVPostFX *pfx, float warmth);
void draw_hud(Player *player, Scene *scene);
void draw_title(float planet_angle);
void draw_postfx(EVPostFX *pfx, RenderTexture2D render_target);
```

### `scene.c`

```c
void add_wall(Scene *s, float x, float y, float z, float w, float h, float d, Color c);
void add_object(Scene *s, float x, float y, float z, const char *name, Color c, int max_steps);
static void add_column(Scene *s, float x, float z, float r, float h, Color c);
void build_hotel_exterior(Scene *s);
void build_lobby(Scene *s);
void build_hallway(Scene *s);
void build_hotel_room(Scene *s);
void build_balcony(Scene *s);
void build_bathroom(Scene *s);
void build_stairwell(Scene *s);
```

---

## Scene Statistics

| Build Function | Walls | Objects |
|----------------|------:|--------:|
| `build_hotel_exterior` | 18 | 0
0 |
| `build_lobby` | 26 | 0
0 |
| `build_hallway` | 23 | 0
0 |
| `build_hotel_room` | 56 | 6 |
| `build_balcony` | 38 | 0
0 |
| `build_bathroom` | 23 | 3 |
| `build_stairwell` | 24 | 0
0 |

---

## Build Info

- **Compiler:** Apple clang version 17.0.0 (clang-1700.3.19.1)
- **Flags:** `-Wall -Wextra -std=c99 -O2`
- **Resolution:** 480x300
- **Total source lines:** 2846
