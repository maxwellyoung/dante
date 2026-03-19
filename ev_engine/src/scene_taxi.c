// scene_taxi.c — STATE_CAR, STATE_DRIVING, STATE_RETURN_TAXI
#include "game_ctx.h"
#include <math.h>
#include <string.h>

extern GameCtx g;

void set_exposure(float exp);
void show_text(const char *text);
void hide_text(void);
void show_choice(const char *question, const char *a, const char *b);
int poll_choice(void);
void transition_to(GameState s);
void hard_cut_to(GameState s);

void taxi_load(void) {
    build_taxi_ride(&g.scene);
    init_player(&g.player, g.scene.spawn);
    g.player.camera.target = (Vector3){0, 0.9f, -1.0f};
    for (int i = 0; i < MAX_RAIN; i++) {
        g.rain[i].x = (float)GetRandomValue(0, RENDER_W);
        g.rain[i].y = (float)GetRandomValue(-RENDER_H, 0);
        g.rain[i].speed = 80.0f + (float)GetRandomValue(0, 120);
        g.rain[i].len = 3.0f + (float)GetRandomValue(0, 8);
    }
    StopAmbient(&g.audio);
    StopTitleMusic(&g.audio);
    PlayCityAmbient(&g.audio);
    SetCityAmbientVolume(&g.audio, 0.04f);
    PlayTaxiRadio(&g.audio);  // warm song on the radio — cuts off at hyperspace

    // THE SECOND TICKET — on the seat beside you. "2 guests."
    // The player's first interaction: picking it up. Pocketing evidence.
    add_object(&g.scene, 0.6f, 0.42f, 0.23f, "ticket", (Color){245,242,235,255}, 1);
    SetSceneLighting(&g.lighting, LightingPreset_Taxi());
    set_exposure(0.05f);
    SetPostFXGrain(&g.postfx, 0.45f);
    SetPostFXCA(&g.postfx, 1.5f);
    SetPostFXVignette(&g.postfx, 1.0f);
    SetPostFXWarmth(&g.postfx, 0.1f);
}

