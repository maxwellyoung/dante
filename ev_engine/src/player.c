// player.c — Full movement system
// Quake air strafing, bunny hopping, wall running, ledge mantling,
// crouch jumping, coyote time, jump buffering, momentum slides
#include "player.h"
#include "raymath.h"
#include <math.h>
#include <stddef.h>

static PhysicsConfig phys;

// ── Collision result — tracks which wall we hit ─────────────────────
typedef struct {
    bool hit;
    Vector3 normal;     // outward normal of the wall face we collided with
    Wall *wall;         // which wall
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
    p->peak_speed = 0;
    p->noclip = false;
}

void kick_camera(Player *p, float pitch, float yaw) {
    p->kick_pitch = pitch;
    p->kick_yaw = yaw;
    p->kick_decay = 1.0f;
}

// ── Quake-style acceleration ────────────────────────────────────────
static void accelerate(Vector3 *vel, Vector3 wish_dir, float wish_speed,
                       float accel, float dt) {
    float current_speed = Vector3DotProduct(*vel, wish_dir);
    float add_speed = wish_speed - current_speed;
    if (add_speed <= 0) return;

    float accel_speed = accel * wish_speed * dt;
    if (accel_speed > add_speed) accel_speed = add_speed;

    vel->x += accel_speed * wish_dir.x;
    vel->z += accel_speed * wish_dir.z;
}

// Apply friction — decelerates velocity uniformly
static void apply_friction(Vector3 *vel, float friction, float dt) {
    float speed = sqrtf(vel->x * vel->x + vel->z * vel->z);
    if (speed < 0.1f) {
        vel->x = 0;
        vel->z = 0;
        return;
    }
    float drop = speed * friction * dt;
    float new_speed = speed - drop;
    if (new_speed < 0) new_speed = 0;
    float scale = new_speed / speed;
    vel->x *= scale;
    vel->z *= scale;
}

// ── Collision with wall contact tracking ────────────────────────────
// Returns info about the last wall hit (for wall running)
static CollisionInfo collide_and_slide(Vector3 *new_pos, Vector3 *vel,
                                       Scene *scene, float ground_y, float pr) {
    CollisionInfo info = {false, {0, 0, 0}, NULL};

    for (int i = 0; i < scene->wall_count; i++) {
        Wall *w = &scene->walls[i];
        if (!w->active) continue;

        float wall_top = w->pos.y + w->size.y / 2;
        float wall_bot = w->pos.y - w->size.y / 2;

        // Skip walls the player can step over
        float step_threshold = 0.5f;
        if (wall_top <= ground_y + step_threshold && wall_top > ground_y - 0.1f)
            continue;

        if (new_pos->x > w->pos.x - w->size.x/2 - pr &&
            new_pos->x < w->pos.x + w->size.x/2 + pr &&
            new_pos->z > w->pos.z - w->size.z/2 - pr &&
            new_pos->z < w->pos.z + w->size.z/2 + pr &&
            new_pos->y > wall_bot &&
            new_pos->y < wall_top) {

            float dx_left  = (w->pos.x - w->size.x/2 - pr) - new_pos->x;
            float dx_right = new_pos->x - (w->pos.x + w->size.x/2 + pr);
            float dz_front = (w->pos.z - w->size.z/2 - pr) - new_pos->z;
            float dz_back  = new_pos->z - (w->pos.z + w->size.z/2 + pr);

            float pen_x, pen_z;

            if (fabsf(dx_left) < fabsf(dx_right)) {
                pen_x = dx_left;
            } else {
                pen_x = -dx_right;
            }
            if (fabsf(dz_front) < fabsf(dz_back)) {
                pen_z = dz_front;
            } else {
                pen_z = -dz_back;
            }

            if (fabsf(pen_x) < fabsf(pen_z)) {
                float nx = (pen_x > 0) ? 1.0f : -1.0f;
                new_pos->x += pen_x;
                if (vel->x * nx < 0) vel->x = 0;
                info.hit = true;
                info.normal = (Vector3){nx, 0, 0};
                info.wall = w;
            } else {
                float nz = (pen_z > 0) ? 1.0f : -1.0f;
                new_pos->z += pen_z;
                if (vel->z * nz < 0) vel->z = 0;
                info.hit = true;
                info.normal = (Vector3){0, 0, nz};
                info.wall = w;
            }
        }
    }
    return info;
}

