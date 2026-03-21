// physics.c — Object physics, grab/carry/throw, breakables, hinge doors
// The wine glass floats away. The pillow drifts. Everything responds.
#include "physics.h"
#include "game_ctx.h"
#include "raymath.h"
#include <math.h>
#include <stdio.h>

static void rotate_xz(float *x, float *z, float angle) {
    float c = cosf(angle), s = sinf(angle);
    float rx = *x * c - *z * s;
    float rz = *x * s + *z * c;
    *x = rx;
    *z = rz;
}

static bool wall_supports_object(const Wall *surface, const Wall *obj, float *out_top) {
    if (!surface->active || surface->no_collide || surface->is_decal) return false;

    float radius = fmaxf(obj->size.x, obj->size.z) * 0.5f;
    float top = surface->pos.y + surface->size.y * 0.5f;
    float x = obj->pos.x;
    float z = obj->pos.z;

    switch (surface->shape) {
        case SHAPE_CUBE:
        case SHAPE_MODEL: {
            float hx = surface->size.x * 0.5f + radius;
            float hz = surface->size.z * 0.5f + radius;
            if (surface->rotation_y != 0.0f) {
                float lx = x - surface->pos.x;
                float lz = z - surface->pos.z;
                rotate_xz(&lx, &lz, -surface->rotation_y * DEG2RAD);
                if (lx < -hx || lx > hx || lz < -hz || lz > hz) return false;
            } else if (x < surface->pos.x - hx || x > surface->pos.x + hx
                       || z < surface->pos.z - hz || z > surface->pos.z + hz) {
                return false;
            }
            break;
        }
        case SHAPE_CYLINDER:
        case SHAPE_CONE:
        case SHAPE_SPHERE:
        case SHAPE_SKYTOWER:
        case SHAPE_TORUS: {
            float r = fmaxf(surface->size.x, surface->size.z) * 0.5f + radius;
            float dx = x - surface->pos.x;
            float dz = z - surface->pos.z;
            if (dx * dx + dz * dz > r * r) return false;
            break;
        }
        default:
            return false;
    }

    *out_top = top;
    return true;
}

static float find_scene_floor_y(Scene *scene, int ignore_index, const Wall *obj, float fallback_floor_y) {
    float best = fallback_floor_y;

    for (int i = 0; i < scene->wall_count; i++) {
        const Wall *surface = &scene->walls[i];
        float top = 0.0f;

        if (i == ignore_index) continue;
        if (!wall_supports_object(surface, obj, &top)) continue;
        if (top > obj->pos.y + 0.25f) continue;
        if (top > best) best = top;
    }

    return best;
}

void physics_init(GrabSystem *grab) {
    grab->state = GRAB_NONE;
    grab->wall_index = -1;
    grab->grab_dist = 0;
    grab->throw_cooldown = 0;
    grab->carry_offset = (Vector3){0, 0, 0};
}

// Surface friction by material
static float material_friction(MaterialType mat) {
    switch (mat) {
        case MAT_CARPET:  return 8.0f;
        case MAT_FABRIC:  return 7.0f;
        case MAT_VELVET:  return 7.0f;
        case MAT_MARBLE:  return 2.0f;
        case MAT_TILE:    return 3.0f;
        case MAT_WOOD:    return 4.0f;
        case MAT_GLASS:   return 1.5f;
        default:          return 4.0f;
    }
}