void taxi_update(float dt) {
    // Mouse look (yaw/pitch only, no position change)
    Vector2 md = GetMouseDelta();
    float car_yaw = -md.x * ev_mouse_sens;
    float car_pitch = -md.y * ev_mouse_sens;
    Vector3 car_fwd = Vector3Normalize(Vector3Subtract(g.player.camera.target, g.player.camera.position));
    Vector3 car_right = Vector3Normalize(Vector3CrossProduct(car_fwd, g.player.camera.up));
    Matrix car_ym = MatrixRotateY(car_yaw);
    car_fwd = Vector3Transform(car_fwd, car_ym);
    float car_cp = asinf(car_fwd.y);
    float car_np = car_cp + car_pitch;
    if (car_np > 0.8f) car_pitch = 0.8f - car_cp;
    if (car_np < -0.6f) car_pitch = -0.6f - car_cp;
    Matrix car_pm = MatrixRotate(car_right, car_pitch);
    car_fwd = Vector3Transform(car_fwd, car_pm);
    g.player.camera.target = Vector3Add(g.player.camera.position, car_fwd);

    // Auto-scroll city walls
    float taxi_speed = 8.0f;
    if (g.state_time > 10.0f) {
        float t = g.state_time - 10.0f;
        taxi_speed = 8.0f + t * t * 2.0f;
    }
    for (int i = g.scene.static_wall_count; i < g.scene.wall_count; i++) {
        g.scene.walls[i].pos.z += taxi_speed * dt;
        if (g.scene.walls[i].pos.z > 10.0f) {
            g.scene.walls[i].pos.z -= 220.0f;
        }
    }

    // Ticket interaction — E to pocket it
    if (IsKeyPressed(KEY_E)) {
        for (int i = 0; i < g.scene.object_count; i++) {
            InteractObject *obj = &g.scene.objects[i];
            if (!obj->active || obj->done) continue;
            if (strcmp(obj->name, "ticket") == 0) {
                obj->done = true;
                show_text("Booking confirmation. 2 guests.");
                // Remove her ticket from the seat (pocketed — yours stays)
                for (int wi = 0; wi < g.scene.wall_count; wi++) {
                    Wall *w = &g.scene.walls[wi];
                    // Her ticket (x=0.62, z=0.25, slightly rotated)
                    if (w->pos.y > 0.40f && w->pos.y < 0.44f &&
                        w->pos.x > 0.55f && w->pos.x < 0.70f &&
                        w->pos.z > 0.15f && w->pos.z < 0.35f &&
                        w->color.r > 230 && w->size.y < 0.01f) {
                        w->active = false;  // pocketed
                    }
                }
                break;
            }
        }
    }
    if (g.state_time > 4.5f && g.scene.objects[0].done && g.vig_text != NULL) {
        hide_text();
    }

    // Radio fades as hyperspace approaches
    if (g.state_time > 10.0f && g.audio.taxi_radio_playing) {
        float fade = (g.state_time - 10.0f) / 3.5f;
        SetSoundVolume(g.audio.snd_taxi_radio, 0.025f * fmaxf(0, 1.0f - fade));
    }

    // Post-FX ramp
    if (g.state_time > 10.0f) {
        float ramp = (g.state_time - 10.0f) / 3.5f;
        if (ramp > 1.0f) ramp = 1.0f;
        SetPostFXCA(&g.postfx, 2.5f + ramp * 8.0f);
        SetPostFXGrain(&g.postfx, 0.5f + ramp * 0.3f);
        g.player.camera.fovy = 70.0f + ramp * 20.0f;
    }

    // ── Taxi dialogue — first play has choices, replay is silent ──
    {
        float t = g.state_time;
        bool replay = g.backstory_count > 3;

        if (!replay) {
            // ── FIRST PLAY: phase-driven dialogue ──
            // Each beat: show driver line → pause → choice → next beat
            // Phase timer tracks time within each beat (reset on phase change)

            // Beat 1: Location stamp
            if (g.backstory_phase == 0 && t > 1.5f) {
                show_text("Auckland. 2:47 AM.");
                g.backstory_phase = 1;
                g.beat_timer = 0;
            }
            if (g.backstory_phase == 1) {
                g.beat_timer += dt;
                if (g.beat_timer > 2.5f) {
                    hide_text();
                    g.backstory_phase = 2;
                    g.beat_timer = 0;
                }
            }

            // Beat 2: "The tower, yeah?"
            if (g.backstory_phase == 2) {
                g.beat_timer += dt;
                if (g.beat_timer > 1.0f && g.beat_timer < 1.1f) {
                    show_text("The tower, yeah?");
                }
                if (g.beat_timer > 3.0f) {
                    hide_text();
                    show_choice("The tower, yeah?", "She wanted to see it.", "I have a reservation.");
                    g.backstory_phase = 3;
                }
            }
            if (g.backstory_phase == 3 && poll_choice() >= 0) {
                g.backstory_phase = 4;
                g.beat_timer = 0;
            }

            // Beat 3: "Three hours, in and out."
            if (g.backstory_phase == 4) {
                g.beat_timer += dt;
                if (g.beat_timer > 1.0f && g.beat_timer < 1.1f) {
                    show_text("They put a hotel up there. Three hours, in and out.");
                }
                if (g.beat_timer > 3.0f) {
                    hide_text();
                    show_choice("Three hours, in and out.", "We booked it months ago.", "So I hear.");
                    g.backstory_phase = 5;
                }
            }
            if (g.backstory_phase == 5 && poll_choice() >= 0) {
                g.backstory_phase = 6;
                g.beat_timer = 0;
            }

            // Beat 4: "At this hour..."
            if (g.backstory_phase == 6) {
                g.beat_timer += dt;
                if (g.beat_timer > 1.0f && g.beat_timer < 1.1f) {
                    show_text("At this hour, you've got the whole city to yourself.");
                }
                if (g.beat_timer > 3.5f) {
                    hide_text();
                    g.player.camera.fovy = 70.0f;
                    SetPostFXCA(&g.postfx, 2.5f);
                    hard_cut_to(STATE_HYPERSPACE);
                }
            }

            // Skip (Enter) — after first choice is done
            if (IsKeyPressed(KEY_ENTER) && !g.choice_active && g.backstory_phase >= 4) {
                hide_text();
                g.player.camera.fovy = 70.0f;
                SetPostFXCA(&g.postfx, 2.5f);
                hard_cut_to(STATE_HYPERSPACE);
            }
        } else {
            // ── SECOND PLAY: no choices, just the ride. You know where you're going. ──
            if (t > 1.5f && t < 2.0f) show_text("Auckland. 2:47 AM.");
            if (t > 4.0f && t < 4.5f) hide_text();

            // The driver says nothing. The radio plays. You watch the city.
            if (t > 7.0f && t < 7.5f) show_text("Same city. Same hour.");
            if (t > 10.0f && t < 10.5f) hide_text();
            if (t > 11.0f && t < 11.5f) show_text("You know the way.");
            if (t > 13.5f) {
                hide_text();
                g.player.camera.fovy = 70.0f;
                SetPostFXCA(&g.postfx, 2.5f);
                hard_cut_to(STATE_HYPERSPACE);
            }
            if (IsKeyPressed(KEY_ENTER) && t > 5) {
                hide_text();
                g.player.camera.fovy = 70.0f;
                SetPostFXCA(&g.postfx, 2.5f);
                hard_cut_to(STATE_HYPERSPACE);
            }
        }
    }
}

