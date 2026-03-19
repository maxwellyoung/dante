// scene_suite.c — STATE_SPACE_SUITE
#include "game_ctx.h"
#include <math.h>
#include <string.h>
#include <stdio.h>

extern GameCtx g;

void set_exposure(float exp);
void hard_cut_to(GameState s);
void show_text(const char *text);
InteractSoundType get_interact_sound_ext(const char *name);

void suite_load(void) {
    bool replay = g.backstory_count > 3;

    if (replay) {
        // Second playthrough: the suite starts cleaned.
        // One pillow. One robe. One glass. The photograph face-up.
        // You arrive into the aftermath. You're not discovering loss — you're revisiting it.
        build_space_suite_cleaned(&g.scene);
    } else {
        build_space_suite(&g.scene);
    }
    init_player(&g.player, g.scene.spawn);
    printf("[DBG-SUITE] spawn=(%.1f,%.1f,%.1f) walls=%d objs=%d replay=%d\n",
           g.scene.spawn.x, g.scene.spawn.y, g.scene.spawn.z,
           g.scene.wall_count, g.scene.object_count, replay);
    // Particle emitters — dust, smoke, debris
    particle_clear(&g.particles);
    particle_add_emitter(&g.particles, (Vector3){-3.5f, 1.5f, -0.5f}, EMIT_DUST, 2.0f, 2.0f);   // window light shaft
    particle_add_emitter(&g.particles, (Vector3){-4.8f, 0.8f, 1.5f}, EMIT_SMOKE, 1.0f, 0.3f);    // ashtray
    particle_add_emitter(&g.particles, (Vector3){0, 2.5f, 0}, EMIT_DEBRIS, 0.5f, 8.0f);           // zero-g ambient
    StopAmbient(&g.audio);
    StopCityAmbient(&g.audio);
    StopWindAmbient(&g.audio);
    StopMuffledPiano(&g.audio); StopFootstepsAbove(&g.audio);
    StopDistantVoices(&g.audio);
    StopCorridorMusic(&g.audio);
    StopSound(g.audio.snd_running_water); StopSound(g.audio.snd_tv_murmur);
    PlayDoorThud(&g.audio);
    PlayAirlockHiss(&g.audio);
    g.player.gravity_mult = 0.5f;
    StartAmbient(&g.audio, DRONE_SPACE_SUITE);
    PlayClockAmbient(&g.audio);
    // Through-wall sounds — other lives in the hotel
    // Commandment 6: inaccessible spaces communicate
    PlayMuffledPiano(&g.audio);
    PlayDistantVoices(&g.audio);
    g.audio.clock_rate = 1.0f;
    SetSoundPitch(g.audio.snd_clock, 1.0f);
    g.agency_removal_timer = 0;
    SetSceneLighting(&g.lighting, LightingPreset_SpaceSuite());
    set_exposure(0.05f);           // slight lift — see the room, not a cave
    SetPostFXGrain(&g.postfx, 0.25f);    // 16mm stock, not VHS
    SetPostFXWarmth(&g.postfx, 0.08f);   // hint of warmth even before tasks — someone was here
    // Gibbons — gestures you in at threshold, walks to sofa, sits
    // "He bows. He deactivates." (Master Plan)
    {
        Vector3 suite_wps[] = {
            {0, 1.6f, 3.5f},    // threshold — pauses, gestures "after you"
            {0, 1.6f, 1},       // steps inside
            {-2.0f, 1.6f, 1},   // toward sofa
        };
        init_npc(&g.gibbons, (Vector3){0, 1.6f, 5.5f}, suite_wps, 3, 2.0f, 3.0f);
        g.gibbons.behavior = NPC_GESTURING;  // starts gesturing at door
        static const char *suite_lines_first[] = {
            "Everything as specified.",
            "The champagne was in the booking.",
            "Three hours passes differently up here.",
        };
        static const char *suite_lines_return[] = {
            "As you left it.",
            "I didn't touch the glass.",
            "Same three hours. Take your time.",
        };
        if (g.backstory_count > 3)
            npc_set_dialogue(&g.gibbons, suite_lines_return, 3, 4.0f);
        else
            npc_set_dialogue(&g.gibbons, suite_lines_first, 3, 4.0f);
    }
    // Thermostat — interactable (changes room warmth)
    add_object(&g.scene, 6.85f, 1.4f, -3.0f, "thermostat", (Color){210,205,195,255}, 3);
    // Adjoining door — interactable (opens to empty room)
    add_object(&g.scene, 6.86f, 1.5f, 3.5f, "adjoining_door", (Color){140,105,65,255}, 1);
    // Photograph — face-down first play, face-up on return (you turned it last time)
    if (replay) {
        // Already face-up — flip the wall color and add the red dot
        for (int wi = 0; wi < g.scene.wall_count; wi++) {
            Wall *w = &g.scene.walls[wi];
            if (fabsf(w->pos.y - 0.64f) < 0.1f &&
                w->size.y < 0.02f && w->size.x < 0.25f &&
                w->color.r > 230 && fabsf(w->pos.x + 2.5f) < 0.3f) {
                w->color = (Color){210,195,170,255};
                break;
            }
        }
        add_wall(&g.scene, -2.48f, 0.645f, -4.48f, 0.03f, 0.003f, 0.02f, (Color){190,50,45,200});
        set_last_decal(&g.scene);
        g.photograph_flipped = true;
        // Don't add photograph as interactable — it's already revealed
    } else {
        add_object(&g.scene, -2.5f, 0.64f, -4.5f, "photograph", (Color){240,238,230,255}, 1);
    }
    // ── HER TRACES — only on first play. Housekeeping cleaned on return. ──
    if (!replay) {
        add_object(&g.scene, -3.2f, 0.52f, 3.3f, "wineglass", (Color){210,210,215,255}, 1);
        add_object(&g.scene, -2.6f, 0.64f, -4.9f, "book", (Color){50,80,175,255}, 1);
        add_object(&g.scene, 1.2f, 0.02f, -3.8f, "sock", (Color){45,40,55,230}, 1);
        add_object(&g.scene, -2.5f, 0.4f, 3.6f, "roomservice", (Color){245,242,235,255}, 1);
        add_object(&g.scene, 2.3f, 0.65f, -4.9f, "postcard", (Color){245,240,232,255}, 1);
        add_object(&g.scene, 5.6f, 0.86f, -2.2f, "phone", (Color){20,20,25,255}, 1);
        add_object(&g.scene, 1.5f, 0.25f, 4.8f, "suitcase", (Color){120,85,45,255}, 1);
    }
    // Bathroom — run the bath
    add_object(&g.scene, 6.85f, 1.3f, 2.5f, "bathroom", (Color){225,222,218,255}, 1);

    // ── THE VOID WINDOW — hidden, behind the bathroom area ──
    // Pure black. No stars. No Earth. No reflection. Just nothing.
    // The player has to explore past the bathroom to find it.
    // No text. No interaction. It's just there.
    // On second playthrough, they go looking for it.
    add_wall(&g.scene, 6.85f, 1.5f, -5.5f, 0.05f, 2.0f, 1.2f, (Color){2,2,4,255});
    set_last_material(&g.scene, MAT_GLASS);
    // Thin frame — brass, barely visible
    add_wall(&g.scene, 6.85f, 2.5f, -5.5f, 0.06f, 0.03f, 1.25f, (Color){120,110,80,180});
    add_wall(&g.scene, 6.85f, 0.5f, -5.5f, 0.06f, 0.03f, 1.25f, (Color){120,110,80,180});
    add_wall(&g.scene, 6.85f, 1.5f, -4.88f, 0.06f, 2.05f, 0.03f, (Color){120,110,80,180});
    add_wall(&g.scene, 6.85f, 1.5f, -6.12f, 0.06f, 2.05f, 0.03f, (Color){120,110,80,180});
}