void physics_update(Scene *scene, Player *player, GrabSystem *grab, float dt) {
    float grav_mult = player->gravity_mult;
    Vector3 pp = player->camera.position;
    float push_range = 1.2f;

    for (int i = 0; i < scene->wall_count; i++) {
        Wall *w = &scene->walls[i];
        if (!w->pushable || !w->active) continue;

        // Skip carried object — handled in grab_input
        if (grab->state == GRAB_CARRYING && grab->wall_index == i) continue;

        if (w->freed) {
            // === FREED OBJECT PHYSICS ===
            // Gravity (reduced/zero in space)
            if (grav_mult > 0.1f) {
                w->push_vy -= PHYS_GRAVITY * grav_mult * dt;
                // Terminal velocity
                if (w->push_vy < -20.0f) w->push_vy = -20.0f;
            } else {
                // Zero-G drift — gentle random tumble
                w->push_vel.x += (sinf(g.total_time * 0.3f + (float)i * 1.7f) * 0.02f) * dt;
                w->push_vel.z += (cosf(g.total_time * 0.4f + (float)i * 2.3f) * 0.02f) * dt;
                w->push_vy += sinf(g.total_time * 0.2f + (float)i) * 0.01f * dt;
            }

            // Apply velocity
            w->pos.x += w->push_vel.x * dt;
            w->pos.z += w->push_vel.z * dt;
            w->pos.y += w->push_vy * dt;

            // Floor collision with bounce
            float floor_y = find_scene_floor_y(scene, i, w, w->size.y * 0.5f);
            if (w->pos.y < floor_y && grav_mult > 0.1f) {
                w->pos.y = floor_y;
                float impact_speed = fabsf(w->push_vy);

                // Impact sound on landing
                if (impact_speed > 1.0f) {
                    float intensity = fminf(1.0f, impact_speed / 8.0f);
                    ImpactType itype = IMPACT_HARD;
                    if (w->material == MAT_GLASS || w->breakable)
                        itype = IMPACT_GLASS;
                    else if (w->material == MAT_CARPET || w->material == MAT_FABRIC || w->material == MAT_VELVET)
                        itype = IMPACT_SOFT;
                    PlayImpactSound(&g.audio, intensity, itype);
                }

                // Breakable check
                if (w->breakable && impact_speed > 4.0f) {
                    w->health -= impact_speed * 0.3f;
                    if (w->health <= 0) {
                        w->active = false;
                        particle_burst(&g.particles, w->pos, EMIT_SPARKS, 12, 1.5f);
                        PlayImpactSound(&g.audio, 1.0f, IMPACT_GLASS);
                        printf("[PHYS] Breakable shattered at (%.1f,%.1f,%.1f)\n",
                               w->pos.x, w->pos.y, w->pos.z);
                    }
                }

                // Bounce (restitution 0.3)
                w->push_vy = impact_speed * 0.3f;
                if (w->push_vy < 0.5f) w->push_vy = 0;

                // Surface friction
                float friction = material_friction(w->material) * dt;
                float speed = sqrtf(w->push_vel.x * w->push_vel.x + w->push_vel.z * w->push_vel.z);
                if (speed > 0.01f) {
                    float new_speed = fmaxf(0, speed - friction);
                    float scale = new_speed / speed;
                    w->push_vel.x *= scale;
                    w->push_vel.z *= scale;
                }
            }

            // Minimal friction in zero-G
            if (grav_mult < 0.5f) {
                float drag = 1.0f - 0.3f * dt;
                w->push_vel.x *= drag;
                w->push_vel.z *= drag;
                w->push_vy *= drag;
            }

            // Wall-wall collision — bounce freed objects off static geometry
            {
                float hw = w->size.x * 0.5f;
                float hd = w->size.z * 0.5f;
                float hh = w->size.y * 0.5f;
                bool collision_sound_played = false;
                for (int iter = 0; iter < 3; iter++) {
                    bool collided = false;
                    for (int j = 0; j < scene->wall_count; j++) {
                    Wall *other = &scene->walls[j];
                    if (j == i || !other->active || other->pushable || other->no_collide || other->is_decal) continue;
                    // Simple AABB overlap
                    float ohw = other->size.x * 0.5f;
                    float ohd = other->size.z * 0.5f;
                    float ohh = other->size.y * 0.5f;
                    float dx = w->pos.x - other->pos.x;
                    float dy = w->pos.y - other->pos.y;
                    float dz = w->pos.z - other->pos.z;
                    float ox = (hw + ohw) - fabsf(dx);
                    float oy = (hh + ohh) - fabsf(dy);
                    float oz = (hd + ohd) - fabsf(dz);
                    if (ox > 0 && oy > 0 && oz > 0) {
                        // Push out along smallest overlap axis
                        if (ox < oy && ox < oz) {
                            float sign = (dx > 0) ? 1.0f : -1.0f;
                            w->pos.x += sign * ox;
                            w->push_vel.x = -w->push_vel.x * 0.4f;
                        } else if (oy < oz) {
                            float sign = (dy > 0) ? 1.0f : -1.0f;
                            w->pos.y += sign * oy;
                            w->push_vy = -w->push_vy * 0.3f;
                        } else {
                            float sign = (dz > 0) ? 1.0f : -1.0f;
                            w->pos.z += sign * oz;
                            w->push_vel.z = -w->push_vel.z * 0.4f;
                        }
                        collided = true;
                        // Impact sound on wall collision
                        float speed = sqrtf(w->push_vel.x * w->push_vel.x +
                                            w->push_vel.z * w->push_vel.z +
                                            w->push_vy * w->push_vy);
                        if (!collision_sound_played && speed > 2.0f) {
                            float intensity = fminf(1.0f, speed / 8.0f);
                            ImpactType itype = (w->material == MAT_GLASS || w->breakable)
                                                ? IMPACT_GLASS : IMPACT_HARD;
                            PlayImpactSound(&g.audio, intensity, itype);
                            collision_sound_played = true;
                        }
                        // Breakable check on wall collision
                        if (w->breakable && speed > 4.0f) {
                            w->health -= speed * 0.3f;
                            if (w->health <= 0) {
                                w->active = false;
                                particle_burst(&g.particles, w->pos, EMIT_SPARKS, 12, 1.5f);
                                PlayImpactSound(&g.audio, 1.0f, IMPACT_GLASS);
                            }
                        }
                        break;
                    }
                }
                    if (!collided) break;
                }
            }
        } else {
            // === SPRING-BACK PHYSICS (original nudge behavior) ===
            // Player push force
            Vector3 diff = {w->pos.x - pp.x, 0, w->pos.z - pp.z};
            float dist = sqrtf(diff.x * diff.x + diff.z * diff.z);
            if (dist < push_range && dist > 0.01f) {
                float force = (push_range - dist) / push_range * 0.5f / fmaxf(w->push_mass, 0.1f);
                w->push_vel.x += (diff.x / dist) * force * dt;
                w->push_vel.z += (diff.z / dist) * force * dt;
            }
            // Spring back toward origin
            float spring_k = 2.0f * fmaxf(0.3f, grav_mult);
            float damp = w->push_damping * fmaxf(0.3f, grav_mult);
            w->push_vel.x += (w->push_origin.x - w->pos.x) * spring_k * dt;
            w->push_vel.z += (w->push_origin.z - w->pos.z) * spring_k * dt;
            w->push_vel.x *= (1.0f - damp * dt);
            w->push_vel.z *= (1.0f - damp * dt);
            w->pos.x += w->push_vel.x * dt;
            w->pos.z += w->push_vel.z * dt;
        }

        // === HINGE DOOR PHYSICS ===
        if (w->hinge) {
            // Spring-damper rotation toward target
            float angle_diff = w->hinge_target - w->hinge_angle;
            w->hinge_vel += angle_diff * 8.0f * dt;
            w->hinge_vel *= (1.0f - 4.0f * dt);
            w->hinge_angle += w->hinge_vel * dt;
            w->rotation_y = w->hinge_angle;

            // Player proximity push — open door when close
            float dx = w->pos.x - pp.x;
            float dz = w->pos.z - pp.z;
            float door_dist = sqrtf(dx * dx + dz * dz);
            if (door_dist < 2.0f) {
                w->hinge_target = w->hinge_angle < 45.0f ? 90.0f : w->hinge_target;
            }
        }
    }

    // Cooldown timer
    if (grab->throw_cooldown > 0) grab->throw_cooldown -= dt;
}