void driving_update(float dt) {
    (void)dt;
    // Arrival — taxi stopped, mouse look only
    Vector2 md2 = GetMouseDelta();
    float drv_yaw = -md2.x * ev_mouse_sens;
    float drv_pitch = -md2.y * ev_mouse_sens;
    Vector3 drv_fwd = Vector3Normalize(Vector3Subtract(g.player.camera.target, g.player.camera.position));
    Vector3 drv_right = Vector3Normalize(Vector3CrossProduct(drv_fwd, g.player.camera.up));
    Matrix drv_ym = MatrixRotateY(drv_yaw);
    drv_fwd = Vector3Transform(drv_fwd, drv_ym);
    float drv_cp = asinf(drv_fwd.y);
    float drv_np = drv_cp + drv_pitch;
    if (drv_np > 0.8f) drv_pitch = 0.8f - drv_cp;
    if (drv_np < -0.6f) drv_pitch = -0.6f - drv_cp;
    Matrix drv_pm = MatrixRotate(drv_right, drv_pitch);
    drv_fwd = Vector3Transform(drv_fwd, drv_pm);
    g.player.camera.target = Vector3Add(g.player.camera.position, drv_fwd);

    if (g.state_time > 7.0f) transition_to(STATE_HOTEL_EXT);
    if (IsKeyPressed(KEY_ENTER) && g.state_time > 2) transition_to(STATE_HOTEL_EXT);
}

void return_taxi_load(void) {
    build_return_taxi(&g.scene);
    init_player(&g.player, g.scene.spawn);
    g.player.camera.target = (Vector3){0, 0.9f, -1.0f};
    g.player.control_mult = 1.0f;
    StopAmbient(&g.audio);
    StopClockAmbient(&g.audio);
    StopWindAmbient(&g.audio);
    PlayCityAmbient(&g.audio);
    SetCityAmbientVolume(&g.audio, 0.02f);
    SetSceneLighting(&g.lighting, LightingPreset_ReturnTaxi());
    set_exposure(0.05f);
    SetPostFXWarmth(&g.postfx, 0.4f);
    SetPostFXGrain(&g.postfx, 0.3f);
    SetPostFXSaturation(&g.postfx, 1.05f);
    SetPostFXCA(&g.postfx, 1.5f);
    SetPostFXVignette(&g.postfx, 1.0f);
    SetPostFXContrast(&g.postfx, 1.0f);

    // Gibbons — in the passenger seat, going home.
    // Brief eye contact. He adjusts his tie. He knew the whole time.
    {
        Vector3 taxi_wps[] = { {-0.5f, 0.8f, -0.3f} };
        init_npc(&g.gibbons, (Vector3){-0.5f, 0.8f, -0.3f}, taxi_wps, 1, 0.3f, 2.0f);
        g.gibbons.behavior = NPC_SITTING;
        static const char *taxi_lines[] = {
            "Good hotel?",
        };
        npc_set_dialogue(&g.gibbons, taxi_lines, 1, 8.0f);
    }
}

