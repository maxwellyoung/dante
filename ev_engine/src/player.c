// player.c — Mirror's Edge movement + slide + ground height + noclip debug
#include "player.h"
#include "raymath.h"
#include <math.h>

void init_player(Player *p, Vector3 pos) {
    p->camera.position = pos;
    p->camera.target = (Vector3){pos.x, pos.y, pos.z - 1};
    p->camera.up = (Vector3){0, 1, 0};
    p->camera.fovy = 70.0f;
    p->camera.projection = CAMERA_PERSPECTIVE;
    p->velocity = (Vector3){0, 0, 0};
    p->bob_timer = 0;
    p->moving = false;
    p->sprinting = false;
    p->sprint_stamina = 1.0f;
    p->speed_current = 0;
    p->fov_current = 70.0f;
    p->tilt_current = 0;
    p->kick_pitch = 0;
    p->kick_yaw = 0;
    p->kick_decay = 0;
    p->vy = 0;
    p->grounded = true;
    p->land_timer = 0;
    p->ground_y = 0.0f;
    p->sliding = false;
    p->slide_speed = 0;
    p->noclip = false;
}

void kick_camera(Player *p, float pitch, float yaw) {
    p->kick_pitch = pitch;
    p->kick_yaw = yaw;
    p->kick_decay = 1.0f;
}

