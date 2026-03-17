// player.c — Full movement system (tightened)
// Quake air strafing, bunny hopping, wall running, ledge mantling,
// crouch jumping, dash, coyote time, jump buffering, momentum slides
#include "player.h"
#include "raymath.h"
#include <math.h>
#include <stddef.h>

static PhysicsConfig phys;

// ── Collision result — tracks wall contact for wall running ─────────
typedef struct {
    bool hit;
    Vector3 normal;     // outward normal of the wall face we hit
    Wall *wall;
    float wall_top;     // top of the wall we hit
    float wall_bot;     // bottom of the wall we hit
} CollisionInfo;

void init_player(Player *p, Vector3 pos) {
    phys = physics_default();
    p->camera.position = pos;
    p->camera.target = (Vector3){pos.x, pos.y, pos.z - 1};
    p->camera.up = (Vector3){0, 1, 0};
    p->camera.fovy = phys.fov_walk;
    p->camera.projection = CAMERA_PERSPECTIVE;
    p->vel = (Vector3){0, 0, 0};
    p->vy = 0;
    p->velocity = (Vector3){0, 0, 0};
    p->bob_timer = 0;
    p->moving = false;
    p->sprinting = false;
    p->sprint_stamina = 1.0f;
    p->speed_current = 0;
    p->fov_current = phys.fov_walk;
    p->tilt_current = 0;
    p->kick_pitch = 0;
    p->kick_yaw = 0;
    p->kick_decay = 0;
    p->grounded = true;
    p->land_timer = 0;
    p->ground_y = 0.0f;
    p->ground_normal_y = 1.0f;
    p->coyote_timer = 0;
    p->jump_buffer = 0;
    p->sliding = false;
    p->slide_speed = 0;
    p->slide_dir = (Vector3){0, 0, 0};
    p->wall_running = false;
    p->wall_run_timer = 0;
    p->wall_normal = (Vector3){0, 0, 0};
    p->wall_run_side = 0;
    p->mantling = false;
    p->mantle_target_y = 0;
    p->mantle_timer = 0;
    p->bhop_timer = 0;
    p->dashing = false;
    p->dash_timer = 0;
    p->dash_cooldown_timer = 0;
    p->dash_dir = (Vector3){0, 0, 0};
    p->peak_speed = 0;
    p->control_mult = 1.0f;
    p->idle_time = 0;
    p->noclip = false;
}

void kick_camera(Player *p, float pitch, float yaw) {
    p->kick_pitch = pitch;
    p->kick_yaw = yaw;
    p->kick_decay = 1.0f;
}

// ── Quake-style acceleration ────────────────────────────────────────
// Project velocity onto wish direction. Only add speed if below cap
// along that axis. This is what enables air strafing: the cap is low
// in air, so perpendicular strafing always adds speed while forward
// projection quickly saturates.
static void accelerate(Vector3 *vel, Vector3 wish_dir, float wish_speed,
                       float accel, float dt) {
    float current_speed = vel->x * wish_dir.x + vel->z * wish_dir.z;
    float add_speed = wish_speed - current_speed;
    if (add_speed <= 0) return;
    float accel_speed = accel * wish_speed * dt;
    if (accel_speed > add_speed) accel_speed = add_speed;
    vel->x += accel_speed * wish_dir.x;
    vel->z += accel_speed * wish_dir.z;
}

static void apply_friction(Vector3 *vel, float friction, float dt) {
    float speed = sqrtf(vel->x * vel->x + vel->z * vel->z);
    if (speed < 0.1f) { vel->x = 0; vel->z = 0; return; }
    float new_speed = speed - speed * friction * dt;
    if (new_speed < 0) new_speed = 0;
    float scale = new_speed / speed;
    vel->x *= scale;
    vel->z *= scale;
}

static float hspeed(Player *p) {
    return sqrtf(p->vel.x * p->vel.x + p->vel.z * p->vel.z);
}