void return_taxi_update(float dt) {
    update_npc(&g.gibbons, g.player.camera.position, &g.scene, dt);
    // Mouse look only
    Vector2 md_r = GetMouseDelta();
    float ry = -md_r.x * ev_mouse_sens;
    float rp = -md_r.y * ev_mouse_sens;
    Vector3 rfwd = Vector3Normalize(Vector3Subtract(g.player.camera.target, g.player.camera.position));
    Vector3 rright = Vector3Normalize(Vector3CrossProduct(rfwd, g.player.camera.up));
    rfwd = Vector3Transform(rfwd, MatrixRotateY(ry));
    float rcp = asinf(rfwd.y);
    float rnp = rcp + rp;
    if (rnp > 0.8f) rp = 0.8f - rcp;
    if (rnp < -0.6f) rp = -0.6f - rcp;
    rfwd = Vector3Transform(rfwd, MatrixRotate(rright, rp));
    g.player.camera.target = Vector3Add(g.player.camera.position, rfwd);

    // City scrolls — DECELERATING
    float rtaxi_speed = 6.0f;
    if (g.state_time > 8.0f) {
        float t2 = g.state_time - 8.0f;
        rtaxi_speed = 6.0f - t2 * t2 * 0.3f;
        if (rtaxi_speed < 0.5f) rtaxi_speed = 0.5f;
    }
    for (int i = g.scene.static_wall_count; i < g.scene.wall_count; i++) {
        g.scene.walls[i].pos.z += rtaxi_speed * dt;
        if (g.scene.walls[i].pos.z > 10.0f)
            g.scene.walls[i].pos.z -= 220.0f;
    }

    // Dialogue
    if (g.state_time > 3.0f && g.state_time < 3.5f) show_text("Auckland. 5:52 AM.");
    if (g.state_time > 6.0f && g.state_time < 6.5f) hide_text();
    // "Good hotel?" — Gibbons' NPC dialogue. After it shows, offer the final choice.
    // The last words of the game. The simplest question deserves the simplest answer.
    if (g.gibbons.line_showing && g.gibbons.current_line == 0) {
        // Gibbons is asking. Wait for his line to finish, then offer response.
    }
    if (!g.gibbons.line_showing && g.gibbons.current_line >= 1 && g.backstory_phase < 10) {
        show_choice("Good hotel?", "Yeah.", "...");
        g.backstory_phase = 10;
    }
    if (g.backstory_phase == 10 && poll_choice() >= 0) {
        g.backstory_phase = 11;
        hide_text();
    }

    // Saturation drain
    {
        float drain = fminf(1.0f, g.state_time / 14.0f);
        SetPostFXSaturation(&g.postfx, 1.05f - drain * 0.25f);
    }

    // Fade to black
    if (g.state_time > 14.0f) {
        float fo = (g.state_time - 14.0f) / 3.0f;
        SetMasterVolume(fmaxf(0, 1.0f - fo));
        g.fade_alpha = fminf(1.0f, fo);
        g.fade_target = 1.0f;
        if (g.state_time > 17.0f) {
            // Return to title — the game loops. Second playthrough,
            // the player sees the twos everywhere.
            SetMasterVolume(1.0f);
            hard_cut_to(STATE_TITLE);
        }
    }
}