void update_player(Player *p, Scene *scene, float dt) {
    Vector2 mouse_delta = GetMouseDelta();
    float yaw = -mouse_delta.x * MOUSE_SENS;
    float pitch = -mouse_delta.y * MOUSE_SENS;

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

    // Sprint
    p->sprinting = IsKeyDown(KEY_LEFT_SHIFT) && p->sprint_stamina > 0.05f;
    if (p->sprinting && p->moving) {
        p->sprint_stamina -= dt * 0.25f;
        if (p->sprint_stamina < 0) p->sprint_stamina = 0;
    } else {
        p->sprint_stamina += dt * 0.15f;
        if (p->sprint_stamina > 1) p->sprint_stamina = 1;
    }

    // Slide — Ctrl while moving and grounded
    bool want_slide = IsKeyDown(KEY_LEFT_CONTROL) && !p->noclip;
    if (want_slide && p->moving && p->grounded) {
        if (!p->sliding) {
            p->sliding = true;
            p->slide_speed = p->speed_current;
        }
    } else if (!want_slide || !p->moving) {
        p->sliding = false;
        p->slide_speed = 0;
    }
    // Landing from jump while holding Ctrl = instant slide
    if (want_slide && p->grounded && !p->sliding && p->land_timer > 0) {
        p->sliding = true;
        p->slide_speed = p->speed_current;
    }

    float target_speed;
    if (p->sliding) {
        target_speed = (p->sprinting ? SPRINT_SPEED : WALK_SPEED) * 1.5f;
    } else {
        target_speed = p->sprinting ? SPRINT_SPEED : WALK_SPEED;
    }

    // Input
    Vector3 flat_forward = {forward.x, 0, forward.z};
    flat_forward = Vector3Normalize(flat_forward);
    Vector3 flat_right = {right.x, 0, right.z};
    flat_right = Vector3Normalize(flat_right);

    Vector3 wish_dir = {0, 0, 0};
    float strafe = 0;
    p->moving = false;
    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) { wish_dir = Vector3Add(wish_dir, p->noclip ? forward : flat_forward); p->moving = true; }
    if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) { wish_dir = Vector3Subtract(wish_dir, p->noclip ? forward : flat_forward); p->moving = true; }
    if (IsKeyDown(KEY_A)) { wish_dir = Vector3Subtract(wish_dir, flat_right); strafe = -1; p->moving = true; }
    if (IsKeyDown(KEY_D)) { wish_dir = Vector3Add(wish_dir, flat_right); strafe = 1; p->moving = true; }
    // Noclip vertical
    if (p->noclip) {
        if (IsKeyDown(KEY_SPACE)) { wish_dir.y += 1; p->moving = true; }
        if (IsKeyDown(KEY_LEFT_CONTROL)) { wish_dir.y -= 1; p->moving = true; }
    }

    // Smooth accel/decel — sliding has halved friction (decel)
    float decel = p->sliding ? 8.0f : 20.0f;
    float accel = p->moving ? (p->sprinting ? 18.0f : 12.0f) : decel;
    float speed_target = p->moving ? target_speed : 0;
    p->speed_current += (speed_target - p->speed_current) * fminf(1.0f, accel * dt);

    Vector3 move = {0, 0, 0};
    if (p->moving && p->speed_current > 0.1f) {
        wish_dir = Vector3Normalize(wish_dir);
        move = Vector3Scale(wish_dir, p->speed_current * dt);
        float bob_rate = p->sliding ? 18.0f : (p->sprinting ? 14.0f : 9.0f);
        p->bob_timer += dt * bob_rate;
    }

    // FOV — sliding is widest
    float target_fov;
    if (p->sliding) {
        target_fov = 88.0f;
    } else if (p->sprinting && p->moving) {
        target_fov = 82.0f;
    } else {
        target_fov = 70.0f;
    }
    p->fov_current += (target_fov - p->fov_current) * fminf(1.0f, 8.0f * dt);
    p->camera.fovy = p->fov_current;

    // Tilt — sliding tilts more
    float tilt_mult = p->sliding ? 3.5f : (p->sprinting ? 2.5f : 1.5f);
    float target_tilt = strafe * tilt_mult;
    if (!p->moving) target_tilt = 0;
    p->tilt_current += (target_tilt - p->tilt_current) * fminf(1.0f, 10.0f * dt);

    Vector3 new_pos = Vector3Add(p->camera.position, move);

    // Collision (skip in noclip)
    if (!p->noclip) {
        for (int i = 0; i < scene->wall_count; i++) {
            Wall *w = &scene->walls[i];
            if (!w->active) continue;
            float pr = 0.45f;
            float wall_top = w->pos.y + w->size.y / 2;
            float wall_bot = w->pos.y - w->size.y / 2;
            // Only collide with walls the player can't step over
            // If the wall top is below the player's feet + step threshold, skip collision
            float feet_y = p->ground_y;
            float step_threshold = 0.5f;  // can step up to 0.5m
            if (wall_top <= feet_y + step_threshold && wall_top > feet_y - 0.1f) {
                // This is a steppable surface, skip horizontal collision
                continue;
            }
            if (new_pos.x > w->pos.x - w->size.x/2 - pr &&
                new_pos.x < w->pos.x + w->size.x/2 + pr &&
                new_pos.z > w->pos.z - w->size.z/2 - pr &&
                new_pos.z < w->pos.z + w->size.z/2 + pr &&
                new_pos.y > wall_bot &&
                new_pos.y < wall_top) {
                Vector3 try_x = {new_pos.x, p->camera.position.y, p->camera.position.z};
                Vector3 try_z = {p->camera.position.x, p->camera.position.y, new_pos.z};
                bool bx = (try_x.x > w->pos.x - w->size.x/2 - pr && try_x.x < w->pos.x + w->size.x/2 + pr &&
                           try_x.z > w->pos.z - w->size.z/2 - pr && try_x.z < w->pos.z + w->size.z/2 + pr);
                bool bz = (try_z.x > w->pos.x - w->size.x/2 - pr && try_z.x < w->pos.x + w->size.x/2 + pr &&
                           try_z.z > w->pos.z - w->size.z/2 - pr && try_z.z < w->pos.z + w->size.z/2 + pr);
                if (bx) new_pos.x = p->camera.position.x;
                if (bz) new_pos.z = p->camera.position.z;
            }
        }

        // Ground height — scan all walls to find the highest surface below feet
        float ground_y = 0.0f;
        float eye_height = p->sliding ? 1.2f : 1.6f;
        for (int i = 0; i < scene->wall_count; i++) {
            Wall *w = &scene->walls[i];
            if (!w->active) continue;
            if (w->shape != SHAPE_CUBE) continue;  // only cubes are walkable
            float top = w->pos.y + w->size.y / 2;
            // Is the player standing on top of this wall?
            // Wall top must be below eye height and above current ground
            if (top < new_pos.y &&       // wall top is below current eye position
                top > ground_y &&         // higher than current known ground
                top <= p->ground_y + 0.6f && // can only step up 0.6m at a time
                new_pos.x > w->pos.x - w->size.x/2 - 0.3f &&
                new_pos.x < w->pos.x + w->size.x/2 + 0.3f &&
                new_pos.z > w->pos.z - w->size.z/2 - 0.3f &&
                new_pos.z < w->pos.z + w->size.z/2 + 0.3f) {
                ground_y = top;
            }
        }
        // Smooth ground_y transition (don't snap, glide)
        float ground_speed = (ground_y > p->ground_y) ? 12.0f : 8.0f;  // faster going up
        p->ground_y += (ground_y - p->ground_y) * fminf(1.0f, ground_speed * dt);

        float base_y = p->ground_y + eye_height;

        // Jump
        if (IsKeyPressed(KEY_SPACE) && p->grounded) {
            p->vy = 5.5f;
            p->grounded = false;
            p->sliding = false;  // jumping cancels slide
        }

        // Gravity
        p->vy -= 18.0f * dt;

        // Head bob (only when grounded and moving)
        float bob = 0;
        if (p->grounded && p->moving) {
            float bob_amp;
            if (p->sliding) {
                bob_amp = 0.10f;  // doubled amplitude while sliding
            } else {
                bob_amp = p->sprinting ? 0.08f : 0.04f;
            }
            bob = sinf(p->bob_timer) * bob_amp * fminf(1.0f, p->speed_current / WALK_SPEED);
        }

        // Landing dip
        if (p->land_timer > 0) {
            bob -= p->land_timer * 0.15f;
            p->land_timer -= dt * 5.0f;
            if (p->land_timer < 0) p->land_timer = 0;
        }

        new_pos.y = p->camera.position.y + p->vy * dt;

        // Ground check
        if (new_pos.y <= base_y + bob) {
            new_pos.y = base_y + bob;
            if (!p->grounded && p->vy < -1.0f) {
                // Landing impact — camera dip proportional to fall speed
                p->land_timer = fminf(1.0f, -p->vy / 8.0f);
            }
            p->vy = 0;
            p->grounded = true;
        }
    }

    p->camera.position = new_pos;
    p->camera.target = Vector3Add(new_pos, forward);

    float tilt_rad = p->tilt_current * 3.14159f / 180.0f;
    p->camera.up = (Vector3){sinf(tilt_rad), cosf(tilt_rad), 0};
}