// ── Collision — two-pass for corners ────────────────────────────────
// Returns info about the most significant wall hit (tallest side wall)
static CollisionInfo collide_and_slide(Vector3 *new_pos, Vector3 *vel,
                                       Scene *scene, float ground_y, float pr,
                                       float eye_y) {
    CollisionInfo best = {false, {0,0,0}, NULL, 0, 0};

    // Two passes: first pass resolves, second catches corners
    for (int pass = 0; pass < 2; pass++) {
        for (int i = 0; i < scene->wall_count; i++) {
            Wall *w = &scene->walls[i];
            if (!w->active) continue;

            float wt = w->pos.y + w->size.y / 2;
            float wb = w->pos.y - w->size.y / 2;

            // Skip steppable surfaces
            if (wt <= ground_y + phys.step_height && wt > ground_y - 0.1f)
                continue;

            if (new_pos->x > w->pos.x - w->size.x/2 - pr &&
                new_pos->x < w->pos.x + w->size.x/2 + pr &&
                new_pos->z > w->pos.z - w->size.z/2 - pr &&
                new_pos->z < w->pos.z + w->size.z/2 + pr &&
                new_pos->y > wb && new_pos->y < wt) {

                // Penetration depth on each axis
                float pen_l = (w->pos.x - w->size.x/2 - pr) - new_pos->x;
                float pen_r = new_pos->x - (w->pos.x + w->size.x/2 + pr);
                float pen_f = (w->pos.z - w->size.z/2 - pr) - new_pos->z;
                float pen_b = new_pos->z - (w->pos.z + w->size.z/2 + pr);

                // Smallest penetration per axis
                float px = (fabsf(pen_l) < fabsf(pen_r)) ? pen_l : -pen_r;
                float pz = (fabsf(pen_f) < fabsf(pen_b)) ? pen_f : -pen_b;

                if (fabsf(px) < fabsf(pz)) {
                    float nx = (px > 0) ? 1.0f : -1.0f;
                    new_pos->x += px;
                    if (vel->x * nx < 0) vel->x = 0;
                    // Track as best if it's a side wall at body height
                    // (wall extends above feet and below eyes — body-height contact)
                    if (wt > ground_y + 1.0f && wb < eye_y) {
                        best.hit = true;
                        best.normal = (Vector3){nx, 0, 0};
                        best.wall = w;
                        best.wall_top = wt;
                        best.wall_bot = wb;
                    }
                } else {
                    float nz = (pz > 0) ? 1.0f : -1.0f;
                    new_pos->z += pz;
                    if (vel->z * nz < 0) vel->z = 0;
                    if (wt > ground_y + 1.0f && wb < eye_y) {
                        best.hit = true;
                        best.normal = (Vector3){0, 0, nz};
                        best.wall = w;
                        best.wall_top = wt;
                        best.wall_bot = wb;
                    }
                }
            }
        }
    }
    return best;
}

// ── Wall proximity check — verify we're still touching the wall ─────
static bool touching_wall(Player *p, Scene *scene) {
    if (!p->wall_running) return false;
    Vector3 pos = p->camera.position;
    float pr = phys.player_radius + 0.1f;  // slightly larger for sticky check

    for (int i = 0; i < scene->wall_count; i++) {
        Wall *w = &scene->walls[i];
        if (!w->active) continue;
        float wt = w->pos.y + w->size.y / 2;
        float wb = w->pos.y - w->size.y / 2;
        if (wt < p->ground_y + 1.0f) continue;  // too short
        if (wb > pos.y) continue;                 // above us

        if (pos.x > w->pos.x - w->size.x/2 - pr &&
            pos.x < w->pos.x + w->size.x/2 + pr &&
            pos.z > w->pos.z - w->size.z/2 - pr &&
            pos.z < w->pos.z + w->size.z/2 + pr) {
            return true;
        }
    }
    return false;
}

