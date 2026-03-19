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
    build_space_suite(&g.scene);
    init_player(&g.player, g.scene.spawn);
    printf("[DBG-SUITE] spawn=(%.1f,%.1f,%.1f) walls=%d objs=%d\n",
           g.scene.spawn.x, g.scene.spawn.y, g.scene.spawn.z,
           g.scene.wall_count, g.scene.object_count);
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
        static const char *suite_lines[] = {
            "Everything as specified.",
            "The champagne was in the booking.",
            "Three hours passes differently up here.",
        };
        npc_set_dialogue(&g.gibbons, suite_lines, 3, 4.0f);
    }
    // Thermostat — interactable (changes room warmth)
    add_object(&g.scene, 6.85f, 1.4f, -3.0f, "thermostat", (Color){210,205,195,255}, 3);
    // Adjoining door — interactable (opens to empty room)
    add_object(&g.scene, 6.86f, 1.5f, 3.5f, "adjoining_door", (Color){140,105,65,255}, 1);
    // Photograph — face-down on nightstand, turnable
    add_object(&g.scene, -2.5f, 0.64f, -4.5f, "photograph", (Color){240,238,230,255}, 1);
    // Wine glass with lipstick — examine
    add_object(&g.scene, -3.2f, 0.52f, 3.3f, "wineglass", (Color){210,210,215,255}, 1);
    // Her book on left nightstand — pick up to see boarding pass bookmark
    add_object(&g.scene, -2.6f, 0.64f, -4.9f, "book", (Color){50,80,175,255}, 1);
    // Bolaño objects — the abstract-mundane. Resist interpretation.
    add_object(&g.scene, 1.2f, 0.02f, -3.8f, "sock", (Color){45,40,55,230}, 1);
    add_object(&g.scene, -2.5f, 0.4f, 3.6f, "roomservice", (Color){245,242,235,255}, 1);
    add_object(&g.scene, 2.3f, 0.65f, -4.9f, "postcard", (Color){245,240,232,255}, 1);
    // Phone on desk — playlist still queued
    add_object(&g.scene, 5.6f, 0.86f, -2.2f, "phone", (Color){20,20,25,255}, 1);
    // Suitcase by the door — hers, never opened
    add_object(&g.scene, 1.5f, 0.25f, 4.8f, "suitcase", (Color){120,85,45,255}, 1);
    // Bathroom — run the bath
    add_object(&g.scene, 6.85f, 1.3f, 2.5f, "bathroom", (Color){225,222,218,255}, 1);
}