// ── Ledge detection ─────────────────────────────────────────────────
// Check if there's a wall top just above the player that could be mantled
static bool find_mantle_ledge(Player *p, Scene *scene, Vector3 forward,
                              float *out_ledge_y) {
    Vector3 pos = p->camera.position;
    float pr = phys.player_radius;
    // Check slightly ahead of the player
    float check_x = pos.x + forward.x * 0.8f;
    float check_z = pos.z + forward.z * 0.8f;
    float eye_y = pos.y;
    float best_ledge = -999.0f;

    for (int i = 0; i < scene->wall_count; i++) {
        Wall *w = &scene->walls[i];
        if (!w->active) continue;
        if (w->shape != SHAPE_CUBE) continue;

        float wall_top = w->pos.y + w->size.y / 2;

        // Ledge must be above eye but within reach
        if (wall_top > eye_y &&
            wall_top < eye_y + phys.mantle_reach &&
            wall_top > best_ledge &&
            check_x > w->pos.x - w->size.x/2 - pr &&
            check_x < w->pos.x + w->size.x/2 + pr &&
            check_z > w->pos.z - w->size.z/2 - pr &&
            check_z < w->pos.z + w->size.z/2 + pr) {
            best_ledge = wall_top;
        }
    }

    if (best_ledge > -999.0f) {
        *out_ledge_y = best_ledge + phys.eye_height;
        return true;
    }
    return false;
}

// ─────────────────────────────────────────────────────────────────────