// ── Ledge detection ─────────────────────────────────────────────────
static bool find_mantle_ledge(Player *p, Scene *scene, Vector3 fwd,
                              float *out_ledge_y) {
    Vector3 pos = p->camera.position;
    float pr = phys.player_radius;
    float cx = pos.x + fwd.x * 0.8f;
    float cz = pos.z + fwd.z * 0.8f;
    float best = -999.0f;

    for (int i = 0; i < scene->wall_count; i++) {
        Wall *w = &scene->walls[i];
        if (!w->active || w->shape != SHAPE_CUBE) continue;
        float wt = w->pos.y + w->size.y / 2;
        if (wt > pos.y && wt < pos.y + phys.mantle_reach && wt > best &&
            cx > w->pos.x - w->size.x/2 - pr &&
            cx < w->pos.x + w->size.x/2 + pr &&
            cz > w->pos.z - w->size.z/2 - pr &&
            cz < w->pos.z + w->size.z/2 + pr) {
            best = wt;
        }
    }
    if (best > -999.0f) { *out_ledge_y = best + phys.eye_height; return true; }
    return false;
}

// ─────────────────────────────────────────────────────────────────────

void update_player(Player *p, Scene *scene, float dt) {
    // ── Mantle in progress ──────────────────────────────────────────
    if (p->mantling) {
        p->mantle_timer += dt * phys.mantle_speed;
        if (p->mantle_timer >= 1.0f) {
            p->mantling = false;
            p->camera.position.y = p->mantle_target_y;
            p->ground_y = p->mantle_target_y - phys.eye_height;
            p->grounded = true;
            p->vy = 0;
            Vector3 fwd = Vector3Normalize(Vector3Subtract(p->camera.target, p->camera.position));
            Vector3 ff = Vector3Normalize((Vector3){fwd.x, 0, fwd.z});
            p->vel.x = ff.x * phys.mantle_forward;
            p->vel.z = ff.z * phys.mantle_forward;
        } else {
            // Quadratic ease-out: fast start, gentle finish
            float t = p->mantle_timer;
            float ease = 1.0f - (1.0f - t) * (1.0f - t);
            p->camera.position.y += (p->mantle_target_y - p->camera.position.y) *
                                     fminf(1.0f, ease * phys.mantle_speed * dt * 4.0f);
            p->vel.x *= 0.85f;
            p->vel.z *= 0.85f;
            p->vy = 0;
        }
        // Update camera target to follow
        Vector3 look = Vector3Normalize(Vector3Subtract(p->camera.target, p->camera.position));
        p->camera.target = Vector3Add(p->camera.position, look);
        // Update speed_current for shader
        p->speed_current = hspeed(p);
        return;
    }

    // ── Mouse look ──────────────────────────────────────────────────
    Vector2 mouse_delta = GetMouseDelta();
    float sens = MOUSE_SENS * p->control_mult;
    float yaw_delta = -mouse_delta.x * sens;
    float pitch_delta = -mouse_delta.y * sens;

    if (p->kick_decay > 0) {
        float kt = p->kick_decay * p->kick_decay;
        pitch_delta += p->kick_pitch * kt * dt * 30.0f;
        yaw_delta += p->kick_yaw * kt * dt * 30.0f;
        p->kick_decay -= dt * 4.0f;
        if (p->kick_decay < 0) p->kick_decay = 0;
    }

    Vector3 forward = Vector3Normalize(Vector3Subtract(p->camera.target, p->camera.position));
    Vector3 right = Vector3Normalize(Vector3CrossProduct(forward, p->camera.up));

    forward = Vector3Transform(forward, MatrixRotateY(yaw_delta));
    float cur_pitch = asinf(forward.y);
    float new_pitch = cur_pitch + pitch_delta;
    if (new_pitch > 1.3f) pitch_delta = 1.3f - cur_pitch;
    if (new_pitch < -1.3f) pitch_delta = -1.3f - cur_pitch;
    forward = Vector3Transform(forward, MatrixRotate(right, pitch_delta));
    p->camera.target = Vector3Add(p->camera.position, forward);

    // ── Sprint stamina ──────────────────────────────────────────────
    p->sprinting = IsKeyDown(KEY_LEFT_SHIFT) && p->sprint_stamina > phys.stamina_min_to_sprint;
    if (p->sprinting && p->moving) {
        p->sprint_stamina -= dt * phys.stamina_drain;
        if (p->sprint_stamina < 0) p->sprint_stamina = 0;
    } else {
        p->sprint_stamina += dt * phys.stamina_regen;
        if (p->sprint_stamina > 1) p->sprint_stamina = 1;
    }

    // ── Input ───────────────────────────────────────────────────────
    Vector3 flat_fwd = Vector3Normalize((Vector3){forward.x, 0, forward.z});
    Vector3 flat_right = Vector3Normalize((Vector3){right.x, 0, right.z});

    Vector3 wish_dir = {0, 0, 0};
    float strafe = 0;
    p->moving = false;
    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))   { wish_dir = Vector3Add(wish_dir, p->noclip ? forward : flat_fwd); p->moving = true; }
    if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))  { wish_dir = Vector3Subtract(wish_dir, p->noclip ? forward : flat_fwd); p->moving = true; }
    if (IsKeyDown(KEY_A)) { wish_dir = Vector3Subtract(wish_dir, flat_right); strafe = -1; p->moving = true; }
    if (IsKeyDown(KEY_D)) { wish_dir = Vector3Add(wish_dir, flat_right); strafe = 1; p->moving = true; }
    if (p->noclip) {
        if (IsKeyDown(KEY_SPACE))        { wish_dir.y += 1; p->moving = true; }
        if (IsKeyDown(KEY_LEFT_CONTROL)) { wish_dir.y -= 1; p->moving = true; }
    }

    float wish_len = Vector3Length(wish_dir);
    if (wish_len > 0.001f) wish_dir = Vector3Scale(wish_dir, 1.0f / wish_len);

    // ── Dash (Q) ────────────────────────────────────────────────────
    p->dash_cooldown_timer -= dt;
    if (p->dash_cooldown_timer < 0) p->dash_cooldown_timer = 0;

    // Can't dash during mantle or wall run (already handled: mantle returns early)
    if (IsKeyPressed(KEY_Q) && !p->dashing && p->dash_cooldown_timer <= 0
        && !p->noclip && !p->wall_running) {
        p->dashing = true;
        p->dash_timer = phys.dash_duration;
        p->dash_cooldown_timer = phys.dash_cooldown;
        p->dash_dir = (wish_len > 0.001f) ? wish_dir : flat_fwd;
        p->sliding = false;
    }

    if (p->dashing) {
        p->dash_timer -= dt;
        if (p->dash_timer <= 0) {
            p->dashing = false;
            // Exit momentum scales with how much dash was left (early cancel = less)
            float exit_frac = 0.4f;
            p->vel.x = p->dash_dir.x * phys.dash_speed * exit_frac;
            p->vel.z = p->dash_dir.z * phys.dash_speed * exit_frac;
        } else {
            p->vel.x = p->dash_dir.x * phys.dash_speed;
            p->vel.z = p->dash_dir.z * phys.dash_speed;
            p->vy *= 0.5f;  // dampen vertical during dash (don't freeze — feels wrong)
        }
    }

    // ── Slide ───────────────────────────────────────────────────────
    bool want_slide = IsKeyDown(KEY_LEFT_CONTROL) && !p->noclip && !p->dashing;
    float cur_hspeed = hspeed(p);

    if (want_slide && p->moving && p->grounded && !p->sliding) {
        // Require minimum speed to initiate slide (no standing slides)
        if (cur_hspeed > phys.walk_speed * 0.6f) {
            p->sliding = true;
            p->slide_speed = cur_hspeed;
            if (cur_hspeed > 0.5f) {
                p->slide_dir = (Vector3){p->vel.x / cur_hspeed, 0, p->vel.z / cur_hspeed};
            } else {
                p->slide_dir = wish_dir;
            }
        }
    }
    // End slide
    if (p->sliding && (!want_slide || !p->grounded || cur_hspeed < 0.5f)) {
        p->sliding = false;
        p->slide_speed = 0;
    }

    // Landing slide: hold Ctrl when landing at speed
    if (want_slide && p->grounded && !p->sliding && p->land_timer > 0 &&
        cur_hspeed > phys.walk_speed) {
        p->sliding = true;
        p->slide_speed = cur_hspeed;
        if (cur_hspeed > 0.5f) {
            p->slide_dir = (Vector3){p->vel.x / cur_hspeed, 0, p->vel.z / cur_hspeed};
        }
        p->land_timer *= 0.5f;
    }

    // ── Target speed ────────────────────────────────────────────────
    float wish_speed = p->sprinting ? phys.sprint_speed : phys.walk_speed;
    if (p->sliding) wish_speed *= phys.slide_speed_mult;
    wish_speed *= p->control_mult;  // agency dial

    // ── Movement physics ────────────────────────────────────────────
    bool just_jumped = false;

    if (p->dashing) {
        // Dash overrides — velocity set above
    } else if (p->noclip) {
        p->vel = Vector3Scale(wish_dir, wish_speed);
    } else if (p->wall_running) {
        // Wall run: move along wall, reduced friction
        Vector3 along = {-p->wall_normal.z, 0, p->wall_normal.x};
        float dot = along.x * p->vel.x + along.z * p->vel.z;
        if (dot < 0) { along.x = -along.x; along.z = -along.z; }
        accelerate(&p->vel, along, wish_speed, phys.ground_accel * 0.8f, dt);
        apply_friction(&p->vel, phys.ground_friction * 0.3f, dt);
    } else if (p->grounded) {
        // Ground movement
        // Bunny hop: if we JUST landed this frame AND we're about to jump,
        // skip friction entirely. This preserves air speed through the landing.
        bool bhop_skip = (p->bhop_timer >= 0 && p->bhop_timer < phys.bhop_friction_window
                          && p->jump_buffer > 0);
        if (!bhop_skip) {
            float friction = p->sliding ? phys.slide_friction : phys.ground_friction;
            apply_friction(&p->vel, friction, dt);
        }

        if (p->sliding) {
            accelerate(&p->vel, p->slide_dir, wish_speed, phys.ground_accel * 0.3f, dt);
            // Perpendicular steering
            if (p->moving) {
                Vector3 steer = wish_dir;
                float d = steer.x * p->slide_dir.x + steer.z * p->slide_dir.z;
                steer.x -= d * p->slide_dir.x;
                steer.z -= d * p->slide_dir.z;
                float slen = sqrtf(steer.x*steer.x + steer.z*steer.z);
                if (slen > 0.01f) {
                    steer.x /= slen; steer.z /= slen;
                    accelerate(&p->vel, steer, wish_speed * 0.3f, phys.ground_accel * 0.5f, dt);
                }
            }
        } else if (p->moving) {
            accelerate(&p->vel, wish_dir, wish_speed, phys.ground_accel, dt);
        }

        // Bhop speed cap
        float hs = hspeed(p);
        if (hs > phys.bhop_speed_cap) {
            float s = phys.bhop_speed_cap / hs;
            p->vel.x *= s;
            p->vel.z *= s;
        }
    } else {
        // Air: low cap + high accel = air strafing
        apply_friction(&p->vel, phys.air_friction, dt);
        if (p->moving) {
            accelerate(&p->vel, wish_dir, phys.air_speed_cap, phys.air_accel, dt);
        }
    }

    // Speed for camera effects
    p->speed_current = hspeed(p);
    p->peak_speed += (p->speed_current - p->peak_speed) * fminf(1.0f, 3.0f * dt);

    // ── Head bob ────────────────────────────────────────────────────
    if ((p->grounded || p->wall_running) && p->speed_current > 0.3f) {
        float rate = p->sliding ? phys.bob_slide_rate :
                    (p->sprinting ? phys.bob_sprint_rate : phys.bob_walk_rate);
        p->bob_timer += dt * rate;
    }

    // ── FOV ─────────────────────────────────────────────────────────
    float target_fov;
    if (p->dashing) {
        target_fov = phys.fov_slide + 5.0f;  // dash punches FOV wider than slide
    } else if (p->sliding) {
        target_fov = phys.fov_slide;
    } else if (p->sprinting && p->moving) {
        target_fov = phys.fov_sprint;
    } else {
        target_fov = phys.fov_walk;
    }
    // Speed-reactive: widen above sprint speed
    float over = p->speed_current - phys.sprint_speed;
    if (over > 0) {
        target_fov += over * phys.speed_fov_scale;
        if (target_fov > phys.speed_fov_max) target_fov = phys.speed_fov_max;
    }
    // Faster lerp when FOV is increasing (snap into speed), slower when decreasing (ease out)
    float fov_rate = (target_fov > p->fov_current) ? phys.fov_lerp_speed * 1.5f : phys.fov_lerp_speed;
    p->fov_current += (target_fov - p->fov_current) * fminf(1.0f, fov_rate * dt);
    p->camera.fovy = p->fov_current;

    // ── Tilt ────────────────────────────────────────────────────────
    float tilt_mult = p->sliding ? phys.tilt_slide :
                     (p->sprinting ? phys.tilt_sprint : phys.tilt_walk);
    float target_tilt = strafe * tilt_mult;
    if (p->wall_running) target_tilt = p->wall_run_side * 12.0f;
    if (p->dashing) target_tilt += (strafe != 0 ? strafe : 0) * 5.0f;  // extra dash lean
    if (!p->moving && !p->wall_running && !p->dashing) target_tilt = 0;
    p->tilt_current += (target_tilt - p->tilt_current) * fminf(1.0f, phys.tilt_lerp_speed * dt);

    // ── Position ────────────────────────────────────────────────────
    Vector3 new_pos = p->camera.position;
    new_pos.x += p->vel.x * dt;
    new_pos.z += p->vel.z * dt;

    // ── Collision & physics ─────────────────────────────────────────
    if (!p->noclip) {
        float eye_y = p->camera.position.y;
        CollisionInfo col = collide_and_slide(&new_pos, &p->vel, scene,
                                              p->ground_y, phys.player_radius, eye_y);

        // ── Wall running ────────────────────────────────────────────
        // Start: airborne + fast + side wall contact + holding forward
        if (col.hit && !p->grounded && !p->wall_running && !p->dashing
            && p->speed_current > phys.wall_run_speed
            && (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))) {
            float side = flat_right.x * col.normal.x + flat_right.z * col.normal.z;
            p->wall_running = true;
            p->wall_run_timer = phys.wall_run_duration;
            p->wall_normal = col.normal;
            p->wall_run_side = (side > 0) ? -1.0f : 1.0f;
            p->vy = fmaxf(p->vy, 0.5f);  // small upward nudge to start
        }

        // Update wall run
        if (p->wall_running) {
            p->wall_run_timer -= dt;
            p->vy -= phys.wall_run_gravity * dt;

            // End conditions: timer, grounded, stopped moving, detached from wall
            if (p->wall_run_timer <= 0 || p->grounded || !p->moving
                || !touching_wall(p, scene)) {
                p->wall_running = false;
            }
        }

        // ── Ground detection ────────────────────────────────────────
        float ground_y = 0.0f;
        float eye_height = p->sliding ? phys.slide_eye_height : phys.eye_height;
        for (int i = 0; i < scene->wall_count; i++) {
            Wall *w = &scene->walls[i];
            if (!w->active) continue;
            float top, hx, hz;
            switch (w->shape) {
                case SHAPE_CUBE:
                    top = w->pos.y + w->size.y / 2;
                    hx = w->size.x / 2 + 0.3f;
                    hz = w->size.z / 2 + 0.3f;
                    if (top < new_pos.y && top > ground_y &&
                        top <= p->ground_y + 0.6f &&
                        new_pos.x > w->pos.x - hx && new_pos.x < w->pos.x + hx &&
                        new_pos.z > w->pos.z - hz && new_pos.z < w->pos.z + hz) {
                        ground_y = top;
                    }
                    break;
                case SHAPE_CYLINDER: {
                    // Cylinder: size.x = diameter, size.y = height
                    top = w->pos.y + w->size.y / 2;
                    float cr = w->size.x / 2 + 0.3f;
                    float cdx = new_pos.x - w->pos.x;
                    float cdz = new_pos.z - w->pos.z;
                    if (top < new_pos.y && top > ground_y &&
                        top <= p->ground_y + 0.6f &&
                        cdx*cdx + cdz*cdz < cr*cr) {
                        ground_y = top;
                    }
                    break;
                }
                case SHAPE_SPHERE: {
                    // Sphere top: center.y + radius (size.y/2 for uniform, approximate as AABB)
                    top = w->pos.y + w->size.y / 2;
                    float sr = w->size.x / 2 + 0.3f;
                    float sdx = new_pos.x - w->pos.x;
                    float sdz = new_pos.z - w->pos.z;
                    if (top < new_pos.y && top > ground_y &&
                        top <= p->ground_y + 0.6f &&
                        sdx*sdx + sdz*sdz < sr*sr) {
                        ground_y = top;
                    }
                    break;
                }
                default:
                    break;
            }
        }

        float gs = (ground_y > p->ground_y) ? phys.step_up_speed : 8.0f;
        p->ground_y += (ground_y - p->ground_y) * fminf(1.0f, gs * dt);
        float base_y = p->ground_y + eye_height;

        // ── Coyote time ─────────────────────────────────────────────
        if (p->grounded) {
            p->coyote_timer = phys.coyote_time;
        } else {
            p->coyote_timer -= dt;
            if (p->coyote_timer < 0) p->coyote_timer = 0;
        }

        // ── Jump buffer ─────────────────────────────────────────────
        if (IsKeyPressed(KEY_SPACE)) {
            p->jump_buffer = phys.jump_buffer_time;
        } else {
            p->jump_buffer -= dt;
            if (p->jump_buffer < 0) p->jump_buffer = 0;
        }

        // ── Jump ────────────────────────────────────────────────────
        bool can_jump = p->grounded || p->coyote_timer > 0;
        bool wants_jump = p->jump_buffer > 0;

        if (p->wall_running && wants_jump) {
            // Wall jump
            p->vy = phys.wall_run_jump_up;
            p->vel.x += p->wall_normal.x * phys.wall_run_jump_out;
            p->vel.z += p->wall_normal.z * phys.wall_run_jump_out;
            p->wall_running = false;
            p->grounded = false;
            p->jump_buffer = 0;
            just_jumped = true;
        } else if (can_jump && wants_jump) {
            float impulse = phys.jump_impulse;
            if (p->sliding) impulse += phys.crouch_jump_bonus;
            p->vy = impulse;
            p->grounded = false;
            p->sliding = false;
            p->coyote_timer = 0;
            p->jump_buffer = 0;
            just_jumped = true;
        }

        // ── Mantle ──────────────────────────────────────────────────
        // Only trigger near apex of jump (|vy| < 2), holding W, not dashing
        if (!p->grounded && !p->wall_running && !p->dashing
            && (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))
            && fabsf(p->vy) < 2.0f) {
            float ledge_y;
            if (find_mantle_ledge(p, scene, flat_fwd, &ledge_y)) {
                p->mantling = true;
                p->mantle_target_y = ledge_y;
                p->mantle_timer = 0;
                p->vy = 0;
                return;
            }
        }

        // ── Gravity ─────────────────────────────────────────────────
        if (!p->wall_running && !p->dashing) {
            p->vy -= phys.gravity * dt;
        }

        // ── Head bob calculation ────────────────────────────────────
        float bob = 0;
        if (p->grounded && p->speed_current > 0.3f) {
            float amp = p->sliding ? phys.bob_slide_amp :
                       (p->sprinting ? phys.bob_sprint_amp : phys.bob_walk_amp);
            bob = sinf(p->bob_timer) * amp *
                  fminf(1.0f, p->speed_current / phys.walk_speed);
        }

        if (p->land_timer > 0) {
            bob -= p->land_timer * phys.land_dip_strength;
            p->land_timer -= dt * phys.land_dip_decay;
            if (p->land_timer < 0) p->land_timer = 0;
        }

        // Speed shake
        if (p->speed_current > phys.speed_shake_threshold) {
            float t = (p->speed_current - phys.speed_shake_threshold) /
                     fmaxf(1.0f, phys.bhop_speed_cap - phys.speed_shake_threshold);
            if (t > 1.0f) t = 1.0f;
            bob += sinf(p->bob_timer * 23.7f) * phys.speed_shake_intensity * t;
        }

        new_pos.y = p->camera.position.y + p->vy * dt;

        // ── Ground check ────────────────────────────────────────────
        if (new_pos.y <= base_y + bob) {
            new_pos.y = base_y + bob;
            bool was_airborne = !p->grounded;

            if (was_airborne && p->vy < -1.0f) {
                p->land_timer = fminf(1.0f, -p->vy / phys.land_dip_max_vy);
                // Auto-slide on landing if holding Ctrl at speed
                if (want_slide && p->speed_current > phys.walk_speed) {
                    p->sliding = true;
                    float hs2 = hspeed(p);
                    p->slide_speed = hs2;
                    if (hs2 > 0.5f) {
                        p->slide_dir = (Vector3){p->vel.x / hs2, 0, p->vel.z / hs2};
                    }
                    p->land_timer *= 0.5f;
                }
            }
            p->vy = 0;
            if (was_airborne) {
                // Start bhop window — set to small positive so the >= 0 check works
                // on the very next frame
                p->bhop_timer = 0.001f;
            }
            p->grounded = true;
        }

        // Bhop timer — counts time since landing
        if (p->grounded && !just_jumped) {
            p->bhop_timer += dt;
        }
        if (just_jumped) {
            p->bhop_timer = -1;  // invalidate until next landing
        }
    } else {
        // Noclip — horizontal vel applied above (lines 476-477), vertical here
        new_pos.y += p->vel.y * dt;
    }

    p->camera.position = new_pos;
    p->camera.target = Vector3Add(new_pos, forward);

    // ── Idle breathing — Kubrick stillness ───────────────────────
    // When nearly still and grounded: subtle sine camera sway
    if (p->speed_current < 0.3f && p->grounded) {
        p->idle_time += dt;
        // Scale inversely with speed — zero when moving
        float idle_scale = 1.0f - (p->speed_current / 0.3f);
        float breath_pitch = sinf(p->idle_time * 0.7f) * 0.001f * idle_scale;
        float breath_yaw = sinf(p->idle_time * 0.5f) * 0.0005f * idle_scale;
        // Apply to camera target (not position — subtle look sway)
        Vector3 look = Vector3Subtract(p->camera.target, p->camera.position);
        look.y += breath_pitch;
        look.x += breath_yaw;
        p->camera.target = Vector3Add(p->camera.position, look);
    } else {
        p->idle_time = 0;
    }

    float tilt_rad = p->tilt_current * (3.14159f / 180.0f);
    p->camera.up = (Vector3){sinf(tilt_rad), cosf(tilt_rad), 0};
}

// 0-1 normalized speed for shader (walk=0, bhop cap=1)
float player_speed_normalized(Player *p) {
    float over = p->speed_current - phys.walk_speed;
    if (over <= 0) return 0;
    float range = phys.bhop_speed_cap - phys.walk_speed;
    if (range <= 0) return 0;
    float t = over / range;
    return t > 1.0f ? 1.0f : t;
}
