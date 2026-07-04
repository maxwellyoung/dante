// particle.c — Particle system for EV engine
// Dust in lamplight. Ashtray smoke. Champagne sparks. Zero-g debris.
#include "particle.h"
#include "raymath.h"
#include "rlgl.h"
#include <math.h>
#include <stdlib.h>

// Simple deterministic random for particle variation
static float prand(ParticleSystem *ps) {
    ps->rng_seed = ps->rng_seed * 1103515245u + 12345u;
    return (float)(ps->rng_seed >> 16 & 0x7FFFu) / 32767.0f;
}

void particle_init(ParticleSystem *ps) {
    for (int i = 0; i < MAX_PARTICLES; i++) ps->particles[i].active = false;
    ps->emitter_count = 0;
    for (int i = 0; i < MAX_EMITTERS; i++) ps->emitters[i].active = false;
    ps->rng_seed = 12345u;
}

void particle_add_emitter(ParticleSystem *ps, Vector3 pos, EmitterType type, float rate, float spread) {
    if (ps->emitter_count >= MAX_EMITTERS) return;
    ps->emitters[ps->emitter_count++] = (Emitter){
        .pos = pos, .type = type, .rate = rate, .spread = spread, .active = true, .accum = 0,
    };
}

static void spawn_particle(ParticleSystem *ps, Vector3 pos, EmitterType type, float spread) {
    // Find free slot
    int slot = -1;
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (!ps->particles[i].active) { slot = i; break; }
    }
    if (slot < 0) return;

    Particle *p = &ps->particles[slot];
    p->active = true;
    p->pos = pos;
    // Random offset within spread
    p->pos.x += (prand(ps) - 0.5f) * spread;
    p->pos.y += (prand(ps) - 0.5f) * spread * 0.5f;
    p->pos.z += (prand(ps) - 0.5f) * spread;

    switch (type) {
    case EMIT_DUST:
        p->vel = (Vector3){(prand(ps)-0.5f)*0.05f, 0.02f + prand(ps)*0.03f, (prand(ps)-0.5f)*0.05f};
        p->max_life = 8.0f + prand(ps) * 7.0f;
        p->size = 0.01f + prand(ps) * 0.01f;
        p->color = (Color){235, 225, 200, (unsigned char)(60 + (int)(prand(ps)*80))};
        break;
    case EMIT_SMOKE:
        p->vel = (Vector3){(prand(ps)-0.5f)*0.1f, 0.08f + prand(ps)*0.06f, (prand(ps)-0.5f)*0.1f};
        p->max_life = 3.0f + prand(ps) * 3.0f;
        p->size = 0.02f;
        p->color = (Color){160, 155, 145, (unsigned char)(40 + (int)(prand(ps)*60))};
        break;
    case EMIT_SPARKS:
        p->vel = (Vector3){(prand(ps)-0.5f)*2.0f, prand(ps)*3.0f, (prand(ps)-0.5f)*2.0f};
        p->max_life = 0.5f + prand(ps) * 1.0f;
        p->size = 0.008f + prand(ps) * 0.005f;
        p->color = (Color){240, 210, 100, (unsigned char)(180 + (int)(prand(ps)*75))};
        break;
    case EMIT_DEBRIS:
        p->vel = (Vector3){(prand(ps)-0.5f)*0.02f, (prand(ps)-0.5f)*0.01f, (prand(ps)-0.5f)*0.02f};
        p->max_life = 20.0f + prand(ps) * 10.0f;
        p->size = 0.005f + prand(ps) * 0.008f;
        p->color = (Color){220, 218, 210, (unsigned char)(30 + (int)(prand(ps)*50))};
        break;
    }
    p->life = p->max_life;
}

void particle_burst(ParticleSystem *ps, Vector3 pos, EmitterType type, int count, float spread) {
    for (int i = 0; i < count; i++) {
        spawn_particle(ps, pos, type, spread);
    }
}

void particle_update(ParticleSystem *ps, float dt) {
    // Emit from active emitters
    for (int e = 0; e < ps->emitter_count; e++) {
        Emitter *em = &ps->emitters[e];
        if (!em->active) continue;
        em->accum += dt;
        float interval = 1.0f / em->rate;
        while (em->accum >= interval) {
            em->accum -= interval;
            spawn_particle(ps, em->pos, em->type, em->spread);
        }
    }
    // Update particles
    for (int i = 0; i < MAX_PARTICLES; i++) {
        Particle *p = &ps->particles[i];
        if (!p->active) continue;
        p->life -= dt;
        if (p->life <= 0) { p->active = false; continue; }
        p->pos.x += p->vel.x * dt;
        p->pos.y += p->vel.y * dt;
        p->pos.z += p->vel.z * dt;
        // Smoke grows over lifetime
        if (p->size < 0.08f && p->max_life > 2.0f && p->max_life < 8.0f) {
            p->size += dt * 0.01f;
        }
        // Turbulence for smoke
        if (p->max_life >= 3.0f && p->max_life <= 6.0f) {
            p->vel.x += (prand(ps) - 0.5f) * 0.3f * dt;
            p->vel.z += (prand(ps) - 0.5f) * 0.3f * dt;
        }
    }
}

void particle_draw(ParticleSystem *ps, Camera3D camera) {
    // Billboard quads facing camera
    Vector3 cam_right = Vector3Normalize(Vector3CrossProduct(
        Vector3Subtract(camera.target, camera.position), camera.up));
    Vector3 cam_up = camera.up;

    rlDisableBackfaceCulling();
    for (int i = 0; i < MAX_PARTICLES; i++) {
        Particle *p = &ps->particles[i];
        if (!p->active) continue;
        float alpha_t = p->life / p->max_life;
        // Fade in first 10%, fade out last 30%
        float fade = 1.0f;
        if (alpha_t > 0.9f) fade = (1.0f - alpha_t) * 10.0f;
        else if (alpha_t < 0.3f) fade = alpha_t / 0.3f;
        Color c = p->color;
        c.a = (unsigned char)(c.a * fade);
        if (c.a < 2) continue;

        float s = p->size;
        Vector3 r = Vector3Scale(cam_right, s);
        Vector3 u = Vector3Scale(cam_up, s);
        Vector3 v0 = Vector3Subtract(Vector3Subtract(p->pos, r), u);
        Vector3 v1 = Vector3Add(Vector3Subtract(p->pos, r), u);
        Vector3 v2 = Vector3Add(Vector3Add(p->pos, r), u);
        Vector3 v3 = Vector3Subtract(Vector3Add(p->pos, r), u);
        DrawTriangle3D(v0, v1, v2, c);
        DrawTriangle3D(v0, v2, v3, c);
    }
    rlEnableBackfaceCulling();
}

void particle_clear(ParticleSystem *ps) {
    for (int i = 0; i < MAX_PARTICLES; i++) ps->particles[i].active = false;
    for (int i = 0; i < MAX_EMITTERS; i++) ps->emitters[i].active = false;
    ps->emitter_count = 0;
    ps->rng_seed = 12345u;
}