void update_player(Player *p, Scene *scene, float dt) {
    // ── Mantle in progress — override everything ────────────────────
    if (p->mantling) {
        p->mantle_timer += dt * phys.mantle_speed;
        float t = p->mantle_timer;
        if (t >= 1.0f) {
            // Mantle complete
            p->mantling = false;
            p->camera.position.y = p->mantle_target_y;
            p->ground_y = p->mantle_target_y - phys.eye_height;
            p->grounded = true;
            p->vy = 0;
            // Push forward onto the ledge
            Vector3 fwd = Vector3Normalize(Vector3Subtract(p->camera.target, p->camera.position));
            Vector3 flat_fwd = {fwd.x, 0, fwd.z};
            flat_fwd = Vector3Normalize(flat_fwd);
            p->vel.x = flat_fwd.x * phys.mantle_forward;
            p->vel.z = flat_fwd.z * phys.mantle_forward;
        } else {
            // Smooth ease-out pull up
            float ease = 1.0f - (1.0f - t) * (1.0f - t);
            float start_y = p->mantle_target_y - (p->mantle_target_y - p->camera.position.y) / fmaxf(0.01f, 1.0f - (p->mantle_timer - dt) * phys.mantle_speed);
            // Simpler: just lerp toward target
            p->camera.position.y += (p->mantle_target_y - p->camera.position.y) * fminf(1.0f, phys.mantle_speed * dt * 3.0f);
            p->vel.x *= 0.9f; // slow horizontal during mantle
            p->vel.z *= 0.9f;
            p->vy = 0;
            (void)ease; (void)start_y;
        }
        p->camera.target = Vector3Add(p->camera.position,
            Vector3Normalize(Vector3Subtract(p->camera.target, p->camera.position)));
        return;
    }

    // ── Mouse look ──────────────────────────────────────────────────
    Vector2 mouse_delta = GetMouseDelta();
    float yaw = -mouse_delta.x * MOUSE_SENS;
    float pitch = -mouse_delta.y * MOUSE_SENS;

    // Camera kick (recoil)
    if (p->kick_decay > 0) {
        float kick_t = p->kick_decay * p->kick_decay;
        pitch += p->kick_pitch * kick_t * dt * 30.0f;
        yaw += p->kick_yaw * kick_t * dt * 30.0f;
        p->kick_decay -= dt * 4.0f;
        if (p->kick_decay < 0) p->kick_decay = 0;
    }

    Vector3 forward = Vector3Normalize(Vector3Subtract(p->camera.target, p->camera.position));
    Vector3 right = Vector3Normalize(Vector3CrossProduct(forward, p->camera.up));

    Matrix yaw_mat = MatrixRotateY(yaw);
    forward = Vector3Transform(forward, yaw_mat);
    float current_pitch = asinf(forward.y);
    float new_pitch = current_pitch + pitch;
    if (new_pitch > 1.3f) pitch = 1.3f - current_pitch;
    if (new_pitch < -1.3f) pitch = -1.3f - current_pitch;
    Matrix pitch_mat = MatrixRotate(right, pitch);
    forward = Vector3Transform(forward, pitch_mat);
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

    // ── Input: build wish direction ─────────────────────────────────
    Vector3 flat_forward = {forward.x, 0, forward.z};
    flat_forward = Vector3Normalize(flat_forward);
    Vector3 flat_right = {right.x, 0, right.z};
    flat_right = Vector3Normalize(flat_right);

    Vector3 wish_dir = {0, 0, 0};
    float strafe = 0;
    p->moving = false;
    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))   { wish_dir = Vector3Add(wish_dir, p->noclip ? forward : flat_forward); p->moving = true; }
    if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))  { wish_dir = Vector3Subtract(wish_dir, p->noclip ? forward : flat_forward); p->moving = true; }
    if (IsKeyDown(KEY_A)) { wish_dir = Vector3Subtract(wish_dir, flat_right); strafe = -1; p->moving = true; }
    if (IsKeyDown(KEY_D)) { wish_dir = Vector3Add(wish_dir, flat_right); strafe = 1; p->moving = true; }

    // Noclip vertical
    if (p->noclip) {
        if (IsKeyDown(KEY_SPACE))        { wish_dir.y += 1; p->moving = true; }
        if (IsKeyDown(KEY_LEFT_CONTROL)) { wish_dir.y -= 1; p->moving = true; }
    }

    float wish_len = Vector3Length(wish_dir);
    if (wish_len > 0.001f) {
        wish_dir = Vector3Scale(wish_dir, 1.0f / wish_len);
    }

    // ── Slide ───────────────────────────────────────────────────────
    bool want_slide = IsKeyDown(KEY_LEFT_CONTROL) && !p->noclip;
    if (want_slide && p->moving && p->grounded) {
        if (!p->sliding) {
            p->sliding = true;
            p->slide_speed = p->speed_current;
            float vlen = sqrtf(p->vel.x * p->vel.x + p->vel.z * p->vel.z);
            if (vlen > 0.5f) {
                p->slide_dir = (Vector3){p->vel.x / vlen, 0, p->vel.z / vlen};
            } else {
                p->slide_dir = wish_dir;
            }
        }
    } else if (!want_slide || !p->moving) {
        p->sliding = false;
        p->slide_speed = 0;
    }
    // Landing from jump while holding Ctrl = instant slide
    if (want_slide && p->grounded && !p->sliding && p->land_timer > 0) {
        p->sliding = true;
        float vlen = sqrtf(p->vel.x * p->vel.x + p->vel.z * p->vel.z);
        p->slide_speed = fmaxf(p->speed_current, vlen);
        if (vlen > 0.5f) {
            p->slide_dir = (Vector3){p->vel.x / vlen, 0, p->vel.z / vlen};
        } else {
            p->slide_dir = wish_dir;
        }
    }

    // ── Determine target speed ──────────────────────────────────────
    float wish_speed;
    if (p->sliding) {
        wish_speed = (p->sprinting ? phys.sprint_speed : phys.walk_speed) * phys.slide_speed_mult;
    } else {
        wish_speed = p->sprinting ? phys.sprint_speed : phys.walk_speed;
    }

    // ── Apply movement physics ──────────────────────────────────────
    if (p->noclip) {
        p->vel = Vector3Scale(wish_dir, wish_speed);
    } else if (p->wall_running) {
        // ── WALL RUN MOVEMENT ───────────────────────────────────────
        // Move along wall, reduced gravity
        Vector3 wall_along = {-p->wall_normal.z, 0, p->wall_normal.x};
        // Pick direction based on which side
        float dot = wall_along.x * p->vel.x + wall_along.z * p->vel.z;
        if (dot < 0) { wall_along.x = -wall_along.x; wall_along.z = -wall_along.z; }
        accelerate(&p->vel, wall_along, wish_speed, phys.ground_accel * 0.8f, dt);
        apply_friction(&p->vel, phys.ground_friction * 0.3f, dt);
    } else if (p->grounded) {
        // ── GROUND MOVEMENT ─────────────────────────────────────────
        // Bunny hop: skip friction if we just landed and are about to jump
        bool skip_friction = (p->bhop_timer > 0 && p->bhop_timer < phys.bhop_friction_window
                              && p->jump_buffer > 0);
        if (!skip_friction) {
            float friction = p->sliding ? phys.slide_friction : phys.ground_friction;
            apply_friction(&p->vel, friction, dt);
        }

        if (p->sliding) {
            accelerate(&p->vel, p->slide_dir, wish_speed, phys.ground_accel * 0.3f, dt);
            if (p->moving) {
                Vector3 steer = wish_dir;
                float dot = steer.x * p->slide_dir.x + steer.z * p->slide_dir.z;
                steer.x -= dot * p->slide_dir.x;
                steer.z -= dot * p->slide_dir.z;
                accelerate(&p->vel, steer, wish_speed * 0.3f, phys.ground_accel * 0.5f, dt);
            }
        } else if (p->moving) {
            accelerate(&p->vel, wish_dir, wish_speed, phys.ground_accel, dt);
        }

        // Bhop speed cap — prevent infinite acceleration
        float hspeed = sqrtf(p->vel.x * p->vel.x + p->vel.z * p->vel.z);
        if (hspeed > phys.bhop_speed_cap) {
            float scale = phys.bhop_speed_cap / hspeed;
            p->vel.x *= scale;
            p->vel.z *= scale;
        }
    } else {
        // ── AIR MOVEMENT ────────────────────────────────────────────
        apply_friction(&p->vel, phys.air_friction, dt);
        if (p->moving) {
            accelerate(&p->vel, wish_dir, phys.air_speed_cap, phys.air_accel, dt);
        }
    }

    // Update speed for camera effects
    p->speed_current = sqrtf(p->vel.x * p->vel.x + p->vel.z * p->vel.z);
    if (p->speed_current > p->peak_speed) p->peak_speed = p->speed_current;
    // Decay peak speed slowly (for smooth feedback)
    p->peak_speed += (p->speed_current - p->peak_speed) * fminf(1.0f, 3.0f * dt);

    // ── Head bob ────────────────────────────────────────────────────
    if ((p->grounded || p->wall_running) && p->speed_current > 0.3f) {
        float bob_rate = p->sliding ? phys.bob_slide_rate :
                        (p->sprinting ? phys.bob_sprint_rate : phys.bob_walk_rate);
        p->bob_timer += dt * bob_rate;
    }

    // ── FOV — speed-reactive ────────────────────────────────────────
    float target_fov;
    if (p->sliding) {
        target_fov = phys.fov_slide;
    } else if (p->sprinting && p->moving) {
        target_fov = phys.fov_sprint;
    } else {
        target_fov = phys.fov_walk;
    }
    // Speed-reactive FOV: widen when going faster than sprint speed
    float speed_over = p->speed_current - phys.sprint_speed;
    if (speed_over > 0) {
        target_fov += speed_over * phys.speed_fov_scale;
        if (target_fov > phys.speed_fov_max) target_fov = phys.speed_fov_max;
    }
    p->fov_current += (target_fov - p->fov_current) * fminf(1.0f, phys.fov_lerp_speed * dt);
    p->camera.fovy = p->fov_current;

    // ── Tilt ────────────────────────────────────────────────────────
    float tilt_mult = p->sliding ? phys.tilt_slide :
                     (p->sprinting ? phys.tilt_sprint : phys.tilt_walk);
    float target_tilt = strafe * tilt_mult;
    // Wall run tilt: lean into the wall
    if (p->wall_running) {
        target_tilt = p->wall_run_side * 12.0f;  // 12° lean
    }
    if (!p->moving && !p->wall_running) target_tilt = 0;
    p->tilt_current += (target_tilt - p->tilt_current) * fminf(1.0f, phys.tilt_lerp_speed * dt);

    // ── Position update ─────────────────────────────────────────────
    Vector3 new_pos = p->camera.position;
    new_pos.x += p->vel.x * dt;
    new_pos.z += p->vel.z * dt;

    // ── Collision ───────────────────────────────────────────────────
    if (!p->noclip) {
        CollisionInfo col = collide_and_slide(&new_pos, &p->vel, scene,
                                              p->ground_y, phys.player_radius);

        // ── Wall running detection ──────────────────────────────────
        if (col.hit && !p->grounded && !p->wall_running && p->speed_current > phys.wall_run_speed) {
            // We're airborne, moving fast, and just hit a wall — start wall run
            // Determine which side the wall is on relative to our movement
            float side = flat_right.x * col.normal.x + flat_right.z * col.normal.z;
            p->wall_running = true;
            p->wall_run_timer = phys.wall_run_duration;
            p->wall_normal = col.normal;
            p->wall_run_side = (side > 0) ? -1.0f : 1.0f;
            p->vy = fmaxf(p->vy, 0);  // stop falling when starting wall run
        }

        // Update wall run state
        if (p->wall_running) {
            p->wall_run_timer -= dt;
            // Reduced gravity on wall
            p->vy -= phys.wall_run_gravity * dt;
            // End wall run conditions
            if (p->wall_run_timer <= 0 || p->grounded || !p->moving) {
                p->wall_running = false;
            }
        }

        // ── Ground detection ────────────────────────────────────────
        float ground_y = 0.0f;
        float eye_height = p->sliding ? phys.slide_eye_height : phys.eye_height;
        for (int i = 0; i < scene->wall_count; i++) {
            Wall *w = &scene->walls[i];
            if (!w->active) continue;
            if (w->shape != SHAPE_CUBE) continue;
            float top = w->pos.y + w->size.y / 2;
            if (top < new_pos.y &&
                top > ground_y &&
                top <= p->ground_y + 0.6f &&
                new_pos.x > w->pos.x - w->size.x/2 - 0.3f &&
                new_pos.x < w->pos.x + w->size.x/2 + 0.3f &&
                new_pos.z > w->pos.z - w->size.z/2 - 0.3f &&
                new_pos.z < w->pos.z + w->size.z/2 + 0.3f) {
                ground_y = top;
            }
        }

        // Smooth ground_y transition
        float ground_speed = (ground_y > p->ground_y) ? phys.step_up_speed : 8.0f;
        p->ground_y += (ground_y - p->ground_y) * fminf(1.0f, ground_speed * dt);

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

        // Wall jump: jump off wall while wall running
        if (p->wall_running && wants_jump) {
            p->vy = phys.wall_run_jump_up;
            // Push away from wall
            p->vel.x += p->wall_normal.x * phys.wall_run_jump_out;
            p->vel.z += p->wall_normal.z * phys.wall_run_jump_out;
            p->wall_running = false;
            p->grounded = false;
            p->jump_buffer = 0;
        } else if (can_jump && wants_jump) {
            float impulse = phys.jump_impulse;
            // Crouch jump bonus: jumping from a slide gives extra height
            if (p->sliding) {
                impulse += phys.crouch_jump_bonus;
            }
            p->vy = impulse;
            p->grounded = false;
            p->sliding = false;
            p->coyote_timer = 0;
            p->jump_buffer = 0;
            p->bhop_timer = 0;  // reset bhop timer on jump
        }

        // ── Mantle check ────────────────────────────────────────────
        // If airborne and moving forward and not already mantling
        if (!p->grounded && !p->wall_running && p->moving && p->vy < 1.0f) {
            float ledge_y;
            Vector3 flat_fwd = {forward.x, 0, forward.z};
            flat_fwd = Vector3Normalize(flat_fwd);
            if (find_mantle_ledge(p, scene, flat_fwd, &ledge_y)) {
                p->mantling = true;
                p->mantle_target_y = ledge_y;
                p->mantle_timer = 0;
                p->vy = 0;
                return;  // mantle takes over next frame
            }
        }

        // ── Gravity ─────────────────────────────────────────────────
        if (!p->wall_running) {
            p->vy -= phys.gravity * dt;
        }

        // ── Head bob calculation ────────────────────────────────────
        float bob = 0;
        if (p->grounded && p->speed_current > 0.3f) {
            float bob_amp = p->sliding ? phys.bob_slide_amp :
                           (p->sprinting ? phys.bob_sprint_amp : phys.bob_walk_amp);
            bob = sinf(p->bob_timer) * bob_amp *
                  fminf(1.0f, p->speed_current / phys.walk_speed);
        }

        // Landing dip
        if (p->land_timer > 0) {
            bob -= p->land_timer * phys.land_dip_strength;
            p->land_timer -= dt * phys.land_dip_decay;
            if (p->land_timer < 0) p->land_timer = 0;
        }

        // ── Speed shake ─────────────────────────────────────────────
        if (p->speed_current > phys.speed_shake_threshold) {
            float shake_t = (p->speed_current - phys.speed_shake_threshold) /
                           (phys.bhop_speed_cap - phys.speed_shake_threshold);
            if (shake_t > 1.0f) shake_t = 1.0f;
            float shake = sinf(p->bob_timer * 23.7f) * phys.speed_shake_intensity * shake_t;
            bob += shake;
        }

        new_pos.y = p->camera.position.y + p->vy * dt;

        // ── Ground check ────────────────────────────────────────────
        if (new_pos.y <= base_y + bob) {
            new_pos.y = base_y + bob;
            if (!p->grounded && p->vy < -1.0f) {
                // Landing impact
                p->land_timer = fminf(1.0f, -p->vy / phys.land_dip_max_vy);
                // Landing slide: if holding Ctrl and moving fast, convert to slide
                if (want_slide && p->speed_current > phys.walk_speed) {
                    p->sliding = true;
                    float vlen = sqrtf(p->vel.x * p->vel.x + p->vel.z * p->vel.z);
                    p->slide_speed = vlen;
                    if (vlen > 0.5f) {
                        p->slide_dir = (Vector3){p->vel.x / vlen, 0, p->vel.z / vlen};
                    }
                    p->land_timer *= 0.5f;  // half dip when rolling into slide
                }
            }
            p->vy = 0;
            if (!p->grounded) {
                p->bhop_timer = 0;  // start bhop window
            }
            p->grounded = true;
        }

        // Update bhop timer
        if (p->grounded) {
            p->bhop_timer += dt;
        }
    } else {
        // Noclip
        new_pos = Vector3Add(new_pos, Vector3Scale(p->vel, dt));
    }

    p->camera.position = new_pos;
    p->camera.target = Vector3Add(new_pos, forward);

    float tilt_rad = p->tilt_current * 3.14159f / 180.0f;
    p->camera.up = (Vector3){sinf(tilt_rad), cosf(tilt_rad), 0};
}