void suite_update(float dt) {
    update_player(&g.player, &g.scene, dt);
    // Speed modulation
    {
        float px = g.player.camera.position.x;
        float pz = g.player.camera.position.z;
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

        // Phone ring — plays once, 30s after entry. Unanswered.
        // The booking confirmation. "Your party of two has checked in."
        if (g.state_time > 30.0f && g.state_time < 30.5f && g.tasks_done == 0) {
            PlayPhoneRing(&g.audio);
        }

        // Clock positional volume
        if (g.audio.clock_playing) {
            float center_dist = sqrtf(px * px + pz * pz);
            float clock_vol = 0.035f / (1.0f + center_dist * 0.15f);
            if (g.player.camera.position.y > 1.8f) clock_vol *= 1.3f;
            SetSoundVolume(g.audio.snd_clock, clock_vol);
        }
    }
    UpdateEVAudio(&g.audio, g.player.moving, g.player.sprinting, g.scene.surface, dt);
    update_npc(&g.gibbons, g.player.camera.position, &g.scene, dt);
    // Gibbons vanishes
    if (!g.gibbons.active && g.gibbons.current_line >= g.gibbons.line_count) {
        g.gibbons.pos.y = -100;
    }
    // Lamp ritual — bedside lamp (slot 0) + floor lamp (slot 4) fade in together
    if (g.interaction_phases[0] == 1) {
        g.interaction_timers[0] -= dt;
        float gt = fminf(1.0f, 1.0f - g.interaction_timers[0] / 1.5f);
        SetPointLightIdx(&g.lighting, 0, -2.5f, 1.2f, -4.8f, gt, gt*0.82f, gt*0.45f, gt*8.0f);
        // Floor lamp between armchairs — warm amber, softer
        SetPointLightIdx(&g.lighting, 4, -5.5f, 1.4f, -1.0f,
                         gt*0.6f, gt*0.45f, gt*0.22f, gt*6.0f);
        if (g.interaction_timers[0] <= 0) g.interaction_phases[0] = 2;
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
        if (g.interaction_timers[2] <= 0) g.interaction_phases[2] = 2;
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
    // Interactions
    if (IsKeyPressed(KEY_E)) {
        for (int i = 0; i < g.scene.object_count; i++) {
            InteractObject *obj = &g.scene.objects[i];
            if (!obj->active || obj->done) continue;
            float dist = Vector3Distance(g.player.camera.position, obj->pos);
            if (dist < obj->radius) {
                Vector3 to = Vector3Normalize(Vector3Subtract(obj->pos, g.player.camera.position));
                Vector3 look = Vector3Normalize(Vector3Subtract(g.player.camera.target, g.player.camera.position));
                if (Vector3DotProduct(to, look) > 0.5f) {
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
                    // Wardrobe — open to see her robe. Yours and hers, side by side.
                    if (strcmp(obj->name, "wardrobe") == 0 && obj->step == 1) {
                        // Slide wardrobe door open — reveal robes inside
                        add_wall(&g.scene, -5.2f, 1.2f, 3.3f, 0.05f, 1.8f, 0.5f, (Color){225,215,200,255});
                        set_last_material(&g.scene, MAT_FABRIC); // her robe
                        add_wall(&g.scene, -5.4f, 1.2f, 3.3f, 0.05f, 1.8f, 0.5f, (Color){50,55,80,255});
                        set_last_material(&g.scene, MAT_FABRIC); // his robe (navy)
                        show_text("Two robes. Different sizes.");
                        obj->done = true;
                        break;
                    }
                    // Wine glass — pick up, examine the lipstick mark
                    if (strcmp(obj->name, "wineglass") == 0 && obj->step == 1) {
                        show_text("Not your shade.");
                        obj->done = true;
                        break;
                    }
                    // Her book — pick up, see boarding pass bookmark
                    if (strcmp(obj->name, "book") == 0 && obj->step == 1) {
                        show_text("Her bookmark. A boarding pass.");
                        obj->done = true;
                        break;
                    }
                    // Bolaño objects — resist interpretation
                    if (strcmp(obj->name, "sock") == 0 && obj->step == 1) {
                        show_text("Not yours.");
                        obj->done = true;
                        break;
                    }
                    if (strcmp(obj->name, "roomservice") == 0 && obj->step == 1) {
                        show_text("Two handwritings. A disagreement about the cheese plate.");
                        obj->done = true;
                        break;
                    }
                    if (strcmp(obj->name, "postcard") == 0 && obj->step == 1) {
                        show_text("Addressed to no one.");
                        obj->done = true;
                        break;
                    }
                    // Phone — her playlist, still queued
                    if (strcmp(obj->name, "phone") == 0 && obj->step == 1) {
                        show_text("47 songs. Her queue. Still counting down.");
                        obj->done = true;
                        break;
                    }
                    // Suitcase — hers, by the door
                    if (strcmp(obj->name, "suitcase") == 0 && obj->step == 1) {
                        // Choice colors interpretation — what did you pack?
                        if (g.backstory[1] == 1)  // "Everything she left behind"
                            show_text("Everything she left. You brought it anyway.");
                        else
                            show_text("Packed for two weeks. Never opened.");
                        obj->done = true;
                        break;
                    }
                    // Bathroom — run the bath (the Chevalier moment)
                    if (strcmp(obj->name, "bathroom") == 0 && obj->step == 1) {
                        // Steam appears on the window — Earth blurs through fog
                        add_wall(&g.scene, -6.8f, 2.0f, -1.0f, 0.1f, 3.0f, 4.0f, (Color){200,210,220,25});
                        add_wall(&g.scene, -6.6f, 2.5f, -0.5f, 0.08f, 2.0f, 3.0f, (Color){200,210,220,15});
                        // Running water — the bath fills
                        SetSoundVolume(g.audio.snd_running_water, 0.04f);
                        PlaySound(g.audio.snd_running_water);
                        show_text("Big enough for two.");
                        obj->done = true;
                        break;
                    }

                    if (strcmp(obj->name, "lamp") == 0 && obj->step == 1) {
                        add_wall(&g.scene, -2.5f, 1.0f, -4.65f, 0.15f, 0.25f, 0.15f, (Color){220,210,185,180});
                        g.interaction_phases[0] = 1;
                        g.interaction_timers[0] = 1.5f;
                    } else if (strcmp(obj->name, "desk") == 0 && obj->step == 1) {
                        add_wall(&g.scene, 5.3f, 0.45f, -1.6f, 0.4f, 0.04f, 0.3f, (Color){55,85,175,255});
                        g.interaction_phases[2] = 1;
                        g.interaction_timers[2] = 1.2f;
                    } else if (strcmp(obj->name, "bed") == 0 && obj->step == 1) {
                        // Pull back the covers — the Chevalier moment
                        add_wall(&g.scene, 0, 0.54f, -4.3f, 2.8f, 0.02f, 1.4f, (Color){245,242,235,255});
                        // Camera lowers — lying down
                        g.interaction_phases[3] = 1;
                        g.interaction_timers[3] = 3.0f;
                        // Play THE piece — once, never repeats
                        PlayBedRitual(&g.audio);
                        // Agency surrender begins
                        g.player.control_mult = 0.3f;
                    } else if (strcmp(obj->name, "champagne") == 0 && obj->step == 1) {
                        add_cone(&g.scene, -3.1f, 0.39f, 3.5f, 0.06f, 0.08f, (Color){210,210,215,200});
                        add_cylinder(&g.scene, -3.1f, 0.44f, 3.5f, 0.02f, 0.08f, (Color){210,210,215,200});
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

                        // ── WRONGNESS — Barton Fink layer ──
                        // After each task, the room shifts. Not horror. Just... wrong.
                        // The wrongness IS the character's psychological state.
                        if (g.tasks_done == 1) {
                            // Photograph shifts slightly — did it move?
                            for (int wi = 0; wi < g.scene.wall_count; wi++) {
                                if (fabsf(g.scene.walls[wi].pos.y - 0.64f) < 0.05f &&
                                    g.scene.walls[wi].size.y < 0.02f &&
                                    g.scene.walls[wi].size.x < 0.25f &&
                                    g.scene.walls[wi].color.r > 230) {
                                    g.scene.walls[wi].pos.x += 0.03f;
                                    g.scene.walls[wi].pos.z += 0.02f;
                                    break;
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
                                    w->pos.x += 1.5f; // Earth glow pool shifts
                                    w->pos.z -= 0.8f;
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
