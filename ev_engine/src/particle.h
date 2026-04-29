// particle.h — Lightweight particle system for EV engine
// Dust catches lamplight. Smoke curls from the ashtray. Sparks on champagne pop.
#ifndef EV_PARTICLE_H
#define EV_PARTICLE_H

#include "raylib.h"
#include <stdbool.h>

#define MAX_PARTICLES 256
#define MAX_EMITTERS 8

typedef struct {
    Vector3 pos, vel;
    Color color;
    float life, max_life, size;
    bool active;
} Particle;

typedef enum {
    EMIT_DUST,     // slow upward drift, warm cream, near point lights tint toward light
    EMIT_SMOKE,    // curls upward + turbulence, grey, grows over life
    EMIT_SPARKS,   // fast random, bright gold, tiny, short-lived
    EMIT_DEBRIS,   // slow tumble, grey-white, long-lived (zero-g replacement)
} EmitterType;

typedef struct {
    Vector3 pos;
    EmitterType type;
    float rate;         // particles per second
    float spread;       // cone spread radius
    bool active;
    float accum;        // time accumulator for emission
} Emitter;

typedef struct {
    Particle particles[MAX_PARTICLES];
    Emitter emitters[MAX_EMITTERS];
    int emitter_count;
    unsigned int rng_seed;
} ParticleSystem;

void particle_init(ParticleSystem *ps);
void particle_add_emitter(ParticleSystem *ps, Vector3 pos, EmitterType type, float rate, float spread);
void particle_burst(ParticleSystem *ps, Vector3 pos, EmitterType type, int count, float spread);
void particle_update(ParticleSystem *ps, float dt);
void particle_draw(ParticleSystem *ps, Camera3D camera);
void particle_clear(ParticleSystem *ps);

#endif
