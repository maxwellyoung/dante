// player.c — Mirror's Edge movement + noclip debug
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
    float target_speed = p->sprinting ? SPRINT_SPEED : WALK_SPEED;

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

    // Smooth accel/decel
    float accel = p->moving ? (p->sprinting ? 18.0f : 12.0f) : 20.0f;
    float speed_target = p->moving ? target_speed : 0;
    p->speed_current += (speed_target - p->speed_current) * fminf(1.0f, accel * dt);

    Vector3 move = {0, 0, 0};
    if (p->moving && p->speed_current > 0.1f) {
        wish_dir = Vector3Normalize(wish_dir);
        move = Vector3Scale(wish_dir, p->speed_current * dt);
        p->bob_timer += dt * (p->sprinting ? 14 : 9);
    }

    // FOV
    float target_fov = p->sprinting && p->moving ? 82.0f : 70.0f;
    p->fov_current += (target_fov - p->fov_current) * fminf(1.0f, 8.0f * dt);
    p->camera.fovy = p->fov_current;

    // Tilt
    float target_tilt = strafe * (p->sprinting ? 2.5f : 1.5f);
    if (!p->moving) target_tilt = 0;
    p->tilt_current += (target_tilt - p->tilt_current) * fminf(1.0f, 10.0f * dt);

    Vector3 new_pos = Vector3Add(p->camera.position, move);

    // Collision (skip in noclip)
    if (!p->noclip) {
        for (int i = 0; i < scene->wall_count; i++) {
            Wall *w = &scene->walls[i];
            if (!w->active) continue;
            float pr = 0.35f;
            if (new_pos.x > w->pos.x - w->size.x/2 - pr &&
                new_pos.x < w->pos.x + w->size.x/2 + pr &&
                new_pos.z > w->pos.z - w->size.z/2 - pr &&
                new_pos.z < w->pos.z + w->size.z/2 + pr &&
                new_pos.y > w->pos.y - w->size.y/2 &&
                new_pos.y < w->pos.y + w->size.y/2) {
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

        // Jump
        if (IsKeyPressed(KEY_SPACE) && p->grounded) {
            p->vy = 5.5f;
            p->grounded = false;
        }

        // Gravity
        p->vy -= 18.0f * dt;
        float base_y = 1.6f;

        // Head bob (only when grounded and moving)
        float bob = 0;
        if (p->grounded && p->moving) {
            float bob_amp = p->sprinting ? 0.08f : 0.04f;
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