void suite_update(float dt) {
    update_player(&g.player, &g.scene, dt);
    float px = g.player.camera.position.x;
    float pz = g.player.camera.position.z;

    // Zone auto-completion flags (set by presence zones, read by phase timers)
    static bool zone_auto_lamp = false;
    static bool zone_auto_champ = false;
    static bool zone_auto_desk = false;
    bool zone_auto = false;

    // Speed modulation
    {
        float speed_mult = 1.0f;
        if (px < -4.0f) {
            float t = fminf(1.0f, (-4.0f - px) / 3.0f);
            speed_mult = 1.0f - t * 0.4f;
        }
        if (pz < -3.0f) {
            float t = fminf(1.0f, (-3.0f - pz) / 2.5f);
            float bed_mult = 1.0f - t * 0.3f;
            if (bed_mult < speed_mult) speed_mult = bed_mult;
        }
        if (speed_mult < 1.0f) {
            g.player.vel.x *= speed_mult;
            g.player.vel.z *= speed_mult;
        }

        // Spatial temperature contrast
        float cold_t = 0;
        if (px < -3.0f) cold_t = fminf(1.0f, (-3.0f - px) / 4.0f);
        float warm_t = 0;
        if (pz < -2.0f) warm_t = fminf(1.0f, (-2.0f - pz) / 4.0f);
        float temp = warm_t * 0.3f - cold_t * 0.2f;
        float base_warmth = (float)g.tasks_done / SPACE_TASK_COUNT;
        float time_warmth = fminf(0.15f, g.state_time * 0.001f);
        SetPostFXWarmth(&g.postfx, base_warmth + temp + time_warmth);
        float spatial_grain = 0.35f + cold_t * 0.3f - warm_t * 0.1f;
        SetPostFXGrain(&g.postfx, spatial_grain);

        // Phone ring — plays at 30s. Dies when you approach.
        // The interaction is the failure to interact.
        {
            static bool phone_ringing = false;
            static bool phone_killed = false;
            if (g.state_time > 30.0f && !phone_ringing && !phone_killed && g.tasks_done == 0) {
                PlayPhoneRing(&g.audio);
                phone_ringing = true;
            }
            // Walk toward the phone — it stops. You didn't answer. It just... stopped.
            if (phone_ringing && !phone_killed) {
                float phone_x = 5.6f, phone_z = -2.2f;
                float pdx = px - phone_x, pdz = pz - phone_z;
                float phone_dist = sqrtf(pdx*pdx + pdz*pdz);
                if (phone_dist < 2.0f) {
                    // Fade ring as you approach
                    float fade = phone_dist / 2.0f;
                    SetSoundVolume(g.audio.snd_phone_ring, 0.03f * fade);
                    if (phone_dist < 0.8f) {
                        StopSound(g.audio.snd_phone_ring);
                        phone_killed = true;
                        phone_ringing = false;
                    }
                }
            }
        }

        // Clock positional volume
        if (g.audio.clock_playing) {
            float center_dist = sqrtf(px * px + pz * pz);
            float clock_vol = 0.035f / (1.0f + center_dist * 0.15f);
            if (g.player.camera.position.y > 1.8f) clock_vol *= 1.3f;
            SetSoundVolume(g.audio.snd_clock, clock_vol);
        }
    }
    // ── VOID WINDOW — hidden, drains warmth when near ──
    {
        float vw_dx = px - 6.85f, vw_dz = pz - (-5.5f);
        float vw_dist = sqrtf(vw_dx*vw_dx + vw_dz*vw_dz);
        if (vw_dist < 3.0f) {
            float vw_t = 1.0f - (vw_dist / 3.0f);
            vw_t *= vw_t;  // quadratic falloff — stronger close up
            // The void pulls warmth from the room
            float bw = (float)g.tasks_done / SPACE_TASK_COUNT;
            SetPostFXWarmth(&g.postfx, fmaxf(0, bw - vw_t * 0.4f));
            SetPostFXGrain(&g.postfx, 0.35f + vw_t * 0.4f);
            // Subtle desaturation
            SetPostFXSaturation(&g.postfx, 0.92f - vw_t * 0.3f);
        }
    }
    // ── WINDOW ZONE — stand at the glass, watch Earth ──
    // Stay for 15 seconds → Earth rotates enough to show New Zealand.
    // The game rewards looking by showing you home.
    {
        static float window_dwell = 0;
        static bool window_revealed = false;
        if (px < -5.5f && !window_revealed) {
            window_dwell += dt;
            // Progressive: grain clears, FOV widens, exposure lifts
            float wt = fminf(1.0f, window_dwell / 15.0f);
            SetPostFXGrain(&g.postfx, 0.35f - wt * 0.25f);
            g.player.fov_current += (75.0f - g.player.fov_current) * wt * 0.02f;
            set_exposure(0.05f + wt * 0.1f);
            if (window_dwell >= 15.0f) {
                window_revealed = true;
                // Earth glow shifts — NZ comes into view (warm green tint in the blue)
                SetPointLightIdx(&g.lighting, 1, -7.0f, 0.5f, -1.0f,
                                 0.15f, 0.4f, 0.25f, 8.0f);  // green-blue → land mass
            }
        } else if (px >= -5.5f) {
            if (window_dwell > 0) window_dwell -= dt * 0.3f;
            if (window_dwell < 0) window_dwell = 0;
        }
    }
    // ── CROUCH AT BED — eye level with the pillow indent ──
    // Crouching near the bed puts your eyes at pillow height.
    // The indent (if it's appeared after all tasks) fills your view.
    if (g.player.crouching && pz < -3.5f && px > -2.0f && px < 2.0f) {
        // At bed height, FOV narrows — intimate, close
        g.player.fov_current += (55.0f - g.player.fov_current) * 0.04f;
        SetPostFXGrain(&g.postfx, 0.2f);  // clarity — see the weave of the fabric
    }

    // ── MICRO-GRAVITY DRIFT — loose objects float slightly ──
    // Small items bob on a slow sine wave. The physics are alive.
    // Only objects above waist height (y > 0.5) and small (size < 0.3)
    {
        float drift_time = g.state_time * 0.3f;
        for (int wi = 0; wi < g.scene.wall_count; wi++) {
            Wall *w = &g.scene.walls[wi];
            if (!w->active) continue;
            // Small objects on surfaces — not walls, floors, or structural
            if (w->size.x < 0.3f && w->size.z < 0.3f &&
                w->pos.y > 0.5f && w->pos.y < 2.0f &&
                w->size.y < 0.15f) {
                // Gentle bob — unique per object via position hash
                float phase = w->pos.x * 3.7f + w->pos.z * 2.3f;
                float bob = sinf(drift_time + phase) * 0.003f;
                w->pos.y += bob * dt;
            }
        }
    }
    UpdateEVAudio(&g.audio, g.player.moving, g.player.sprinting, g.scene.surface, dt);
    update_npc(&g.gibbons, g.player.camera.position, &g.scene, dt);
    // Gibbons vanishes
    if (!g.gibbons.active && g.gibbons.current_line >= g.gibbons.line_count) {
        g.gibbons.pos.y = -100;
    }
    // ── Zone auto-completion — when presence triggers step 1,
    // auto-advance to step 2 after the visual timer finishes.
    // The room completes its program. You just have to be there.

    // Lamp ritual — bedside lamp (slot 0) + floor lamp (slot 4) fade in together
    if (g.interaction_phases[0] == 1) {
        g.interaction_timers[0] -= dt;
        float gt = fminf(1.0f, 1.0f - g.interaction_timers[0] / 1.5f);
        SetPointLightIdx(&g.lighting, 0, -2.5f, 1.2f, -4.8f, gt, gt*0.82f, gt*0.45f, gt*8.0f);
        SetPointLightIdx(&g.lighting, 4, -5.5f, 1.4f, -1.0f,
                         gt*0.6f, gt*0.45f, gt*0.22f, gt*6.0f);
        if (g.interaction_timers[0] <= 0) {
            g.interaction_phases[0] = 2;
            // Auto-complete: if triggered by zone, simulate step 2
            if (zone_auto_lamp) {
                zone_auto_lamp = false;
                zone_auto = true;  // will trigger E-handler next frame
            }
        }
    }
    // Champagne ritual
    if (g.interaction_phases[1] == 1) {
        g.interaction_timers[1] -= dt;
        float pt = fminf(1.0f, 1.0f - g.interaction_timers[1] / 2.0f);
        SetPointLightIdx(&g.lighting, 2, -3.1f, 0.42f, 3.5f,
                         pt * 0.6f, pt * 0.5f, pt * 0.15f, pt * 3.0f);
        if (g.interaction_timers[1] <= 0) {
            g.interaction_phases[1] = 2;
            add_wall(&g.scene, -3.1f, 0.41f, 3.5f, 0.05f, 0.06f, 0.05f, (Color){240,210,100,200});
            add_sphere(&g.scene, -2.8f, 0.7f, 3.3f, 0.1f, (Color){240,210,100,180});
            add_sphere(&g.scene, -3.2f, 0.9f, 3.6f, 0.08f, (Color){240,210,100,160});
            add_sphere(&g.scene, -3.0f, 1.2f, 3.4f, 0.06f, (Color){240,210,100,140});
            if (zone_auto_champ) {
                zone_auto_champ = false;
                zone_auto = true;
            }
        }
    }
    // Desk ritual — desk lamp slot 3 + model lamp slot 5
    if (g.interaction_phases[2] == 1) {
        g.interaction_timers[2] -= dt;
        float dt2 = fminf(1.0f, 1.0f - g.interaction_timers[2] / 1.2f);
        SetPointLightIdx(&g.lighting, 3, 5.5f, 1.4f, -2.0f,
                         dt2 * 0.7f, dt2 * 0.6f, dt2 * 0.3f, dt2 * 5.0f);
        // Desk lamp model — tighter pool on desk surface
        SetPointLightIdx(&g.lighting, 5, 6.0f, 0.6f, -1.5f,
                         dt2 * 0.5f, dt2 * 0.4f, dt2 * 0.2f, dt2 * 4.0f);
        if (g.interaction_timers[2] <= 0) {
            g.interaction_phases[2] = 2;
            if (zone_auto_desk) {
                zone_auto_desk = false;
                zone_auto = true;
            }
        }
    }
    // Bed ritual — the emotional center of the game
    // Phase 1: lying down (camera descends, looks at ceiling)
    if (g.interaction_phases[3] == 1) {
        g.interaction_timers[3] -= dt;
        float bt = fminf(1.0f, 1.0f - g.interaction_timers[3] / 3.0f);
        // Camera sinks to pillow height
        float target_y = 1.6f - bt * 0.9f; // 1.6 → 0.7
        g.player.camera.position.y += (target_y - g.player.camera.position.y) * dt * 2.0f;
        // Look up at ceiling
        g.player.camera.target.y += (3.5f - g.player.camera.target.y) * dt * 1.5f;
        // Agency drains
        g.player.control_mult = fmaxf(0.0f, 0.3f - bt * 0.3f);
        if (g.interaction_timers[3] <= 0) {
            g.interaction_phases[3] = 2;
            g.player.control_mult = 0.0f; // fully surrendered
        }
    }
    // Phase 2: holding — the game holds you. Earth rotates. Music plays.
    if (g.interaction_phases[3] == 2) {
        // Gentle breathing camera
        float breath = sinf(g.state_time * 0.4f) * 0.003f;
        g.player.camera.position.y = 0.72f + breath;
        g.player.camera.target.y = 3.5f + breath * 2.0f;
        g.player.control_mult = 0.0f;
        // Progressive warmth — the room accepts you
        float hold_time = g.state_time - g.agency_removal_timer;
        if (g.agency_removal_timer < 0.01f) g.agency_removal_timer = g.state_time;
        hold_time = g.state_time - g.agency_removal_timer;
        float warm = fminf(1.0f, hold_time / 15.0f);
        SetPostFXWarmth(&g.postfx, warm);
        // After 20 seconds: hard cut to Paris dream
        if (hold_time > 20.0f) {
            g.player.control_mult = 1.0f;
            hard_cut_to(STATE_PARIS_DREAM);
        }
    }
    // Micro-animations
    if (g.interaction_phases[0] == 2) {
        float flk = 0.95f + 0.05f * sinf(g.state_time * 7.3f) * sinf(g.state_time * 11.1f);
        SetPointLightIdx(&g.lighting, 0, -2.5f, 1.2f, -4.8f, flk, flk*0.82f, flk*0.45f, 8.0f);
        // Floor lamp — gentler flicker, offset phase
        float flk2 = 0.92f + 0.08f * sinf(g.state_time * 5.1f) * sinf(g.state_time * 8.7f);
        SetPointLightIdx(&g.lighting, 4, -5.5f, 1.4f, -1.0f, flk2*0.6f, flk2*0.45f, flk2*0.22f, 6.0f);
    }
    if (g.interaction_phases[1] == 2) {
        float glow = 0.5f + 0.1f * sinf(g.state_time * 1.2f);
        SetPointLightIdx(&g.lighting, 2, -3.1f, 0.42f, 3.5f, glow*0.6f, glow*0.5f, glow*0.15f, 3.0f);
    }
    // ── PRESENCE ZONES — the room responds to where you stand ──
    // The suite was prepared for two. Everything was going to happen.
    // The champagne, the bath, the bed turned down. The hotel doesn't
    // know the trip changed. Things trigger by BEING there.
    // E-press still works as fallback for players who want buttons.
    {
        // Zone positions match task object positions
        typedef struct { float x, z, radius, threshold; const char *name; } Zone;
        static Zone zones[] = {
            { -2.5f, -4.8f, 2.0f, 4.0f, "lamp" },        // bedside area
            { -3.1f,  3.5f, 2.0f, 3.0f, "champagne" },    // drinks tray
            {  5.5f, -2.0f, 2.0f, 5.0f, "desk" },          // desk area
            {  0.0f, -4.5f, 2.5f, 8.0f, "bed" },           // bed area (longest dwell)
        };
        static float zone_timers[4] = {0};
        static bool zone_fired[4] = {false};
        int zone_count = 4;

        for (int zi = 0; zi < zone_count; zi++) {
            if (zone_fired[zi]) continue;
            float dx = px - zones[zi].x;
            float dz = pz - zones[zi].z;
            float dist = sqrtf(dx*dx + dz*dz);
            if (dist < zones[zi].radius) {
                zone_timers[zi] += dt;
                // Visual hint: subtle warmth increase as you dwell (the room notices you)
                if (zone_timers[zi] > zones[zi].threshold * 0.5f) {
                    float hint = (zone_timers[zi] - zones[zi].threshold * 0.5f) /
                                 (zones[zi].threshold * 0.5f);
                    hint = fminf(hint, 1.0f) * 0.03f;
                    // Micro-exposure lift — barely perceptible, player won't notice consciously
                    set_exposure(0.05f + hint);
                }
                if (zone_timers[zi] >= zones[zi].threshold) {
                    zone_fired[zi] = true;
                    zone_auto = true;
                    // Soft camera kick — the room activated
                    kick_camera(&g.player, -0.005f, 0.003f);
                }
            } else {
                // Decay slowly when leaving — you almost had it
                if (zone_timers[zi] > 0) zone_timers[zi] -= dt * 0.5f;
                if (zone_timers[zi] < 0) zone_timers[zi] = 0;
            }
        }
    }

    // Interactions — E-press OR zone proximity trigger
    if (IsKeyPressed(KEY_E) || zone_auto) {
        for (int i = 0; i < g.scene.object_count; i++) {
            InteractObject *obj = &g.scene.objects[i];
            if (!obj->active || obj->done) continue;
            float dist = Vector3Distance(g.player.camera.position, obj->pos);
            if (dist < obj->radius) {
                Vector3 to = Vector3Normalize(Vector3Subtract(obj->pos, g.player.camera.position));
                Vector3 look = Vector3Normalize(Vector3Subtract(g.player.camera.target, g.player.camera.position));
                // zone_auto bypasses facing check — presence, not attention
                if (Vector3DotProduct(to, look) > 0.5f || zone_auto) {
                    obj->step++;
                    PlayInteract(&g.audio, get_interact_sound_ext(obj->name));
                    kick_camera(&g.player, -0.01f, 0.005f);
                    g.interact_freeze = 0.05f;
                    g.interact_lean = 0.5f;
                    g.interact_lean_vel = 0;

                    // Thermostat — each step changes warmth. Not a task.
                    if (strcmp(obj->name, "thermostat") == 0) {
                        float warmth_step = obj->step * 0.15f;
                        SetPostFXWarmth(&g.postfx, warmth_step);
                        if (obj->step >= obj->max_steps) obj->step = 0; // cycles
                        obj->done = false; // never "done" — keep cycling
                        break;
                    }
                    // Adjoining door — opens to reveal empty room
                    if (strcmp(obj->name, "adjoining_door") == 0 && obj->step == 1) {
                        // Deactivate the door panel — reveal void behind
                        for (int wi = 0; wi < g.scene.wall_count; wi++) {
                            Wall *w = &g.scene.walls[wi];
                            if (w->material == MAT_WOOD &&
                                fabsf(w->pos.x - 6.86f) < 0.2f &&
                                fabsf(w->pos.z - 3.5f) < 0.3f &&
                                w->size.y > 2.0f) {
                                w->active = false; // door slides open
                            }
                        }
                        // Dark rectangle behind — the empty room
                        add_wall(&g.scene, 7.5f, 2.0f, 3.5f, 2.0f, 4.0f, 3.0f, (Color){8,8,12,255});
                        // One pillow visible through doorway
                        add_wall(&g.scene, 8.0f, 0.68f, 3.0f, 0.5f, 0.15f, 0.3f, (Color){240,238,232,255});
                        set_last_material(&g.scene, MAT_FABRIC);
                        obj->done = true;
                        break;
                    }

                    // Photograph — turn it face-up. The two of you. Paris.
                    if (strcmp(obj->name, "photograph") == 0) {
                        // Flip the photo — find the face-down wall and change its color
                        for (int wi = 0; wi < g.scene.wall_count; wi++) {
                            Wall *w = &g.scene.walls[wi];
                            if (fabsf(w->pos.y - 0.64f) < 0.1f &&
                                w->size.y < 0.02f && w->size.x < 0.25f &&
                                w->color.r > 230) {
                                // Flip to photo side — warm sepia with red accent
                                w->color = (Color){210,195,170,255};
                                break;
                            }
                        }
                        // Add a tiny red dot — her dress in the photo
                        add_wall(&g.scene, -2.48f, 0.645f, -4.48f, 0.03f, 0.003f, 0.02f, (Color){190,50,45,200});
                        set_last_decal(&g.scene);
                        g.photograph_flipped = true;
                        // Choice colors interpretation — "Before" vs "After"
                        if (g.backstory[0] == 1)  // "After" — booked after she left
                            show_text("Paris. The trip that was supposed to fix things.");
                        else
                            show_text("Paris. Before all this.");
                        obj->done = true;
                        break;
                    }
                    // ── SILENT OBJECTS — architecture speaks, not text ──
                    // These objects have VISUAL consequence only. No show_text().
                    // The player sees. The player understands. Trust the architecture.

                    // Wardrobe — opens to reveal two robes. Different sizes. Say nothing.
                    if (strcmp(obj->name, "wardrobe") == 0 && obj->step == 1) {
                        add_wall(&g.scene, -5.2f, 1.2f, 3.3f, 0.05f, 1.8f, 0.5f, (Color){225,215,200,255});
                        set_last_material(&g.scene, MAT_FABRIC); // her robe — cream, smaller
                        add_wall(&g.scene, -5.4f, 1.2f, 3.3f, 0.05f, 1.8f, 0.5f, (Color){50,55,80,255});
                        set_last_material(&g.scene, MAT_FABRIC); // his robe — navy, larger
                        // The size difference IS the text.
                        obj->done = true;
                        break;
                    }
                    // Wine glass — lean in, see the lipstick mark. Say nothing.
                    if (strcmp(obj->name, "wineglass") == 0 && obj->step == 1) {
                        // Stronger lean-in so the player sees the mark
                        g.interact_lean = 1.0f;
                        obj->done = true;
                        break;
                    }
                    // Her book — opens, boarding pass drops out as geometry
                    if (strcmp(obj->name, "book") == 0 && obj->step == 1) {
                        // Boarding pass falls — blue stripe on white, lands on nightstand
                        add_wall(&g.scene, -2.55f, 0.645f, -4.7f, 0.15f, 0.004f, 0.08f, (Color){245,242,235,255});
                        set_last_decal(&g.scene);
                        add_wall(&g.scene, -2.55f, 0.648f, -4.73f, 0.15f, 0.003f, 0.02f, (Color){50,80,180,255});
                        set_last_decal(&g.scene);
                        obj->done = true;
                        break;
                    }
                    // Sock — say nothing. It's a sock under the bed. You see it.
                    if (strcmp(obj->name, "sock") == 0 && obj->step == 1) {
                        obj->done = true;
                        break;
                    }
                    // Room service card — you're READING. Text is diegetic.
                    if (strcmp(obj->name, "roomservice") == 0 && obj->step == 1) {
                        show_text("Two handwritings. A disagreement about the cheese plate.");
                        obj->done = true;
                        break;
                    }
                    // Postcard — you're READING an address line.
                    if (strcmp(obj->name, "postcard") == 0 && obj->step == 1) {
                        show_text("Addressed to no one.");
                        obj->done = true;
                        break;
                    }
                    // Phone — screen lights up. Warm glow on desk. Say nothing.
                    if (strcmp(obj->name, "phone") == 0 && obj->step == 1) {
                        // Phone screen glow — warm rectangle of light on desk surface
                        add_light_panel(&g.scene, 5.6f, 0.88f, -2.2f, 0.12f, 0.01f, 0.06f, (Color){180,200,240,120});
                        obj->done = true;
                        break;
                    }
                    // Suitcase — say nothing. Tags still on. Never opened. You see it.
                    if (strcmp(obj->name, "suitcase") == 0 && obj->step == 1) {
                        obj->done = true;
                        break;
                    }
                    // Bathroom — water runs, steam appears. Say nothing.
                    if (strcmp(obj->name, "bathroom") == 0 && obj->step == 1) {
                        add_wall(&g.scene, -6.8f, 2.0f, -1.0f, 0.1f, 3.0f, 4.0f, (Color){200,210,220,25});
                        add_wall(&g.scene, -6.6f, 2.5f, -0.5f, 0.08f, 2.0f, 3.0f, (Color){200,210,220,15});
                        SetSoundVolume(g.audio.snd_running_water, 0.04f);
                        PlaySound(g.audio.snd_running_water);
                        // The bath is big. You can see that. The water runs. That's enough.
                        obj->done = true;
                        break;
                    }

                    if (strcmp(obj->name, "lamp") == 0 && obj->step == 1) {
                        add_wall(&g.scene, -2.5f, 1.0f, -4.65f, 0.15f, 0.25f, 0.15f, (Color){220,210,185,180});
                        g.interaction_phases[0] = 1;
                        g.interaction_timers[0] = 1.5f;
                        if (zone_auto) zone_auto_lamp = true;  // auto-complete when timer finishes
                    } else if (strcmp(obj->name, "desk") == 0 && obj->step == 1) {
                        add_wall(&g.scene, 5.3f, 0.45f, -1.6f, 0.4f, 0.04f, 0.3f, (Color){55,85,175,255});
                        g.interaction_phases[2] = 1;
                        g.interaction_timers[2] = 1.2f;
                        if (zone_auto) zone_auto_desk = true;
                    } else if (strcmp(obj->name, "bed") == 0 && obj->step == 1) {
                        // Pull back the covers — the Chevalier moment
                        add_wall(&g.scene, 0, 0.54f, -4.3f, 2.8f, 0.02f, 1.4f, (Color){245,242,235,255});
                        g.interaction_phases[3] = 1;
                        g.interaction_timers[3] = 3.0f;
                        PlayBedRitual(&g.audio);
                        g.player.control_mult = 0.3f;
                        // Bed auto-completes via interaction_phases[3]==2 (already wired)
                    } else if (strcmp(obj->name, "champagne") == 0 && obj->step == 1) {
                        add_cone(&g.scene, -3.1f, 0.39f, 3.5f, 0.06f, 0.08f, (Color){210,210,215,200});
                        add_cylinder(&g.scene, -3.1f, 0.44f, 3.5f, 0.02f, 0.08f, (Color){210,210,215,200});
                        if (zone_auto) zone_auto_champ = true;
                    }

                    if (strcmp(obj->name, "champagne") == 0 && obj->step == 2) {
                        PlayInteract(&g.audio, INTERACT_GLASS_CLINK);
                        g.interaction_phases[1] = 1;
                        g.interaction_timers[1] = 2.0f;
                        // Champagne pop — burst of gold sparks
                        particle_burst(&g.particles, (Vector3){-3.1f, 0.5f, 3.5f}, EMIT_SPARKS, 20, 0.3f);
                    }

                    if (obj->step >= obj->max_steps) {
                        obj->done = true;
                        obj->reward_timer = 1.5f;
                        g.tasks_done++;
                        PlayRewardSound(&g.audio);
                        SetPostFXWarmth(&g.postfx, (float)g.tasks_done / SPACE_TASK_COUNT);
                        g.scene.fog_density = 0.001f - ((float)g.tasks_done / SPACE_TASK_COUNT * 0.0005f);

                        if (g.tasks_done == 2) PlaySuiteMusic(&g.audio);

                        // ── WRONGNESS — The Witches layer ──
                        // After each task, the room shifts. Not horror. Just... wrong.
                        // The wrongness IS the character's psychological state.
                        // The photograph changes like the girl in the painting.
                        // The background empties. The light shifts. Nobody comments.
                        if (g.tasks_done == 1) {
                            // Photograph: the café background gets darker.
                            // The afternoon light is fading. Was it always evening?
                            if (g.photograph_flipped) {
                                // Darken the photo base — afternoon → dusk
                                for (int wi = 0; wi < g.scene.wall_count; wi++) {
                                    Wall *w = &g.scene.walls[wi];
                                    if (fabsf(w->pos.y - 0.64f) < 0.05f &&
                                        w->size.y < 0.02f && w->size.x < 0.25f &&
                                        w->color.r > 200 && w->color.r < 220) {
                                        w->color = (Color){185,170,145,255}; // dusk sepia
                                        break;
                                    }
                                }
                                // A shadow appears on one side — the light changed
                                add_wall_decal(&g.scene, -2.42f, 0.645f, -4.55f,
                                    0.06f, 0.002f, 0.12f, (Color){140,125,105,200});
                            }
                            // Photograph also shifts position — did it move?
                            for (int wi = 0; wi < g.scene.wall_count; wi++) {
                                if (fabsf(g.scene.walls[wi].pos.y - 0.64f) < 0.05f &&
                                    g.scene.walls[wi].size.y < 0.02f &&
                                    g.scene.walls[wi].size.x < 0.25f &&
                                    g.scene.walls[wi].color.r > 130) {
                                    g.scene.walls[wi].pos.x += 0.03f;
                                    g.scene.walls[wi].pos.z += 0.02f;
                                }
                            }
                        } else if (g.tasks_done == 2) {
                            // Bathroom door cracks open — warm amber light spills out
                            for (int wi = 0; wi < g.scene.wall_count; wi++) {
                                Wall *w = &g.scene.walls[wi];
                                if (w->material == MAT_WOOD &&
                                    fabsf(w->pos.x - 6.86f) < 0.5f &&
                                    fabsf(w->pos.z + 3.0f) < 0.5f &&
                                    w->size.y > 2.0f) {
                                    w->pos.x += 0.15f; // door slightly ajar
                                    // Warm light through the crack
                                    add_light_panel(&g.scene, w->pos.x - 0.1f, 1.0f, w->pos.z,
                                        0.05f, 2.0f, 0.05f, (Color){240,200,120,40});
                                    break;
                                }
                            }
                        } else if (g.tasks_done == 3) {
                            // Earth glow shifts angle — it's not where it should be
                            for (int wi = 0; wi < g.scene.wall_count; wi++) {
                                Wall *w = &g.scene.walls[wi];
                                if (w->color.r < 70 && w->color.g > 100 && w->color.b > 180 &&
                                    w->color.a < 200 && w->pos.y < 0.1f) {
                                    w->pos.x += 1.5f;
                                    w->pos.z -= 0.8f;
                                }
                            }
                            // The second champagne glass drifts — 3 inches to the left
                            for (int wi = 0; wi < g.scene.wall_count; wi++) {
                                Wall *w = &g.scene.walls[wi];
                                if (w->pos.x > -2.7f && w->pos.x < -2.5f &&
                                    w->pos.y > 0.4f && w->pos.y < 0.5f &&
                                    w->pos.z > 3.2f && w->pos.z < 3.5f &&
                                    (w->shape == SHAPE_CYLINDER || w->shape == SHAPE_CONE)) {
                                    w->pos.x -= 0.08f;
                                }
                            }
                            // Her book shifts position
                            for (int wi = 0; wi < g.scene.wall_count; wi++) {
                                Wall *w = &g.scene.walls[wi];
                                if (w->pos.x < -2.4f && w->pos.x > -2.8f &&
                                    w->pos.y > 0.6f && w->pos.y < 0.7f &&
                                    w->pos.z < -4.7f && w->color.b > 150) {
                                    w->pos.z += 0.04f;
                                    break;
                                }
                            }
                            // ── THE WITCHES — photograph task 3 ──
                            // The red dot (her dress) fades. She's turning away.
                            // The café chair beside her is empty now.
                            if (g.photograph_flipped) {
                                // Darken further — dusk → near-night
                                for (int wi = 0; wi < g.scene.wall_count; wi++) {
                                    Wall *w = &g.scene.walls[wi];
                                    if (fabsf(w->pos.y - 0.64f) < 0.05f &&
                                        w->size.y < 0.02f && w->size.x < 0.25f &&
                                        w->color.r > 170 && w->color.r < 200) {
                                        w->color = (Color){155,140,120,255}; // evening
                                        break;
                                    }
                                }
                                // Her dress fades — the red loses saturation
                                for (int wi = 0; wi < g.scene.wall_count; wi++) {
                                    Wall *w = &g.scene.walls[wi];
                                    if (w->color.r > 180 && w->color.g < 60 && w->color.b < 60 &&
                                        fabsf(w->pos.y - 0.645f) < 0.01f) {
                                        w->color = (Color){140,90,75,150}; // faded red
                                        break;
                                    }
                                }
                                // Empty chair appears — a shadow where someone sat
                                add_wall_decal(&g.scene, -2.55f, 0.645f, -4.42f,
                                    0.025f, 0.002f, 0.025f, (Color){120,110,95,180});
                            }
                        }
                        // After ALL tasks: the photograph reaches final state
                        // and the second pillow gains an indent.
                        // The Witches: the girl has aged. The café is closed.
                        if (g.tasks_done == SPACE_TASK_COUNT) {
                            if (g.photograph_flipped) {
                                // Final state — near-black. Night. The café is closed.
                                // She's still there but you can barely see her.
                                for (int wi = 0; wi < g.scene.wall_count; wi++) {
                                    Wall *w = &g.scene.walls[wi];
                                    if (fabsf(w->pos.y - 0.64f) < 0.05f &&
                                        w->size.y < 0.02f && w->size.x < 0.25f &&
                                        w->color.r > 100 && w->color.r < 170) {
                                        w->color = (Color){85,75,60,255}; // night
                                        break;
                                    }
                                }
                                // The red is almost gone — a memory of color
                                for (int wi = 0; wi < g.scene.wall_count; wi++) {
                                    Wall *w = &g.scene.walls[wi];
                                    if (w->color.r > 120 && w->color.g < 100 && w->color.b < 90 &&
                                        fabsf(w->pos.y - 0.645f) < 0.01f) {
                                        w->color = (Color){90,70,60,80}; // nearly invisible
                                        break;
                                    }
                                }
                            }
                            // Find second pillow and add a slight depression
                            for (int wi = 0; wi < g.scene.wall_count; wi++) {
                                Wall *w = &g.scene.walls[wi];
                                if (w->pos.z < -5.0f && w->pos.z > -5.4f &&
                                    w->pos.x < -0.3f && w->pos.x > -0.9f &&
                                    w->pos.y > 0.6f && w->pos.y < 0.75f &&
                                    w->size.x > 0.4f) {
                                    // Lower it slightly — indent appears
                                    w->pos.y -= 0.03f;
                                    w->size.y *= 0.7f;
                                    // Darker shadow in the indent
                                    add_wall_decal(&g.scene, w->pos.x, w->pos.y + 0.01f, w->pos.z,
                                        0.2f, 0.005f, 0.15f, (Color){220,218,212,255});
                                    break;
                                }
                            }
                        }

                        float new_rate = 1.0f - (float)g.tasks_done * 0.2f;
                        if (new_rate < 0.1f) new_rate = 0.1f;
                        SetClockRate(&g.audio, new_rate);

                        if (strcmp(obj->name, "lamp") == 0) {
                            add_light_panel(&g.scene, -2.5f, 1.2f, -4.8f, 0.5f, 0.6f, 0.5f, (Color){255,220,120,200});
                            add_wall(&g.scene, -2.5f, 1.8f, -5.35f, 1.2f, 1.5f, 0.01f, (Color){60,45,20,40});
                        } else if (strcmp(obj->name, "desk") == 0) {
                            add_wall(&g.scene, 5.5f, 0.85f, -2, 1.8f, 0.01f, 0.8f, (Color){20,25,45,255});
                            add_wall_decal(&g.scene, 5.2f, 0.86f, -2.1f, 0.06f, 0.005f, 0.06f, (Color){240,235,220,200});
                            add_wall_decal(&g.scene, 5.7f, 0.86f, -1.8f, 0.04f, 0.005f, 0.04f, (Color){240,235,220,180});
                            add_wall_decal(&g.scene, 5.4f, 0.86f, -1.6f, 0.05f, 0.005f, 0.05f, (Color){240,235,220,160});
                            add_wall_decal(&g.scene, 6.0f, 0.86f, -2.3f, 0.03f, 0.005f, 0.03f, (Color){240,235,220,170});
                            add_wall_decal(&g.scene, 5.45f, 0.862f, -1.95f, 0.8f, 0.003f, 0.02f, (Color){180,60,50,100});
                            add_cylinder(&g.scene, 5.7f, 0.863f, -1.8f, 0.12f, 0.003f, (Color){180,60,50,80});
                            set_last_decal(&g.scene);
                        } else if (strcmp(obj->name, "bed") == 0) {
                            add_wall(&g.scene, 0, 0.68f, -5.0f, 0.18f, 0.04f, 0.12f, (Color){90,50,25,255});
                            g.interaction_phases[3] = 1;
                            g.interaction_timers[3] = 2.0f;
                        } else if (strcmp(obj->name, "champagne") == 0) {
                            add_wall(&g.scene, -3.1f, 0.41f, 3.5f, 0.05f, 0.06f, 0.05f, (Color){240,210,100,200});
                            add_sphere(&g.scene, -2.8f, 0.7f, 3.3f, 0.1f, (Color){240,210,100,180});
                            add_sphere(&g.scene, -3.2f, 0.9f, 3.6f, 0.08f, (Color){240,210,100,160});
                            add_sphere(&g.scene, -3.0f, 1.2f, 3.4f, 0.06f, (Color){240,210,100,140});
                            add_sphere(&g.scene, -2.9f, 2.0f, 3.5f, 0.04f, (Color){180,200,240,100});
                            add_sphere(&g.scene, -3.1f, 2.8f, 3.35f, 0.025f, (Color){160,190,230,60});
                        }

                        if (g.tasks_done >= SPACE_TASK_COUNT) {
                            kick_camera(&g.player, -0.05f, 0.02f);
                            SetPostFXWarmth(&g.postfx, 1.0f);
                        } else {
                            kick_camera(&g.player, -0.02f, 0.008f);
                        }
                    }
                    break;
                }
            }
        }
    }
    // Gibbons sits
    if (!g.gibbons.active && g.tasks_done < SPACE_TASK_COUNT) {
        g.gibbons.behavior = NPC_SITTING;
        g.gibbons.active = true;
        g.gibbons.waiting = true;
    }
    // All tasks done
    if (g.tasks_done >= SPACE_TASK_COUNT) {
        g.done_pause += dt;
        if (g.done_pause < 0.1f && g.gibbons.behavior == NPC_SITTING) {
            g.gibbons.behavior = NPC_WALKING;
            g.gibbons.waiting = false;
            g.gibbons.waypoints[0] = (Vector3){0, 1.6f, 5};
            g.gibbons.waypoint_count = 1;
            g.gibbons.current_waypoint = 0;
            g.gibbons.target_pos = g.gibbons.waypoints[0];
            static const char *exit_line[] = {"Time."};
            npc_set_dialogue(&g.gibbons, exit_line, 1, 3.0f);
        }
        if (g.done_pause > 0.5f) {
            StopClockAmbient(&g.audio);
        }
        if (g.done_pause < 4.0f) {
            float removal = g.done_pause / 4.0f;
            g.player.control_mult = 1.0f - removal;
        } else {
            g.player.control_mult = 0.0f;
        }
        if (g.done_pause > 5.0f) {
            g.player.control_mult = 1.0f;
            g.balcony_flash_triggered = false;
            g.balcony_flash_timer = 0;
            hard_cut_to(STATE_BALCONY);
        }
    }
}
