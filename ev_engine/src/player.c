// player.c — Player movement, collision, camera
#include "player.h"
#include "raymath.h"
#include <math.h>

void init_player(Player *p, Vector3 pos) {
    p->camera.position = pos;
    p->camera.target = (Vector3){pos.x, pos.y, pos.z - 1};
    p->camera.up = (Vector3){0, 1, 0};
    p->camera.fovy = 65.0f;
    p->camera.projection = CAMERA_PERSPECTIVE;
    p->velocity = (Vector3){0, 0, 0};
    p->sprinting = false;
    p->sprint_stamina = 1.0f;
    p->bob_timer = 0;
    p->moving = false;
}

void update_player(Player *p, Scene *scene, float dt) {
    // Mouse look
    Vector2 mouse_delta = GetMouseDelta();
    float yaw = -mouse_delta.x * MOUSE_SENS;
    float pitch = mouse_delta.y * MOUSE_SENS;

    Vector3 forward = Vector3Normalize(Vector3Subtract(p->camera.target, p->camera.position));
    Vector3 right = Vector3Normalize(Vector3CrossProduct(forward, p->camera.up));

    Matrix yaw_mat = MatrixRotateY(yaw);
    forward = Vector3Transform(forward, yaw_mat);
    Vector3 pitch_axis = right;
    float current_pitch = asinf(forward.y);
    float new_pitch = current_pitch + pitch;
    if (new_pitch > 1.2f) pitch = 1.2f - current_pitch;
    if (new_pitch < -1.2f) pitch = -1.2f - current_pitch;
    Matrix pitch_mat = MatrixRotate(pitch_axis, pitch);
    forward = Vector3Transform(forward, pitch_mat);

    p->camera.target = Vector3Add(p->camera.position, forward);

    // Sprint
    p->sprinting = IsKeyDown(KEY_LEFT_SHIFT) && p->sprint_stamina > 0;
    if (p->sprinting) {
        p->sprint_stamina -= dt * 0.5f;
        if (p->sprint_stamina < 0) p->sprint_stamina = 0;
    } else {
        p->sprint_stamina += dt * 0.3f;
        if (p->sprint_stamina > 1) p->sprint_stamina = 1;
    }
    float speed = p->sprinting ? SPRINT_SPEED : MOVE_SPEED;

    // Movement
    Vector3 flat_forward = {forward.x, 0, forward.z};
    flat_forward = Vector3Normalize(flat_forward);
    Vector3 flat_right = {right.x, 0, right.z};
    flat_right = Vector3Normalize(flat_right);

    Vector3 move = {0, 0, 0};
    p->moving = false;
    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) { move = Vector3Add(move, flat_forward); p->moving = true; }
    if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) { move = Vector3Subtract(move, flat_forward); p->moving = true; }
    if (IsKeyDown(KEY_A)) { move = Vector3Add(move, flat_right); p->moving = true; }
    if (IsKeyDown(KEY_D)) { move = Vector3Subtract(move, flat_right); p->moving = true; }

    if (p->moving) {
        move = Vector3Normalize(move);
        move = Vector3Scale(move, speed * dt);
        p->bob_timer += dt * (p->sprinting ? 12 : 8);
    }

    // Collision
    Vector3 new_pos = Vector3Add(p->camera.position, move);
    for (int i = 0; i < scene->wall_count; i++) {
        Wall *w = &scene->walls[i];
        if (!w->active) continue;
        float pr = 0.4f;
        if (new_pos.x > w->pos.x - w->size.x/2 - pr &&
            new_pos.x < w->pos.x + w->size.x/2 + pr &&
            new_pos.z > w->pos.z - w->size.z/2 - pr &&
            new_pos.z < w->pos.z + w->size.z/2 + pr &&
            new_pos.y > w->pos.y - w->size.y/2 &&
            new_pos.y < w->pos.y + w->size.y/2) {
            Vector3 try_x = {new_pos.x, p->camera.position.y, p->camera.position.z};
            Vector3 try_z = {p->camera.position.x, p->camera.position.y, new_pos.z};
            bool block_x = (try_x.x > w->pos.x - w->size.x/2 - pr &&
                           try_x.x < w->pos.x + w->size.x/2 + pr &&
                           try_x.z > w->pos.z - w->size.z/2 - pr &&
                           try_x.z < w->pos.z + w->size.z/2 + pr);
            bool block_z = (try_z.x > w->pos.x - w->size.x/2 - pr &&
                           try_z.x < w->pos.x + w->size.x/2 + pr &&
                           try_z.z > w->pos.z - w->size.z/2 - pr &&
                           try_z.z < w->pos.z + w->size.z/2 + pr);
            if (block_x) new_pos.x = p->camera.position.x;
            if (block_z) new_pos.z = p->camera.position.z;
        }
    }

    // Head bob
    float bob = sinf(p->bob_timer) * 0.06f;
    new_pos.y = 1.6f + (p->moving ? bob : 0);

    p->camera.position = new_pos;
    p->camera.target = Vector3Add(new_pos, forward);
}