// Raycast: find nearest pushable wall in crosshair
static int raycast_pushable(Scene *scene, Camera3D camera, float max_dist) {
    Vector3 origin = camera.position;
    Vector3 dir = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
    int best = -1;
    float best_dist = max_dist;

    for (int i = 0; i < scene->wall_count; i++) {
        Wall *w = &scene->walls[i];
        if (!w->pushable || !w->active) continue;

        // Simple sphere test against wall center
        Vector3 to_wall = Vector3Subtract(w->pos, origin);
        float along = Vector3DotProduct(to_wall, dir);
        if (along < 0 || along > max_dist) continue;

        Vector3 closest = Vector3Add(origin, Vector3Scale(dir, along));
        float perp_dist = Vector3Distance(closest, w->pos);
        float radius = fmaxf(w->size.x, fmaxf(w->size.y, w->size.z)) * 0.6f;

        if (perp_dist < radius && along < best_dist) {
            best_dist = along;
            best = i;
        }
    }
    return best;
}

void physics_grab_input(Scene *scene, Player *player, GrabSystem *grab, float dt) {
    Camera3D cam = player->camera;
    (void)dt;

    switch (grab->state) {
        case GRAB_NONE: {
            // E key to grab — only if no InteractObject in range
            if (IsKeyPressed(KEY_E) && grab->throw_cooldown <= 0) {
                // Check if any interact object is closer first
                bool interact_closer = false;
                for (int i = 0; i < scene->object_count; i++) {
                    if (!scene->objects[i].active) continue;
                    float d = Vector3Distance(cam.position, scene->objects[i].pos);
                    if (d < 2.0f) { interact_closer = true; break; }
                }
                if (interact_closer) break;

                int hit = raycast_pushable(scene, cam, 2.5f);
                if (hit >= 0) {
                    grab->state = GRAB_CARRYING;
                    grab->wall_index = hit;
                    Wall *w = &scene->walls[hit];
                    w->freed = true;
                    w->no_collide = true;
                    grab->grab_dist = Vector3Distance(cam.position, w->pos);
                    printf("[PHYS] Grabbed wall %d\n", hit);
                }
            }
            break;
        }
        case GRAB_CARRYING: {
            if (grab->wall_index < 0 || grab->wall_index >= scene->wall_count) {
                physics_init(grab);
                break;
            }
            Wall *w = &scene->walls[grab->wall_index];
            if (!w->active) { physics_init(grab); break; }

            // Target position: 1.5m in front of camera
            Vector3 dir = Vector3Normalize(Vector3Subtract(cam.target, cam.position));
            Vector3 target = Vector3Add(cam.position, Vector3Scale(dir, 1.5f));

            // Spring constraint — HL2 floaty feel
            float k = 120.0f;
            float d = 15.0f;
            Vector3 diff = Vector3Subtract(target, w->pos);
            w->push_vel.x += diff.x * k * dt;
            w->push_vel.z += diff.z * k * dt;
            w->push_vy += diff.y * k * dt;
            w->push_vel.x *= (1.0f - d * dt);
            w->push_vel.z *= (1.0f - d * dt);
            w->push_vy *= (1.0f - d * dt);
            w->pos.x += w->push_vel.x * dt;
            w->pos.z += w->push_vel.z * dt;
            w->pos.y += w->push_vy * dt;

            // Throw (left click)
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                float throw_force = 8.0f / fmaxf(w->push_mass, 0.1f);
                throw_force = fminf(throw_force, 25.0f);
                w->push_vel = Vector3Scale(dir, throw_force);
                w->push_vy = dir.y * throw_force + 2.0f;
                w->no_collide = false;
                grab->state = GRAB_NONE;
                grab->wall_index = -1;
                grab->throw_cooldown = 0.3f;
                printf("[PHYS] Threw object\n");
                break;
            }

            // Drop (E while carrying)
            if (IsKeyPressed(KEY_E)) {
                w->push_vel = (Vector3){0, 0, 0};
                w->push_vy = 0;
                w->no_collide = false;
                grab->state = GRAB_NONE;
                grab->wall_index = -1;
                printf("[PHYS] Dropped object\n");
                break;
            }
            break;
        }
        case GRAB_THROWING:
            // Transient state — immediately goes to NONE
            grab->state = GRAB_NONE;
            break;
    }
}
